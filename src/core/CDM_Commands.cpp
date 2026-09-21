// User-typed ".cdm ..." dot-command handler, moved verbatim out of CDMSingle.cpp.
// See src/core/CDMGlobals.hpp for the shared-state rationale.

#include "CDMSingle.hpp"
#include "src/core/CDMGlobals.hpp"

#include <thread>

bool CDM::OnCompileCommand(const char* sCommandLine) {
    if (startsWith(".cdm ecfmp", sCommandLine)) {
        addLogLine(sCommandLine);
        return true;
    }

    if (startsWith(".cdm refresh", sCommandLine)) {
        addLogLine(sCommandLine);
        sendMessage("Refreshing Now...");
        countTime = std::time(nullptr) - refreshTime;
        countFetchServerTime = std::time(nullptr) - 60;
        return true;
    }

    if (startsWith(".cdm refreshtime", sCommandLine)) {
        try {
            addLogLine(sCommandLine);
            string line = sCommandLine;
            if (line.substr(line.length() - 3, 1) == " ") {
                refreshTime = stoi(line.substr(line.length() - 2)) * 50;
                sendMessage("Refresh Time set to: " + line.substr(line.length() - 2));
            } else if (line.substr(line.length() - 2, 1) == " ") {
                refreshTime = stoi(line.substr(line.length() - 1));
                sendMessage("Refresh Time set to: " + line.substr(line.length() - 1));
            } else {
                sendMessage("INCORRECT REFRESH TIME VALUE...");
            }
            return true;
        } catch (const std::exception& e) {
            addLogLine("ERROR: Unhandled exception .cdm refreshtime: " + (string)e.what());
            return true;
        } catch (...) {
            addLogLine("ERROR: Unhandled exception .cdm refreshtime");
            return true;
        }
    }

    if (startsWith(".cdm help", sCommandLine)) {
        addLogLine(sCommandLine);
        sendMessage(
            "CDM Commands: .cdm ctot - .cdm panel - .cdm master {airport} - .cdm slave {airport} - .cdm refreshtime "
            "{seconds} - .cdm startupdelay {icao}/{rwy} {start_time} - .cdm departuredelay {icao}/{rwy} {start_time} - "
            ".cdm lvo - .cdm realmode - .cdm server - .cdm remarks - .cdm rate - .cdm help");
        return true;
    }

    if (startsWith(".cdm realmode", sCommandLine)) {
        addLogLine(sCommandLine);
        if (realMode) {
            realMode = false;
            sendMessage("Real Mode set to OFF");
        } else {
            realMode = true;
            sendMessage("Real Mode set to ON");
        }
        return true;
    }

    if (startsWith(".cdm event", sCommandLine)) {
        addLogLine(sCommandLine);
        if (eventMode) {
            eventMode = false;
            sendMessage("Event Mode set to OFF");
        } else {
            eventMode = true;
            sendMessage("Event Mode set to ON");
        }
        return true;
    }

    if (startsWith(".cdm panel", sCommandLine)) {
        addLogLine(sCommandLine);
        if (showPanel) {
            showPanel = false;
        } else {
            showPanel = true;
        }
        return true;
    }

    if (startsWith(".cdm atfcm", sCommandLine)) {
        addLogLine(sCommandLine);
        if (showAtfcmList) {
            showAtfcmList = false;
        } else {
            showAtfcmList = true;
            std::thread t73(&CDM::getCdmServerRelevantFlights, this);
            t73.detach();
        }
        return true;
    }

    if (startsWith(".cdm server", sCommandLine)) {
        addLogLine(sCommandLine);
        if (serverEnabled) {
            serverEnabled = false;
            sendMessage("Server Disabled");
        } else {
            serverEnabled = true;
            sendMessage("Server Enabled");
        }
        return true;
    }

    if (startsWith(".cdm debug", sCommandLine)) {
        addLogLine(sCommandLine);
        if (debugMode) {
            debugMode = false;
            sendMessage("Debug Mode set to OFF");
        } else {
            debugMode = true;
            sendMessage("Debug Mode set to ON");
        }
        return true;
    }

    if (startsWith(".cdm rate", sCommandLine)) {
        addLogLine(sCommandLine);
        sendMessage("Reloading rates....");
        if (rateUrl.length() <= 1) {
            if (debugMode) {
                sendMessage("[DEBUG MESSAGE] - USING RATE FROM LOCAL TXT FILE");
            }
            getRate();
        } else {
            if (debugMode) {
                sendMessage("[DEBUG MESSAGE] - USING TAXIZONES FROM URL");
            }
            getRateFromUrl(rateUrl);
        }
        return true;
    }

    if (startsWith(".cdm remarks", sCommandLine)) {
        addLogLine(sCommandLine);
        if (remarksOption) {
            remarksOption = false;
            sendMessage("Set TSAT to Scratchpad to OFF");
        } else {
            remarksOption = true;
            sendMessage("Set TSAT to Scratchpad to ON");
        }
        return true;
    }

    if (startsWith(".cdm rmkctot", sCommandLine)) {
        addLogLine(sCommandLine);
        if (remarksOptionCtot) {
            remarksOptionCtot = false;
            sendMessage("Set CTOT to Scratchpad to OFF");
        } else {
            remarksOptionCtot = true;
            sendMessage("Set CTOT to Scratchpad to ON");
        }
        return true;
    }

    if (startsWith(".cdm startupdelay", sCommandLine)) {
        try {
            addLogLine(sCommandLine);
            string line = sCommandLine;
            string apt = line.substr(line.find("/") - 4, 4);
            boost::to_upper(apt);
            string rwy = "";
            if (line.substr(line.find("/") + 3, 1) == " ") {
                rwy = line.substr(line.find("/") + 1, 2);
            } else if (line.substr(line.find("/") + 4, 1) == " ") {
                rwy = line.substr(line.find("/") + 1, 3);
            }

            bool isTimeOk = false;
            std::istringstream iss(line);
            std::vector<std::string> customDelayValues;
            std::string token;
            while (std::getline(iss, token, ' ')) {
                customDelayValues.push_back(token);
            }
            for (const auto& substring : customDelayValues) {
                std::cout << substring << std::endl;
            }
            string myTime = customDelayValues[customDelayValues.size() - 1];

            if (myTime.length() == 4 && isNumber(myTime)) {
                isTimeOk = true;
            } else {
                if ((myTime.length() == 2 || myTime.length() == 1) && isNumber(myTime)) {
                    isTimeOk = true;
                    myTime = (calculateTime(GetTimeNow(), stoi(myTime)).substr(0, 4));
                }
            }

            if (isTimeOk) {
                // use myTime 9999 to remove delay for APT/RWY config
                if (myTime == "9999") {
                    for (size_t i = 0; i < delayList.size(); i++) {
                        if (delayList[i].airport == apt && delayList[i].rwy == rwy) {
                            sendMessage("REMOVING START-UP DELAY " + apt + "/" + rwy);
                            delayList.erase(delayList.begin() + i);
                        }
                    }
                } else {
                    Delay d = Delay(apt, rwy, myTime, "tsat");

                    // Get Time now
                    time_t rawtime;
                    struct tm ptm;
                    time(&rawtime);
                    gmtime_s(&ptm, &rawtime);
                    string hour = to_string(ptm.tm_hour % 24);
                    string min = to_string(ptm.tm_min);

                    int difTime = difftime(stoi(d.time), stoi(hour + min));

                    if (difTime > 0) {
                        // Remove existing delay for the same airport and runway
                        for (size_t i = 0; i < delayList.size(); i++) {
                            if (delayList[i].airport == apt && delayList[i].rwy == rwy) {
                                delayList.erase(delayList.begin() + i);
                            }
                        }
                        sendMessage("Adding START-UP DELAY for " + apt + " rwy: " + rwy + " from time: " + myTime +
                                    "z.");
                        delayList.push_back(d);
                        // Update times to slaves
                        countTime = std::time(nullptr) - refreshTime;
                        // addTimeToListForSpecificAirportAndRunway(difTime, GetTimeNow(), d.airport, d.rwy);
                    } else {
                        sendMessage("START-UP DELAY NOT ADDED. Time must be in the future");
                    }
                }
            } else {
                sendMessage(
                    "Wrong time formatting to add delay. Please use time in 4 digits format (1234) or minutes with 1 "
                    "or 2 digits codes (12 or 1)");
            }

            return true;
        } catch (const std::exception& e) {
            addLogLine("ERROR: Unhandled exception .cdm startupdelay: " + (string)e.what());
            return true;
        } catch (...) {
            addLogLine("ERROR: Unhandled exception .cdm startupdelay");
            return true;
        }
    }

    if (startsWith(".cdm departuredelay", sCommandLine)) {
        try {
            addLogLine(sCommandLine);
            string line = sCommandLine;
            string apt = line.substr(line.find("/") - 4, 4);
            boost::to_upper(apt);
            string rwy = "";
            if (line.substr(line.find("/") + 3, 1) == " ") {
                rwy = line.substr(line.find("/") + 1, 2);
            } else if (line.substr(line.find("/") + 4, 1) == " ") {
                rwy = line.substr(line.find("/") + 1, 3);
            }

            bool isTimeOk = false;
            std::istringstream iss(line);
            std::vector<std::string> customDelayValues;
            std::string token;
            while (std::getline(iss, token, ' ')) {
                customDelayValues.push_back(token);
            }
            for (const auto& substring : customDelayValues) {
                std::cout << substring << std::endl;
            }
            string myTime = customDelayValues[customDelayValues.size() - 1];

            if (myTime.length() == 4 && isNumber(myTime)) {
                isTimeOk = true;
            } else {
                if ((myTime.length() == 2 || myTime.length() == 1) && isNumber(myTime)) {
                    isTimeOk = true;
                    myTime = (calculateTime(GetTimeNow(), stoi(myTime)).substr(0, 4));
                }
            }

            if (isTimeOk) {
                // use myTime 9999 to remove delay for APT/RWY config
                if (myTime == "9999") {
                    for (size_t i = 0; i < delayList.size(); i++) {
                        if (delayList[i].airport == apt && delayList[i].rwy == rwy) {
                            sendMessage("REMOVING DEPARTURE DELAY " + apt + "/" + rwy);
                            delayList.erase(delayList.begin() + i);
                        }
                    }
                } else {
                    Delay d = Delay(apt, rwy, myTime, "ttot");

                    // Get Time now
                    time_t rawtime;
                    struct tm ptm;
                    time(&rawtime);
                    gmtime_s(&ptm, &rawtime);
                    string hour = to_string(ptm.tm_hour % 24);
                    string min = to_string(ptm.tm_min);

                    int difTime = difftime(stoi(d.time), stoi(hour + min));

                    if (difTime > 0) {
                        // Remove existing delay for the same airport and runway
                        for (size_t i = 0; i < delayList.size(); i++) {
                            if (delayList[i].airport == apt && delayList[i].rwy == rwy) {
                                delayList.erase(delayList.begin() + i);
                            }
                        }
                        sendMessage("Adding DEPARTURE DELAY for " + apt + " rwy: " + rwy + " from time: " + myTime +
                                    "z.");
                        delayList.push_back(d);
                        // Update times to slaves
                        countTime = std::time(nullptr) - refreshTime;
                        // addTimeToListForSpecificAirportAndRunway(difTime, GetTimeNow(), d.airport, d.rwy);
                    } else {
                        sendMessage("DEPARTURE DELAY NOT ADDED. Time must be in the future");
                    }
                }
            } else {
                sendMessage(
                    "Wrong time formatting to add delay. Please use time in 4 digits format (1234) or minutes with 1 "
                    "or 2 digits codes (12 or 1)");
            }

            return true;
        } catch (const std::exception& e) {
            addLogLine("ERROR: Unhandled exception .cdm departuredelay: " + (string)e.what());
            return true;
        } catch (...) {
            addLogLine("ERROR: Unhandled exception .cdm departuredelay");
            return true;
        }
    }

    if (startsWith(".cdm save", sCommandLine)) {
        try {
            addLogLine(sCommandLine);
            sendMessage("Saving CDM data....");
            // save data to file
            ofstream outfile(sfad.c_str());

            for (Plane pl : slotList) {
                outfile << pl.callsign + "," + pl.eobt + "," + pl.tsat + "," + pl.ttot << std::endl;
            }

            outfile.close();
            sendMessage("Done");
            return true;
        } catch (const std::exception& e) {
            addLogLine("ERROR: Unhandled exception .cdm save: " + (string)e.what());
            return true;
        } catch (...) {
            addLogLine("ERROR: Unhandled exception .cdm save");
            return true;
        }
    }

    if (startsWith(".cdm nvo", sCommandLine)) {
        addLogLine(sCommandLine);
        rateString = getFromXml("/CDM/rate/@ops");
        sendMessage("Normal Visibility Operations Rate Set: " + rateString);
        return true;
    }

    if (startsWith(".cdm load", sCommandLine)) {
        addLogLine(sCommandLine);
        sendMessage("Loading CDM data....");
        /*slotList.clear();
        //load data from file
        fstream file;
        string lineValue;
        file.open(sfad.c_str(), std::ios::in);
        while (getline(file, lineValue))
        {
                slotList.push_back(lineValue);
        }*/
        sendMessage("Command disabled");
        return true;
    }

    if (startsWith(".cdm ctot", sCommandLine)) {
        addLogLine(sCommandLine);
        sendMessage("Loading CTOTs data....");

        if (slotURL.length() <= 1) {
            ctotCid = false;
            if (debugMode) {
                sendMessage("[DEBUG MESSAGE] - NOT SHOWING EVCTOTs");
            }
            // Get data from .txt file
            /*fstream fileCtot;
            string lineValueCtot;
            fileCtot.open(cfad.c_str(), std::ios::in);
            while (getline(fileCtot, lineValueCtot))
            {
                    addCtotToMainList(lineValueCtot);
            }*/
        } else {
            ctotCid = true;
            if (debugMode) {
                sendMessage("[DEBUG MESSAGE] - SHOWING EVCTOTs");
            }
            getCtotsFromUrl(slotURL);
        }

        return true;
    }

    if (startsWith(".cdm lvo", sCommandLine)) {
        addLogLine(sCommandLine);
        if (lvo) {
            sendMessage("Low Visibility Operations desactivated");
            lvo = false;
        } else {
            sendMessage("Low Visibility Operations activated");
            lvo = true;
        }
        return true;
    }

    if (startsWith(".cdm master", sCommandLine)) {
        try {
            addLogLine(sCommandLine);
            string line = sCommandLine;
            boost::to_upper(line);
            vector<string> lineAirports = explode(line, ' ');

            if (lineAirports.size() > 2) {
                string ATC_Position = ControllerMyself().GetCallsign();
                for (size_t i = 2; i < lineAirports.size(); i++) {
                    string addedAirport = lineAirports[i];
                    bool found = false;
                    for (string apt : masterAirports) {
                        if (apt == addedAirport) {
                            found = true;
                        }
                    }
                    if (!found) {
                        std::thread t(&CDM::setMasterAirport, this, addedAirport, ATC_Position);
                        t.detach();
                    }
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

    if (startsWith(".cdm slave", sCommandLine)) {
        try {
            addLogLine(sCommandLine);
            string line = sCommandLine;
            boost::to_upper(line);
            vector<string> lineAirports = explode(line, ' ');

            if (lineAirports.size() > 2) {
                string ATC_Position = ControllerMyself().GetCallsign();
                for (size_t i = 2; i < lineAirports.size(); i++) {
                    string addedAirport = lineAirports[i];
                    bool found = false;
                    int a = 0;
                    for (string apt : masterAirports) {
                        if (apt == addedAirport) {
                            std::thread t(&CDM::removeMasterAirport, this, addedAirport, ATC_Position);
                            t.detach();
                            found = true;
                        }
                        a++;
                    }
                    if (!found) {
                        sendMessage("AIRPORT " + addedAirport + " NOT FOUND");
                    }
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

    if (startsWith(".cdm resetmaster", sCommandLine)) {
        try {
            addLogLine(sCommandLine);
            string callsign = "";
            bool correctPosition = false;
            if (ControllerMyself().IsValid()) {
                if (ControllerMyself().IsController()) {
                    callsign = ControllerMyself().GetCallsign();
                    if (callsign.size() > 3) {
                        if (callsign.find("_DEL") != string::npos || callsign.find("_GND") != string::npos ||
                            callsign.find("_TWR") != string::npos || callsign.find("_APP") != string::npos ||
                            callsign.find("_CTR") != string::npos || callsign.find("_FMP") != string::npos) {
                            correctPosition = true;
                        }
                    }
                }
            }
            if (!correctPosition) {
                sendMessage("RESETMASTER NOT SUCCESFULL. NOT AN ACTIVE ATC POSITION.");
                return true;
            }
            string line = sCommandLine;
            boost::to_upper(line);
            vector<string> lineAirports = explode(line, ' ');

            if (lineAirports.size() > 2) {
                string ATC_Position = ControllerMyself().GetCallsign();
                for (size_t i = 2; i < lineAirports.size(); i++) {
                    string addedAirport = lineAirports[i];
                    std::thread t(&CDM::removeAllMasterAirportsByAirport, this, addedAirport);
                    t.detach();
                }
            } else {
                sendMessage("NO AIRPORT SET");
            }

            return true;
        } catch (const std::exception& e) {
            addLogLine("ERROR: Unhandled exception .cdm resetmaster: " + (string)e.what());
            return true;
        } catch (...) {
            addLogLine("ERROR: Unhandled exception .cdm resetmaster");
            return true;
        }
    }

    if (startsWith(".cdm recover", sCommandLine)) {
        try {
            addLogLine(sCommandLine);
            string callsign = "";
            bool correctPosition = false;
            if (ControllerMyself().IsValid()) {
                if (ControllerMyself().IsController()) {
                    callsign = ControllerMyself().GetCallsign();
                    if (callsign.size() > 3) {
                        if (callsign.find("_DEL") != string::npos || callsign.find("_GND") != string::npos ||
                            callsign.find("_TWR") != string::npos || callsign.find("_APP") != string::npos ||
                            callsign.find("_CTR") != string::npos || callsign.find("_FMP") != string::npos) {
                            correctPosition = true;
                        }
                    }
                }
            }
            if (!correctPosition) {
                sendMessage("RECOVER NOT SUCCESFULL. NOT AN ACTIVE ATC POSITION.");
                return true;
            }
            string line = sCommandLine;
            boost::to_upper(line);
            vector<string> lineAirports = explode(line, ' ');

            if (lineAirports.size() > 2) {
                string ATC_Position = ControllerMyself().GetCallsign();
                for (size_t i = 2; i < lineAirports.size(); i++) {
                    string addedAirport = lineAirports[i];
                    std::thread t(&CDM::removeAllMasterAirportsByAirport, this, addedAirport);
                    t.detach();
                    copyServerSavedData(addedAirport);
                }
            } else {
                sendMessage("NO AIRPORT SET");
            }
            return true;
        } catch (const std::exception& e) {
            addLogLine("ERROR: Unhandled exception .cdm recover: " + (string)e.what());
            return true;
        } catch (...) {
            addLogLine("ERROR: Unhandled exception .cdm recover");
            return true;
        }
    }

    if (startsWith(".cdm data", sCommandLine)) {
        addLogLine(sCommandLine);
        string planes = "";
        if (slotList.size() > 0) {
            for (Plane p : slotList) {
                planes += p.callsign + " ";
            }
            sendMessage("PLANES: " + planes);
        } else {
            sendMessage("NO PLANES IN THE LIST");
        }

        return true;
    }

    if (startsWith(".cdm status", sCommandLine)) {
        addLogLine(sCommandLine);
        string apts = "";
        if (masterAirports.size() > 0) {
            for (string apt : masterAirports) {
                apts += apt + " ";
            }
            sendMessage("MASTER AIRPORTS: " + apts);
        } else {
            sendMessage("NO MASTER AIRPORTS");
        }
        sendMessage("DEFAULT RATE: " + rateString);

        return true;
    }
    return false;
}
