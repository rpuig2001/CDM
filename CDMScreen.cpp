#include <windows.h>
#include "CDMScreen.h"
#include "CDMSingle.hpp"
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>
#include <map>

#define RADARSCR_OBJECT_CUSTOM 1000

#define PANEL_WIDTH 290
#define PANEL_HEADER_HEIGHT 15
#define BTN_WIDTH 42
#define BTN_HEIGHT 24
#define BTN_GAP_X 5
#define BTN_GAP_Y 4
#define PER_ROW 6

void DrawRoundedRect(HDC hDC, RECT rect, COLORREF fill, COLORREF border = RGB(50, 50, 90)) {
    HRGN rgn = CreateRoundRectRgn(rect.left, rect.top, rect.right, rect.bottom, 12, 12);
    HBRUSH brush = CreateSolidBrush(fill);
    FillRgn(hDC, rgn, brush);

    HBRUSH oldBrush = (HBRUSH)SelectObject(hDC, GetStockObject(NULL_BRUSH));
    HPEN pen = CreatePen(PS_SOLID, 2, border);
    HPEN oldPen = (HPEN)SelectObject(hDC, pen);
    RoundRect(hDC, rect.left, rect.top, rect.right, rect.bottom, 12, 12);
    SelectObject(hDC, oldBrush);
    SelectObject(hDC, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
    DeleteObject(rgn);
}

// Helper to calculate panel height based on airports count
int CDMScreen::CalculatePanelHeight(int airportCount) const {
    int displayCount = min(airportCount, MAX_AIRPORTS_DISPLAYED);
    int totalBtns = displayCount + 1; // plus button
    int rows = (totalBtns + PER_ROW - 1) / PER_ROW;
    int btnsAreaHeight = rows * BTN_HEIGHT + (rows - 1) * BTN_GAP_Y;
    return PANEL_HEADER_HEIGHT + 7 + btnsAreaHeight + 7;
}

CDMScreen::CDMScreen(CDM* pCDM)
    : cdm(pCDM), panelPosition({ 40, 560 }), minimized(false)
{
    masterAirportPanelRect = RECT{ panelPosition.x, panelPosition.y, panelPosition.x + PANEL_WIDTH, panelPosition.y + PANEL_HEADER_HEIGHT };
    for (int i = 0; i < MAX_AIRPORTS_DISPLAYED; ++i) {
        masterAirportBtnRects[i] = { 0, 0, 0, 0 };
    }
    plusBtnRect = { 0, 0, 0, 0 };
    minBtnRect = { 0, 0, 0, 0 };
    cached_airports.clear();
    pendingMasterChanges.clear();
}

CDMScreen::~CDMScreen()
{
    this->OnAsrContentToBeSaved();
}

void CDMScreen::OnAsrContentToBeClosed(void)
{
    delete this;
}

void CDMScreen::OnAsrContentToBeSaved(void) {
    cdm->SaveDataToSettings("CDMWindowPositionX", "CDM Window X", std::to_string(panelPosition.x).c_str());
    cdm->SaveDataToSettings("CDMWindowPositionY", "CDM Window Y", std::to_string(panelPosition.y).c_str());
    cdm->SaveDataToSettings("CDMMinimized", "CDM Minimized", minimized ? "1" : "0");
}

void CDMScreen::OnAsrContentLoaded(bool Loaded) {
    const char* savedX = cdm->GetDataFromSettings("CDMWindowPositionX");
    const char* savedY = cdm->GetDataFromSettings("CDMWindowPositionY");
    if (savedX && savedY) {
        panelPosition.x = std::stoi(savedX);
        panelPosition.y = std::stoi(savedY);
    }
    const char* savedMin = cdm->GetDataFromSettings("CDMMinimized");
    if (savedMin) {
        minimized = (std::string(savedMin) == "1");
    }
}

// Called every refresh cycle or timer tick to update pending indications
void CDMScreen::CheckPendingMasterChanges() {
    auto now = std::chrono::steady_clock::now();
    std::vector<std::string> toRemove;

    for (auto& kv : pendingMasterChanges) {
        const std::string& apt = kv.first;
        PendingMasterChange& pending = kv.second;

        // Timeout reached
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - pending.startTime).count();
        if (elapsed > 15) {
            toRemove.push_back(apt);
            continue;
        }

        // Check if the master airport state has changed
        auto currentMasters = cdm->getMasterAirports();
        bool nowPresent = std::find(currentMasters.begin(), currentMasters.end(), apt) != currentMasters.end();

        if ((pending.adding && nowPresent) || (!pending.adding && !nowPresent)) {
            // Change completed, remove from pending
            toRemove.push_back(apt);
        }
    }

    for (const auto& apt : toRemove) {
        pendingMasterChanges.erase(apt);
    }
}

