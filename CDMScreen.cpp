#include <windows.h>
#include "CDMScreen.h"
#include "CDMSingle.hpp"
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>
#include <map>
#include <cctype>

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
#define FLT_PANEL_WIDTH 840
#define FLT_HEADER_HEIGHT 16
#define FLT_ROW_HEIGHT 18
#define FLT_ROW_GAP 2
#define FLT_MAX_ROWS 500

static std::string ToLowerCopy(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return (char)std::tolower(c); });
    return s;
}

static bool ContainsCI(const std::string& hay, const std::string& needle) {
    if (needle.empty()) return true;
    return ToLowerCopy(hay).find(ToLowerCopy(needle)) != std::string::npos;
}

static bool IsTrueString(const std::string& s)
{
    std::string v = ToLowerCopy(s);
    v.erase(std::remove_if(v.begin(), v.end(), [](unsigned char c) { return std::isspace(c); }), v.end());
    return (v == "true" || v == "1" || v == "yes" || v == "y" || v == "ok");
}

// renamed to avoid collision with anything else in your project
static void DrawCellTextA_Flt(HDC hDC, const std::string& s, RECT r, UINT fmt = DT_LEFT, COLORREF color = RGB(255, 255, 255))
{
    SetBkMode(hDC, TRANSPARENT);
    SetTextColor(hDC, color);
    DrawTextA(hDC, s.c_str(), -1, &r,
        fmt | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
}

static std::string GetColSafe(const std::vector<std::string>& row, int idx)
{
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

static COLORREF DimColor(COLORREF c, float factor)
{
    BYTE r = (BYTE)(GetRValue(c) * factor);
    BYTE g = (BYTE)(GetGValue(c) * factor);
    BYTE b = (BYTE)(GetBValue(c) * factor);
    return RGB(r, g, b);
}

static void DrawSmallTextQuality(HDC hDC, const char* text, RECT rect, COLORREF color)
{
    if (!hDC || !text || !*text)
        return;

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

    if (wlen <= 0)
    {
        usedCP = CP_ACP;
        wlen = MultiByteToWideChar(CP_ACP, 0, text, -1, nullptr, 0);
    }

    if (wlen > 0)
    {
        wtext.resize((size_t)wlen);
        MultiByteToWideChar(usedCP,
            (usedCP == CP_UTF8) ? MB_ERR_INVALID_CHARS : 0,
            text,
            -1,
            &wtext[0],
            wlen);

        if (!wtext.empty() && wtext.back() == L'\0')
            wtext.pop_back();
    }

    if (!wtext.empty())
    {
        SIZE sz{};
        GetTextExtentPoint32W(hDC, wtext.c_str(), (int)wtext.length(), &sz);

        int x = rect.left + ((rect.right - rect.left) - sz.cx) / 2;
        int y = rect.top + ((rect.bottom - rect.top) - sz.cy) / 2;

        SetTextAlign(hDC, TA_LEFT | TA_TOP);

        ExtTextOutW(hDC, x, y, ETO_CLIPPED, &rect,
            wtext.c_str(),
            (UINT)wtext.length(),
            nullptr);
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

    flightsPanelPos = { 40, 640 };
    flightsPanelRect = { 0,0,0,0 };
    flightsHeaderRect = { 0,0,0,0 };
    flightsFilterRect = { 0,0,0,0 };

    // state
    showAtfcmAllFlights = false;     // informed filter
    showAtfcmAllCdmFlights = true;   // NEW: true=all (CDM+non-CDM), false=only non-CDM

    // column click sorting
    sortColumn = -1;
    sortAscending = true;
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
    pendingMasterChanges[icao] = PendingMasterChange{ std::chrono::steady_clock::now(), adding };
    RequestRefresh();
}

// ------------------------------
// Flights Panel
// Data layout (server parsing):
//  0 callsign, 1 dep, 2 arr, 3 eobt, 4 tobt, 5 taxi, 6 ctot, 7 aobt,
//  8 eta, 9 mostPenalizingAirspace, 10 atfcmStatus, 11 informed, 12 isCdm,
//  13 isExcluded, 14 isRea, 15 isSir
// ------------------------------
std::vector<std::vector<std::string>> CDMScreen::GetFilteredRelevantFlightsRows() const
{
    if (!cdm) return {};
    const auto& all = cdm->returnRelevantFlights();

    std::vector<std::vector<std::string>> out;
    out.reserve(all.size());

    for (const auto& row : all) {
        // checkbox #1: if showAll==false -> only non-informed
        if (!showAtfcmAllFlights) {
            bool informed = IsTrueString(GetColSafe(row, 11));
            if (informed)
                continue;
        }

        // checkbox #2: if showAllCdm==false -> only non-CDM
        if (!showAtfcmAllCdmFlights) {
            bool isCdm = IsTrueString(GetColSafe(row, 12));
            if (isCdm)
                continue;
        }

        std::string flightsFilterText = cdm->getFilterFlightsText();

        // text filter: match any column
        if (!flightsFilterText.empty()) {
            bool any = false;
            for (const auto& cell : row) {
                if (ContainsCI(cell, flightsFilterText)) { any = true; break; }
            }
            if (!any)
                continue;
        }

        out.push_back(row);
    }

    // sorting by clicked column
    if (sortColumn >= 0) {
        std::stable_sort(out.begin(), out.end(),
            [&](const std::vector<std::string>& a, const std::vector<std::string>& b)
            {
                std::string av = GetColSafe(a, sortColumn);
                std::string bv = GetColSafe(b, sortColumn);
                int cmp = _stricmp(av.c_str(), bv.c_str());
                if (sortAscending) return cmp < 0;
                return cmp > 0;
            });
    }

    return out;
}

void CDMScreen::DrawRelevantFlightsPanel(HDC hDC)
{
    if (!cdm) return;
    if (!cdm->getAtfcmList()) return;

    std::string flightsFilterText = cdm->getFilterFlightsText();
    auto flights = GetFilteredRelevantFlightsRows();

    int rows = (int)std::min<size_t>(flights.size(), FLT_MAX_ROWS);
    int bodyH = rows * FLT_ROW_HEIGHT + (rows > 0 ? (rows - 1) * FLT_ROW_GAP : 0);

    // header + gap + checkbox row + gap + checkbox row + gap + filter row + gap + column header + gap + list + gap
    int panelH = FLT_HEADER_HEIGHT + 6 + 18 + 6 + 18 + 6 + 18 + 6 + 18 + 6 + bodyH + 6;

    flightsPanelRect = RECT{ flightsPanelPos.x, flightsPanelPos.y, flightsPanelPos.x + FLT_PANEL_WIDTH, flightsPanelPos.y + panelH };
    flightsHeaderRect = RECT{ flightsPanelRect.left, flightsPanelRect.top, flightsPanelRect.right, flightsPanelRect.top + FLT_HEADER_HEIGHT };

    AddScreenObject(RADARSCR_OBJECT_CUSTOM, "FLT_HDR", flightsHeaderRect, true, NULL);

    DrawRoundedRect(hDC, flightsPanelRect, RGB(70, 70, 85));
    DrawRoundedRect(hDC, flightsHeaderRect, RGB(45, 45, 105));
    SetBkMode(hDC, TRANSPARENT);
    SetTextColor(hDC, RGB(245, 245, 255));
    DrawTextA(hDC, "ATFCM Flight List", -1, &flightsHeaderRect,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

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

    DrawCellTextA_Flt(hDC, "Show CDM AND non-CDM).", chkLabel2, DT_LEFT, RGB(255, 255, 255));

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
        RECT r{ x, colHdr.top, x + w, colHdr.bottom };
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

    addHeader("FLT_SEND_HDR", "", 45, true);

    // Rows
    int y = colHdr.bottom + 6;

    // Helper: draw centered green/red square instead of text
    auto boolFill = [](bool v) { return v ? RGB(0, 170, 0) : RGB(220, 0, 0); };

    auto drawBoolSquare = [&](RECT r, bool value)
        {
            int cellW = (r.right - r.left);
            int cellH = (r.bottom - r.top);

            int size = min(cellW, cellH) - 6; // padding
            if (size < 6) size = min(cellW, cellH);

            int left = r.left + (cellW - size) / 2;
            int top = r.top + (cellH - size) / 2;

            RECT sq{ left, top, left + size, top + size };

            // Using DrawRoundedRect with small radius still looks like a square here
            DrawRoundedRect(hDC, sq, boolFill(value), RGB(30, 30, 30));
        };

    for (int i = 0; i < rows; i++) {
        RECT rowRect{ flightsPanelRect.left + 6, y, flightsPanelRect.right - 6, y + FLT_ROW_HEIGHT };

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
        auto cell = [&](int w) { RECT r{ cx, rowRect.top, cx + w, rowRect.bottom }; cx += w; return r; };

        bool isCdm = IsTrueString(GetColSafe(row, 12));
        COLORREF callsignColor = isCdm ? RGB(100, 255, 100) : RGB(255, 255, 255);

        DrawCellTextA_Flt(hDC, GetColSafe(row, 0), cell(70), DT_LEFT, callsignColor); // callsign
        DrawCellTextA_Flt(hDC, GetColSafe(row, 1), cell(55));   // dep
        DrawCellTextA_Flt(hDC, GetColSafe(row, 2), cell(55));   // arr
        DrawCellTextA_Flt(hDC, GetColSafe(row, 3), cell(50));   // eobt
        DrawCellTextA_Flt(hDC, GetColSafe(row, 4), cell(50));   // tobt
        DrawCellTextA_Flt(hDC, GetColSafe(row, 5), cell(35));   // taxi
        DrawCellTextA_Flt(hDC, GetColSafe(row, 6), cell(50));   // ctot
        DrawCellTextA_Flt(hDC, GetColSafe(row, 7), cell(50));   // aobt
        DrawCellTextA_Flt(hDC, GetColSafe(row, 8), cell(50));   // eta
        DrawCellTextA_Flt(hDC, GetColSafe(row, 9), cell(130));  // mostPenalizingAirspace
        DrawCellTextA_Flt(hDC, GetColSafe(row, 10), cell(60));  // atfcmStatus

        // NEW: EXCL/REA/SIR clickable cells with colored squares
        RECT exclRect = cell(40);
        RECT reaRect = cell(40);
        RECT sirRect = cell(40);

        char exclId[64], reaId[64], sirId[64];
        sprintf_s(exclId, "FLT_EXCL_%d", i);
        sprintf_s(reaId, "FLT_REA_%d", i);
        sprintf_s(sirId, "FLT_SIR_%d", i);

        AddScreenObject(RADARSCR_OBJECT_CUSTOM, exclId, exclRect, true, NULL);
        AddScreenObject(RADARSCR_OBJECT_CUSTOM, reaId, reaRect, true, NULL);
        AddScreenObject(RADARSCR_OBJECT_CUSTOM, sirId, sirRect, true, NULL);

        bool isExcluded = IsTrueString(GetColSafe(row, 13));
        bool isRea = IsTrueString(GetColSafe(row, 14));
        bool isSir = IsTrueString(GetColSafe(row, 15));

        drawBoolSquare(exclRect, isExcluded);
        drawBoolSquare(reaRect, isRea);
        drawBoolSquare(sirRect, isSir);

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
        int col = static_cast<int>(i % PER_ROW);
        int row = static_cast<int>(i / PER_ROW);
        int x = xStart + col * (BTN_WIDTH + BTN_GAP_X);
        int y = yStart + row * (BTN_HEIGHT + BTN_GAP_Y);

        RECT btnRect = { x, y, x + BTN_WIDTH, y + BTN_HEIGHT };
        masterAirportBtnRects[i] = btnRect;

        std::string btnId = "APTBTN_" + airports[i];
        AddScreenObject(RADARSCR_OBJECT_CUSTOM, btnId.c_str(), btnRect, true, NULL);

        COLORREF colFill;

        bool showServerPos = false;
        std::string serverPosText;

        if (pendingMasterChanges.find(airports[i]) != pendingMasterChanges.end()) {
            colFill = RGB(255, 200, 30);
        }
        else {
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
            }
            else if (isServerMaster) {
                colFill = RGB(160, 0, 160);
            }
            else {
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
        }
        else {
            DrawTextA(hDC, airports[i].c_str(), -1, &btnRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        }
    }

    for (size_t i = airports.size(); i < MAX_AIRPORTS_DISPLAYED; ++i) {
        masterAirportBtnRects[i] = { 0, 0, 0, 0 };
    }

    int plusIndex = static_cast<int>(std::min<size_t>(airports.size(), MAX_AIRPORTS_DISPLAYED));
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

    DrawRelevantFlightsPanel(hDC);

    if (!cdm || !cdm->getPanelStatus()) {
        return;
    }

    CheckPendingMasterChanges();
    DrawMasterAirportPanel(hDC);

    static auto lastCallTime = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastCallTime).count();
    if (elapsed >= 30)
    {
        lastCallTime = now;
        cdm->fetchRelevantFlights();
    }
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
        if (sortColumn == col) sortAscending = !sortAscending;
        else { sortColumn = col; sortAscending = true; }
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

    // Flights: SEND
    if (strncmp(sObjectId, "FLT_ACT_", 8) == 0) {
        int idx = atoi(sObjectId + 8);
        auto flights = GetFilteredRelevantFlightsRows();
        if (idx >= 0 && idx < (int)flights.size()) {
            cdm->sendAtfcmPrivateMessageToPilot(flights[(size_t)idx]);
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

    // Drag flights panel by its header
    if (strcmp(sObjectId, "FLT_HDR") == 0) {
        flightsPanelPos.x = Area.left;
        flightsPanelPos.y = Area.top;
        RequestRefresh();
    }
}