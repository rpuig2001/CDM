// De-ice state, disconnect/flight-removal cleanup, CTOT-disable flag, SID-interval loading,
// and CDM-airport check, moved verbatim out of CDMSingle.cpp.
// See src/core/CDMGlobals.hpp for the shared-state rationale.

#include "CDMSingle.hpp"
#include "src/core/CDMGlobals.hpp"

void CDM::setDeice(string remText, CFlightPlan fp, string index) {
    addLogLine("TRIGGER - TAG_FUNC_DEICE_" + remText);

    bool found = false;
    for (size_t i = 0; i < deiceList.size(); i++) {
        if (deiceList[i][0] == fp.GetCallsign()) {
            found = true;
        }
    }

    if (found) {
        // Remove plane from deice list
        for (size_t i = 0; i < deiceList.size(); i++) {
            if (deiceList[i][0] == fp.GetCallsign()) {
                deiceList.erase(deiceList.begin() + i);
            }
        }
    }

    deiceList.push_back({fp.GetCallsign(), remText, index});
    setFlightStripInfo(fp, remText, 5);

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

void CDM::disconnectTfcs() {
    addLogLine("Called disconnectTfcs...");
    try {
        for (string callsign : disconnectionList) {
            RemoveDataFromTfc(callsign);
        }
        disconnectionList.clear();
    } catch (const std::exception& e) {
        addLogLine("ERROR: Unhandled exception disconnectTfcs: " + (string)e.what());
    } catch (...) {
        addLogLine("ERROR: Unhandled exception disconnectTfcs");
    }
}

void CDM::RemoveDataFromTfc(string callsign) {
    addLogLine("Called RemoveDataFromTfc...");
    try {
        // Delete from vector (fix iterator invalidation: only increment if no erase)
        for (size_t i = 0; i < slotList.size(); ) {
            if (callsign == slotList[i].callsign) {
                if (debugMode) {
                    sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 1");
                }
                slotList.erase(slotList.begin() + i);
                // Don't increment i - next element shifts into current position
            } else {
                i++;
            }
        }
        // Delete from reaSent list (fix iterator invalidation: only increment if no erase)
        for (size_t i = 0; i < reaSent.size(); ) {
            if (callsign == reaSent[i]) {
                if (debugMode) {
                    sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 2");
                }
                reaSent.erase(reaSent.begin() + i);
                // Don't increment i - next element shifts into current position
            } else {
                i++;
            }
        }
        // Delete from reaCTOTSent list (fix iterator invalidation: only increment if no erase)
        for (size_t i = 0; i < reaCTOTSent.size(); ) {
            if (callsign == reaCTOTSent[i]) {
                if (debugMode) {
                    sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 3");
                }
                reaCTOTSent.erase(reaCTOTSent.begin() + i);
                // Don't increment i - next element shifts into current position
            } else {
                i++;
            }
        }
        // Remove Plane From airport List (fix iterator invalidation: only increment if no erase)
        for (size_t j = 0; j < planeAiportList.size(); ) {
            if (planeAiportList[j].substr(0, planeAiportList[j].find(",")) == callsign) {
                if (debugMode) {
                    sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 5");
                }
                planeAiportList.erase(planeAiportList.begin() + j);
                // Don't increment j - next element shifts into current position
            } else {
                j++;
            }
        }

        // Remove Plane From finalTimesList (fix iterator invalidation: only increment if no erase)
        for (size_t i = 0; i < finalTimesList.size(); ) {
            if (finalTimesList[i] == callsign) {
                if (debugMode) {
                    sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 6");
                }
                finalTimesList.erase(finalTimesList.begin() + i);
                // Don't increment i - next element shifts into current position
            } else {
                i++;
            }
        }

        // Remove Taxi Times List (fix iterator invalidation: only increment if no erase)
        for (size_t j = 0; j < taxiTimesList.size(); ) {
            if (taxiTimesList[j].substr(0, taxiTimesList[j].find(",")) == callsign) {
                if (debugMode) {
                    sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 7");
                }
                taxiTimesList.erase(taxiTimesList.begin() + j);
                // Don't increment j - next element shifts into current position
            } else {
                j++;
            }
        }

        // Remove ctotCheck
        for (size_t i = 0; i < CTOTcheck.size(); i++) {
            if (CTOTcheck[i] == callsign) {
                if (debugMode) {
                    sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 8");
                }
                CTOTcheck.erase(CTOTcheck.begin() + i);
            }
        }

        // Remove ASAT
        for (size_t x = 0; x < asatList.size(); x++) {
            string actualListCallsign = asatList[x].substr(0, asatList[x].find(","));
            if (actualListCallsign == callsign) {
                if (debugMode) {
                    sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 9");
                }
                asatList.erase(asatList.begin() + x);
            }
        }
        // Remove from OutOfTsat
        for (size_t i = 0; i < OutOfTsat.size(); i++) {
            if (callsign == OutOfTsat[i][0]) {
                if (debugMode) {
                    sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 10");
                }
                OutOfTsat.erase(OutOfTsat.begin() + i);
            }
        }
        // Remove from setOBTlater
        {
            std::lock_guard<std::mutex> lock(later1Mutex);
            for (size_t i = 0; i < setOBTlater.size(); i++) {
                if (callsign == setOBTlater[i].callsign) {
                    if (debugMode) {
                        sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 11");
                    }
                    setOBTlater.erase(setOBTlater.begin() + i);
                }
            }
        }
        // Remove from suWaitList
        for (size_t i = 0; i < suWaitList.size(); i++) {
            if (callsign == suWaitList[i]) {
                if (debugMode) {
                    sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 12");
                }
                suWaitList.erase(suWaitList.begin() + i);
            }
        }
        // Remove from setCdmStslater
        {
            std::lock_guard<std::mutex> lock(later2Mutex);
            for (size_t i = 0; i < setCdmStslater.size(); i++) {
                if (callsign == setCdmStslater[i][0]) {
                    if (debugMode) {
                        sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 13");
                    }
                    setCdmStslater.erase(setCdmStslater.begin() + i);
                }
            }
        }
        // Remove from setCdmDatalater
        {
            std::lock_guard<std::mutex> lock(later4Mutex);
            for (size_t i = 0; i < setCdmDatalater.size(); i++) {
                if (callsign == setCdmDatalater[i].callsign) {
                    if (debugMode) {
                        sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 14");
                    }
                    setCdmDatalater.erase(setCdmDatalater.begin() + i);
                }
            }
        }
        // Remove from disabledCtots
        for (size_t i = 0; i < disabledCtots.size(); i++) {
            if (callsign == disabledCtots[i]) {
                if (debugMode) {
                    sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 15");
                }
                disabledCtots.erase(disabledCtots.begin() + i);
            }
        }
        // Remove from checkCIDLater
        {
            std::lock_guard<std::mutex> lock(later3Mutex);
            for (size_t i = 0; i < checkCIDLater.size(); i++) {
                if (callsign == checkCIDLater[i]) {
                    if (debugMode) {
                        sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 16");
                    }
                    checkCIDLater.erase(checkCIDLater.begin() + i);
                }
            }
        }

        // Remove from deiceList
        for (size_t i = 0; i < deiceList.size(); i++) {
            if (callsign == deiceList[i][0]) {
                if (debugMode) {
                    sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 16");
                }
                deiceList.erase(deiceList.begin() + i);
            }
        }
        // Remove from atotSet
        for (size_t i = 0; i < atotSet.size(); i++) {
            if (callsign == atotSet[i]) {
                if (debugMode) {
                    sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 17");
                }
                atotSet.erase(atotSet.begin() + i);
            }
        }
        // Remove from dataSaved
        for (size_t i = 0; i < dataSaved.size(); i++) {
            if (callsign == dataSaved[i][0]) {
                if (debugMode) {
                    sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 19");
                }
                dataSaved.erase(dataSaved.begin() + i);
            }
        }
        // Remove from obtList
        for (size_t i = 0; i < obtList.size(); i++) {
            if (callsign == obtList[i][0]) {
                if (debugMode) {
                    sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 20");
                }
                obtList.erase(obtList.begin() + i);
            }
        }
        // Remove from reqTobtList
        for (size_t i = 0; i < reqTobtList.size(); i++) {
            if (callsign == reqTobtList[i]) {
                if (debugMode) {
                    sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 21");
                }
                reqTobtList.erase(reqTobtList.begin() + i);
            }
        }
        // Remove from messagesSent
        for (size_t i = 0; i < messagesSent.size(); i++) {
            if (callsign == messagesSent[i]) {
                if (debugMode) {
                    sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 22");
                }
                messagesSent.erase(messagesSent.begin() + i);
            }
        }

        deleteFlightStrips(callsign);
    } catch (const std::exception& e) {
        addLogLine("ERROR: Unhandled exception removeDataFromTfc: " + (string)e.what());
    } catch (...) {
        addLogLine("ERROR: Unhandled exception removeDataFromTfc");
    }
}

bool CDM::flightHasCtotDisabled(string callsign) {
    for (string cs : disabledCtots) {
        if (cs == callsign) {
            return true;
        }
    }
    return false;
}

void CDM::getSidIntervalValuesUrl(string url) {
    addLogLine("Called getSidIntervalValuesUrl...");
    const auto response = restclient_->get(url);
    int responseCode = response.statusCode;

    if (responseCode == 404 || responseCode == 401 || responseCode == 502 || responseCode == -1) {
        // handle error 404
        sendMessage("UNABLE TO LOAD SidInterval URL...");
        addLogLine("UNABLE TO LOAD SidInterval DATA: rc=" + to_string(responseCode) + " result=" + to_string(response.statusCode));
    } else {
        std::istringstream is(response.body);

        // Get data from .txt file
        string lineValue;
        while (getline(is, lineValue)) {
            if (!lineValue.empty()) {
                if (lineValue.substr(0, 1) != "#") {
                    vector<string> tempList = explode(lineValue, ',');
                    if (tempList.size() == 5) {
                        // Old format: airport,rwy,sid1,sid2,value  (rwy2 = any)
                        sidInterval si =
                            sidInterval(tempList[0], tempList[1], tempList[2], "", tempList[3], stod(tempList[4]));
                        sidIntervalList.push_back(si);
                    } else if (tempList.size() == 6) {
                        // New format: airport,rwy,sid1,rwy2,sid2,value
                        sidInterval si =
                            sidInterval(tempList[0], tempList[1], tempList[2], tempList[3], tempList[4], stod(tempList[5]));
                        sidIntervalList.push_back(si);
                    }
                }
            }
        }
    }
    addLogLine("FINISHED getSidIntervalValuesUrl");
}

double CDM::getSidInterval(string mySid, string listSid, string depAirport, string depRwy, string listDepRwy) {
    if (mySid.length() > 3 && listSid.length() > 3) {
        // substr to get only the SID point (strip the runway designator suffix)
        string sid1 = mySid.substr(0, mySid.length() - 2);
        string sid2 = listSid.substr(0, listSid.length() - 2);
        for (sidInterval si : sidIntervalList) {
            if (si.airport == depAirport && si.rwy == depRwy) {
                bool sidMatch = (si.sid1 == sid1 && si.sid2 == sid2) || (si.sid2 == sid1 && si.sid1 == sid2);
                // rwy2 empty means the rule applies regardless of the other plane's runway
                bool rwy2Match = si.rwy2.empty() || si.rwy2 == listDepRwy;
                if (sidMatch && rwy2Match) {
                    return si.value;
                }
            }
        }
    }
    return -1;
}

bool CDM::isCdmAirport(string airport) {
    bool cdmAirport = false;
    for (string a : CDMairports) {
        if (airport == a) {
            return true;
        }
    }
    return false;
}
