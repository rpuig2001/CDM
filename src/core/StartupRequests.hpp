#pragma once

#include <string>
#include <unordered_map>

#include <json/json.h>

enum class StartupRequestState { None, Requested, Cleared };

inline StartupRequestState startupRequestState(const std::string& request) {
    if (request.find("CLR") != std::string::npos) return StartupRequestState::Cleared;
    if (request == "STUP") return StartupRequestState::Requested;
    return StartupRequestState::None;
}

inline std::string startupClearedFrequency(const std::string& request) {
    const auto marker = request.find("CLR-");
    if (marker == std::string::npos || request.size() < marker + 11) return "";
    const std::string frequency = request.substr(marker + 4, 7);
    for (size_t index = 0; index < frequency.size(); ++index) {
        if (index == 3 ? frequency[index] != '.' : frequency[index] < '0' || frequency[index] > '9') return "";
    }
    return frequency;
}

inline bool applyConfirmedStartupClearance(std::unordered_map<std::string, std::string>& requests,
                                           const std::string& callsign, const std::string& dpi) {
    const std::string prefix = "REQUEST/CLR-";
    if (dpi.compare(0, prefix.size(), prefix) != 0 || startupClearedFrequency(dpi).empty()) return false;
    const auto request = requests.find(callsign);
    if (request == requests.end() || startupRequestState(request->second) == StartupRequestState::None) return false;
    request->second = dpi.substr(8);
    return true;
}

inline std::unordered_map<std::string, std::string> startupRequestsFromFlights(const Json::Value& flights) {
    std::unordered_map<std::string, std::string> requests;
    if (!flights.isArray()) return requests;
    for (const auto& flight : flights) {
        if (!flight.isObject() || !flight["callsign"].isString() || !flight["cdmData"].isObject()) continue;
        const auto& atot = flight["atot"];
        if (!atot.isNull() && (!atot.isString() || !atot.asString().empty())) continue;
        const auto& request = flight["cdmData"]["request"];
        if (request.isString() && startupRequestState(request.asString()) != StartupRequestState::None) {
            requests[flight["callsign"].asString()] = request.asString();
        }
    }
    return requests;
}