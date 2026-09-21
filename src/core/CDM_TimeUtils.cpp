// Pure time/string calculation helpers for CDM, split out of CDMSingle.cpp.
// Moved verbatim - no logic changes. See src/core/CDMGlobals.hpp for the shared-state rationale.

#include "CDMSingle.hpp"
#include "src/core/CDMGlobals.hpp"

int CDM::ToMinutes(const std::string& hhmm) {
    int h = std::stoi(hhmm.substr(0, 2));
    int m = std::stoi(hhmm.substr(2, 2));

    return h * 60 + m;
}

string CDM::GetActualTime() {
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
    return hour + min;
}

string CDM::GetDateMonthNow() {
    // Get Time now
    time_t rawtime;
    struct tm ptm;
    time(&rawtime);
    gmtime_s(&ptm, &rawtime);
    string day = to_string(ptm.tm_mday);
    string month = to_string(ptm.tm_mon + 1);
    return day + "-" + month;
}

string CDM::EobtPlusTime(string EOBT, int addedTime) {
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

    return calculateTime(hour + min + "00", addedTime);
}

string CDM::formatTime(string timeString) {
    try {
        if (timeString.length() <= 0) {
            timeString = "0000" + timeString;
            return timeString;
        } else if (timeString.length() <= 1) {
            timeString = "000" + timeString;
            return timeString;
        } else if (timeString.length() <= 2) {
            timeString = "00" + timeString;
            return timeString;
        } else if (timeString.length() <= 3) {
            timeString = "0" + timeString;
            return timeString;
        } else if (timeString.length() == 4) {
            return timeString;
        } else if (timeString.length() >= 5) {
            timeString = timeString.substr(0, 4);
            return timeString;
        } else {
            return timeString;
        }
    } catch (const std::exception& e) {
        addLogLine("ERROR: Unhandled exception formatTime: " + (string)e.what());
        return timeString;
    } catch (...) {
        addLogLine("ERROR: Unhandled exception formatTime");
        return timeString;
    }
}

string CDM::calculateTime(string timeString, double minsToAdd) {
    try {
        if (timeString.length() < 4 || timeString.length() == 5) {
            timeString = "000000";
        } else if (timeString.length() == 4) {
            timeString = timeString + "00";
        }
        int hours = stoi(timeString.substr(0, 2));
        int mins = stoi(timeString.substr(2, 2));
        int sec = stoi(timeString.substr(4, 2));

        int movTime = minsToAdd * 60;
        while (movTime > 0) {
            sec += 1;
            if (sec > 59) {
                sec = 0;
                mins += 1;
                if (mins > 59) {
                    mins = 0;
                    hours += 1;
                    if (hours > 23) {
                        hours = 0;
                    }
                }
            }
            movTime -= 1;
        };

        // calculate hours
        string hourFinal;
        if (hours < 10) {
            hourFinal = "0" + to_string(hours);
        } else {
            hourFinal = to_string(hours);
        }
        // calculate mins
        string minsFinal;
        if (mins < 10) {
            minsFinal = "0" + to_string(mins);
        } else {
            minsFinal = to_string(mins);
        }
        // calculate sec
        string secFinal;
        if (sec < 10) {
            secFinal = "0" + to_string(sec);
        } else {
            secFinal = to_string(sec);
        }
        string timeFinal = hourFinal + minsFinal + secFinal;

        return timeFinal;
    } catch (const std::exception& e) {
        addLogLine("ERROR: Unhandled exception calculateTime: " + (string)e.what());
        return timeString;
    } catch (...) {
        addLogLine("ERROR: Unhandled exception calculateTime");
        return timeString;
    }
}

string CDM::calculateLessTime(string timeString, double minsToAdd) {
    try {
        if (timeString.length() < 4 || timeString.length() == 5) {
            timeString = "000000";
        } else if (timeString.length() == 4) {
            timeString = timeString + "00";
        }
        int hours = stoi(timeString.substr(0, 2));
        int mins = stoi(timeString.substr(2, 2));
        int sec = stoi(timeString.substr(4, 2));

        int movTime = minsToAdd * 60;
        while (movTime > 0) {
            sec -= 1;
            if (sec < 0) {
                sec = 59;
                mins -= 1;
                if (mins < 0) {
                    mins = 59;
                    hours -= 1;
                    if (hours < 0) {
                        hours = 23;
                    }
                }
            }
            movTime -= 1;
        };

        // calculate hours
        string hourFinal;
        if (hours < 10) {
            hourFinal = "0" + to_string(hours);
        } else {
            hourFinal = to_string(hours);
        }
        // calculate mins
        string minsFinal;
        if (mins < 10) {
            minsFinal = "0" + to_string(mins);
        } else {
            minsFinal = to_string(mins);
        }
        // calculate sec
        string secFinal;
        if (sec < 10) {
            secFinal = "0" + to_string(sec);
        } else {
            secFinal = to_string(sec);
        }
        string timeFinal = hourFinal + minsFinal + secFinal;

        return timeFinal;
    } catch (const std::exception& e) {
        addLogLine("ERROR: Unhandled exception calculateLessTime: " + (string)e.what());
        return timeString;
    } catch (...) {
        addLogLine("ERROR: Unhandled exception calculateLessTime");
        return timeString;
    }
}

