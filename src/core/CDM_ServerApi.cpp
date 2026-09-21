// CDM-server REST API integration: fetching restrictions/CTOTs, queued retry senders,
// OBT/CDM-status updates, network rates & TOBT sync, master-airport data recovery.
// Moved verbatim out of CDMSingle.cpp. See src/core/CDMGlobals.hpp for the shared-state rationale.

#include "CDMSingle.hpp"
#include "src/core/CDMGlobals.hpp"

#include <thread>

void CDM::getCdmServerRestricted(vector<Plane> slotListTemp) {
    if (serverEnabled) {
        addLogLine("Called getCdmServerRestricted...");
        try {
            vector<ServerRestricted> serverRestrictedPlanesTemp;
            vector<Plane> initialslotListTemp = slotListTemp;
            addLogLine("Call - Fetching CTOTs");
            // sendMessage("Fetching CTOTs...");
            string url = cdmServerUrl + "/etfms/restricted";
            if (customRestrictedUrl != "") url = customRestrictedUrl;
            std::unordered_map<std::string, std::string> headers = {{"x-api-key", apikey}};
            const auto response = restclient_->get(url, headers);
            int responseCode = response.statusCode;

            if (responseCode == 404 || responseCode == 401 || responseCode == 502 || responseCode == -1) {
                // handle error 404
                addLogLine("UNABLE TO LOAD CDM-API URL...");
            } else {
                Json::Reader reader;
                Json::Value obj;
                Json::FastWriter fastWriter;
                reader.parse(response.body, obj);

                // Reset all CTOTs
                for (size_t i = 0; i < slotListTemp.size(); i++) {
                    if (slotListTemp[i].ctot != "") {
                        slotListTemp[i].hasManualCtot = false;
                    }
                    slotListTemp[i].ctot = "";
                    slotListTemp[i].flowReason = "";
                }

                serverRestrictedPlanesTemp.clear();

                const Json::Value& restricted = obj;
                for (size_t i = 0; i < restricted.size(); i++) {
                    if (restricted[i].isMember("callsign") && restricted[i].isMember("ctot") &&
                        restricted[i].isMember("mostPenalisingRegulation")) {
                        // Get callsign
                        string callsign = fastWriter.write(restricted[i]["callsign"]);
                        callsign.erase(std::remove(callsign.begin(), callsign.end(), '"'));
                        callsign.erase(std::remove(callsign.begin(), callsign.end(), '\n'));
                        callsign.erase(std::remove(callsign.begin(), callsign.end(), '\n'));

                        // Get CTOT
                        string ctot = fastWriter.write(restricted[i]["ctot"]);
                        ctot.erase(std::remove(ctot.begin(), ctot.end(), '"'));
                        ctot.erase(std::remove(ctot.begin(), ctot.end(), '\n'));
                        ctot.erase(std::remove(ctot.begin(), ctot.end(), '\n'));

                        // Get reason
                        string reason = fastWriter.write(restricted[i]["mostPenalisingRegulation"]);
                        reason.erase(std::remove(reason.begin(), reason.end(), '"'));
                        reason.erase(std::remove(reason.begin(), reason.end(), '\n'));
                        reason.erase(std::remove(reason.begin(), reason.end(), '\n'));

                        serverRestrictedPlanesTemp.push_back({callsign, ctot, reason});

                        if (ctot.size() == 4) {
                            for (size_t z = 0; z < slotListTemp.size(); z++) {
                                if (slotListTemp[z].callsign == callsign && !flightHasCtotDisabled(callsign)) {
                                    slotListTemp[z] = {callsign,
                                                       slotListTemp[z].eobt,
                                                       slotListTemp[z].tsat,
                                                       slotListTemp[z].ttot,
                                                       ctot,
                                                       reason,
                                                       true,
                                                       true,
                                                       true};
                                }
                            }
                        }
                    }
                }
                serverRestrictedPlanes = serverRestrictedPlanesTemp;
                sendWaitingTOBT();
                sendWaitingCdmSts();
                sendCheckCIDLater();
                sendWaitingCdmData();
                std::vector<Plane> toAdd;
                for (Plane p : slotListTemp) {
                    for (int d = 0; d < initialslotListTemp.size(); d++) {
                        if (p.callsign == initialslotListTemp[d].callsign) {
                            if (initialslotListTemp[d].ctot != p.ctot ||
                                initialslotListTemp[d].flowReason != p.flowReason ||
                                initialslotListTemp[d].hasManualCtot != p.hasManualCtot) {
                                {
                                    toAdd.push_back(p);
                                }
                            }
                        }
                    }
                }
                if (!toAdd.empty()) {
                    std::lock_guard<std::mutex> lock(apiQueueResponseMutex);
                    apiQueueResponse.insert(apiQueueResponse.end(), toAdd.begin(), toAdd.end());
                }
            }
            addLogLine("COMPLETED - Fetching CTOTs");
        } catch (const std::exception& e) {
            addLogLine("ERROR: Unhandled exception Fetching CTOTs: " + (string)e.what());
        } catch (...) {
            addLogLine("ERROR: Unhandled exception Fetching CTOTs");
        }
    }
}

void CDM::sendWaitingTOBT() {
    try {
        vector<Plane> setOBTlaterTemp;
        {
            addLogLine("Call sendWaitingTOBT - " + to_string(setOBTlater.size()));
            addLogLine("Called sendWaitingTOBT...");
            std::lock_guard<std::mutex> lock(later1Mutex);
            setOBTlaterTemp = setOBTlater;
            setOBTlater.clear();
        }

        vector<Plane> alreadyProcessed;
        bool found = false;

        for (int i = 0; i < setOBTlaterTemp.size(); i++) {
            found = false;
            for (Plane p : alreadyProcessed) {
                if (p.callsign == setOBTlaterTemp[i].callsign) {
                    found = true;
                }
            }

            if (!found) {
                alreadyProcessed.push_back(setOBTlaterTemp[i]);
                addLogLine("sendWaitingTOBT - " + setOBTlaterTemp[i].callsign);
                if (serverEnabled) {
                    setOBTApi(setOBTlaterTemp[i].callsign, setOBTlaterTemp[i].tsat,
                              setOBTlaterTemp[i].showData,    /*Used as manualTrigger*/
                              setOBTlaterTemp[i].isCdmAirport /*Used as useEobt*/
                    );
                }
            }
        }
    } catch (const std::exception& e) {
        addLogLine("ERROR: Unhandled exception sendWaitingTOBT: " + (string)e.what());
    } catch (...) {
        addLogLine("Error occurred parsing data from the cdm-api (sendWaitingTOBT)");
    }
}

