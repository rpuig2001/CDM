// Taxi-time / de-ice geometry / slot-list ordering helpers, moved verbatim out of CDMSingle.cpp.
// See src/core/CDMGlobals.hpp for the shared-state rationale.

#include "CDMSingle.hpp"
#include "src/core/CDMGlobals.hpp"

#include <unordered_set>

void CDM::PushToOtherControllers(CFlightPlan fp) {
    string callsign = "";
    for (CController c = ControllerSelectFirst(); c.IsValid(); c = ControllerSelectNext(c)) {
        if (c.IsController()) {
            callsign = c.GetCallsign();
            if (callsign.size() > 3) {
                if (callsign.find("_DEL") != string::npos || callsign.find("_GND") != string::npos ||
                    callsign.find("_TWR") != string::npos || callsign.find("_APP") != string::npos ||
                    callsign.find("_CTR") != string::npos || callsign.find("_FMP") != string::npos) {
                    fp.PushFlightStrip(c.GetCallsign());
                }
            }
        } else if (callsign.find("OBS") != string::npos) {
            fp.PushFlightStrip(c.GetCallsign());
        }
    }
}

void CDM::deleteFlightStrips(string callsign) {
    addLogLine("Called deleteFlightStrips...");
    try {
        CFlightPlan fp = FlightPlanSelect(callsign.c_str());
        if (fp.IsValid()) {
            setFlightStripInfo(fp, "", 0);
            setFlightStripInfo(fp, "", 1);
            setFlightStripInfo(fp, "", 2);
            setFlightStripInfo(fp, "", 3);
            setFlightStripInfo(fp, "", 4);
        }
    } catch (const std::exception& e) {
        addLogLine("ERROR: Unhandled exception deleteFlightStrips: " + (string)e.what());
    } catch (...) {
        addLogLine("ERROR: Unhandled exception deleteFlightStrips");
    }
}

vector<string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delimiter)) {
        tokens.push_back(item);
    }
    return tokens;
}

