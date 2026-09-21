// Plugin lifecycle: construction/destruction, log path setup, basic messaging.
// Moved verbatim out of CDMSingle.cpp - see src/core/CDMGlobals.hpp for the shared-state rationale.

#include "CDMSingle.hpp"
#include "src/core/CDMGlobals.hpp"

#include "src/screen/CDMScreen.h"
#include "third_party/pugixml/pugixml.hpp"
#include "src/api/CurlRestClient.h"

extern "C" IMAGE_DOS_HEADER __ImageBase;

using namespace pugi;

CDM::CDM(void)
    : CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE, MY_PLUGIN_NAME, MY_PLUGIN_VERSION, MY_PLUGIN_DEVELOPER,
              MY_PLUGIN_COPYRIGHT) {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    restclient_ = std::make_shared<api::CurlRestClient>();

    string loadingMessage = "Version: ";
    loadingMessage += MY_PLUGIN_VERSION;
    loadingMessage += " loaded.";
    sendMessage(loadingMessage);

    // Register Tag Item "CDM-OPTIONS"
    RegisterTagItemType("Options", TAG_ITEM_OPTIONS);
    RegisterTagItemFunction("Options", TAG_FUNC_OPT);

    // Register Tag Item "CDM-EOBT"
    RegisterTagItemType("EOBT", TAG_ITEM_EOBT);
    RegisterTagItemFunction("Edit EOBT", TAG_FUNC_EDITEOBT);
    RegisterTagItemFunction("Ready EOBT", TAG_FUNC_READYEOBT);
    RegisterTagItemFunction("EOBT Options", TAG_FUNC_OPT_EOBT);

    // Register Tag Item "CDM-TOBT"
    RegisterTagItemType("TOBT", TAG_ITEM_TOBT);
    RegisterTagItemFunction("Ready TOBT", TAG_FUNC_READYTOBT);
    RegisterTagItemFunction("Edit TOBT", TAG_FUNC_EDITTOBT);
    RegisterTagItemFunction("EOBT to TOBT", TAG_FUNC_EOBTTOTOBT);
    RegisterTagItemFunction("TOBT Options", TAG_FUNC_OPT_TOBT);

    // Register Tag Item "CDM-E/TOBT"
    RegisterTagItemType("E/TOBT", TAG_ITEM_ETOBT);
    RegisterTagItemFunction("E/TOBT Options", TAG_FUNC_OPT_ETOBT);

    // Register Tag Item "CDM-TSAT"
    RegisterTagItemType("TSAT", TAG_ITEM_TSAT);
    RegisterTagItemType("TSAT/TOBT-DIFF", TAG_ITEM_TSAT_TOBT_DIFF);
    RegisterTagItemType("TSAT-TOBT DIFF", TAG_ITEM_TSAT_DIFF_TOBT);

    // Register Tag Item "CDM-TTOT"
    RegisterTagItemType("TTOT", TAG_ITEM_TTOT);
    RegisterTagItemFunction("TTOT Options", TAG_FUNC_OPT_TTOT);

    // Register Tag Item "CDM-TSAC"
    RegisterTagItemType("TSAC", TAG_ITEM_TSAC);
    RegisterTagItemType("TSAC-Simple", TAG_ITEM_TSAC_SIMPLE);
    RegisterTagItemFunction("Add TSAT to TSAC", TAG_FUNC_ADDTSAC);
    RegisterTagItemFunction("Remove TSAC", TAG_FUNC_REMOVETSAC);
    RegisterTagItemFunction("Edit TSAC", TAG_FUNC_EDITTSAC);
    RegisterTagItemFunction("TSAC Options", TAG_FUNC_OPT_TSAC);

    // Register Tag Item "CDM-CTOC"
    RegisterTagItemType("CTOC", TAG_ITEM_CTOC);
    RegisterTagItemType("CTOC-Simple", TAG_ITEM_CTOC_SIMPLE);
    RegisterTagItemFunction("Add CTOT to CTOC", TAG_FUNC_ADDCTOC);
    RegisterTagItemFunction("Remove CTOC", TAG_FUNC_REMOVECTOC);
    RegisterTagItemFunction("Edit CTOC", TAG_FUNC_EDITCTOC);
    RegisterTagItemFunction("CTOC Options", TAG_FUNC_OPT_CTOC);

    // Register Tag Item "CDM-ASAT"
    RegisterTagItemType("ASAT", TAG_ITEM_ASAT);

    // Register Tag Item "CDM-ASAT"
    RegisterTagItemType("ASRT", TAG_ITEM_ASRT);
    RegisterTagItemFunction("Toggle ASRT", TAG_FUNC_TOGGLEASRT);
    RegisterTagItemFunction("Toggle ASRT+REA", TAG_FUNC_TOGGLEASRTREA);

    // Register Tag Item "CDM-ASAT"
    RegisterTagItemType("Ready Start-up", TAG_ITEM_READYSTARTUP);
    RegisterTagItemFunction("Toggle Ready Start-up", TAG_FUNC_READYSTARTUP);

    // Register Tag Item "CDM-E"
    RegisterTagItemType("E", TAG_ITEM_E);

    // Register Others
    RegisterTagItemType("Flow Message", TAG_ITEM_FLOW_MESSAGE);
    RegisterTagItemType("TimeNow to TSAT diff", NOW_TSAT_DIFF);
    RegisterTagItemType("TimeNow to TTOT diff", NOW_TTOT_DIFF);
    RegisterTagItemType("TimeNow to CTOT diff", NOW_CTOT_DIFF);

    // Register Tag Item "CDM-CTOT"
    RegisterTagItemType("CTOT", TAG_ITEM_CTOT);
    RegisterTagItemFunction("CTOT Options", TAG_FUNC_CTOTOPTIONS);
    RegisterTagItemFunction("Get FM as text", TAG_FUNC_FMASTEXT);

    // Register Tag Item "CDM-EVENT-CTOT"
    RegisterTagItemType("EV-SLOT", TAG_ITEM_EV_CTOT);
    RegisterTagItemFunction("EvSLOT Options", TAG_FUNC_OPT_EvCTOT);
    RegisterTagItemFunction("EvSLOT to MANUAL CTOT", TAG_FUNC_EvCTOTtoCTOT);
    RegisterTagItemFunction("EvSLOT to TOBT", TAG_FUNC_EvCTOTtoTOBT);

    // Register Tag Item and functions "NETWORK STATUS"
    RegisterTagItemType("Network Sts", TAG_ITEM_NETWORK_STATUS);
    RegisterTagItemType("Network Sts Airborne", TAG_ITEM_NETWORK_STATUS_AIRBORNE);
    RegisterTagItemFunction("Network Sts Options", TAG_FUNC_NETWORK_STATUS_OPTIONS);

    // Register Tag Item "CDM-DEICE"
    RegisterTagItemType("DE-ICE", TAG_ITEM_DEICE);
    RegisterTagItemFunction("DE-ICE Options", TAG_FUNC_OPT_DEICE);

    // Register Tag Item "REQTOBT-TYPE"
    RegisterTagItemType("TOBT-SET-BY", TAG_ITEM_TOBT_SETBY);
    RegisterTagItemType("TOBT-SET-BY SHORT", TAG_ITEM_TOBT_SETBY_SHORT);

    // Register Tag Item "ON_TIME_STATUS"
    RegisterTagItemType("On Time Sts", TAG_ITEM_ON_TIME_STATUS);

    // Register Tag Item "VDGS PM SEND"
    RegisterTagItemType("VDGS PM SEND", TAG_ITEM_SEND_STATUS);
    RegisterTagItemType("VDGS PM SEND SHORT", TAG_ITEM_SEND_STATUS_SHORT);
    RegisterTagItemFunction("Send VDGS PM", TAG_FUNC_PM_SEND);

    GetModuleFileNameA(HINSTANCE(&__ImageBase), DllPathFile, sizeof(DllPathFile));
    pfad = DllPathFile;
    pfad.resize(pfad.size() - strlen("CDM.dll"));
    pfad += "CDMconfig.xml";

    dfad = DllPathFile;
    dfad.resize(dfad.size() - strlen("CDM.dll"));
    dfad += "CDM_data";

    lfad = DllPathFile;
    lfad.resize(lfad.size() - strlen("CDM.dll"));
    lfad += "taxizones.txt";

    sfad = DllPathFile;
    sfad.resize(sfad.size() - strlen("CDM.dll"));
    sfad += "savedData.txt";

    cfad = DllPathFile;
    cfad.resize(cfad.size() - strlen("CDM.dll"));
    cfad += "slot.txt";

    vfad = DllPathFile;
    vfad.resize(vfad.size() - strlen("CDM.dll"));
    vfad += "colors.txt";

    rfad = DllPathFile;
    rfad.resize(rfad.size() - strlen("CDM.dll"));
    rfad += "rate.txt";

    std::time_t now = std::time(nullptr);
    std::tm* localTime = std::localtime(&now);
    tfad = DllPathFile;
    tfad.resize(tfad.size() - strlen("CDM.dll"));
    tfad += "logs\\";
    BuildAndEnsureLogPath(tfad);
    // Build a sortable timestamp: log_YYYYMMDD_HHMMSS.txt
    char tsBuffer[32];
    std::strftime(tsBuffer, sizeof(tsBuffer), "%Y%m%d_%H%M%S", localTime);
    tfad += std::string("log_") + tsBuffer + ".txt";
    addLogLine(loadingMessage);

    debugMode = false;
    initialSidLoad = false;

    countTime = std::time(nullptr);
    countTimeNonCdm = std::time(nullptr);
    countFetchServerTime = std::time(nullptr);
    countRefreshActions4Time = std::time(nullptr);
    countNetworkTobt = std::time(nullptr);
    countTfcDisconnectionTime = std::time(nullptr);
    // countTime = stoi(GetTimeNow()) - refreshTime;
    addTime = false;

    countTfcDisconnection = -1;

    GetVersion();

    // Get data from xml config file
    // airport = getFromXml("/CDM/apt/@icao");
    // airport = getFromXml("/CDM/apt/@icao");

    // Validate CDMconfig.xml exists and parses before using it
    {
        xml_document testDoc;
        xml_parse_result parseResult = testDoc.load_file(pfad.c_str());
        if (!parseResult) {
            sendMessage("Error",
                        "CDMconfig.xml not found or failed to parse. Place CDMconfig.xml next to CDM.dll. Details: " +
                            string(parseResult.description()));
            addLogLine("FATAL: CDMconfig.xml not found or failed to parse: " + string(parseResult.description()));
            return;
        }
    }

    try {
        defTaxiTime = stoi(getFromXml("/CDM/DefaultTaxiTime/@minutes"));
        string deIceLight = getFromXml("/CDM/DeIceTimes/@light");
        string deIceMedium = getFromXml("/CDM/DeIceTimes/@medium");
        string deIceHeavy = getFromXml("/CDM/DeIceTimes/@heavy");
        string deIceSuper = getFromXml("/CDM/DeIceTimes/@super");
        deIceTaxiRem1Name = getFromXml("/CDM/DeIceRemTaxi/@rem1Name");
        deIceTaxiRem2Name = getFromXml("/CDM/DeIceRemTaxi/@rem2Name");
        deIceTaxiRem3Name = getFromXml("/CDM/DeIceRemTaxi/@rem3Name");
        deIceTaxiRem4Name = getFromXml("/CDM/DeIceRemTaxi/@rem4Name");
        deIceTaxiRem5Name = getFromXml("/CDM/DeIceRemTaxi/@rem5Name");
        string deIceRem1 = getFromXml("/CDM/DeIceRemTaxi/@rem1");
        string deIceRem2 = getFromXml("/CDM/DeIceRemTaxi/@rem2");
        string deIceRem3 = getFromXml("/CDM/DeIceRemTaxi/@rem3");
        string deIceRem4 = getFromXml("/CDM/DeIceRemTaxi/@rem4");
        string deIceRem5 = getFromXml("/CDM/DeIceRemTaxi/@rem5");
        refreshTime = stoi(getFromXml("/CDM/RefreshTime/@seconds"));
        string eventModeTimeString = getFromXml("/CDM/eventModeMin/@time");
        string realModeStr = getFromXml("/CDM/realMode/@mode");
        string bmiModeString = getFromXml("/CDM/bmi/@mode");
        string pilotTobtStr = getFromXml("/CDM/pilotTobt/@mode");
        string autSetAtot = getFromXml("/CDM/autoAtot/@mode");
        rateString = getFromXml("/CDM/rate/@ops");
        lvoRateString = getFromXml("/CDM/rateLvo/@ops");
        rateUrl = getFromXml("/CDM/Rates/@url");
        taxiZonesUrl = getFromXml("/CDM/Taxizones/@url");
        slotURL = getFromXml("/CDM/Slots/@url");
        sidIntervalUrl = getFromXml("/CDM/sidInterval/@url");
        string invalidateTSAT_OptionStr = getFromXml("/CDM/invalidateAtTsat/@mode");
        string invalidateTOBT_OptionStr = getFromXml("/CDM/invalidateAtTobt/@mode");
        string readySetTsacOpt = getFromXml("/CDM/readySetTsac/@mode");
        string stringDebugMode = getFromXml("/CDM/Debug/@mode");
        vdgsFileType = getFromXml("/CDM/vdgsFileType/@type");
        ftpHost = getFromXml("/CDM/ftpHost/@host");
        ftpUser = getFromXml("/CDM/ftpUser/@user");
        ftpPassword = getFromXml("/CDM/ftpPassword/@password");
        string sftpConnectionString = getFromXml("/CDM/sftpConnection/@mode");
        string cdmserver = getFromXml("/CDM/Server/@mode");
        string opt_su_wait = getFromXml("/CDM/Su_Wait/@mode");
        cdmServerUrl = getFromXml("/CDM/viffSystem/@url");
        customRestrictedUrl = getFromXml("/CDM/customRestricted/@url");
        string flashingTOBTendString = getFromXml("/CDM/flashingMode/@tobtLastMin");
        string flashingTSATstartString = getFromXml("/CDM/flashingMode/@tsatFirstMin");
        string flashingTSATendString = getFromXml("/CDM/flashingMode/@tsatLastMin");
        string reqTobtPriorityString = getFromXml("/CDM/reqTobtPriority/@mode");
        string eventPriorityString = getFromXml("/CDM/eventPriority/@mode");
        string autoSetTobtFromEvSlotString = getFromXml("/CDM/autoSetTobtFromEvSlot/@mode");
        pm_message = getFromXml("/CDM/PrivateMessage/@text");
        string remarksOptionCtotString = getFromXml("/CDM/remarksOptionCtot/@mode");
        string disableTobtReqAfterAsrt = getFromXml("/CDM/disableTobtReqAfterAsrt/@mode");
        string loadingTextString = getFromXml("/CDM/loading/@text");

        apikey = "TEST";
        if (ftpHost == "" && ftpUser == "") {
            ftpHost = "ftp.vatsimspain.es";
            ftpUser = "aman_vatspa";
            ftpPassword = "TEST";
        }

        // Get Values from sidInterval
        sidIntervalEnabled = false;
        if (sidIntervalUrl.length() > 5) {
            addLogLine("sidInterval - ENABLED");
            sidIntervalEnabled = true;
            getSidIntervalValuesUrl(sidIntervalUrl);
        } else {
            addLogLine("sidInterval - DISABLED");
        }

        // min 10 seconds Refresh Time
        if (refreshTime < 10) {
            refreshTime = 10;
        }

        if (pm_message == "") {
            pm_message = "[CDM MSG] PLEASE, MONITOR https://vats.im/vdgs FOR CDM AND ATFCM UPDATES. [END OF CDM MSG]";
        }

        deIceTimeL = 5;
        deIceTimeM = 9;
        deIceTimeH = 12;
        deIceTimeJ = 15;
        if (deIceLight != "") {
            deIceTimeL = stoi(deIceLight);
        }
        if (deIceMedium != "") {
            deIceTimeM = stoi(deIceMedium);
        }
        if (deIceHeavy != "") {
            deIceTimeH = stoi(deIceHeavy);
        }
        if (deIceSuper != "") {
            deIceTimeJ = stoi(deIceSuper);
        }

        deIceTaxiRem1 = 0;
        deIceTaxiRem2 = 0;
        deIceTaxiRem3 = 0;
        deIceTaxiRem4 = 0;
        deIceTaxiRem5 = 0;
        if (deIceRem1 != "") {
            deIceTaxiRem1 = stoi(deIceRem1);
        }
        if (deIceRem2 != "") {
            deIceTaxiRem2 = stoi(deIceRem2);
        }
        if (deIceRem3 != "") {
            deIceTaxiRem3 = stoi(deIceRem3);
        }
        if (deIceRem4 != "") {
            deIceTaxiRem4 = stoi(deIceRem4);
        }
        if (deIceRem5 != "") {
            deIceTaxiRem5 = stoi(deIceRem5);
        }

        flashingTOBTend = false;
        flashingTSATstart = false;
        flashingTSATend = false;

        if (flashingTOBTendString == "true") flashingTOBTend = true;
        if (flashingTSATstartString == "true") flashingTSATstart = true;
        if (flashingTSATendString == "true") flashingTSATend = true;

        bmiMode = false;
        if (bmiModeString == "true") {
            bmiMode = true;
        }

        loadingText = "....";
        if (loadingTextString != "") {
            loadingText = loadingTextString;
        }

        eventPriorityEnabled = false;
        if (eventPriorityString == "true") {
            eventPriorityEnabled = true;
        }

        reqTobtPriority = false;
        if (reqTobtPriorityString == "true") {
            reqTobtPriority = true;
        }

        autoSetTobtFromEvSlot = false;
        if (autoSetTobtFromEvSlotString == "true") {
            autoSetTobtFromEvSlot = true;
        }

        option_su_wait = false;
        if (opt_su_wait == "true") {
            option_su_wait = true;
        }

        debugMode = false;
        if (stringDebugMode == "true") {
            debugMode = true;
            sendMessage("[DEBUG MESSAGE] - USING DEBUG MODE");
        }

        pilotTobt = false;
        if (pilotTobtStr == "true") {
            pilotTobt = true;
        }

        atotEnabled = false;
        if (autSetAtot == "true") {
            atotEnabled = true;
        }

        eventMode = false;
        if (eventModeTimeString == "") {
            eventModeTime = 0;
        } else {
            eventModeTime = stoi(eventModeTimeString);
        }

        realMode = false;
        if (realModeStr == "true") {
            realMode = true;
        }

        serverEnabled = true;
        if (cdmserver == "false") {
            serverEnabled = false;
        }

        sftpConnection = true;
        if (sftpConnectionString == "false") {
            sftpConnection = false;
        }

        // Invalidate FP at TSAT+6
        invalidateTSAT_Option = true;
        invalidateTSAT_Option_asrt = false;
        if (invalidateTSAT_OptionStr == "false") {
            invalidateTSAT_Option = false;
        } else if (invalidateTSAT_OptionStr == "asrt") {
            invalidateTSAT_Option = true;
            invalidateTSAT_Option_asrt = true;
        }

        invalidateTOBT_Option = true;
        if (invalidateTOBT_OptionStr == "false") {
            invalidateTOBT_Option = false;
        }

        readySetTsac = true;
        if (readySetTsacOpt == "false") {
            readySetTsac = false;
        }

        
        remarksOptionCtot = false;
        if (remarksOptionCtotString == "true") {
            remarksOptionCtot = true;
        }

        tobtReqAfterAsrtDisabledOption = false;
        if (disableTobtReqAfterAsrt == "true") {
            tobtReqAfterAsrtDisabledOption = true;
        }

    } catch (const std::exception& e) {
        sendMessage("Error",
                    "CDMconfig.xml has missing or invalid values. Check configuration. Details: " + string(e.what()));
        addLogLine("FATAL: CDMconfig.xml has missing or invalid values: " + string(e.what()));
        return;
    }

    // Check rates
    runDetachedTask(&CDM::getNetworkRates);

    // Get Server Status
    runDetachedTask(&CDM::getCdmServerStatus);

    runDetachedTask(&CDM::getCdmServerOnTime);

    // CDM-Server
    if (cdmServerUrl.length() <= 1) {
        cdmServerUrl = "https://api.viffsys.com";
    }

    if (customRestrictedUrl.length() <= 1) {
        customRestrictedUrl = "";
    }

    // CDM-Server Fetch restricted
    runDetachedTask(&CDM::getCdmServerRestricted, slotList);

    runDetachedTask(&CDM::getCdmServerMasterAirports);

    runDetachedTask(&CDM::getCdmServerRelevantFlights);

    if (ftpPassword == "") {
        ftpPassword = "test";
    }

    // Init reamrksOption
    remarksOption = false;

    // Init refreshActions
    refresh1 = false;
    refresh3 = false;
    refresh4 = false;

    showPanel = true;
    showAtfcmList = false;

    // Initialize with empty callsign
    myAtcCallsign = "";

    lvo = false;
    if (rateUrl.length() <= 1) {
        if (debugMode) {
            sendMessage("[DEBUG MESSAGE] - USING RATES FROM LOCAL TXT FILE");
        }
        getRate();
    } else {
        if (debugMode) {
            sendMessage("[DEBUG MESSAGE] - USING RATES FROM URL");
        }
        getRateFromUrl(rateUrl);
    }

    if (taxiZonesUrl.length() <= 1) {
        if (debugMode) {
            sendMessage("[DEBUG MESSAGE] - USING TAXIZONES FROM LOCAL TXT FILE");
        }
        // Get data from .txt file
        fstream file;
        string lineValue;
        file.open(lfad.c_str(), std::ios::in);
        while (getline(file, lineValue)) {
            if (!lineValue.empty()) {
                if (lineValue.substr(0, 1) != "#") {
                    TxtTimesVector.push_back(lineValue);
                }
            }
        }
    } else {
        if (debugMode) {
            sendMessage("[DEBUG MESSAGE] - USING TAXIZONES FROM URL");
        }
        getTaxiZonesFromUrl(taxiZonesUrl);
    }

    // Get Values from ctot web or file
    if (slotURL.length() <= 1) {
        ctotCid = false;
        if (debugMode) {
            sendMessage("[DEBUG MESSAGE] - NOT SHOWING EVCTOTs");
        }
        // Get data from .txt file
        /*fstream fileCtot;
        string lineValueCtot;
        fileCtot.open(cfad.c_str(), std::ios::in);
        while (getline(fileCtot, lineValueCtot))
        {
                addCtotToMainList(lineValueCtot);
        }*/
    } else {
        ctotCid = true;
        if (debugMode) {
            sendMessage("[DEBUG MESSAGE] - SHOWING EVCTOTs");
        }
        getCtotsFromUrl(slotURL);
    }

    fstream fileColors;
    string lineValueColors;
    vector<int> sep;
    fileColors.open(vfad.c_str(), std::ios::in);
    COLORREF color = RGB(0, 0, 0);
    smatch match;
    while (getline(fileColors, lineValueColors)) {
        if (regex_match(lineValueColors, match, regex("^color(\\d+):(\\d+),(\\d+),(\\d+)$", regex::icase))) {
            color = RGB(stoi(match[2]), stoi(match[3]), stoi(match[4]));
            switch (stoi(match[1])) {
                case 1:
                    TAG_GREEN = color;
                    break;
                case 2:
                    TAG_GREENNOTACTIVE = color;
                    break;
                case 3:
                    TAG_GREY = color;
                    break;
                case 4:
                    TAG_ORANGE = color;
                    break;
                case 5:
                    TAG_YELLOW = color;
                    break;
                case 6:
                    TAG_DARKYELLOW = color;
                    break;
                case 7:
                    TAG_RED = color;
                    break;
                case 8:
                    TAG_EOBT = color;
                    break;
                case 9:
                    TAG_TTOT = color;
                    break;
                case 10:
                    TAG_ASRT = color;
                    break;
                case 11:
                    TAG_CTOT = color;
                    break;
                case 12:
                    SU_SET_COLOR = color;
                    break;
                case 13:
                    BLOCKS_CALLSIGN_COLOR = color;
                    break;
                default:
                    break;
            }
        }
    }

    restclient_ = std::make_shared<api::CurlRestClient>();
}

