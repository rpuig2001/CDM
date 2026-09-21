// CDM-server "relevant flights" ATFCM list fetch/display + related getters, and the flight-row
// checkbox toggle dialog handler. Moved verbatim out of CDMSingle.cpp.
// See src/core/CDMGlobals.hpp for the shared-state rationale.

#include "CDMSingle.hpp"
#include "src/core/CDMGlobals.hpp"

#include <thread>

void CDM::getCdmServerRelevantFlights() {
    if (serverEnabled && showAtfcmList) {
        addLogLine("Called getCdmServerRelevantFlights...");
        try {
            vector<vector<string>> relevantFlightsTemp;
            string callsign = ControllerMyself().GetCallsign();
            if (callsign.length() > 4) {
                callsign = callsign.substr(0, 2);
            } else {
                // return;
            }

            string url = cdmServerUrl + "/etfms/relevant" /* ? filter = " + callsign */;
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

                relevantFlightsTemp.clear();

                const Json::Value& data = obj;
                for (size_t i = 0; i < data.size(); i++) {
                    if (data[i].isMember("callsign") && data[i].isMember("cid") && data[i].isMember("departure") &&
                        data[i].isMember("arrival") && data[i].isMember("eobt") && data[i].isMember("atfcmStatus") &&
                        data[i].isMember("tobt") && data[i].isMember("taxi") && data[i].isMember("ctot") &&
                        data[i].isMember("aobt") && data[i].isMember("atot") && data[i].isMember("eta") &&
                        data[i].isMember("atfcmData") && data[i]["atfcmData"].isMember("mostPenalisingRegulation") &&
                        data[i].isMember("informed") && data[i].isMember("isCdm")) {
                        auto cleanString = [&](const Json::Value& val) -> std::string {
                            std::string s = fastWriter.write(val);
                            s.erase(std::remove(s.begin(), s.end(), '"'), s.end());
                            s.erase(std::remove(s.begin(), s.end(), '\n'));
                            s.erase(std::remove(s.begin(), s.end(), '\n'));
                            return s;
                        };

                        // Extract all fields
                        std::string callsign = cleanString(data[i]["callsign"]);
                        std::string departure = cleanString(data[i]["departure"]);
                        std::string arrival = cleanString(data[i]["arrival"]);
                        std::string eobt = cleanString(data[i]["eobt"]);
                        std::string tobt = cleanString(data[i]["tobt"]);
                        std::string taxi = cleanString(data[i]["taxi"]);
                        std::string ctot = cleanString(data[i]["ctot"]);
                        std::string aobt = cleanString(data[i]["aobt"]);
                        std::string eta = cleanString(data[i]["eta"]);
                        std::string mostPenalisingRegulation =
                            cleanString(data[i]["atfcmData"]["mostPenalisingRegulation"]);
                        std::string atfcmStatus = cleanString(data[i]["atfcmStatus"]);
                        std::string informed = cleanString(data[i]["informed"]);
                        std::string isCdm = cleanString(data[i]["isCdm"]);
                        const Json::Value& atfcm = data[i]["atfcmData"];
                        std::string isExcluded = cleanString(atfcm["excluded"]);
                        std::string isRea = cleanString(atfcm["isRea"]);
                        std::string isSir = cleanString(atfcm["SIR"]);

                        if (mostPenalisingRegulation.length() <= 2 && ctot.length() > 2) {
                            mostPenalisingRegulation = "N/A";
                        }

                        relevantFlightsTemp.push_back({callsign, departure, arrival, eobt, tobt, taxi, ctot, aobt, eta,
                                                       mostPenalisingRegulation, atfcmStatus, informed, isCdm,
                                                       isExcluded, isRea, isSir});
                    }
                }
            }
            relevantFlights = relevantFlightsTemp;
            addLogLine("COMPLETED - getCdmServerRelevantFlights");
        } catch (const std::exception& e) {
            addLogLine("ERROR: Unhandled exception getCdmServerRelevantFlights: " + (string)e.what());
        } catch (...) {
            addLogLine("ERROR: Unhandled exception getCdmServerRelevantFlights");
        }
    }
}

vector<string> CDM::getCDMAirports() { return CDMairports; }

string CDM::getFilterFlightsText() { return flightsFilterText; }

vector<vector<string>> CDM::returnRelevantFlights() { return relevantFlights; }

vector<string> CDM::getMasterAirports() { return masterAirports; }

vector<vector<string>> CDM::getServerMasterAirports() { return serverMasterAirports; }

void CDM::fetchRelevantFlights() {
    std::thread t78(&CDM::getCdmServerRelevantFlights, this);
    t78.detach();
}

bool CDM::setCdmServerStatusFromDialog(std::vector<std::string> flight, string request) {
    string requestToDo = "";
    if (request == "EXCL") {
        if (flight[13] == "true")
            requestToDo = "EXCLUDED/0";
        else
            requestToDo = "EXCLUDED/1";

        for (int i = 0; i < (int)relevantFlights.size(); i++) {
            if (relevantFlights[i][0] == flight[0])
                if (requestToDo == "EXCLUDED/1")
                    relevantFlights[i][13] = "true";
                else
                    relevantFlights[i][13] = "false";
        }
    }
    if (request == "REA") {
        if (flight[14] == "true")
            requestToDo = "REA/0";
        else
            requestToDo = "REA/1";

        for (int i = 0; i < (int)relevantFlights.size(); i++) {
            if (relevantFlights[i][0] == flight[0])
                if (requestToDo == "REA/1")
                    relevantFlights[i][14] = "true";
                else
                    relevantFlights[i][14] = "false";
        }
    }
    if (request == "SIR") {
        if (flight[15] == "true")
            requestToDo = "SIR/0";
        else
            requestToDo = "SIR/1";

        for (int i = 0; i < (int)relevantFlights.size(); i++) {
            if (relevantFlights[i][0] == flight[0])
                if (requestToDo == "SIR/1")
                    relevantFlights[i][15] = "true";
                else
                    relevantFlights[i][15] = "false";
        }
    }
    if (request == "SWM") {
        if (flight[16] == "true")
            requestToDo = "SWM/0";
        else
            requestToDo = "SWM/1";

        for (int i = 0; i < (int)relevantFlights.size(); i++) {
            if (relevantFlights[i][0] == flight[0])
                if (requestToDo == "SWM/1")
                    relevantFlights[i][16] = "true";
                else
                    relevantFlights[i][16] = "false";
        }
    }
    std::thread t6(&CDM::setCdmSts, this, flight[0], requestToDo);
    t6.detach();
    return true;
}
