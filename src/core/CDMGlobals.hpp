#pragma once

// Centralized definitions for the plugin's shared state, using C++17 `inline` variables.
//
// The CDM plugin currently keeps its runtime state as free (file-scope) globals rather than as
// members of the CDM class - CDMSingle.cpp used to be the single translation unit that defined
// them, which meant splitting it into multiple .cpp files wasn't possible without some header
// declaring these symbols for the other files. This header is that point: any .cpp file extracted
// out of CDMSingle.cpp includes this header to access the shared state. Each variable is marked
// `inline` so every translation unit that includes this header shares one definition instead of
// each one allocating its own copy (which would be a linker error for a plain, non-inline global).
//
// This is intentionally NOT a redesign (no CDMState class, no encapsulation) - the goal is to keep
// the CDMSingle.cpp split mechanical and behavior-preserving. Turning these into a proper state
// object is a separate, larger effort.

#include <unordered_map>

#include "CDMSingle.hpp"
#include "src/screen/CDMScreen.h"
#include "src/models/Delay.h"

inline bool blink;
inline bool debugMode, initialSidLoad;

inline int disCount;
inline ifstream sidDatei;
inline char DllPathFile[_MAX_PATH];
inline string pfad;
inline string lfad;
inline string sfad;
inline string cfad;
inline string vfad;
inline string rfad;
inline string dfad;
inline string xfad;
inline string tfad;
inline string rateString;
inline string lvoRateString;
inline string pm_message;
inline bool defaultRate;
inline time_t countTime;
inline time_t countTimeNonCdm;
inline time_t countFetchServerTime;
inline time_t countRefreshActions4Time;
inline time_t countTfcDisconnectionTime;
inline time_t countNetworkTobt;
inline int countTfcDisconnection;
inline int refreshTime;
inline bool addTime;
inline bool lvo;
inline bool bmiMode;
inline bool eventPriorityEnabled;
inline bool reqTobtPriority;
inline bool ctotCid;
inline bool realMode;
inline bool eventMode;
inline int eventModeTime;
inline bool pilotTobt;
inline bool atotEnabled;
inline bool remarksOption;
inline bool remarksOptionCtot;
inline bool tobtReqAfterAsrtDisabledOption;
inline bool invalidateTSAT_Option;
inline bool invalidateTSAT_Option_asrt;
inline bool invalidateTOBT_Option;
inline bool readySetTsac;
inline bool sidIntervalEnabled;
inline bool readyToUpdateList;
inline bool autoSetTobtFromEvSlot;
inline string lastAddedIcao;
inline string myTimeToAdd;
inline string rateUrl;
inline string taxiZonesUrl;
inline string slotURL;
inline string cdmServerUrl;
inline string customRestrictedUrl;
inline string sidIntervalUrl;
inline string loadingText;
inline int defTaxiTime;
inline bool flashingTOBTend;
inline bool flashingTSATstart;
inline bool flashingTSATend;
inline string cdm_api;
inline string myAtcCallsign;
inline bool option_su_wait;
inline string apikey = "";
inline bool serverEnabled;
inline bool sftpConnection;
inline bool refresh1;
inline bool refresh3;
inline bool refresh4;
inline string flightsFilterText;

inline bool showPanel;
inline bool showAtfcmList;

inline CDMScreen* cs;

inline int deIceTimeL;
inline int deIceTimeM;
inline int deIceTimeH;
inline int deIceTimeJ;

inline int deIceTaxiRem1;
inline int deIceTaxiRem2;
inline int deIceTaxiRem3;
inline int deIceTaxiRem4;
inline int deIceTaxiRem5;

inline string deIceTaxiRem1Name = "";
inline string deIceTaxiRem2Name = "";
inline string deIceTaxiRem3Name = "";
inline string deIceTaxiRem4Name = "";
inline string deIceTaxiRem5Name = "";

// Ftp data
inline string ftpHost;
inline string ftpUser;
inline string ftpPassword;
inline string vdgsFileType;