string CDM::getTaxiTime(double lat, double lon, string origin, string depRwy, int deIceTime, string callsign) {
    string line, TxtOrigin, TxtDepRwy, TxtTime;
    CPosition p1, p2, p3, p4;
    smatch match;

    // Compiled once (function-static) instead of re-compiled on every line of every call - regex
    // compilation is far more expensive than matching and this function runs per-aircraft.
    static const regex pattern1(
        "([A-Z]{4}):(\\d{2}[LRC]?):([^:]+):([^:]+):([^:]+):([^:]+):([^:]+):([^:]+):([^:]+):([^:]+):(\\d+)$",
        regex::icase);
    static const regex pattern2(
        "([A-Z]{4}):(\\d{2}[LRC]?):([^:]+):([^:]+):([^:]+):([^:]+):([^:]+):([^:]+):([^:]+):([^:]+):(\\d+):([^:]+)$",
        regex::icase);
    static const regex pattern3(
        "([A-Z]{4}):(\\d{2}[LRC]?):([^:]+):([^:]+):([^:]+):([^:]+):([^:]+):([^:]+):([^:]+):([^:]+):(\\d+):([^:]+):(\\d+)"
        "$",
        regex::icase);

    try {
        for (size_t t = 0; t < TxtTimesVector.size(); t++) {
            line = TxtTimesVector[t];

            // Cheap pre-filter on airport/runway before attempting the much more expensive regex match.
            size_t firstColon = line.find(':');
            if (firstColon == string::npos || line.compare(0, firstColon, origin) != 0) continue;
            size_t secondColon = line.find(':', firstColon + 1);
            if (secondColon == string::npos ||
                line.compare(firstColon + 1, secondColon - firstColon - 1, depRwy) != 0)
                continue;

            if (regex_match(line, match, pattern1) || regex_match(line, match, pattern2) ||
                regex_match(line, match, pattern3)) {
                if (origin != match[1]) continue;

                if (depRwy != match[2]) continue;

                p1 = readPosition(match[3], match[4]);
                p2 = readPosition(match[5], match[6]);
                p3 = readPosition(match[7], match[8]);
                p4 = readPosition(match[9], match[10]);

                double LatArea[] = {p1.m_Latitude, p2.m_Latitude, p3.m_Latitude, p4.m_Latitude};
                double LonArea[] = {p1.m_Longitude, p2.m_Longitude, p3.m_Longitude, p4.m_Longitude};

                int remId = getDeIceId(callsign);

                vector<string> times;
                if (match.size() > 12 && match[12].matched && match[12].length() > 0) {
                    times = splitString(match[12], ',');
                }

                int eventAdd = 0;
                if (eventMode) {
                    if (match.size() > 13 && match[13].matched && match[13].length() > 0) {
                        if (isNumber(match[13])) {
                            eventAdd = stoi(match[13]);
                        } else {
                            addLogLine("ERROR: Non-numeric EVENT_TAXI in line: " + line);
                            eventAdd = eventModeTime;
                        }
                    } else {
                        eventAdd = eventModeTime;
                    }
                }

                if (inPoly(4, LatArea, LonArea, lat, lon) % 2 != 0) {
                    if (remId > 0 && times.size() >= (size_t)remId) {
                        if (isNumber(times[remId - 1])) {
                            return to_string(deIceTime + stoi(times[remId - 1]) + eventAdd);
                        } else {
                            addLogLine("ERROR: Non-numeric REM time in line: " + line);
                        }
                    }
                    return to_string((isNumber(match[11]) ? stoi(match[11]) : defTaxiTime) + deIceTime + eventAdd);
                }
            }
        }
    } catch (std::runtime_error const& e) {
        DisplayUserMessage(MY_PLUGIN_NAME, "Error", e.what(), true, true, false, true, false);
        DisplayUserMessage(MY_PLUGIN_NAME, "Error", line.c_str(), true, true, false, true, false);
        addLogLine("ERROR: Unhandled exception getTaxiTime: " + (string)e.what());
    } catch (...) {
        DisplayUserMessage(MY_PLUGIN_NAME, "Error", std::to_string(GetLastError()).c_str(), true, true, false, true,
                           false);
        DisplayUserMessage(MY_PLUGIN_NAME, "Error", line.c_str(), true, true, false, true, false);
        addLogLine("ERROR: Unhandled exception getTaxiTime");
    }

    return to_string(defTaxiTime);
}

int CDM::inPoly(int nvert, double* vertx, double* verty, double testx, double testy) {
    int i, j, c = 0;
    for (i = 0, j = nvert - 1; i < nvert; j = i++) {
        if (((verty[i] > testy) != (verty[j] > testy)) &&
            (testx < (vertx[j] - vertx[i]) * (testy - verty[i]) / (verty[j] - verty[i]) + vertx[i]))
            c = !c;
    }
    return c;
}

void CDM::toggleReaMsg(CFlightPlan fp, bool deleteIfExist) {
    bool inreaList = false;
    int i = 0;
    for (string s : reaCTOTSent) {
        if (s == fp.GetCallsign()) {
            inreaList = true;
            if (deleteIfExist) {
                reaCTOTSent.erase(reaCTOTSent.begin() + i);
            }
        }
        i++;
    }
    if (!inreaList) {
        reaCTOTSent.push_back(fp.GetCallsign());
    }

    // Update times to slaves
    countTime = std::time(nullptr) - (refreshTime);
}