static bool EnsureDirExists(const std::string& dir) {
#ifdef _WIN32
    if (_mkdir(dir.c_str()) == 0) return true;
    return (errno == EEXIST);
#else
    if (mkdir(dir.c_str(), 0755) == 0) return true;
    return (errno == EEXIST);
#endif
}

// helper: drop trailing slash/backslash (if any)
static std::string RTrimSlash(std::string s) {
    while (!s.empty() && (s.back() == '\\' || s.back() == '/')) s.pop_back();
    return s;
}

void CDM::BuildAndEnsureLogPath(std::string& tfad) {
    // Ensure "...\logs\" directory exists
    std::string logsDir = RTrimSlash(tfad);  // => "...\logs"
    EnsureDirExists(logsDir);

    // Prune old log files, keeping only the 3 most recent.
    // Files are named log_YYYYMMDD_HHMMSS.txt so lexicographic sort == time sort.
    std::vector<std::string> logFiles;
    WIN32_FIND_DATAA findData;
    std::string searchPattern = logsDir + "\\log_*.txt";
    HANDLE hFind = FindFirstFileA(searchPattern.c_str(), &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            logFiles.push_back(logsDir + "\\" + findData.cFileName);
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }
    std::sort(logFiles.begin(), logFiles.end());
    const int MAX_LOGS = 3;
    while ((int)logFiles.size() >= MAX_LOGS) {
        remove(logFiles.front().c_str());
        logFiles.erase(logFiles.begin());
    }
}