int CDM::GetdifferenceTime(string hour1, string min1, string hour2, string min2) {
    string stringHour1 = hour1;
    string stringMin1 = min1;
    string stringTime1 = stringHour1 + stringMin1;

    string stringHour2 = hour2;
    string stringMin2 = min2;
    string stringTime2 = stringHour2 + stringMin2;

    int time1 = stoi(stringTime1);
    int time2 = stoi(stringTime2);

    return time1 - time2;
}

int CDM::GetDifferenceTimeHHMMSS(const std::string& time1, const std::string& time2, bool negativeValues) {
    if (time1.length() != 6 || time2.length() != 6) {
        return 0;
    }

    int h1 = std::stoi(time1.substr(0, 2));
    int m1 = std::stoi(time1.substr(2, 2));
    int s1 = std::stoi(time1.substr(4, 2));

    int h2 = std::stoi(time2.substr(0, 2));
    int m2 = std::stoi(time2.substr(2, 2));
    int s2 = std::stoi(time2.substr(4, 2));

    int totalSec1 = h1 * 3600 + m1 * 60 + s1;
    int totalSec2 = h2 * 3600 + m2 * 60 + s2;

    if (negativeValues) return (totalSec1 - totalSec2) / 60;
    return std::abs(totalSec1 - totalSec2) / 60;
}

string CDM::GetTimeNow() {
    time_t rawtime;
    struct tm ptm;
    time(&rawtime);
    gmtime_s(&ptm, &rawtime);
    string hour = to_string(ptm.tm_hour % 24);
    if (stoi(hour) < 10) {
        hour = "0" + hour;
    }
    string min = to_string(ptm.tm_min);
    if (stoi(min) < 10) {
        min = "0" + min;
    }
    string sec = to_string(ptm.tm_sec);
    if (stoi(sec) < 10) {
        sec = "0" + sec;
    }

    return hour + min + sec;
}

string CDM::getDiffTOBTTSAT(string TSAT, string TOBT) {
    try {
        if (TSAT.length() < 4 || TOBT.length() < 4) {
            return "";
        }
        if (TSAT.substr(0, 4) == TOBT.substr(0, 4)) {
            return "";
        }

        int tsat_hours = stoi(TSAT.substr(0, 2));
        int tsat_minutes = stoi(TSAT.substr(2, 2));
        int tobt_hours = stoi(TOBT.substr(0, 2));
        int tobt_minutes = stoi(TOBT.substr(2, 2));

        int tsat_total_minutes = tsat_hours * 60 + tsat_minutes;
        int tobt_total_minutes = tobt_hours * 60 + tobt_minutes;

        return "/" + to_string(tsat_total_minutes - tobt_total_minutes);
    } catch (const std::exception& e) {
        addLogLine("ERROR: Unhandled exception getDiffTOBTTSAT: " + (string)e.what());
        return "";
    } catch (...) {
        addLogLine("ERROR: Unhandled exception getDiffTOBTTSAT");
        return "";
    }
}

string CDM::getDiffNowTime(string time, bool equalReturnZero, string timeNow) {
    try {
        if (timeNow == "") {
            timeNow = GetTimeNow();
        }
        if (time == "") {
            return "";
        }
        if (time.length() < 4 || timeNow.length() < 4) {
            return equalReturnZero ? "0" : "";
        }
        if (timeNow.substr(0, 4) == time.substr(0, 4)) {
            return equalReturnZero ? "0" : "";
        }

        int time_hours = stoi(time.substr(0, 2));
        int time_minutes = stoi(time.substr(2, 2));
        int timeNow_hours = stoi(timeNow.substr(0, 2));
        int timeNow_minutes = stoi(timeNow.substr(2, 2));

        int time_total_minutes = time_hours * 60 + time_minutes;
        int timeNow_total_minutes = timeNow_hours * 60 + timeNow_minutes;

        if (time > timeNow) {
            return to_string(timeNow_total_minutes - time_total_minutes);
        }
        return "+" + to_string(timeNow_total_minutes - time_total_minutes);
    } catch (const std::exception& e) {
        addLogLine("ERROR: Unhandled exception getDiffNowTTOT: " + (string)e.what());
        return "0";
    } catch (...) {
        addLogLine("ERROR: Unhandled exception getDiffNowTTOT");
        return "0";
    }
}

string CDM::GetTimedStatus(string status) {
    size_t dashPos = status.find('-');
    if (dashPos == string::npos) return status;

    // Use current time only (no static state)
    auto now = std::chrono::steady_clock::now();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

    // Toggle every 2 seconds
    bool firstPart = ((seconds / 2) % 2) == 0;

    return firstPart ? status.substr(0, dashPos) : status.substr(dashPos + 1);
}
