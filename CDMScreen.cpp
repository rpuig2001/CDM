#include "CDMScreen.h"

#include <windows.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "CDMSingle.hpp"

// External global variables
extern std::vector<Plane> slotList;

#define RADARSCR_OBJECT_CUSTOM 1000

#define PANEL_WIDTH 290
#define PANEL_HEADER_HEIGHT 15
#define BTN_WIDTH 42
#define BTN_HEIGHT 24
#define BTN_GAP_X 5
#define BTN_GAP_Y 4
#define PER_ROW 6

// -----------------------
// Relevant flights panel (UI reads CDM::relevantFlights)
// -----------------------
#define FLT_PANEL_WIDTH 880
#define FLT_HEADER_HEIGHT 16
#define FLT_ROW_HEIGHT 18
#define FLT_ROW_GAP 2
#define FLT_MAX_ROWS 500

static std::string ToLowerCopy(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    return s;
}

static bool ContainsCI(const std::string& hay, const std::string& needle) {
    if (needle.empty()) return true;
    return ToLowerCopy(hay).find(ToLowerCopy(needle)) != std::string::npos;
}

static bool IsTrueString(const std::string& s) {
    std::string v = ToLowerCopy(s);
    v.erase(std::remove_if(v.begin(), v.end(), [](unsigned char c) { return std::isspace(c); }), v.end());
    return (v == "true" || v == "1" || v == "yes" || v == "y" || v == "ok");
}

// renamed to avoid collision with anything else in your project
static void DrawCellTextA_Flt(HDC hDC, const std::string& s, RECT r, UINT fmt = DT_LEFT,
                              COLORREF color = RGB(255, 255, 255)) {
    SetBkMode(hDC, TRANSPARENT);
    SetTextColor(hDC, color);
    DrawTextA(hDC, s.c_str(), -1, &r, fmt | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
}

static std::string GetColSafe(const std::vector<std::string>& row, int idx) {
    if (idx < 0) return "";
    if ((size_t)idx >= row.size()) return "";
    return row[(size_t)idx];
}

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

static COLORREF DimColor(COLORREF c, float factor) {
    BYTE r = (BYTE)(GetRValue(c) * factor);
    BYTE g = (BYTE)(GetGValue(c) * factor);
    BYTE b = (BYTE)(GetBValue(c) * factor);
    return RGB(r, g, b);
}

static void DrawSmallTextQuality(HDC hDC, const char* text, RECT rect, COLORREF color) {
    if (!hDC || !text || !*text) return;

    int oldBkMode = SetBkMode(hDC, TRANSPARENT);

    COLORREF dimmed = DimColor(color, 0.75f);
    COLORREF oldClr = SetTextColor(hDC, dimmed);

    int oldAlign = GetTextAlign(hDC);
    int oldGm = SetGraphicsMode(hDC, GM_COMPATIBLE);

    LOGFONTW lf{};
    lf.lfHeight = -6;
    lf.lfWeight = FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfQuality = NONANTIALIASED_QUALITY;
    lstrcpyW(lf.lfFaceName, L"Small Fonts");

    HFONT hFont = CreateFontIndirectW(&lf);
    HFONT oldFont = (HFONT)SelectObject(hDC, hFont);

    std::wstring wtext;
    int wlen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, nullptr, 0);
    UINT usedCP = CP_UTF8;

    if (wlen <= 0) {
        usedCP = CP_ACP;
        wlen = MultiByteToWideChar(CP_ACP, 0, text, -1, nullptr, 0);
    }

    if (wlen > 0) {
        wtext.resize((size_t)wlen);
        MultiByteToWideChar(usedCP, (usedCP == CP_UTF8) ? MB_ERR_INVALID_CHARS : 0, text, -1, &wtext[0], wlen);

        if (!wtext.empty() && wtext.back() == L'\0') wtext.pop_back();
    }

    if (!wtext.empty()) {
        SIZE sz{};
        GetTextExtentPoint32W(hDC, wtext.c_str(), (int)wtext.length(), &sz);

        int x = rect.left + ((rect.right - rect.left) - sz.cx) / 2;
        int y = rect.top + ((rect.bottom - rect.top) - sz.cy) / 2;

        SetTextAlign(hDC, TA_LEFT | TA_TOP);

        ExtTextOutW(hDC, x, y, ETO_CLIPPED, &rect, wtext.c_str(), (UINT)wtext.length(), nullptr);
    }

    SelectObject(hDC, oldFont);
    DeleteObject(hFont);

    SetTextAlign(hDC, oldAlign);
    SetGraphicsMode(hDC, oldGm);
    SetTextColor(hDC, oldClr);
    SetBkMode(hDC, oldBkMode);
}

// Helper to calculate panel height based on airports count
int CDMScreen::CalculatePanelHeight(int airportCount) const {
    int displayCount = min(airportCount, MAX_AIRPORTS_DISPLAYED);
    int totalBtns = displayCount + 1;  // plus button
    int rows = (totalBtns + PER_ROW - 1) / PER_ROW;
    int btnsAreaHeight = rows * BTN_HEIGHT + (rows - 1) * BTN_GAP_Y;
    return PANEL_HEADER_HEIGHT + 7 + btnsAreaHeight + 7;
}

CDMScreen::CDMScreen(CDM* pCDM) : cdm(pCDM), panelPosition({40, 560}), minimized(false) {
    masterAirportPanelRect =
        RECT{panelPosition.x, panelPosition.y, panelPosition.x + PANEL_WIDTH, panelPosition.y + PANEL_HEADER_HEIGHT};
    for (int i = 0; i < MAX_AIRPORTS_DISPLAYED; ++i) {
        masterAirportBtnRects[i] = {0, 0, 0, 0};
    }
    plusBtnRect = {0, 0, 0, 0};
    minBtnRect = {0, 0, 0, 0};
    cached_airports.clear();
    pendingMasterChanges.clear();

    flightsPanelPos = {40, 640};
    flightsPanelRect = {0, 0, 0, 0};
    flightsHeaderRect = {0, 0, 0, 0};
    flightsFilterRect = {0, 0, 0, 0};

    // state
    showAtfcmAllFlights = false;    // informed filter
    showAtfcmAllCdmFlights = true;  // NEW: true=all (CDM+non-CDM), false=only non-CDM

    // column click sorting
    sortColumn = -1;
    sortAscending = true;
}

CDMScreen::~CDMScreen() { this->OnAsrContentToBeSaved(); }

void CDMScreen::OnAsrContentToBeClosed(void) { delete this; }

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

        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - pending.startTime).count();
        if (elapsed > 15) {
            toRemove.push_back(apt);
            continue;
        }

        auto currentMasters = cdm->getMasterAirports();
        bool nowPresent = std::find(currentMasters.begin(), currentMasters.end(), apt) != currentMasters.end();

        if ((pending.adding && nowPresent) || (!pending.adding && !nowPresent)) {
            toRemove.push_back(apt);
        }
    }

    for (const auto& apt : toRemove) {
        pendingMasterChanges.erase(apt);
    }
}

