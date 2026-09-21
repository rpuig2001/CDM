// Master-airport set/remove operations (server + local fallback) and Event-CTOT/CID linking,
// moved verbatim out of CDMSingle.cpp. See src/core/CDMGlobals.hpp for the shared-state rationale.

#include "CDMSingle.hpp"
#include "src/core/CDMGlobals.hpp"

#include <thread>

bool CDM::setMasterAirport(string airport, string position) {
    addLogLine("Called setMasterAirport...");
    if (serverEnabled) {
        try {
            addLogLine("Call - Set Master airport " + airport + "(" + position + ")");
            string url = cdmServerUrl + "/airport/setMaster?airport=" + airport + "&position=" + position;
            std::unordered_map<std::string, std::string> headers = {{"x-api-key", apikey}};
            const auto response = restclient_->post(url, /*body=*/"", headers);
            int responseCode = response.statusCode;

            if (responseCode == 404 || responseCode == 401 || responseCode == 502 || responseCode == -1) {
                addLogLine("UNABLE TO CONNECT CDM-API... Master not set.");
            } else {
                std::istringstream is(response.body);
                string lineValue;
                while (getline(is, lineValue)) {
                    if (lineValue == "true") {
                        for (int attempt = 0; attempt < 10; ++attempt) {  // Retry up to 3 times
                            try {
                                masterAirports.push_back(airport);
                                sendMessage("Successfully set master airport " + airport);
                                addLogLine("Successfully set master airport " + airport);
                                lastAddedIcao = "";
                                // Update server master list
                                std::thread t99(&CDM::getCdmServerMasterAirports, this);
                                t99.detach();
                                return true;
                            } catch (const std::system_error& e) {
                                addLogLine("ERROR: Unhandled exception setMasterAirport: " + (string)e.what());
                            }
                        }
                    }
                    sendMessage("Unable to set master airport " + airport);
                    addLogLine("Unable to set master airport " + airport);
                }
            }
        } catch (const std::exception& e) {
            addLogLine("ERROR: Unhandled exception setMasterAirport: " + (string)e.what());
            lastAddedIcao = "";
            return false;
        } catch (...) {
            addLogLine("ERROR: Unhandled exception setMasterAirport");
            lastAddedIcao = "";
            return false;
        }
    } else {
        masterAirports.push_back(airport);
        sendMessage("Successfully set master airport (Locally only) " + airport);
        addLogLine("Successfully set master airport (Locally only) " + airport);
    }

    lastAddedIcao = "";
    return false;
}

bool CDM::removeMasterAirport(string airport, string position) {
    addLogLine("Call - Remove Master airport " + airport + "(" + position + ")");
    if (serverEnabled) {
        string url = cdmServerUrl + "/airport/removeMaster?airport=" + airport + "&position=" + position;
        std::unordered_map<std::string, std::string> headers = {{"x-api-key", apikey}};
        const auto response = restclient_->post(url, /*body=*/"", headers);
        int responseCode = response.statusCode;

        if (responseCode == 404 || responseCode == 401 || responseCode == 502 || responseCode == -1) {
            addLogLine("UNABLE TO CONNECT CDM-API...");
            for (int a = 0; a < masterAirports.size(); a++) {
                if (masterAirports[a] == airport) {
                    masterAirports.erase(masterAirports.begin() + a);
                    addLogLine("Successfully removed master airport (Locally only) " + airport);
                    sendMessage("Successfully removed master airport (Locally only) " + airport);
                    return true;
                }
            }
        } else {
            std::istringstream is(response.body);
            // Get data from .txt file
            string lineValue;
            while (getline(is, lineValue)) {
                if (lineValue == "true") {
                    addLogLine("Successfully removed master airport " + airport);
                    sendMessage("Successfully removed master airport " + airport);
                } else {
                    addLogLine("Successfully removed master airport (Locally only) " + airport);
                    sendMessage("Successfully removed master airport (Locally only) " + airport);
                }
            }
            for (int a = 0; a < masterAirports.size(); a++) {
                if (masterAirports[a] == airport) {
                    masterAirports.erase(masterAirports.begin() + a);
                    // Update server master list
                    std::thread t99(&CDM::getCdmServerMasterAirports, this);
                    t99.detach();
                    return true;
                }
            }
        }
    } else {
        for (int a = 0; a < masterAirports.size(); a++) {
            if (masterAirports[a] == airport) {
                masterAirports.erase(masterAirports.begin() + a);
                addLogLine("Successfully removed master airport (Locally only) " + airport);
                sendMessage("Successfully removed master airport (Locally only) " + airport);
                return true;
            }
        }
    }
    return false;
}

bool CDM::removeAllMasterAirports(string position) {
    addLogLine("Called removeAllMasterAirports...");
    if (serverEnabled) {
        try {
            addLogLine("Call - Remove all masters for " + position);
            string url = cdmServerUrl + "/airport/removeAllMasterByPosition?position=" + position;
            std::unordered_map<std::string, std::string> headers = {{"x-api-key", apikey}};
            const auto response = restclient_->post(url, /*body=*/"", headers);
            int responseCode = response.statusCode;

            if (responseCode == 404 || responseCode == 401 || responseCode == 502 || responseCode == -1) {
                addLogLine("UNABLE TO CONNECT CDM-API...");
            } else {
                std::istringstream is(response.body);
                string lineValue;
                while (getline(is, lineValue)) {
                    if (lineValue == "true") {
                        addLogLine("Successfully removed all master airports for " + position);
                        sendMessage("Successfully removed all master airports for " + position);
                        masterAirports.clear();
                        // Update server master list
                        std::thread t99(&CDM::getCdmServerMasterAirports, this);
                        t99.detach();
                        return true;
                    }
                }
            }
            return false;
        } catch (const std::exception& e) {
            addLogLine("ERROR: Unhandled exception removeAllMasterAirports: " + (string)e.what());
            return false;
        } catch (...) {
            addLogLine("ERROR: Unhandled exception removeAllMasterAirports");
            return false;
        }
    } else {
        addLogLine("Successfully removed all master airports for " + position);
        sendMessage("Successfully removed all master airports for " + position);
        masterAirports.clear();
        return true;
    }
}

