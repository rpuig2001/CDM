// Non-CDM airport EXOT (taxi time) support via DPI messaging.
// Handles detection of flights at non-CDM airports, reading taxi zone definitions,
// and sending EXOT messages via DPI with retry logic.

#include "CDMSingle.hpp"
#include "src/core/CDMGlobals.hpp"

bool CDM::hasNonCdmTaxiZones(string airport) {
    // Check if this airport has taxi zone definitions defined for non-CDM use
    // Returns true only if taxi zones are defined AND airport is NOT a CDM airport
    if (isCdmAirport(airport)) {
        return false;  // CDM airports use the regular CDM flow, not this feature
    }

    // Check if any taxi zones exist for this airport in TxtTimesVector
    for (const string& line : TxtTimesVector) {
        if (line.find(airport) != string::npos) {
            // Found a taxi zone definition for this airport
            size_t firstColon = line.find(':');
            if (firstColon != string::npos) {
                string airportCode = line.substr(0, firstColon);
                if (airportCode == airport) {
                    return true;
                }
            }
        }
    }

    return false;
}

int CDM::getNonCdmTaxiTime(string airport) {
    // Default version - gets any taxi zone for this airport
    // For runway-specific taxi time, use getNonCdmTaxiTime(airport, runway)
    return getNonCdmTaxiTime(airport, "");
}

int CDM::getNonCdmTaxiTime(string airport, string runway) {
    // Extract taxi time from taxi zone definitions for non-CDM airport
    // Extract taxi time from taxi zone definitions for airport
    // Matches flight coordinates to polygon area for that runway
    // Format: AIRPORT:RUNWAY:LAT1:LON1:LAT2:LON2:LAT3:LON3:LAT4:LON4:TAXITIME
    // Returns taxi time if runway entry found, or defTaxiTime if no match

    if (!hasNonCdmTaxiZones(airport)) {
        return defTaxiTime;
    }

    // If no runway specified, use default taxi time
    if (runway.empty()) {
        addLogLine("DEBUG: Non-CDM EXOT: No runway specified for " + airport + ", using defTaxiTime=" + to_string(defTaxiTime));
        return defTaxiTime;
    }

    // Parse the taxi zone file to find taxi times for this airport and runway
    for (const string& line : TxtTimesVector) {
        size_t firstColon = line.find(':');
        if (firstColon == string::npos) continue;

        string airportCode = line.substr(0, firstColon);
        if (airportCode != airport) continue;

        // Check if runway matches
        size_t secondColon = line.find(':', firstColon + 1);
        if (secondColon != string::npos) {
            string lineRunway = line.substr(firstColon + 1, secondColon - firstColon - 1);
            
            // Check for exact match or partial match (e.g., "27" matches "27L")
            if (lineRunway != runway && lineRunway.find(runway) == string::npos) {
                continue;  // Runway doesn't match, skip this line
            }
        }

        // Found matching airport and runway entry
        // Now parse polygon coordinates and taxi time from this line
        // Format: AIRPORT:RUNWAY:LAT1:LON1:LAT2:LON2:LAT3:LON3:LAT4:LON4:TAXITIME
        try {
            vector<string> parts = explode(line, ':');
            // parts[0] = airport, parts[1] = runway, parts[2..9] = lat/lon pairs (4 points), parts[10] = taxitime
            
            if (parts.size() < 11) {
                addLogLine("DEBUG: Non-CDM EXOT: Invalid format for " + airport + " (expected 11 parts, got " + to_string(parts.size()) + ")");
                continue;
            }

            // Extract taxi time (last segment)
            string taxiTimeStr = parts[parts.size() - 1];
            if (!isNumber(taxiTimeStr)) {
                addLogLine("DEBUG: Non-CDM EXOT: Taxi time not a number: " + taxiTimeStr);
                continue;
            }

            int taxiTime = stoi(taxiTimeStr);
            if (taxiTime <= 0 || taxiTime > 120) {
                addLogLine("DEBUG: Non-CDM EXOT: Taxi time out of range: " + to_string(taxiTime));
                continue;
            }

            // Extract polygon points for later polygon matching in processNonCdmExotFlights
            // We store the taxi time here; polygon matching happens at flight level
            addLogLine("DEBUG: Non-CDM EXOT: Found taxi time " + to_string(taxiTime) + 
                       " for " + airport + ":" + runway);
            return taxiTime;

        } catch (...) {
            addLogLine("ERROR: Exception parsing taxi time for airport " + airport + " runway " + runway);
            continue;
        }
    }

    // No matching runway entry found, use default
    addLogLine("DEBUG: Non-CDM EXOT: No taxi time entry for " + airport + ":" + runway + 
               ", using defTaxiTime=" + to_string(defTaxiTime));
    return defTaxiTime;
}

