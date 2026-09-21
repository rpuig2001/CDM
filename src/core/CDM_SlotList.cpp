// Slot-list recalculation background pass + windowed TTOT algorithm, moved verbatim out of
// CDMSingle.cpp. See src/core/CDMGlobals.hpp for the shared-state rationale.

#include "CDMSingle.hpp"
#include "src/core/CDMGlobals.hpp"

vector<Plane> CDM::backgroundProcess_recaulculate() {
    addLogLine("Called backgroundProcess_recaulculate...");
    try {
        vector<Plane> tempSlotList;
        vector<Plane> copySlotList = slotList;
        addLogLine("[AUTO] - Starting CDM recalculation process");
        // Get Time NOW
        time_t rawtime;
        struct tm ptm;
        time(&rawtime);
        gmtime_s(&ptm, &rawtime);
        string hour = to_string(ptm.tm_hour % 24);
        string min = to_string(ptm.tm_min);
        string timeNow = GetActualTime() + "00";

        for (size_t i = 0; i < copySlotList.size(); i++) {
            // Update TSAT in scratchpad if enabled remarksOption
            if (remarksOption || remarksOptionCtot) {
                CFlightPlan fplSelect = FlightPlanSelect(copySlotList[i].callsign.c_str());
                string testTsat = copySlotList[i].tsat;
                string testCtot = copySlotList[i].ctot;
                if (!fplSelect.IsValid()) {
                    continue;
                }
                if ((string)fplSelect.GetGroundState() != "STUP" && (string)fplSelect.GetGroundState() != "ST-UP" &&
                    (string)fplSelect.GetGroundState() != "PUSH" && (string)fplSelect.GetGroundState() != "TAXI" &&
                    (string)fplSelect.GetGroundState() != "DEPA") {
                    if (testCtot.length() >= 4 && remarksOptionCtot) {
                        fplSelect.GetControllerAssignedData().SetScratchPadString(
                            std::string(".C" + testCtot.substr(0, 4)).c_str());
                    } else if (testTsat.length() >= 4 && remarksOption) {
                        fplSelect.GetControllerAssignedData().SetScratchPadString(testTsat.substr(0, 4).c_str());
                    } else if (testCtot == "" && remarksOptionCtot) {
                        string scratchPadOld = fplSelect.GetControllerAssignedData().GetScratchPadString();
                        if (scratchPadOld.find(".C") != string::npos) {
                             fplSelect.GetControllerAssignedData().SetScratchPadString("");
                        }
                    }
                }
            }

            string myCallsign = copySlotList[i].callsign;

            string myTTOT, myTSAT, myEOBT, myAirport, myDepRwy = "";
            int myTTime = defTaxiTime;

            // Check if aircraft has state and no need to recalculate
            bool aicraftInFinalTimesList = false;
            for (string aircraft : finalTimesList) {
                if (aircraft == myCallsign) {
                    aicraftInFinalTimesList = true;
                }
            }

            //Mark acftInLastTsatMinutes true, when TSAT is earlier than "now" and NOT in aicraftInFinalTimesList
            string timeNow = GetActualTime() + "00";
            bool acftInLastTsatMinutes = (stoi(copySlotList[i].tsat) < stoi(timeNow)) && !aicraftInFinalTimesList;

            // Do not calculate if has CTOT
            // if (copySlotList[i].hasManualCtot && copySlotList[i].ctot != "") aicraftInFinalTimesList = false;
            
            //Run always this when ATOT enabled to keep flights in sequence and keep rolling/updated TTOT in the order affecting not yet modified flights
            if ((atotEnabled || !aicraftInFinalTimesList) && !acftInLastTsatMinutes) {
                CFlightPlan myFlightPlan = FlightPlanSelect(myCallsign.c_str());
                if (!myFlightPlan.IsValid()) {
                    continue;
                }

                for (size_t s = 0; s < planeAiportList.size(); s++) {
                    if (myCallsign == planeAiportList[s].substr(0, planeAiportList[s].find(","))) {
                        myAirport = planeAiportList[s].substr(planeAiportList[s].find(",") + 1, 4);
                    }
                }

                bool depRwyFound = false;
                for (size_t t = 0; t < taxiTimesList.size(); t++) {
                    if (myCallsign == taxiTimesList[t].substr(0, taxiTimesList[t].find(","))) {
                        if (taxiTimesList[t].substr(taxiTimesList[t].find(",") + 3, 1) == ",") {
                            myDepRwy = taxiTimesList[t].substr(taxiTimesList[t].find(",") + 1, 2);
                            depRwyFound = true;
                        } else if (taxiTimesList[t].substr(taxiTimesList[t].find(",") + 4, 1) == ",") {
                            myDepRwy = taxiTimesList[t].substr(taxiTimesList[t].find(",") + 1, 3);
                            depRwyFound = true;
                        }

                        if (taxiTimesList[t].substr(taxiTimesList[t].length() - 2, 1) == ",") {
                            myTTime = stoi(taxiTimesList[t].substr(taxiTimesList[t].length() - 1, 1));
                        } else {
                            myTTime = stoi(taxiTimesList[t].substr(taxiTimesList[t].length() - 2, 2));
                        }
                    }
                }

                myEOBT = copySlotList[i].eobt;

                Rate dataRate = rateForRunway(myFlightPlan.GetFlightPlanData().GetOrigin(),
                                              myFlightPlan.GetFlightPlanData().GetDepartureRwy(),
                                              myFlightPlan.GetFlightPlanData().GetSidName());

                bool tempAddTime_DELAY_TSAT = false;
                bool tempAddTime_DELAY_TTOT = false;
                string myTimeToAddTemp_DELAY = "";
                for (Delay d : delayList) {
                    if (d.airport == myFlightPlan.GetFlightPlanData().GetOrigin() &&
                        d.rwy == myFlightPlan.GetFlightPlanData().GetDepartureRwy()) {
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

                myTSAT = myEOBT;
                myTTOT = calculateTime(myEOBT, myTTime);
                if ((addTime || tempAddTime_DELAY_TSAT || tempAddTime_DELAY_TTOT) && !aicraftInFinalTimesList) {
                    if (tempAddTime_DELAY_TSAT || tempAddTime_DELAY_TTOT) {
                        string timeToUse = myTimeToAddTemp_DELAY;
                        if (tempAddTime_DELAY_TTOT) {
                            timeToUse = calculateLessTime(myTimeToAddTemp_DELAY, myTTime);
                        }
                        int addMinutes = ToMinutes(timeToUse);
                        int tsatMinutes = ToMinutes(myTSAT);

                        int diff = addMinutes - tsatMinutes;

                        // Handle midnight crossing
                        if (diff < -720)
                            diff += 1440;
                        else if (diff > 720)
                            diff -= 1440;

                        bool fixTime = (diff <= 0);

                        if (!fixTime) {
                            myTSAT = timeToUse;
                            myTTOT = calculateTime(timeToUse, myTTime);
                        }
                    } else {
                        int addMinutes = ToMinutes(myTimeToAdd);
                        int tsatMinutes = ToMinutes(myTSAT);

                        int diff = addMinutes - tsatMinutes;

                        // Handle midnight crossing
                        if (diff < -720)
                            diff += 1440;
                        else if (diff > 720)
                            diff -= 1440;

                        bool fixTime = (diff <= 0);

                        if (!fixTime) {
                            myTSAT = myTimeToAdd;
                            myTTOT = calculateTime(myTimeToAdd, myTTime);
                        }
                    }
                }

                if (copySlotList[i].hasManualCtot && copySlotList[i].ctot != "") {
                    string ctotTTOT = copySlotList[i].ctot + "00";
                    string ctotTSAT  = calculateLessTime(ctotTTOT, myTTime);
                    // Only use CTOT if it doesn't push TSAT before EOBT
                    if (stoi(ctotTSAT) >= stoi(myEOBT)) {
                        myTTOT = ctotTTOT;
                    } else {
                        myTTOT = calculateTime(myEOBT, myTTime); // natural departure
                    }
                }

                Plane item = refreshTimes(copySlotList[i], tempSlotList, myFlightPlan, myCallsign, myEOBT, myTSAT, myTTOT,
                                 myAirport, myTTime, myDepRwy, dataRate, true, aicraftInFinalTimesList);
                tempSlotList.push_back(item);
                // refreshTimes(myFlightPlan, myCallsign, myEOBT, myTSAT, myTTOT, myAirport, myTTime, myRemarks,
                // myDepRwy, dataRate, myhasCTOT, myCtotPos, i, true);
            } else {
                tempSlotList.push_back(copySlotList[i]);
            }
        }

        addLogLine("[AUTO] - Finished CDM recalculation process");
        return tempSlotList;
    } catch (const std::exception& e) {
        addLogLine("ERROR: Unhandled exception backgroundProcess_recaulculate: " + (string)e.what());
    } catch (...) {
        addLogLine("ERROR: Unhandled exception backgroundProcess_recaulculate");
    }
    return slotList;
}

Plane CDM::refreshTimes(Plane plane, vector<Plane> planes, CFlightPlan FlightPlan, string callsign, string EOBT,
                        string TSATfinal, string TTOTFinal, string origin, int taxiTime, string depRwy, Rate dataRate,
                        bool aircraftFind, bool aicraftInFinalTimesList) {
    try {
        bool equalTTOT = true;
        bool correctTTOT = true;
        bool equalTempoTTOT = true;
        bool alreadySetTOStd = false;
        bool okToLook = false;
        string timeNow = GetActualTime() + "00";
        bool initialTSATInPast = (stoi(plane.tsat) < stoi(timeNow));
        string myFlow = plane.flowReason;

        // Calculate Rate
        int rate;
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

        okToLook = true;
        //Disabled to allow modifications of all flights without GND Status, not only in the future.
        /*string checkedTSAT = plane.tsat;
        if (stoi(checkedTSAT) >= stoi(timeNow)) {
            okToLook = true;
        }*/

        if (okToLook) {
            bool sameOrDependantRwys = false;
            string mySid = FlightPlan.GetFlightPlanData().GetSidName();

            while (equalTTOT) {
                correctTTOT = true;
                // Checks if the calculated TSAT is in the past and adjusts TTOT accordingly
                if (!aicraftInFinalTimesList) {
                    string calculatedTSATNow = calculateLessTime(TTOTFinal, taxiTime);
                    if (calculatedTSATNow.substr(0, 2) == "00") {
                        calculatedTSATNow = "24" + calculatedTSATNow.substr(2, 4);
                    }
                    if (stoi(calculatedTSATNow) < stoi(timeNow) && !initialTSATInPast) {
                        TTOTFinal = calculateTime(TTOTFinal, 0.5);
                        correctTTOT = false;
                    }
                }
                if (correctTTOT) {
                    if (bmiMode) {
                        TTOTFinal = getCorrectTTOT_Windowed(TTOTFinal, plane.hasManualCtot, planes, rate, plane.callsign, origin,
                                                        depRwy, timeNow, taxiTime, mySid, aicraftInFinalTimesList);
                    } else {
                        TTOTFinal = getCorrectTTOT(TTOTFinal, plane.hasManualCtot, planes, rate, plane.callsign, origin,
                                                        depRwy, timeNow, taxiTime, mySid, dataRate, sameOrDependantRwys);
                    }
                }

                if (correctTTOT) {
                    bool doRequest = false;
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
                        int deIceTime = getDeIceTime(FlightPlan.GetFlightPlanData().GetAircraftWtc(), 0);
                        TSATfinal = calculateTime(TSATfinal, deIceTime);
                    }
                    /* END Check stand de-ice */
                    string TSAT = TSATfinal.c_str();
                    string TTOT = TTOTFinal.c_str();
                    if (plane.hasManualCtot) {
                        if (aircraftFind) {
                            if (TTOT != plane.ttot && TTOT.length() >= 4) {
                                Plane p(callsign, EOBT, TSAT, TTOT, plane.ctot, myFlow, plane.hasManualCtot, true,
                                        true);
                                plane = p;
                                doRequest = true;
                                setFlightStripInfo(FlightPlan, p.tsat, 3);
                                setFlightStripInfo(FlightPlan, p.ttot, 4);
                            }
                        } else if (TTOT.length() >= 4) {
                            Plane p(callsign, EOBT, TSAT, TTOT, plane.ctot, myFlow, plane.hasManualCtot, true, true);
                            plane = p;
                            setFlightStripInfo(FlightPlan, p.tsat, 3);
                            setFlightStripInfo(FlightPlan, p.ttot, 4);
                        }
                    } else {
                        if (aircraftFind) {
                            if (TTOT != plane.ttot && TTOT.length() >= 4) {
                                Plane p(callsign, EOBT, TSAT, TTOT, plane.ctot, myFlow, plane.hasManualCtot, true,
                                        true);
                                plane = p;
                                doRequest = true;
                                setFlightStripInfo(FlightPlan, p.tsat, 3);
                                setFlightStripInfo(FlightPlan, p.ttot, 4);
                            }
                        } else if (TTOT.length() >= 4) {
                            Plane p(callsign, EOBT, TSAT, TTOT, plane.ctot, myFlow, plane.hasManualCtot, true, true);
                            plane = p;
                            setFlightStripInfo(FlightPlan, p.tsat, 3);
                            setFlightStripInfo(FlightPlan, p.ttot, 4);
                        }
                    }
                    // Check API
                    if (doRequest && !aicraftInFinalTimesList && TSATfinal.length() >= 4) {
                        if (serverEnabled) {
                            string myTSATApi = TSAT;
                            if (plane.hasManualCtot && plane.ctot != "" && plane.ttot.length() >= 4) {
                                string myTTOT = TTOT;
                                myTTOT = myTTOT.substr(0, 4);
                                if (stoi(myTTOT) > stoi(plane.ctot) &&
                                    stoi(myTTOT + "00") <= stoi(calculateTime(plane.ctot + "00", 7))) {
                                    // Update TOBT API with TSAT if TTOT is greater than CTOT but less or equal to
                                    // CTOT+7
                                    string myCOBT = calculateLessTime(plane.ctot + "00", taxiTime);
                                    setOBTApi(callsign, myCOBT, false, false);
                                } else {
                                    setOBTApi(callsign, myTSATApi, false, false);
                                }

                            } else {
                                setOBTApi(callsign, myTSATApi, false, false);
                            }
                        }
                    }
                }
            }
        }
        return plane;
    } catch (const std::exception& e) {
        addLogLine("ERROR: Unhandled exception refreshTimes: " + (string)e.what());
        return plane;
    } catch (...) {
        addLogLine("ERROR: Unhandled exception refreshTimes");
        return plane;
    }
}

// Custom block capacity management
void CDM::setCustomBlockCapacity(const std::string& runway, int blockHour, int blockIndex, int capacity) {
    if (blockIndex >= 0 && blockIndex < 6) {
        customBlockCapacities[std::make_tuple(runway, blockHour, blockIndex)] = capacity;
    }
}

int CDM::getCustomBlockCapacity(const std::string& runway, int blockHour, int blockIndex) {
    auto key = std::make_tuple(runway, blockHour, blockIndex);
    if (customBlockCapacities.find(key) != customBlockCapacities.end()) {
        return customBlockCapacities[key];
    }
    return -1;  // Return -1 if no custom capacity set (means use default)
}

void CDM::clearCustomBlockCapacity(const std::string& runway, int blockHour, int blockIndex) {
    auto key = std::make_tuple(runway, blockHour, blockIndex);
    if (customBlockCapacities.find(key) != customBlockCapacities.end()) {
        customBlockCapacities.erase(key);
    }
}

string CDM::getCorrectTTOT(string TTOTInitial, bool hasManualCtot, const vector<Plane>& planes, int rateHour,
                           const string& callsign, const string& origin, const string& depRwy, const string& timeNow,
                           double taxiTime, const string& mySid, Rate dataRate, bool& sameOrDependantRwysOut) {
    string TTOTFinal = TTOTInitial;
    double rateHourMinutes = (double)60 / rateHour;
    bool alreadySetTOStd = false;
    bool correctTTOT = false;

    while (!correctTTOT) {
        correctTTOT = true;
        for (size_t t = 0; t < planes.size(); t++) {
            string listTTOT;
            string listCallsign = planes[t].callsign;
            string listDepRwy = "";
            CFlightPlan listFlightPlan = FlightPlanSelect(listCallsign.c_str());
            if (!listFlightPlan.IsValid()) {
                continue;
            }
            string listSid = listFlightPlan.GetFlightPlanData().GetSidName();
            bool depRwyFound = false;
            for (size_t i = 0; i < taxiTimesList.size(); i++) {
                if (listCallsign == taxiTimesList[i].substr(0, taxiTimesList[i].find(","))) {
                    if (taxiTimesList[i].substr(taxiTimesList[i].find(",") + 3, 1) == ",") {
                        listDepRwy = taxiTimesList[i].substr(taxiTimesList[i].find(",") + 1, 2);
                        depRwyFound = true;
                    } else if (taxiTimesList[i].substr(taxiTimesList[i].find(",") + 4, 1) == ",") {
                        listDepRwy = taxiTimesList[i].substr(taxiTimesList[i].find(",") + 1, 3);
                        depRwyFound = true;
                    }
                }
            }
            string listAirport;
            for (size_t i = 0; i < planeAiportList.size(); i++) {
                if (listCallsign == planeAiportList[i].substr(0, planeAiportList[i].find(","))) {
                    listAirport = planeAiportList[i].substr(planeAiportList[i].find(",") + 1, 4);
                }
            }

            if (!depRwyFound) {
                listDepRwy = depRwy;
            }

            bool sameOrDependantRwys = false;

            if (depRwy == listDepRwy) {
                sameOrDependantRwys = true;
            }

            if (dataRate.airport != "-1") {
                for (string testRwy : dataRate.dependentRwy) {
                    if (testRwy == listDepRwy || patternMatches(testRwy, listSid)) {
                        sameOrDependantRwys = true;
                    }
                }
            }
            sameOrDependantRwysOut = sameOrDependantRwys;

            if (hasManualCtot) {
                bool found = false;
                while (!found) {
                    found = true;
                    if (planes[t].hasManualCtot) {
                        listTTOT = planes[t].ttot;

                        if (TTOTFinal == listTTOT && callsign != listCallsign && sameOrDependantRwys &&
                            listAirport == origin) {
                            found = false;
                            if (alreadySetTOStd) {
                                TTOTFinal = calculateTime(TTOTFinal, 0.5);
                                correctTTOT = false;
                            } else {
                                TTOTFinal = calculateTime(listTTOT, 0.5);
                                correctTTOT = false;
                                alreadySetTOStd = true;
                            }
                        } else if (callsign != listCallsign && sameOrDependantRwys && listAirport == origin) {
                            if ((stoi(TTOTFinal) < stoi(calculateTime(listTTOT, rateHourMinutes))) &&
                                (stoi(TTOTFinal) > stoi(calculateLessTime(listTTOT, rateHourMinutes)))) {
                                found = false;
                                if (alreadySetTOStd) {
                                    TTOTFinal = calculateTime(TTOTFinal, 0.5);
                                    correctTTOT = false;
                                } else {
                                    TTOTFinal = calculateTime(listTTOT, 0.5);
                                    correctTTOT = false;
                                    alreadySetTOStd = true;
                                }
                            }
                        }
                    }
                }
            } else {
                bool found = false;
                while (!found) {
                    found = true;
                    listTTOT = planes[t].ttot;

                    if (planes[t].tsat == "999999") {
                        listDepRwy = depRwy;
                        listAirport = origin;
                    }

                    if (TTOTFinal == listTTOT && callsign != listCallsign && sameOrDependantRwys &&
                        listAirport == origin) {
                        found = false;
                        if (alreadySetTOStd) {
                            TTOTFinal = calculateTime(TTOTFinal, 0.5);
                            correctTTOT = false;
                        } else {
                            TTOTFinal = calculateTime(listTTOT, 0.5);
                            correctTTOT = false;
                            alreadySetTOStd = true;
                        }
                    } else if (callsign != listCallsign && sameOrDependantRwys && listAirport == origin) {
                        if ((stoi(TTOTFinal) < stoi(calculateTime(listTTOT, rateHourMinutes))) &&
                            (stoi(TTOTFinal) > stoi(calculateLessTime(listTTOT, rateHourMinutes)))) {
                            found = false;
                            if (alreadySetTOStd) {
                                TTOTFinal = calculateTime(TTOTFinal, 0.5);
                                correctTTOT = false;
                            } else {
                                TTOTFinal = calculateTime(listTTOT, 0.5);
                                correctTTOT = false;
                                alreadySetTOStd = true;
                            }
                        }
                    }
                }
            }
            // Check SID Interval
            if (correctTTOT && sidIntervalEnabled && callsign != listCallsign) {
                listTTOT = planes[t].ttot;
                double interval = getSidInterval(mySid, listSid, origin, depRwy, listDepRwy);
                if (interval > 0) {
                    bool found = false;
                    while (!found) {
                        found = true;
                        if ((stoi(TTOTFinal) < stoi(calculateTime(listTTOT, interval))) &&
                            (stoi(TTOTFinal) > stoi(calculateLessTime(listTTOT, interval)))) {
                            found = false;
                            if (alreadySetTOStd) {
                                TTOTFinal = calculateTime(TTOTFinal, 0.5);
                                correctTTOT = false;
                            } else {
                                TTOTFinal = calculateTime(listTTOT, 0.5);
                                correctTTOT = false;
                                alreadySetTOStd = true;
                            }
                        }
                    }
                }
            }
        }
    }

    return TTOTFinal;
}

string CDM::getCorrectTTOT_Windowed(string TTOTInitial, bool hasManualCtot, const vector<Plane>& planes, int rateHour,
                                    const string& callsign, const string& origin, const string& depRwy,
                                    const string& timeNow, double taxiTime, const string& mySid,
                                    bool aicraftInFinalTimesList) {
    string TTOTFinal = TTOTInitial;
    bool correctTTOT = true;
    bool alreadySetTOStd = false;

    string initialTSATCheck = calculateLessTime(TTOTInitial, taxiTime);
    if (initialTSATCheck.substr(0, 2) == "00") initialTSATCheck = "24" + initialTSATCheck.substr(2, 4);
    bool initialTSATInPast = (stoi(initialTSATCheck) < stoi(timeNow));

    const int windowMinutes = 10;
    const int windowsPerHour = 60 / windowMinutes;

    // Instead of ceil -> distribute remainder across windows
    const int baseCap = rateHour / windowsPerHour;    // e.g. 40/6 = 6
    const int remainder = rateHour % windowsPerHour;  // e.g. 40%6 = 4  -> 4 windows get +1

    auto getWindowStartFromTTOT = [&](const string& ttot) -> string {
        int v = stoi(ttot);
        int hh = v / 10000;
        int mm = (v / 100) % 100;

        int mmStart = (mm / windowMinutes) * windowMinutes;
        int startVal = (hh * 10000) + (mmStart * 100);

        string s = to_string(startVal);
        while ((int)s.size() < 6) s = "0" + s;
        return s;
    };

    auto capForWindowStart = [&](const string& windowStartTTOT) -> int {
        int v = stoi(windowStartTTOT);
        int hhStart = v / 10000;
        int mmStart = (v / 100) % 100;
        int windowIndex = mmStart / windowMinutes;
        
        // Check for custom capacity override first
        auto customCap = getCustomBlockCapacity(depRwy, hhStart, windowIndex);
        if (customCap >= 0) {  // -1 means not set, any value >= 0 is valid (including 0)
            return customCap;
        }
        
        // Fall back to calculated capacity with evenly-spaced remainder distribution
        // e.g. rate=40: base=6, remainder=4, spacing=1.5 -> +1 at blocks 0,1,3,4
        if (remainder > 0) {
            double spacing = static_cast<double>(windowsPerHour) / remainder;
            for (int i = 0; i < remainder; i++) {
                if (windowIndex == static_cast<int>(i * spacing)) {
                    return baseCap + 1;
                }
            }
        }
        return baseCap;
    };

    auto inSameWindow = [&](const string& a, const string& b) -> bool {
        return getWindowStartFromTTOT(a) == getWindowStartFromTTOT(b);
    };

    auto bumpToNextWindowStart = [&](const string& ttot) -> string {
        string winStart = getWindowStartFromTTOT(ttot);
        string next = calculateTime(winStart, 10.0);
        return getWindowStartFromTTOT(next);
    };

    auto countInWindow = [&](const string& windowTTOT) -> int {
        int count = 0;
        for (int t = 0; t < (int)planes.size(); t++) {
            if (planes[t].callsign == callsign) continue;
            // COUNT ALL AIRCRAFT IN WINDOW, NOT JUST SAME MODE
            // The window capacity is shared between manual and non-manual flights

            string listCallsign = planes[t].callsign;
            string listDepRwy = "";

            CFlightPlan listFlightPlan = FlightPlanSelect(listCallsign.c_str());
            if (!listFlightPlan.IsValid()) continue;

            string listSid = listFlightPlan.GetFlightPlanData().GetSidName();

            bool depRwyFound = false;
            for (size_t i = 0; i < taxiTimesList.size(); i++) {
                if (listCallsign == taxiTimesList[i].substr(0, taxiTimesList[i].find(","))) {
                    if (taxiTimesList[i].substr(taxiTimesList[i].find(",") + 3, 1) == ",") {
                        listDepRwy = taxiTimesList[i].substr(taxiTimesList[i].find(",") + 1, 2);
                        depRwyFound = true;
                    } else if (taxiTimesList[i].substr(taxiTimesList[i].find(",") + 4, 1) == ",") {
                        listDepRwy = taxiTimesList[i].substr(taxiTimesList[i].find(",") + 1, 3);
                        depRwyFound = true;
                    }
                }
            }

            string listAirport;
            for (size_t i = 0; i < planeAiportList.size(); i++) {
                if (listCallsign == planeAiportList[i].substr(0, planeAiportList[i].find(","))) {
                    listAirport = planeAiportList[i].substr(planeAiportList[i].find(",") + 1, 4);
                }
            }

            if (!depRwyFound) listDepRwy = depRwy;

            bool sameOrDependantRwys = (depRwy == listDepRwy);
            
            // Check dependent runways
            if (origin != "" && depRwy != "") {
                Rate dataRate = rateForRunway(origin, depRwy, mySid);
                if (dataRate.airport != "-1") {
                    for (string testRwy : dataRate.dependentRwy) {
                        if (testRwy == listDepRwy || patternMatches(testRwy, listSid)) {
                            sameOrDependantRwys = true;
                            break;
                        }
                    }
                }
            }

            if (!(listAirport == origin)) continue;
            if (!sameOrDependantRwys) continue;

            if (inSameWindow(windowTTOT, planes[t].ttot)) {
                count++;
            }
        }
        return count;
    };

    bool found = false;
    while (!found) {
        found = true;

        string currentWindowStart = getWindowStartFromTTOT(TTOTFinal);

        int used = countInWindow(currentWindowStart);
        int capThisWindow = capForWindowStart(currentWindowStart);

        if (used >= capThisWindow) {
            found = false;

            TTOTFinal = bumpToNextWindowStart(TTOTFinal);

            correctTTOT = false;
            alreadySetTOStd = true;
        }

        if (found && sidIntervalEnabled) {
            for (int t = 0; t < (int)planes.size(); t++) {
                if (planes[t].callsign == callsign) continue;
                string listAirport;
                for (size_t i = 0; i < planeAiportList.size(); i++) {
                    if (planes[t].callsign == planeAiportList[i].substr(0, planeAiportList[i].find(","))) {
                        listAirport = planeAiportList[i].substr(planeAiportList[i].find(",") + 1, 4);
                    }
                }
                if (listAirport != origin) continue;
                string listCallsign = planes[t].callsign;
                string listTTOT = planes[t].ttot;
                CFlightPlan listFlightPlan = FlightPlanSelect(listCallsign.c_str());
                if (!listFlightPlan.IsValid()) continue;
                string listSid = listFlightPlan.GetFlightPlanData().GetSidName();
                string listDepRwy = listFlightPlan.GetFlightPlanData().GetDepartureRwy();
                double interval = getSidInterval(mySid, listSid, origin, depRwy, listDepRwy);
                if (interval <= 0) continue;
                int ttotFinalInt = stoi(TTOTFinal);
                int listTTOTInt = stoi(listTTOT);
                int requiredTTOTAfter = stoi(calculateTime(listTTOT, interval));
                int minAllowedTTOT = stoi(calculateLessTime(listTTOT, interval));
                // Check if in conflict zone: between (listTTOT - interval) and (listTTOT + interval)
                if (ttotFinalInt > minAllowedTTOT && ttotFinalInt < requiredTTOTAfter) {
                    found = false;
                    TTOTFinal = calculateTime(listTTOT, interval);
                    correctTTOT = false;
                    alreadySetTOStd = true;
                    break;
                }
            }
        }
    }

    while ((int)TTOTFinal.size() < 6) TTOTFinal = "0" + TTOTFinal;
    if ((int)TTOTFinal.size() > 6) TTOTFinal = TTOTFinal.substr(0, 6);

    return TTOTFinal;
}