void CDM::addTimeToList(int timeToAdd, string minTSAT) {
    try {
        vector<Plane> mySlotList = slotList;

        for (size_t i = 0; i < mySlotList.size(); i++) {
            if (!mySlotList[i].hasManualCtot) {
                CFlightPlan myFp = FlightPlanSelect(mySlotList[i].callsign.c_str());
                if (!myFp.IsValid()) {
                    continue;
                }
                if ((string)myFp.GetGroundState() != "STUP" && (string)myFp.GetGroundState() != "ST-UP" &&
                    (string)myFp.GetGroundState() != "PUSH" && (string)myFp.GetGroundState() != "TAXI" &&
                    (string)myFp.GetGroundState() != "DEPA") {
                    int difTime = GetdifferenceTime(mySlotList[i].tsat.substr(0, 2), mySlotList[i].tsat.substr(2, 2),
                                                    minTSAT.substr(0, 2), minTSAT.substr(2, 2));
                    bool ok = false;
                    if (minTSAT.substr(0, 2) == mySlotList[i].tsat.substr(0, 2)) {
                        if (difTime >= 0) {
                            ok = true;
                        }
                    } else {
                        if (difTime >= 40) {
                            ok = true;
                        }
                    }
                    if (ok) {
                        mySlotList[i].tsat = calculateTime(mySlotList[i].tsat, timeToAdd);
                        mySlotList[i].ttot = calculateTime(mySlotList[i].ttot, timeToAdd);
                    }
                }
            }
        }

        for (Plane p : mySlotList) {
            for (int d = 0; d < slotList.size(); d++) {
                if (p.callsign == slotList[d].callsign) {
                    slotList[d] = p;
                }
            }
        }
    } catch (const std::exception& e) {
        addLogLine("ERROR: Unhandled exception addTimeToList: " + (string)e.what());
    } catch (...) {
        addLogLine("ERROR: Unhandled exception addTimeToList");
    }
}

void CDM::addTimeToListForSpecificAirportAndRunway(int timeToAdd, string minTSAT, string airport, string runway) {
    try {
        vector<Plane> mySlotList = slotList;

        for (size_t i = 0; i < mySlotList.size(); i++) {
            if (!mySlotList[i].hasManualCtot) {
                CFlightPlan myFp = FlightPlanSelect(mySlotList[i].callsign.c_str());
                if (!myFp.IsValid()) {
                    continue;
                }
                if (myFp.GetFlightPlanData().GetDepartureRwy() == runway &&
                    myFp.GetFlightPlanData().GetOrigin() == airport) {
                    if ((string)myFp.GetGroundState() != "STUP" && (string)myFp.GetGroundState() != "ST-UP" &&
                        (string)myFp.GetGroundState() != "PUSH" && (string)myFp.GetGroundState() != "TAXI" &&
                        (string)myFp.GetGroundState() != "DEPA") {
                        int difTime =
                            GetdifferenceTime(mySlotList[i].tsat.substr(0, 2), mySlotList[i].tsat.substr(2, 2),
                                              minTSAT.substr(0, 2), minTSAT.substr(2, 2));
                        bool ok = false;
                        if (minTSAT.substr(0, 2) == mySlotList[i].tsat.substr(0, 2)) {
                            if (difTime >= 0) {
                                ok = true;
                            }
                        } else {
                            if (difTime >= 40) {
                                ok = true;
                            }
                        }
                        if (ok) {
                            mySlotList[i].tsat = calculateTime(mySlotList[i].tsat, timeToAdd);
                            mySlotList[i].ttot = calculateTime(mySlotList[i].ttot, timeToAdd);
                        }
                    }
                }
            }
        }

        for (Plane p : mySlotList) {
            for (int d = 0; d < slotList.size(); d++) {
                if (p.callsign == slotList[d].callsign) {
                    slotList[d] = p;
                }
            }
        }
    } catch (const std::exception& e) {
        addLogLine("ERROR: Unhandled exception addTimeToListForSpecificAirportAndRunway: " + (string)e.what());
    } catch (...) {
        addLogLine("ERROR: Unhandled exception addTimeToListForSpecificAirportAndRunway");
    }
}

