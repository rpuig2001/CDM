// Automatic startup, flight-strip annotation get/set, and the periodic background
// refresh actions 1-4. Moved verbatim out of CDMSingle.cpp.
// See src/core/CDMGlobals.hpp for the shared-state rationale.

#include "CDMSingle.hpp"
#include "src/core/CDMGlobals.hpp"
#include "src/core/StartupRequests.hpp"

namespace {
struct VsidScratchUpdate {
    string airport;
    string request;
    string message;
    string previous;
    std::chrono::steady_clock::time_point started;
    bool acknowledged = false;
};

std::unordered_map<string, VsidScratchUpdate> vsidScratchUpdates;
std::unordered_map<string, string> vsidDispatchedRequests;
}

void CDM::OnFlightPlanControllerAssignedDataUpdate(CFlightPlan flightPlan, int dataType) {
    if (dataType != CTR_DATA_TYPE_SCRATCH_PAD_STRING || !flightPlan.IsValid()) return;
    auto update = vsidScratchUpdates.find(flightPlan.GetCallsign());
    if (update != vsidScratchUpdates.end() &&
        string(flightPlan.GetControllerAssignedData().GetScratchPadString()) == update->second.message) {
        update->second.acknowledged = true;
    }
}

void CDM::processVsidRequests() {
    const auto now = std::chrono::steady_clock::now();
    for (auto update = vsidScratchUpdates.begin(); update != vsidScratchUpdates.end();) {
        CFlightPlan flightPlan = FlightPlanSelect(update->first.c_str());
        const bool expired = now - update->second.started >= std::chrono::seconds(5);
        if (!flightPlan.IsValid()) {
            update = vsidScratchUpdates.erase(update);
            continue;
        }
        const string current = flightPlan.GetControllerAssignedData().GetScratchPadString();
        const auto action = vsidScratchAction(current, update->second.message, update->second.previous,
                                              update->second.acknowledged, expired);
        if (action == VsidScratchAction::Wait) {
            ++update;
            continue;
        }
        const VsidScratchUpdate completed = update->second;
        const string callsign = update->first;
        update = vsidScratchUpdates.erase(update);
        if (completed.acknowledged) vsidDispatchedRequests[completed.airport + "|" + callsign] = completed.request;
        if (action == VsidScratchAction::Restore) {
            flightPlan.GetControllerAssignedData().SetScratchPadString(completed.previous.c_str());
        }
    }

    if (!vsidRequestsEnabled || !serverEnabled || GetConnectionType() == CONNECTION_TYPE_NO ||
        !ControllerMyself().IsController()) {
        vsidDispatchedRequests.clear();
        return;
    }

    static auto lastRequestCheck = std::chrono::steady_clock::time_point{};
    if (now - lastRequestCheck < std::chrono::seconds(1)) return;
    lastRequestCheck = now;

    std::unordered_map<string, std::unordered_map<string, string>> requests;
    {
        std::lock_guard<std::mutex> lock(startupRequestsMutex);
        requests = vsidRequests;
    }
    for (auto sent = vsidDispatchedRequests.begin(); sent != vsidDispatchedRequests.end();) {
        const auto separator = sent->first.find('|');
        const string airport = sent->first.substr(0, separator);
        const string callsign = sent->first.substr(separator + 1);
        const auto airportRequests = requests.find(airport);
        if (find(masterAirports.begin(), masterAirports.end(), airport) == masterAirports.end() ||
            airportRequests == requests.end() || airportRequests->second.find(callsign) == airportRequests->second.end() ||
            airportRequests->second.at(callsign) != sent->second) {
            sent = vsidDispatchedRequests.erase(sent);
        } else {
            ++sent;
        }
    }

    for (const auto& airport : requests) {
        if (find(masterAirports.begin(), masterAirports.end(), airport.first) == masterAirports.end()) continue;
        for (const auto& request : airport.second) {
            const string key = airport.first + "|" + request.first;
            if (vsidDispatchedRequests.find(key) != vsidDispatchedRequests.end() ||
                vsidScratchUpdates.find(request.first) != vsidScratchUpdates.end()) continue;
            CFlightPlan flightPlan = FlightPlanSelect(request.first.c_str());
            if (!flightPlan.IsValid() || !updateFlightRange(flightPlan, CRadarTarget()) ||
                string(flightPlan.GetFlightPlanData().GetOrigin()) != airport.first) continue;
            const string groundState = flightPlan.GetGroundState();
            if (request.second == "STUP" && (groundState == "STUP" || groundState == "ST-UP" ||
                groundState == "PUSH" || groundState == "TAXI" || groundState == "DEPA")) continue;
            const string previous = flightPlan.GetControllerAssignedData().GetScratchPadString();
            if (!previous.empty() && previous[0] == '.') continue;
            const string message = vsidRequestScratchpad(request.second, std::time(nullptr));
            if (message.empty()) continue;
            vsidScratchUpdates[request.first] = {airport.first, request.second, message, previous, now, false};
            if (!flightPlan.GetControllerAssignedData().SetScratchPadString(message.c_str())) {
                vsidScratchUpdates.erase(request.first);
            }
        }
    }
}

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
    if (position >= 0 && position <= 11 && FlightPlan.IsValid()) {
        //   0    1    2    3   4     5       6       7       8     9       10          11
        // ASRT/TSAC/TOBT/TSAT/TTOT/deIce/ecfmpId/manualCtot/CTOC/setBy/autoStartup/ATOT
        string annotation = FlightPlan.GetControllerAssignedData().GetFlightStripAnnotation(0);
        vector<string> values = split(annotation, '/');

        if (position < (int)values.size()) {
            return values[position];
        }
    }
    return "";
}