void CDM::sendWaitingCdmSts() {
    vector<vector<string>> callsignsToProcess;
    {
        addLogLine("Call sendWaitingCdmSts - " + to_string(setCdmStslater.size()));
        std::lock_guard<std::mutex> lock(later2Mutex);
        callsignsToProcess = setCdmStslater;
        setCdmStslater.clear();
    }

    for (int i = 0; i < callsignsToProcess.size(); i++) {
        addLogLine("sendWaitingCdmSts - " + callsignsToProcess[i][0]);
        setCdmSts(callsignsToProcess[i][0], callsignsToProcess[i][1]);
    }
}

void CDM::sendWaitingCdmData() {
    vector<Plane> callsignsToProcess;
    {
        addLogLine("Call sendWaitingCdmData - " + to_string(setCdmDatalater.size()));
        std::lock_guard<std::mutex> lock(later4Mutex);
        callsignsToProcess = setCdmDatalater;
        setCdmDatalater.clear();
    }

    for (const Plane& plane : callsignsToProcess) {
        addLogLine("sendWaitingCdmData - " + plane.callsign);
        updateCdmDataApi(plane);
    }
}

void CDM::sendCheckCIDLater() {
    vector<string> callsignsToProcess;
    {
        addLogLine("Call sendCheckCIDLater - " + to_string(checkCIDLater.size()));
        std::lock_guard<std::mutex> lock(later3Mutex);
        callsignsToProcess = checkCIDLater;
        checkCIDLater.clear();
    }

    for (const string& callsign : callsignsToProcess) {
        addLogLine("sendCheckCIDLater - " + callsign);
        setEvCtot(callsign);
    }
}

void CDM::updateCdmDataApi(Plane p) {
    if (serverEnabled) {
        addLogLine("Called updateCdmDataApi...");
        try {
            string str;
            CFlightPlan fp = FlightPlanSelect(p.callsign.c_str());
            if (p.hasManualCtot && p.ctot != "" && p.ttot.length() >= 4) {
                str = "callsign=" + p.callsign + "&tobt=" + p.eobt + "&tsat=" + p.tsat + "&ttot=" + p.ttot +
                      "&ctot=" + p.ctot.substr(0, 4) + "&reason=" + p.flowReason;
                if (fp.IsValid()) {
                    str += "&asrt=" + getFlightStripInfo(fp, 0) +
                           "&depInfo=" + fp.GetFlightPlanData().GetDepartureRwy() + "/" +
                           fp.GetFlightPlanData().GetSidName();
                }
            } else if (p.hasManualCtot && p.ttot.length() >= 4) {
                str = "callsign=" + p.callsign + "&tobt=" + p.eobt + "&tsat=" + p.tsat + "&ttot=" + p.ttot +
                      "&ctot=" + p.ttot.substr(0, 4) + "&reason=MANUAL";
                if (fp.IsValid()) {
                    str += "&asrt=" + getFlightStripInfo(fp, 0) +
                           "&depInfo=" + fp.GetFlightPlanData().GetDepartureRwy() + "/" +
                           fp.GetFlightPlanData().GetSidName();
                }
            } else {
                str = "callsign=" + p.callsign + "&tobt=" + p.eobt + "&tsat=" + p.tsat + "&ttot=" + p.ttot +
                      "&ctot=&reason=";
                if (fp.IsValid()) {
                    str += "&asrt=" + getFlightStripInfo(fp, 0) +
                           "&depInfo=" + fp.GetFlightPlanData().GetDepartureRwy() + "/" +
                           fp.GetFlightPlanData().GetSidName();
                }
            }
            string url = cdmServerUrl + "/ifps/setCdmData?" + str;
            std::unordered_map<std::string, std::string> headers = {{"x-api-key", apikey}};
            const auto response = restclient_->post(url, /*body=*/"", headers);
            int responseCode = response.statusCode;

            if (responseCode == 404 || responseCode == 401 || responseCode == 502 || responseCode == -1) {
                std::lock_guard<std::mutex> lock(later4Mutex);
                setCdmDatalater.push_back(p);
                addLogLine("UNABLE TO CONNECT CDM-API...");
            } else {
                std::istringstream is(response.body);
                // Get data from .txt file
                string lineValue;
                while (getline(is, lineValue)) {
                    if (lineValue != "true") {
                        addLogLine("setCdmData RESPONSE: " + lineValue);
                        std::lock_guard<std::mutex> lock(later4Mutex);
                        setCdmDatalater.push_back(p);
                    }
                }
            }
            addLogLine("COMPLETED - updateCdmDataApi");
        } catch (const std::exception& e) {
            std::lock_guard<std::mutex> lock(later4Mutex);
            setCdmDatalater.push_back(p);
            addLogLine("ERROR: Unhandled exception updateCdmDataApi: " + (string)e.what());
        } catch (...) {
            std::lock_guard<std::mutex> lock(later4Mutex);
            setCdmDatalater.push_back(p);
            addLogLine("ERROR: Unhandled exception updateCdmDataApi");
        }
    }
}