void CDMScreen::MarkAirportPending(const std::string& icao, bool adding) {
    pendingMasterChanges[icao] = PendingMasterChange{std::chrono::steady_clock::now(), adding};
    RequestRefresh();
}

// ------------------------------
// Flights Panel
// Data layout (server parsing):
//  0 callsign, 1 dep, 2 arr, 3 eobt, 4 tobt, 5 taxi, 6 ctot, 7 aobt,
//  8 eta, 9 mostPenalizingAirspace, 10 atfcmStatus, 11 informed, 12 isCdm,
//  13 isExcluded, 14 isRea, 15 isSir, 16 isSwm
// ------------------------------
std::vector<std::vector<std::string>> CDMScreen::GetFilteredRelevantFlightsRows() const {
    if (!cdm) return {};
    const auto& all = cdm->returnRelevantFlights();

    std::vector<std::vector<std::string>> out;
    out.reserve(all.size());

    for (const auto& row : all) {
        // checkbox #1: if showAll==false -> only non-informed
        if (!showAtfcmAllFlights) {
            bool informed = IsTrueString(GetColSafe(row, 11));
            if (informed) continue;
        }

        // checkbox #2: if showAllCdm==false -> only non-CDM
        if (!showAtfcmAllCdmFlights) {
            bool isCdm = IsTrueString(GetColSafe(row, 12));
            if (isCdm) continue;
        }

        std::string flightsFilterText = cdm->getFilterFlightsText();

        // text filter: match any column
        if (!flightsFilterText.empty()) {
            bool any = false;
            for (const auto& cell : row) {
                if (ContainsCI(cell, flightsFilterText)) {
                    any = true;
                    break;
                }
            }
            if (!any) continue;
        }

        out.push_back(row);
    }

    // sorting by clicked column
    if (sortColumn >= 0) {
        std::stable_sort(out.begin(), out.end(),
                         [&](const std::vector<std::string>& a, const std::vector<std::string>& b) {
                             std::string av = GetColSafe(a, sortColumn);
                             std::string bv = GetColSafe(b, sortColumn);
                             int cmp = _stricmp(av.c_str(), bv.c_str());
                             if (sortAscending) return cmp < 0;
                             return cmp > 0;
                         });
    }

    return out;
}

