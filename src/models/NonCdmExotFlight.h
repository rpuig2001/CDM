#pragma once
#include <iostream>
#include <string>
#include <ctime>

using namespace std;

// Tracks non-CDM airport flights that need EXOT (taxi time) sent via DPI
class NonCdmExotFlight {
   public:
    string callsign;
    string airport;
    string runway;
    int taxiTimeMinutes;
    time_t firstDetectionTime;
    time_t lastAttemptTime;
    int attemptCount;
    bool sentSuccessfully;

    NonCdmExotFlight(string myCallsign, string myAirport, string myRunway, int myTaxiTimeMinutes)
        : callsign(myCallsign),
          airport(myAirport),
          runway(myRunway),
          taxiTimeMinutes(myTaxiTimeMinutes),
          firstDetectionTime(std::time(nullptr)),
          lastAttemptTime(0),
          attemptCount(0),
          sentSuccessfully(false) {}

    NonCdmExotFlight() : runway(""), taxiTimeMinutes(0), firstDetectionTime(0), lastAttemptTime(0), attemptCount(0), sentSuccessfully(false) {};
};
