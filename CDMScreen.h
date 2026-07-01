#pragma once

#include <windows.h>

#include <string>
#include <vector>
#pragma warning(push, 0)
#include "EuroScopePlugIn.h"
#pragma warning(pop)
#include <chrono>
#include <map>

#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "User32.lib")

using namespace std;
using namespace EuroScopePlugIn;

class CDM;

struct PendingMasterChange {
    std::chrono::steady_clock::time_point startTime;
    bool adding;  // true=add, false=remove
};

// Pending block capacity change structure
struct PendingBlockChange {
    std::string runway;
    int blockHour;        // Hour this block belongs to (19, 20, etc.)
    int blockIndex;
    int newCapacity;
    int previousCapacity;  // To allow reverting changes
};

// Block occupancy data structure
struct BlockData {
    std::string runway;
    int blockIndex;  // 0-5 (for 60 minutes)
    int blockHour;   // hour this block belongs to
    int capacity;
    int occupancy;
    std::string timeRange;  // e.g., "16:30-16:39"
};

class CDMScreen : public CRadarScreen {
   public:
    CDMScreen(CDM* pCDM);
    virtual ~CDMScreen();

    std::map<std::string, PendingMasterChange> pendingMasterChanges;

    void OnAsrContentToBeClosed(void);
    void OnAsrContentToBeSaved(void);
    void OnAsrContentLoaded(bool Loaded);
    void OnAirportRunwayActivityChanged(void);
    void OnRefresh(HDC hDC, int Phase);

    void DrawMasterAirportPanel(HDC hDC);
    void ToggleMasterAirport(const std::string& icao);
    void OnClickScreenObject(int ObjectType, const char* sObjectId, POINT Pt, RECT Area, int Button);
    void OnMoveScreenObject(int ObjectType, const char* sObjectId, POINT Pt, RECT Area, bool Released);
    void OnMouseMove(POINT Pt);

    // Blocks panel methods
    void DrawBlocksPanel(HDC hDC);
    void ShowBlocksWindow(const std::string& airport);
    void HideBlocksWindow();
    void UpdateBlocksData(const std::string& airport);
    void RefreshBlocksData();
    void SetRunwayFilter(const std::string& runway);
    std::vector<std::pair<std::string, std::string>> GetCallsignsForBlock(const std::string& runway, int blockIndex);
    std::vector<std::string> GetBlockRunways() const;

    static constexpr int MAX_AIRPORTS_DISPLAYED = 9999;
    int CalculatePanelHeight(int airportCount) const;
    void CheckPendingMasterChanges();
    void MarkAirportPending(const std::string& icao, bool adding);

    // -----------------------
    // Flights panel (reads from CDM::relevantFlights)
    // -----------------------
    void DrawRelevantFlightsPanel(HDC hDC);
    std::vector<std::vector<std::string>> GetFilteredRelevantFlightsRows() const;

    // -----------------------
    // Airport panel geometry
    // -----------------------
    RECT masterAirportPanelRect;
    RECT masterAirportBtnRects[MAX_AIRPORTS_DISPLAYED];
    RECT plusBtnRect;
    RECT minBtnRect;

    std::vector<std::string> cached_airports;
    POINT panelPosition;
    bool minimized;

    // -----------------------
    // Flights panel state
    // -----------------------
    POINT flightsPanelPos{40, 640};
    RECT flightsPanelRect{0, 0, 0, 0};
    RECT flightsHeaderRect{0, 0, 0, 0};
    bool showAtfcmAllFlights = false;
    bool showAtfcmAllCdmFlights = false;
    RECT flightsFilterRect;
    int sortColumn;
    bool sortAscending;

    // -----------------------
    // Blocks panel state
    // -----------------------
    std::string selectedAirportForBlocks;
    std::string selectedRunwayFilter;  // Empty = show all
    bool showBlocksPanel = false;
    bool blocksPanelBmiMode = true;  // true=BMI (6 blocks), false=Airport Monitoring (3 windows)
    POINT blocksPanelPos{1100, 100};
    RECT blocksPanelRect{0, 0, 0, 0};
    std::vector<BlockData> currentBlocksData;
    std::map<std::tuple<std::string, int, int>, int> customBlockCapacities;  // {runway, hour, blockIndex} -> custom capacity override
    std::map<std::tuple<std::string, int, int>, int> calculatedBlockCapacities;  // {runway, hour, blockIndex} -> default calculated capacity
    std::chrono::steady_clock::time_point lastBlocksDataUpdate;  // Debounce frequent updates
    std::string selectedBlockRunway;  // For displaying callsigns of a selected block
    int selectedBlockIndex = -1;  // -1 means no block selected
    std::vector<PendingBlockChange> pendingBlockChanges;  // Pending capacity changes waiting for APPLY
    RECT blocksPanelApplyBtnRect;  // Rectangle for APPLY button
    
    // Pending block changes methods
    void ApplyPendingBlockChanges();
    void RevertPendingBlockChanges();

   private:
    CDM* cdm;
};