void CDM::setFlightStripInfo(CFlightPlan FlightPlan, string text, int position) {
    if (position >= 0 && position <= 11 && FlightPlan.IsValid()) {
        //   0    1    2    3   4     5       6       7       8     9       10          11
        // ASRT/TSAC/TOBT/TSAT/TTOT/deIce/ecfmpId/manualCtot/CTOC/setBy/autoStartup/ATOT
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
        if (position == 0) {
            const string callsign = FlightPlan.GetCallsign();
            std::lock_guard<std::mutex> lock(asrtListMutex);
            for (size_t i = 0; i < asrtList.size();) {
                if (asrtList[i][0] == callsign) {
                    asrtList.erase(asrtList.begin() + i);
                } else {
                    i++;
                }
            }
            if (!text.empty()) {
                asrtList.push_back({callsign, text});
            }
        }
    }
}

void CDM::syncAtotFromFlightStrip(CFlightPlan FlightPlan, Plane& plane) {
    if (!atotEnabled) {
        plane.atot.clear();
        return;
    }
    if (!FlightPlan.IsValid()) return;

    const string marker = getFlightStripInfo(FlightPlan, 11);
    const vector<string> values = split(marker, '|');
    const string identity = string(FlightPlan.GetCallsign()) + "~" +
                            FlightPlan.GetFlightPlanData().GetOrigin() + "~" +
                            FlightPlan.GetFlightPlanData().GetDestination() + "~" +
                            FlightPlan.GetFlightPlanData().GetEstimatedDepartureTime();
    if (values.size() == 3 && values[0] == "CDM_ATOT" && values[2] == identity) {
        plane.atot = values[1];
    } else {
        plane.atot.clear();
        if (marker.rfind("CDM_ATOT|", 0) == 0) {
            setFlightStripInfo(FlightPlan, "", 11);
        }
    }
}

void CDM::syncAtotToFlightStrip(CFlightPlan FlightPlan, Plane& plane) {
    if (!atotEnabled) return;
    if (!FlightPlan.IsValid() || plane.atot.empty()) return;

    const string identity = string(FlightPlan.GetCallsign()) + "~" +
                            FlightPlan.GetFlightPlanData().GetOrigin() + "~" +
                            FlightPlan.GetFlightPlanData().GetDestination() + "~" +
                            FlightPlan.GetFlightPlanData().GetEstimatedDepartureTime();
    const string marker = "CDM_ATOT|" + plane.atot + "|" + identity;
    if (getFlightStripInfo(FlightPlan, 11) != marker) {
        setFlightStripInfo(FlightPlan, marker, 11);
        PushToOtherControllers(FlightPlan);
    }
}

void CDM::refreshActions1() {
    refresh1 = true;
    readyToUpdateList = false;
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
