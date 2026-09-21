#include "CDMSingle.hpp"

#include <thread>

#include "src/screen/CDMScreen.h"
#include "src/models/Delay.h"
#include "src/net/SFTP.h"
#include "third_party/pugixml/pugixml.cpp"
#include "src/api/CurlRestClient.h"
#include "third_party/pugixml/pugixml.hpp"
#include "src/core/CDMGlobals.hpp"

extern "C" IMAGE_DOS_HEADER __ImageBase;

using namespace std;
using namespace EuroScopePlugIn;
using namespace pugi;

// Tag-item rendering for EuroScope tag columns (OnGetTagItem). This is the last remaining piece
// of the original monolithic CDMSingle.cpp; all other functionality was split into src/core/*.cpp.

void CDM::OnGetTagItem(CFlightPlan FlightPlan, CRadarTarget RadarTarget, int ItemCode, int TagData,
                       char sItemString[16], int* pColorCode, COLORREF* pRGB, double* pFontSize) {
    try {
        if (ItemCode == TAG_ITEM_TSAT || ItemCode == TAG_ITEM_TSAT_TOBT_DIFF) {
            // Check master status
            if (myAtcCallsign != ControllerMyself().GetCallsign()) {
                if (myAtcCallsign == "") {
                    myAtcCallsign = ControllerMyself().GetCallsign();
                } else {
                    RemoveMasterAirports();
                }
            }
        }

        COLORREF ItemRGB = 0xFFFFFFFF;
        string callsign = FlightPlan.GetCallsign();

        // Serve from cache if this callsign+column was already computed this second - see
        // tagItemDisplayCache declaration for why. Falls through to the full computation below on a
        // miss, which then refreshes the cache at the end of the function.
        time_t tagCacheNow = std::time(nullptr);
        auto& tagCacheForAircraft = tagItemDisplayCache[callsign];
        auto tagCacheIt = tagCacheForAircraft.find(ItemCode);
        if (tagCacheIt != tagCacheForAircraft.end() && tagCacheIt->second.tick == tagCacheNow) {
            memcpy(sItemString, tagCacheIt->second.text, sizeof(tagCacheIt->second.text));
            if (tagCacheIt->second.colorSet) {
                *pColorCode = tagCacheIt->second.colorCode;
                *pRGB = tagCacheIt->second.rgb;
            }
            return;
        }

        string origin = FlightPlan.GetFlightPlanData().GetOrigin();
        boost::to_upper(origin);
        string destination = FlightPlan.GetFlightPlanData().GetDestination();
        boost::to_upper(destination);

        string depRwy = FlightPlan.GetFlightPlanData().GetDepartureRwy();
        boost::to_upper(depRwy);
        bool isVfr = false;
        if (strcmp(FlightPlan.GetFlightPlanData().GetPlanType(), "V") > -1) {
            isVfr = true;
        }

        if (ItemCode == TAG_ITEM_OPTIONS) {
            ItemRGB = TAG_GREY;
            strcpy_s(sItemString, 16, "->");
        }

        std::vector<std::vector<std::string>> myNetworkStatus;
        {
            std::lock_guard<std::mutex> lock(networkStatusMutex);
            myNetworkStatus = networkStatus;
        }

        if (!isVfr) {
            time_t timeNow = std::time(nullptr);
            // Refresh <refreshTime> min timers
            if ((timeNow - countNetworkTobt) > refreshTime) {
                countNetworkTobt = timeNow;
                runDetachedTask(&CDM::getNetworkTobt);
                if (debugMode) {
                    sendMessage("[DEBUG MESSAGE] - REFRESHING FLOW DATA");
                }
            }
            if ((timeNow - countFetchServerTime) > 15 && !refresh3) {
                refresh3 = true;
                countFetchServerTime = timeNow;
                runDetachedTask(&CDM::refreshActions3);
                if (debugMode) {
                    sendMessage("[DEBUG MESSAGE] - REFRESHING CDM API DATA 1");
                }
            }
            if ((timeNow - countRefreshActions4Time) > 10 && !refresh4) {
                refresh4 = true;
                countRefreshActions4Time = timeNow;
                runDetachedTask(&CDM::refreshActions4);
                if (debugMode) {
                    sendMessage("[DEBUG MESSAGE] - REFRESHING CDM API DATA 2");
                }
            }

            bool isCDMairport = false;
            for (string a : CDMairports) {
                if (origin == a) {
                    isCDMairport = true;
                }
            }

            if (isCDMairport) {
                // If aircraft is in aircraftFind Base vector
                int pos = -1;
                bool aircraftFind = false;
                for (size_t i = 0; i < slotList.size(); i++) {
                    if (callsign == slotList[i].callsign) {
                        aircraftFind = true;
                        pos = i;
                    }
                }
                const char* EOBT = "";
                const char* TSAT = "";
                const char* TTOT = "";
                int taxiTime = defTaxiTime;

                // Check if update in the queue
                std::vector<Plane> localPlaneQueue;
                {
                    std::lock_guard<std::mutex> lock(apiQueueResponseMutex);
                    localPlaneQueue.swap(apiQueueResponse);
                    apiQueueResponse.clear();
                }

                if (!localPlaneQueue.empty()) {
                    bool ctotUpdated = false;
                    for (const Plane p : localPlaneQueue) {
                        for (int t = 0; t < slotList.size(); t++) {
                            if (p.callsign == slotList[t].callsign) {
                                if (slotList[t].ctot != p.ctot) {
                                    ctotUpdated = true;
                                }
                                slotList[t] = p;
                            }
                        }
                    }

                    if (ctotUpdated) {
                        countTime = std::time(nullptr) - refreshTime;
                    }
                }

                // Get Taxi times
                int TaxiTimePos = 0;
                bool planeHasTaxiTimeAssigned = false;
                for (size_t j = 0; j < taxiTimesList.size(); j++) {
                    if (taxiTimesList[j].substr(0, taxiTimesList[j].find(",")) == callsign) {
                        planeHasTaxiTimeAssigned = true;
                        TaxiTimePos = j;
                    }
                }

                // Check if runway changed
                if (aircraftFind && (ItemCode == TAG_ITEM_TSAT || ItemCode == TAG_ITEM_TSAT_TOBT_DIFF)) {
                    if (planeHasTaxiTimeAssigned) {
                        if (taxiTimesList[TaxiTimePos].substr(taxiTimesList[TaxiTimePos].find(",") + 3, 1) == ",") {
                            if (depRwy !=
                                taxiTimesList[TaxiTimePos].substr(taxiTimesList[TaxiTimePos].find(",") + 1, 2)) {
                                planeHasTaxiTimeAssigned = false;
                                taxiTimesList.erase(taxiTimesList.begin() + TaxiTimePos);
                                slotList.erase(slotList.begin() + pos);
                                aircraftFind = false;
                            }
                        } else if (taxiTimesList[TaxiTimePos].substr(taxiTimesList[TaxiTimePos].find(",") + 4, 1) ==
                                   ",") {
                            if (depRwy !=
                                taxiTimesList[TaxiTimePos].substr(taxiTimesList[TaxiTimePos].find(",") + 1, 3)) {
                                planeHasTaxiTimeAssigned = false;
                                taxiTimesList.erase(taxiTimesList.begin() + TaxiTimePos);
                            }
                        }
                    }
                }

                if (!planeHasTaxiTimeAssigned) {
                    if (RadarTargetSelect(callsign.c_str()).IsValid() && depRwy.length() > 0) {
                        double lat = RadarTargetSelect(callsign.c_str()).GetPosition().GetPosition().m_Latitude;
                        double lon = RadarTargetSelect(callsign.c_str()).GetPosition().GetPosition().m_Longitude;
                        if (debugMode) {
                            sendMessage("[DEBUG MESSAGE] - " + callsign + " LAT: " + to_string(lat) +
                                        " LON: " + to_string(lon) + " DEP RWY: " + depRwy);
                        }
                        int deIceTime = addDeIceTime(callsign, FlightPlan.GetFlightPlanData().GetAircraftWtc());
                        string myTaxiTime = getTaxiTime(lat, lon, origin, depRwy, deIceTime, callsign);
                        taxiTimesList.push_back(callsign + "," + depRwy + "," + myTaxiTime);
                        planeHasTaxiTimeAssigned = true;
                        TaxiTimePos = taxiTimesList.size() - 1;
                    }
                }

                if (planeHasTaxiTimeAssigned) {
                    if (taxiTimesList[TaxiTimePos].substr(taxiTimesList[TaxiTimePos].length() - 2, 1) == ",") {
                        taxiTime = stoi(taxiTimesList[TaxiTimePos].substr(taxiTimesList[TaxiTimePos].length() - 1, 1));
                    } else {
                        taxiTime = stoi(taxiTimesList[TaxiTimePos].substr(taxiTimesList[TaxiTimePos].length() - 2, 2));
                    }
                }

                // Check if update in the queue
                std::vector<vector<string>> localTobtTypesQueue;
                {
                    std::lock_guard<std::mutex> lock(reqTobtTypesQueueMutex);
                    localTobtTypesQueue.swap(reqTobtTypesQueue);
                    reqTobtTypesQueue.clear();
                }

                if (!localTobtTypesQueue.empty()) {
                    bool found = false;
                    for (vector<string> s : localTobtTypesQueue) {
                        CFlightPlan fp1 = FlightPlanSelect(s[0].c_str());
                        string prevSetBy = getFlightStripInfo(fp1, 9);
                        string setBy = s[1];
                        if (setBy.length() > 1) setBy = setBy.substr(0, 1);
                        if (prevSetBy != setBy) {
                            setFlightStripInfo(fp1, setBy, 9);
                        }
                    }
                }

                if (ctotCid) {
                    bool evCtotFound = false;
                    for (size_t i = 0; i < evCtots.size(); i++) {
                        if (evCtots[i][0] == callsign) {
                            evCtotFound = true;
                        }
                    }
                    if (!evCtotFound) {
                        evCtots.push_back({callsign, ""});
                        runDetachedTask(&CDM::setEvCtot, callsign);
                    }
                }

                bool isValidToCalculateEventMode = false;
                string tobt = getFlightStripInfo(FlightPlan, 2);

                // If realMode is activated, then it will set TOBT from the EOBT auromatically
                if (realMode) {
                    isValidToCalculateEventMode = true;
                }

                // If relaMode is NOT activated, then it'll wait to press the READY TOBT Function to activate the
                // variable "isValidToCalculateEventMode"
                if (tobt.length() == 4) {
                    isValidToCalculateEventMode = true;
                }

                if (!isValidToCalculateEventMode) {
                    for (size_t i = 0; i < disconnectionList.size(); i++) {
                        if (disconnectionList[i] == callsign) {
                            disconnectionList.erase(disconnectionList.begin() + i);
                            if (aircraftFind) {
                                setFlightStripInfo(FlightPlan, formatTime(slotList[pos].eobt), 2);
                            }
                            isValidToCalculateEventMode = true;
                        }
                    }
                }

                bool hasManualCtot = false;
                if (aircraftFind) {
                    if (slotList[pos].hasManualCtot || getFlightStripInfo(FlightPlan, 7) == "1") {
                        hasManualCtot = true;
                    }
                } else if (getFlightStripInfo(FlightPlan, 7) == "1") {
                    hasManualCtot = true;
                }

                // It'll calculate pilot's times after pressing READY TOBT Function
                if (isValidToCalculateEventMode) {
                    // EOBT
                    EOBT = FlightPlan.GetFlightPlanData().GetEstimatedDepartureTime();
                    string EOBTstring = EOBT;
                    string EOBTfinal = formatTime(EOBTstring);

                    if (tobt.length() == 4) {
                        EOBTfinal = tobt;
                    }

                    EOBTfinal += "00";
                    EOBT = EOBTfinal.c_str();
                    bool stillOutOfTsat = false;
                    int stillOutOfTsatPos;

                    // Get Time NOW
                    time_t rawtime;
                    struct tm ptm;
                    time(&rawtime);
                    gmtime_s(&ptm, &rawtime);
                    string hour = to_string(ptm.tm_hour % 24);
                    string min = to_string(ptm.tm_min);

                    if (stoi(min) < 10) {
                        min = "0" + min;
                    }
                    if (stoi(hour) < 10) {
                        hour = "0" + hour.substr(0, 1);
                    }

                    bool stsDepa = false;
                    if ((string)FlightPlan.GetGroundState() == "DEPA") {
                        stsDepa = true;
                        setFlightStripInfo(FlightPlan, "", 3);
                        setFlightStripInfo(FlightPlan, "", 4);
                    }

                    // Get airport
                    bool aptFind = false;
                    for (size_t i = 0; i < planeAiportList.size(); i++) {
                        if (planeAiportList[i].substr(0, planeAiportList[i].find(",")) == callsign) {
                            aptFind = true;
                            if (planeAiportList[i].substr(planeAiportList[i].find(",") + 1, 4) != origin) {
                                planeAiportList[i] = callsign + "," + origin;
                            }
                        }
                    }

                    if (!aptFind) {
                        planeAiportList.push_back(callsign + "," + origin);
                    }

                    bool master = false;
                    for (string apt : masterAirports) {
                        if (apt == origin) {
                            master = true;
                        }
                    }

                    bool SU_ISSET = false;
                    if ((string)FlightPlan.GetGroundState() == "STUP" ||
                        (string)FlightPlan.GetGroundState() == "ST-UP" ||
                        (string)FlightPlan.GetGroundState() == "PUSH" ||
                        (string)FlightPlan.GetGroundState() == "TAXI" ||
                        (string)FlightPlan.GetGroundState() == "DEPA") {
                        SU_ISSET = true;
                    }

                    if (master) {
                        if (realMode) {
                            if (tobt.length() > 0 == false) {
                                string mySetEobt =
                                    formatTime(FlightPlan.GetFlightPlanData().GetEstimatedDepartureTime());
                                setFlightStripInfo(FlightPlan, mySetEobt, 2);
                            }
                        }

                        // Atot check
                        if (atotEnabled) {
                            if ((string)FlightPlan.GetGroundState() == "DEPA") {
                                bool atotFound = false;
                                for (size_t i = 0; i < atotSet.size(); i++) {
                                    if (callsign == atotSet[i]) {
                                        atotFound = true;
                                    }
                                }
                                if (!atotFound) {
                                    atotSet.push_back(callsign);
                                    // set TTOT to now
                                    if (aircraftFind) {
                                        slotList[pos].ttot = GetTimeNow();
                                    }
                                }
                            }
                        }

                        bool gndStatusSet = false;
                        if ((string)FlightPlan.GetGroundState() == "STUP" ||
                            (string)FlightPlan.GetGroundState() == "ST-UP" ||
                            (string)FlightPlan.GetGroundState() == "PUSH" ||
                            (string)FlightPlan.GetGroundState() == "TAXI" ||
                            (string)FlightPlan.GetGroundState() == "DEPA") {
                            gndStatusSet = true;
                            bool aicraftInFinalTimesList = false;
                            for (string aircraft : finalTimesList) {
                                if (aircraft == callsign) {
                                    aicraftInFinalTimesList = true;
                                }
                            }
                            if (!aicraftInFinalTimesList) {
                                finalTimesList.push_back(callsign);
                            }
                        } else {
                            for (size_t i = 0; i < finalTimesList.size(); i++) {
                                if (finalTimesList[i] == callsign) {
                                    finalTimesList.erase(finalTimesList.begin() + i);
                                }
                            }
                        }

                        string outOfTsatString = "";

                        for (size_t i = 0; i < OutOfTsat.size(); i++) {
                            bool networkSuspended = false;
                            if (callsign == OutOfTsat[i][0]) {
                                for (size_t s = 0; s < myNetworkStatus.size(); s++) {
                                    if (myNetworkStatus[s][0] == callsign) {
                                        if (myNetworkStatus[s][1].find("FLS") != string::npos &&
                                            myNetworkStatus[s][1].find("CDM") == string::npos) {
                                            networkSuspended = true;
                                        }
                                    }
                                }

                                if (EOBTfinal == OutOfTsat[i][1] || networkSuspended) {
                                    stillOutOfTsat = true;
                                    stillOutOfTsatPos = i;
                                    outOfTsatString = OutOfTsat[i][2];
                                    if (outOfTsatString.length() > 4) outOfTsatString = outOfTsatString.substr(0, 4);
                                } else {
                                    OutOfTsat.erase(OutOfTsat.begin() + i);
                                }
                            }
                        }

                        if (stillOutOfTsat && !gndStatusSet) {
                            // Remove ACFT Find
                            if (aircraftFind) {
                                PushToOtherControllers(FlightPlan);
                                if (aircraftFind) {
                                    slotList.erase(slotList.begin() + pos);
                                }
                            }
                            // Show basic lists with no info, only EOBT, TOBT, ASAT and *TSAC*
                            bool notYetEOBT = false;
                            bool actualTOBT = false;

                            string completeEOBT = (string)EOBT;
                            string EOBThour = completeEOBT.substr(completeEOBT.length() - 6, 2);
                            string EOBTmin = completeEOBT.substr(completeEOBT.length() - 4, 2);

                            if (hour != "00") {
                                if (EOBThour == "00") {
                                    EOBThour = "24";
                                }
                            }

                            // ASAT
                            bool ASATFound = false;
                            int ASATpos = 0;
                            bool correctState = false;
                            string ASATtext = " ";
                            for (size_t x = 0; x < asatList.size(); x++) {
                                string actualListCallsign = asatList[x].substr(0, asatList[x].find(","));
                                if (actualListCallsign == callsign) {
                                    ASATFound = true;
                                    ASATpos = x;
                                }
                            }

                            if ((string)FlightPlan.GetGroundState() == "STUP" ||
                                (string)FlightPlan.GetGroundState() == "ST-UP" ||
                                (string)FlightPlan.GetGroundState() == "PUSH" ||
                                (string)FlightPlan.GetGroundState() == "TAXI" ||
                                (string)FlightPlan.GetGroundState() == "DEPA") {
                                correctState = true;
                            }

                            if (!ASATFound) {
                                if (correctState) {
                                    ASATtext = formatTime(hour + min);
                                    asatList.push_back(callsign + "," + ASATtext.substr(0, 4));
                                    runDetachedTask(&CDM::setCdmSts, callsign, "AOBT/" + ASATtext);
                                    ASATFound = true;
                                }
                            } else {
                                if (correctState) {
                                    ASATtext = asatList[ASATpos].substr(asatList[ASATpos].length() - 4, 4);
                                } else if (!correctState) {
                                    asatList.erase(asatList.begin() + ASATpos);
                                    runDetachedTask(&CDM::setCdmSts, callsign, "AOBT/NULL");
                                    ASATFound = false;
                                }
                            }

                            if (ItemCode == TAG_ITEM_EOBT) {
                                string myeobt = FlightPlan.GetFlightPlanData().GetEstimatedDepartureTime();
                                string ShowEOBT = formatTime(myeobt);
                                ItemRGB = TAG_EOBT;
                                for (size_t i = 0; i < myNetworkStatus.size(); i++) {
                                    if (myNetworkStatus[i][0] == callsign) {
                                        if (myNetworkStatus[i][1].find("FLS") != string::npos) {
                                            ItemRGB = TAG_RED;
                                        }
                                        break;
                                    }
                                }
                                strcpy_s(sItemString, 16, ShowEOBT.c_str());
                            } else if (ItemCode == TAG_ITEM_TOBT) {
                                string ShowEOBT = (string)EOBT;
                                ItemRGB = TAG_RED;
                                strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
                            } else if (ItemCode == TAG_ITEM_ETOBT) {
                                string ShowEOBT = (string)EOBT;
                                ItemRGB = TAG_RED;
                                strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
                            } else if (ItemCode == TAG_ITEM_TSAT) {
                                ItemRGB = TAG_RED;
                                if (isEvSlot(callsign)) {
                                    strcpy_s(sItemString, 16, (outOfTsatString + "E").c_str());
                                } else {
                                    strcpy_s(sItemString, 16, outOfTsatString.c_str());
                                }
                            } else if (ItemCode == TAG_ITEM_TSAC) {
                                ItemRGB = TAG_GREEN;
                                if (SU_ISSET) ItemRGB = SU_SET_COLOR;
                                strcpy_s(sItemString, 16, "____");
                            } else if (ItemCode == TAG_ITEM_TSAC_SIMPLE) {
                                string annotTSAC = getFlightStripInfo(FlightPlan, 1);
                                if (!annotTSAC.empty()) {
                                    ItemRGB = TAG_GREEN;
                                    if (SU_ISSET) ItemRGB = SU_SET_COLOR;
                                    strcpy_s(sItemString, 16, "\xA4");
                                } else {
                                    ItemRGB = TAG_GREEN;
                                    if (SU_ISSET) ItemRGB = SU_SET_COLOR;
                                    strcpy_s(sItemString, 16, "\xAC");
                                }
                            } else if (ItemCode == TAG_ITEM_CTOC) {
                                string annotCTOC = getFlightStripInfo(FlightPlan, 8);
                                if (!annotCTOC.empty()) {
                                    ItemRGB = SU_ISSET ? SU_SET_COLOR : TAG_ORANGE;
                                    strcpy_s(sItemString, 16, annotCTOC.c_str());
                                } else {
                                    ItemRGB = TAG_GREEN;
                                    if (SU_ISSET) ItemRGB = SU_SET_COLOR;
                                    strcpy_s(sItemString, 16, "____");
                                }
                            } else if (ItemCode == TAG_ITEM_CTOC_SIMPLE) {
                                string annotCTOC = getFlightStripInfo(FlightPlan, 8);
                                if (!annotCTOC.empty()) {
                                    ItemRGB = SU_ISSET ? SU_SET_COLOR : TAG_ORANGE;
                                    strcpy_s(sItemString, 16, "X");
                                }
                            } else if (ItemCode == TAG_ITEM_ASAT) {
                                ItemRGB = TAG_GREEN;
                                if (ASATFound)
                                    strcpy_s(sItemString, 16, ASATtext.c_str());
                                else
                                    strcpy_s(sItemString, 16, " ");
                            } else if (ItemCode == TAG_ITEM_E) {
                                ItemRGB = TAG_RED;
                                strcpy_s(sItemString, 16, "I");
                            } else if (ItemCode == TAG_ITEM_EV_CTOT) {
                                bool inEvCtotsList = false;
                                string slot = "";
                                for (size_t i = 0; i < evCtots.size(); i++) {
                                    if (evCtots[i][0] == callsign) {
                                        inEvCtotsList = true;
                                        slot = evCtots[i][1];
                                    }
                                }
                                if (inEvCtotsList) {
                                    ItemRGB = TAG_GREY;
                                    strcpy_s(sItemString, 16, slot.c_str());
                                }
                            } else if (ItemCode == TAG_ITEM_NETWORK_STATUS) {
                                string status = "";
                                for (size_t i = 0; i < myNetworkStatus.size(); i++) {
                                    if (myNetworkStatus[i][0] == callsign) {
                                        status = myNetworkStatus[i][1];
                                    }
                                }
                                if (status != "") {
                                    ItemRGB = TAG_YELLOW;
                                    if (status == "REA") {
                                        ItemRGB = TAG_YELLOW;
                                        strcpy_s(sItemString, 16, status.c_str());
                                    } else if (status.find("FLS") != string::npos) {
                                        ItemRGB = TAG_RED;
                                        status = GetTimedStatus(status);
                                        strcpy_s(sItemString, 16, status.c_str());
                                    }
                                }
                            } else if (ItemCode == TAG_ITEM_NETWORK_STATUS_AIRBORNE) {
                                string status = "";
                                for (size_t i = 0; i < myNetworkStatus.size(); i++) {
                                    if (myNetworkStatus[i][0] == callsign) {
                                        status = myNetworkStatus[i][1];
                                    }
                                }
                                if (status != "") {
                                    ItemRGB = TAG_YELLOW;
                                    if (status == "COMPLY") {
                                        ItemRGB = TAG_GREEN;
                                        status = "C";
                                        strcpy_s(sItemString, 16, status.c_str());
                                    } else if (status == "AIRB") {
                                        ItemRGB = TAG_RED;
                                        status = "A";
                                        strcpy_s(sItemString, 16, status.c_str());
                                    }
                                }
                            } else if (ItemCode == TAG_ITEM_ON_TIME_STATUS) {
                                string status = "";
                                for (size_t i = 0; i < onTimeStatus.size(); i++) {
                                    if (onTimeStatus[i][0] == callsign) {
                                        status = onTimeStatus[i][1];
                                    }
                                }
                                if (status != "") {
                                    ItemRGB = TAG_YELLOW;
                                    if (status.find("+") != string::npos) {
                                        ItemRGB = TAG_RED;
                                    } else if (status.find("-") != string::npos) {
                                        ItemRGB = TAG_GREEN;
                                    } else {
                                        status = "";
                                    }
                                    strcpy_s(sItemString, 16, status.c_str());
                                }
                            } else if (ItemCode == TAG_ITEM_SEND_STATUS) {
                                ItemRGB = TAG_RED;
                                for (string flt : messagesSent) {
                                    if (flt == callsign) {
                                        ItemRGB = TAG_GREEN;
                                    }
                                }
                                strcpy_s(sItemString, 16, "SEND");
                            } else if (ItemCode == TAG_ITEM_SEND_STATUS_SHORT) {
                                ItemRGB = TAG_RED;
                                for (string flt : messagesSent) {
                                    if (flt == callsign) {
                                        ItemRGB = TAG_GREEN;
                                    }
                                }
                                strcpy_s(sItemString, 16, "S");
                            } else if (ItemCode == TAG_ITEM_DEICE) {
                                string status = "";
                                for (vector<string> deice : deiceList) {
                                    if (deice[0] == callsign) {
                                        status = deice[1];
                                    }
                                }
                                ItemRGB = TAG_YELLOW;
                                strcpy_s(sItemString, 16, status.c_str());
                            } else if (ItemCode == TAG_ITEM_TOBT_SETBY) {
                                string status = getFlightStripInfo(FlightPlan, 9);
                                if (status == "A") status = "ATC";
                                else if (status == "P") status = "PILOT";
                                ItemRGB = TAG_GREEN;
                                strcpy_s(sItemString, 16, status.c_str());
                            } else if (ItemCode == TAG_ITEM_TOBT_SETBY_SHORT) {
                                string status = getFlightStripInfo(FlightPlan, 9);
                                ItemRGB = TAG_GREEN;
                                if (status.length() > 1) {
                                    status = status.substr(0, 1);
                                }
                                strcpy_s(sItemString, 16, status.c_str());
                            }
                        } else {
                            if (addTime) {
                                string myHour = myTimeToAdd.substr(0, 2);
                                if (myHour == "00") {
                                    myHour = "24";
                                }
                                string myMin = myTimeToAdd.substr(2, 2);
                                if (GetdifferenceTime(hour, min, myHour, myMin) >= 0) {
                                    addTime = false;
                                }
                            }

                            string TSATfinal = "";
                            string TTOTFinal = "";
                            bool recalculate = false;
                            if (aircraftFind) {
                                string tempEOBT = EOBT;
                                if (tempEOBT != slotList[pos].eobt) {
                                    // sendMessage(slotList[pos].callsign + " EOBT: " + tempEOBT + " != TOBT: " +
                                    // slotList[pos].eobt);
                                    recalculate = true;
                                    // Update times to slaves
                                    countTime = std::time(nullptr) - refreshTime;
                                } else if (readySetTsac) {
                                    // Update TSAC
                                    string myTsac = getFlightStripInfo(FlightPlan, 1);
                                    if (slotList[pos].showData && myTsac == "9999") {
                                        myTsac = slotList[pos].tsat;
                                        myTsac = (myTsac.length() >= 4) ? myTsac.substr(0, 4) : "";
                                        setFlightStripInfo(FlightPlan, myTsac, 1);
                                    }
                                }
                            }
                            int currentPriority = 0;
                            if (!aircraftFind || recalculate) {
                                // Check if this plane has event CTOTs or reqTobt status
                                // Priority levels: 2=evCtots (highest), 1=reqTobt, 0=none (lowest)
                                if (!aircraftFind) {
                                    currentPriority = 0;
                                    for (const auto& row : evCtots) {
                                        if (row.size() >= 2 && row[0] == callsign && !row[1].empty()) {
                                            currentPriority = 2;
                                            break;
                                        }
                                    }
                                    if (currentPriority == 0) {
                                        for (const auto& cs : reqTobtList) {
                                            if (cs == callsign) {
                                                currentPriority = 1;
                                                break;
                                            }
                                        }
                                    }
                                }

                                TSAT = EOBT;
                                // TSAT
                                string TSATstring = TSAT;
                                TSATfinal = formatTime(TSATstring) + "00";

                                // TTOT
                                TTOTFinal = calculateTime(TSATstring, taxiTime);

                                bool tempAddTime_DELAY_TSAT = false;
                                bool tempAddTime_DELAY_TTOT = false;
                                string myTimeToAddTemp_DELAY = "";
                                for (Delay d : delayList) {
                                    if (d.airport == origin && d.rwy == depRwy) {
                                        if (d.type == "ttot") {
                                            tempAddTime_DELAY_TTOT = true;
                                            myTimeToAddTemp_DELAY = d.time + "00";
                                            break;
                                        } else if (d.type == "tsat") {
                                            tempAddTime_DELAY_TSAT = true;
                                            myTimeToAddTemp_DELAY = d.time + "00";
                                            break;
                                        }
                                    }
                                }

                                if (addTime || tempAddTime_DELAY_TSAT || tempAddTime_DELAY_TTOT) {
                                    // USE DELAY GIVEN TIMES OTHERWISE USE THE DEFAULT DELAY FUNC
                                    if (tempAddTime_DELAY_TSAT || tempAddTime_DELAY_TTOT) {
                                        string timeToUse = myTimeToAddTemp_DELAY;
                                        if (tempAddTime_DELAY_TTOT) {
                                            timeToUse = calculateLessTime(myTimeToAddTemp_DELAY, taxiTime);
                                        }
                                        string timeToAddHour = timeToUse.substr(0, 2);
                                        string timeToAddMin = timeToUse.substr(2, 2);
                                        if (hour != "00") {
                                            if (timeToAddHour == "00") {
                                                timeToAddHour = "24";
                                            }
                                        }

                                        string myTSATHour = TSATfinal.substr(0, 2);
                                        string myTSATMin = TSATfinal.substr(2, 2);
                                        if (hour != "00") {
                                            if (myTSATHour == "00") {
                                                myTSATHour = "24";
                                            }
                                        }

                                        int difTime =
                                            GetdifferenceTime(timeToAddHour, timeToAddMin, myTSATHour, myTSATMin);
                                        bool fixTime = true;
                                        if (hour != timeToAddHour) {
                                            if (difTime > 40) {
                                                fixTime = false;
                                            }
                                        } else {
                                            if (difTime > 0) {
                                                fixTime = false;
                                            }
                                        }

                                        if (!fixTime) {
                                            TSATfinal = timeToUse;
                                            TTOTFinal = calculateTime(timeToUse, taxiTime);
                                        }
                                    } else {
                                        string timeToAddHour = myTimeToAdd.substr(0, 2);
                                        string timeToAddMin = myTimeToAdd.substr(2, 2);
                                        if (hour != "00") {
                                            if (timeToAddHour == "00") {
                                                timeToAddHour = "24";
                                            }
                                        }

                                        string myTSATHour = TSATfinal.substr(0, 2);
                                        string myTSATMin = TSATfinal.substr(2, 2);
                                        if (hour != "00") {
                                            if (myTSATHour == "00") {
                                                myTSATHour = "24";
                                            }
                                        }

                                        int difTime =
                                            GetdifferenceTime(timeToAddHour, timeToAddMin, myTSATHour, myTSATMin);
                                        bool fixTime = true;
                                        if (hour != timeToAddHour) {
                                            if (difTime > 40) {
                                                fixTime = false;
                                            }
                                        } else {
                                            if (difTime > 0) {
                                                fixTime = false;
                                            }
                                        }

                                        if (!fixTime) {
                                            TSATfinal = myTimeToAdd;
                                            TTOTFinal = calculateTime(myTimeToAdd, taxiTime);
                                        }
                                    }
                                }
                                TSAT = TSATfinal.c_str();
                                TTOT = TTOTFinal.c_str();
                            } else {
                                // TSAT
                                string TSATstring = slotList[pos].tsat;
                                TSATfinal = formatTime(TSATstring) + "00";
                                TSAT = TSATfinal.c_str();

                                // TTOT
                                TTOTFinal = slotList[pos].ttot;
                                TTOT = TTOTFinal.c_str();
                            }

                            bool equalTTOT = true;
                            bool correctTTOT = true;
                            bool equalTempoTTOT = true;
                            bool alreadySetTOStd = false;
                            string mySid = FlightPlan.GetFlightPlanData().GetSidName();

                            if (!aircraftFind || recalculate) {
                                // Calculate Rate
                                int rate;

                                Rate dataRate = rateForRunway(origin, depRwy, mySid);
                                if (dataRate.airport == "-1") {
                                    if (!lvo) {
                                        rate = stoi(rateString);
                                    } else {
                                        rate = stoi(lvoRateString);
                                    }
                                } else {
                                    int a = 0;
                                    int dataRatePos = 0;
                                    if (dataRate.rates.size() > 1) {
                                        for (string dr : dataRate.depRwyYes) {
                                            if (dr == depRwy) {
                                                dataRatePos = a;
                                            }
                                            a++;
                                        }
                                    }
                                    if (!lvo) {
                                        rate = stoi(dataRate.rates[dataRatePos]);
                                    } else {
                                        rate = stoi(dataRate.ratesLvo[dataRatePos]);
                                    }
                                }

                                double rateHour = (double)60 / rate;
                                bool sameOrDependantRwys = false;

                                while (equalTTOT) {
                                    correctTTOT = true;
                                    if (!hasManualCtot) {
                                        if (bmiMode) {
                                            TTOTFinal = getCorrectTTOT_Windowed(
                                                TTOTFinal, hasManualCtot, slotList, rate, callsign, origin, depRwy,
                                                GetActualTime() + "00", taxiTime, mySid, false);
                                        } else {
                                            TTOTFinal = getCorrectTTOT(TTOTFinal, hasManualCtot, slotList, rate, callsign,
                                                               origin, depRwy, GetActualTime() + "00", taxiTime, mySid,
                                                               dataRate, sameOrDependantRwys);
                                        }
                                    }

                                    if (correctTTOT) {
                                        equalTTOT = false;
                                        TSATfinal = calculateLessTime(TTOTFinal, taxiTime);
                                        /* START Check stand de-ice */
                                        bool standDeice = false;
                                        for (vector<string> deice : deiceList) {
                                            if (deice[0] == callsign) {
                                                if (deice[1] == "STND") {
                                                    standDeice = true;
                                                }
                                            }
                                        }
                                        if (standDeice) {
                                            int deIceTime =
                                                getDeIceTime(FlightPlan.GetFlightPlanData().GetAircraftWtc(), 0);
                                            TSATfinal = calculateTime(TSATfinal, deIceTime);
                                        }
                                        /* END Check stand de-ice */
                                        TSAT = TSATfinal.c_str();
                                        TTOT = TTOTFinal.c_str();
                                        bool doRequest = false;
                                        if (aircraftFind) {
                                            if (TTOT != slotList[pos].ttot || EOBT != slotList[pos].eobt) {
                                                Plane p(callsign, EOBT, TSAT, TTOT, slotList[pos].ctot,
                                                        slotList[pos].flowReason, hasManualCtot, true, true);
                                                doRequest = true;
                                                slotList[pos] = p;
                                                setFlightStripInfo(FlightPlan, p.tsat, 3);
                                                setFlightStripInfo(FlightPlan, p.ttot, 4);
                                            }
                                        } else {
                                            Plane p(callsign, EOBT, TSAT, TTOT, "", "", hasManualCtot, true, true);
                                            doRequest = true;
                                            slotList.push_back(p);
                                            pos = getPlanePosition(callsign);
                                            setFlightStripInfo(FlightPlan, p.tsat, 3);
                                            setFlightStripInfo(FlightPlan, p.ttot, 4);
                                        }
                                        // Check API
                                        if (doRequest) {
                                            if (serverEnabled) {
                                                string myTSATApi = TSAT;
                                                // Hide calculation
                                                for (size_t a = 0; a < slotList.size(); a++) {
                                                    if (slotList[a].callsign == callsign) {
                                                        slotList[a].showData = false;
                                                    }
                                                }
                                                if (slotList[pos].hasManualCtot && slotList[pos].ctot != "" &&
                                                    slotList[pos].ttot.length() >= 4) {
                                                    string myTTOT = TTOT;
                                                    myTTOT = myTTOT.substr(0, 4);
                                                    if (stoi(myTTOT) > stoi(slotList[pos].ctot) &&
                                                        stoi(myTTOT + "00") <=
                                                            stoi(calculateTime(slotList[pos].ctot + "00", 7))) {
                                                        // Update TOBT API with TSAT if TTOT is greater than CTOT
                                                        // but less or equal to CTOT+7
                                                        string myCOBT =
                                                            calculateLessTime(slotList[pos].ctot + "00", taxiTime);
                                                        runDetachedTask(&CDM::setOBTApi, callsign, myCOBT, true, false);
                                                    } else {
                                                        runDetachedTask(&CDM::setOBTApi, callsign, myTSATApi, true, false);
                                                    }

                                                } else {
                                                    runDetachedTask(&CDM::setOBTApi, callsign, myTSATApi, true, false);
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            if (ItemCode == TAG_ITEM_TSAT || ItemCode == TAG_ITEM_TSAT_TOBT_DIFF) {
                                // Sync TTOT
                                if (aircraftFind) {
                                    if (string(TSAT) != slotList[pos].tsat || string(TTOT) != slotList[pos].ttot) {
                                        setFlightStripInfo(FlightPlan, slotList[pos].tsat, 3);
                                        setFlightStripInfo(FlightPlan, slotList[pos].ttot, 4);
                                    }
                                } else if (aircraftFind && !stsDepa) {
                                    string myTSAT = TSAT;
                                    string myTTOT = TTOT;
                                    setFlightStripInfo(FlightPlan, myTSAT, 3);
                                    setFlightStripInfo(FlightPlan, myTTOT, 4);
                                }

                                // Set ASRT if SU_ISSET
                                if (SU_ISSET) {
                                    string myASRTText = getFlightStripInfo(FlightPlan, 0);
                                    if (myASRTText.empty()) {
                                        // Get Time now
                                        time_t rawtime;
                                        struct tm ptm;
                                        time(&rawtime);
                                        gmtime_s(&ptm, &rawtime);
                                        string hour = to_string(ptm.tm_hour % 24);
                                        string min = to_string(ptm.tm_min);

                                        if (stoi(min) < 10) {
                                            min = "0" + min;
                                        }
                                        if (stoi(hour) < 10) {
                                            hour = "0" + hour.substr(0, 1);
                                        }

                                        setFlightStripInfo(FlightPlan, (hour + min), 0);
                                    }
                                }
                            }

                            string ASRTtext = getFlightStripInfo(FlightPlan, 0);

                            if (ASRTtext != "") {
                                bool found = false;
                                for (string reqTobtCallsign : reqTobtList) {
                                    if (reqTobtCallsign == callsign) {
                                        found = true;
                                        break;
                                    }
                                }
                                if (!found) reqTobtList.push_back(callsign);
                            }

                            // If oldTOBT
                            bool oldTOBT = false;
                            if (ASRTtext.empty()) {
                                string TOBThour = EOBTfinal.substr(0, 2);
                                string TOBTmin = EOBTfinal.substr(2, 2);

                                if (hour != "00") {
                                    if (TOBThour == "00") {
                                        TOBTmin = "24";
                                    }
                                }

                                int difTime = GetdifferenceTime(hour, min, TOBThour, TOBTmin);

                                if (hour != TOBThour) {
                                    if (difTime > 45) {
                                        oldTOBT = true;
                                    }
                                } else {
                                    if (difTime > 5) {
                                        oldTOBT = true;
                                    }
                                }

                                string getTTOT = getFlightStripInfo(FlightPlan, 4);
                                if (oldTOBT && !getTTOT.empty() && invalidateTOBT_Option) {
                                    OutOfTsat.push_back({callsign, EOBT, aircraftFind ? TSAT : ""});
                                    setFlightStripInfo(FlightPlan, "", 0);
                                    setFlightStripInfo(FlightPlan, "", 3);
                                    setFlightStripInfo(FlightPlan, "", 4);
                                    // Update CDM-API
                                    runDetachedTask(&CDM::setCdmSts, callsign, "SUSP");
                                }
                            }

                            // If oldTSAT
                            string TSAThour = TSATfinal.substr(TSATfinal.length() - 6, 2);
                            string TSATmin = TSATfinal.substr(TSATfinal.length() - 4, 2);

                            bool oldTSAT = false;
                            bool moreLessFive = false;
                            bool lastMinute = false;
                            bool firstMinute = false;
                            bool lastMinuteTOBT = false;
                            bool notYetEOBT = false;
                            bool actualTOBT = false;

                            // Hour(LOCAL), Min(LOCAL), Hour(PLANE), Min(PLANE)

                            if (hour != "00") {
                                if (TSAThour == "00") {
                                    TSAThour = "24";
                                }
                            }

                            int difTime = GetdifferenceTime(hour, min, TSAThour, TSATmin);

                            if (hour != TSAThour) {
                                if (difTime == -45) {
                                    firstMinute = true;
                                } else if (difTime >= 44 && difTime <= 45) {
                                    lastMinute = true;
                                } else if (difTime >= -45 && difTime <= 45) {
                                    moreLessFive = true;
                                } else if (difTime > 45) {
                                    oldTSAT = true;
                                }
                            } else {
                                if (difTime == -5) {
                                    firstMinute = true;
                                    moreLessFive = true;
                                } else if (difTime > 5) {
                                    oldTSAT = true;
                                } else if (difTime >= 4 && difTime <= 5) {
                                    lastMinute = true;
                                } else if (difTime >= -5 && difTime <= 5) {
                                    moreLessFive = true;
                                }
                            }

                            //Check TTOT difTime from now to TTOT and if TOBT+10min > now then oldTTOT flag to true
                            bool oldTTOT = false;
                            if (TTOTFinal.length() >= 4) {
                                string TTOTHour = TTOTFinal.substr(0, 2);
                                string TTOTmin = TTOTFinal.substr(2, 2);
                                int difTTOTTime = GetdifferenceTime(hour, min, TTOTHour, TTOTmin);
                                if (hour != TTOTHour) {
                                    if (difTTOTTime > 50) {
                                        oldTTOT = true;
                                    }
                                } else {
                                    if (difTTOTTime > 10) {
                                        oldTTOT = true;
                                    }
                                }
                            }

                            bool correctState = false;
                            if ((string)FlightPlan.GetGroundState() == "STUP" ||
                                (string)FlightPlan.GetGroundState() == "ST-UP" ||
                                (string)FlightPlan.GetGroundState() == "PUSH" ||
                                (string)FlightPlan.GetGroundState() == "TAXI" ||
                                (string)FlightPlan.GetGroundState() == "DEPA") {
                                correctState = true;
                            }

                            string getTTOT = getFlightStripInfo(FlightPlan, 4);
                            if (oldTSAT && !correctState && (!oldTOBT || !invalidateTOBT_Option) &&
                                invalidateTSAT_Option && !getTTOT.empty() &&
                                ((invalidateTSAT_Option_asrt && ASRTtext.empty()) || (!invalidateTSAT_Option_asrt))) {
                                OutOfTsat.push_back({callsign, EOBT, aircraftFind ? TSAT : ""});
                                setFlightStripInfo(FlightPlan, "", 0);
                                setFlightStripInfo(FlightPlan, "", 3);
                                setFlightStripInfo(FlightPlan, "", 4);

                                // Update CDM-API
                                runDetachedTask(&CDM::setCdmSts, callsign, "SUSP");
                            }

                            // If suspended by network Status, mark it as Invalid (I)
                            if (!stillOutOfTsat) {
                                for (size_t i = 0; i < myNetworkStatus.size(); i++) {
                                    if (myNetworkStatus[i][0] == callsign) {
                                        // Check for any FLS status (except "FLS-CDM")
                                        if (myNetworkStatus[i][1].find("FLS") != string::npos &&
                                            myNetworkStatus[i][1].find("CDM") == string::npos) {
                                            OutOfTsat.push_back({callsign, EOBT, aircraftFind ? TSAT : ""});
                                            setFlightStripInfo(FlightPlan, "", 0);
                                            setFlightStripInfo(FlightPlan, "", 3);
                                            setFlightStripInfo(FlightPlan, "", 4);
                                        }
                                    }
                                    // Do NOT Update CDM-API (As we are SUSP because of network status)
                                }
                            }

                            // EOBT
                            string completeEOBT = (string)EOBT;
                            string EOBThour = completeEOBT.substr(completeEOBT.length() - 6, 2);
                            string EOBTmin = completeEOBT.substr(completeEOBT.length() - 4, 2);

                            if (hour != "00") {
                                if (EOBThour == "00") {
                                    EOBThour = "24";
                                }
                            }

                            int EOBTdifTime = GetdifferenceTime(hour, min, EOBThour, EOBTmin);
                            if (hour != EOBThour) {
                                if (EOBTdifTime < -75) {
                                    notYetEOBT = true;
                                }
                            } else {
                                if (EOBTdifTime < -35) {
                                    notYetEOBT = true;
                                }
                            }

                            if (hour != EOBThour) {
                                if (EOBTdifTime >= -45) {
                                    actualTOBT = true;
                                }
                            } else {
                                if (EOBTdifTime >= -5) {
                                    actualTOBT = true;
                                }
                            }

                            if (hour != EOBThour) {
                                if (EOBTdifTime == 45) {
                                    lastMinuteTOBT = true;
                                }
                            } else {
                                if (EOBTdifTime == 5) {
                                    lastMinuteTOBT = true;
                                }
                            }

                            time_t now = time(nullptr);

                            // ASAT
                            bool ASATFound = false;
                            bool ASATPlusFiveLessTen = false;
                            int ASATpos = 0;
                            string ASATtext = " ";
                            for (size_t x = 0; x < asatList.size(); x++) {
                                string actualListCallsign = asatList[x].substr(0, asatList[x].find(","));
                                if (actualListCallsign == callsign) {
                                    ASATFound = true;
                                    ASATpos = x;
                                }
                            }

                            if (!ASATFound) {
                                if (correctState) {
                                    ASATtext = hour + min;
                                    asatList.push_back(callsign + "," + ASATtext);
                                    runDetachedTask(&CDM::setCdmSts, callsign, "AOBT/" + ASATtext);
                                    ASATFound = true;
                                }
                            } else {
                                if (correctState) {
                                    ASATtext = asatList[ASATpos].substr(asatList[ASATpos].length() - 4, 4);
                                } else if (!correctState) {
                                    asatList.erase(asatList.begin() + ASATpos);
                                    runDetachedTask(&CDM::setCdmSts, callsign, "AOBT/NULL");
                                    ASATFound = false;
                                }
                            }

                            // Check disply of items
                            bool showData = true;
                            if (aircraftFind) {
                                if (!slotList[pos].showData) {
                                    showData = false;
                                }
                            }

                            if (aircraftFind && showData && option_su_wait) {
                                for (size_t a = 0; a < suWaitList.size(); a++) {
                                    if (suWaitList[a] == FlightPlan.GetCallsign()) {
                                        suWaitList.erase(suWaitList.begin() + a);
                                        FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(4, "SU_WAIT");
                                    }
                                }
                            }

                            if (ItemCode == TAG_ITEM_EOBT) {
                                string ShowEOBT =
                                    formatTime(FlightPlan.GetFlightPlanData().GetEstimatedDepartureTime());
                                //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                ItemRGB = TAG_EOBT;
                                if (tobt.length() > 0) {
                                    string mySetEobt =
                                        formatTime(FlightPlan.GetFlightPlanData().GetEstimatedDepartureTime());
                                    if (mySetEobt != tobt && realMode) {
                                        ItemRGB = TAG_ORANGE;
                                    }
                                }
                                for (size_t i = 0; i < myNetworkStatus.size(); i++) {
                                    if (myNetworkStatus[i][0] == callsign) {
                                        if (myNetworkStatus[i][1].find("FLS") != string::npos) {
                                            ItemRGB = TAG_RED;
                                        }
                                        break;
                                    }
                                }
                                strcpy_s(sItemString, 16, ShowEOBT.c_str());
                            } else if (ItemCode == TAG_ITEM_TOBT) {
                                string ShowEOBT = (string)EOBT;
                                if (showData) {
                                    if (SU_ISSET) {
                                        ItemRGB = SU_SET_COLOR;
                                        strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
                                    } else if (notYetEOBT) {
                                        //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                        ItemRGB = TAG_GREENNOTACTIVE;
                                        strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
                                    } else if (!actualTOBT) {
                                        //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                        ItemRGB = TAG_GREENNOTACTIVE;
                                        strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
                                    } else if (lastMinuteTOBT && ASRTtext == "" && invalidateTOBT_Option) {
                                        //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                        bool toggle = (now % 2) == 0;
                                        if (toggle || !flashingTOBTend) {
                                            ItemRGB = TAG_YELLOW;
                                        } else {
                                            ItemRGB = TAG_GREEN;
                                        }
                                        strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
                                    } else {
                                        //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                        ItemRGB = TAG_GREEN;
                                        strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
                                    }
                                } else {
                                    ItemRGB = TAG_RED;
                                    strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
                                }
                            } else if (ItemCode == TAG_ITEM_ETOBT) {
                                string ShowEOBT = (string)EOBT;
                                if (showData) {
                                    if (SU_ISSET) {
                                        ItemRGB = SU_SET_COLOR;
                                        strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
                                    } else if (notYetEOBT) {
                                        //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                        ItemRGB = TAG_GREENNOTACTIVE;
                                        strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
                                    } else if (!actualTOBT) {
                                        //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                        ItemRGB = TAG_GREENNOTACTIVE;
                                        strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
                                    } else if (lastMinuteTOBT && ASRTtext == "" && invalidateTOBT_Option) {
                                        //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                        bool toggle = (now % 2) == 0;
                                        if (toggle || !flashingTOBTend) {
                                            ItemRGB = TAG_YELLOW;
                                        } else {
                                            ItemRGB = TAG_GREEN;
                                        }
                                        strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
                                    } else {
                                        //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                        ItemRGB = TAG_GREEN;
                                        strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
                                    }
                                } else {
                                    ItemRGB = TAG_RED;
                                    strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
                                }
                            } else if (ItemCode == TAG_ITEM_TSAC) {
                                // TSAC
                                bool TSACNotTSAT = false;
                                string annotTSAC = getFlightStripInfo(FlightPlan, 1);
                                if (annotTSAC != "9999") {
                                    if (!annotTSAC.empty()) {
                                        string TSAChour = annotTSAC.substr(annotTSAC.length() - 4, 2);
                                        string TSACmin = annotTSAC.substr(annotTSAC.length() - 2, 2);

                                        int TSACDif = GetdifferenceTime(TSAThour, TSATmin, TSAChour, TSACmin);
                                        if (TSAThour == TSAChour) {
                                            if (TSACDif > 5 || TSACDif < -5) {
                                                TSACNotTSAT = true;
                                            }
                                        } else {
                                            if (TSACDif > 45 || TSACDif < -45) {
                                                TSACNotTSAT = true;
                                            }
                                        }
                                    }

                                    if (TSACNotTSAT) {
                                        //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                        ItemRGB = TAG_ORANGE;
                                        if (SU_ISSET) ItemRGB = SU_SET_COLOR;
                                        strcpy_s(sItemString, 16, annotTSAC.c_str());
                                    } else if (!annotTSAC.empty()) {
                                        //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                        ItemRGB = TAG_GREEN;
                                        if (SU_ISSET) ItemRGB = SU_SET_COLOR;
                                        strcpy_s(sItemString, 16, annotTSAC.c_str());
                                    } else {
                                        //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                        ItemRGB = TAG_GREEN;
                                        if (SU_ISSET) ItemRGB = SU_SET_COLOR;
                                        strcpy_s(sItemString, 16, "____");
                                    }
                                }
                            } else if (ItemCode == TAG_ITEM_TSAC_SIMPLE) {
                                // TSAC
                                bool TSACNotTSAT = false;
                                string annotTSAC = getFlightStripInfo(FlightPlan, 1);

                                if (!annotTSAC.empty()) {
                                    string TSAChour = annotTSAC.substr(annotTSAC.length() - 4, 2);
                                    string TSACmin = annotTSAC.substr(annotTSAC.length() - 2, 2);

                                    int TSACDif = GetdifferenceTime(TSAThour, TSATmin, TSAChour, TSACmin);
                                    if (TSAThour == TSAChour) {
                                        if (TSACDif > 5 || TSACDif < -5) {
                                            TSACNotTSAT = true;
                                        }
                                    } else {
                                        if (TSACDif > 45 || TSACDif < -45) {
                                            TSACNotTSAT = true;
                                        }
                                    }
                                }

                                if (!annotTSAC.empty()) {
                                    if (SU_ISSET) {
                                        ItemRGB = SU_SET_COLOR;
                                    } else if (TSACNotTSAT) {
                                        ItemRGB = TAG_ORANGE;
                                    } else {
                                        //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                        ItemRGB = TAG_GREEN;
                                        if (SU_ISSET) ItemRGB = SU_SET_COLOR;
                                    }
                                    strcpy_s(sItemString, 16, "\xA4");
                                } else {
                                    ItemRGB = TAG_GREEN;
                                    if (SU_ISSET) ItemRGB = SU_SET_COLOR;
                                    strcpy_s(sItemString, 16, "\xAC");
                                }
                            } else if (ItemCode == TAG_ITEM_CTOC) {
                                // CTOC: orange=no CTOT, red=delay, green=improvement, grey=within window
                                string annotCTOC = getFlightStripInfo(FlightPlan, 8);
                                string ctotRef = "";
                                if (aircraftFind) {
                                    ctotRef = slotList[pos].ctot;
                                    if (ctotRef.empty() && slotList[pos].hasManualCtot) {
                                        ctotRef = slotList[pos].ttot;
                                    }
                                }
                                if (annotCTOC != "9999") {
                                    if (annotCTOC.empty()) {
                                        ItemRGB = TAG_GREEN;
                                        if (SU_ISSET) ItemRGB = SU_SET_COLOR;
                                        strcpy_s(sItemString, 16, "____");
                                    } else if (ctotRef.length() < 4) {
                                        ItemRGB = SU_ISSET ? SU_SET_COLOR : TAG_ORANGE;
                                        strcpy_s(sItemString, 16, annotCTOC.c_str());
                                    } else {
                                        string CTOThour = ctotRef.substr(0, 2);
                                        string CTOTmin = ctotRef.substr(2, 2);
                                        string CTOChour = annotCTOC.substr(annotCTOC.length() - 4, 2);
                                        string CTOCmin = annotCTOC.substr(annotCTOC.length() - 2, 2);
                                        int CTOCDif = GetdifferenceTime(CTOThour, CTOTmin, CTOChour, CTOCmin);
                                        int threshold = (CTOThour == CTOChour) ? 5 : 45;
                                        if (SU_ISSET) {
                                            ItemRGB = SU_SET_COLOR;
                                        } else if (CTOCDif > threshold) {
                                            ItemRGB = TAG_RED;
                                        } else if (CTOCDif < -threshold) {
                                            ItemRGB = TAG_GREEN;
                                        } else {
                                            ItemRGB = TAG_GREY;
                                        }
                                        strcpy_s(sItemString, 16, annotCTOC.c_str());
                                    }
                                }
                            } else if (ItemCode == TAG_ITEM_CTOC_SIMPLE) {
                                // CTOC diff: orange=no CTOT, red=delay, green=improvement, grey=within window
                                string annotCTOC2 = getFlightStripInfo(FlightPlan, 8);
                                string ctotRef2 = "";
                                if (aircraftFind) {
                                    ctotRef2 = slotList[pos].ctot;
                                    if (ctotRef2.empty() && slotList[pos].hasManualCtot) {
                                        ctotRef2 = slotList[pos].ttot;
                                    }
                                }
                                if (!annotCTOC2.empty()) {
                                    if (ctotRef2.length() < 4) {
                                        ItemRGB = SU_ISSET ? SU_SET_COLOR : TAG_ORANGE;
                                        strcpy_s(sItemString, 16, "X");
                                    } else if (annotCTOC2.length() >= 4) {
                                        int ctocDiff = GetDifferenceTimeHHMMSS(ctotRef2.substr(0, 4) + "00", annotCTOC2.substr(0, 4) + "00", true);
                                        string sign = (ctocDiff > 0) ? "+" : (ctocDiff < 0 ? "-" : "");
                                        string diffStr = sign + to_string(abs(ctocDiff));
                                        if (SU_ISSET) {
                                            ItemRGB = SU_SET_COLOR;
                                        } else if (ctocDiff > 5) {
                                            ItemRGB = TAG_RED;
                                        } else if (ctocDiff < -5) {
                                            ItemRGB = TAG_GREEN;
                                        } else {
                                            ItemRGB = TAG_GREY;
                                        }
                                        strcpy_s(sItemString, 16, diffStr.c_str());
                                    }
                                }
                            } else if (ItemCode == TAG_ITEM_TSAT) {
                                if (showData) {
                                    if (aircraftFind) {
                                        string ShowTSAT = (string)TSAT;
                                        ShowTSAT = ShowTSAT.substr(0, ShowTSAT.length() - 2);
                                        if (isEvSlot(callsign)) ShowTSAT = ShowTSAT + "E";

                                        if (SU_ISSET) {
                                            ItemRGB = SU_SET_COLOR;
                                            strcpy_s(sItemString, 16, ShowTSAT.c_str());
                                        } else if (notYetEOBT) {
                                            //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                            ItemRGB = TAG_GREY;
                                            strcpy_s(sItemString, 16, "~");
                                        } else if (firstMinute && flashingTSATstart) {
                                            bool toggle = (now % 2) == 0;
                                            if (toggle) {
                                                ItemRGB = TAG_ORANGE;
                                            } else {
                                                ItemRGB = TAG_GREEN;
                                            }
                                            strcpy_s(sItemString, 16, ShowTSAT.c_str());
                                        } else if (lastMinute) {
                                            //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                            bool toggle = (now % 2) == 0;
                                            if (toggle || !flashingTSATend) {
                                                ItemRGB = TAG_YELLOW;
                                            } else {
                                                ItemRGB = TAG_GREEN;
                                            }
                                            strcpy_s(sItemString, 16, ShowTSAT.c_str());
                                        } else if (moreLessFive) {
                                            //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                            ItemRGB = TAG_GREEN;
                                            strcpy_s(sItemString, 16, ShowTSAT.c_str());
                                        } else if (oldTSAT) {
                                            //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                            ItemRGB = TAG_RED;
                                            if (!invalidateTSAT_Option) {
                                                ItemRGB = TAG_YELLOW;
                                            }
                                            strcpy_s(sItemString, 16, ShowTSAT.c_str());
                                        } else {
                                            //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                            ItemRGB = TAG_GREENNOTACTIVE;
                                            strcpy_s(sItemString, 16, ShowTSAT.c_str());
                                        }
                                    }
                                } else {
                                    ItemRGB = TAG_GREY;
                                    strcpy_s(sItemString, 16, loadingText.c_str());
                                }
                            } else if (ItemCode == TAG_ITEM_TSAT_DIFF_TOBT) {
                                if (showData) {
                                    if (aircraftFind) {
                                        string value = getDiffNowTime(slotList[pos].eobt, false, slotList[pos].tsat);
                                        ItemRGB = TAG_GREENNOTACTIVE;
                                        strcpy_s(sItemString, 16, value.c_str());
                                    }
                                } else {
                                    ItemRGB = TAG_GREY;
                                    strcpy_s(sItemString, 16, loadingText.c_str());
                                }
                            } else if (ItemCode == TAG_ITEM_TSAT_TOBT_DIFF) {
                                if (showData) {
                                    if (aircraftFind) {
                                        string value = slotList[pos].tsat.substr(0, 4) +
                                                       getDiffTOBTTSAT(slotList[pos].tsat, slotList[pos].eobt);

                                        if (SU_ISSET) {
                                            ItemRGB = SU_SET_COLOR;
                                            strcpy_s(sItemString, 16, value.c_str());
                                        } else if (notYetEOBT) {
                                            //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                            ItemRGB = TAG_GREY;
                                            strcpy_s(sItemString, 16, "~");
                                        } else if (lastMinute) {
                                            ItemRGB = TAG_GREEN;
                                            strcpy_s(sItemString, 16, value.c_str());
                                        } else if (moreLessFive) {
                                            //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                            ItemRGB = TAG_GREEN;
                                            strcpy_s(sItemString, 16, value.c_str());
                                        } else if (oldTSAT) {
                                            //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                            ItemRGB = TAG_RED;
                                            if (!invalidateTSAT_Option) {
                                                ItemRGB = TAG_YELLOW;
                                            }
                                            strcpy_s(sItemString, 16, value.c_str());
                                        } else {
                                            //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                            ItemRGB = TAG_GREENNOTACTIVE;
                                            strcpy_s(sItemString, 16, value.c_str());
                                        }
                                    }
                                } else {
                                    ItemRGB = TAG_GREY;
                                    strcpy_s(sItemString, 16, loadingText.c_str());
                                }
                            } else if (ItemCode == NOW_TSAT_DIFF) {
                                if (showData) {
                                    if (aircraftFind) {
                                        string value = getDiffNowTime(slotList[pos].tsat, true, "");

                                        if (SU_ISSET) {
                                            ItemRGB = SU_SET_COLOR;
                                            strcpy_s(sItemString, 16, value.c_str());
                                        } else if (notYetEOBT) {
                                            //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                            ItemRGB = TAG_GREY;
                                            strcpy_s(sItemString, 16, "~");
                                        } else if (lastMinute) {
                                            ItemRGB = TAG_GREEN;
                                            strcpy_s(sItemString, 16, value.c_str());
                                        } else if (moreLessFive) {
                                            //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                            ItemRGB = TAG_GREEN;
                                            strcpy_s(sItemString, 16, value.c_str());
                                        } else if (oldTSAT) {
                                            //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                            ItemRGB = TAG_RED;
                                            if (!invalidateTSAT_Option) {
                                                ItemRGB = TAG_YELLOW;
                                            }
                                            strcpy_s(sItemString, 16, value.c_str());
                                        } else {
                                            //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                            ItemRGB = TAG_GREENNOTACTIVE;
                                            strcpy_s(sItemString, 16, value.c_str());
                                        }
                                    }
                                }
                            } else if (ItemCode == TAG_ITEM_TTOT) {
                                if (showData) {
                                    string ShowTTOT = (string)TTOT;
                                    if (notYetEOBT) {
                                        //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                        ItemRGB = TAG_GREY;
                                        strcpy_s(sItemString, 16, "~");
                                    } else if (ShowTTOT.length() >= 4) {
                                        ItemRGB = TAG_TTOT;
                                        if (oldTTOT) {
                                            ItemRGB = TAG_RED;
                                        }
                                        strcpy_s(sItemString, 16, ShowTTOT.substr(0, 4).c_str());
                                    }
                                } else {
                                    ItemRGB = TAG_GREY;
                                    strcpy_s(sItemString, 16, loadingText.c_str());
                                }
                            } else if (ItemCode == NOW_TTOT_DIFF) {
                                if (showData) {
                                    string value = getDiffNowTime(slotList[pos].ttot, true, "");
                                    if (notYetEOBT) {
                                        //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                        ItemRGB = TAG_GREY;
                                        strcpy_s(sItemString, 16, "~");
                                    } else if (moreLessFive || lastMinute) {
                                        //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                        ItemRGB = TAG_TTOT;
                                        strcpy_s(sItemString, 16, value.c_str());
                                    } else {
                                        //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                        ItemRGB = TAG_TTOT;
                                        strcpy_s(sItemString, 16, value.c_str());
                                    }
                                }
                            } else if (ItemCode == TAG_ITEM_ASAT) {
                                if (ASATFound) {
                                    string ASATHour = ASATtext.substr(0, 2);
                                    string ASATMin = ASATtext.substr(2, 2);
                                    if (hour != "00") {
                                        if (ASATHour == "00") {
                                            ASATHour = "24";
                                        }
                                    }

                                    int ASATDifTIme = GetdifferenceTime(hour, min, ASATHour, ASATMin);
                                    if ((string)FlightPlan.GetGroundState() == "STUP" ||
                                        (string)FlightPlan.GetGroundState() == "ST-UP" ||
                                        (string)FlightPlan.GetGroundState() == "") {
                                        if (hour == ASATHour) {
                                            if (ASATDifTIme >= 5) {
                                                ASATPlusFiveLessTen = true;
                                            }
                                        } else {
                                            if (ASATDifTIme >= 45) {
                                                ASATPlusFiveLessTen = true;
                                            }
                                        }
                                    }
                                }
                                if (ASATFound) {
                                    if ((string)FlightPlan.GetGroundState() == "" ||
                                        (string)FlightPlan.GetGroundState() == "STUP" ||
                                        (string)FlightPlan.GetGroundState() == "ST-UP") {
                                        if (ASATPlusFiveLessTen) {
                                            ItemRGB = TAG_YELLOW;
                                            strcpy_s(sItemString, 16, ASATtext.c_str());
                                        } else {
                                            ItemRGB = TAG_GREEN;
                                            strcpy_s(sItemString, 16, ASATtext.c_str());
                                        }
                                    } else {
                                        ItemRGB = SU_SET_COLOR;
                                        strcpy_s(sItemString, 16, ASATtext.c_str());
                                    }
                                } else {
                                    //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                    ItemRGB = TAG_GREEN;
                                    strcpy_s(sItemString, 16, " ");
                                }
                            } else if (ItemCode == TAG_ITEM_ASRT) {
                                string ASRTtext = getFlightStripInfo(FlightPlan, 0);
                                if (!ASRTtext.empty()) {
                                    //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                    if (SU_ISSET) {
                                        ItemRGB = SU_SET_COLOR;
                                    } else {
                                        ItemRGB = TAG_ASRT;
                                    }
                                    strcpy_s(sItemString, 16, ASRTtext.c_str());
                                } else {
                                    //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                    ItemRGB = TAG_ASRT;
                                    strcpy_s(sItemString, 16, " ");
                                }
                            } else if (ItemCode == TAG_ITEM_READYSTARTUP) {
                                string ASRTtext = getFlightStripInfo(FlightPlan, 0);
                                if (!ASRTtext.empty()) {
                                    ItemRGB = TAG_GREEN;
                                    strcpy_s(sItemString, 16, "RSTUP");
                                } else {
                                    ItemRGB = TAG_RED;
                                    strcpy_s(sItemString, 16, "RSTUP");
                                }
                            } else if (ItemCode == TAG_ITEM_E) {
                                if (notYetEOBT) {
                                    //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                    ItemRGB = TAG_GREEN;
                                    strcpy_s(sItemString, 16, "P");
                                } else {
                                    //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                    ItemRGB = TAG_GREEN;
                                    strcpy_s(sItemString, 16, "C");
                                }
                            } else if (ItemCode == TAG_ITEM_FLOW_MESSAGE) {
                                if (showData) {
                                    if (aircraftFind) {
                                        if (slotList[pos].hasManualCtot) {
                                            string message = "MAN ACT";
                                            if (slotList[pos].ctot != "") {
                                                message = slotList[pos].flowReason;
                                            }
                                            ItemRGB = TAG_YELLOW;
                                            strcpy_s(sItemString, 16, message.c_str());
                                        }
                                    }
                                } else {
                                    ItemRGB = TAG_GREY;
                                    strcpy_s(sItemString, 16, loadingText.c_str());
                                }
                            } else if (ItemCode == TAG_ITEM_CTOT) {
                                if (showData) {
                                    if (aircraftFind) {
                                        if (slotList[pos].hasManualCtot) {
                                            string value = "";
                                            if (slotList[pos].ctot == "") {
                                                value = slotList[pos].ttot.substr(0, 4);
                                                ItemRGB = TAG_ORANGE;
                                            } else {
                                                value = slotList[pos].ctot;
                                                ItemRGB = TAG_CTOT;
                                                for (size_t i = 0; i < myNetworkStatus.size(); i++) {
                                                    if (myNetworkStatus[i][0] == callsign &&
                                                        myNetworkStatus[i][1] == "REA" && !ASATFound) {
                                                        ItemRGB = TAG_YELLOW;
                                                    }
                                                }
                                            }
                                            strcpy_s(sItemString, 16, value.c_str());
                                        }
                                    }
                                } else {
                                    ItemRGB = TAG_GREY;
                                    strcpy_s(sItemString, 16, loadingText.c_str());
                                }
                            } else if (ItemCode == NOW_CTOT_DIFF) {
                                if (showData) {
                                    bool inreaList = false;
                                    for (string s : reaCTOTSent) {
                                        if (s == callsign) {
                                            inreaList = true;
                                        }
                                    }

                                    if (aircraftFind) {
                                        if (slotList[pos].hasManualCtot) {
                                            string ctotSource = slotList[pos].ttot;
                                            if (slotList[pos].ctot != "") ctotSource = slotList[pos].ctot;
                                            string value = getDiffNowTime(ctotSource, true, "");
                                            if (slotList[pos].ctot == "") {
                                                ItemRGB = TAG_ORANGE;
                                            } else {
                                                ItemRGB = TAG_CTOT;
                                                for (size_t i = 0; i < myNetworkStatus.size(); i++) {
                                                    if (myNetworkStatus[i][0] == callsign &&
                                                        myNetworkStatus[i][1] == "REA" && !ASATFound) {
                                                        ItemRGB = TAG_YELLOW;
                                                    }
                                                }
                                            }
                                            strcpy_s(sItemString, 16, value.c_str());
                                        }
                                    }
                                }
                            } else if (ItemCode == TAG_ITEM_EV_CTOT) {
                                bool inEvCtotsList = false;
                                string slot = "";
                                for (size_t i = 0; i < evCtots.size(); i++) {
                                    if (evCtots[i][0] == callsign) {
                                        inEvCtotsList = true;
                                        slot = evCtots[i][1];
                                    }
                                }
                                if (inEvCtotsList) {
                                    ItemRGB = TAG_GREY;
                                    strcpy_s(sItemString, 16, slot.c_str());
                                }
                            } else if (ItemCode == TAG_ITEM_NETWORK_STATUS) {
                                string status = "";
                                for (size_t i = 0; i < myNetworkStatus.size(); i++) {
                                    if (myNetworkStatus[i][0] == callsign) {
                                        status = myNetworkStatus[i][1];
                                    }
                                }
                                if (status != "") {
                                    ItemRGB = TAG_YELLOW;
                                    if (status == "REA") {
                                        ItemRGB = TAG_YELLOW;
                                        strcpy_s(sItemString, 16, status.c_str());
                                    } else if (status.find("FLS") != string::npos) {
                                        ItemRGB = TAG_RED;
                                        status = GetTimedStatus(status);
                                        strcpy_s(sItemString, 16, status.c_str());
                                    }
                                }
                            } else if (ItemCode == TAG_ITEM_NETWORK_STATUS_AIRBORNE) {
                                string status = "";
                                for (size_t i = 0; i < myNetworkStatus.size(); i++) {
                                    if (myNetworkStatus[i][0] == callsign) {
                                        status = myNetworkStatus[i][1];
                                    }
                                }
                                if (status != "") {
                                    ItemRGB = TAG_YELLOW;
                                    if (status == "COMPLY") {
                                        ItemRGB = TAG_GREEN;
                                        status = "C";
                                        strcpy_s(sItemString, 16, status.c_str());
                                    } else if (status == "AIRB") {
                                        ItemRGB = TAG_RED;
                                        status = "A";
                                        strcpy_s(sItemString, 16, status.c_str());
                                    }
                                }
                            } else if (ItemCode == TAG_ITEM_ON_TIME_STATUS) {
                                string status = "";
                                for (size_t i = 0; i < onTimeStatus.size(); i++) {
                                    if (onTimeStatus[i][0] == callsign) {
                                        status = onTimeStatus[i][1];
                                    }
                                }
                                if (status != "") {
                                    ItemRGB = TAG_YELLOW;
                                    if (status.find("+") != string::npos) {
                                        ItemRGB = TAG_RED;
                                    } else if (status.find("-") != string::npos) {
                                        ItemRGB = TAG_GREEN;
                                    } else {
                                        status = "";
                                    }
                                    strcpy_s(sItemString, 16, status.c_str());
                                }
                            } else if (ItemCode == TAG_ITEM_SEND_STATUS) {
                                ItemRGB = TAG_RED;
                                for (string flt : messagesSent) {
                                    if (flt == callsign) {
                                        ItemRGB = TAG_GREEN;
                                    }
                                }
                                strcpy_s(sItemString, 16, "SEND");
                            } else if (ItemCode == TAG_ITEM_SEND_STATUS_SHORT) {
                                ItemRGB = TAG_RED;
                                for (string flt : messagesSent) {
                                    if (flt == callsign) {
                                        ItemRGB = TAG_GREEN;
                                    }
                                }
                                strcpy_s(sItemString, 16, "S");
                            } else if (ItemCode == TAG_ITEM_DEICE) {
                                string status = "";
                                for (vector<string> deice : deiceList) {
                                    if (deice[0] == callsign) {
                                        status = deice[1];
                                    }
                                }
                                ItemRGB = TAG_YELLOW;
                                strcpy_s(sItemString, 16, status.c_str());
                            } else if (ItemCode == TAG_ITEM_TOBT_SETBY) {
                                string status = getFlightStripInfo(FlightPlan, 9);
                                if (status == "A") status = "ATC";
                                else if (status == "P") status = "PILOT";
                                ItemRGB = TAG_GREEN;
                                strcpy_s(sItemString, 16, status.c_str());
                            } else if (ItemCode == TAG_ITEM_TOBT_SETBY_SHORT) {
                                string status = getFlightStripInfo(FlightPlan, 9);
                                ItemRGB = TAG_GREEN;
                                if (status.length() > 1) {
                                    status = status.substr(0, 1);
                                }
                                strcpy_s(sItemString, 16, status.c_str());
                            }

                            // Update Manual CTOT to Slaves
                            if (aircraftFind) {
                                string stripManualCtot = getFlightStripInfo(FlightPlan, 7);
                                if (stripManualCtot == "" && slotList[pos].hasManualCtot && slotList[pos].ctot == "") {
                                    setFlightStripInfo(FlightPlan, "1", 7);
                                } else if (stripManualCtot != "" && !slotList[pos].hasManualCtot) {
                                    setFlightStripInfo(FlightPlan, "", 7);
                                }
                            }

                            // Refresh CDM API every 30 seconds
                            time_t timeNow = std::time(nullptr);

                            // Remove disconnected planes after 3 min disconnected
                            if (countTfcDisconnection != -1) {
                                if ((timeNow - countTfcDisconnectionTime) > 180) {
                                    countTfcDisconnectionTime = timeNow;
                                    countTfcDisconnection = -1;
                                    disconnectTfcs();
                                    pos = getPlanePosition(callsign);
                                    if (pos == -1) {
                                        aircraftFind = false;
                                    }
                                }
                            }

                            // Check readyToUpdateList;
                            if (readyToUpdateList && !refresh1) {
                                addLogLine("[AUTO] - Updating slotList with latest update...");
                                for (Plane p : slotListToUpdate) {
                                    for (int d = 0; d < slotList.size(); d++) {
                                        if (p.callsign == slotList[d].callsign && p.eobt == slotList[d].eobt &&
                                            p.ctot == slotList[d].ctot) {
                                            //Do not update if has a GND status set
                                            bool aicraftInFinalTimesList = false;
                                            for (string aircraft : finalTimesList) {
                                                if (aircraft == p.callsign) {
                                                    aicraftInFinalTimesList = true;
                                                }
                                            }
                                            if (!aicraftInFinalTimesList) {
                                                p.showData = slotList[d].showData;
                                                slotList[d] = p;
                                            } else if (atotEnabled) {
                                                slotList[d].ttot = p.ttot;
                                            }
                                        }
                                    }
                                }
                                addLogLine("[AUTO] - SlotList list updated succesfully");
                                slotListToUpdate.clear();
                                readyToUpdateList = false;
                            }

                            // Refresh times every x sec
                            if ((timeNow - countTime) > refreshTime && !refresh1) {
                                refresh1 = true;
                                countTime = timeNow;
                                addLogLine("[AUTO] - REFRESH CDM INTERNAL DATA");
                                // Order list according TTOT
                                slotList = recalculateSlotList(slotList);
                                pos = getPlanePosition(callsign);

                                for (size_t t = 0; t < slotList.size(); t++) {
                                    CFlightPlan fpSelected = FlightPlanSelect(slotList[t].callsign.c_str());
                                    PushToOtherControllers(fpSelected);
                                }
                                if (debugMode) {
                                    sendMessage("[DEBUG MESSAGE] - REFRESHING");
                                    sendMessage("[DEBUG MESSAGE] - " + to_string(slotList.size()) +
                                                " Planes in the list");
                                }

                                runDetachedTask(&CDM::refreshActions1);
                            }
                        }
                    } else {
                        // Remove disconnected planes after 3 min disconnected
                        if (countTfcDisconnection != -1) {
                            if ((timeNow - countTfcDisconnectionTime) > 180) {
                                countTfcDisconnectionTime = timeNow;
                                countTfcDisconnection = -1;
                                disconnectTfcs();
                                pos = getPlanePosition(callsign);
                                if (pos == -1) {
                                    aircraftFind = false;
                                }
                            }
                        }

                        bool TSATFind = true;
                        string TSATString = getFlightStripInfo(FlightPlan, 3);
                        string TTOTString = getFlightStripInfo(FlightPlan, 4);
                        if (TSATString == "") {
                            TSATFind = false;
                        }

                        if (aircraftFind) {
                            if (!TSATFind) {
                                if (pos < slotList.size()) {  // Check if pos is within bounds
                                    slotList.erase(slotList.begin() + pos);
                                }
                            } else if (TSATString != slotList[pos].tsat || TTOTString != slotList[pos].ttot) {
                                if (pos < slotList.size()) {  // Check if pos is within bounds
                                    slotList[pos].tsat = TSATString;
                                    slotList[pos].ttot = TTOTString;
                                }
                            }
                        } else {
                            if (TSATFind) {
                                Plane p(callsign, EOBT, TSATString, TTOTString, "", "", hasManualCtot, true, true);
                                slotList.push_back(p);
                            }
                        }

                        // Update de-ice status
                        if (aircraftFind) {
                            string deIce = getFlightStripInfo(FlightPlan, 5);
                            bool found = false;
                            for (int z = 0; z < deiceList.size(); z++) {
                                if (deiceList[z][0] == callsign) {
                                    found = true;
                                    if (deIce == "") {
                                        // Remove from list
                                        deiceList.erase(deiceList.begin() + z);
                                    } else if (deIce != deiceList[z][1]) {
                                        // Modify list value
                                        deiceList[z] = {callsign, deIce};
                                    }
                                }
                            }

                            if (!found && deIce != "") {
                                // Add to main de-ice list
                                deiceList.push_back({callsign, deIce});
                            }
                        }

                        // Update Manual CTOT from Master
                        if (aircraftFind) {
                            string manualCtot = getFlightStripInfo(FlightPlan, 7);
                            if (manualCtot != "" && !slotList[pos].hasManualCtot) {
                                slotList[pos].hasManualCtot = 1;
                            } else if (manualCtot == "" && slotList[pos].hasManualCtot) {
                                slotList[pos].hasManualCtot = 0;
                            }
                        }

                        // If oldTSAT
                        if (TSATFind) {
                            string TSAThour = TSATString.substr(TSATString.length() - 6, 2);
                            string TSATmin = TSATString.substr(TSATString.length() - 4, 2);

                            bool oldTSAT = false;
                            bool moreLessFive = false;
                            bool lastMinute = false;
                            bool firstMinute = false;
                            bool lastMinuteTOBT = false;
                            bool notYetEOBT = false;
                            bool actualTOBT = false;

                            if (hour != "00" && TSAThour == "00") {
                                TSAThour = "24";
                            }

                            int difTime = GetdifferenceTime(hour, min, TSAThour, TSATmin);

                            if (hour != TSAThour) {
                                if (difTime == -45) {
                                    firstMinute = true;
                                } else if (difTime >= 44 && difTime <= 45) {
                                    lastMinute = true;
                                } else if (difTime >= -45 && difTime <= 45) {
                                    moreLessFive = true;
                                } else if (difTime > 45) {
                                    oldTSAT = true;
                                }
                            } else {
                                if (difTime == -5) {
                                    firstMinute = true;
                                    moreLessFive = true;
                                } else if (difTime > 5) {
                                    oldTSAT = true;
                                } else if (difTime >= 4 && difTime <= 5) {
                                    lastMinute = true;
                                } else if (difTime >= -5 && difTime <= 5) {
                                    moreLessFive = true;
                                }
                            }

                            bool correctState = false;
                            string groundState = (string)FlightPlan.GetGroundState();
                            if (groundState == "STUP" || groundState == "ST-UP" || groundState == "PUSH" ||
                                groundState == "TAXI" || groundState == "DEPA") {
                                correctState = true;
                            }

                            if (oldTSAT && !correctState) {
                                bool alreadyInList = false;
                                for (size_t i = 0; i < OutOfTsat.size(); i++) {
                                    if (callsign == OutOfTsat[i][0]) {
                                        alreadyInList = true;
                                    }
                                }

                                if (!alreadyInList) {
                                    OutOfTsat.push_back({callsign, EOBT, aircraftFind ? TSAT : ""});
                                }
                            }

                            string completeEOBT = (string)EOBT;
                            string EOBThour = completeEOBT.substr(completeEOBT.length() - 6, 2);
                            string EOBTmin = completeEOBT.substr(completeEOBT.length() - 4, 2);

                            if (hour != "00" && EOBThour == "00") {
                                EOBThour = "24";
                            }

                            int EOBTdifTime = GetdifferenceTime(hour, min, EOBThour, EOBTmin);
                            if (hour != EOBThour) {
                                if (EOBTdifTime < -75) {
                                    notYetEOBT = true;
                                }
                            } else {
                                if (EOBTdifTime < -35) {
                                    notYetEOBT = true;
                                }
                            }

                            if (hour != EOBThour) {
                                if (EOBTdifTime >= -45) {
                                    actualTOBT = true;
                                }
                            } else {
                                if (EOBTdifTime >= -5) {
                                    actualTOBT = true;
                                }
                            }

                            if (hour != EOBThour) {
                                if (EOBTdifTime == 45) {
                                    lastMinuteTOBT = true;
                                }
                            } else {
                                if (EOBTdifTime == 5) {
                                    lastMinuteTOBT = true;
                                }
                            }

                            // Check TTOT difTime from now to TTOT and if TOBT+10min > now then oldTTOT flag to true
                            bool oldTTOT = false;
                            if (TTOTString.length() >= 4) {
                                string TTOTHour = TTOTString.substr(0, 2);
                                string TTOTmin = TTOTString.substr(2, 2);
                                int difTTOTTime = GetdifferenceTime(hour, min, TTOTHour, TTOTmin);
                                if (hour != TTOTHour) {
                                    if (difTTOTTime > 50) {
                                        oldTTOT = true;
                                    }
                                } else {
                                    if (difTTOTTime > 10) {
                                        oldTTOT = true;
                                    }
                                }
                            }

                            // ASRT
                            string ASRTtext = getFlightStripInfo(FlightPlan, 0);

                            // TSAC
                            bool TSACNotTSAT = false;
                            string annotTSAC = getFlightStripInfo(FlightPlan, 1);

                            if (!annotTSAC.empty()) {
                                string TSAChour = annotTSAC.substr(annotTSAC.length() - 4, 2);
                                string TSACmin = annotTSAC.substr(annotTSAC.length() - 2, 2);

                                int TSACDif = GetdifferenceTime(TSAThour, TSATmin, TSAChour, TSACmin);
                                if (TSAThour == TSAChour) {
                                    if (TSACDif > 5 || TSACDif < -5) {
                                        TSACNotTSAT = true;
                                    }
                                } else {
                                    if (TSACDif > 45 || TSACDif < -45) {
                                        TSACNotTSAT = true;
                                    }
                                }
                            }

                            // CTOC
                            bool CTOCNotCTOT = false;
                            string annotCTOC = getFlightStripInfo(FlightPlan, 8);
                            string ctotRef = "";
                            if (aircraftFind) {
                                ctotRef = slotList[pos].ctot;
                                if (ctotRef.empty() && slotList[pos].hasManualCtot) {
                                    ctotRef = slotList[pos].ttot;
                                }
                            }

                            if (!annotCTOC.empty() && ctotRef.length() >= 4) {
                                string CTOThour = ctotRef.substr(0, 2);
                                string CTOTmin = ctotRef.substr(2, 2);
                                string CTOChour = annotCTOC.substr(annotCTOC.length() - 4, 2);
                                string CTOCmin = annotCTOC.substr(annotCTOC.length() - 2, 2);

                                int CTOCDif = GetdifferenceTime(CTOThour, CTOTmin, CTOChour, CTOCmin);
                                if (CTOThour == CTOChour) {
                                    if (CTOCDif > 5 || CTOCDif < -5) {
                                        CTOCNotCTOT = true;
                                    }
                                } else {
                                    if (CTOCDif > 45 || CTOCDif < -45) {
                                        CTOCNotCTOT = true;
                                    }
                                }
                            }

                            time_t now = time(nullptr);

                            // ASAT
                            bool ASATFound = false;
                            bool ASATPlusFiveLessTen = false;
                            int ASATpos = 0;
                            string ASATtext = " ";
                            for (size_t x = 0; x < asatList.size(); x++) {
                                string actualListCallsign = asatList[x].substr(0, asatList[x].find(","));
                                if (actualListCallsign == callsign) {
                                    ASATFound = true;
                                    ASATpos = x;
                                    break;  // Break the loop once found
                                }
                            }

                            if (!ASATFound) {
                                if (correctState) {
                                    ASATtext = hour + min;
                                    asatList.push_back(callsign + "," + ASATtext);
                                    ASATFound = true;
                                }
                            } else {
                                if (correctState) {
                                    ASATtext = asatList[ASATpos].substr(asatList[ASATpos].length() - 4, 4);
                                } else if (!correctState) {
                                    if (ASATpos < asatList.size()) {  // Check if ASATpos is within bounds
                                        asatList.erase(asatList.begin() + ASATpos);
                                        ASATFound = false;
                                    }
                                }
                            }

                            if (ASATFound) {
                                string ASATHour = ASATtext.substr(0, 2);
                                string ASATMin = ASATtext.substr(2, 2);
                                if (hour != "00" && ASATHour == "00") {
                                    ASATHour = "24";
                                }

                                int ASATDifTIme = GetdifferenceTime(hour, min, ASATHour, ASATMin);
                                string groundState = (string)FlightPlan.GetGroundState();
                                if (groundState == "STUP" || groundState == "ST-UP" || groundState == "") {
                                    if (hour == ASATHour) {
                                        if (ASATDifTIme >= 5) {
                                            ASATPlusFiveLessTen = true;
                                        }
                                    } else {
                                        if (ASATDifTIme >= 45) {
                                            ASATPlusFiveLessTen = true;
                                        }
                                    }
                                }
                            }

                            if (ItemCode == TAG_ITEM_EOBT) {
                                string ShowEOBT =
                                    formatTime(FlightPlan.GetFlightPlanData().GetEstimatedDepartureTime());
                                ItemRGB = TAG_EOBT;
                                for (size_t i = 0; i < myNetworkStatus.size(); i++) {
                                    if (myNetworkStatus[i][0] == callsign) {
                                        if (myNetworkStatus[i][1].find("FLS") != string::npos) {
                                            ItemRGB = TAG_RED;
                                        }
                                        break;
                                    }
                                }
                                strcpy_s(sItemString, 16, ShowEOBT.c_str());
                            } else if (ItemCode == TAG_ITEM_TOBT) {
                                string ShowEOBT = (string)EOBT;
                                if (SU_ISSET) {
                                    ItemRGB = SU_SET_COLOR;
                                    strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
                                } else if (notYetEOBT) {
                                    ItemRGB = TAG_GREY;
                                    strcpy_s(sItemString, 16, "~");
                                } else if (!actualTOBT) {
                                    ItemRGB = TAG_GREENNOTACTIVE;
                                    strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
                                } else if (lastMinuteTOBT && ASRTtext == "" && invalidateTOBT_Option) {
                                    //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                    bool toggle = (now % 2) == 0;
                                    if (toggle || !flashingTOBTend) {
                                        ItemRGB = TAG_YELLOW;
                                    } else {
                                        ItemRGB = TAG_GREEN;
                                    }
                                    strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
                                } else {
                                    ItemRGB = TAG_GREEN;
                                    strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
                                }
                            } else if (ItemCode == TAG_ITEM_ETOBT) {
                                string ShowEOBT = (string)EOBT;
                                if (SU_ISSET) {
                                    ItemRGB = SU_SET_COLOR;
                                    strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
                                } else if (notYetEOBT) {
                                    ItemRGB = TAG_GREY;
                                    strcpy_s(sItemString, 16, "~");
                                } else if (!actualTOBT) {
                                    ItemRGB = TAG_GREENNOTACTIVE;
                                    strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
                                } else if (lastMinuteTOBT && ASRTtext == "" && invalidateTOBT_Option) {
                                    //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                    bool toggle = (now % 2) == 0;
                                    if (toggle || !flashingTOBTend) {
                                        ItemRGB = TAG_YELLOW;
                                    } else {
                                        ItemRGB = TAG_GREEN;
                                    }
                                    strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
                                } else {
                                    ItemRGB = TAG_GREEN;
                                    strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
                                }
                            } else if (ItemCode == TAG_ITEM_TSAC) {
                                if (TSACNotTSAT) {
                                    ItemRGB = TAG_ORANGE;
                                    if (SU_ISSET) ItemRGB = SU_SET_COLOR;
                                    strcpy_s(sItemString, 16, annotTSAC.c_str());
                                } else if (!annotTSAC.empty()) {
                                    ItemRGB = TAG_GREEN;
                                    if (SU_ISSET) ItemRGB = SU_SET_COLOR;
                                    strcpy_s(sItemString, 16, annotTSAC.c_str());
                                } else {
                                    ItemRGB = TAG_GREEN;
                                    if (SU_ISSET) ItemRGB = SU_SET_COLOR;
                                    strcpy_s(sItemString, 16, "____");
                                }
                            } else if (ItemCode == TAG_ITEM_TSAC_SIMPLE) {
                                string annotTSAC = getFlightStripInfo(FlightPlan, 1);
                                if (!annotTSAC.empty()) {
                                    if (TSACNotTSAT) {
                                        ItemRGB = TAG_ORANGE;
                                        if (SU_ISSET) ItemRGB = SU_SET_COLOR;
                                    } else {
                                        //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                        ItemRGB = TAG_GREEN;
                                        if (SU_ISSET) ItemRGB = SU_SET_COLOR;
                                    }
                                    strcpy_s(sItemString, 16, "\xA4");
                                } else {
                                    ItemRGB = TAG_GREEN;
                                    if (SU_ISSET) ItemRGB = SU_SET_COLOR;
                                    strcpy_s(sItemString, 16, "\xAC");
                                }
                            } else if (ItemCode == TAG_ITEM_CTOC) {
                                // CTOC: orange=no CTOT, red=delay, green=improvement, grey=within window
                                if (annotCTOC.empty()) {
                                    ItemRGB = TAG_GREEN;
                                    if (SU_ISSET) ItemRGB = SU_SET_COLOR;
                                    strcpy_s(sItemString, 16, "____");
                                } else if (ctotRef.length() < 4) {
                                    ItemRGB = SU_ISSET ? SU_SET_COLOR : TAG_ORANGE;
                                    strcpy_s(sItemString, 16, annotCTOC.c_str());
                                } else {
                                    string CTOThour3 = ctotRef.substr(0, 2);
                                    string CTOTmin3 = ctotRef.substr(2, 2);
                                    string CTOChour3 = annotCTOC.substr(annotCTOC.length() - 4, 2);
                                    string CTOCmin3 = annotCTOC.substr(annotCTOC.length() - 2, 2);
                                    int CTOCDif3 = GetdifferenceTime(CTOThour3, CTOTmin3, CTOChour3, CTOCmin3);
                                    int threshold3 = (CTOThour3 == CTOChour3) ? 5 : 45;
                                    if (SU_ISSET) {
                                        ItemRGB = SU_SET_COLOR;
                                    } else if (CTOCDif3 > threshold3) {
                                        ItemRGB = TAG_RED;
                                    } else if (CTOCDif3 < -threshold3) {
                                        ItemRGB = TAG_GREEN;
                                    } else {
                                        ItemRGB = TAG_GREY;
                                    }
                                    strcpy_s(sItemString, 16, annotCTOC.c_str());
                                }
                            } else if (ItemCode == TAG_ITEM_CTOC_SIMPLE) {
                                // CTOC diff: orange=no CTOT, red=delay, green=improvement, grey=within window
                                if (!annotCTOC.empty()) {
                                    if (ctotRef.length() < 4) {
                                        ItemRGB = SU_ISSET ? SU_SET_COLOR : TAG_ORANGE;
                                        strcpy_s(sItemString, 16, "X");
                                    } else if (annotCTOC.length() >= 4) {
                                        int ctocDiff = GetDifferenceTimeHHMMSS(ctotRef.substr(0, 4) + "00", annotCTOC.substr(0, 4) + "00", true);
                                        string sign = (ctocDiff > 0) ? "+" : (ctocDiff < 0 ? "-" : "");
                                        string diffStr2 = sign + to_string(abs(ctocDiff));
                                        if (SU_ISSET) {
                                            ItemRGB = SU_SET_COLOR;
                                        } else if (ctocDiff > 5) {
                                            ItemRGB = TAG_RED;
                                        } else if (ctocDiff < -5) {
                                            ItemRGB = TAG_GREEN;
                                        } else {
                                            ItemRGB = TAG_GREY;
                                        }
                                        strcpy_s(sItemString, 16, diffStr2.c_str());
                                    }
                                }
                            } else if (ItemCode == TAG_ITEM_TSAT) {
                                if (TSATString.length() > 0 && aircraftFind) {
                                    TSATString = TSATString.substr(0, 4);
                                    if (isEvSlot(callsign)) TSATString = TSATString + "E";

                                    if (SU_ISSET) {
                                        ItemRGB = SU_SET_COLOR;
                                        strcpy_s(sItemString, 16, TSATString.c_str());
                                    } else if (notYetEOBT) {
                                        ItemRGB = TAG_GREY;
                                        strcpy_s(sItemString, 16, "~");
                                    } else if (firstMinute && flashingTSATstart) {
                                        bool toggle = (now % 2) == 0;
                                        if (toggle) {
                                            ItemRGB = TAG_ORANGE;
                                        } else {
                                            ItemRGB = TAG_GREEN;
                                        }
                                        strcpy_s(sItemString, 16, TSATString.c_str());
                                    } else if (lastMinute) {
                                        //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                        bool toggle = (now % 2) == 0;
                                        if (toggle || !flashingTSATend) {
                                            ItemRGB = TAG_YELLOW;
                                        } else {
                                            ItemRGB = TAG_GREEN;
                                        }
                                        strcpy_s(sItemString, 16, TSATString.c_str());
                                    } else if (moreLessFive) {
                                        ItemRGB = TAG_GREEN;
                                        strcpy_s(sItemString, 16, TSATString.c_str());
                                    } else if (oldTSAT) {
                                        ItemRGB = TAG_RED;
                                        if (!invalidateTSAT_Option) {
                                            ItemRGB = TAG_YELLOW;
                                        }
                                        strcpy_s(sItemString, 16, TSATString.c_str());
                                    } else {
                                        ItemRGB = TAG_GREENNOTACTIVE;
                                        strcpy_s(sItemString, 16, TSATString.c_str());
                                    }
                                }
                            } else if (ItemCode == TAG_ITEM_TSAT_DIFF_TOBT) {
                                if (aircraftFind) {
                                    string value = getDiffNowTime(slotList[pos].eobt, false, slotList[pos].tsat);
                                    ItemRGB = TAG_GREENNOTACTIVE;
                                    strcpy_s(sItemString, 16, value.c_str());
                                }
                            } else if (ItemCode == TAG_ITEM_TSAT_TOBT_DIFF) {
                                if (aircraftFind) {
                                    string value = slotList[pos].tsat.substr(0, 4) +
                                                   getDiffTOBTTSAT(slotList[pos].tsat, slotList[pos].eobt);
                                    if (SU_ISSET) {
                                        ItemRGB = SU_SET_COLOR;
                                        strcpy_s(sItemString, 16, value.c_str());
                                    } else if (notYetEOBT) {
                                        //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                        ItemRGB = TAG_GREY;
                                        strcpy_s(sItemString, 16, "~");
                                    } else if (lastMinute) {
                                        //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                        ItemRGB = TAG_YELLOW;
                                        strcpy_s(sItemString, 16, value.c_str());
                                    } else if (moreLessFive) {
                                        //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                        ItemRGB = TAG_GREEN;
                                        strcpy_s(sItemString, 16, value.c_str());
                                    } else if (oldTSAT) {
                                        //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                        ItemRGB = TAG_RED;
                                        if (!invalidateTSAT_Option) {
                                            ItemRGB = TAG_YELLOW;
                                        }
                                        strcpy_s(sItemString, 16, value.c_str());
                                    } else {
                                        //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                        ItemRGB = TAG_GREENNOTACTIVE;
                                        strcpy_s(sItemString, 16, value.c_str());
                                    }
                                }
                            } else if (ItemCode == NOW_TSAT_DIFF) {
                                if (aircraftFind) {
                                    string value = getDiffNowTime(slotList[pos].tsat, true, "");
                                    if (SU_ISSET) {
                                        ItemRGB = SU_SET_COLOR;
                                        strcpy_s(sItemString, 16, value.c_str());
                                    } else if (notYetEOBT) {
                                        //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                        ItemRGB = TAG_GREY;
                                        strcpy_s(sItemString, 16, "~");
                                    } else if (lastMinute) {
                                        //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                        ItemRGB = TAG_YELLOW;
                                        strcpy_s(sItemString, 16, value.c_str());
                                    } else if (moreLessFive) {
                                        //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                        ItemRGB = TAG_GREEN;
                                        strcpy_s(sItemString, 16, value.c_str());
                                    } else if (oldTSAT) {
                                        //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                        ItemRGB = TAG_RED;
                                        if (!invalidateTSAT_Option) {
                                            ItemRGB = TAG_YELLOW;
                                        }
                                        strcpy_s(sItemString, 16, value.c_str());
                                    } else {
                                        //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                        ItemRGB = TAG_GREENNOTACTIVE;
                                        strcpy_s(sItemString, 16, value.c_str());
                                    }
                                }
                            } else if (ItemCode == TAG_ITEM_TTOT) {
                                if (TTOTString.length() > 0) {
                                    if (notYetEOBT) {
                                        ItemRGB = TAG_GREY;
                                        strcpy_s(sItemString, 16, "~");
                                    } else {
                                        ItemRGB = TAG_TTOT;
                                        if (oldTTOT) {
                                            ItemRGB = TAG_RED;
                                        }
                                        strcpy_s(sItemString, 16, TTOTString.substr(0, 4).c_str());
                                    }
                                }
                            } else if (ItemCode == NOW_TTOT_DIFF) {
                                if (TTOTString.length() >= 4) {
                                    string value = getDiffNowTime(TTOTString.substr(0, 4), true, "");
                                    if (notYetEOBT) {
                                        ItemRGB = TAG_GREY;
                                        strcpy_s(sItemString, 16, "~");
                                    } else if (moreLessFive || lastMinute) {
                                        ItemRGB = TAG_TTOT;
                                        strcpy_s(sItemString, 16, value.c_str());
                                    } else {
                                        ItemRGB = TAG_TTOT;
                                        strcpy_s(sItemString, 16, value.c_str());
                                    }
                                }
                            } else if (ItemCode == TAG_ITEM_ASAT) {
                                if (ASATFound) {
                                    if ((string)FlightPlan.GetGroundState() == "" ||
                                        (string)FlightPlan.GetGroundState() == "STUP" ||
                                        (string)FlightPlan.GetGroundState() == "ST-UP") {
                                        if (ASATPlusFiveLessTen) {
                                            ItemRGB = TAG_YELLOW;
                                            strcpy_s(sItemString, 16, ASATtext.c_str());
                                        } else {
                                            ItemRGB = TAG_GREEN;
                                            strcpy_s(sItemString, 16, ASATtext.c_str());
                                        }
                                    } else {
                                        ItemRGB = SU_SET_COLOR;
                                        strcpy_s(sItemString, 16, ASATtext.c_str());
                                    }
                                } else {
                                    ItemRGB = TAG_GREEN;
                                    strcpy_s(sItemString, 16, " ");
                                }
                            } else if (ItemCode == TAG_ITEM_ASRT) {
                                string ASRTtext = getFlightStripInfo(FlightPlan, 0);
                                if (!ASRTtext.empty()) {
                                    //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                    if (SU_ISSET) {
                                        ItemRGB = SU_SET_COLOR;
                                    } else {
                                        ItemRGB = TAG_ASRT;
                                    }
                                    strcpy_s(sItemString, 16, ASRTtext.c_str());
                                } else {
                                    //*pColorCode = TAG_COLOR_RGB_DEFINED;
                                    ItemRGB = TAG_ASRT;
                                    strcpy_s(sItemString, 16, " ");
                                }
                            } else if (ItemCode == TAG_ITEM_READYSTARTUP) {
                                string ASRTtext = getFlightStripInfo(FlightPlan, 0);
                                if (!ASRTtext.empty()) {
                                    ItemRGB = TAG_GREEN;
                                    strcpy_s(sItemString, 16, "RSTUP");
                                } else {
                                    ItemRGB = TAG_RED;
                                    strcpy_s(sItemString, 16, "RSTUP");
                                }
                            } else if (ItemCode == TAG_ITEM_E) {
                                if (notYetEOBT) {
                                    ItemRGB = TAG_GREEN;
                                    strcpy_s(sItemString, 16, "P");
                                } else {
                                    ItemRGB = TAG_GREEN;
                                    strcpy_s(sItemString, 16, "C");
                                }
                            } else if (ItemCode == TAG_ITEM_FLOW_MESSAGE) {
                                for (ServerRestricted sr : serverRestrictedPlanes) {
                                    if (sr.callsign == (string)FlightPlan.GetCallsign()) {
                                        ItemRGB = TAG_YELLOW;
                                        strcpy_s(sItemString, 50, sr.reason.c_str());
                                    }
                                }
                            } else if (ItemCode == TAG_ITEM_CTOT) {
                                for (ServerRestricted sr : serverRestrictedPlanes) {
                                    if (sr.callsign == (string)FlightPlan.GetCallsign()) {
                                        ItemRGB = TAG_CTOT;
                                        for (size_t i = 0; i < myNetworkStatus.size(); i++) {
                                            if (myNetworkStatus[i][0] == callsign && myNetworkStatus[i][1] == "REA" &&
                                                !ASATFound) {
                                                ItemRGB = TAG_YELLOW;
                                            }
                                        }
                                        strcpy_s(sItemString, 16, sr.ctot.c_str());
                                    }
                                }
                            } else if (ItemCode == NOW_CTOT_DIFF) {
                                for (ServerRestricted sr : serverRestrictedPlanes) {
                                    if (sr.callsign == (string)FlightPlan.GetCallsign()) {
                                        ItemRGB = TAG_CTOT;
                                        for (size_t i = 0; i < myNetworkStatus.size(); i++) {
                                            if (myNetworkStatus[i][0] == callsign && myNetworkStatus[i][1] == "REA" &&
                                                !ASATFound) {
                                                ItemRGB = TAG_YELLOW;
                                            }
                                        }
                                        string value = getDiffNowTime(sr.ctot, true, "");
                                        strcpy_s(sItemString, 16, value.c_str());
                                    }
                                }
                            } else if (ItemCode == TAG_ITEM_EV_CTOT) {
                                bool inEvCtotsList = false;
                                string slot = "";
                                for (size_t i = 0; i < evCtots.size(); i++) {
                                    if (evCtots[i][0] == callsign) {
                                        inEvCtotsList = true;
                                        slot = evCtots[i][1];
                                    }
                                }
                                if (inEvCtotsList) {
                                    ItemRGB = TAG_GREY;
                                    strcpy_s(sItemString, 16, slot.c_str());
                                }
                            } else if (ItemCode == TAG_ITEM_NETWORK_STATUS) {
                                string status = "";
                                for (size_t i = 0; i < myNetworkStatus.size(); i++) {
                                    if (myNetworkStatus[i][0] == callsign) {
                                        status = myNetworkStatus[i][1];
                                    }
                                }
                                if (status != "") {
                                    ItemRGB = TAG_YELLOW;
                                    if (status == "REA") {
                                        ItemRGB = TAG_YELLOW;
                                        strcpy_s(sItemString, 16, status.c_str());
                                    } else if (status.find("FLS") != string::npos) {
                                        ItemRGB = TAG_RED;
                                        status = GetTimedStatus(status);
                                        strcpy_s(sItemString, 16, status.c_str());
                                    }
                                }
                            } else if (ItemCode == TAG_ITEM_NETWORK_STATUS_AIRBORNE) {
                                string status = "";
                                for (size_t i = 0; i < myNetworkStatus.size(); i++) {
                                    if (myNetworkStatus[i][0] == callsign) {
                                        status = myNetworkStatus[i][1];
                                    }
                                }
                                if (status != "") {
                                    ItemRGB = TAG_YELLOW;
                                    if (status == "COMPLY") {
                                        ItemRGB = TAG_GREEN;
                                        status = "C";
                                        strcpy_s(sItemString, 16, status.c_str());
                                    } else if (status == "AIRB") {
                                        ItemRGB = TAG_RED;
                                        status = "A";
                                        strcpy_s(sItemString, 16, status.c_str());
                                    }
                                }
                            } else if (ItemCode == TAG_ITEM_ON_TIME_STATUS) {
                                string status = "";
                                for (size_t i = 0; i < onTimeStatus.size(); i++) {
                                    if (onTimeStatus[i][0] == callsign) {
                                        status = onTimeStatus[i][1];
                                    }
                                }
                                if (status != "") {
                                    ItemRGB = TAG_YELLOW;
                                    if (status.find("+") != string::npos) {
                                        ItemRGB = TAG_RED;
                                    } else if (status.find("-") != string::npos) {
                                        ItemRGB = TAG_GREEN;
                                    } else {
                                        status = "";
                                    }
                                    strcpy_s(sItemString, 16, status.c_str());
                                }
                            } else if (ItemCode == TAG_ITEM_SEND_STATUS) {
                                ItemRGB = TAG_RED;
                                for (string flt : messagesSent) {
                                    if (flt == callsign) {
                                        ItemRGB = TAG_GREEN;
                                    }
                                }
                                strcpy_s(sItemString, 16, "SEND");
                            } else if (ItemCode == TAG_ITEM_SEND_STATUS_SHORT) {
                                ItemRGB = TAG_RED;
                                for (string flt : messagesSent) {
                                    if (flt == callsign) {
                                        ItemRGB = TAG_GREEN;
                                    }
                                }
                                strcpy_s(sItemString, 16, "S");
                            } else if (ItemCode == TAG_ITEM_DEICE) {
                                string status = "";
                                for (vector<string> deice : deiceList) {
                                    if (deice[0] == callsign) {
                                        status = deice[1];
                                    }
                                }
                                ItemRGB = TAG_YELLOW;
                                strcpy_s(sItemString, 16, status.c_str());
                            } else if (ItemCode == TAG_ITEM_TOBT_SETBY) {
                                string status = getFlightStripInfo(FlightPlan, 9);
                                if (status == "A") status = "ATC";
                                else if (status == "P") status = "PILOT";
                                ItemRGB = TAG_GREEN;
                                strcpy_s(sItemString, 16, status.c_str());
                            } else if (ItemCode == TAG_ITEM_TOBT_SETBY_SHORT) {
                                string status = getFlightStripInfo(FlightPlan, 9);
                                ItemRGB = TAG_GREEN;
                                if (status.length() > 1) {
                                    status = status.substr(0, 1);
                                }
                                strcpy_s(sItemString, 16, status.c_str());
                            }
                        } else {
                            if (ItemCode == TAG_ITEM_EOBT) {
                                string ShowEOBT =
                                    formatTime(FlightPlan.GetFlightPlanData().GetEstimatedDepartureTime());
                                ItemRGB = TAG_EOBT;
                                for (size_t i = 0; i < myNetworkStatus.size(); i++) {
                                    if (myNetworkStatus[i][0] == callsign) {
                                        if (myNetworkStatus[i][1].find("FLS") != string::npos) {
                                            ItemRGB = TAG_RED;
                                        }
                                        break;
                                    }
                                }
                                strcpy_s(sItemString, 16, ShowEOBT.c_str());
                            } else if (ItemCode == TAG_ITEM_TOBT) {
                                ItemRGB = TAG_GREY;
                                strcpy_s(sItemString, 16, "----");
                            } else if (ItemCode == TAG_ITEM_ETOBT) {
                                ItemRGB = TAG_GREY;
                                strcpy_s(sItemString, 16, "----");
                            } else if (ItemCode == TAG_ITEM_FLOW_MESSAGE) {
                                if (aircraftFind) {
                                    if (slotList[pos].hasManualCtot) {
                                        string message = "MAN ACT";
                                        if (slotList[pos].ctot != "") {
                                            message = slotList[pos].flowReason;
                                        }
                                        ItemRGB = TAG_YELLOW;
                                        strcpy_s(sItemString, 16, message.c_str());
                                    }
                                } else {
                                    for (ServerRestricted sr : serverRestrictedPlanes) {
                                        if (sr.callsign == (string)FlightPlan.GetCallsign()) {
                                            ItemRGB = TAG_YELLOW;
                                            strcpy_s(sItemString, 50, sr.reason.c_str());
                                        }
                                    }
                                }
                            } else if (ItemCode == TAG_ITEM_CTOT) {
                                if (aircraftFind) {
                                    if (slotList[pos].hasManualCtot) {
                                        string value = "";
                                        if (slotList[pos].ctot == "") {
                                            value = slotList[pos].ttot.substr(0, 4);
                                            ItemRGB = TAG_ORANGE;
                                        } else {
                                            value = slotList[pos].ctot;
                                            ItemRGB = TAG_CTOT;
                                            for (size_t i = 0; i < myNetworkStatus.size(); i++) {
                                                if (myNetworkStatus[i][0] == callsign &&
                                                    myNetworkStatus[i][1] == "REA") {
                                                    ItemRGB = TAG_YELLOW;
                                                }
                                            }
                                        }
                                        strcpy_s(sItemString, 16, value.c_str());
                                    }
                                } else {
                                    for (ServerRestricted sr : serverRestrictedPlanes) {
                                        if (sr.callsign == (string)FlightPlan.GetCallsign()) {
                                            ItemRGB = TAG_CTOT;
                                            for (size_t i = 0; i < myNetworkStatus.size(); i++) {
                                                if (myNetworkStatus[i][0] == callsign &&
                                                    myNetworkStatus[i][1] == "REA") {
                                                    ItemRGB = TAG_YELLOW;
                                                }
                                            }
                                            strcpy_s(sItemString, 16, sr.ctot.c_str());
                                        }
                                    }
                                }
                            } else if (ItemCode == NOW_CTOT_DIFF) {
                                bool inreaList = false;
                                for (string s : reaCTOTSent) {
                                    if (s == callsign) {
                                        inreaList = true;
                                    }
                                }

                                if (aircraftFind) {
                                    if (slotList[pos].hasManualCtot) {
                                        string ctotSource = slotList[pos].ttot;
                                        if (slotList[pos].ctot != "") ctotSource = slotList[pos].ctot;
                                        string value = getDiffNowTime(ctotSource, true, "");
                                        if (slotList[pos].ctot == "") {
                                            ItemRGB = TAG_ORANGE;
                                        } else {
                                            ItemRGB = TAG_CTOT;
                                            for (size_t i = 0; i < myNetworkStatus.size(); i++) {
                                                if (myNetworkStatus[i][0] == callsign &&
                                                    myNetworkStatus[i][1] == "REA") {
                                                    ItemRGB = TAG_YELLOW;
                                                }
                                            }
                                        }
                                        strcpy_s(sItemString, 16, value.c_str());
                                    }
                                }
                            } else if (ItemCode == TAG_ITEM_EV_CTOT) {
                                bool inEvCtotsList = false;
                                string slot = "";
                                for (size_t i = 0; i < evCtots.size(); i++) {
                                    if (evCtots[i][0] == callsign) {
                                        inEvCtotsList = true;
                                        slot = evCtots[i][1];
                                    }
                                }
                                if (inEvCtotsList) {
                                    ItemRGB = TAG_GREY;
                                    strcpy_s(sItemString, 16, slot.c_str());
                                }
                            } else if (ItemCode == TAG_ITEM_NETWORK_STATUS) {
                                string status = "";
                                for (size_t i = 0; i < myNetworkStatus.size(); i++) {
                                    if (myNetworkStatus[i][0] == callsign) {
                                        status = myNetworkStatus[i][1];
                                    }
                                }
                                if (status != "") {
                                    ItemRGB = TAG_YELLOW;
                                    if (status == "REA") {
                                        ItemRGB = TAG_YELLOW;
                                        strcpy_s(sItemString, 16, status.c_str());
                                    } else if (status.find("FLS") != string::npos) {
                                        ItemRGB = TAG_RED;
                                        status = GetTimedStatus(status);
                                        strcpy_s(sItemString, 16, status.c_str());
                                    }
                                }
                            } else if (ItemCode == TAG_ITEM_NETWORK_STATUS_AIRBORNE) {
                                string status = "";
                                for (size_t i = 0; i < myNetworkStatus.size(); i++) {
                                    if (myNetworkStatus[i][0] == callsign) {
                                        status = myNetworkStatus[i][1];
                                    }
                                }
                                if (status != "") {
                                    ItemRGB = TAG_YELLOW;
                                    if (status == "COMPLY") {
                                        ItemRGB = TAG_GREEN;
                                        status = "C";
                                        strcpy_s(sItemString, 16, status.c_str());
                                    } else if (status == "AIRB") {
                                        ItemRGB = TAG_RED;
                                        status = "A";
                                        strcpy_s(sItemString, 16, status.c_str());
                                    }
                                }
                            } else if (ItemCode == TAG_ITEM_ON_TIME_STATUS) {
                                string status = "";
                                for (size_t i = 0; i < onTimeStatus.size(); i++) {
                                    if (onTimeStatus[i][0] == callsign) {
                                        status = onTimeStatus[i][1];
                                    }
                                }
                                if (status != "") {
                                    ItemRGB = TAG_YELLOW;
                                    if (status.find("+") != string::npos) {
                                        ItemRGB = TAG_RED;
                                    } else if (status.find("-") != string::npos) {
                                        ItemRGB = TAG_GREEN;
                                    } else {
                                        status = "";
                                    }
                                    strcpy_s(sItemString, 16, status.c_str());
                                }
                            } else if (ItemCode == TAG_ITEM_SEND_STATUS) {
                                ItemRGB = TAG_RED;
                                for (string flt : messagesSent) {
                                    if (flt == callsign) {
                                        ItemRGB = TAG_GREEN;
                                    }
                                }
                                strcpy_s(sItemString, 16, "SEND");
                            } else if (ItemCode == TAG_ITEM_SEND_STATUS_SHORT) {
                                ItemRGB = TAG_RED;
                                for (string flt : messagesSent) {
                                    if (flt == callsign) {
                                        ItemRGB = TAG_GREEN;
                                    }
                                }
                                strcpy_s(sItemString, 16, "S");
                            } else if (ItemCode == TAG_ITEM_DEICE) {
                                string status = "";
                                for (vector<string> deice : deiceList) {
                                    if (deice[0] == callsign) {
                                        status = deice[1];
                                    }
                                }
                                ItemRGB = TAG_YELLOW;
                                strcpy_s(sItemString, 16, status.c_str());
                            } else if (ItemCode == TAG_ITEM_TOBT_SETBY) {
                                string status = getFlightStripInfo(FlightPlan, 9);
                                if (status == "A") status = "ATC";
                                else if (status == "P") status = "PILOT";
                                ItemRGB = TAG_GREEN;
                                strcpy_s(sItemString, 16, status.c_str());
                            } else if (ItemCode == TAG_ITEM_TOBT_SETBY_SHORT) {
                                string status = getFlightStripInfo(FlightPlan, 9);
                                ItemRGB = TAG_GREEN;
                                if (status.length() > 1) {
                                    status = status.substr(0, 1);
                                }
                                strcpy_s(sItemString, 16, status.c_str());
                            }
                        }
                    }
                } else {
                    string EOBTstring = FlightPlan.GetFlightPlanData().GetEstimatedDepartureTime();
                    string EOBTfinal = formatTime(EOBTstring);

                    if (ItemCode == TAG_ITEM_EOBT) {
                        ItemRGB = TAG_EOBT;
                        for (size_t i = 0; i < myNetworkStatus.size(); i++) {
                            if (myNetworkStatus[i][0] == callsign) {
                                if (myNetworkStatus[i][1].find("FLS") != string::npos) {
                                    ItemRGB = TAG_RED;
                                }
                                break;
                            }
                        }
                        strcpy_s(sItemString, 16, EOBTfinal.c_str());
                    } else if (ItemCode == TAG_ITEM_TOBT) {
                        ItemRGB = TAG_GREY;
                        strcpy_s(sItemString, 16, "----");
                    } else if (ItemCode == TAG_ITEM_ETOBT) {
                        ItemRGB = TAG_GREY;
                        strcpy_s(sItemString, 16, "----");
                    } else if (ItemCode == TAG_ITEM_FLOW_MESSAGE) {
                        if (aircraftFind) {
                            if (slotList[pos].hasManualCtot) {
                                string message = "MAN ACT";
                                if (slotList[pos].ctot != "") {
                                    message = slotList[pos].flowReason;
                                }
                                ItemRGB = TAG_YELLOW;
                                strcpy_s(sItemString, 16, message.c_str());
                            }
                        } else {
                            for (ServerRestricted sr : serverRestrictedPlanes) {
                                if (sr.callsign == (string)FlightPlan.GetCallsign()) {
                                    ItemRGB = TAG_YELLOW;
                                    strcpy_s(sItemString, 50, sr.reason.c_str());
                                }
                            }
                        }
                    } else if (ItemCode == TAG_ITEM_CTOT) {
                        if (aircraftFind) {
                            if (slotList[pos].hasManualCtot) {
                                string value = "";
                                if (slotList[pos].ctot == "") {
                                    value = slotList[pos].ttot.substr(0, 4);
                                    ItemRGB = TAG_ORANGE;
                                } else {
                                    value = slotList[pos].ctot;
                                    ItemRGB = TAG_CTOT;
                                    for (size_t i = 0; i < myNetworkStatus.size(); i++) {
                                        if (myNetworkStatus[i][0] == callsign && myNetworkStatus[i][1] == "REA") {
                                            ItemRGB = TAG_YELLOW;
                                        }
                                    }
                                }
                                strcpy_s(sItemString, 16, value.c_str());
                            }
                        } else {
                            for (ServerRestricted sr : serverRestrictedPlanes) {
                                if (sr.callsign == (string)FlightPlan.GetCallsign()) {
                                    ItemRGB = TAG_CTOT;
                                    for (size_t i = 0; i < myNetworkStatus.size(); i++) {
                                        if (myNetworkStatus[i][0] == callsign && myNetworkStatus[i][1] == "REA") {
                                            ItemRGB = TAG_YELLOW;
                                        }
                                    }
                                    strcpy_s(sItemString, 16, sr.ctot.c_str());
                                }
                            }
                        }
                    } else if (ItemCode == NOW_CTOT_DIFF) {
                        bool inreaList = false;
                        for (string s : reaCTOTSent) {
                            if (s == callsign) {
                                inreaList = true;
                            }
                        }

                        if (aircraftFind) {
                            if (slotList[pos].hasManualCtot) {
                                string ctotSource = slotList[pos].ttot;
                                if (slotList[pos].ctot != "") ctotSource = slotList[pos].ctot;
                                string value = getDiffNowTime(ctotSource, true, "");
                                if (slotList[pos].ctot == "") {
                                    ItemRGB = TAG_ORANGE;
                                } else {
                                    ItemRGB = TAG_CTOT;
                                    for (size_t i = 0; i < myNetworkStatus.size(); i++) {
                                        if (myNetworkStatus[i][0] == callsign && myNetworkStatus[i][1] == "REA") {
                                            ItemRGB = TAG_YELLOW;
                                        }
                                    }
                                }
                                strcpy_s(sItemString, 16, value.c_str());
                            }
                        }
                    } else if (ItemCode == TAG_ITEM_EV_CTOT) {
                        bool inEvCtotsList = false;
                        string slot = "";
                        for (size_t i = 0; i < evCtots.size(); i++) {
                            if (evCtots[i][0] == callsign) {
                                inEvCtotsList = true;
                                slot = evCtots[i][1];
                            }
                        }
                        if (inEvCtotsList) {
                            ItemRGB = TAG_GREY;
                            strcpy_s(sItemString, 16, slot.c_str());
                        }
                    } else if (ItemCode == TAG_ITEM_NETWORK_STATUS) {
                        string status = "";
                        for (size_t i = 0; i < myNetworkStatus.size(); i++) {
                            if (myNetworkStatus[i][0] == callsign) {
                                status = myNetworkStatus[i][1];
                            }
                        }
                        if (status != "") {
                            ItemRGB = TAG_YELLOW;
                            if (status == "REA") {
                                ItemRGB = TAG_YELLOW;
                                strcpy_s(sItemString, 16, status.c_str());
                            } else if (status.find("FLS") != string::npos) {
                                ItemRGB = TAG_RED;
                                status = GetTimedStatus(status);
                                strcpy_s(sItemString, 16, status.c_str());
                            }
                        }
                    } else if (ItemCode == TAG_ITEM_NETWORK_STATUS_AIRBORNE) {
                        string status = "";
                        for (size_t i = 0; i < myNetworkStatus.size(); i++) {
                            if (myNetworkStatus[i][0] == callsign) {
                                status = myNetworkStatus[i][1];
                            }
                        }
                        if (status != "") {
                            ItemRGB = TAG_YELLOW;
                            if (status == "COMPLY") {
                                ItemRGB = TAG_GREEN;
                                status = "C";
                                strcpy_s(sItemString, 16, status.c_str());
                            } else if (status == "AIRB") {
                                ItemRGB = TAG_RED;
                                status = "A";
                                strcpy_s(sItemString, 16, status.c_str());
                            }
                        }
                    } else if (ItemCode == TAG_ITEM_ON_TIME_STATUS) {
                        string status = "";
                        for (size_t i = 0; i < onTimeStatus.size(); i++) {
                            if (onTimeStatus[i][0] == callsign) {
                                status = onTimeStatus[i][1];
                            }
                        }
                        if (status != "") {
                            ItemRGB = TAG_YELLOW;
                            if (status.find("+") != string::npos) {
                                ItemRGB = TAG_RED;
                            } else if (status.find("-") != string::npos) {
                                ItemRGB = TAG_GREEN;
                            } else {
                                status = "";
                            }
                            strcpy_s(sItemString, 16, status.c_str());
                        }
                    } else if (ItemCode == TAG_ITEM_SEND_STATUS) {
                        ItemRGB = TAG_RED;
                        for (string flt : messagesSent) {
                            if (flt == callsign) {
                                ItemRGB = TAG_GREEN;
                            }
                        }
                        strcpy_s(sItemString, 16, "SEND");
                    } else if (ItemCode == TAG_ITEM_SEND_STATUS_SHORT) {
                        ItemRGB = TAG_RED;
                        for (string flt : messagesSent) {
                            if (flt == callsign) {
                                ItemRGB = TAG_GREEN;
                            }
                        }
                        strcpy_s(sItemString, 16, "S");
                    } else if (ItemCode == TAG_ITEM_DEICE) {
                        string status = "";
                        for (vector<string> deice : deiceList) {
                            if (deice[0] == callsign) {
                                status = deice[1];
                            }
                        }
                        ItemRGB = TAG_YELLOW;
                        strcpy_s(sItemString, 16, status.c_str());
                    } else if (ItemCode == TAG_ITEM_TOBT_SETBY) {
                        string status = getFlightStripInfo(FlightPlan, 9);
                        if (status == "A") status = "ATC";
                        else if (status == "P") status = "PILOT";
                        ItemRGB = TAG_GREEN;
                        strcpy_s(sItemString, 16, status.c_str());
                    } else if (ItemCode == TAG_ITEM_TOBT_SETBY_SHORT) {
                        string status = getFlightStripInfo(FlightPlan, 9);
                        ItemRGB = TAG_GREEN;
                        if (status.length() > 1) {
                            status = status.substr(0, 1);
                        }
                        strcpy_s(sItemString, 16, status.c_str());
                    }
                }

                if (ItemRGB != 0xFFFFFFFF) {
                    *pColorCode = TAG_COLOR_RGB_DEFINED;
                    *pRGB = ItemRGB;
                }
            } else {
                // Check if update in the queue
                std::vector<Plane> localPlaneQueue;
                {
                    std::lock_guard<std::mutex> lock(apiQueueResponseMutex);
                    localPlaneQueue.swap(apiQueueResponse);
                    apiQueueResponse.clear();
                }

                if (!localPlaneQueue.empty()) {
                    for (const Plane p : localPlaneQueue) {
                        for (int t = 0; t < slotList.size(); t++) {
                            if (p.callsign == slotList[t].callsign) {
                                slotList[t] = p;
                            }
                        }
                    }
                }

                bool master = false;
                for (string apt : masterAirports) {
                    if (apt == FlightPlan.GetFlightPlanData().GetOrigin()) {
                        master = true;
                    }
                }

                string EOBTstring = FlightPlan.GetFlightPlanData().GetEstimatedDepartureTime();
                string EOBTfinal = formatTime(EOBTstring);

                int slotListPos = -1;
                for (int i = 0; i < slotList.size(); i++) {
                    if (slotList[i].callsign == (string)FlightPlan.GetCallsign()) {
                        slotListPos = i;
                    }
                }

                if (slotListPos == -1) {
                    Plane p(callsign, EOBTfinal, EOBTfinal, EOBTfinal, "", "", false, true, false);
                    slotList.push_back(p);
                }

                if (master) {
                    // Sync data
                    if (slotListPos != -1) {
                        // Update TTOT to Slaves
                        if (slotList[slotListPos].ttot != getFlightStripInfo(FlightPlan, 4)) {
                            setFlightStripInfo(FlightPlan, slotList[slotListPos].ttot, 4);
                        }
                        // Update Manual CTOT to Slaves
                        if (slotList[slotListPos].hasManualCtot == true && getFlightStripInfo(FlightPlan, 7) != "1") {
                            setFlightStripInfo(FlightPlan, "1", 7);
                        } else if (slotList[slotListPos].hasManualCtot == false &&
                                   getFlightStripInfo(FlightPlan, 7) != "") {
                            setFlightStripInfo(FlightPlan, "", 7);
                        }

                        // Push to other ATCs
                        if ((timeNow - countTimeNonCdm) > refreshTime) {
                            countTimeNonCdm = timeNow;
                            addLogLine("[AUTO] - REFRESH CDM INTERNAL DATA (Non-CDM)");
                            for (size_t t = 0; t < slotList.size(); t++) {
                                CFlightPlan fpSelected = FlightPlanSelect(slotList[t].callsign.c_str());
                                PushToOtherControllers(fpSelected);
                            }
                        }
                    }
                }

                // Get Time NOW
                time_t rawtime;
                struct tm ptm;
                time(&rawtime);
                gmtime_s(&ptm, &rawtime);
                string hour = to_string(ptm.tm_hour % 24);
                string min = to_string(ptm.tm_min);

                // Set/Remove AOBT automaically base on state
                bool ASATFound = false;
                int ASATpos = 0;
                bool correctState = false;
                string ASATtext = " ";
                for (size_t x = 0; x < asatList.size(); x++) {
                    string actualListCallsign = asatList[x].substr(0, asatList[x].find(","));
                    if (actualListCallsign == callsign) {
                        ASATFound = true;
                        ASATpos = x;
                    }
                }

                if ((string)FlightPlan.GetGroundState() == "STUP" || (string)FlightPlan.GetGroundState() == "ST-UP" ||
                    (string)FlightPlan.GetGroundState() == "PUSH" || (string)FlightPlan.GetGroundState() == "TAXI" ||
                    (string)FlightPlan.GetGroundState() == "DEPA") {
                    correctState = true;
                }

                if (!ASATFound) {
                    if (correctState) {
                        ASATtext = formatTime(hour + min);
                        asatList.push_back(callsign + "," + ASATtext.substr(0, 4));
                        runDetachedTask(&CDM::setCdmSts, callsign, "AOBT/" + ASATtext);
                        ASATFound = true;
                    }
                } else {
                    if (correctState) {
                        ASATtext = asatList[ASATpos].substr(asatList[ASATpos].length() - 4, 4);
                    } else if (!correctState) {
                        asatList.erase(asatList.begin() + ASATpos);
                        runDetachedTask(&CDM::setCdmSts, callsign, "AOBT/NULL");
                        ASATFound = false;
                    }
                }

                if (ItemCode == TAG_ITEM_EOBT) {
                    ItemRGB = TAG_EOBT;
                    for (size_t i = 0; i < myNetworkStatus.size(); i++) {
                        if (myNetworkStatus[i][0] == callsign) {
                            if (myNetworkStatus[i][1].find("FLS") != string::npos) {
                                ItemRGB = TAG_RED;
                            }
                            break;
                        }
                    }
                    string eobtValue = EOBTfinal;
                    for (vector<string> obtItem : obtList) {
                        if (obtItem[0] == callsign && obtItem[1] != "") {
                            eobtValue = obtItem[1];
                            break;
                        }
                    }
                    strcpy_s(sItemString, 16, eobtValue.c_str());
                }
                if (ItemCode == TAG_ITEM_ETOBT) {
                    ItemRGB = TAG_EOBT;
                    for (size_t i = 0; i < myNetworkStatus.size(); i++) {
                        if (myNetworkStatus[i][0] == callsign) {
                            if (myNetworkStatus[i][1].find("FLS") != string::npos) {
                                ItemRGB = TAG_RED;
                            }
                            break;
                        }
                    }
                    string eobtValue = EOBTfinal;
                    for (vector<string> obtItem : obtList) {
                        if (obtItem[0] == callsign && obtItem[1] != "") {
                            eobtValue = obtItem[1];
                            break;
                        }
                    }
                    strcpy_s(sItemString, 16, eobtValue.c_str());
                }
                if (ItemCode == TAG_ITEM_CTOT) {
                    for (ServerRestricted sr : serverRestrictedPlanes) {
                        if (sr.callsign == (string)FlightPlan.GetCallsign()) {
                            ItemRGB = TAG_CTOT;
                            for (size_t i = 0; i < myNetworkStatus.size(); i++) {
                                if (myNetworkStatus[i][0] == callsign && myNetworkStatus[i][1] == "REA" && !ASATFound) {
                                    ItemRGB = TAG_YELLOW;
                                }
                            }
                            strcpy_s(sItemString, 16, sr.ctot.c_str());
                        }
                    }
                } else if (ItemCode == NOW_CTOT_DIFF) {
                    for (ServerRestricted sr : serverRestrictedPlanes) {
                        if (sr.callsign == (string)FlightPlan.GetCallsign()) {
                            string value = getDiffNowTime(sr.ctot, true, "");
                            ItemRGB = TAG_CTOT;
                            for (size_t i = 0; i < myNetworkStatus.size(); i++) {
                                if (myNetworkStatus[i][0] == callsign && myNetworkStatus[i][1] == "REA" && !ASATFound) {
                                    ItemRGB = TAG_YELLOW;
                                }
                            }
                            strcpy_s(sItemString, 16, value.c_str());
                        }
                    }
                }
                if (ItemCode == TAG_ITEM_FLOW_MESSAGE) {
                    for (ServerRestricted sr : serverRestrictedPlanes) {
                        if (sr.callsign == (string)FlightPlan.GetCallsign()) {
                            ItemRGB = TAG_YELLOW;
                            strcpy_s(sItemString, 50, sr.reason.c_str());
                        }
                    }
                } else if (ItemCode == TAG_ITEM_NETWORK_STATUS) {
                    string status = "";
                    for (size_t i = 0; i < myNetworkStatus.size(); i++) {
                        if (myNetworkStatus[i][0] == callsign) {
                            status = myNetworkStatus[i][1];
                        }
                    }
                    if (status != "") {
                        ItemRGB = TAG_YELLOW;
                        if (status == "REA") {
                            ItemRGB = TAG_YELLOW;
                            strcpy_s(sItemString, 16, status.c_str());
                        } else if (status.find("FLS") != string::npos) {
                            ItemRGB = TAG_RED;
                            status = GetTimedStatus(status);
                            strcpy_s(sItemString, 16, status.c_str());
                        }
                    }
                } else if (ItemCode == TAG_ITEM_NETWORK_STATUS_AIRBORNE) {
                    string status = "";
                    for (size_t i = 0; i < myNetworkStatus.size(); i++) {
                        if (myNetworkStatus[i][0] == callsign) {
                            status = myNetworkStatus[i][1];
                        }
                    }
                    if (status != "") {
                        ItemRGB = TAG_YELLOW;
                        if (status == "COMPLY") {
                            ItemRGB = TAG_GREEN;
                            status = "C";
                            strcpy_s(sItemString, 16, status.c_str());
                        } else if (status == "AIRB") {
                            ItemRGB = TAG_RED;
                            status = "A";
                            strcpy_s(sItemString, 16, status.c_str());
                        }
                    }
                } else if (ItemCode == TAG_ITEM_ON_TIME_STATUS) {
                    string status = "";
                    for (size_t i = 0; i < onTimeStatus.size(); i++) {
                        if (onTimeStatus[i][0] == callsign) {
                            status = onTimeStatus[i][1];
                        }
                    }
                    if (status != "") {
                        ItemRGB = TAG_YELLOW;
                        if (status.find("+") != string::npos) {
                            ItemRGB = TAG_RED;
                        } else if (status.find("-") != string::npos) {
                            ItemRGB = TAG_GREEN;
                        } else {
                            status = "";
                        }
                        strcpy_s(sItemString, 16, status.c_str());
                    }
                } else if (ItemCode == TAG_ITEM_SEND_STATUS) {
                    ItemRGB = TAG_RED;
                    for (string flt : messagesSent) {
                        if (flt == callsign) {
                            ItemRGB = TAG_GREEN;
                        }
                    }
                    strcpy_s(sItemString, 16, "SEND");
                } else if (ItemCode == TAG_ITEM_SEND_STATUS_SHORT) {
                    ItemRGB = TAG_RED;
                    for (string flt : messagesSent) {
                        if (flt == callsign) {
                            ItemRGB = TAG_GREEN;
                        }
                    }
                    strcpy_s(sItemString, 16, "S");
                }
                if (ItemRGB != 0xFFFFFFFF) {
                    *pColorCode = TAG_COLOR_RGB_DEFINED;
                    *pRGB = ItemRGB;
                }
            }
        }

        // Refresh the display cache with this call's finished result for reuse within the same second.
        TagItemCacheEntry& cacheEntryToStore = tagItemDisplayCache[callsign][ItemCode];
        cacheEntryToStore.tick = tagCacheNow;
        memcpy(cacheEntryToStore.text, sItemString, sizeof(cacheEntryToStore.text));
        cacheEntryToStore.colorSet = (ItemRGB != 0xFFFFFFFF);
        if (cacheEntryToStore.colorSet) {
            cacheEntryToStore.colorCode = *pColorCode;
            cacheEntryToStore.rgb = *pRGB;
        }
    } catch (const std::exception& e) {
        addLogLine("ERROR: Unhandled exception OnGetTagItem: " + (string)e.what());
    } catch (...) {
        addLogLine("ERROR: Unhandled exception OnGetTagItem");
    }
}

