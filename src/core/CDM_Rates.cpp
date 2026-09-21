// Rate loading (file/URL) and runway-rate lookup, moved verbatim out of CDMSingle.cpp.
// See src/core/CDMGlobals.hpp for the shared-state rationale.

#include "CDMSingle.hpp"
#include "src/core/CDMGlobals.hpp"

#include <thread>

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

bool CDM::getRateFromUrl(string url) {
    vector<Rate> myRates;
    const auto response = restclient_->get(url);
    int responseCode = response.statusCode;

    if (responseCode == 404 || responseCode == 401 || responseCode == 502 || responseCode == -1) {
        // handle error 404
        addLogLine("UNABLE TO LOAD TaxiZones URL...");
    } else {
        std::istringstream is(response.body);

        // Get data from .txt file
        bool found;
        string lineValue;
        while (getline(is, lineValue)) {
            auto firstNonSpace = lineValue.find_first_not_of(" \t\r\n");
            if (firstNonSpace == std::string::npos) continue;

            if (lineValue[firstNonSpace] == '#') continue;

            vector<string> data = explode(lineValue, ':');
            if (data.size() == 9) {
                string myRateDataAirport = data[0];
                vector<string> ArrRwyList = explode(data[2], ',');
                if (ArrRwyList.size() == 0) {
                    ArrRwyList.push_back(data[2]);
                }
                vector<string> NotArrRwyList = explode(data[3], ',');
                if (NotArrRwyList.size() == 0) {
                    NotArrRwyList.push_back(data[3]);
                }
                vector<string> DepRwyList = explode(data[5], ',');
                if (DepRwyList.size() == 0) {
                    DepRwyList.push_back(data[5]);
                }
                vector<string> NotDepRwyList = explode(data[6], ',');
                if (NotDepRwyList.size() == 0) {
                    NotDepRwyList.push_back(data[6]);
                }
                vector<string> DependentRwyList = explode(data[7], ',');
                if (DependentRwyList.size() == 0) {
                    DependentRwyList.push_back(data[7]);
                }
                vector<string> ratesFromData = explode(data[8], ',');
                vector<string> ratesFromDataRate;
                vector<string> ratesFromDataRateLvo;
                if (ratesFromData.size() == 0) {
                    ratesFromData.push_back(data[8]);
                } else {
                    for (string completeRate : ratesFromData) {
                        vector<string> tempCompleteRate = explode(completeRate, '_');
                        ratesFromDataRate.push_back(tempCompleteRate[0]);
                        ratesFromDataRateLvo.push_back(tempCompleteRate[1]);
                    }
                }

                Rate myRate(myRateDataAirport, ArrRwyList, NotArrRwyList, DepRwyList, NotDepRwyList, DependentRwyList,
                            ratesFromDataRate, ratesFromDataRateLvo);

                myRates.push_back(myRate);

                found = false;
                for (string airport : CDMairports) {
                    if (airport == myRateDataAirport) {
                        found = true;
                    }
                }
                if (!found) {
                    CDMairports.push_back(myRateDataAirport);
                }
            }
        }
    }
    vector<Rate> ratesChanged;
    for (Rate r1 : rate) {
        for (Rate r2 : myRates) {
            if (r1.airport == r2.airport && r1.arrRwyNo == r2.arrRwyNo && r1.arrRwyYes == r2.arrRwyYes &&
                r1.dependentRwy == r2.dependentRwy && r1.depRwyNo == r2.depRwyNo && r1.depRwyYes == r2.depRwyYes &&
                r1.rates.size() == r2.rates.size() && r1.ratesLvo.size() == r2.ratesLvo.size()) {
                for (size_t i = 0; i < r1.rates.size(); i++) {
                    if (r1.rates[i] != r2.rates[i]) {
                        ratesChanged.push_back(r2);
                    }
                }
            }
        }
    }
    for (Rate r : ratesChanged) {
        for (size_t z = 0; z < slotList.size(); z++) {
            CFlightPlan fp = FlightPlanSelect(slotList[z].callsign.c_str());
            if (!fp.IsValid()) {
                continue;
            }
            if ((string)fp.GetGroundState() != "STUP" && (string)fp.GetGroundState() != "ST-UP" &&
                (string)fp.GetGroundState() != "PUSH" && (string)fp.GetGroundState() != "TAXI" &&
                (string)fp.GetGroundState() != "DEPA") {
                Rate dataRate =
                    rateForRunway(fp.GetFlightPlanData().GetOrigin(), fp.GetFlightPlanData().GetDepartureRwy(), fp.GetFlightPlanData().GetSidName());
                if (r.airport == dataRate.airport && r.arrRwyNo == dataRate.arrRwyNo &&
                    r.arrRwyYes == dataRate.arrRwyYes && r.dependentRwy == dataRate.dependentRwy &&
                    r.depRwyNo == dataRate.depRwyNo && r.depRwyYes == dataRate.depRwyYes &&
                    r.rates.size() == dataRate.rates.size() && r.ratesLvo.size() == dataRate.ratesLvo.size()) {
                    for (string dr : dataRate.depRwyYes) {
                        int myPos = 0;
                        if (dataRate.rates.size() > 1) {
                            for (size_t i = 0; i < dataRate.rates.size(); i++) {
                                if (dataRate.rates[i] != r.rates[i]) {
                                    myPos = i;
                                }
                            }
                        }
                        // Increase time according the rate change
                        slotList[z].tsat = calculateTime(slotList[z].tsat, (60.0 / stoi(r.rates[myPos])));
                        slotList[z].ttot = calculateTime(slotList[z].ttot, (60.0 / stoi(r.rates[myPos])));
                    }
                }
            }
        }
    }

    rate = myRates;
    initialRate = rate;

    return true;
}