void CDMScreen::DrawRelevantFlightsPanel(HDC hDC) {
    if (!cdm) return;
    if (!cdm->getAtfcmList()) return;

    std::string flightsFilterText = cdm->getFilterFlightsText();
    auto flights = GetFilteredRelevantFlightsRows();

    int rows = (int)std::min<size_t>(flights.size(), FLT_MAX_ROWS);
    int bodyH = rows * FLT_ROW_HEIGHT + (rows > 0 ? (rows - 1) * FLT_ROW_GAP : 0);

    // header + gap + checkbox row + gap + checkbox row + gap + filter row + gap + column header + gap + list + gap
    int panelH = FLT_HEADER_HEIGHT + 6 + 18 + 6 + 18 + 6 + 18 + 6 + 18 + 6 + bodyH + 6;

    flightsPanelRect =
        RECT{flightsPanelPos.x, flightsPanelPos.y, flightsPanelPos.x + FLT_PANEL_WIDTH, flightsPanelPos.y + panelH};
    flightsHeaderRect = RECT{flightsPanelRect.left, flightsPanelRect.top, flightsPanelRect.right,
                             flightsPanelRect.top + FLT_HEADER_HEIGHT};

    AddScreenObject(RADARSCR_OBJECT_CUSTOM, "FLT_HDR", flightsHeaderRect, true, NULL);

    DrawRoundedRect(hDC, flightsPanelRect, RGB(70, 70, 85));
    DrawRoundedRect(hDC, flightsHeaderRect, RGB(45, 45, 105));
    SetBkMode(hDC, TRANSPARENT);
    SetTextColor(hDC, RGB(245, 245, 255));
    DrawTextA(hDC, "ATFCM Flight List", -1, &flightsHeaderRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    // Checkbox row #1 (Informed filter)
    RECT chkRow = flightsPanelRect;
    chkRow.top = flightsHeaderRect.bottom + 6;
    chkRow.bottom = chkRow.top + 18;
    chkRow.left += 6;
    chkRow.right -= 6;

    RECT chkBox = chkRow;
    chkBox.right = chkBox.left + 18;

    RECT chkLabel = chkRow;
    chkLabel.left = chkBox.right + 6;

    AddScreenObject(RADARSCR_OBJECT_CUSTOM, "FLT_CHK_ALL", chkRow, true, NULL);

    DrawRoundedRect(hDC, chkBox, RGB(90, 90, 110), RGB(30, 30, 30));
    if (showAtfcmAllFlights) {
        SetBkMode(hDC, TRANSPARENT);
        SetTextColor(hDC, RGB(255, 255, 255));
        DrawTextA(hDC, "X", -1, &chkBox, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }

    DrawCellTextA_Flt(hDC, "Show ALL flights (otherwise only non-informed)", chkLabel, DT_LEFT, RGB(255, 255, 255));

    // Checkbox row #2 (CDM/non-CDM filter)
    RECT chkRow2 = flightsPanelRect;
    chkRow2.top = chkRow.bottom + 6;
    chkRow2.bottom = chkRow2.top + 18;
    chkRow2.left += 6;
    chkRow2.right -= 6;

    RECT chkBox2 = chkRow2;
    chkBox2.right = chkBox2.left + 18;

    RECT chkLabel2 = chkRow2;
    chkLabel2.left = chkBox2.right + 6;

    AddScreenObject(RADARSCR_OBJECT_CUSTOM, "FLT_CHK_CDM", chkRow2, true, NULL);

    DrawRoundedRect(hDC, chkBox2, RGB(90, 90, 110), RGB(30, 30, 30));
    if (showAtfcmAllCdmFlights) {
        SetBkMode(hDC, TRANSPARENT);
        SetTextColor(hDC, RGB(255, 255, 255));
        DrawTextA(hDC, "X", -1, &chkBox2, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }

    DrawCellTextA_Flt(hDC, "Show CDM AND non-CDM.", chkLabel2, DT_LEFT, RGB(255, 255, 255));

    // Filter row (click to edit)
    RECT filterRow = flightsPanelRect;
    filterRow.top = chkRow2.bottom + 6;
    filterRow.bottom = filterRow.top + 18;
    filterRow.left += 6;
    filterRow.right -= 6;

    flightsFilterRect = filterRow;
    AddScreenObject(RADARSCR_OBJECT_CUSTOM, "FLT_FILTER", flightsFilterRect, true, NULL);

    DrawRoundedRect(hDC, flightsFilterRect, RGB(90, 90, 110), RGB(30, 30, 30));
    RECT fr = flightsFilterRect;
    fr.left += 6;
    std::string filterLabel = "Filter: " + (flightsFilterText.empty() ? std::string("<none>") : flightsFilterText);
    DrawCellTextA_Flt(hDC, filterLabel, fr, DT_LEFT, RGB(255, 255, 255));

    // Column header row (each header is clickable)
    RECT colHdr = flightsPanelRect;
    colHdr.top = filterRow.bottom + 6;
    colHdr.bottom = colHdr.top + 18;
    colHdr.left += 6;
    colHdr.right -= 6;

    DrawRoundedRect(hDC, colHdr, RGB(55, 55, 70), RGB(30, 30, 30));

    int x = colHdr.left + 4;

    auto addHeader = [&](const char* id, const char* label, int w, bool center = false) {
        RECT r{x, colHdr.top, x + w, colHdr.bottom};
        x += w;
        AddScreenObject(RADARSCR_OBJECT_CUSTOM, id, r, true, NULL);
        DrawCellTextA_Flt(hDC, label, r, center ? DT_CENTER : DT_LEFT, RGB(255, 255, 255));
        return r;
    };

    // NOTE: We include only visible columns in this compact layout
    addHeader("FLT_COL_0", "CS", 70);
    addHeader("FLT_COL_1", "DEP", 55);
    addHeader("FLT_COL_2", "ARR", 55);
    addHeader("FLT_COL_3", "EOBT", 50);
    addHeader("FLT_COL_4", "TOBT", 50);
    addHeader("FLT_COL_5", "TX", 35);
    addHeader("FLT_COL_6", "CTOT", 50);
    addHeader("FLT_COL_7", "AOBT", 50);
    addHeader("FLT_COL_8", "ETA", 50);
    addHeader("FLT_COL_9", "REGUL", 130);
    addHeader("FLT_COL_10", "ATFCM", 60);
    addHeader("FLT_COL_13", "EXCL", 40, true);
    addHeader("FLT_COL_14", "REA", 40, true);
    addHeader("FLT_COL_15", "SIR", 40, true);
    addHeader("FLT_COL_16", "SWM", 40, true);

    addHeader("FLT_SEND_HDR", "", 45, true);

    // Rows
    int y = colHdr.bottom + 6;

    // Helper: draw centered green/red square instead of text
    auto boolFill = [](bool v) { return v ? RGB(0, 170, 0) : RGB(220, 0, 0); };

    auto drawBoolSquare = [&](RECT r, bool value) {
        int cellW = (r.right - r.left);
        int cellH = (r.bottom - r.top);

        int size = min(cellW, cellH) - 6;  // padding
        if (size < 6) size = min(cellW, cellH);

        int left = r.left + (cellW - size) / 2;
        int top = r.top + (cellH - size) / 2;

        RECT sq{left, top, left + size, top + size};

        // Using DrawRoundedRect with small radius still looks like a square here
        DrawRoundedRect(hDC, sq, boolFill(value), RGB(30, 30, 30));
    };

    for (int i = 0; i < rows; i++) {
        RECT rowRect{flightsPanelRect.left + 6, y, flightsPanelRect.right - 6, y + FLT_ROW_HEIGHT};

        // SEND button rect (existing behavior)
        RECT sendRect = rowRect;
        sendRect.left = sendRect.right - 45;

        char sendId[64];
        sprintf_s(sendId, "FLT_ACT_%d", i);
        AddScreenObject(RADARSCR_OBJECT_CUSTOM, sendId, sendRect, true, NULL);

        COLORREF bg = (i % 2 == 0) ? RGB(80, 80, 95) : RGB(74, 74, 88);
        DrawRoundedRect(hDC, rowRect, bg, RGB(30, 30, 30));

        const auto& row = flights[(size_t)i];

        bool informed = IsTrueString(GetColSafe(row, 11));
        COLORREF sendFill = informed ? RGB(0, 160, 0) : RGB(220, 0, 0);
        DrawRoundedRect(hDC, sendRect, sendFill, RGB(60, 60, 60));

        int cx = rowRect.left + 4;
        auto cell = [&](int w) {
            RECT r{cx, rowRect.top, cx + w, rowRect.bottom};
            cx += w;
            return r;
        };

        bool isCdm = IsTrueString(GetColSafe(row, 12));
        COLORREF callsignColor = isCdm ? RGB(100, 255, 100) : RGB(255, 255, 255);

        DrawCellTextA_Flt(hDC, GetColSafe(row, 0), cell(70), DT_LEFT, callsignColor);  // callsign
        DrawCellTextA_Flt(hDC, GetColSafe(row, 1), cell(55));                          // dep
        DrawCellTextA_Flt(hDC, GetColSafe(row, 2), cell(55));                          // arr
        DrawCellTextA_Flt(hDC, GetColSafe(row, 3), cell(50));                          // eobt
        DrawCellTextA_Flt(hDC, GetColSafe(row, 4), cell(50));                          // tobt
        DrawCellTextA_Flt(hDC, GetColSafe(row, 5), cell(35));                          // taxi
        DrawCellTextA_Flt(hDC, GetColSafe(row, 6), cell(50));                          // ctot
        DrawCellTextA_Flt(hDC, GetColSafe(row, 7), cell(50));                          // aobt
        DrawCellTextA_Flt(hDC, GetColSafe(row, 8), cell(50));                          // eta
        DrawCellTextA_Flt(hDC, GetColSafe(row, 9), cell(130));                         // mostPenalizingAirspace
        DrawCellTextA_Flt(hDC, GetColSafe(row, 10), cell(60));                         // atfcmStatus

        // NEW: EXCL/REA/SIR clickable cells with colored squares
        RECT exclRect = cell(40);
        RECT reaRect = cell(40);
        RECT sirRect = cell(40);
        RECT swmRect = cell(40);

        char exclId[64], reaId[64], sirId[64], swmId[64];
        sprintf_s(exclId, "FLT_EXCL_%d", i);
        sprintf_s(reaId, "FLT_REA_%d", i);
        sprintf_s(sirId, "FLT_SIR_%d", i);
        sprintf_s(swmId, "FLT_SWM_%d", i);

        AddScreenObject(RADARSCR_OBJECT_CUSTOM, exclId, exclRect, true, NULL);
        AddScreenObject(RADARSCR_OBJECT_CUSTOM, reaId, reaRect, true, NULL);
        AddScreenObject(RADARSCR_OBJECT_CUSTOM, sirId, sirRect, true, NULL);
        AddScreenObject(RADARSCR_OBJECT_CUSTOM, swmId, swmRect, true, NULL);

        bool isExcluded = IsTrueString(GetColSafe(row, 13));
        bool isRea = IsTrueString(GetColSafe(row, 14));
        bool isSir = IsTrueString(GetColSafe(row, 15));
        bool isSwm = IsTrueString(GetColSafe(row, 16));

        drawBoolSquare(exclRect, isExcluded);
        drawBoolSquare(reaRect, isRea);
        drawBoolSquare(sirRect, isSir);
        drawBoolSquare(swmRect, isSwm);

        SetBkMode(hDC, TRANSPARENT);
        SetTextColor(hDC, RGB(255, 255, 255));
        DrawTextA(hDC, "SEND", -1, &sendRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

        y += FLT_ROW_HEIGHT + FLT_ROW_GAP;
    }
}

// ------------------------------
// Existing airport panel (unchanged)
// ------------------------------
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

    auto allMasters = cdm->getServerMasterAirports();

    int fullPanelHeight = CalculatePanelHeight(static_cast<int>(airports.size()));

    masterAirportPanelRect = RECT{panelPosition.x, panelPosition.y, panelPosition.x + PANEL_WIDTH,
                                  panelPosition.y + (minimized ? PANEL_HEADER_HEIGHT : fullPanelHeight)};

    RECT headerRect = {panelPosition.x, panelPosition.y, panelPosition.x + PANEL_WIDTH,
                       panelPosition.y + PANEL_HEADER_HEIGHT};

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
        int col = static_cast<int>(i % PER_ROW);
        int row = static_cast<int>(i / PER_ROW);
        int x = xStart + col * (BTN_WIDTH + BTN_GAP_X);
        int y = yStart + row * (BTN_HEIGHT + BTN_GAP_Y);

        RECT btnRect = {x, y, x + BTN_WIDTH, y + BTN_HEIGHT};
        masterAirportBtnRects[i] = btnRect;

        std::string btnId = "APTBTN_" + airports[i];
        AddScreenObject(RADARSCR_OBJECT_CUSTOM, btnId.c_str(), btnRect, true, NULL);

        COLORREF colFill;

        bool showServerPos = false;
        std::string serverPosText;

        if (pendingMasterChanges.find(airports[i]) != pendingMasterChanges.end()) {
            colFill = RGB(255, 200, 30);
        } else {
            const bool isCurrentMaster =
                (std::find(masterAirports.begin(), masterAirports.end(), airports[i]) != masterAirports.end());

            bool isServerMaster = false;
            for (const auto& entry : allMasters) {
                if (!entry.empty() && entry[0] == airports[i]) {
                    isServerMaster = true;
                    if (entry.size() >= 2) {
                        serverPosText = entry[1];
                    }
                    showServerPos = !serverPosText.empty();
                    break;
                }
            }

            if (isCurrentMaster) {
                colFill = RGB(0, 160, 0);
                showServerPos = false;
            } else if (isServerMaster) {
                colFill = RGB(160, 0, 160);
            } else {
                colFill = RGB(220, 0, 0);
            }
        }

        DrawRoundedRect(hDC, btnRect, colFill, RGB(30, 30, 30));

        SetBkMode(hDC, TRANSPARENT);
        SetTextColor(hDC, RGB(255, 255, 255));

        if (showServerPos) {
            RECT topRect = btnRect;
            RECT botRect = btnRect;

            int h = (btnRect.bottom - btnRect.top);
            int split = btnRect.top + (h * 3) / 5;

            topRect.bottom = split;
            botRect.top = split - 1;
            topRect.top += 2;

            DrawTextA(hDC, airports[i].c_str(), -1, &topRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            DrawSmallTextQuality(hDC, serverPosText.c_str(), botRect, RGB(255, 255, 255));
        } else {
            DrawTextA(hDC, airports[i].c_str(), -1, &btnRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        }
    }

    for (size_t i = airports.size(); i < MAX_AIRPORTS_DISPLAYED; ++i) {
        masterAirportBtnRects[i] = {0, 0, 0, 0};
    }

    int plusIndex = static_cast<int>(std::min<size_t>(airports.size(), MAX_AIRPORTS_DISPLAYED));
    int col = plusIndex % PER_ROW;
    int row = plusIndex / PER_ROW;
    int x = xStart + col * (BTN_WIDTH + BTN_GAP_X);
    int y = yStart + row * (BTN_HEIGHT + BTN_GAP_Y);
    plusBtnRect = {x, y, x + BTN_WIDTH, y + BTN_HEIGHT};
    AddScreenObject(RADARSCR_OBJECT_CUSTOM, "APTBTN_PLUS", plusBtnRect, true, NULL);
    DrawRoundedRect(hDC, plusBtnRect, RGB(40, 180, 255), RGB(40, 120, 200));
    SetBkMode(hDC, TRANSPARENT);
    SetTextColor(hDC, RGB(255, 255, 255));
    DrawTextA(hDC, "+", -1, &plusBtnRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void CDMScreen::OnRefresh(HDC hDC, int Phase) {
    if (Phase != REFRESH_PHASE_AFTER_LISTS) {
        return;
    }

    DrawRelevantFlightsPanel(hDC);

    if (!cdm || !cdm->getPanelStatus()) {
        return;
    }

    CheckPendingMasterChanges();
    DrawMasterAirportPanel(hDC);

    // Draw blocks panel if visible
    if (showBlocksPanel) {
        RefreshBlocksData();
        DrawBlocksPanel(hDC);
    }

    static auto lastCallTime = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastCallTime).count();
    if (elapsed >= 30) {
        lastCallTime = now;
        cdm->fetchRelevantFlights();
    }
}

void CDMScreen::ToggleMasterAirport(const std::string& icao) {
    std::vector<std::string> masterAirports = cdm->getMasterAirports();
    if (std::find(masterAirports.begin(), masterAirports.end(), icao) != masterAirports.end()) {
        cdm->clearMasterAirport(icao);
        MarkAirportPending(icao, false);
    } else {
        cdm->addMasterAirport(icao);
        MarkAirportPending(icao, true);
    }
}

void CDMScreen::OnClickScreenObject(int ObjectType, const char* sObjectId, POINT Pt, RECT Area, int Button) {
    // Flights: checkbox (informed)
    if (strcmp(sObjectId, "FLT_CHK_ALL") == 0) {
        showAtfcmAllFlights = !showAtfcmAllFlights;
        RequestRefresh();
        return;
    }

    // Flights: checkbox (CDM / non-CDM)
    if (strcmp(sObjectId, "FLT_CHK_CDM") == 0) {
        showAtfcmAllCdmFlights = !showAtfcmAllCdmFlights;
        RequestRefresh();
        return;
    }

    // Flights: filter edit
    if (strcmp(sObjectId, "FLT_FILTER") == 0) {
        cdm->OpenPopupEdit(Area, TAG_FUNC_RELEVANT_FLIGHTS_FILTER, cdm->getFilterFlightsText().c_str());
        return;
    }

    // Flights: column click -> sort (keep existing behavior)
    if (strncmp(sObjectId, "FLT_COL_", 8) == 0) {
        int col = atoi(sObjectId + 8);
        if (sortColumn == col)
            sortAscending = !sortAscending;
        else {
            sortColumn = col;
            sortAscending = true;
        }
        RequestRefresh();
        return;
    }

    // NEW: clicking EXCL / REA / SIR per-row cells calls a function (like SEND)
    if (strncmp(sObjectId, "FLT_EXCL_", 9) == 0) {
        int idx = atoi(sObjectId + 9);
        auto flights = GetFilteredRelevantFlightsRows();
        if (idx >= 0 && idx < (int)flights.size()) {
            cdm->setCdmServerStatusFromDialog(flights[(size_t)idx], "EXCL");
        }
        return;
    }
    if (strncmp(sObjectId, "FLT_REA_", 8) == 0) {
        int idx = atoi(sObjectId + 8);
        auto flights = GetFilteredRelevantFlightsRows();
        if (idx >= 0 && idx < (int)flights.size()) {
            cdm->setCdmServerStatusFromDialog(flights[(size_t)idx], "REA");
        }
        return;
    }
    if (strncmp(sObjectId, "FLT_SIR_", 8) == 0) {
        int idx = atoi(sObjectId + 8);
        auto flights = GetFilteredRelevantFlightsRows();
        if (idx >= 0 && idx < (int)flights.size()) {
            cdm->setCdmServerStatusFromDialog(flights[(size_t)idx], "SIR");
        }
        return;
    }
    if (strncmp(sObjectId, "FLT_SWM_", 8) == 0) {
        int idx = atoi(sObjectId + 8);
        auto flights = GetFilteredRelevantFlightsRows();
        if (idx >= 0 && idx < (int)flights.size()) {
            cdm->setCdmServerStatusFromDialog(flights[(size_t)idx], "SWM");
        }
        return;
    }

    // Flights: SEND
    if (strncmp(sObjectId, "FLT_ACT_", 8) == 0) {
        int idx = atoi(sObjectId + 8);
        auto flights = GetFilteredRelevantFlightsRows();
        if (idx >= 0 && idx < (int)flights.size()) {
            cdm->sendAtfcmPrivateMessageToPilotCon(flights[(size_t)idx]);
        }
        return;
    }

    // Existing airport panel
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

        if (pendingMasterChanges.find(apt) != pendingMasterChanges.end()) {
            return;
        }

        extern bool bmiMode;

        // Check for right-click to show blocks
        if (Button == 2) {  // Right-click (Button 2 = VK_RBUTTON)
            if (bmiMode) {
                ShowBlocksWindow(apt);
            }
            return;
        }

        // Left-click: toggle connection (existing behavior)
        ToggleMasterAirport(apt);
        return;
    }

    // Blocks panel interactions
    if (strcmp(sObjectId, "BLOCKS_CLOSE") == 0) {
        HideBlocksWindow();
        return;
    }

    // Handle runway filter buttons
    if (strncmp(sObjectId, "BLOCKS_RWY_", 11) == 0) {
        std::string runway = std::string(sObjectId + 11);
        SetRunwayFilter(runway);
        return;
    }

    // Handle block cell clicks for capacity adjustment and callsigns display
    if (strncmp(sObjectId, "BLOCKS_CELL_", 12) == 0) {
        std::string cellStr = std::string(sObjectId + 12);
        
        // Format: runway_blockindex
        size_t underscorePos = cellStr.rfind('_');
        if (underscorePos != std::string::npos) {
            std::string runway = cellStr.substr(0, underscorePos);
            int blockIndex = std::atoi(cellStr.substr(underscorePos + 1).c_str());
            
            auto key = std::make_pair(runway, blockIndex);
            
            // Right-click: toggle callsigns display for this specific runway/block
            if (Button == 2) {
                if (selectedBlockIndex == blockIndex && selectedBlockRunway == runway) {
                    // Deselect if clicking same cell again
                    selectedBlockIndex = -1;
                    selectedBlockRunway = "";
                } else {
                    // Select this runway/block
                    selectedBlockRunway = runway;
                    selectedBlockIndex = blockIndex;
                }
                RequestRefresh();
                return;
            }
            
            // Check if user is master of the selected airport before allowing capacity modifications
            if (Button == 1 || Button == 3) {
                auto masterAirports = cdm->getMasterAirports();
                bool isMaster = std::find(masterAirports.begin(), masterAirports.end(), selectedAirportForBlocks) != masterAirports.end();
                
                if (!isMaster) {
                    // Not master, no capacity modifications allowed
                    return;
                }
            }
            
            // Get current capacity: pending > custom > calculated (same priority as display)
            int currentCapacity = 0;
            
            // First check pending changes (highest priority)
            bool foundPending = false;
            for (const auto& pending : pendingBlockChanges) {
                if (pending.runway == runway && pending.blockIndex == blockIndex) {
                    currentCapacity = pending.newCapacity;
                    foundPending = true;
                    break;
                }
            }
            
            // If no pending change, check custom overrides, then calculated baseline
            if (!foundPending) {
                if (customBlockCapacities.find(key) != customBlockCapacities.end()) {
                    currentCapacity = customBlockCapacities[key];
                } else if (calculatedBlockCapacities.find(key) != calculatedBlockCapacities.end()) {
                    currentCapacity = calculatedBlockCapacities[key];
                }
            }
            
            // Left-click: decrease capacity (allow reducing more than default)
            if (Button == 1) {
                int newCapacity = currentCapacity - 1;
                
                // Prevent going below 0
                if (newCapacity < 0) {
                    newCapacity = 0;
                }
                
                // Check if this change already exists in pending list
                bool found = false;
                for (auto& pending : pendingBlockChanges) {
                    if (pending.runway == runway && pending.blockIndex == blockIndex) {
                        pending.newCapacity = newCapacity;
                        found = true;
                        break;
                    }
                }
                
                // If not found, add new pending change
                if (!found) {
                    pendingBlockChanges.push_back({runway, blockIndex, newCapacity, currentCapacity});
                }
                
                RequestRefresh();
                return;
            }
            
            // Middle-click: increase capacity
            if (Button == 3) {
                int newCapacity = currentCapacity + 1;
                
                // Check if this change already exists in pending list
                bool found = false;
                for (auto& pending : pendingBlockChanges) {
                    if (pending.runway == runway && pending.blockIndex == blockIndex) {
                        pending.newCapacity = newCapacity;
                        found = true;
                        break;
                    }
                }
                
                // If not found, add new pending change
                if (!found) {
                    pendingBlockChanges.push_back({runway, blockIndex, newCapacity, currentCapacity});
                }
                
                RequestRefresh();
                return;
            }
        }
        return;
    }

    // Handle APPLY button click
    if (strcmp(sObjectId, "BLOCKS_APPLY") == 0) {
        ApplyPendingBlockChanges();
        RequestRefresh();
        return;
    }

    // Handle REVERT button click
    if (strcmp(sObjectId, "BLOCKS_REVERT") == 0) {
        RevertPendingBlockChanges();
        RequestRefresh();
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

    // Drag flights panel by its header
    if (strcmp(sObjectId, "FLT_HDR") == 0) {
        flightsPanelPos.x = Area.left;
        flightsPanelPos.y = Area.top;
        RequestRefresh();
    }

    // Drag blocks panel by its header
    if (strcmp(sObjectId, "BLOCKS_HDR") == 0) {
        blocksPanelPos.x = Area.left;
        blocksPanelPos.y = Area.top;
        RequestRefresh();
    }
}

void CDMScreen::OnMouseMove(POINT Pt) {
    // Optional: Add any mouse move handlers if needed in the future
}

// ===========================
// BLOCKS PANEL IMPLEMENTATION
// ===========================

void CDMScreen::ShowBlocksWindow(const std::string& airport) {
    if (!cdm) return;

    // Check if BMI mode is enabled
    extern bool bmiMode;
    if (!bmiMode) {
        return;  // BMI must be enabled to show blocks
    }

    selectedAirportForBlocks = airport;
    selectedRunwayFilter = "";  // Reset filter when opening
    showBlocksPanel = true;
    UpdateBlocksData(airport);
    RequestRefresh();
}

void CDMScreen::HideBlocksWindow() {
    showBlocksPanel = false;
    selectedAirportForBlocks = "";
    selectedRunwayFilter = "";
    currentBlocksData.clear();
    RequestRefresh();
}

void CDMScreen::UpdateBlocksData(const std::string& airport) {
    if (!cdm) {
        currentBlocksData.clear();
        return;
    }

    currentBlocksData.clear();

    const int windowMinutes = 10;
    const int windowsPerHour = 60 / windowMinutes;

    // Get current time to determine window hours
    std::string nowStr = cdm->GetTimeNow();
    int nowHour = 0, nowMin = 0;
    if (nowStr.length() >= 4) {
        try {
            nowHour = std::stoi(nowStr.substr(0, 2));
            nowMin = std::stoi(nowStr.substr(2, 2));
        } catch (...) {
            return;
        }
    }

    std::set<std::string> uniqueRunways;
    std::map<std::pair<std::string, int>, int> blockOccupancy;

    // First pass: collect runways and occupancy
    for (const auto& plane : slotList) {
        // Skip if no TTOT assigned
        if (plane.ttot.empty() || plane.ttot.length() < 4) continue;

        // Get the flight plan directly by callsign
        CFlightPlan fp = cdm->FlightPlanSelect(plane.callsign.c_str());
        if (!fp.IsValid()) continue;

        CFlightPlanData fpData = fp.GetFlightPlanData();
        
        // Check if this flight departs from our selected airport
        if (std::string(fpData.GetOrigin()) == airport) {
            std::string runway = fpData.GetDepartureRwy();
            if (runway.empty()) runway = "UNK";

            // Parse TTOT to determine static window block
            try {
                int tobtMin = std::stoi(plane.ttot.substr(2, 2));
                int blockIndex = tobtMin / windowMinutes;
                if (blockIndex >= 0 && blockIndex < 6) {
                    uniqueRunways.insert(runway);
                    auto key = std::make_pair(runway, blockIndex);
                    blockOccupancy[key]++;
                }
            } catch (...) {}
        }
    }

    // Create block data structure with per-runway capacity calculation
    for (const auto& runway : uniqueRunways) {
        // Get hourly rate for this specific runway (with fallback to config default)
        int hourlyRate = cdm->getHourlyRateForRunway(airport, runway.c_str());

        // Calculate block capacities for this runway with evenly spaced remainder allocation
        const int baseCap = hourlyRate / windowsPerHour;      // e.g. 40/6 = 6
        const int remainder = hourlyRate % windowsPerHour;    // e.g. 40%6 = 4

        std::vector<int> blockCapacities(6);
        for (int i = 0; i < 6; i++) {
            blockCapacities[i] = baseCap;
        }

        // Distribute remainder evenly across blocks
        if (remainder > 0) {
            double spacing = static_cast<double>(windowsPerHour) / remainder;  // e.g., 6/4 = 1.5
            for (int i = 0; i < remainder; i++) {
                int blockIdx = static_cast<int>(i * spacing);  // 0, 1, 3, 4 for spacing=1.5
                if (blockIdx < 6) {
                    blockCapacities[blockIdx]++;
                }
            }
        }

        // Create BlockData for each block of this runway
        for (int block = 0; block < 6; block++) {
            int blockStartMin = block * windowMinutes;
            int blockEndMin = blockStartMin + windowMinutes - 1;
            
            // Calculate hour for this block based on current time
            int displayHour = nowHour;
            int displayStartMin = blockStartMin;
            int displayEndMin = blockEndMin;
            
            // If we're past this block's minute range, advance to next hour
            if (displayStartMin < nowMin && displayEndMin < nowMin) {
                displayHour = nowHour + 1;
                if (displayHour >= 24) displayHour -= 24;
            }

            char timeRange[20];
            sprintf_s(timeRange, "%02d:%02d-%02d:%02d", displayHour, displayStartMin, displayHour, displayEndMin);

            auto key = std::make_pair(runway, block);
            int calculatedCap = blockCapacities[block];
            
            // Store calculated capacity and check for custom override
            calculatedBlockCapacities[key] = calculatedCap;
            int finalCapacity = calculatedCap;
            if (customBlockCapacities.find(key) != customBlockCapacities.end()) {
                finalCapacity = customBlockCapacities[key];
            }

            BlockData bd;
            bd.runway = runway;
            bd.blockIndex = block;
            bd.capacity = finalCapacity;  // Use custom override if available, else calculated
            bd.occupancy = blockOccupancy[key];
            bd.timeRange = timeRange;

            currentBlocksData.push_back(bd);
        }
    }
}

void CDMScreen::RefreshBlocksData() {
    if (showBlocksPanel && !selectedAirportForBlocks.empty()) {
        // Debounce: only update if at least 500ms has passed since last update
        // This prevents oscillation from rapid changes
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastBlocksDataUpdate);
        
        if (elapsed.count() >= 500) {
            UpdateBlocksData(selectedAirportForBlocks);
            lastBlocksDataUpdate = now;
        }
    }
}

void CDMScreen::SetRunwayFilter(const std::string& runway) {
    if (selectedRunwayFilter == runway) {
        selectedRunwayFilter = "";  // Toggle off if same runway clicked
    } else {
        selectedRunwayFilter = runway;
    }
    RequestRefresh();
}

#define BLOCKS_PANEL_WIDTH 600
#define BLOCKS_PANEL_HEIGHT 185
#define BLOCKS_HEADER_HEIGHT 20
#define BLOCKS_ROW_HEIGHT 24

void CDMScreen::DrawBlocksPanel(HDC hDC) {
    if (!showBlocksPanel || selectedAirportForBlocks.empty()) {
        return;
    }

    extern bool bmiMode;
    if (!bmiMode) {
        showBlocksPanel = false;
        return;
    }

    blocksPanelRect = RECT{blocksPanelPos.x, blocksPanelPos.y, blocksPanelPos.x + BLOCKS_PANEL_WIDTH,
                           blocksPanelPos.y + BLOCKS_PANEL_HEIGHT};

    RECT headerRect = {blocksPanelRect.left, blocksPanelRect.top, blocksPanelRect.right,
                       blocksPanelRect.top + BLOCKS_HEADER_HEIGHT};

    AddScreenObject(RADARSCR_OBJECT_CUSTOM, "BLOCKS_HDR", headerRect, true, NULL);

    DrawRoundedRect(hDC, blocksPanelRect, RGB(70, 85, 100));
    DrawRoundedRect(hDC, headerRect, RGB(45, 65, 100));

    SetBkMode(hDC, TRANSPARENT);
    SetTextColor(hDC, RGB(255, 255, 255));

    char headerText[64];
    sprintf_s(headerText, "Blocks - %s (Next 60 min)", selectedAirportForBlocks.c_str());
    DrawTextA(hDC, headerText, -1, &headerRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    // Close button
    RECT closeBtn = headerRect;
    closeBtn.left = closeBtn.right - 25;
    AddScreenObject(RADARSCR_OBJECT_CUSTOM, "BLOCKS_CLOSE", closeBtn, true, NULL);
    DrawRoundedRect(hDC, closeBtn, RGB(180, 100, 100), RGB(80, 40, 40));
    SetTextColor(hDC, RGB(255, 255, 255));
    DrawTextA(hDC, "X", -1, &closeBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    // Draw APPLY button if there are pending changes
    if (!pendingBlockChanges.empty()) {
        RECT applyBtn = headerRect;
        applyBtn.right = applyBtn.right - 30;
        applyBtn.left = applyBtn.right - 80;
        AddScreenObject(RADARSCR_OBJECT_CUSTOM, "BLOCKS_APPLY", applyBtn, true, NULL);
        DrawRoundedRect(hDC, applyBtn, RGB(100, 180, 100), RGB(40, 80, 40));
        SetTextColor(hDC, RGB(255, 255, 255));
        DrawTextA(hDC, "APPLY", -1, &applyBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        blocksPanelApplyBtnRect = applyBtn;

        // Draw REVERT button next to APPLY
        RECT revertBtn = headerRect;
        revertBtn.right = applyBtn.left - 5;
        revertBtn.left = revertBtn.right - 75;
        AddScreenObject(RADARSCR_OBJECT_CUSTOM, "BLOCKS_REVERT", revertBtn, true, NULL);
        DrawRoundedRect(hDC, revertBtn, RGB(180, 100, 100), RGB(80, 40, 40));
        SetTextColor(hDC, RGB(255, 255, 255));
        DrawTextA(hDC, "REVERT", -1, &revertBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }

    // Group blocks by runway
    std::map<std::string, std::vector<BlockData>> blocksByRunway;
    for (const auto& block : currentBlocksData) {
        blocksByRunway[block.runway].push_back(block);
    }

    // Get sorted list of runways
    std::vector<std::string> runways;
    for (const auto& rwyPair : blocksByRunway) {
        if (selectedRunwayFilter.empty() || rwyPair.first == selectedRunwayFilter) {
            runways.push_back(rwyPair.first);
        }
    }
    std::sort(runways.begin(), runways.end());

    if (runways.empty()) {
        // No runways to display
        RECT noDataRect = {blocksPanelRect.left + 10, blocksPanelRect.top + BLOCKS_HEADER_HEIGHT + 20, 
                          blocksPanelRect.right - 10, blocksPanelRect.top + BLOCKS_HEADER_HEIGHT + 40};
        DrawCellTextA_Flt(hDC, "No departures in next 60 min", noDataRect, DT_LEFT, RGB(200, 200, 200));
        return;
    }

    // Column layout: time blocks as rows, runways as columns
    int colWidth = (blocksPanelRect.right - blocksPanelRect.left - 100) / (int)runways.size();
    if (colWidth < 60) colWidth = 60;

    int yPos = blocksPanelRect.top + BLOCKS_HEADER_HEIGHT + 8;
    int rowHeight = 20;

    // Draw column headers (runway names)
    RECT timeHeaderRect = {blocksPanelRect.left + 8, yPos, blocksPanelRect.left + 100, yPos + rowHeight};
    DrawCellTextA_Flt(hDC, "Time", timeHeaderRect, DT_CENTER, RGB(200, 200, 255));

    int xPos = blocksPanelRect.left + 100;
    for (const auto& runway : runways) {
        RECT rwyHeaderRect = {xPos, yPos, xPos + colWidth, yPos + rowHeight};
        DrawRoundedRect(hDC, rwyHeaderRect, RGB(45, 65, 100), RGB(20, 30, 60));
        DrawCellTextA_Flt(hDC, runway, rwyHeaderRect, DT_CENTER, RGB(200, 255, 200));
        xPos += colWidth;
    }

    yPos += rowHeight + 4;

    // Get current time to determine which block to start from (closest first)
    std::string nowStr = cdm->GetTimeNow();
    int nowMin = 0;
    if (nowStr.length() >= 4) {
        try {
            nowMin = std::stoi(nowStr.substr(2, 2));
        } catch (...) {}
    }
    
    const int windowMinutes = 10;
    int startBlockIdx = nowMin / windowMinutes;
    if (startBlockIdx >= 6) startBlockIdx = 0;

    // Create ordered block indices: start from closest block, then cycle through
    std::vector<int> blockOrder;
    for (int i = 0; i < 6; i++) {
        blockOrder.push_back((startBlockIdx + i) % 6);
    }

    // Draw time blocks as rows (ordered by closest first)
    for (int orderIdx = 0; orderIdx < 6; orderIdx++) {
        int blockIdx = blockOrder[orderIdx];
        RECT timeRect = {blocksPanelRect.left + 8, yPos, blocksPanelRect.left + 100, yPos + rowHeight};
        
        // Get time range from first block (all blocks have same time for same blockIdx)
        std::string timeRange = "N/A";
        for (const auto& rwyPair : blocksByRunway) {
            for (const auto& block : rwyPair.second) {
                if (block.blockIndex == blockIdx) {
                    timeRange = block.timeRange;
                    break;
                }
            }
            if (timeRange != "N/A") break;
        }

        COLORREF timeBg = (blockIdx % 2 == 0) ? RGB(60, 75, 90) : RGB(70, 85, 100);
        DrawRoundedRect(hDC, timeRect, timeBg, RGB(30, 40, 60));
        DrawCellTextA_Flt(hDC, timeRange, timeRect, DT_CENTER, RGB(255, 255, 255));

        xPos = blocksPanelRect.left + 100;

        // Draw occupancy cells for each runway
        for (const auto& runway : runways) {
            RECT cellRect = {xPos, yPos, xPos + colWidth, yPos + rowHeight};
            
            // Create unique ID for this cell: BLOCKS_CELL_runway_blockindex
            std::string cellId = "BLOCKS_CELL_" + runway + "_" + std::to_string(blockIdx);
            AddScreenObject(RADARSCR_OBJECT_CUSTOM, cellId.c_str(), cellRect, true, NULL);

            int occupancy = 0;
            int capacity = 6;
            
            // Find occupancy and base capacity for this runway and block
            auto it = blocksByRunway.find(runway);
            if (it != blocksByRunway.end()) {
                for (const auto& block : it->second) {
                    if (block.blockIndex == blockIdx) {
                        occupancy = block.occupancy;
                        capacity = block.capacity;  // Base capacity from calculations
                        break;
                    }
                }
            }

            // Check for pending changes for this cell (highest priority)
            bool hasPendingChange = false;
            auto key = std::make_pair(runway, blockIdx);
            bool foundPending = false;
            for (const auto& pending : pendingBlockChanges) {
                if (pending.runway == runway && pending.blockIndex == blockIdx) {
                    capacity = pending.newCapacity;
                    hasPendingChange = true;
                    foundPending = true;
                    break;
                }
            }

            // If no pending change, check for custom applied capacity
            if (!foundPending && customBlockCapacities.find(key) != customBlockCapacities.end()) {
                capacity = customBlockCapacities[key];
            }

            int percentage = (capacity > 0) ? (occupancy * 100) / capacity : 0;
            if (percentage > 100) percentage = 100;

            // Color based on occupancy percentage
            COLORREF occColor = RGB(0, 200, 0);  // Green
            if (percentage > 75) {
                occColor = RGB(255, 0, 0);  // Red
            } else if (percentage > 50) {
                occColor = RGB(255, 165, 0);  // Orange
            } else if (percentage > 25) {
                occColor = RGB(200, 200, 0);  // Yellow
            }

            COLORREF bgColor = (blockIdx % 2 == 0) ? RGB(60, 75, 90) : RGB(70, 85, 100);
            // Highlight cells with pending changes with a brighter border
            COLORREF borderColor = hasPendingChange ? RGB(100, 200, 255) : RGB(30, 40, 60);
            DrawRoundedRect(hDC, cellRect, bgColor, borderColor);

            // Draw occupancy bar
            int barWidth = colWidth - 8;
            int barHeight = 12;
            int barX = xPos + 4;
            int barY = yPos + 2;

            RECT barBg = {barX, barY, barX + barWidth, barY + barHeight};
            DrawRoundedRect(hDC, barBg, RGB(40, 40, 40), RGB(80, 80, 80));

            // Draw occupancy fill
            int fillWidth = (barWidth * percentage) / 100;
            if (fillWidth > 0) {
                RECT barFill = {barX, barY, barX + fillWidth, barY + barHeight};
                DrawRoundedRect(hDC, barFill, occColor, occColor);
            }

            // Draw percentage text centered in cell
            char percentText[32];
            sprintf_s(percentText, "%d/%d", occupancy, capacity);
            RECT percentRect = {xPos + 2, yPos + 2, xPos + colWidth - 2, yPos + rowHeight - 2};
            SetTextColor(hDC, hasPendingChange ? RGB(100, 200, 255) : RGB(255, 255, 255));  // Highlight pending text color
            DrawCellTextA_Flt(hDC, percentText, percentRect, DT_CENTER, hasPendingChange ? RGB(100, 200, 255) : RGB(255, 255, 255));

            xPos += colWidth;
        }

        yPos += rowHeight + 2;
    }
    
    // Draw callsigns list for selected block
    if (selectedBlockIndex >= 0 && !selectedBlockRunway.empty()) {
        auto callsigns = GetCallsignsForBlock(selectedBlockRunway, selectedBlockIndex);
        
        if (!callsigns.empty()) {
            int listYPos = yPos + 10;
            int listHeight = callsigns.size() * 15 + 25;
            RECT listHeaderRect = {blocksPanelRect.left, listYPos, blocksPanelRect.right, listYPos + 20};
            
            DrawRoundedRect(hDC, listHeaderRect, RGB(45, 65, 100), RGB(20, 30, 60));
            char headerText[64];
            sprintf_s(headerText, "Block %d (Runway %s) - %d flights", selectedBlockIndex, selectedBlockRunway.c_str(), (int)callsigns.size());
            DrawCellTextA_Flt(hDC, headerText, listHeaderRect, DT_LEFT, RGB(200, 255, 200));
            
            listYPos += 22;
            for (const auto& item : callsigns) {
                RECT itemRect = {blocksPanelRect.left + 10, listYPos, blocksPanelRect.right - 10, listYPos + 15};
                char itemText[64];
                sprintf_s(itemText, "%s - %s", item.first.c_str(), item.second.c_str());
                DrawCellTextA_Flt(hDC, itemText, itemRect, DT_LEFT, RGB(220, 220, 220));
                listYPos += 15;
            }
        }
    }
}

void CDMScreen::ApplyPendingBlockChanges() {
    // Apply all pending block capacity changes
    for (const auto& pending : pendingBlockChanges) {
        auto key = std::make_pair(pending.runway, pending.blockIndex);
        customBlockCapacities[key] = pending.newCapacity;
        cdm->setCustomBlockCapacity(pending.runway, pending.blockIndex, pending.newCapacity);
    }
    
    // Clear pending changes
    pendingBlockChanges.clear();
    
    // Force immediate update by resetting the debounce timer
    lastBlocksDataUpdate = std::chrono::steady_clock::now() - std::chrono::milliseconds(500);
}

void CDMScreen::RevertPendingBlockChanges() {
    // Clear pending changes without applying them
    pendingBlockChanges.clear();
}

std::vector<std::pair<std::string, std::string>> CDMScreen::GetCallsignsForBlock(const std::string& runway, int blockIndex) {
    std::vector<std::pair<std::string, std::string>> result;
    
    if (!cdm) return result;
    
    // Calculate time range for the block (10-minute blocks)
    int startMin = blockIndex * 10;
    int endMin = startMin + 10;
    
    // Get external slotList
    extern std::vector<Plane> slotList;
    
    // Iterate through slotList and find flights in this block
    for (const auto& plane : slotList) {
        // Parse TTOT to get minutes
        if (plane.ttot.length() >= 4) {
            try {
                int ttotMin = std::stoi(plane.ttot.substr(2, 2));
                
                // Check if TTOT falls in this block's time range
                if (ttotMin >= startMin && ttotMin < endMin) {
                    // Get flight plan to check runway
                    CFlightPlan fp = cdm->FlightPlanSelect(plane.callsign.c_str());
                    if (fp.IsValid()) {
                        CFlightPlanData fpData = fp.GetFlightPlanData();
                        std::string planeRunway = fpData.GetDepartureRwy();
                        if (planeRunway.empty()) planeRunway = "UNK";
                        
                        // Check if runway matches (if filtering by runway)
                        if (runway.empty() || planeRunway == runway) {
                            result.push_back({plane.callsign, plane.ttot});
                        }
                    }
                }
            } catch (...) {}
        }
    }
    
    // Sort by TTOT
    std::sort(result.begin(), result.end(), 
        [](const auto& a, const auto& b) { return a.second < b.second; });
    
    return result;
}

std::vector<std::string> CDMScreen::GetBlockRunways() const {
    std::set<std::string> runwaySet;
    
    if (!cdm) return std::vector<std::string>();
    
    extern std::vector<Plane> slotList;
    
    for (const auto& plane : slotList) {
        if (plane.ttot.empty() || plane.ttot.length() < 4) continue;
        
        CFlightPlan fp = cdm->FlightPlanSelect(plane.callsign.c_str());
        if (!fp.IsValid()) continue;
        
        CFlightPlanData fpData = fp.GetFlightPlanData();
        if (std::string(fpData.GetOrigin()) == selectedAirportForBlocks) {
            std::string runway = fpData.GetDepartureRwy();
            if (runway.empty()) runway = "UNK";
            runwaySet.insert(runway);
        }
    }
    
    return std::vector<std::string>(runwaySet.begin(), runwaySet.end());
}