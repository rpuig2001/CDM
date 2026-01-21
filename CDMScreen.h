#pragma once

#include <vector>
#include <string>
#include <windows.h>
#include "EuroScopePlugIn.h"

#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "User32.lib")

using namespace std;
using namespace EuroScopePlugIn;

class CDM;

class CDMScreen : public CRadarScreen {
public:
    CDMScreen(CDM* pCDM);
    virtual ~CDMScreen();

    void OnAsrContentToBeClosed(void);
    void OnAsrContentToBeSaved(void);
    void OnAsrContentLoaded(bool Loaded);
    void OnRefresh(HDC hDC, int Phase);
    void DrawMasterAirportPanel(HDC hDC);
    void ToggleMasterAirport(const std::string& icao);
    void OnClickScreenObject(int ObjectType, const char* sObjectId, POINT Pt, RECT Area, int Button);
    void OnMoveScreenObject(int ObjectType, const char* sObjectId, POINT Pt, RECT Area, bool Released);
    static constexpr int MAX_AIRPORTS_DISPLAYED = 24;

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