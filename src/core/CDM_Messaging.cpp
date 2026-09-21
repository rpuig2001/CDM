// Pilot-messaging helpers: raw Windows SendInput keystroke injection (static helpers) and the
// ATFCM/CDM private-message senders + isEvSlot. This is the final block of CDMSingle.cpp.
// Moved verbatim out of CDMSingle.cpp. See src/core/CDMGlobals.hpp for the shared-state rationale.

#include "CDMSingle.hpp"
#include "src/core/CDMGlobals.hpp"

#include <thread>

static void SendUnicodeChar(wchar_t ch) {
    INPUT in[2]{};
    in[0].type = INPUT_KEYBOARD;
    in[0].ki.wScan = ch;
    in[0].ki.dwFlags = KEYEVENTF_UNICODE;

    in[1] = in[0];
    in[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;

    SendInput(2, in, sizeof(INPUT));
}

static void SendEnter() {
    INPUT in[2]{};
    in[0].type = INPUT_KEYBOARD;
    in[0].ki.wVk = VK_RETURN;

    in[1] = in[0];
    in[1].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(2, in, sizeof(INPUT));
}

// Types a string using Unicode input (reliable across layouts)
static void TypeTextInstant(const std::string& text) {
    // Press + and Escape first
    INPUT plusDown{};
    plusDown.type = INPUT_KEYBOARD;
    plusDown.ki.wVk = VK_OEM_PLUS;

    INPUT plusUp = plusDown;
    plusUp.ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(1, &plusDown, sizeof(INPUT));
    SendInput(1, &plusUp, sizeof(INPUT));

    INPUT escDown{};
    escDown.type = INPUT_KEYBOARD;
    escDown.ki.wVk = VK_ESCAPE;

    INPUT escUp = escDown;
    escUp.ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(1, &escDown, sizeof(INPUT));
    SendInput(1, &escUp, sizeof(INPUT));

    // Convert to UTF-16
    int wlen = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (wlen <= 1) return;

    std::vector<wchar_t> wbuf((size_t)wlen);
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, wbuf.data(), wlen);

    // Each character needs 2 INPUT events (down + up)
    std::vector<INPUT> inputs;
    inputs.reserve((wlen - 1) * 2);

    for (int i = 0; i < wlen - 1; ++i) {
        INPUT down{};
        down.type = INPUT_KEYBOARD;
        down.ki.wScan = wbuf[i];
        down.ki.dwFlags = KEYEVENTF_UNICODE;

        INPUT up = down;
        up.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;

        inputs.push_back(down);
        inputs.push_back(up);
    }

    SendInput((UINT)inputs.size(), inputs.data(), sizeof(INPUT));
}

void CDM::sendAtfcmPrivateMessageToPilotCon(std::vector<std::string> flight) {
    for (int i = 0; i < (int)relevantFlights.size(); i++) {
        if (relevantFlights[i][0] == flight[0]) relevantFlights[i][11] = "true";
    }

    runDetachedTask(&CDM::sendAtfcmPrivateMessageToPilot, flight);
}

bool CDM::sendAtfcmPrivateMessageToPilot(std::vector<std::string> flight) {
    string callsign = "";
    bool correctPosition = false;
    if (ControllerMyself().IsValid()) {
        if (ControllerMyself().IsController()) {
            callsign = ControllerMyself().GetCallsign();
            if (callsign.size() > 3) {
                if (callsign.find("_DEL") != string::npos || callsign.find("_GND") != string::npos ||
                    callsign.find("_TWR") != string::npos || callsign.find("_APP") != string::npos ||
                    callsign.find("_CTR") != string::npos || callsign.find("_FMP") != string::npos) {
                    correctPosition = true;
                }
            }
        }
    }

    if (!correctPosition) {
        sendMessage("You are not in a position able to send ATFCM messages to pilots.");
        return false;
    }
    runDetachedTask(&CDM::setCdmSts, flight[0], "INFORMED/1");

    std::string message;

    const std::string& status = flight[10];

    if (status.find("FLS") != std::string::npos) {
        message = ".msg " + flight[0] + " [ATFCM MSG] OFF-BLOCK TIME EXPIRED - " + flight[0] + " (" + flight[1] +
                  " - " + flight[2] +
                  "). PLEASE, UPDATE YOUR NEW OFF-BLOCK TIME IN https://vats.im/vdgs AND MONITOR THE VDGS PANEL FOR "
                  "FUTHER UPDATES. [END OF ATFCM MSG - TRIAL IN PROGRESS]";
    } else if (flight[6] != "") {
        message = ".msg " + flight[0] + " [ATFCM MSG] SLOT ALLOCATION MESSAGE - " + flight[0] + " (" + flight[1] +
                  " - " + flight[2] + ") CTOT:" + flight[6] + " REGUL:" + flight[9] +
                  " RMK:PLEASE, MONITOR https://vats.im/vdgs FOR FURTHER CTOT UPDATES AND START-UP TIME INFORMATION. "
                  "[END OF ATFCM MSG - TRIAL IN PROGRESS]";
    } else {
        sendMessage("Unable to identify ATFCM status for flight " + flight[0] + ". Message not sent.");
        return false;
    }

    TypeTextInstant(message);
    SendEnter();

    return true;
}

void CDM::sendCdmMessageToPilot(string callsign) {
    string position = "";
    bool correctPosition = false;
    if (ControllerMyself().IsValid()) {
        if (ControllerMyself().IsController()) {
            position = ControllerMyself().GetCallsign();
            if (position.size() > 3) {
                if (position.find("_DEL") != string::npos || position.find("_GND") != string::npos ||
                    position.find("_TWR") != string::npos || position.find("_APP") != string::npos ||
                    position.find("_CTR") != string::npos || position.find("_FMP") != string::npos) {
                    correctPosition = true;
                }
            }
        }
    }

    if (!correctPosition) {
        sendMessage("You are not in a position able to send CDM messages to pilots.");
    }
    bool found = false;
    for (string flt : messagesSent) {
        if (flt == callsign) {
            found = true;
        }
    }
    if (!found) {
        messagesSent.push_back(callsign);
    }

    string msg = ".msg " + callsign + " " + pm_message;

    runDetachedTask(&CDM::setCdmSts, callsign, "INFORMED/1");

    runDetachedTask(&CDM::sendCdmPrivateMessageToPilot, msg);
}

bool CDM::sendCdmPrivateMessageToPilot(string message) {
    TypeTextInstant(message);
    SendEnter();

    return true;
}

bool CDM::isEvSlot(string callsign) {
    for (size_t i = 0; i < evCtots.size(); i++) {
        if (evCtots[i][0] == callsign && evCtots[i][1] != "") {
            return true;
        }
    }
    return false;
}