bool CDM::getRate() {
    vector<Rate> myRates;
    // Get data from rate.txt file
    fstream rateFile;
    string lineValue;
    rateFile.open(rfad.c_str(), std::ios::in);
    if (!rateFile.is_open()) {
        sendMessage("Error", "rate.txt not found. Place rate.txt next to CDM.dll.");
        addLogLine("ERROR: rate.txt not found at " + rfad);
        return false;
    }
    bool found;
    while (getline(rateFile, lineValue)) {
        vector<string> data = explode(lineValue, ':');
        if (data.size() == 9) {
            string myRateDataAirport = data[0];
            vector<string> ArrRwyList = explode(data[2], ',');
            if (ArrRwyList.size() == 0) {
                ArrRwyList.push_back(data[2]);
            }
            vector<string> NotArrRwyList = explode(data[3], ',');
            if (NotArrRwyList.size() == 0) {
                NotArrRwyList.push_back(data[3]);
            }
            vector<string> DepRwyList = explode(data[5], ',');
            if (DepRwyList.size() == 0) {
                DepRwyList.push_back(data[5]);
            }
            vector<string> NotDepRwyList = explode(data[6], ',');
            if (NotDepRwyList.size() == 0) {
                NotDepRwyList.push_back(data[6]);
            }
            vector<string> DependentRwyList = explode(data[7], ',');
            if (DependentRwyList.size() == 0) {
                DependentRwyList.push_back(data[7]);
            }
            vector<string> ratesFromData = explode(data[8], ',');
            vector<string> ratesFromDataRate;
            vector<string> ratesFromDataRateLvo;
            if (ratesFromData.size() == 0) {
                ratesFromData.push_back(data[8]);
            } else {
                for (string completeRate : ratesFromData) {
                    vector<string> tempCompleteRate = explode(completeRate, '_');
                    ratesFromDataRate.push_back(tempCompleteRate[0]);
                    ratesFromDataRateLvo.push_back(tempCompleteRate[1]);
                }
            }

            Rate myRate(myRateDataAirport, ArrRwyList, NotArrRwyList, DepRwyList, NotDepRwyList, DependentRwyList,
                        ratesFromDataRate, ratesFromDataRateLvo);

            myRates.push_back(myRate);

            found = false;
            for (string airport : CDMairports) {
                if (airport == myRateDataAirport) {
                    found = true;
                }
            }
            if (!found) {
                CDMairports.push_back(myRateDataAirport);
            }
        }
    }
    vector<Rate> ratesChanged;
    for (Rate r1 : rate) {
        for (Rate r2 : myRates) {
            if (r1.airport == r2.airport && r1.arrRwyNo == r2.arrRwyNo && r1.arrRwyYes == r2.arrRwyYes &&
                r1.dependentRwy == r2.dependentRwy && r1.depRwyNo == r2.depRwyNo && r1.depRwyYes == r2.depRwyYes &&
                r1.rates.size() == r2.rates.size() && r1.ratesLvo.size() == r2.ratesLvo.size()) {
                for (size_t i = 0; i < r1.rates.size(); i++) {
                    if (r1.rates[i] != r2.rates[i]) {
                        ratesChanged.push_back(r2);
                    }
                }
            }
        }
    }
    for (Rate r : ratesChanged) {
        for (size_t z = 0; z < slotList.size(); z++) {
            CFlightPlan fp = FlightPlanSelect(slotList[z].callsign.c_str());
            if (!fp.IsValid()) {
                continue;
            }
            if ((string)fp.GetGroundState() != "STUP" && (string)fp.GetGroundState() != "ST-UP" &&
                (string)fp.GetGroundState() != "PUSH" && (string)fp.GetGroundState() != "TAXI" &&
                (string)fp.GetGroundState() != "DEPA") {
                Rate dataRate =
                    rateForRunway(fp.GetFlightPlanData().GetOrigin(), fp.GetFlightPlanData().GetDepartureRwy(), fp.GetFlightPlanData().GetSidName());
                if (r.airport == dataRate.airport && r.arrRwyNo == dataRate.arrRwyNo &&
                    r.arrRwyYes == dataRate.arrRwyYes && r.dependentRwy == dataRate.dependentRwy &&
                    r.depRwyNo == dataRate.depRwyNo && r.depRwyYes == dataRate.depRwyYes &&
                    r.rates.size() == dataRate.rates.size() && r.ratesLvo.size() == dataRate.ratesLvo.size()) {
                    for (string dr : dataRate.depRwyYes) {
                        int myPos = 0;
                        if (dataRate.rates.size() > 1) {
                            for (size_t i = 0; i < dataRate.rates.size(); i++) {
                                if (dataRate.rates[i] != r.rates[i]) {
                                    myPos = i;
                                }
                            }
                        }
                        // Increase time according the rate change
                        slotList[z].tsat = calculateTime(slotList[z].tsat, (60.0 / stoi(r.rates[myPos])));
                        slotList[z].ttot = calculateTime(slotList[z].ttot, (60.0 / stoi(r.rates[myPos])));
                    }
                }
            }
        }
    }

    rate = myRates;
    initialRate = rate;

    return true;
}

