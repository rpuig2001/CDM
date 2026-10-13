// De-ice time lookup tables, EvCTOT/vatCan slot parsing, panel/ATFCM display flags, and the
// `explode` string-splitting helper. Moved verbatim out of CDMSingle.cpp.
// See src/core/CDMGlobals.hpp for the shared-state rationale.

#include "CDMSingle.hpp"
#include "src/core/CDMGlobals.hpp"

int CDM::addDeIceTime(string callsign, char wtc) {
    bool isDeice = false;
    int remNum = getDeIceId(callsign);

    if (remNum >= 0) {
        return getDeIceTime(wtc, remNum);
    }
    return 0;
}

int CDM::getDeIceTime(char wtc, int remNum) {
    if (wtc == 'L') {
        if (remNum == 1) return deIceTimeL + deIceTaxiRem1;
        if (remNum == 2) return deIceTimeL + deIceTaxiRem2;
        if (remNum == 3) return deIceTimeL + deIceTaxiRem3;
        if (remNum == 4) return deIceTimeL + deIceTaxiRem4;
        if (remNum == 5) return deIceTimeL + deIceTaxiRem5;
        return deIceTimeL;
    } else if (wtc == 'M') {
        if (remNum == 1) return deIceTimeM + deIceTaxiRem1;
        if (remNum == 2) return deIceTimeM + deIceTaxiRem2;
        if (remNum == 3) return deIceTimeM + deIceTaxiRem3;
        if (remNum == 4) return deIceTimeM + deIceTaxiRem4;
        if (remNum == 5) return deIceTimeM + deIceTaxiRem5;
        return deIceTimeM;
    } else if (wtc == 'H') {
        if (remNum == 1) return deIceTimeH + deIceTaxiRem1;
        if (remNum == 2) return deIceTimeH + deIceTaxiRem2;
        if (remNum == 3) return deIceTimeH + deIceTaxiRem3;
        if (remNum == 4) return deIceTimeH + deIceTaxiRem4;
        if (remNum == 5) return deIceTimeH + deIceTaxiRem5;
        return deIceTimeH;
    } else if (wtc == 'J') {
        if (remNum == 1) return deIceTimeJ + deIceTaxiRem1;
        if (remNum == 2) return deIceTimeJ + deIceTaxiRem2;
        if (remNum == 3) return deIceTimeJ + deIceTaxiRem3;
        if (remNum == 4) return deIceTimeJ + deIceTaxiRem4;
        if (remNum == 5) return deIceTimeJ + deIceTaxiRem5;
        return deIceTimeJ;
    }

    return deIceTimeM;
}

void CDM::addVatcanCtotToEvCTOT(string line) {
    // Expected:
    //  - "vatcan,ctot"
    //  - "vatcan,callsign,ctot"
    //
    // Store as:
    //  [0] vatcan
    //  [1] callsign ("" if not provided)
    //  [2] departure ICAO ("" if not provided)
    //  [3] destination ICAO ("" if not provided)
    //  [4] ctot

    const auto parts = explode(line, ',');

    if (parts.size() == 2) {
        slotFile.push_back({parts[0], "", "", "", parts[1]});
    } else if (parts.size() == 3) {
        slotFile.push_back({parts[0], parts[1], "", "", parts[2]});
    } else if (parts.size() == 5) {
        slotFile.push_back({parts[0], parts[1], parts[2], parts[3], parts[4]});
    } else {
        return;
    }
}

bool CDM::getPanelStatus() { return showPanel; }

bool CDM::getAtfcmList() { return showAtfcmList; }

vector<string> CDM::explode(std::string const& s, char delim) {
    std::vector<std::string> result;
    std::istringstream iss(s);

    for (std::string token; std::getline(iss, token, delim);) {
        result.push_back(std::move(token));
    }

    return result;
}