std::vector<Plane> CDM::recalculateSlotList(std::vector<Plane> mySlotList) {
    addLogLine("Called recalculateSlotList...");

    std::unordered_set<std::string> eventCtotCallsigns;
    eventCtotCallsigns.reserve(evCtots.size());
    for (const auto& row : evCtots) {
        if (row.size() >= 2 && !row[1].empty()) {
            eventCtotCallsigns.insert(row[0]);
        }
    }

    std::unordered_set<std::string> reqTobtCallsigns;
    for (const auto& callsign : reqTobtList) {
        reqTobtCallsigns.insert(callsign);
    }

    std::unordered_set<std::string> depaCallsigns;
    if (atotEnabled) {
        for (const auto& entry : asatList) {
            //Only add if GND state is DEPA
            string callsign = entry.substr(0, entry.find(","));
            CFlightPlan fp = FlightPlanSelect(callsign.c_str());
            if ((string)fp.GetGroundState() == "DEPA") {
                depaCallsigns.insert(callsign);
            }
        }
    }

    try {
        std::sort(mySlotList.begin(), mySlotList.end(), [&eventCtotCallsigns, &depaCallsigns, &reqTobtCallsigns](const Plane& a, const Plane& b) {
            // 0. DEPA set before no DEPA
            if (atotEnabled) {
                const bool aHasDepaSet = depaCallsigns.find(a.callsign) != depaCallsigns.end();
                const bool bHasDepaSet = depaCallsigns.find(b.callsign) != depaCallsigns.end();
                if (aHasDepaSet != bHasDepaSet) return aHasDepaSet > bHasDepaSet;
            }

            // 1. Manual CTOT first
            if (a.hasManualCtot != b.hasManualCtot) return a.hasManualCtot > b.hasManualCtot;

            // 2. Both manual CTOT AND both CTOTs present → order by CTOT
            if (a.hasManualCtot && b.hasManualCtot && !a.ctot.empty() && !b.ctot.empty()) {
                return std::stoi(a.ctot) < std::stoi(b.ctot);
            }

            // 3) Event SLOT present (found in evCtots with non-empty value) before those without SLOT
            if (eventPriorityEnabled) {
                const bool aHasEventCtot = eventCtotCallsigns.find(a.callsign) != eventCtotCallsigns.end();
                const bool bHasEventCtot = eventCtotCallsigns.find(b.callsign) != eventCtotCallsigns.end();
                if (aHasEventCtot != bHasEventCtot) return aHasEventCtot > bHasEventCtot;
            }

            // 4) Has Req TOBT or ASRT over none ASRT/none reqTOBT
            if (reqTobtPriority && realMode) {
                const bool aHasReqTobt = reqTobtCallsigns.find(a.callsign) != reqTobtCallsigns.end();
                const bool bHasReqTobt = reqTobtCallsigns.find(b.callsign) != reqTobtCallsigns.end();
                if (aHasReqTobt != bHasReqTobt) return aHasReqTobt > bHasReqTobt;
            }

            // 4. Fallback → order by TTOT
            return std::stoi(a.ttot) < std::stoi(b.ttot);
        });
    } catch (const std::exception& e) {
        addLogLine("ERROR: Unhandled exception recalculateSlotList: " + std::string(e.what()));
    } catch (...) {
        addLogLine("ERROR: Unhandled exception recalculateSlotList");
    }

    return cleanUpSlotListVector(mySlotList);
}

vector<Plane> CDM::cleanUpSlotListVector(vector<Plane> mySlotList) {
    vector<Plane> finalSlotList;
    string lastCallsign = "";
    for (Plane p : mySlotList) {
        if (lastCallsign != p.callsign) {
            finalSlotList.push_back(p);
            lastCallsign = p.callsign;
        }
    }

    return finalSlotList;
}

bool CDM::patternMatches(const string& pattern, const string& str) {
    // Returns true if pattern matches str, where % or * in pattern match any single character
    // Handles both exact length and prefix matching
    
    if (str.empty()) return false;  // SID is empty, can't match
    
    // Check for exact length match with wildcards
    if (pattern.length() == str.length()) {
        for (size_t i = 0; i < pattern.length(); i++) {
            if (pattern[i] != '%' && pattern[i] != '*' && pattern[i] != str[i]) {
                return false;
            }
        }
        return true;
    }
    
    return false;
}

bool CDM::checkIsNumber(string str) {
    bool hasNoNumber = true;
    for (size_t i = 0; i < str.length(); i++) {
        if (isdigit(str[i]) == false) {
            hasNoNumber = false;
        }
    }
    if (hasNoNumber) {
        return true;
    } else {
        return false;
    }
}