Rate CDM::rateForRunway(string airport, string depRwy, string mySid) {
    string lineAirport, lineDepRwy;
    Rate knownMyrate;

    // This function is called per-aircraft on every recalculation, but the sector file's active-runway
    // configuration barely ever changes - walking every SECTOR_ELEMENT_RUNWAY (across ALL airports,
    // with 4 native IsElementActive() calls each) on every single call is the actual bottleneck.
    // Scan once and cache per airport for a few seconds instead of once per aircraft.
    static unordered_map<string, vector<string>> activeDepRwyCache;
    static unordered_map<string, vector<string>> activeArrRwyCache;
    static time_t activeRwyCacheTime = 0;
    time_t rwyCacheNow = std::time(nullptr);
    if (rwyCacheNow - activeRwyCacheTime > 5) {
        activeDepRwyCache.clear();
        activeArrRwyCache.clear();
        string myairport;
        for (CSectorElement runway = this->SectorFileElementSelectFirst(SECTOR_ELEMENT_RUNWAY); runway.IsValid();
             runway = this->SectorFileElementSelectNext(runway, SECTOR_ELEMENT_RUNWAY)) {
            if (runway.IsElementActive(false, 1)) {
                myairport = runway.GetAirportName();
                activeArrRwyCache[myairport.substr(0, 4)].push_back(runway.GetRunwayName(1));
            }
            if (runway.IsElementActive(false, 0)) {
                myairport = runway.GetAirportName();
                activeArrRwyCache[myairport.substr(0, 4)].push_back(runway.GetRunwayName(0));
            }
            if (runway.IsElementActive(true, 1)) {
                myairport = runway.GetAirportName();
                activeDepRwyCache[myairport.substr(0, 4)].push_back(runway.GetRunwayName(1));
            }
            if (runway.IsElementActive(true, 0)) {
                myairport = runway.GetAirportName();
                activeDepRwyCache[myairport.substr(0, 4)].push_back(runway.GetRunwayName(0));
            }
        }
        activeRwyCacheTime = rwyCacheNow;
    }
    vector<string>& myActiveRwysDep = activeDepRwyCache[airport];
    vector<string>& myActiveRwysArr = activeArrRwyCache[airport];

    for (Rate r : rate) {
        if (r.airport == airport) {
            bool found = false;
            for (string dr : r.depRwyYes) {
                if (depRwy == dr) {
                    found = true;
                } else if (dr == "*") {
                    found = true;
                }
            }

            if (found) {
                bool foundArrRwyYes = false;
                for (string ar : r.arrRwyYes) {
                    if (ar == "*") {
                        foundArrRwyYes = true;
                    } else {
                        for (string arrRwy : myActiveRwysArr) {
                            if (arrRwy == ar) {
                                foundArrRwyYes = true;
                            }
                        }
                    }
                }

                bool foundArrRwyNo = true;
                for (string arn : r.arrRwyNo) {
                    if (arn == "*") {
                        foundArrRwyNo = true;
                    } else {
                        bool foundIt = false;
                        for (string arrRwy : myActiveRwysArr) {
                            if (arrRwy != arn) {
                                foundIt = true;
                            }
                        }
                        if (!foundIt) {
                            foundArrRwyNo = false;
                        } else if (myActiveRwysArr.size() != r.arrRwyNo.size()) {
                            foundArrRwyNo = false;
                        }
                    }
                }

                bool foundDepRwyYes = true;
                for (string dr : r.depRwyYes) {
                    if (dr == "*") {
                        foundDepRwyYes = true;
                    } else {
                        for (string myRwy : myActiveRwysDep) {
                            if (myRwy == dr) {
                                foundDepRwyYes = true;
                            }
                        }
                    }
                }

                bool foundDepRwyNo = true;
                for (string drn : r.depRwyNo) {
                    if (drn == "*") {
                        foundDepRwyNo = true;
                    } else {
                        bool foundIt = false;
                        for (string myRwy : myActiveRwysDep) {
                            if (myRwy != drn) {
                                foundIt = true;
                            }
                        }
                        if (!foundIt) {
                            foundDepRwyNo = false;
                        } else if (myActiveRwysDep.size() != r.depRwyNo.size()) {
                            foundDepRwyNo = false;
                        }
                    }
                }

                // Check if ok to be valid rate
                if (foundArrRwyYes && foundArrRwyNo && foundDepRwyYes && foundDepRwyNo) {
                    if (mySid != "" && knownMyrate.airport != "-1") {
                        int a = 0;
                        int dataRatePos = 0;
                        if (r.rates.size() > 1) {
                            for (const string& dr : r.depRwyYes) {
                                if (dr == depRwy) {
                                    dataRatePos = a;
                                    break;
                                }
                                a++;
                            }
                        }
                        if (dataRatePos < r.rates.size()) {
                            knownMyrate.rates = {r.rates[dataRatePos]};
                            knownMyrate.ratesLvo = {r.ratesLvo[dataRatePos]};
                        }
                        return knownMyrate;
                    }
                    return r;
                }
            } else {
                // Check if SID is found in dependent runway is valid
                for (string dr : r.dependentRwy) {
                    if (patternMatches(dr, mySid)) {
                        knownMyrate = r;
                        break;
                    }
                }
            }
        }
    }
    return Rate("-1");
}

