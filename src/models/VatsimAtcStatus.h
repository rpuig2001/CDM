#pragma once

#include <chrono>
#include <mutex>
#include <string>
#include <unordered_set>
#include <utility>

#include "json/json.h"
#include "src/api/IRestClient.h"

class VatsimAtcStatus {
   public:
    using Clock = std::chrono::steady_clock;

    void refresh(interfaces::IRestClient& client, Clock::time_point now = Clock::now()) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (attempted_ && now - lastAttempt_ < std::chrono::minutes(5)) return;
            attempted_ = true;
            lastAttempt_ = now;
        }

        const auto response = client.get("https://data.vatsim.net/v3/vatsim-data.json");
        Json::Value data;
        Json::Reader reader;
        if (response.statusCode != 200 || !reader.parse(response.body, data) || !data.isObject() ||
            !data["controllers"].isArray() || !data["atis"].isArray()) return;

        std::unordered_set<std::string> airports;
        for (const char* collection : {"controllers", "atis"}) {
            for (const auto& position : data[collection]) {
                if (!position.isObject() || !position["callsign"].isString()) return;
                const auto callsign = position["callsign"].asString();
                if (callsign.size() >= 4) airports.insert(callsign.substr(0, 4));
            }
        }

        std::lock_guard<std::mutex> lock(mutex_);
        airports_ = std::move(airports);
        lastSuccess_ = now;
        available_ = true;
    }

    bool isConfirmedOffline(const std::string& airport, Clock::time_point now = Clock::now()) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return airport.size() == 4 && available_ && now - lastSuccess_ < std::chrono::minutes(5) &&
               airports_.find(airport) == airports_.end();
    }

   private:
    mutable std::mutex mutex_;
    std::unordered_set<std::string> airports_;
    Clock::time_point lastAttempt_{};
    Clock::time_point lastSuccess_{};
    bool attempted_ = false;
    bool available_ = false;
};