bool CDM::sendNonCdmExotMessage(string callsign, string airport, int taxiTimeMinutes) {
    // Send EXOT message via DPI: callsign/EXOT/MM (where MM is taxi time in minutes)
    // NOTE: This function must be called while holding nonCdmExotFlightsMutex lock!
    // Returns: true if server confirmed success, false otherwise
    // The caller is responsible for locking/unlocking to avoid nested locks and deadlocks.
    if (!serverEnabled) {
        return false;
    }

    try {
        string exotValue = "EXOT/" + to_string(taxiTimeMinutes);
        string url = cdmServerUrl + "/ifps/dpi?callsign=" + callsign + "&value=" + exotValue;

        std::unordered_map<std::string, std::string> headers = {{"x-api-key", apikey}};
        const auto response = restclient_->post(url, /*body=*/"", headers);
        int responseCode = response.statusCode;

        addLogLine("Non-CDM EXOT: Sent " + exotValue + " for " + callsign + " at " + airport);

        if (responseCode == 404 || responseCode == 401 || responseCode == 502 || responseCode == -1) {
            // Connection error - will retry later
            addLogLine("Non-CDM EXOT: Connection error for " + callsign + ". Will retry.");
        } else {
            // Check response body for success
            std::istringstream is(response.body);
            string lineValue;
            bool success = false;
            while (getline(is, lineValue)) {
                if (lineValue == "true") {
                    success = true;
                    break;
                }
            }

            if (success) {
                return true;
            } else {
                addLogLine("Non-CDM EXOT: Server returned false for " + callsign + ". Will retry.");
                return false;
            }
        }
        return false;  // Connection error - will retry
    } catch (const std::exception& e) {
        addLogLine("ERROR: Exception in sendNonCdmExotMessage for " + callsign + ": " + (string)e.what());
        return false;
    } catch (...) {
        addLogLine("ERROR: Unhandled exception in sendNonCdmExotMessage for " + callsign);
        return false;
    }
}

int CDM::getNonCdmTaxiTimeWithPolygonMatch(string airport, string runway, double flightLat, double flightLon) {
    // Get taxi time for airport/runway, checking if flight coordinates match polygon area
    // Returns taxi time if flight is inside polygon area, or defTaxiTime if no match
    
    if (!hasNonCdmTaxiZones(airport) || runway.empty()) {
        return defTaxiTime;
    }

    // Find matching taxi zone entry for this airport and runway, then check polygon
    for (const string& line : TxtTimesVector) {
        size_t firstColon = line.find(':');
        if (firstColon == string::npos) continue;

        string airportCode = line.substr(0, firstColon);
        if (airportCode != airport) continue;

        // Check if runway matches
        size_t secondColon = line.find(':', firstColon + 1);
        if (secondColon != string::npos) {
            string lineRunway = line.substr(firstColon + 1, secondColon - firstColon - 1);
            if (lineRunway != runway && lineRunway.find(runway) == string::npos) {
                continue;
            }
        }

        // Found matching airport and runway
        try {
            vector<string> parts = explode(line, ':');
            if (parts.size() < 11) continue;

            // Extract polygon points: parts[2..9] are lat1, lon1, lat2, lon2, lat3, lon3, lat4, lon4
            double lat1 = stod(parts[2]);
            double lon1 = stod(parts[3]);
            double lat2 = stod(parts[4]);
            double lon2 = stod(parts[5]);
            double lat3 = stod(parts[6]);
            double lon3 = stod(parts[7]);
            double lat4 = stod(parts[8]);
            double lon4 = stod(parts[9]);

            // Check if flight coordinates are inside the polygon
            double vertx[] = {lon1, lon2, lon3, lon4};
            double verty[] = {lat1, lat2, lat3, lat4};
            int nvert = 4;

            if (inPoly(nvert, vertx, verty, flightLon, flightLat) == 1) {
                // Flight is inside polygon - use this taxi time
                string taxiTimeStr = parts[parts.size() - 1];
                if (isNumber(taxiTimeStr)) {
                    int taxiTime = stoi(taxiTimeStr);
                    if (taxiTime > 0 && taxiTime <= 120) {
                        addLogLine("Non-CDM EXOT: Flight inside polygon for " + airport + ":" + runway + 
                                   " using taxi time " + to_string(taxiTime));
                        return taxiTime;
                    }
                }
            } else {
                // Flight is outside polygon for this runway
                addLogLine("DEBUG: Non-CDM EXOT: Flight outside polygon for " + airport + ":" + runway);
            }
        } catch (...) {
            addLogLine("ERROR: Exception in polygon match for " + airport + ":" + runway);
            continue;
        }
    }

    // No matching polygon found, use default taxi time
    addLogLine("DEBUG: Non-CDM EXOT: No polygon match for " + airport + ":" + runway + 
               ", using defTaxiTime=" + to_string(defTaxiTime));
    return defTaxiTime;
}