int CDM::getHourlyRateForRunway(const string& airport, const string& depRwy) {
    // Extract hourly rate using same logic as TTOT calculations
    Rate dataRate = rateForRunway(airport, depRwy, "");
    
    if (dataRate.airport == "-1") {
        // No matching rate found in rate.txt, fall back to XML config
        if (!rateString.empty()) {
            try {
                return std::stoi(rateString);
            } catch (...) {}
        }
        return 6;  // Default
    }
    
    // Found matching rate - extract correct rate using runway position
    int a = 0;
    int dataRatePos = 0;
    if (dataRate.rates.size() > 1) {
        for (const string& dr : dataRate.depRwyYes) {
            if (dr == depRwy) {
                dataRatePos = a;
            }
            a++;
        }
    }
    try {
        return std::stoi(dataRate.rates[dataRatePos]);
    } catch (...) {
        return 6;
    }
}

void CDM::RemoveMasterAirports() {
    if (!masterAirports.empty()) {
        string ATC_Position = myAtcCallsign;
        if (ATC_Position.size() < 2) {
            ATC_Position = ControllerMyself().GetCallsign();
        }
        sendMessage("Removed master airports from previous connection.");
        std::thread t(&CDM::removeAllMasterAirports, this, ATC_Position);
        t.detach();
        myAtcCallsign = ControllerMyself().GetCallsign();
    }
}
