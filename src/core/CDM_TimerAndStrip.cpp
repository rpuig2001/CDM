// Automatic startup, flight-strip annotation get/set, and the periodic background
// refresh actions 1-4. Moved verbatim out of CDMSingle.cpp.
// See src/core/CDMGlobals.hpp for the shared-state rationale.

#include "CDMSingle.hpp"
#include "src/core/CDMGlobals.hpp"

void CDM::tryAutoSetStup(CFlightPlan fp, Plane flight) {
    if (!fp.IsValid() || !fp.GetClearenceFlag() || getFlightStripInfo(fp, 10) != "1") {
        return;
    }
    if (!flight.isCdmAirport || !flight.showData ||
        (flight.tsat.length() != 4 && flight.tsat.length() != 6) ||
        flight.tsat.find_first_not_of("0123456789") != string::npos) {
        return;
    }

    const int tsatHours = stoi(flight.tsat.substr(0, 2));
    const int tsatMinutes = stoi(flight.tsat.substr(2, 2));
    if (tsatHours > 23 || tsatMinutes > 59) {
        return;
    }
    const int nowMinutes = ToMinutes(GetActualTime());
    const int difference = (nowMinutes - (tsatHours * 60 + tsatMinutes) + 2160) % 1440 - 720;
    if (difference < -5 || difference > 6) {
        return;
    }

    const string origin = fp.GetFlightPlanData().GetOrigin();
    if (std::find(masterAirports.begin(), masterAirports.end(), origin) == masterAirports.end()) {
        return;
    }
    if (string(fp.GetTrackingControllerCallsign()) != "" && !fp.GetTrackingControllerIsMe()) {
        return;
    }
    const string groundState = fp.GetGroundState();
    if (groundState == "STUP" || groundState == "ST-UP" || groundState == "PUSH" ||
        groundState == "TAXI" || groundState == "DEPA") {
        return;
    }

    setFlightStripInfo(fp, "2", 10);
    try {
        if (!fp.GetControllerAssignedData().SetScratchPadString("STUP")) {
            setFlightStripInfo(fp, "1", 10);
            return;
        }
    } catch (...) {
        setFlightStripInfo(fp, "1", 10);
        throw;
    }
    PushToOtherControllers(fp);
}

vector<string> CDM::splitString(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::istringstream stream(str);
    std::string token;

    while (std::getline(stream, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

string CDM::getFlightStripInfo(CFlightPlan FlightPlan, int position) {
    if (position >= 0 && position <= 10 && FlightPlan.IsValid()) {
        //   0    1    2    3   4     5       6       7       8     9       10
        // ASRT/TSAC/TOBT/TSAT/TTOT/deIce/ecfmpId/manualCtot/CTOC/setBy/autoStartup (1=armed, 2=assigned)
        string annotation = FlightPlan.GetControllerAssignedData().GetFlightStripAnnotation(0);
        vector<string> values = split(annotation, '/');

        if (position < (int)values.size()) {
            return values[position];
        }
    }
    return "";
}

void CDM::setFlightStripInfo(CFlightPlan FlightPlan, string text, int position) {
    if (position >= 0 && position <= 10 && FlightPlan.IsValid()) {
        //   0    1    2    3   4     5       6       7       8     9       10
        // ASRT/TSAC/TOBT/TSAT/TTOT/deIce/ecfmpId/manualCtot/CTOC/setBy/autoStartup (1=armed, 2=assigned)
        string annotation = FlightPlan.GetControllerAssignedData().GetFlightStripAnnotation(0);
        if (annotation == "") {
            annotation = "///////////";
        }
        vector<string> values = split(annotation, '/');

        while ((int)values.size() <= position) {
            values.push_back("");
        }

        values[position] = text;

        string finalString = "";
        for (int i = 0; i < (int)values.size(); i++) {
            finalString += values[i];
            if (i < (int)values.size() - 1) {
                finalString += "/";
            }
        }
        FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(0, finalString.c_str());
    }
}

void CDM::refreshActions1() {
    refresh1 = true;
    readyToUpdateList = false;
    saveData();
    // Execute background process in the background
    slotListToUpdate = backgroundProcess_recaulculate();
    // Check rates
    getNetworkRates();
    readyToUpdateList = true;
    refresh1 = false;
}

void CDM::refreshActions3() {
    addLogLine("[AUTO] - REFRESH API 1");
    getCdmServerRestricted(slotList);
    getCdmServerMasterAirports();
    getCdmServerRelevantFlights();
    getCdmServerOnTime();
    refresh3 = false;
}

void CDM::refreshActions4() {
    addLogLine("[AUTO] - REFRESH API 2");
    getCdmServerStatus();
    getIffOffBlockTimes();
    refresh4 = false;
}