inline vector<Plane> slotList;

// Finished (text, color) result of OnGetTagItem per callsign+tag column, recomputed at most once per
// second. EuroScope calls OnGetTagItem far more often than the underlying data actually changes, so
// this lets OnGetTagItem just display the last computed value instead of redoing all the work.
struct TagItemCacheEntry {
    time_t tick = 0;
    char text[16] = {0};
    bool colorSet = false;
    int colorCode = 0;
    COLORREF rgb = 0;
};
inline std::unordered_map<string, std::unordered_map<int, TagItemCacheEntry>> tagItemDisplayCache;

// Self-validating callsign->index cache: avoids an O(n) slotList scan on every OnGetTagItem call
// (one call per tag column per aircraft). Always re-validated before use, so a stale entry (after an
// erase/sort elsewhere) just falls back to a full scan instead of returning wrong data.
inline std::unordered_map<string, size_t> slotListIndexCache;

inline vector<Plane> slotListToUpdate;
inline vector<Plane> slotListSaved;
inline vector<vector<string>> dataSaved;
inline vector<vector<string>> obtList;
inline vector<Plane> apiCtots;
inline vector<string> asatList;
inline vector<string> taxiTimesList;
inline vector<string> TxtTimesVector;
inline vector<vector<string>> OutOfTsat;
inline vector<string> colors;
inline vector<Rate> rate;
inline vector<Rate> initialRate;
inline vector<string> planeAiportList;
inline vector<string> masterAirports;
inline vector<vector<string>> serverMasterAirports;
inline vector<string> CDMairports;
inline vector<string> CTOTcheck;
inline vector<string> finalTimesList;
inline vector<string> disconnectionList;
inline vector<string> reaSent;
inline vector<string> reaCTOTSent;
inline vector<vector<string>> slotFile;
inline vector<vector<string>> evCtots;
inline vector<Delay> delayList;
inline vector<ServerRestricted> serverRestrictedPlanes;
inline vector<Plane> setOBTlater;
inline vector<vector<string>> setCdmStslater;
inline vector<Plane> setCdmDatalater;
inline vector<string> suWaitList;
inline vector<string> checkCIDLater;
inline vector<string> disabledCtots;
inline vector<vector<string>> networkStatus;
inline vector<vector<string>> onTimeStatus;
inline vector<Plane> apiQueueResponse;
inline std::mutex apiQueueResponseMutex;
inline vector<vector<string>> deiceList;
inline vector<sidInterval> sidIntervalList;
inline vector<string> atotSet;
inline vector<vector<string>> relevantFlights;
inline vector<string> messagesSent;
inline vector<string> reqTobtList;
inline std::mutex later1Mutex;
inline std::mutex later2Mutex;
inline std::mutex later3Mutex;
inline std::mutex later4Mutex;
inline std::mutex networkStatusMutex;
inline std::mutex reqTobtTypesQueueMutex;
inline vector<vector<string>> reqTobtTypesQueue;

inline COLORREF TAG_GREEN = 0xFFFFFFFF;
inline COLORREF TAG_GREENNOTACTIVE = 0xFFFFFFFF;
inline COLORREF TAG_GREY = 0xFFFFFFFF;
inline COLORREF TAG_ORANGE = 0xFFFFFFFF;
inline COLORREF TAG_YELLOW = 0xFFFFFFFF;
inline COLORREF TAG_DARKYELLOW = 0xFFFFFFFF;
inline COLORREF TAG_RED = 0xFFFFFFFF;
inline COLORREF TAG_EOBT = 0xFFFFFFFF;
inline COLORREF TAG_TTOT = 0xFFFFFFFF;
inline COLORREF TAG_ASRT = 0xFFFFFFFF;
inline COLORREF TAG_CTOT = 0xFFFFFFFF;
inline COLORREF SU_SET_COLOR = 0xFFFFFFFF;
inline COLORREF BLOCKS_CALLSIGN_COLOR = RGB(220, 220, 220);

