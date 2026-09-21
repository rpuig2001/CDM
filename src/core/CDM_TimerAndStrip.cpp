// OnTimer blink counter, flight-strip annotation get/set, and the periodic background
// refresh actions 1-4. Moved verbatim out of CDMSingle.cpp.
// See src/core/CDMGlobals.hpp for the shared-state rationale.

#include "CDMSingle.hpp"
#include "src/core/CDMGlobals.hpp"

void CDM::OnTimer(int Counter) {
    FuncBuffer = 0;

    blink = !blink;

    if (blink) {
        if (disCount < 3) {
            disCount++;
        } else {
            disCount = 0;
        }
    }
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
    if (position >= 0 && position <= 9 && FlightPlan.IsValid()) {
        //   0    1    2    3   4     5       6       7       8     9
        // ASRT/TSAC/TOBT/TSAT/TTOT/deIce/ecfmpId/manualCtot/CTOC/setBy/
        string annotation = FlightPlan.GetControllerAssignedData().GetFlightStripAnnotation(0);
        vector<string> values = split(annotation, '/');

        if (position < (int)values.size()) {
            return values[position];
        }
    }
    return "";
}

void CDM::setFlightStripInfo(CFlightPlan FlightPlan, string text, int position) {
    if (position >= 0 && position <= 9 && FlightPlan.IsValid()) {
        //   0    1    2    3   4     5       6       7       8     9
        // ASRT/TSAC/TOBT/TSAT/TTOT/deIce/ecfmpId/manualCtot/CTOC/setBy/
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

void CDM::refreshActions2() {
    addLogLine("[AUTO] - REFRESH ECFMP");
    refresh2 = false;
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