void CDMScreen::MarkAirportPending(const std::string& icao, bool adding) {
    pendingMasterChanges[icao] = PendingMasterChange{ std::chrono::steady_clock::now(), adding };
    RequestRefresh();
}

void CDMScreen::DrawMasterAirportPanel(HDC hDC) {
    std::vector<std::string> cdmAirports = cdm->getCDMAirports();
    std::vector<std::string> masterAirports = cdm->getMasterAirports();
    std::vector<std::string> airports = cdmAirports;

    for (const auto& mapt : masterAirports) {
        if (std::find(airports.begin(), airports.end(), mapt) == airports.end()) {
            airports.push_back(mapt);
        }
    }

    cached_airports = airports;

    int fullPanelHeight = CalculatePanelHeight(static_cast<int>(airports.size()));

    masterAirportPanelRect = RECT{
        panelPosition.x,
        panelPosition.y,
        panelPosition.x + PANEL_WIDTH,
        panelPosition.y + (minimized ? PANEL_HEADER_HEIGHT : fullPanelHeight)
    };

    RECT headerRect = {
        panelPosition.x,
        panelPosition.y,
        panelPosition.x + PANEL_WIDTH,
        panelPosition.y + PANEL_HEADER_HEIGHT
    };

    minBtnRect = headerRect;
    minBtnRect.left = minBtnRect.right - 25;

    if (minimized) {
        AddScreenObject(RADARSCR_OBJECT_CUSTOM, "APTBTN_MINIMIZE", minBtnRect, true, NULL);
        DrawRoundedRect(hDC, minBtnRect, RGB(160, 160, 160), RGB(100, 100, 120));
        SetBkMode(hDC, TRANSPARENT);
        SetTextColor(hDC, RGB(50, 50, 80));
        DrawTextA(hDC, "_", -1, &minBtnRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        return;
    }

    AddScreenObject(RADARSCR_OBJECT_CUSTOM, "APTBTN_HEADER", headerRect, true, NULL);
    DrawRoundedRect(hDC, masterAirportPanelRect, RGB(90, 90, 105));
    DrawRoundedRect(hDC, headerRect, RGB(65, 65, 120));
    SetBkMode(hDC, TRANSPARENT);
    SetTextColor(hDC, RGB(245, 245, 255));
    DrawTextA(hDC, "CDM Airport Panel", -1, &headerRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    AddScreenObject(RADARSCR_OBJECT_CUSTOM, "APTBTN_MINIMIZE", minBtnRect, true, NULL);
    DrawRoundedRect(hDC, minBtnRect, RGB(200, 200, 200), RGB(100, 100, 120));
    SetTextColor(hDC, RGB(50, 50, 80));
    DrawTextA(hDC, "_", -1, &minBtnRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    int xStart = masterAirportPanelRect.left + 7;
    int yStart = masterAirportPanelRect.top + PANEL_HEADER_HEIGHT + 7;

    for (size_t i = 0; i < airports.size() && i < MAX_AIRPORTS_DISPLAYED; ++i) {
        int col = i % PER_ROW;
        int row = i / PER_ROW;
        int x = xStart + col * (BTN_WIDTH + BTN_GAP_X);
        int y = yStart + row * (BTN_HEIGHT + BTN_GAP_Y);

        RECT btnRect = { x, y, x + BTN_WIDTH, y + BTN_HEIGHT };
        masterAirportBtnRects[i] = btnRect;

        std::string btnId = "APTBTN_" + airports[i];
        AddScreenObject(RADARSCR_OBJECT_CUSTOM, btnId.c_str(), btnRect, true, NULL);

        COLORREF colFill;
        if (pendingMasterChanges.find(airports[i]) != pendingMasterChanges.end()) {
            colFill = RGB(255, 200, 30); // waiting indication
        }
        else {
            bool isMaster = std::find(masterAirports.begin(), masterAirports.end(), airports[i]) != masterAirports.end();
            colFill = isMaster ? RGB(0, 160, 0) : RGB(220, 0, 0);
        }
        DrawRoundedRect(hDC, btnRect, colFill, RGB(30, 30, 30));
        SetBkMode(hDC, TRANSPARENT);
        SetTextColor(hDC, RGB(255, 255, 255));
        DrawTextA(hDC, airports[i].c_str(), -1, &btnRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
    for (size_t i = airports.size(); i < MAX_AIRPORTS_DISPLAYED; ++i) {
        masterAirportBtnRects[i] = { 0, 0, 0, 0 };
    }

    // Plus button
    int plusIndex = std::min<size_t>(airports.size(), MAX_AIRPORTS_DISPLAYED);
    int col = plusIndex % PER_ROW;
    int row = plusIndex / PER_ROW;
    int x = xStart + col * (BTN_WIDTH + BTN_GAP_X);
    int y = yStart + row * (BTN_HEIGHT + BTN_GAP_Y);
    plusBtnRect = { x, y, x + BTN_WIDTH, y + BTN_HEIGHT };
    AddScreenObject(RADARSCR_OBJECT_CUSTOM, "APTBTN_PLUS", plusBtnRect, true, NULL);
    DrawRoundedRect(hDC, plusBtnRect, RGB(40, 180, 255), RGB(40, 120, 200));
    SetBkMode(hDC, TRANSPARENT);
    SetTextColor(hDC, RGB(255, 255, 255));
    DrawTextA(hDC, "+", -1, &plusBtnRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void CDMScreen::OnRefresh(HDC hDC, int Phase) {
    if (Phase != REFRESH_PHASE_AFTER_LISTS)
    {
        return;
    }

    CheckPendingMasterChanges();
    DrawMasterAirportPanel(hDC);
}

void CDMScreen::ToggleMasterAirport(const std::string& icao) {
    std::vector<std::string> masterAirports = cdm->getMasterAirports();
    if (std::find(masterAirports.begin(), masterAirports.end(), icao) != masterAirports.end()) {
        cdm->clearMasterAirport(icao);
        MarkAirportPending(icao, false);
    }
    else {
        cdm->addMasterAirport(icao);
        MarkAirportPending(icao, true);
    }
}

void CDMScreen::OnClickScreenObject(int ObjectType, const char* sObjectId, POINT Pt, RECT Area, int Button) {
    if (strcmp(sObjectId, "APTBTN_MINIMIZE") == 0) {
        minimized = !minimized;
        return;
    }
    if (strcmp(sObjectId, "APTBTN_PLUS") == 0) {
        cdm->OpenPopupEdit(Area, TAG_FUNC_NEW_MASTER_AIRPORT, "");
        return;
    }
    if (strncmp(sObjectId, "APTBTN_", 7) == 0) {
        std::string apt = std::string(sObjectId + 7);

        // ----- Block clicks if pending -----
        if (pendingMasterChanges.find(apt) != pendingMasterChanges.end()) {
            // Change in progress; ignore click
            return;
        }

        ToggleMasterAirport(apt);
        return;
    }
}

void CDMScreen::OnMoveScreenObject(int ObjectType, const char* sObjectId, POINT Pt, RECT Area, bool Released) {
    if (strcmp(sObjectId, "APTBTN_HEADER") == 0) {
        panelPosition.x = Area.left;
        panelPosition.y = Area.top;
        RequestRefresh();
    }
    if (strcmp(sObjectId, "APTBTN_MINIMIZE") == 0) {
        panelPosition.x = Area.left - PANEL_WIDTH + 25;
        panelPosition.y = Area.top;
        RequestRefresh();
    }
}