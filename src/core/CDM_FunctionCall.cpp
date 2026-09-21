// Tag function (right-click menu / popup) handlers, moved verbatim out of CDMSingle.cpp.
// See src/core/CDMGlobals.hpp for the shared-state rationale.

#include "CDMSingle.hpp"
#include "src/core/CDMGlobals.hpp"

#include <thread>

void CDM::OnFunctionCall(int FunctionId, const char* ItemString, POINT Pt, RECT Area) {
    try {
        if (FunctionId == TAG_FUNC_NEW_MASTER_AIRPORT && ItemString && strlen(ItemString) > 0) {
            std::string airport_icao = ItemString;
            if (lastAddedIcao.empty()) {
                lastAddedIcao = airport_icao;
                addMasterAirport(airport_icao);
            }
        } else if (FunctionId == TAG_FUNC_RELEVANT_FLIGHTS_FILTER) {
            flightsFilterText = ItemString;
        }

        // FP Required functions
        CFlightPlan fp = FlightPlanSelectASEL();
        if (!fp.IsValid()) {
            return;
        }
        bool AtcMe = false;
        bool master = false;

        for (string apt : masterAirports) {
            if (apt == fp.GetFlightPlanData().GetOrigin()) {
                master = true;
            }
        }
        AtcMe = false;
        string position = ControllerMyself().GetCallsign();
        bool isPositionOk = position.find("_DEL") != string::npos || position.find("_GND") != string::npos ||
                            position.find("_TWR") != string::npos || position.find("_APP") != string::npos ||
                            position.find("_CTR") != string::npos || position.find("_FMP") != string::npos;
        if ((fp.GetTrackingControllerIsMe() || strlen(fp.GetTrackingControllerId()) == 0) && isPositionOk) {
            AtcMe = true;
        }

        if (FunctionId == TAG_FUNC_PM_SEND) {
            sendCdmMessageToPilot(fp.GetCallsign());
        } else if (FunctionId == TAG_FUNC_EDITEOBT) {
            // Can be modified as non-master
            if (AtcMe) {
                addLogLine("TRIGGER - TAG_FUNC_EDITEOBT");
                OpenPopupEdit(Area, TAG_FUNC_NEWEOBT, fp.GetFlightPlanData().GetEstimatedDepartureTime());
            }
        } else if (FunctionId == TAG_FUNC_NEWEOBT) {
            addLogLine("TRIGGER - TAG_FUNC_NEWEOBT");
            string editedEOBT = ItemString;
            bool hasNoNumber = true;
            if (editedEOBT.length() <= 4) {
                for (size_t i = 0; i < editedEOBT.length(); i++) {
                    if (isdigit(editedEOBT[i]) == false) {
                        hasNoNumber = false;
                    }
                }
                if (hasNoNumber) {
                    fp.GetFlightPlanData().SetEstimatedDepartureTime(editedEOBT.c_str());
                    fp.GetFlightPlanData().AmendFlightPlan();
                    if (editedEOBT.length() == 4) {
                        for (int u = 0; u < obtList.size(); u++) {
                            if (obtList[u][0] == fp.GetCallsign()) {
                                obtList[u][1] = editedEOBT;
                                break;
                            }
                        }
                        // Set EOBT in API
                        std::thread t(&CDM::setOBTApi, this, fp.GetCallsign(), editedEOBT, true, true);
                        t.detach();
                    }
                }
            }
        } else if (FunctionId == TAG_FUNC_READYEOBT) {
            try {
                if ((string)fp.GetGroundState() != "STUP" && (string)fp.GetGroundState() != "ST-UP" &&
                    (string)fp.GetGroundState() != "PUSH" && (string)fp.GetGroundState() != "TAXI" &&
                    (string)fp.GetGroundState() != "DEPA") {
                        addLogLine("TRIGGER - TAG_FUNC_READYEOBT");
                        // SET SU_WAIT WHEN OPTION ENABLED
                        if (option_su_wait) {
                            suWaitList.push_back(fp.GetCallsign());
                        }

                        if (getFlightStripInfo(fp, 2) != formatTime(GetActualTime())) {
                            setFlightStripInfo(fp, formatTime(GetActualTime()), 2);

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

                            fp.GetFlightPlanData().SetEstimatedDepartureTime((hour + min).c_str());
                            fp.GetFlightPlanData().AmendFlightPlan();
                            if ((hour + min).length() == 4) {
                                for (int u = 0; u < obtList.size(); u++) {
                                    if (obtList[u][0] == fp.GetCallsign()) {
                                        obtList[u][1] = hour + min;
                                        break;
                                    }
                                }
                                // Set EOBT in API
                                std::thread t(&CDM::setOBTApi, this, fp.GetCallsign(), hour + min, true, true);
                                t.detach();
                            }

                            // Set REA Status
                            std::thread t99(&CDM::setCdmSts, this, fp.GetCallsign(), "REA/1");
                            t99.detach();
                        }
                }
            } catch (const std::exception& ex) {
                addLogLine(string("EXCEPTION in TAG_FUNC_READYEOBT: ") + ex.what());
            } catch (...) {
                addLogLine("UNKNOWN EXCEPTION in TAG_FUNC_READYEOBT");
            }
        } else if (FunctionId == TAG_FUNC_EOBTTOTOBT) {
            if ((string)fp.GetGroundState() != "STUP" && (string)fp.GetGroundState() != "ST-UP" &&
                (string)fp.GetGroundState() != "PUSH" && (string)fp.GetGroundState() != "TAXI" &&
                (string)fp.GetGroundState() != "DEPA") {
                if (master && AtcMe) {
                    addLogLine("TRIGGER - TAG_FUNC_EOBTTOTOBT");
                    setFlightStripInfo(fp, formatTime(fp.GetFlightPlanData().GetEstimatedDepartureTime()), 2);
                }
            }
        } else if (FunctionId == TAG_FUNC_ADDTSAC) {
            addLogLine("TRIGGER - TAG_FUNC_ADDTSAC");
            string completeTOBT = getFlightStripInfo(fp, 2);
            if (!completeTOBT.empty()) {
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

                bool notYetTOBT = false;
                string TOBThour = completeTOBT.substr(0, 2);
                string TOBTmin = completeTOBT.substr(2, 2);

                if (hour != "00") {
                    if (TOBThour == "00") {
                        TOBThour = "24";
                    }
                }

                int EOBTdifTime = GetdifferenceTime(hour, min, TOBThour, TOBTmin);
                if (hour != TOBThour) {
                    if (EOBTdifTime < -75) {
                        notYetTOBT = true;
                    }
                } else {
                    if (EOBTdifTime < -35) {
                        notYetTOBT = true;
                    }
                }
                if (!notYetTOBT) {
                    for (size_t a = 0; a < slotList.size(); a++) {
                        if (slotList[a].callsign == fp.GetCallsign()) {
                            string getTSAT = slotList[a].tsat;
                            if (getTSAT.length() >= 4) {
                                setFlightStripInfo(fp, getTSAT.substr(0, 4), 1);
                            }
                        }
                    }
                }
            }
        }

        else if (FunctionId == TAG_FUNC_REMOVETSAC) {
            addLogLine("TRIGGER - TAG_FUNC_REMOVETSAC");
            setFlightStripInfo(fp, "", 1);
        }

        else if (FunctionId == TAG_FUNC_EDITTSAC) {
            addLogLine("TRIGGER - TAG_FUNC_EDITTSAC");
            OpenPopupEdit(Area, TAG_FUNC_NEWTSAC, getFlightStripInfo(fp, 1).c_str());
        }

        else if (FunctionId == TAG_FUNC_NEWTSAC) {
            addLogLine("TRIGGER - TAG_FUNC_NEWTSAC");
            string editedTSAC = ItemString;
            if (editedTSAC.length() > 0) {
                bool hasNoNumber = true;
                if (editedTSAC.length() == 4) {
                    for (size_t i = 0; i < editedTSAC.length(); i++) {
                        if (isdigit(editedTSAC[i]) == false) {
                            hasNoNumber = false;
                        }
                    }
                    if (hasNoNumber) {
                        setFlightStripInfo(fp, editedTSAC, 1);
                    }
                }
            }
        }

        else if (FunctionId == TAG_FUNC_TOGGLEASRT || FunctionId == TAG_FUNC_READYSTARTUP ||
                 FunctionId == TAG_FUNC_TOGGLEASRTREA) {
            if (master && AtcMe) {
                addLogLine("TRIGGER - TAG_FUNC_READYSTARTUP");
                string annotAsrt = getFlightStripInfo(fp, 0);
                if (annotAsrt.empty()) {
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

                    setFlightStripInfo(fp, (hour + min), 0);
                    if (FunctionId == TAG_FUNC_TOGGLEASRTREA) {
                        std::thread t74(&CDM::setCdmSts, this, fp.GetCallsign(), "REA/1");
                        t74.detach();
                    }
                } else {
                    setFlightStripInfo(fp, "", 0);
                }
            }
        }

        else if (FunctionId == TAG_FUNC_FMASTEXT) {
            addLogLine("TRIGGER - TAG_FUNC_FMASTEXT");
            bool found = false;
            for (size_t i = 0; i < slotList.size(); i++) {
                if (slotList[i].callsign == fp.GetCallsign()) {
                    if (slotList[i].hasManualCtot) {
                        sendMessage(slotList[i].callsign + " FM -> " + slotList[i].flowReason);
                        found = true;
                    }
                }
            }

            if (!found) {
                for (ServerRestricted sr : serverRestrictedPlanes) {
                    if (sr.callsign == (string)fp.GetCallsign()) {
                        sendMessage(sr.callsign + " FM -> " + sr.reason);
                    }
                }
            }
        } else if (FunctionId == TAG_FUNC_OPT_DEICE) {
            if (master && AtcMe) {
                addLogLine("TRIGGER - TAG_FUNC_OPT_DEICE");

                OpenPopupList(Area, "De-Ice", 1);
                AddPopupListElement("NONE", "", TAG_FUNC_DEICE_NONE, false, 2, false);
                AddPopupListElement("STND", "", TAG_FUNC_DEICE_STAND, false, 2, false);
                if (deIceTaxiRem1Name != "") {
                    AddPopupListElement(deIceTaxiRem1Name.c_str(), "", TAG_FUNC_DEICE_REMOTE1, false, 2, false);
                }
                if (deIceTaxiRem2Name != "") {
                    AddPopupListElement(deIceTaxiRem2Name.c_str(), "", TAG_FUNC_DEICE_REMOTE2, false, 2, false);
                }
                if (deIceTaxiRem3Name != "") {
                    AddPopupListElement(deIceTaxiRem3Name.c_str(), "", TAG_FUNC_DEICE_REMOTE3, false, 2, false);
                }
                if (deIceTaxiRem4Name != "") {
                    AddPopupListElement(deIceTaxiRem4Name.c_str(), "", TAG_FUNC_DEICE_REMOTE4, false, 2, false);
                }
                if (deIceTaxiRem5Name != "") {
                    AddPopupListElement(deIceTaxiRem5Name.c_str(), "", TAG_FUNC_DEICE_REMOTE5, false, 2, false);
                }
            }
        } else if (FunctionId == TAG_FUNC_DEICE_NONE) {
            if (master && AtcMe) {
                addLogLine("TRIGGER - TAG_FUNC_DEICE_NONE");

                // Remove plane from deice list
                for (size_t i = 0; i < deiceList.size(); i++) {
                    if (deiceList[i][0] == fp.GetCallsign()) {
                        deiceList.erase(deiceList.begin() + i);
                    }
                }
                setFlightStripInfo(fp, "", 5);
                // Remove plane from taxiTimesList
                for (size_t j = 0; j < taxiTimesList.size(); j++) {
                    if (taxiTimesList[j].substr(0, taxiTimesList[j].find(",")) == fp.GetCallsign()) {
                        taxiTimesList.erase(taxiTimesList.begin() + j);
                    }
                }
                // Remove plane from slotlist to recalculate times
                for (size_t i = 0; i < slotList.size(); i++) {
                    if (slotList[i].callsign == fp.GetCallsign()) {
                        slotList.erase(slotList.begin() + i);
                    }
                }
            }
        } else if (FunctionId == TAG_FUNC_DEICE_STAND) {
            if (master && AtcMe) {
                setDeice("STND", fp, "STND");
            }
        } else if (FunctionId == TAG_FUNC_DEICE_REMOTE1) {
            if (master && AtcMe) {
                setDeice(deIceTaxiRem1Name, fp, "REM1");
            }
        } else if (FunctionId == TAG_FUNC_DEICE_REMOTE2) {
            if (master && AtcMe) {
                setDeice(deIceTaxiRem2Name, fp, "REM2");
            }
        } else if (FunctionId == TAG_FUNC_DEICE_REMOTE3) {
            if (master && AtcMe) {
                setDeice(deIceTaxiRem3Name, fp, "REM3");
            }
        } else if (FunctionId == TAG_FUNC_DEICE_REMOTE4) {
            if (master && AtcMe) {
                setDeice(deIceTaxiRem4Name, fp, "REM4");
            }
        } else if (FunctionId == TAG_FUNC_DEICE_REMOTE5) {
            if (master && AtcMe) {
                setDeice(deIceTaxiRem5Name, fp, "REM5");
            }
        } else if (FunctionId == TAG_FUNC_NETWORK_STATUS_OPTIONS) {
            if (AtcMe) {
                addLogLine("TRIGGER - TAG_FUNC_NETWORK_STATUS_OPTIONS");

                // Get actual status
                string status = "";
                std::lock_guard<std::mutex> lock(networkStatusMutex);
                for (size_t i = 0; i < networkStatus.size(); i++) {
                    if (networkStatus[i][0] == fp.GetCallsign()) {
                        status = networkStatus[i][1];
                    }
                }

                // Check has CTOT
                bool hasCtot = false;
                for (size_t i = 0; i < slotList.size(); i++) {
                    if (slotList[i].callsign == fp.GetCallsign()) {
                        if (slotList[i].hasManualCtot && slotList[i].ctot != "") {
                            hasCtot = true;
                        }
                    }
                }

                if (hasCtot && status == "") {
                    OpenPopupList(Area, "CDM-Network", 1);
                    if (status != "REA") {
                        AddPopupListElement("Set REA", "", TAG_FUNC_NETWORK_SET_REA, false, 2, false);
                    }
                } else if (status == "REA") {
                    OpenPopupList(Area, "CDM-Network", 1);
                    AddPopupListElement("Remove REA", "", TAG_FUNC_NETWORK_REMOVE_REA, false, 2, false);
                }
            }
        } else if (FunctionId == TAG_FUNC_NETWORK_SET_REA) {
            if (AtcMe) {
                addLogLine("TRIGGER - TAG_FUNC_NETWORK_SET_REA");
                std::thread t3(&CDM::setCdmSts, this, fp.GetCallsign(), "REA/1");
                t3.detach();
            }
        } else if (FunctionId == TAG_FUNC_NETWORK_REMOVE_REA) {
            if (AtcMe) {
                addLogLine("TRIGGER - TAG_FUNC_NETWORK_REMOVE_REA");
                std::thread t3(&CDM::setCdmSts, this, fp.GetCallsign(), "REA/0");
                t3.detach();
            }
        }

        else if (FunctionId == TAG_FUNC_CTOTOPTIONS) {
            if (master && AtcMe) {
                addLogLine("TRIGGER - TAG_FUNC_CTOTOPTIONS");
                Plane plane;
                for (size_t i = 0; i < slotList.size(); i++) {
                    if (slotList[i].callsign == fp.GetCallsign()) {
                        plane = slotList[i];
                    }
                }
                OpenPopupList(Area, "CTOT Options", 1);
                if (!plane.hasManualCtot || plane.ctot != "") {
                    AddPopupListElement("Set Manual CTOT", "", TAG_FUNC_EDITMANCTOT, false, 2, false);
                    if (flightHasCtotDisabled(fp.GetCallsign())) {
                        AddPopupListElement("Enable CDM-Network", "", TAG_FUNC_ENABLECTOT, false, 2, false);
                    } else {
                        AddPopupListElement("Disable CDM-Network", "", TAG_FUNC_DISABLECTOT, false, 2, false);
                    }
                } else if (plane.ctot == "" && plane.hasManualCtot) {
                    AddPopupListElement("Remove Manual CTOT", "", TAG_FUNC_REMOVEMANCTOT, false, 2, false);
                    if (flightHasCtotDisabled(fp.GetCallsign())) {
                        AddPopupListElement("Enable CDM-Network", "", TAG_FUNC_ENABLECTOT, false, 2, false);
                    } else {
                        AddPopupListElement("Disable CDM-Network", "", TAG_FUNC_DISABLECTOT, false, 2, false);
                    }
                }
            }
        } else if (FunctionId == TAG_FUNC_TOGGLEREAMSG) {
            addLogLine("TRIGGER - TAG_FUNC_TOGGLEREAMSG");
            toggleReaMsg(fp, true);
        } else if (FunctionId == TAG_FUNC_REMOVECTOT) {
            addLogLine("TRIGGER - TAG_FUNC_REMOVECTOT");
            for (size_t i = 0; i < slotList.size(); i++) {
                if (slotList[i].callsign == fp.GetCallsign()) {
                    slotList[i].ctot = "";
                }
            }
            // Update times to slaves
            countTime = std::time(nullptr) - (refreshTime + 5);
            countTimeNonCdm = std::time(nullptr) - (refreshTime + 5);
        }

        else if (FunctionId == TAG_FUNC_OPT_TTOT) {
            if (master && AtcMe) {
                addLogLine("TRIGGER - TAG_FUNC_OPT_TTOT");
                OpenPopupList(Area, "TTOT Options", 1);
                // CDT OPTIONS
                bool planeFound = false;
                Plane plane;
                for (size_t i = 0; i < slotList.size(); i++) {
                    if (slotList[i].callsign == fp.GetCallsign()) {
                        planeFound = true;
                        plane = slotList[i];
                    }
                }

                if (planeFound) {
                    if (plane.hasManualCtot) {
                        AddPopupListElement("Edit Custom CDT", "", TAG_FUNC_EDITCDT, false, 2, false);
                    } else {
                        AddPopupListElement("Set Custom CDT", "", TAG_FUNC_EDITCDT, false, 2, false);
                    }
                } else {
                    AddPopupListElement("Set Custom CDT", "", TAG_FUNC_EDITCDT, false, 2, false);
                }
            }
        }

        else if (FunctionId == TAG_FUNC_OPT) {
            if (master && AtcMe) {
                addLogLine("TRIGGER - TAG_FUNC_OPT");
                OpenPopupList(Area, "CDM - Options", 1);
                // EOBT OPTIONS
                AddPopupListElement("Ready EOBT", "", TAG_FUNC_READYEOBT, false, 2, false);
                AddPopupListElement("Edit EOBT", "", TAG_FUNC_EDITEOBT, false, 2, false);
                AddPopupListElement("----------------", "", -1, false, 2, false);

                // TOBT OPTIONS
                if ((string)fp.GetGroundState() != "STUP" && (string)fp.GetGroundState() != "ST-UP" &&
                    (string)fp.GetGroundState() != "PUSH" && (string)fp.GetGroundState() != "TAXI" &&
                    (string)fp.GetGroundState() != "DEPA") {
                    AddPopupListElement("Ready TOBT", "", TAG_FUNC_READYTOBT, false, 2, false);
                    AddPopupListElement("Edit TOBT", "", TAG_FUNC_EDITTOBT, false, 2, false);
                    AddPopupListElement("----------------", "", -1, false, 2, false);
                }

                // TSAC OPTIONS
                string tsacvalue = getFlightStripInfo(fp, 1);
                if (tsacvalue.empty()) {
                    AddPopupListElement("Add TSAT to TSAC", "", TAG_FUNC_ADDTSAC, false, 2, false);
                } else {
                    AddPopupListElement("Remove TSAC", "", TAG_FUNC_REMOVETSAC, false, 2, false);
                }
                AddPopupListElement("Edit TSAC", "", TAG_FUNC_EDITTSAC, false, 2, false);
                AddPopupListElement("----------------", "", -1, false, 2, false);

                // ASRT OPTIONS
                string asrtvalue = getFlightStripInfo(fp, 0);
                if (asrtvalue.empty()) {
                    AddPopupListElement("Set RSTUP State", "", TAG_FUNC_READYSTARTUP, false, 2, false);
                } else {
                    AddPopupListElement("Remove RSTUP State", "", TAG_FUNC_READYSTARTUP, false, 2, false);
                }
                AddPopupListElement("----------------", "", -1, false, 2, false);

                // CDT OPTIONS
                bool planeFound = false;
                Plane plane;
                for (size_t i = 0; i < slotList.size(); i++) {
                    if (slotList[i].callsign == fp.GetCallsign()) {
                        planeFound = true;
                        plane = slotList[i];
                    }
                }

                if (planeFound) {
                    if (plane.hasManualCtot) {
                        AddPopupListElement("Edit Custom CDT", "", TAG_FUNC_EDITCDT, false, 2, false);
                    } else {
                        AddPopupListElement("Set Custom CDT", "", TAG_FUNC_EDITCDT, false, 2, false);
                    }
                } else {
                    AddPopupListElement("Set Custom CDT", "", TAG_FUNC_EDITCDT, false, 2, false);
                }

                // CTOT OPTIONS
                if (!plane.hasManualCtot || plane.ctot != "") {
                    AddPopupListElement("Set Manual CTOT", "", TAG_FUNC_EDITMANCTOT, false, 2, false);
                    if (flightHasCtotDisabled(fp.GetCallsign())) {
                        AddPopupListElement("Enable CDM-Network", "", TAG_FUNC_ENABLECTOT, false, 2, false);
                    } else {
                        AddPopupListElement("Disable CDM-Network", "", TAG_FUNC_DISABLECTOT, false, 2, false);
                    }
                } else if (plane.ctot == "" && plane.hasManualCtot) {
                    AddPopupListElement("Remove Manual CTOT", "", TAG_FUNC_REMOVEMANCTOT, false, 2, false);
                    if (flightHasCtotDisabled(fp.GetCallsign())) {
                        AddPopupListElement("Enable CDM-Network", "", TAG_FUNC_ENABLECTOT, false, 2, false);
                    } else {
                        AddPopupListElement("Disable CDM-Network", "", TAG_FUNC_DISABLECTOT, false, 2, false);
                    }
                }
            }
        }

        else if (FunctionId == TAG_FUNC_OPT_TOBT) {
            if (AtcMe) {
                if ((string)fp.GetGroundState() != "STUP" && (string)fp.GetGroundState() != "ST-UP" &&
                    (string)fp.GetGroundState() != "PUSH" && (string)fp.GetGroundState() != "TAXI" &&
                    (string)fp.GetGroundState() != "DEPA") {
                    addLogLine("TRIGGER - TAG_FUNC_OPT_TOBT");
                    OpenPopupList(Area, "TOBT Options", 1);
                    AddPopupListElement("Ready TOBT", "", TAG_FUNC_READYTOBT, false, 2, false);
                    AddPopupListElement("Edit TOBT", "", TAG_FUNC_EDITTOBT, false, 2, false);
                }
            }
        }

        else if (FunctionId == TAG_FUNC_OPT_EOBT) {
            // Can be modified as non-master
            if (AtcMe) {
                addLogLine("TRIGGER - TAG_FUNC_OPT_EOBT");
                OpenPopupList(Area, "EOBT Options", 1);
                AddPopupListElement("Ready EOBT", "", TAG_FUNC_READYEOBT, false, 2, false);
                AddPopupListElement("Edit EOBT", "", TAG_FUNC_EDITEOBT, false, 2, false);
            }
        }

        else if (FunctionId == TAG_FUNC_OPT_ETOBT) {
            addLogLine("TRIGGER - TAG_FUNC_OPT_ETOBT");
            bool isCDMairport = false;
            for (string a : CDMairports) {
                if (fp.GetFlightPlanData().GetOrigin() == a) {
                    isCDMairport = true;
                }
            }
            if (!isCDMairport && AtcMe) {
                addLogLine("TRIGGER - EOBT options");
                OpenPopupList(Area, "E/TOBT Options", 1);
                AddPopupListElement("Ready EOBT", "", TAG_FUNC_READYEOBT, false, 2, false);
                AddPopupListElement("Edit EOBT", "", TAG_FUNC_EDITEOBT, false, 2, false);
            } else if (AtcMe) {
                addLogLine("TRIGGER - TOBT options");
                OpenPopupList(Area, "E/TOBT Options", 1);
                AddPopupListElement("Ready TOBT", "", TAG_FUNC_READYTOBT, false, 2, false);
                AddPopupListElement("Edit TOBT", "", TAG_FUNC_EDITTOBT, false, 2, false);
            }
        }

        else if (FunctionId == TAG_FUNC_OPT_TSAC) {
            if (master && AtcMe) {
                addLogLine("TRIGGER - TAG_FUNC_OPT_TSAC");
                OpenPopupList(Area, "TSAC Options", 1);
                string tsacvalue = getFlightStripInfo(fp, 1);
                AddPopupListElement("Add TSAT to TSAC", "", TAG_FUNC_ADDTSAC, false, 2, false);
                if (!tsacvalue.empty()) {
                    AddPopupListElement("Remove TSAC", "", TAG_FUNC_REMOVETSAC, false, 2, false);
                }
                AddPopupListElement("Edit TSAC", "", TAG_FUNC_EDITTSAC, false, 2, false);
            }
        }

        else if (FunctionId == TAG_FUNC_ADDCTOC) {
            addLogLine("TRIGGER - TAG_FUNC_ADDCTOC");
            bool foundCTOT = false;
            for (size_t a = 0; a < slotList.size(); a++) {
                if (slotList[a].callsign == fp.GetCallsign()) {
                    string getCTOT = slotList[a].ctot;
                    if (getCTOT.empty() && slotList[a].hasManualCtot) {
                        getCTOT = slotList[a].ttot;
                    }
                    if (getCTOT.length() >= 4) {
                        setFlightStripInfo(fp, getCTOT.substr(0, 4), 8);
                        foundCTOT = true;
                    }
                }
            }
            if (!foundCTOT) {
                setFlightStripInfo(fp, "", 8);
            }
        }

        else if (FunctionId == TAG_FUNC_REMOVECTOC) {
            addLogLine("TRIGGER - TAG_FUNC_REMOVECTOC");
            setFlightStripInfo(fp, "", 8);
        }

        else if (FunctionId == TAG_FUNC_EDITCTOC) {
            addLogLine("TRIGGER - TAG_FUNC_EDITCTOC");
            OpenPopupEdit(Area, TAG_FUNC_NEWCTOC, getFlightStripInfo(fp, 8).c_str());
        }

        else if (FunctionId == TAG_FUNC_NEWCTOC) {
            addLogLine("TRIGGER - TAG_FUNC_NEWCTOC");
            string editedCTOC = ItemString;
            if (editedCTOC.length() > 0) {
                bool hasNoNumber = true;
                if (editedCTOC.length() == 4) {
                    for (size_t i = 0; i < editedCTOC.length(); i++) {
                        if (isdigit(editedCTOC[i]) == false) {
                            hasNoNumber = false;
                        }
                    }
                    if (hasNoNumber) {
                        setFlightStripInfo(fp, editedCTOC, 8);
                    }
                }
            }
        }

        else if (FunctionId == TAG_FUNC_OPT_CTOC) {
            if (master && AtcMe) {
                addLogLine("TRIGGER - TAG_FUNC_OPT_CTOC");
                OpenPopupList(Area, "CTOC Options", 1);
                string ctocvalue = getFlightStripInfo(fp, 8);
                AddPopupListElement("Add CTOT to CTOC", "", TAG_FUNC_ADDCTOC, false, 2, false);
                if (!ctocvalue.empty()) {
                    AddPopupListElement("Remove CTOC", "", TAG_FUNC_REMOVECTOC, false, 2, false);
                }
                AddPopupListElement("Edit CTOC", "", TAG_FUNC_EDITCTOC, false, 2, false);
            }
        }

        else if (FunctionId == TAG_FUNC_OPT_EvCTOT) {
            if (master && AtcMe) {
                addLogLine("TRIGGER - TAG_FUNC_OPT_EvCTOT");
                OpenPopupList(Area, "Event SLOT Options", 1);
                AddPopupListElement("Add Event SLOT as TOBT", "", TAG_FUNC_EvCTOTtoTOBT, false, 2, false);
                AddPopupListElement("Add Event SLOT as MAN CTOT", "", TAG_FUNC_EvCTOTtoCTOT, false, 2, false);
            }
        }

        else if (FunctionId == TAG_FUNC_EvCTOTtoTOBT) {
            if ((string)fp.GetGroundState() != "STUP" && (string)fp.GetGroundState() != "ST-UP" &&
                (string)fp.GetGroundState() != "PUSH" && (string)fp.GetGroundState() != "TAXI" &&
                (string)fp.GetGroundState() != "DEPA") {
                if (master && AtcMe) {
                    bool inEvCtotsList = false;
                    string slot = "";
                    for (size_t i = 0; i < evCtots.size(); i++) {
                        if (evCtots[i][0] == fp.GetCallsign() && evCtots[i][1] != "") {
                            inEvCtotsList = true;
                            slot = evCtots[i][1];
                        }
                    }
                    if (inEvCtotsList) {
                        setFlightStripInfo(fp, formatTime(slot), 2);
                    }
                }
            }
        }

        else if (FunctionId == TAG_FUNC_READYTOBT) {
            try {
                if ((string)fp.GetGroundState() != "STUP" && (string)fp.GetGroundState() != "ST-UP" &&
                    (string)fp.GetGroundState() != "PUSH" && (string)fp.GetGroundState() != "TAXI" &&
                    (string)fp.GetGroundState() != "DEPA") {
                    if (master && AtcMe) {
                        addLogLine("TRIGGER - TAG_FUNC_READYTOBT");
                        // SET SU_WAIT WHEN OPTION ENABLED
                        if (option_su_wait) {
                            suWaitList.push_back(fp.GetCallsign());
                        }

                        if (getFlightStripInfo(fp, 2) != formatTime(GetActualTime())) {
                            setFlightStripInfo(fp, formatTime(GetActualTime()), 2);

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

                            string annotAsrt = getFlightStripInfo(fp, 0);
                            if (annotAsrt.empty()) {
                                setFlightStripInfo(fp, (hour + min), 0);
                            }

                            // Update TOBT-setBy
                            string prevSetBy = getFlightStripInfo(fp, 9);
                            if (prevSetBy != "A") {
                                setFlightStripInfo(fp, "A", 9);
                            }

                            bool found = false;
                            for (string callsign : reqTobtList) {
                                if (callsign == fp.GetCallsign()) {
                                    found = true;
                                    break;
                                }
                            }
                            if (!found) reqTobtList.push_back(fp.GetCallsign());

                            // Set TSAC 9999 to later set the correct TSAT when calculation of TSAT completed
                            if (readySetTsac) setFlightStripInfo(fp, "9999", 1);

                            // Set REA Status
                            std::thread t99(&CDM::setCdmSts, this, fp.GetCallsign(), "REA/1");
                            t99.detach();
                        }
                    } else if (AtcMe) {
                        addLogLine("TRIGGER - TAG_FUNC_READYTOBT_SLAVE");
                        if (getFlightStripInfo(fp, 2) != formatTime(GetActualTime())) {
                            setFlightStripInfo(fp, formatTime(GetActualTime()), 2);

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

                            // Set REQ TOBT
                            std::thread t99(&CDM::setCdmSts, this, fp.GetCallsign(), "REQTOBT/" + hour + min + "/ATC");
                            t99.detach();
                        }
                    }
                }
            } catch (const std::exception& ex) {
                addLogLine(string("EXCEPTION in TAG_FUNC_READYTOBT: ") + ex.what());
            } catch (...) {
                addLogLine("UNKNOWN EXCEPTION in TAG_FUNC_READYTOBT");
            }
        }

        else if (FunctionId == TAG_FUNC_EDITCDT) {
            if (master && AtcMe) {
                addLogLine("TRIGGER - TAG_FUNC_EDITCDT");
                bool found = false;
                string ttot;
                for (size_t i = 0; i < slotList.size(); i++) {
                    if (slotList[i].callsign == fp.GetCallsign()) {
                        if (!slotList[i].ttot.empty()) {
                            found = true;
                            ttot = slotList[i].ttot.substr(0, 4);
                        }
                    }
                }
                if (found) {
                    OpenPopupEdit(Area, TAG_FUNC_TRY_TO_SET_CDT, ttot.c_str());
                } else {
                    OpenPopupEdit(Area, TAG_FUNC_TRY_TO_SET_CDT, "");
                }
            }
        }

        else if (FunctionId == TAG_FUNC_TRY_TO_SET_CDT) {
            if (master && AtcMe) {
                addLogLine("TRIGGER - TAG_FUNC_TRY_TO_SET_CDT");
                // only before start-up/push back
                const string groundState = fp.GetGroundState();
                if (groundState != "STUP" && groundState != "ST-UP" && groundState != "PUSH" && groundState != "TAXI" &&
                    groundState != "DEPA") {
                    string editedCDT = ItemString;
                    bool hasNoNumber = true;
                    if (editedCDT.length() == 4) {
                        for (size_t i = 0; i < editedCDT.length(); i++) {
                            if (isdigit(editedCDT[i]) == false) {
                                hasNoNumber = false;
                            }
                        }
                        if (hasNoNumber) {
                            // First, Re-order main list
                            slotList = recalculateSlotList(slotList);

                            string callsign = fp.GetCallsign();
                            string depRwy = fp.GetFlightPlanData().GetDepartureRwy();
                            boost::to_upper(depRwy);
                            if (RadarTargetSelect(callsign.c_str()).IsValid() && depRwy.length() > 0) {
                                double lat = RadarTargetSelect(callsign.c_str()).GetPosition().GetPosition().m_Latitude;
                                double lon =
                                    RadarTargetSelect(callsign.c_str()).GetPosition().GetPosition().m_Longitude;
                                int deIceTime = addDeIceTime(callsign, fp.GetFlightPlanData().GetAircraftWtc());
                                string myTaxiTime = getTaxiTime(lat, lon, fp.GetFlightPlanData().GetOrigin(), depRwy,
                                                                deIceTime, callsign);
                                string calculatedTOBT = calculateLessTime(editedCDT + "00", stod(myTaxiTime));
                                // at the earlierst at present time + EXOT
                                if (stoi(calculatedTOBT) > stoi(GetTimeNow())) {
                                    setFlightStripInfo(fp, calculatedTOBT.substr(0, 4), 2);
                                    for (size_t i = 0; i < slotList.size(); i++) {
                                        if ((string)fp.GetCallsign() == slotList[i].callsign &&
                                            !slotList[i].hasManualCtot) {
                                            slotList[i].ttot = editedCDT + "00";
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        else if (FunctionId == TAG_FUNC_EvCTOTtoCTOT) {
            if ((string)fp.GetGroundState() != "STUP" && (string)fp.GetGroundState() != "ST-UP" &&
                (string)fp.GetGroundState() != "PUSH" && (string)fp.GetGroundState() != "TAXI" &&
                (string)fp.GetGroundState() != "DEPA") {
                if (master && AtcMe) {
                    addLogLine("TRIGGER - TAG_FUNC_EvCTOTtoCTOT");
                    // only before start-up/push back
                    const string groundState = fp.GetGroundState();
                    if (groundState != "STUP" && groundState != "ST-UP" && groundState != "PUSH" &&
                        groundState != "TAXI" && groundState != "DEPA") {
                        bool hasEvCTOT = false;
                        string editedCTOT = "";
                        for (size_t i = 0; i < evCtots.size(); i++) {
                            if (evCtots[i][0] == fp.GetCallsign()) {
                                if (evCtots[i][1] != "") {
                                    hasEvCTOT = true;
                                    editedCTOT = evCtots[i][1];
                                }
                            }
                        }

                        if (hasEvCTOT) {
                            bool hasNoNumber = true;
                            if (editedCTOT.length() == 4) {
                                for (size_t i = 0; i < editedCTOT.length(); i++) {
                                    if (isdigit(editedCTOT[i]) == false) {
                                        hasNoNumber = false;
                                    }
                                }

                                if (hasNoNumber) {
                                    slotList = recalculateSlotList(slotList);
                                    string callsign = fp.GetCallsign();
                                    string depRwy = fp.GetFlightPlanData().GetDepartureRwy();
                                    boost::to_upper(depRwy);
                                    if (RadarTargetSelect(callsign.c_str()).IsValid() && depRwy.length() > 0) {
                                        double lat =
                                            RadarTargetSelect(callsign.c_str()).GetPosition().GetPosition().m_Latitude;
                                        double lon =
                                            RadarTargetSelect(callsign.c_str()).GetPosition().GetPosition().m_Longitude;
                                        int deIceTime = addDeIceTime(callsign, fp.GetFlightPlanData().GetAircraftWtc());
                                        string myTaxiTime = getTaxiTime(lat, lon, fp.GetFlightPlanData().GetOrigin(),
                                                                        depRwy, deIceTime, callsign);
                                        string calculatedTOBT = calculateLessTime(editedCTOT + "00", stod(myTaxiTime));
                                        // at the earlierst at present time + EXOT
                                        if (stoi(calculatedTOBT) > stoi(GetTimeNow())) {
                                            setFlightStripInfo(fp, calculatedTOBT.substr(0, 4), 2);
                                            setFlightStripInfo(fp, "1", 7);
                                            for (size_t i = 0; i < slotList.size(); i++) {
                                                if (slotList[i].callsign == fp.GetCallsign()) {
                                                    slotList[i].hasManualCtot = true;
                                                    addTimeToListForSpecificAirportAndRunway(
                                                        10, calculateTime(GetTimeNow(), 5),
                                                        fp.GetFlightPlanData().GetOrigin(),
                                                        fp.GetFlightPlanData().GetDepartureRwy());
                                                }
                                            }
                                            // Update times to slaves
                                            countTime = std::time(nullptr) - refreshTime;
                                            countTimeNonCdm = std::time(nullptr) - refreshTime;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        else if (FunctionId == TAG_FUNC_DISABLECTOT) {
            if (master && AtcMe) {
                addLogLine("TRIGGER - TAG_FUNC_DISABLECTOT");
                bool found = false;
                for (string callsign : disabledCtots) {
                    if (callsign == fp.GetCallsign()) {
                        found = true;
                    }
                }
                if (!found) {
                    addLogLine("Disabled CTOT for: " + (string)fp.GetCallsign());
                    disabledCtots.push_back(fp.GetCallsign());
                }
            }
        } else if (FunctionId == TAG_FUNC_ENABLECTOT) {
            if (master && AtcMe) {
                addLogLine("TRIGGER - TAG_FUNC_ENABLECTOT");
                bool found = false;
                for (int i = 0; i < disabledCtots.size(); i++) {
                    if (disabledCtots[i] == fp.GetCallsign()) {
                        disabledCtots.erase(disabledCtots.begin() + i);
                    }
                }
            }
        } else if (FunctionId == TAG_FUNC_EDITMANCTOT) {
            if (master && AtcMe) {
                addLogLine("TRIGGER - TAG_FUNC_EDITMANCTOT");
                bool found = false;
                for (size_t i = 0; i < slotList.size(); i++) {
                    if (slotList[i].callsign == fp.GetCallsign()) {
                        if (!slotList[i].ttot.empty()) {
                            found = true;
                        }
                    }
                }
                if (found) {
                    OpenPopupEdit(Area, TAG_FUNC_MODIFYMANCTOT, "");
                } else {
                    OpenPopupEdit(Area, TAG_FUNC_MODIFYMANCTOT, "");
                }
            }
        }

        else if (FunctionId == TAG_FUNC_MODIFYMANCTOT) {
            if (master && AtcMe) {
                addLogLine("TRIGGER - TAG_FUNC_MODIFYMANCTOT");
                // only before start-up/push back
                const string groundState = fp.GetGroundState();
                if (groundState != "STUP" && groundState != "ST-UP" && groundState != "PUSH" && groundState != "TAXI" &&
                    groundState != "DEPA") {
                    string editedCTOT = ItemString;
                    bool hasNoNumber = true;
                    if (editedCTOT.length() == 4) {
                        for (size_t i = 0; i < editedCTOT.length(); i++) {
                            if (isdigit(editedCTOT[i]) == false) {
                                hasNoNumber = false;
                            }
                        }
                        if (hasNoNumber) {
                            // First, Re-order main list
                            slotList = recalculateSlotList(slotList);

                            string callsign = fp.GetCallsign();
                            string depRwy = fp.GetFlightPlanData().GetDepartureRwy();
                            boost::to_upper(depRwy);
                            if (RadarTargetSelect(callsign.c_str()).IsValid() && depRwy.length() > 0) {
                                double lat = RadarTargetSelect(callsign.c_str()).GetPosition().GetPosition().m_Latitude;
                                double lon =
                                    RadarTargetSelect(callsign.c_str()).GetPosition().GetPosition().m_Longitude;
                                int deIceTime = addDeIceTime(callsign, fp.GetFlightPlanData().GetAircraftWtc());
                                string myTaxiTime = getTaxiTime(lat, lon, fp.GetFlightPlanData().GetOrigin(), depRwy,
                                                                deIceTime, callsign);
                                string calculatedTOBT = calculateLessTime(editedCTOT + "00", stod(myTaxiTime));
                                // at the earlierst at present time + EXOT
                                if (stoi(calculatedTOBT) > stoi(GetTimeNow())) {
                                    setFlightStripInfo(fp, calculatedTOBT.substr(0, 4), 2);
                                    setFlightStripInfo(fp, "1", 7);
                                    for (size_t i = 0; i < slotList.size(); i++) {
                                        if ((string)fp.GetCallsign() == slotList[i].callsign &&
                                            !slotList[i].hasManualCtot) {
                                            slotList[i].hasManualCtot = true;
                                            slotList[i].ttot = editedCTOT + "00";
                                            // Delay all aircraft to adjust sequence.
                                            // addTimeToListForSpecificAirportAndRunway(10, calculateTime(GetTimeNow(),
                                            // 5), fp.GetFlightPlanData().GetOrigin(),
                                            // fp.GetFlightPlanData().GetDepartureRwy());
                                        }
                                    }
                                }
                                // Update times to slaves
                                countTime = std::time(nullptr) - refreshTime;
                                countTimeNonCdm = std::time(nullptr) - refreshTime;
                            }
                        }
                    }
                }
            }
        }

        else if (FunctionId == TAG_FUNC_REMOVEMANCTOT) {
            if (master && AtcMe) {
                addLogLine("TRIGGER - TAG_FUNC_REMOVEMANCTOT");
                for (size_t i = 0; i < slotList.size(); i++) {
                    if (slotList[i].callsign == fp.GetCallsign()) {
                        if (slotList[i].hasManualCtot) {
                            slotList[i].hasManualCtot = false;
                            if (serverEnabled) {
                                for (size_t a = 0; a < slotList.size(); a++) {
                                    if (slotList[a].callsign == (string)fp.GetCallsign()) {
                                        slotList[a].showData = false;
                                    }
                                }
                                // Check API
                                std::thread t(&CDM::setOBTApi, this, slotList[i].callsign, slotList[i].tsat, true,
                                              false);
                                t.detach();
                            }

                            if (!isCdmAirport(fp.GetFlightPlanData().GetOrigin())) {
                                slotList[i].ttot = "";
                            }
                        }
                    }
                }
            }
        }

        else if (FunctionId == TAG_FUNC_EDITTOBT) {
            if (AtcMe) {
                if ((string)fp.GetGroundState() != "STUP" && (string)fp.GetGroundState() != "ST-UP" &&
                    (string)fp.GetGroundState() != "PUSH" && (string)fp.GetGroundState() != "TAXI" &&
                    (string)fp.GetGroundState() != "DEPA") {
                    addLogLine("TRIGGER - TAG_FUNC_EDITTOBT");
                    OpenPopupEdit(Area, TAG_FUNC_NEWTOBT, getFlightStripInfo(fp, 2).c_str());
                }
            }
        } else if (FunctionId == TAG_FUNC_NEWTOBT) {
            try {
                if ((string)fp.GetGroundState() != "STUP" && (string)fp.GetGroundState() != "ST-UP" &&
                    (string)fp.GetGroundState() != "PUSH" && (string)fp.GetGroundState() != "TAXI" &&
                    (string)fp.GetGroundState() != "DEPA") {
                    addLogLine("TRIGGER - TAG_FUNC_NEWTOBT");
                    string editedTOBT = ItemString;
                    string setBy = "NONE";
                        bool hasNoNumber = true;
                        if (editedTOBT.length() == 4) {
                            for (size_t i = 0; i < editedTOBT.length(); i++) {
                                if (isdigit(editedTOBT[i]) == false) {
                                    hasNoNumber = false;
                                }
                            }

                            if (hasNoNumber) {
                                if (!master) {
                                    if (editedTOBT.length() == 4) {
                                        // Set REQ TOBT
                                        std::thread t99(&CDM::setCdmSts, this, fp.GetCallsign(),
                                                        "REQTOBT/" + editedTOBT + "/ATC");
                                        t99.detach();
                                    }
                                } else {
                                    int hours = stoi(editedTOBT.substr(0, 2));
                                    int minutes = stoi(editedTOBT.substr(2, 2));
                                    if (hours >= 0 && hours <= 23 && minutes >= 0 && minutes <= 59) {
                                        bool found = false;
                                        for (size_t i = 0; i < slotList.size(); i++) {
                                            if (slotList[i].callsign == fp.GetCallsign()) {
                                                found = true;
                                                setFlightStripInfo(fp, editedTOBT, 2);
                                            }
                                        }
                                        if (!found) {
                                            setFlightStripInfo(fp, editedTOBT, 2);
                                        }
                                        setBy = "ATC";
                                        found = false;
                                        for (string callsign : reqTobtList) {
                                            if (callsign == fp.GetCallsign()) {
                                                found = true;
                                                break;
                                            }
                                        }
                                        if (!found) reqTobtList.push_back(fp.GetCallsign());
                                    }
                                }
                            }
                        } else if (editedTOBT.empty() && master && AtcMe) {
                            setFlightStripInfo(fp, "", 0);
                            setFlightStripInfo(fp, "", 2);
                            for (size_t i = 0; i < slotList.size(); i++) {
                                if ((string)fp.GetCallsign() == slotList[i].callsign) {
                                    slotList.erase(slotList.begin() + i);
                                    // Update times to slaves
                                    setFlightStripInfo(fp, "", 3);
                                    setFlightStripInfo(fp, "", 4);
                                    PushToOtherControllers(fp);
                                }
                            }

                            // if (!realMode) {
                            // Check API
                            setBy = "";
                            for (int z = 0; z < reqTobtList.size(); z++) {
                                reqTobtList.erase(reqTobtList.begin() + z);
                                break;
                            }

                            if (serverEnabled) {
                                // Hide calculation
                                for (size_t a = 0; a < slotList.size(); a++) {
                                    if (slotList[a].callsign == (string)fp.GetCallsign()) {
                                        slotList[a].showData = false;
                                    }
                                }
                                std::thread t(&CDM::setOBTApi, this, (string)fp.GetCallsign(), "", true, false);
                                t.detach();
                            }
                            //}
                        }

                        // Update TOBT-setBy
                        if (setBy != "NONE" && master && AtcMe) {
                            string prevSetBy = getFlightStripInfo(fp, 9);
                            if (setBy.length() > 1) setBy = setBy.substr(0, 1);
                            if (prevSetBy != setBy) {
                                setFlightStripInfo(fp, setBy, 9);
                            }
                        }
                }
            } catch (const std::exception& ex) {
                addLogLine(string("EXCEPTION in TAG_FUNC_NEWTOBT: ") + ex.what());
            } catch (...) {
                addLogLine("UNKNOWN EXCEPTION in TAG_FUNC_NEWTOBT");
            }
        }

    } catch (const std::exception& e) {
        addLogLine("ERROR: Unhandled exception OnFunctionCall: " + (string)e.what());
    } catch (...) {
        addLogLine("ERROR: Unhandled exception OnFunctionCall");
    }
}

void CDM::OnFlightPlanDisconnect(CFlightPlan FlightPlan) {
    if (FlightPlan.IsValid()) {
        string tsat = getFlightStripInfo(FlightPlan, 3);
        if (tsat != "") {
            disconnectionList.push_back(FlightPlan.GetCallsign());
            countTfcDisconnection = 1;
            countTfcDisconnectionTime = std::time(nullptr);
        } else {
            RemoveDataFromTfc(FlightPlan.GetCallsign());
        }
    }
}