void CDM::processNonCdmExotFlights() {
    // Process non-CDM flights that need EXOT messages sent
    // Called periodically to:
    // 1. Detect new flights at non-CDM airports with taxi zones
    // 2. Send initial EXOT message on first detection
    // 3. Retry failed sends every N seconds
    // 4. Detect runway changes and re-send EXOT with new taxi time
    
    try {
        time_t now = std::time(nullptr);

        // Skip if not enough time has passed since last check
        if (nonCdmExotLastCheckTime > 0 && (now - nonCdmExotLastCheckTime) < 5) {
            return;
        }
        nonCdmExotLastCheckTime = now;

        // Iterate through known flights (via slotList and planeAiportList) to detect non-CDM airport flights
        // Use planeAiportList which has: callsign,airport pairs
        for (const auto& planeAirportEntry : planeAiportList) {
            // Parse the entry: "CALLSIGN,AIRPORT"
            size_t commaPos = planeAirportEntry.find(",");
            if (commaPos == string::npos || commaPos == 0) {
                continue;
            }

            string callsign = planeAirportEntry.substr(0, commaPos);
            string airport = planeAirportEntry.substr(commaPos + 1);

            // Clean up airport string (might have extra spaces or characters)
            if (airport.length() > 4) {
                airport = airport.substr(0, 4);
            }

            // First check if flight is already tracked - if yes and runway unchanged, skip it
            bool isTracked = false;
            bool runwayChanged = false;
            string currentRunway = "";
            {
                std::lock_guard<std::mutex> lock(nonCdmExotFlightsMutex);
                for (auto& flight : nonCdmExotFlights) {
                    if (flight.callsign == callsign) {
                        isTracked = true;
                        
                        // Check current runway for runway change detection
                        CFlightPlan fp = FlightPlanSelect(callsign.c_str());
                        if (fp.IsValid()) {
                            currentRunway = fp.GetFlightPlanData().GetDepartureRwy();
                            if (flight.runway != currentRunway && !currentRunway.empty()) {
                                runwayChanged = true;
                                addLogLine("Non-CDM EXOT: Runway changed for " + callsign + " from " +
                                           flight.runway + " to " + currentRunway);
                            }
                        }
                        break;
                    }
                }
            }

            // If already tracked and runway hasn't changed, skip this flight (no re-checking needed)
            if (isTracked && !runwayChanged) {
                continue;
            }

            // For NEW flights or runway-changed flights, check taxi zone definitions
            if (!hasNonCdmTaxiZones(airport)) {
                continue;  // Airport has no taxi zone definitions, skip
            }

            // Get flight plan and coordinates (only for new/changed flights)
            CFlightPlan fp = FlightPlanSelect(callsign.c_str());
            if (!fp.IsValid()) {
                continue;  // Skip if flight plan not valid
            }

            // Skip flights that have a status set (STUP, ST-UP, PUSH, TAXI, DEPA, etc.)
            string groundState = (string)fp.GetGroundState();
            if (!groundState.empty() && 
                (groundState == "STUP" || groundState == "ST-UP" || 
                 groundState == "PUSH" || groundState == "TAXI" || 
                 groundState == "DEPA")) {
                continue;
            }

            if (currentRunway.empty()) {
                currentRunway = fp.GetFlightPlanData().GetDepartureRwy();
            }

            // Get flight coordinates for polygon matching (only once per detection/runway change)
            double flightLat = 0, flightLon = 0;
            CRadarTarget rt = RadarTargetSelect(callsign.c_str());
            if (!rt.IsValid()) {
                addLogLine("DEBUG: Non-CDM EXOT: No radar target for " + callsign);
                continue;  // Can't get coordinates, skip this flight
            }
            
            flightLat = rt.GetPosition().GetPosition().m_Latitude;
            flightLon = rt.GetPosition().GetPosition().m_Longitude;
            addLogLine("DEBUG: Non-CDM EXOT: " + callsign + " at position LAT=" + to_string(flightLat) + 
                       " LON=" + to_string(flightLon));

            // Handle new flight detection and runway changes
            {
                std::lock_guard<std::mutex> lock(nonCdmExotFlightsMutex);
                
                if (runwayChanged) {
                    // Update runway and get new taxi time
                    for (auto& flight : nonCdmExotFlights) {
                        if (flight.callsign == callsign) {
                            int newTaxiTime = getNonCdmTaxiTimeWithPolygonMatch(airport, currentRunway, flightLat, flightLon);
                            flight.runway = currentRunway;
                            flight.taxiTimeMinutes = newTaxiTime;
                            flight.sentSuccessfully = false;  // Re-send with new taxi time
                            flight.lastAttemptTime = 0;       // Reset attempt time
                            flight.attemptCount = 0;          // Reset attempt count

                            addLogLine("Non-CDM EXOT: Taxi time updated to " + to_string(newTaxiTime) +
                                       " min for runway " + currentRunway);
                            break;
                        }
                    }
                } else if (!isTracked) {
                    // NEW flight - get taxi time with polygon match and add to tracking list
                    int taxiTime = defTaxiTime;
                    if (!currentRunway.empty()) {
                        taxiTime = getNonCdmTaxiTimeWithPolygonMatch(airport, currentRunway, flightLat, flightLon);
                    }
                    NonCdmExotFlight newFlight(callsign, airport, currentRunway, taxiTime);
                    nonCdmExotFlights.push_back(newFlight);
                    addLogLine("Non-CDM EXOT: Added flight " + callsign + " at " + airport + 
                               " with taxi time " + to_string(taxiTime));
                }
            }
        }

        // Process pending flights (send initial message or retry)
        {
            std::lock_guard<std::mutex> lock(nonCdmExotFlightsMutex);
            for (size_t i = 0; i < nonCdmExotFlights.size(); i++) {
                NonCdmExotFlight& flight = nonCdmExotFlights[i];

                // Skip if already successfully sent
                if (flight.sentSuccessfully) {
                    continue;
                }

                // Skip if max attempts exceeded
                if (flight.attemptCount >= nonCdmExotMaxAttempts) {
                    addLogLine("Non-CDM EXOT: Stopped retrying " + flight.callsign + 
                               " after " + to_string(flight.attemptCount) + " failed attempts");
                    continue;
                }

                // Check if enough time has passed since last attempt
                bool shouldAttempt = false;
                if (flight.lastAttemptTime == 0) {
                    // First attempt
                    shouldAttempt = true;
                } else if ((now - flight.lastAttemptTime) >= nonCdmExotRetryIntervalSeconds) {
                    // Retry after 30-second interval
                    shouldAttempt = true;
                }

                if (shouldAttempt) {
                    // sendNonCdmExotMessage is called while holding the lock, so it can safely modify the flight object
                    bool sendSuccess = sendNonCdmExotMessage(flight.callsign, flight.airport, flight.taxiTimeMinutes);
                    
                    // Update flight object based on send result
                    if (sendSuccess) {
                        flight.sentSuccessfully = true;
                    }
                    flight.lastAttemptTime = std::time(nullptr);
                    flight.attemptCount++;
                }
            }
        }
    } catch (const std::exception& e) {
        addLogLine("ERROR: Exception in processNonCdmExotFlights: " + (string)e.what());
    } catch (...) {
        addLogLine("ERROR: Unhandled exception in processNonCdmExotFlights");
    }
}