void CDM::removeAllMasterAirportsByAirport(string airport) {
    if (serverEnabled) {
        addLogLine("Called removeAllMasterAirportsByAirport...");
        try {
            addLogLine("Call - Remove all masters from " + airport);
            string url = cdmServerUrl + "/airport/removeAllMasterByAirport?airport=" + airport;
            std::unordered_map<std::string, std::string> headers = {{"x-api-key", apikey}};
            const auto response = restclient_->post(url, /*body=*/"", headers);
            int responseCode = response.statusCode;

            if (responseCode == 404 || responseCode == 401 || responseCode == 502 || responseCode == -1) {
                addLogLine("UNABLE TO CONNECT CDM-API...");
            } else {
                addLogLine("Removed masters for airport " + airport);
                // Update server master list
                std::thread t99(&CDM::getCdmServerMasterAirports, this);
                t99.detach();
            }
        } catch (const std::exception& e) {
            addLogLine("ERROR: Unhandled exception removeAllMasterAirportsByAirport: " + (string)e.what());
        } catch (...) {
            addLogLine("ERROR: Unhandled exception removeAllMasterAirportsByAirport");
        }
    }
}

bool CDM::setEvCtot(string callsign) {
    if (serverEnabled) {
        addLogLine("Called setEvCtot...");
        try {
            addLogLine("Call - Set Event CTOT for " + callsign);
            string url = cdmServerUrl + "/ifps/cidCheck?callsign=" + callsign;
            std::unordered_map<std::string, std::string> headers = {{"x-api-key", apikey}};
            const auto response = restclient_->get(url, headers);
            int responseCode = response.statusCode;

            if (responseCode == 404 || responseCode == 401 || responseCode == 502 || responseCode == -1) {
                std::lock_guard<std::mutex> lock(later3Mutex);
                checkCIDLater.push_back(callsign);
                addLogLine("UNABLE TO CONNECT CDM-API...");
            } else {
                std::istringstream is(response.body);
                // Get data from .txt file
                string cid = "";
                while (getline(is, cid)) {
                    if (cid.length() > 4) {
                        for (int i = 0; i < slotFile.size(); i++) {
                            if (slotFile[i].size() > 1) {
                                bool match = false;
                                if (slotFile[i][1] == "" && slotFile[i][2] == "" && slotFile[i][3] == "") {
                                    // Case where we only have CID
                                    if (slotFile[i][0] == cid) match = true;
                                } else if (slotFile[i][2] == "" && slotFile[i][3] == "") {
                                    // Case where we only have CID and callsign
                                    if (slotFile[i][0] == cid && slotFile[i][1] == callsign) match = true;
                                } else {
                                    // Case where we have CID, callsign, departure and destination
                                    string departure =
                                        FlightPlanSelect(callsign.c_str()).GetFlightPlanData().GetOrigin();
                                    string destination =
                                        FlightPlanSelect(callsign.c_str()).GetFlightPlanData().GetDestination();
                                    if (slotFile[i][0] == cid && slotFile[i][1] == callsign &&
                                        slotFile[i][2] == departure && slotFile[i][3] == destination) {
                                        match = true;
                                    } else if (slotFile[i][0] == cid && slotFile[i][2] == departure &&
                                               slotFile[i][3] == destination) {
                                        // Fallback using cid, departure and destination only
                                        match = true;
                                    }
                                }
                                if (match) {
                                    addLogLine(callsign + " linked with EvCTOT " + slotFile[i][4]);
                                    sendMessage(callsign + " linked with EvCTOT " + slotFile[i][4]);
                                    for (int a = 0; a < evCtots.size(); a++) {
                                        if (evCtots[a].size() > 0) {
                                            if (evCtots[a][0] == callsign) {
                                                evCtots[a] = {callsign, slotFile[i][4]};
                                                if (autoSetTobtFromEvSlot)
                                                    setFlightStripInfo(FlightPlanSelect(callsign.c_str()),
                                                                       formatTime(slotFile[i][4]), 2);
                                                return true;
                                            };
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                if (cid == "") {
                    std::lock_guard<std::mutex> lock(later3Mutex);
                    checkCIDLater.push_back(callsign);
                }
            }
            return false;
        } catch (const std::exception& e) {
            std::lock_guard<std::mutex> lock(later3Mutex);
            checkCIDLater.push_back(callsign);
            addLogLine("ERROR: Unhandled exception setEvCtot: " + (string)e.what());
            return false;
        } catch (...) {
            std::lock_guard<std::mutex> lock(later3Mutex);
            checkCIDLater.push_back(callsign);
            addLogLine("ERROR: Unhandled exception setEvCtot");
            return false;
        }
    }
    return false;
}
