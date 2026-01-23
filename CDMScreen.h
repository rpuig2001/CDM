#pragma once

#include <vector>
#include <string>
#include <windows.h>
#include "EuroScopePlugIn.h"
#include <map>
#include <chrono>

#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "User32.lib")

using namespace std;
using namespace EuroScopePlugIn;

class CDM;

struct PendingMasterChange {
    std::chrono::steady_clock::time_point startTime;
    bool adding; // true=add, false=remove
};

class CDMScreen : public CRadarScreen {
public:
    CDMScreen(CDM* pCDM);
    virtual ~CDMScreen();

    std::map<std::string, PendingMasterChange> pendingMasterChanges;

    void OnAsrContentToBeClosed(void);
    void OnAsrContentToBeSaved(void);
    void OnAsrContentLoaded(bool Loaded);
    void OnRefresh(HDC hDC, int Phase);
    void DrawMasterAirportPanel(HDC hDC);
    void ToggleMasterAirport(const std::string& icao);
    void OnClickScreenObject(int ObjectType, const char* sObjectId, POINT Pt, RECT Area, int Button);
    void OnMoveScreenObject(int ObjectType, const char* sObjectId, POINT Pt, RECT Area, bool Released);
    static constexpr int MAX_AIRPORTS_DISPLAYED = 9999;
    int CalculatePanelHeight(int airportCount) const;
    void CheckPendingMasterChanges();
    void MarkAirportPending(const std::string& icao, bool adding);

    RECT masterAirportPanelRect;
    RECT masterAirportBtnRects[MAX_AIRPORTS_DISPLAYED];
    RECT plusBtnRect;
    RECT minBtnRect;

    std::vector<std::string> cached_airports;
    POINT panelPosition;
    bool minimized;

private:
    CDM* cdm;
};