void CDM::setOBTApi(string callsign, string obt, bool triggeredByUser, bool useEobt) {
    addLogLine("Called setOBTApi...");
    try {
        vector<Plane> slotListTemp;  // Local copy of the slotList
        {
            slotListTemp = slotList;  // Copy the slotList
        }

        addLogLine("Call - Set OBT (" + obt + ") for " + callsign + " with triggeredByUser=" +
                   (triggeredByUser ? "true" : "false") + " and useEobt=" + (useEobt ? "true" : "false"));
        bool createRequest = false;

        if (obt != "") {
            for (Plane p : slotListTemp) {
                if (p.callsign == callsign) {
                    // Only create request if TOBT is manually triggered (or initially triggered or when no ctot), to
                    // avoid update set TSAT when syncing from CTOT
                    if ((p.ctot != "" && triggeredByUser) || p.ctot == "") {
                        createRequest = true;
                        break;
                    } else {
                        createRequest = false;
                        break;
                    }
                }
            }
        } else {
            createRequest = true;
        }

        if (isFligthSusp(callsign)) createRequest = false;

        if (createRequest) {
            obt = (obt.length() >= 4) ? obt.substr(0, 4) : "";
            string taxiTime = getTaxiTime(callsign);

            addLogLine("Requesting OBT (" + obt + ") for " + callsign);
            string url = cdmServerUrl + "/ifps/dpi?callsign=" + callsign + "&value=OBT/" + obt + "/" + taxiTime;
            if (useEobt) url = cdmServerUrl + "/ifps/dpi?callsign=" + callsign + "&value=EOBT/" + obt;

            std::unordered_map<std::string, std::string> headers = {{"x-api-key", apikey}};
            const auto response = restclient_->post(url, /*body=*/"", headers);
            int responseCode = response.statusCode;

            if (responseCode == 404 || responseCode == 401 || responseCode == 502 || responseCode == -1) {
                Plane plane(callsign, "", obt, "", "", "", false, triggeredByUser, useEobt);
                {
                    std::lock_guard<std::mutex> lock(later1Mutex);
                    setOBTlater.push_back(plane);  // Safely modify setOBTlater
                }
                addLogLine("UNABLE TO CONNECT CDM-API...");
            } else {
                Json::Reader reader;
                Json::Value obj;
                Json::FastWriter fastWriter;
                reader.parse(response.body, obj);
                if (obj.isMember("callsign") && obj.isMember("ctot") && obj.isMember("atfcmData") &&
                    obj["atfcmData"].isMember("mostPenalisingRegulation")) {
                    string apiCallsign = fastWriter.write(obj["callsign"]);
                    apiCallsign.erase(remove(apiCallsign.begin(), apiCallsign.end(), '"'), apiCallsign.end());
                    apiCallsign.erase(remove(apiCallsign.begin(), apiCallsign.end(), '\n'), apiCallsign.end());

                    string ctot = fastWriter.write(obj["ctot"]);
                    ctot.erase(remove(ctot.begin(), ctot.end(), '"'), ctot.end());
                    ctot.erase(remove(ctot.begin(), ctot.end(), '\n'), ctot.end());

                    string reason = fastWriter.write(obj["atfcmData"]["mostPenalisingRegulation"]);
                    reason.erase(remove(reason.begin(), reason.end(), '"'), reason.end());
                    reason.erase(remove(reason.begin(), reason.end(), '\n'), reason.end());

                    for (size_t i = 0; i < slotListTemp.size(); i++) {
                        if (slotListTemp[i].callsign == apiCallsign) {
                            addLogLine(apiCallsign + " returned with CTOT: [" + ctot + "] and reason: [" + reason +
                                       "]");
                            if (!ctot.empty() && !flightHasCtotDisabled(apiCallsign)) {
                                // Update with thread-safe access
                                {
                                    slotListTemp[i] = {apiCallsign,
                                                       slotListTemp[i].eobt,
                                                       slotListTemp[i].tsat,
                                                       slotListTemp[i].ttot,
                                                       ctot,
                                                       reason,
                                                       true,
                                                       true,
                                                       true};
                                }
                            } else {
                                if (slotListTemp[i].ctot != "") {
                                    // Reset CTOT
                                    {
                                        slotListTemp[i].ctot = "";
                                        slotListTemp[i].flowReason = "";
                                        slotListTemp[i].hasManualCtot = false;
                                        slotListTemp[i].showData = true;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    Plane plane(callsign, "", obt, "", "", "", false, triggeredByUser, useEobt);
                    {
                        std::lock_guard<std::mutex> lock(later1Mutex);
                        setOBTlater.push_back(plane);
                    }
                }
            }
        }

        // Add to queue
        std::vector<Plane> toAdd;
        for (Plane p : slotListTemp) {
            for (int d = 0; d < slotList.size(); d++) {
                if (p.callsign == slotList[d].callsign) {
                    p.showData = true;
                    toAdd.push_back(p);
                }
            }
        }

        if (!toAdd.empty()) {
            std::lock_guard<std::mutex> lock(apiQueueResponseMutex);
            apiQueueResponse.insert(apiQueueResponse.end(), toAdd.begin(), toAdd.end());
        }

        addLogLine("COMPLETED - setOBTApi for " + callsign);
    } catch (const std::exception& e) {
        addLogLine("ERROR: Unhandled exception setOBTApi: " + (string)e.what());
        {
            for (size_t a = 0; a < slotList.size(); a++) {
                if (slotList[a].callsign == callsign) {
                    slotList[a].showData = true;
                }
            }
        }
    } catch (...) {
        addLogLine("ERROR: Unhandled exception setOBTApi");
        {
            for (size_t a = 0; a < slotList.size(); a++) {
                if (slotList[a].callsign == callsign) {
                    slotList[a].showData = true;
                }
            }
        }
    }
}

string CDM::getTaxiTime(string callsign) {
    addLogLine("Call - getTaxiTime");
    string taxiTime = "15";
    for (size_t j = 0; j < taxiTimesList.size(); j++) {
        if (taxiTimesList[j].substr(0, taxiTimesList[j].find(",")) == callsign) {
            if (taxiTimesList[j].substr(taxiTimesList[j].length() - 2, 1) == ",") {
                taxiTime = taxiTimesList[j].substr(taxiTimesList[j].length() - 1, 1);
            } else {
                taxiTime = taxiTimesList[j].substr(taxiTimesList[j].length() - 2, 2);
            }
        }
    }
    addLogLine("getTaxiTime: " + taxiTime);
    return taxiTime;
}

void CDM::setCdmSts(string callsign, string cdmSts) {
    if (serverEnabled) {
        addLogLine("Called setCdmSts...");
        try {
            addLogLine("Call - Set DPI:" + cdmSts + " - for " + callsign);

            string url = cdmServerUrl + "/ifps/dpi?callsign=" + callsign + "&value=" + cdmSts;
            std::unordered_map<std::string, std::string> headers = {{"x-api-key", apikey}};
            const auto response = restclient_->post(url, /*body=*/"", headers);
            int responseCode = response.statusCode;

            if (responseCode == 404 || responseCode == 401 || responseCode == 502 || responseCode == -1) {
                std::lock_guard<std::mutex> lock(later2Mutex);
                setCdmStslater.push_back({callsign, cdmSts});
                addLogLine("UNABLE TO CONNECT CDM-API...");
            } else {
                std::istringstream is(response.body);
                string lineValue;
                while (getline(is, lineValue)) {
                    if (lineValue != "true") {
                        std::lock_guard<std::mutex> lock(later2Mutex);
                        addLogLine("setCdmSts: true not received. Retrying later... Received: " + lineValue);
                        setCdmStslater.push_back({callsign, cdmSts});
                    }
                }
            }
            std::thread t59(&CDM::getCdmServerStatus, this);
            t59.detach();

            addLogLine("COMPLETED setCdmSts...");
        } catch (const std::exception& e) {
            std::lock_guard<std::mutex> lock(later2Mutex);
            setCdmStslater.push_back({callsign, cdmSts});
            addLogLine("ERROR: Unhandled exception setCdmSts: " + (string)e.what());
        } catch (...) {
            std::lock_guard<std::mutex> lock(later2Mutex);
            setCdmStslater.push_back({callsign, cdmSts});
            addLogLine("ERROR: Unhandled exception setCdmSts");
        }
    }
}

bool CDM::isFligthSusp(string callsign) {
    addLogLine("Call - isFligthSusp");
    bool outOfTsat = false;
    for (size_t i = 0; i < OutOfTsat.size(); i++) {
        if (callsign == OutOfTsat[i][0]) {
            outOfTsat = true;
        }
    }

    if (outOfTsat) {
        addLogLine("Flight is SUSP");
        return true;
    }
    addLogLine("Flight not SUSP: ");
    return false;
}

void CDM::getCdmServerStatus() {
    if (serverEnabled) {
        addLogLine("Called getCdmServerStatus...");
        try {
            vector<vector<string>> networkStatusTemp;

            string url = cdmServerUrl + "/ifps/allStatus";
            std::unordered_map<std::string, std::string> headers = {{"x-api-key", apikey}};
            const auto response = restclient_->get(url, headers);
            int responseCode = response.statusCode;

            if (responseCode == 404 || responseCode == 401 || responseCode == 502 || responseCode == -1) {
                addLogLine("UNABLE TO LOAD CDM-API URL...");
            } else {
                Json::Reader reader;
                Json::Value obj;
                Json::FastWriter fastWriter;
                reader.parse(response.body, obj);

                networkStatusTemp.clear();

                const Json::Value& data = obj;
                for (size_t i = 0; i < data.size(); i++) {
                    if (data[i].isMember("callsign") && data[i].isMember("cdmSts")) {
                        // Get callsign
                        string callsign = fastWriter.write(data[i]["callsign"]);
                        callsign.erase(std::remove(callsign.begin(), callsign.end(), '"'));
                        callsign.erase(std::remove(callsign.begin(), callsign.end(), '\n'));
                        callsign.erase(std::remove(callsign.begin(), callsign.end(), '\n'));

                        // Get CTOT
                        string cdmSts = fastWriter.write(data[i]["cdmSts"]);
                        cdmSts.erase(std::remove(cdmSts.begin(), cdmSts.end(), '"'));
                        cdmSts.erase(std::remove(cdmSts.begin(), cdmSts.end(), '\n'));
                        cdmSts.erase(std::remove(cdmSts.begin(), cdmSts.end(), '\n'));

                        networkStatusTemp.push_back({callsign, cdmSts});
                    }
                }
                {
                    std::lock_guard<std::mutex> lock(networkStatusMutex);
                    networkStatus = std::move(networkStatusTemp);
                }
            }
            addLogLine("COMPLETED - getCdmServerStatus");
        } catch (const std::exception& e) {
            addLogLine("ERROR: Unhandled exception getCdmServerStatus: " + (string)e.what());
        } catch (...) {
            addLogLine("ERROR: Unhandled exception getCdmServerStatus");
        }
    }
}

void CDM::getCdmServerOnTime() {
    if (serverEnabled) {
        addLogLine("Called getCdmServerOnTime...");
        try {
            vector<vector<string>> onTimeStatusTemp;

            string url = cdmServerUrl + "/ifps/allOnTime";
            std::unordered_map<std::string, std::string> headers = {{"x-api-key", apikey}};
            const auto response = restclient_->get(url, headers);
            int responseCode = response.statusCode;

            if (responseCode == 404 || responseCode == 401 || responseCode == 502 || responseCode == -1) {
                addLogLine("UNABLE TO LOAD CDM-API URL...");
            } else {
                Json::Reader reader;
                Json::Value obj;
                Json::FastWriter fastWriter;
                reader.parse(response.body, obj);

                onTimeStatusTemp.clear();

                const Json::Value& data = obj;
                for (size_t i = 0; i < data.size(); i++) {
                    if (data[i].isMember("callsign") && data[i].isMember("onTime")) {
                        // Get callsign
                        string callsign = fastWriter.write(data[i]["callsign"]);
                        callsign.erase(std::remove(callsign.begin(), callsign.end(), '"'));
                        callsign.erase(std::remove(callsign.begin(), callsign.end(), '\n'));
                        callsign.erase(std::remove(callsign.begin(), callsign.end(), '\n'));

                        // Get CTOT
                        string onTime = fastWriter.write(data[i]["onTime"]);
                        onTime.erase(std::remove(onTime.begin(), onTime.end(), '"'));
                        onTime.erase(std::remove(onTime.begin(), onTime.end(), '\n'));
                        onTime.erase(std::remove(onTime.begin(), onTime.end(), '\n'));

                        onTimeStatusTemp.push_back({callsign, onTime});
                    }
                }
            }
            onTimeStatus = onTimeStatusTemp;
            addLogLine("COMPLETED - getCdmServerStatus");
        } catch (const std::exception& e) {
            addLogLine("ERROR: Unhandled exception getCdmServerStatus: " + (string)e.what());
        } catch (...) {
            addLogLine("ERROR: Unhandled exception getCdmServerStatus");
        }
    }
}

void CDM::getCdmServerMasterAirports() {
    if (serverEnabled) {
        addLogLine("Called getCdmServerMasterAirports...");
        try {
            vector<vector<string>> serverMasterAirportsTemp;

            string url = cdmServerUrl + "/airport";
            std::unordered_map<std::string, std::string> headers = {{"x-api-key", apikey}};
            const auto response = restclient_->get(url, headers);
            int responseCode = response.statusCode;

            if (responseCode == 404 || responseCode == 401 || responseCode == 502 || responseCode == -1) {
                addLogLine("UNABLE TO LOAD CDM-API URL...");
            } else {
                Json::Reader reader;
                Json::Value obj;
                Json::FastWriter fastWriter;
                reader.parse(response.body, obj);

                serverMasterAirportsTemp.clear();

                const Json::Value& data = obj;
                for (size_t i = 0; i < data.size(); i++) {
                    if (data[i].isMember("icao") && data[i].isMember("position")) {
                        // Get callsign
                        string icao = fastWriter.write(data[i]["icao"]);
                        icao.erase(std::remove(icao.begin(), icao.end(), '"'));
                        icao.erase(std::remove(icao.begin(), icao.end(), '\n'));
                        icao.erase(std::remove(icao.begin(), icao.end(), '\n'));

                        // Get CTOT
                        string position = fastWriter.write(data[i]["position"]);
                        position.erase(std::remove(position.begin(), position.end(), '"'));
                        position.erase(std::remove(position.begin(), position.end(), '\n'));
                        position.erase(std::remove(position.begin(), position.end(), '\n'));
                        
                        serverMasterAirportsTemp.push_back({icao, position});
                    }
                }
            }
            serverMasterAirports = serverMasterAirportsTemp;
            addLogLine("COMPLETED - getCdmServerMasterAirports");
        } catch (const std::exception& e) {
            addLogLine("ERROR: Unhandled exception getCdmServerMasterAirports: " + (string)e.what());
        } catch (...) {
            addLogLine("ERROR: Unhandled exception getCdmServerMasterAirports");
        }
    }
}

void CDM::getNetworkRates() {
    if (serverEnabled) {
        addLogLine("Called getNetworkRates...");
        try {
            vector<Rate> tempRate = initialRate;

            string url = cdmServerUrl + "/etfms/restrictions?type=DEP";
            std::unordered_map<std::string, std::string> headers = {{"x-api-key", apikey}};
            const auto response = restclient_->get(url, headers);
            int responseCode = response.statusCode;

            if (responseCode == 404 || responseCode == 401 || responseCode == 502 || responseCode == -1) {
                addLogLine("UNABLE TO LOAD CDM-API URL...");
            } else {
                Json::Reader reader;
                Json::Value obj;
                Json::FastWriter fastWriter;
                reader.parse(response.body, obj);

                const Json::Value& data = obj;
                for (size_t i = 0; i < data.size(); i++) {
                    if (data[i].isMember("type") && data[i].isMember("airspace") && data[i].isMember("capacity") &&
                        data[i].isMember("runway")) {
                        // Get airspace name
                        string airspace = fastWriter.write(data[i]["airspace"]);
                        airspace.erase(std::remove(airspace.begin(), airspace.end(), '"'));
                        airspace.erase(std::remove(airspace.begin(), airspace.end(), '\n'));
                        airspace.erase(std::remove(airspace.begin(), airspace.end(), '\n'));
                        bool aptFound = false;
                        for (string apt : masterAirports) {
                            if (apt == airspace) {
                                aptFound = true;
                            }
                        }
                        if (aptFound) {
                            // Get callsign
                            string type = fastWriter.write(data[i]["type"]);
                            type.erase(std::remove(type.begin(), type.end(), '"'));
                            type.erase(std::remove(type.begin(), type.end(), '\n'));
                            type.erase(std::remove(type.begin(), type.end(), '\n'));

                            if (type == "DEP") {
                                // Get capacity
                                string capacity = fastWriter.write(data[i]["capacity"]);
                                capacity.erase(std::remove(capacity.begin(), capacity.end(), '"'));
                                capacity.erase(std::remove(capacity.begin(), capacity.end(), '\n'));
                                capacity.erase(std::remove(capacity.begin(), capacity.end(), '\n'));

                                // Get runway
                                string runway = fastWriter.write(data[i]["runway"]);
                                runway.erase(std::remove(runway.begin(), runway.end(), '"'));
                                runway.erase(std::remove(runway.begin(), runway.end(), '\n'));
                                runway.erase(std::remove(runway.begin(), runway.end(), '\n'));

                                for (int i = 0; i < tempRate.size(); i++) {
                                    if (tempRate[i].airport == airspace) {
                                        for (int s = 0; s < tempRate[i].depRwyYes.size(); s++) {
                                            if (tempRate[i].depRwyYes[s] == runway || tempRate[i].depRwyYes[s] == "*") {
                                                if (tempRate[i].rates.size() > 1) {
                                                    tempRate[i].rates[s] = capacity;
                                                } else if (tempRate[i].rates.size() == 1) {
                                                    tempRate[i].rates[0] = capacity;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            rate = tempRate;
            addLogLine("COMPLETED - getNetworkRates");
        } catch (const std::exception& e) {
            addLogLine("ERROR: Unhandled exception getNetworkRates: " + (string)e.what());
        } catch (...) {
            addLogLine("ERROR: Unhandled exception getNetworkRates");
        }
    }
}

vector<vector<string>> CDM::getDepAirportPlanes(string airport) {
    vector<vector<string>> planes;
    if (serverEnabled) {
        addLogLine("Called getDepAirportPlanes...");
        try {
            vector<Rate> tempRate = initialRate;

            string url = cdmServerUrl + "/ifps/depAirport?airport=" + airport;
            std::unordered_map<std::string, std::string> headers = {{"x-api-key", apikey}};
            const auto response = restclient_->get(url, headers);
            int responseCode = response.statusCode;

            if (responseCode == 404 || responseCode == 401 || responseCode == 502 || responseCode == -1) {
                addLogLine("UNABLE TO LOAD CDM-API URL...");
            } else {
                Json::Reader reader;
                Json::Value obj;
                Json::FastWriter fastWriter;
                reader.parse(response.body, obj);

                const Json::Value& data = obj;
                for (size_t i = 0; i < data.size(); i++) {
                    if (data[i].isMember("cdmData") && data[i]["cdmData"].isMember("reqTobt") &&
                        data[i]["cdmData"].isMember("reqTobtType") && data[i]["cdmData"].isMember("reqAsrt") &&
                        data[i].isMember("callsign") && data[i].isMember("atot")) {
                        string callsign = fastWriter.write(data[i]["callsign"]);
                        callsign.erase(std::remove(callsign.begin(), callsign.end(), '"'));
                        callsign.erase(std::remove(callsign.begin(), callsign.end(), '\n'));
                        callsign.erase(std::remove(callsign.begin(), callsign.end(), '\n'));

                        string tobt = fastWriter.write(data[i]["cdmData"]["reqTobt"]);
                        tobt.erase(std::remove(tobt.begin(), tobt.end(), '"'));
                        tobt.erase(std::remove(tobt.begin(), tobt.end(), '\n'));
                        tobt.erase(std::remove(tobt.begin(), tobt.end(), '\n'));

                        string type = fastWriter.write(data[i]["cdmData"]["reqTobtType"]);
                        type.erase(std::remove(type.begin(), type.end(), '"'));
                        type.erase(std::remove(type.begin(), type.end(), '\n'));
                        type.erase(std::remove(type.begin(), type.end(), '\n'));

                        string asrt = fastWriter.write(data[i]["cdmData"]["reqAsrt"]);
                        asrt.erase(std::remove(asrt.begin(), asrt.end(), '"'));
                        asrt.erase(std::remove(asrt.begin(), asrt.end(), '\n'));
                        asrt.erase(std::remove(asrt.begin(), asrt.end(), '\n'));

                        string atot = fastWriter.write(data[i]["atot"]);
                        atot.erase(std::remove(atot.begin(), atot.end(), '"'));
                        atot.erase(std::remove(atot.begin(), atot.end(), '\n'));
                        atot.erase(std::remove(atot.begin(), atot.end(), '\n'));

                        if (atot == "") {
                            planes.push_back({callsign, tobt, type, asrt});
                        }

                        // Sync informed flag into messagesSent
                        if (data[i].isMember("informed")) {
                            string informed = fastWriter.write(data[i]["informed"]);
                            informed.erase(std::remove(informed.begin(), informed.end(), '"'));
                            informed.erase(std::remove(informed.begin(), informed.end(), '\n'));
                            informed.erase(std::remove(informed.begin(), informed.end(), '\n'));
                            if (informed == "true" || informed == "1") {
                                bool alreadySent = false;
                                for (const string& s : messagesSent) {
                                    if (s == callsign) {
                                        alreadySent = true;
                                        break;
                                    }
                                }
                                if (!alreadySent) messagesSent.push_back(callsign);
                            }
                        }
                    }
                }
            }
        } catch (const std::exception& e) {
            addLogLine("ERROR: Unhandled exception getDepAirportPlanes: " + (string)e.what());
        } catch (...) {
            addLogLine("ERROR: Unhandled exception getDepAirportPlanes");
        }
    }
    return planes;
}

void CDM::getIffOffBlockTimes() {
    vector<vector<string>> myObts;
    if (serverEnabled) {
        vector<Plane> mySlotList = slotList;
        vector<string> airports;
        for (int i = 0; i < mySlotList.size(); i++) {
            CFlightPlan fp1 = FlightPlanSelect(mySlotList[i].callsign.c_str());
            if (fp1.IsValid()) {
                string depAirport = fp1.GetFlightPlanData().GetOrigin();
                if (find(CDMairports.begin(), CDMairports.end(), depAirport) == CDMairports.end())
                    airports.push_back(depAirport);
            }
        }
        sort(airports.begin(), airports.end());
        airports.erase(unique(airports.begin(), airports.end()), airports.end());

        addLogLine("Called getIffOffBlockTimes...");
        try {
            for (string apt : airports) {
                string url = cdmServerUrl + "/ifps/depAirport?airport=" + apt;
                std::unordered_map<std::string, std::string> headers = {{"x-api-key", apikey}};
                const auto response = restclient_->get(url, headers);
                int responseCode = response.statusCode;

                if (responseCode == 404 || responseCode == 401 || responseCode == 502 || responseCode == -1) {
                    addLogLine("UNABLE TO LOAD CDM-API URL...");
                } else {
                    Json::Reader reader;
                    Json::Value obj;
                    Json::FastWriter fastWriter;
                    reader.parse(response.body, obj);

                    const Json::Value& data = obj;
                    for (size_t i = 0; i < data.size(); i++) {
                        if (data[i].isMember("obt") && data[i].isMember("callsign") && data[i].isMember("atot")) {
                            string callsign = fastWriter.write(data[i]["callsign"]);
                            callsign.erase(std::remove(callsign.begin(), callsign.end(), '"'));
                            callsign.erase(std::remove(callsign.begin(), callsign.end(), '\n'));
                            callsign.erase(std::remove(callsign.begin(), callsign.end(), '\n'));

                            string obt = fastWriter.write(data[i]["obt"]);
                            obt.erase(std::remove(obt.begin(), obt.end(), '"'));
                            obt.erase(std::remove(obt.begin(), obt.end(), '\n'));
                            obt.erase(std::remove(obt.begin(), obt.end(), '\n'));

                            string atot = fastWriter.write(data[i]["atot"]);
                            atot.erase(std::remove(atot.begin(), atot.end(), '"'));
                            atot.erase(std::remove(atot.begin(), atot.end(), '\n'));
                            atot.erase(std::remove(atot.begin(), atot.end(), '\n'));

                            if (atot == "") {
                                myObts.push_back({callsign, obt});
                            }
                        }
                    }
                }
            }
        } catch (const std::exception& e) {
            addLogLine("ERROR: Unhandled exception getIffOffBlockTimes: " + (string)e.what());
        } catch (...) {
            addLogLine("ERROR: Unhandled exception getIffOffBlockTimes");
        }
    }
    obtList = myObts;
}

void CDM::getNetworkTobt() {
    if (serverEnabled && pilotTobt) {
        addLogLine("Called getNetworkTobt...");
        vector<vector<string>> planes;
        for (string airport : masterAirports) {
            vector<vector<string>> newplanes = getDepAirportPlanes(airport);
            planes.insert(planes.end(), newplanes.begin(), newplanes.end());
        }

        vector<Plane> mySlotList = slotList;
        std::vector<vector<string>> toAdd;

        for (vector<string> plane : planes) {
            bool updated = false;
            if (plane.size() == 4) {
                if (plane[3] != "" && plane[0] != "") {
                    CFlightPlan fp1 = FlightPlanSelect(plane[0].c_str());
                    if (fp1.IsValid()) {
                        // Update ASRT
                        string prevAsrt = getFlightStripInfo(fp1, 0);
                        if ((string)fp1.GetGroundState() != "STUP" && (string)fp1.GetGroundState() != "ST-UP" &&
                            (string)fp1.GetGroundState() != "PUSH" && (string)fp1.GetGroundState() != "TAXI" &&
                            (string)fp1.GetGroundState() != "DEPA") {
                            addLogLine("Updating ASRT for: " + plane[0] + " Old: " + prevAsrt + " New: " + plane[3] +
                                       "00");
                            setFlightStripInfo(fp1, plane[3], 0);
                            setCdmSts(plane[0], "REQASRT/NULL");
                        }
                    }
                }
                if (plane[1] != "" && plane[0] != "") {
                    bool found = false;
                    for (int i = 0; i < mySlotList.size(); i++) {
                        if (plane[0] == mySlotList[i].callsign) {
                            if (!mySlotList[i].showData) {
                                found = true;
                            }
                            // Check if not manual CTOT assigned
                            else if ((!mySlotList[i].hasManualCtot && mySlotList[i].ctot == "") ||
                                     (mySlotList[i].hasManualCtot && mySlotList[i].ctot != "")) {
                                found = true;
                                CFlightPlan fp = FlightPlanSelect(mySlotList[i].callsign.c_str());
                                if (!fp.IsValid()) {
                                    continue;
                                }

                                // Update TOBT
                                string annotAsrt = getFlightStripInfo(fp, 0);
                                if (((tobtReqAfterAsrtDisabledOption && annotAsrt.empty()) || !tobtReqAfterAsrtDisabledOption) &&
                                (string)fp.GetGroundState() != "STUP" &&
                                (string)fp.GetGroundState() != "ST-UP" && 
                                (string)fp.GetGroundState() != "PUSH" &&
                                (string)fp.GetGroundState() != "TAXI" && 
                                (string)fp.GetGroundState() != "DEPA") 
                                {
                                    addLogLine("Updating TOBT for: " + mySlotList[i].callsign +
                                               " Old: " + mySlotList[i].eobt + " New: " + plane[1] + "00");
                                    /*int posPlane = getPlanePosition(mySlotList[i].callsign);
                                    if (posPlane != -1) {
                                            slotList.erase(slotList.begin() + posPlane);
                                    }*/
                                    setFlightStripInfo(fp, plane[1], 2);
                                    bool found = false;
                                    for (string callsign : reqTobtList) {
                                        if (callsign == mySlotList[i].callsign) {
                                            found = true;
                                            break;
                                        }
                                    }
                                    if (!found) reqTobtList.push_back(mySlotList[i].callsign);
                                    setCdmSts(plane[0], "REQTOBT/NULL/NULL");
                                    // Trigger TOBT update to update TAXI TIME
                                    // setOBTApi(plane[0], plane[1], true, false);
                                    updated = true;
                                }
                            }
                        }
                    } 
                    if (!found) {
                        if (plane[1] != "") {
                            addLogLine("Updating TOBT for: " + plane[0] + " Old: outdated New: " + plane[1] + "00");
                            CFlightPlan fp = FlightPlanSelect(plane[0].c_str());
                            if (!fp.IsValid()) {
                                continue;
                            }
                            setFlightStripInfo(fp, plane[1], 2);
                            bool found = false;
                            for (string callsign : reqTobtList) {
                                if (callsign == plane[0]) {
                                    found = true;
                                    break;
                                }
                            }
                            if (!found) reqTobtList.push_back(plane[0]);
                            setCdmSts(plane[0], "REQTOBT/NULL/NULL");
                            // Trigger TOBT update to update TAXI TIME
                            //setOBTApi(plane[0], plane[1], true, false);
                            updated = true;
                        }
                    }

                    if (plane[0] != "") {
                        toAdd.push_back({plane[0], plane[2]});
                    }
                }
            }
        }

        if (!toAdd.empty()) {
            std::lock_guard<std::mutex> lock(reqTobtTypesQueueMutex);
            reqTobtTypesQueue.insert(reqTobtTypesQueue.end(), toAdd.begin(), toAdd.end());
        }
        addLogLine("COMPLETED - getNetworkTobt");
        // Update times to slaves
        countTime = std::time(nullptr) - refreshTime;
    }
}

vector<vector<string>> CDM::getAirportPlanesCdmDataSection(string airport) {
    vector<vector<string>> planes;
    if (serverEnabled) {
        addLogLine("Called getAirportPlanesCdmDataSection...");
        try {
            vector<Rate> tempRate = initialRate;

            string url = cdmServerUrl + "/ifps/depAirport?airport=" + airport;
            std::unordered_map<std::string, std::string> headers = {{"x-api-key", apikey}};
            const auto response = restclient_->get(url, headers);
            int responseCode = response.statusCode;

            if (responseCode == 404 || responseCode == 401 || responseCode == 502 || responseCode == -1) {
                addLogLine("UNABLE TO LOAD CDM-API URL...");
            } else {
                Json::Reader reader;
                Json::Value obj;
                Json::FastWriter fastWriter;
                reader.parse(response.body, obj);

                const Json::Value& data = obj;
                for (size_t i = 0; i < data.size(); i++) {
                    if (data[i].isMember("cdmData") && data[i]["cdmData"].isMember("tobt") &&
                        data[i]["cdmData"].isMember("tsat") && data[i]["cdmData"].isMember("ttot")) {
                        string callsign = fastWriter.write(data[i]["callsign"]);
                        callsign.erase(std::remove(callsign.begin(), callsign.end(), '"'));
                        callsign.erase(std::remove(callsign.begin(), callsign.end(), '\n'));
                        callsign.erase(std::remove(callsign.begin(), callsign.end(), '\n'));

                        string tobt = fastWriter.write(data[i]["cdmData"]["tobt"]);
                        tobt.erase(std::remove(tobt.begin(), tobt.end(), '"'));
                        tobt.erase(std::remove(tobt.begin(), tobt.end(), '\n'));
                        tobt.erase(std::remove(tobt.begin(), tobt.end(), '\n'));

                        string tsat = fastWriter.write(data[i]["cdmData"]["tsat"]);
                        tsat.erase(std::remove(tsat.begin(), tsat.end(), '"'));
                        tsat.erase(std::remove(tsat.begin(), tsat.end(), '\n'));
                        tsat.erase(std::remove(tsat.begin(), tsat.end(), '\n'));

                        string ttot = fastWriter.write(data[i]["cdmData"]["ttot"]);
                        ttot.erase(std::remove(ttot.begin(), ttot.end(), '"'));
                        ttot.erase(std::remove(ttot.begin(), ttot.end(), '\n'));
                        ttot.erase(std::remove(ttot.begin(), ttot.end(), '\n'));

                        if (ttot != "" && tobt != "" && tsat != "") {
                            planes.push_back({callsign, tobt, tsat, ttot});
                        }
                    }
                }
            }
        } catch (const std::exception& e) {
            addLogLine("ERROR: Unhandled exception getDepAirportPlanes: " + (string)e.what());
        } catch (...) {
            addLogLine("ERROR: Unhandled exception getDepAirportPlanes");
        }
    }
    return planes;
}

void CDM::copyServerSavedData(string airport) {
    if (serverEnabled) {
        addLogLine("Called copyServerSavedData...");
        for (string apt : masterAirports) {
            if (apt == airport) {
                sendMessage("Airport: " + airport + ". is already MASTER. Sync not possible.");
                return;
            }
        }

        /* [callisgn, tobt, tsat, ttot] */
        vector<vector<string>> newplanes = getAirportPlanesCdmDataSection(airport);

        vector<Plane> mySlotList = slotList;
        std::vector<vector<string>> toAdd;

        for (vector<string> newplane : newplanes) {
            bool updated = false;
            CFlightPlan fp = FlightPlanSelect(newplane[0].c_str());
            if (!fp.IsValid()) {
                continue;
            }
            for (Plane plane : slotList) {
                if (plane.callsign == newplane[0]) {
                    updated = true;
                    plane.eobt = newplane[1];
                    plane.tsat = newplane[2];
                    plane.ttot = newplane[3];
                    setFlightStripInfo(fp, formatTime(plane.eobt), 2);
                    setFlightStripInfo(fp, plane.tsat, 3);
                    setFlightStripInfo(fp, plane.ttot, 4);
                }
            }
            if (!updated) {
                Plane plane = Plane(newplane[0], newplane[1], newplane[2], newplane[3], "", "", false, false, true);
                setFlightStripInfo(fp, formatTime(plane.eobt), 2);
                setFlightStripInfo(fp, plane.tsat, 3);
                setFlightStripInfo(fp, plane.ttot, 4);
            }
        }
        addLogLine("COMPLETED - copyServerSavedData");
        // Update times to slaves
        countTime = std::time(nullptr) - refreshTime;
    }
}

bool CDM::addMasterAirport(string icao) {
    try {
        if (icao.length() == 4) {
            string ATC_Position = ControllerMyself().GetCallsign();
            bool found = false;
            for (string apt : masterAirports) {
                if (apt == icao) {
                    found = true;
                }
            }
            if (!found) {
                std::thread t(&CDM::setMasterAirport, this, icao, ATC_Position);
                t.detach();
            }
        } else {
            sendMessage("NO AIRPORT SET");
        }
        return true;
    } catch (const std::exception& e) {
        addLogLine("ERROR: Unhandled exception .cdm master: " + (string)e.what());
        return true;
    } catch (...) {
        addLogLine("ERROR: Unhandled exception .cdm master");
        return true;
    }
}

bool CDM::clearMasterAirport(string icao) {
    try {
        if (icao.length() == 4) {
            string ATC_Position = ControllerMyself().GetCallsign();
            bool found = false;
            int a = 0;
            for (string apt : masterAirports) {
                if (apt == icao) {
                    std::thread t(&CDM::removeMasterAirport, this, icao, ATC_Position);
                    t.detach();
                    found = true;
                }
                a++;
            }
            if (!found) {
                sendMessage("AIRPORT " + icao + " NOT FOUND");
            }
        } else {
            sendMessage("NO AIRPORT SET");
        }
        return true;
    } catch (const std::exception& e) {
        addLogLine("ERROR: Unhandled exception .cdm slave: " + (string)e.what());
        return true;
    } catch (...) {
        addLogLine("ERROR: Unhandled exception .cdm slave");
        return true;
    }
}