CRadarScreen* CDM::OnRadarScreenCreated(const char* sDisplayName, bool NeedRadarContent, bool GeoReferenced,
                                        bool CanBeSaved, bool CanBeCreated) {
    cs = new CDMScreen(this);
    return cs;
}

// Run on Plugin destruction, Ie. Closing EuroScope or unloading plugin
CDM::~CDM() {
    // Detached background threads (spawned via runDetachedTask/multithread) capture
    // `this`; wait for them to finish before this object is torn down, otherwise
    // they would touch freed memory.
    waitForDetachedTasks();
    curl_global_cleanup();
}

/*
        Custom Functions
*/

void CDM::debugMessage(string type, string message) {
    // Display Debug Message if debugMode = true
    if (debugMode) {
        DisplayUserMessage("CDM", type.c_str(), message.c_str(), true, true, true, false, false);
    }
}

void CDM::sendMessage(string type, string message) {
    // Show a message
    DisplayUserMessage("CDM", type.c_str(), message.c_str(), true, true, true, true, false);
}

void CDM::sendMessage(string message) {
    DisplayUserMessage(MY_PLUGIN_NAME, "", message.c_str(), true, true, true, false, false);
}

void CDM::OnFlightPlanFlightPlanDataUpdate(CFlightPlan FlightPlan) {
    if (myAtcCallsign != ControllerMyself().GetCallsign()) {
        if (myAtcCallsign == "") {
            myAtcCallsign = ControllerMyself().GetCallsign();
        } else {
            RemoveMasterAirports();
        }
    }
}
