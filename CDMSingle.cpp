#include "stdafx.h"
#include "CDMSingle.hpp"
#include "pugixml.hpp"
#include "pugixml.cpp"
#include <thread>
#include "Delay.h"
#include "EcfmpRestriction.h"
#include "SFTP.h"
#include "CDMScreen.h"

extern "C" IMAGE_DOS_HEADER __ImageBase;

bool blink;
bool debugMode, initialSidLoad;

int disCount;
ifstream sidDatei;
char DllPathFile[_MAX_PATH];
string pfad;
string lfad;
string sfad;
string cfad;
string vfad;
string rfad;
string dfad;
string xfad;
string tfad;
string rateString;
string lvoRateString;
int expiredCTOTTime;
bool defaultRate;
time_t countTime;
time_t countTimeNonCdm;
time_t countFetchServerTime;
time_t countRefreshActions4Time;
time_t countTfcDisconnectionTime;
time_t countEcfmpTime;
time_t countNetworkTobt;
int countTfcDisconnection;
int refreshTime;
bool addTime;
bool lvo;
bool bmiMode;
bool ctotCid;
bool realMode;
bool eventMode;
int eventModeTime;
bool pilotTobt;
bool atotEnabled;
bool remarksOption;
bool remarksOptionCtot;
bool invalidateTSAT_Option;
bool invalidateTSAT_Option_asrt;
bool invalidateTOBT_Option;
bool readySetTsac;
bool sidIntervalEnabled;
bool readyToUpdateList;
string lastAddedIcao;
string myTimeToAdd;
string rateUrl;
string taxiZonesUrl;
string ctotURL;
string cdmServerUrl;
string customRestrictedUrl;
string sidIntervalUrl;
int defTaxiTime;
bool flashingTOBTend;
bool flashingTSATstart;
bool flashingTSATend;
string flowRestrictionsUrl;
string cdm_api;
string myAtcCallsign;
bool option_su_wait;
string apikey;
bool serverEnabled;
bool sftpConnection;
bool refresh1;
bool refresh2;
bool refresh3;
bool refresh4;
string flightsFilterText;

bool showPanel;
bool showAtfcmList;

CDMScreen* cs;

int deIceTimeL;
int deIceTimeM;
int deIceTimeH;
int deIceTimeJ;

int deIceTaxiRem1;
int deIceTaxiRem2;
int deIceTaxiRem3;
int deIceTaxiRem4;
int deIceTaxiRem5;

string deIceTaxiRem1Name = "";
string deIceTaxiRem2Name = "";
string deIceTaxiRem3Name = "";
string deIceTaxiRem4Name = "";
string deIceTaxiRem5Name = "";

//Ftp data
string ftpHost;
string ftpUser;
string ftpPassword;
string vdgsFileType;

vector<Plane> slotList;
vector<Plane> slotListToUpdate;
vector<Plane> slotListSaved;
vector<vector<string>> dataSaved;
vector<vector<string>> obtList;
vector<EcfmpRestriction> ecfmpData;
vector<Plane> apiCtots;
vector<string> asatList;
vector<string> taxiTimesList;
vector<string> TxtTimesVector;
vector<vector<string>> OutOfTsat;
vector<string> colors;
vector<Rate> rate;
vector<Rate> initialRate;
vector<string> planeAiportList;
vector<string> masterAirports;
vector<vector<string>> serverMasterAirports;
vector<string> CDMairports;
vector<string> CTOTcheck;
vector<string> finalTimesList;
vector<string> disconnectionList;
vector<string> reaSent;
vector<string> reaCTOTSent;
vector<vector<string>> slotFile;
vector<vector<string>> evCtots;
vector<Delay> delayList;
vector<ServerRestricted> serverRestrictedPlanes;
vector<Plane> setOBTlater;
vector<vector<string>> setCdmStslater;
vector<Plane> setCdmDatalater;
vector<string> suWaitList;
vector<string> checkCIDLater;
vector<string> disabledCtots;
vector<vector<string>> networkStatus;
vector<vector<string>> onTimeStatus;
vector<Plane> apiQueueResponse;
std::mutex apiQueueResponseMutex;
vector<vector<string>> deiceList;
vector<sidInterval> sidIntervalList;
vector<string> atotSet;
vector<vector<string>> reqTobtTypes;
vector<vector<string>> reqTobtTypesQueue;
vector<vector<string>> relevantFlights;
vector<string> messagesSent;
std::mutex reqTobtTypesQueueMutex;
std::mutex later1Mutex;
std::mutex later2Mutex;
std::mutex later3Mutex;
std::mutex later4Mutex;
std::mutex networkStatusMutex;

using namespace std;
using namespace EuroScopePlugIn;
using namespace pugi;

COLORREF TAG_GREEN = 0xFFFFFFFF;
COLORREF TAG_GREENNOTACTIVE = 0xFFFFFFFF;
COLORREF TAG_GREY = 0xFFFFFFFF;
COLORREF TAG_ORANGE = 0xFFFFFFFF;
COLORREF TAG_YELLOW = 0xFFFFFFFF;
COLORREF TAG_DARKYELLOW = 0xFFFFFFFF;
COLORREF TAG_RED = 0xFFFFFFFF;
COLORREF TAG_EOBT = 0xFFFFFFFF;
COLORREF TAG_TTOT = 0xFFFFFFFF;
COLORREF TAG_ASRT = 0xFFFFFFFF;
COLORREF TAG_CTOT = 0xFFFFFFFF;
COLORREF SU_SET_COLOR = 0xFFFFFFFF;


// Run on Plugin Initialization
CDM::CDM(void) :CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE, MY_PLUGIN_NAME, MY_PLUGIN_VERSION, MY_PLUGIN_DEVELOPER, MY_PLUGIN_COPYRIGHT)
{
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
	RegisterTagItemFunction("EOBT Options", TAG_FUNC_OPT_EOBT);

	//Register Tag Item "CDM-TOBT"
	RegisterTagItemType("TOBT", TAG_ITEM_TOBT);
	RegisterTagItemFunction("Ready TOBT", TAG_FUNC_READYTOBT);
	RegisterTagItemFunction("Edit TOBT", TAG_FUNC_EDITTOBT);
	RegisterTagItemFunction("EOBT to TOBT", TAG_FUNC_EOBTTOTOBT);
	RegisterTagItemFunction("TOBT Options", TAG_FUNC_OPT_TOBT);

	//Register Tag Item "CDM-E/TOBT"
	RegisterTagItemType("E/TOBT", TAG_ITEM_ETOBT);
	RegisterTagItemFunction("E/TOBT Options", TAG_FUNC_OPT_ETOBT);

	// Register Tag Item "CDM-TSAT"
	RegisterTagItemType("TSAT", TAG_ITEM_TSAT);
	RegisterTagItemType("TSAT/TOBT-DIFF", TAG_ITEM_TSAT_TOBT_DIFF);

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

	//Register Others
	RegisterTagItemType("Flow Message", TAG_ITEM_FLOW_MESSAGE);
	RegisterTagItemType("TimeNow to TSAT diff", NOW_TSAT_DIFF);
	RegisterTagItemType("TimeNow to TTOT diff", NOW_TTOT_DIFF);
	RegisterTagItemType("TimeNow to CTOT diff", NOW_CTOT_DIFF);

	// Register Tag Item "CDM-CTOT"
	RegisterTagItemType("CTOT", TAG_ITEM_CTOT);
	RegisterTagItemFunction("CTOT Options", TAG_FUNC_CTOTOPTIONS);
	RegisterTagItemFunction("Get FM as text", TAG_FUNC_FMASTEXT);

	// Register Tag Item "CDM-EVENT-CTOT"
	RegisterTagItemType("EV-CTOT", TAG_ITEM_EV_CTOT);
	RegisterTagItemFunction("EvCTOT Options", TAG_FUNC_OPT_EvCTOT);
	RegisterTagItemFunction("EvCTOT to MANUAL CTOT", TAG_FUNC_EvCTOTtoCTOT);

	// Register Tag Item and functions "NETWORK STATUS"
	RegisterTagItemType("Network Sts", TAG_ITEM_NETWORK_STATUS);
	RegisterTagItemType("Network Sts Airborne", TAG_ITEM_NETWORK_STATUS_AIRBORNE);
	RegisterTagItemFunction("Network Sts Options", TAG_FUNC_NETWORK_STATUS_OPTIONS);

	//Register Tag Item "CDM-DEICE"
	RegisterTagItemType("DE-ICE", TAG_ITEM_DEICE);
	RegisterTagItemFunction("DE-ICE Options", TAG_FUNC_OPT_DEICE);

	//Register Tag Item "REQTOBT-TYPE"
	RegisterTagItemType("TOBT-SET-BY", TAG_ITEM_TOBT_SETBY);

	// Register Tag Item "ON_TIME_STATUS"
	RegisterTagItemType("On Time Sts", TAG_ITEM_ON_TIME_STATUS);

	// Register Tag Item "VDGS PM SEND"
	RegisterTagItemType("VDGS PM SEND", TAG_ITEM_SEND_STATUS);
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
	cfad += "ctot.txt";

	vfad = DllPathFile;
	vfad.resize(vfad.size() - strlen("CDM.dll"));
	vfad += "colors.txt";

	rfad = DllPathFile;
	rfad.resize(rfad.size() - strlen("CDM.dll"));
	rfad += "rate.txt";

	std::time_t now = std::time(nullptr);
	std::tm* localTime = std::localtime(&now);
	int day = localTime->tm_mday;
	tfad = DllPathFile;
	tfad.resize(tfad.size() - strlen("CDM.dll"));
	tfad += "logs\\";
	BuildAndEnsureLogPath(tfad);
	tfad += "log_" + GetTimeNow().substr(0,3) + ".txt";
	removeLog();
	addLogLine(loadingMessage);

	debugMode = false;
	initialSidLoad = false;

	countTime = std::time(nullptr);
	countTimeNonCdm = std::time(nullptr);
	countFetchServerTime = std::time(nullptr);
	countRefreshActions4Time = std::time(nullptr);
	countEcfmpTime = std::time(nullptr);
	countNetworkTobt = std::time(nullptr);
	countTfcDisconnectionTime = std::time(nullptr);
	//countTime = stoi(GetTimeNow()) - refreshTime;
	addTime = false;

	countTfcDisconnection = -1;

	GetVersion();

	//Get data from xml config file
	//airport = getFromXml("/CDM/apt/@icao");
	//airport = getFromXml("/CDM/apt/@icao");

	// Validate CDMconfig.xml exists and parses before using it
	{
		xml_document testDoc;
		xml_parse_result parseResult = testDoc.load_file(pfad.c_str());
		if (!parseResult) {
			sendMessage("Error", "CDMconfig.xml not found or failed to parse. Place CDMconfig.xml next to CDM.dll. Details: " + string(parseResult.description()));
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
	expiredCTOTTime = stoi(getFromXml("/CDM/expiredCtot/@time"));
	string eventModeTimeString = getFromXml("/CDM/eventModeMin/@time");
	string realModeStr = getFromXml("/CDM/realMode/@mode");
	string bmiModeString = getFromXml("/CDM/bmi/@mode");
	string pilotTobtStr = getFromXml("/CDM/pilotTobt/@mode");
	string autSetAtot = getFromXml("/CDM/autoAtot/@mode");
	rateString = getFromXml("/CDM/rate/@ops");
	lvoRateString = getFromXml("/CDM/rateLvo/@ops");
	rateUrl = getFromXml("/CDM/Rates/@url");
	taxiZonesUrl = getFromXml("/CDM/Taxizones/@url");
	ctotURL = getFromXml("/CDM/Ctot/@url");
	sidIntervalUrl = getFromXml("/CDM/sidInterval/@url");
	string invalidateTSAT_OptionStr = getFromXml("/CDM/invalidateAtTsat/@mode");
	string invalidateTOBT_OptionStr = getFromXml("/CDM/invalidateAtTobt/@mode");
	string readySetTsacOpt = getFromXml("/CDM/readySetTsac/@mode");
	string stringDebugMode = getFromXml("/CDM/Debug/@mode");
	flowRestrictionsUrl = getFromXml("/CDM/FlowRestrictions/@url");
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

	if (ftpHost == "" && ftpUser == "") {
		ftpHost = "ftp.vatsimspain.es";
		ftpUser = "aman_vatspa";
		//Password defined internally
	}

	//Get Values from sidInterval
	sidIntervalEnabled = false;
	if (sidIntervalUrl.length() > 5) {
		addLogLine("sidInterval - ENABLED");
		sidIntervalEnabled = true;
		getSidIntervalValuesUrl(sidIntervalUrl);
	}
	else {
		addLogLine("sidInterval - DISABLED");
	}

	//min 10 seconds Refresh Time
	if (refreshTime < 10) {
		refreshTime = 10;
	}

	deIceTimeL = 5;
	deIceTimeM = 9;
	deIceTimeH = 12;
	deIceTimeJ = 15;
	if (deIceLight != "") {
		deIceTimeL = stoi(deIceLight);
	}
	if (deIceLight != "") {
		deIceTimeM = stoi(deIceMedium);
	}
	if (deIceLight != "") {
		deIceTimeH = stoi(deIceHeavy);
	}
	if (deIceLight != "") {
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
	}
	else {
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

	//Invalidate FP at TSAT+6
	invalidateTSAT_Option = true;
	invalidateTSAT_Option_asrt = false;
	if (invalidateTSAT_OptionStr == "false") {
		invalidateTSAT_Option = false;
	}
	else if (invalidateTSAT_OptionStr == "asrt") {
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

	} catch (const std::exception& e) {
		sendMessage("Error", "CDMconfig.xml has missing or invalid values. Check configuration. Details: " + string(e.what()));
		addLogLine("FATAL: CDMconfig.xml has missing or invalid values: " + string(e.what()));
		return;
	}

	//Flow Data
	std::thread t(&CDM::getEcfmpData, this);
	t.detach();

	//Check rates
	std::thread t7(&CDM::getNetworkRates, this);
	t7.detach();

	//Get Server Status
	std::thread t0(&CDM::getCdmServerStatus, this);
	t0.detach();

	std::thread t63(&CDM::getCdmServerOnTime, this);
	t63.detach();

	//CDM-Server
	if (cdmServerUrl.length() <= 1) {
		cdmServerUrl = "https://viff-system.network";
	}

	if (customRestrictedUrl.length() <= 1) {
		customRestrictedUrl = "";
	}

	//CDM-Server Fetch restricted
	std::thread t34(&CDM::getCdmServerRestricted, this, slotList);
	t34.detach();

	std::thread t75(&CDM::getCdmServerMasterAirports, this);
	t75.detach();

	std::thread t73(&CDM::getCdmServerRelevantFlights, this);
	t73.detach();

	if (ftpPassword == "") {
		ftpPassword = "test";
		ftpPassword = "Ek0TxdyF33yaxBqxRAK5";
	}

	//Init reamrksOption
	remarksOption = false;
	remarksOptionCtot = false;

	//Init refreshActions
	refresh1 = false;
	refresh2 = false;
	refresh3 = false;
	refresh4 = false;

	showPanel = true;
	showAtfcmList = false;

	//Initialize with empty callsign
	myAtcCallsign = "";

	lvo = false;
	if (rateUrl.length() <= 1) {
		if (debugMode) {
			sendMessage("[DEBUG MESSAGE] - USING RATES FROM LOCAL TXT FILE");
		}
		getRate();
	}
	else {
		if (debugMode) {
			sendMessage("[DEBUG MESSAGE] - USING RATES FROM URL");
		}
		getRateFromUrl(rateUrl);
	}


	if (taxiZonesUrl.length() <= 1) {
		if (debugMode) {
			sendMessage("[DEBUG MESSAGE] - USING TAXIZONES FROM LOCAL TXT FILE");
		}
		//Get data from .txt file
		fstream file;
		string lineValue;
		file.open(lfad.c_str(), std::ios::in);
		while (getline(file, lineValue))
		{
			if (!lineValue.empty()) {
				if (lineValue.substr(0, 1) != "#") {
					TxtTimesVector.push_back(lineValue);
				}
			}
		}
	}
	else {
		if (debugMode) {
			sendMessage("[DEBUG MESSAGE] - USING TAXIZONES FROM URL");
		}
		getTaxiZonesFromUrl(taxiZonesUrl);
	}


	//Get Values from ctot web or file
	if (ctotURL.length() <= 1) {
		ctotCid = false;
		if (debugMode) {
			sendMessage("[DEBUG MESSAGE] - NOT SHOWING EVCTOTs");
		}
		//Get data from .txt file
		/*fstream fileCtot;
		string lineValueCtot;
		fileCtot.open(cfad.c_str(), std::ios::in);
		while (getline(fileCtot, lineValueCtot))
		{
			addCtotToMainList(lineValueCtot);
		}*/
	}
	else {
		ctotCid = true;
		if (debugMode) {
			sendMessage("[DEBUG MESSAGE] - SHOWING EVCTOTs");
		}
		getCtotsFromUrl(ctotURL);
	}

	fstream fileColors;
	string lineValueColors;
	vector<int> sep;
	fileColors.open(vfad.c_str(), std::ios::in);
	COLORREF color = RGB(0, 0, 0);
	smatch match;
	while (getline(fileColors, lineValueColors))
	{
		if (regex_match(lineValueColors, match, regex("^color(\\d+):(\\d+),(\\d+),(\\d+)$", regex::icase)))
		{
			color = RGB(stoi(match[2]), stoi(match[3]), stoi(match[4]));
			switch (stoi(match[1]))
			{
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
			default:
				break;
			}
		}
	}
}

static bool EnsureDirExists(const std::string& dir)
{
#ifdef _WIN32
	if (_mkdir(dir.c_str()) == 0) return true;
	return (errno == EEXIST);
#else
	if (mkdir(dir.c_str(), 0755) == 0) return true;
	return (errno == EEXIST);
#endif
}

// helper: drop trailing slash/backslash (if any)
static std::string RTrimSlash(std::string s)
{
	while (!s.empty() && (s.back() == '\\' || s.back() == '/'))
		s.pop_back();
	return s;
}

void CDM::BuildAndEnsureLogPath(std::string& tfad)
{
	// ensure "...\logs\" exists
	std::string logsDir = RTrimSlash(tfad);   // => "...\logs"
	EnsureDirExists(logsDir);
}

CRadarScreen* CDM::OnRadarScreenCreated(const char* sDisplayName, bool NeedRadarContent, bool GeoReferenced, bool CanBeSaved, bool CanBeCreated) {
	cs = new CDMScreen(this);
	return cs;
}

// Run on Plugin destruction, Ie. Closing EuroScope or unloading plugin
CDM::~CDM()
{

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

void CDM::OnFlightPlanFlightPlanDataUpdate(CFlightPlan FlightPlan)
{
	if (myAtcCallsign != ControllerMyself().GetCallsign()) {
		if (myAtcCallsign == "") {
			myAtcCallsign = ControllerMyself().GetCallsign();
		}
		else {
			RemoveMasterAirports();
		}
	}
}

//
void CDM::OnFunctionCall(int FunctionId, const char* ItemString, POINT Pt, RECT Area) {
	try{
		if (FunctionId == TAG_FUNC_NEW_MASTER_AIRPORT && ItemString && strlen(ItemString) > 0) {
			std::string airport_icao = ItemString;
			if (lastAddedIcao.empty()) {
				lastAddedIcao = airport_icao;
				addMasterAirport(airport_icao);
			}
		}
		else if (FunctionId == TAG_FUNC_RELEVANT_FLIGHTS_FILTER) {
			flightsFilterText = ItemString;
		}

	//FP Required functions
	CFlightPlan fp = FlightPlanSelectASEL();
	if (!fp.IsValid()) {
		return;
	}
	bool AtcMe = false;
	bool master = false;

	for (string apt : masterAirports)
	{
		if (apt == fp.GetFlightPlanData().GetOrigin()) {
			master = true;
		}
	}
	AtcMe = false;
	string position = ControllerMyself().GetCallsign();
	bool isPositionOk = position.find("_DEL") != string::npos || position.find("_GND") != string::npos || position.find("_TWR") != string::npos || position.find("_APP") != string::npos || position.find("_CTR") != string::npos || position.find("_FMP") != string::npos;
	if ((fp.GetTrackingControllerIsMe() || strlen(fp.GetTrackingControllerId()) == 0) && isPositionOk) {
		AtcMe = true;
	}

	if (FunctionId == TAG_FUNC_PM_SEND) {
		sendCdmMessageToPilot(fp.GetCallsign());
	}
	else if (FunctionId == TAG_FUNC_EDITEOBT)
	{
		//Can be modified as non-master
		if (AtcMe) {
			addLogLine("TRIGGER - TAG_FUNC_EDITEOBT");
			OpenPopupEdit(Area, TAG_FUNC_NEWEOBT, fp.GetFlightPlanData().GetEstimatedDepartureTime());
		}
	}
	else if (FunctionId == TAG_FUNC_NEWEOBT) {
		addLogLine("TRIGGER - TAG_FUNC_NEWEOBT");
		string editedEOBT = ItemString;
		bool hasNoNumber = true;
		if (editedEOBT.length() <= 4) {

			for (size_t i = 0; i < editedEOBT.length(); i++) {
				if (isdigit(editedEOBT[i]) == false) {
					hasNoNumber = false;
				}
			}
			if (hasNoNumber) {
				fp.GetFlightPlanData().SetEstimatedDepartureTime(editedEOBT.c_str());
				fp.GetFlightPlanData().AmendFlightPlan();
				if (editedEOBT.length() == 4) {
					for (int u = 0; u < obtList.size(); u++) {
						if (obtList[u][0] == fp.GetCallsign()) {
							obtList[u][1] = editedEOBT;
							break;
						}
					}
					//Set EOBT in API
					std::thread t(&CDM::setOBTApi, this, fp.GetCallsign(), editedEOBT, true, true);
					t.detach();
				}
			}
		}
	}
	else if (FunctionId == TAG_FUNC_EOBTTOTOBT) {
		addLogLine("TRIGGER - TAG_FUNC_EOBTTOTOBT");
		setFlightStripInfo(fp, formatTime(fp.GetFlightPlanData().GetEstimatedDepartureTime()), 2);
	}
	else if (FunctionId == TAG_FUNC_ADDTSAC) {
		addLogLine("TRIGGER - TAG_FUNC_ADDTSAC");
		string completeTOBT = getFlightStripInfo(fp, 2);
		if (!completeTOBT.empty()) {
			//Get Time now
			time_t rawtime;
			struct tm ptm;
			time(&rawtime);
			gmtime_s(&ptm, &rawtime);
			string hour = to_string(ptm.tm_hour % 24);
			string min = to_string(ptm.tm_min);

			if (stoi(min) < 10) {
				min = "0" + min;
			}
			if (stoi(hour) < 10) {
				hour = "0" + hour.substr(0, 1);
			}

			bool notYetTOBT = false;
			string TOBThour = completeTOBT.substr(0, 2);
			string TOBTmin = completeTOBT.substr(2, 2);

			if (hour != "00") {
				if (TOBThour == "00") {
					TOBThour = "24";
				}
			}

			int EOBTdifTime = GetdifferenceTime(hour, min, TOBThour, TOBTmin);
			if (hour != TOBThour) {
				if (EOBTdifTime < -75) {
					notYetTOBT = true;
				}
			}
			else {
				if (EOBTdifTime < -35) {
					notYetTOBT = true;
				}
			}
			if (!notYetTOBT) {
				for (size_t a = 0; a < slotList.size(); a++)
				{
					if (slotList[a].callsign == fp.GetCallsign()) {
						string getTSAT = slotList[a].tsat;
						if (getTSAT.length() >= 4) {
							setFlightStripInfo(fp, getTSAT.substr(0, 4), 1);
						}
					}
				}
			}
		}
	}

	else if (FunctionId == TAG_FUNC_REMOVETSAC) {
		addLogLine("TRIGGER - TAG_FUNC_REMOVETSAC");
		setFlightStripInfo(fp, "", 1);
	}

	else if (FunctionId == TAG_FUNC_EDITTSAC) {
		addLogLine("TRIGGER - TAG_FUNC_EDITTSAC");
		OpenPopupEdit(Area, TAG_FUNC_NEWTSAC, getFlightStripInfo(fp, 1).c_str());
	}

	else if (FunctionId == TAG_FUNC_NEWTSAC) {
		addLogLine("TRIGGER - TAG_FUNC_NEWTSAC");
		string editedTSAC = ItemString;
		if (editedTSAC.length() > 0) {
			bool hasNoNumber = true;
			if (editedTSAC.length() == 4) {
				for (size_t i = 0; i < editedTSAC.length(); i++) {
					if (isdigit(editedTSAC[i]) == false) {
						hasNoNumber = false;
					}
				}
				if (hasNoNumber) {
					setFlightStripInfo(fp, editedTSAC, 1);
				}
			}
		}
	}

	else if (FunctionId == TAG_FUNC_TOGGLEASRT || FunctionId == TAG_FUNC_READYSTARTUP || FunctionId == TAG_FUNC_TOGGLEASRTREA) {
		if (master && AtcMe) {
			addLogLine("TRIGGER - TAG_FUNC_READYSTARTUP");
			string annotAsrt = getFlightStripInfo(fp, 0);
			if (annotAsrt.empty()) {
				//Get Time now
				time_t rawtime;
				struct tm ptm;
				time(&rawtime);
				gmtime_s(&ptm, &rawtime);
				string hour = to_string(ptm.tm_hour % 24);
				string min = to_string(ptm.tm_min);

				if (stoi(min) < 10) {
					min = "0" + min;
				}
				if (stoi(hour) < 10) {
					hour = "0" + hour.substr(0, 1);
				}

				setFlightStripInfo(fp, (hour + min), 0);
				if (FunctionId == TAG_FUNC_TOGGLEASRTREA) {
					std::thread t74(&CDM::setCdmSts, this, fp.GetCallsign(), "REA/1");
					t74.detach();
				}
			}
			else {
				setFlightStripInfo(fp, "", 0);
			}
		}
	}

	else if (FunctionId == TAG_FUNC_FMASTEXT) {
			addLogLine("TRIGGER - TAG_FUNC_FMASTEXT");
			bool found = false;
			for (size_t i = 0; i < slotList.size(); i++)
			{
				if (slotList[i].callsign == fp.GetCallsign()) {
					if (slotList[i].hasManualCtot) {
						sendMessage(slotList[i].callsign + " FM -> " + slotList[i].flowReason);
						found = true;
					}
				}
			}

			if (!found) {
				for (ServerRestricted sr : serverRestrictedPlanes) {
					if (sr.callsign == (string)fp.GetCallsign()) {
						sendMessage(sr.callsign + " FM -> " + sr.reason);
					}
				}
			}
	}
	else if (FunctionId == TAG_FUNC_OPT_DEICE) {
		if (master && AtcMe) {
			addLogLine("TRIGGER - TAG_FUNC_OPT_DEICE");

			OpenPopupList(Area, "De-Ice", 1);
			AddPopupListElement("NONE", "", TAG_FUNC_DEICE_NONE, false, 2, false);
			AddPopupListElement("STND", "", TAG_FUNC_DEICE_STAND, false, 2, false);
			if (deIceTaxiRem1Name != "") {
				AddPopupListElement(deIceTaxiRem1Name.c_str(), "", TAG_FUNC_DEICE_REMOTE1, false, 2, false);
			}
			if (deIceTaxiRem2Name != "") {
				AddPopupListElement(deIceTaxiRem2Name.c_str(), "", TAG_FUNC_DEICE_REMOTE2, false, 2, false);
			}
			if (deIceTaxiRem3Name != "") {
				AddPopupListElement(deIceTaxiRem3Name.c_str(), "", TAG_FUNC_DEICE_REMOTE3, false, 2, false);
			}
			if (deIceTaxiRem4Name != "") {
				AddPopupListElement(deIceTaxiRem4Name.c_str(), "", TAG_FUNC_DEICE_REMOTE4, false, 2, false);
			}
			if (deIceTaxiRem5Name != "") {
				AddPopupListElement(deIceTaxiRem5Name.c_str(), "", TAG_FUNC_DEICE_REMOTE5, false, 2, false);
			}
		}
	}
	else if (FunctionId == TAG_FUNC_DEICE_NONE) {
		if (master && AtcMe) {
			addLogLine("TRIGGER - TAG_FUNC_DEICE_NONE");

			//Remove plane from deice list
			for (size_t i = 0; i < deiceList.size(); i++) {
				if (deiceList[i][0] == fp.GetCallsign()) {
					deiceList.erase(deiceList.begin() + i);
				}
			}
			setFlightStripInfo(fp, "", 5);
			//Remove plane from taxiTimesList
			for (size_t j = 0; j < taxiTimesList.size(); j++)
			{
				if (taxiTimesList[j].substr(0, taxiTimesList[j].find(",")) == fp.GetCallsign()) {
					taxiTimesList.erase(taxiTimesList.begin() + j);
				}
			}
			//Remove plane from slotlist to recalculate times
			for (size_t i = 0; i < slotList.size(); i++)
			{
				if (slotList[i].callsign == fp.GetCallsign()) {
					slotList.erase(slotList.begin() + i);
				}
			}
		
		}
	}
	else if (FunctionId == TAG_FUNC_DEICE_STAND) {
		if (master && AtcMe) {
			setDeice("STND", fp, "STND");
		}
	}
	else if (FunctionId == TAG_FUNC_DEICE_REMOTE1) {
		if (master && AtcMe) {
			setDeice(deIceTaxiRem1Name, fp, "REM1");
		}
	}
	else if (FunctionId == TAG_FUNC_DEICE_REMOTE2) {
		if (master && AtcMe) {
			setDeice(deIceTaxiRem2Name, fp, "REM2");
		}
	}
	else if (FunctionId == TAG_FUNC_DEICE_REMOTE3) {
		if (master && AtcMe) {
			setDeice(deIceTaxiRem3Name, fp, "REM3");
		}
	}
	else if (FunctionId == TAG_FUNC_DEICE_REMOTE4) {
		if (master && AtcMe) {
			setDeice(deIceTaxiRem4Name , fp, "REM4");
		}
	}
	else if (FunctionId == TAG_FUNC_DEICE_REMOTE5) {
		if (master && AtcMe) {
			setDeice(deIceTaxiRem5Name, fp, "REM5");
		}
	}
	else if (FunctionId == TAG_FUNC_NETWORK_STATUS_OPTIONS) {
		if (AtcMe) {
			addLogLine("TRIGGER - TAG_FUNC_NETWORK_STATUS_OPTIONS");

			//Get actual status
			string status = "";
			std::lock_guard<std::mutex> lock(networkStatusMutex);
			for (size_t i = 0; i < networkStatus.size(); i++) {
				if (networkStatus[i][0] == fp.GetCallsign()) {
					status = networkStatus[i][1];
				}
			}

			//Check has CTOT
			bool hasCtot = false;
			for (size_t i = 0; i < slotList.size(); i++)
			{
				if (slotList[i].callsign == fp.GetCallsign()) {
					if (slotList[i].hasManualCtot && slotList[i].ctot != "") {
						hasCtot = true;
					}
				}
			}

			if (hasCtot && status == "") {
				OpenPopupList(Area, "CDM-Network", 1);
				if (status != "REA") {
					AddPopupListElement("Set REA", "", TAG_FUNC_NETWORK_SET_REA, false, 2, false);
				}
			} else if (status == "REA") {
				OpenPopupList(Area, "CDM-Network", 1);
				AddPopupListElement("Remove REA", "", TAG_FUNC_NETWORK_REMOVE_REA, false, 2, false);
			}
		}
	}
	else if (FunctionId == TAG_FUNC_NETWORK_SET_REA) {
		if (AtcMe) {
			addLogLine("TRIGGER - TAG_FUNC_NETWORK_SET_REA");
			std::thread t3(&CDM::setCdmSts, this, fp.GetCallsign(), "REA/1");
			t3.detach();
		}
	}
	else if (FunctionId == TAG_FUNC_NETWORK_REMOVE_REA) {
		if (AtcMe) {
			addLogLine("TRIGGER - TAG_FUNC_NETWORK_REMOVE_REA");
			std::thread t3(&CDM::setCdmSts, this, fp.GetCallsign(), "REA/0");
			t3.detach();
		}
	}

	else if (FunctionId == TAG_FUNC_CTOTOPTIONS) {
		if (master && AtcMe) {
			addLogLine("TRIGGER - TAG_FUNC_CTOTOPTIONS");
			Plane plane;
			for (size_t i = 0; i < slotList.size(); i++)
			{
				if (slotList[i].callsign == fp.GetCallsign()) {
					plane = slotList[i];
				}
			}
			OpenPopupList(Area, "CTOT Options", 1);
			if (!plane.hasManualCtot || plane.ctot != "") {
				AddPopupListElement("Set Manual CTOT", "", TAG_FUNC_EDITMANCTOT, false, 2, false);
				if (flightHasCtotDisabled(fp.GetCallsign())) {
					AddPopupListElement("Enable CDM-Network", "", TAG_FUNC_ENABLECTOT, false, 2, false);
				}
				else {
					AddPopupListElement("Disable CDM-Network", "", TAG_FUNC_DISABLECTOT, false, 2, false);
				}
			}
			else if (plane.ctot == "" && plane.hasManualCtot) {
				AddPopupListElement("Remove Manual CTOT", "", TAG_FUNC_REMOVEMANCTOT, false, 2, false);
				if (flightHasCtotDisabled(fp.GetCallsign())) {
					AddPopupListElement("Enable CDM-Network", "", TAG_FUNC_ENABLECTOT, false, 2, false);
				}
				else {
					AddPopupListElement("Disable CDM-Network", "", TAG_FUNC_DISABLECTOT, false, 2, false);
				}
			}
		}
	}
	else if (FunctionId == TAG_FUNC_TOGGLEREAMSG) {
		addLogLine("TRIGGER - TAG_FUNC_TOGGLEREAMSG");
		toggleReaMsg(fp, true);
	}
	else if (FunctionId == TAG_FUNC_REMOVECTOT) {
		addLogLine("TRIGGER - TAG_FUNC_REMOVECTOT");
		for (size_t i = 0; i < slotList.size(); i++)
		{
			if (slotList[i].callsign == fp.GetCallsign()) {
				slotList[i].ctot = "";
			}
		}
		//Update times to slaves
		countTime = std::time(nullptr) - (refreshTime+5);
		countTimeNonCdm = std::time(nullptr) - (refreshTime + 5);
	}

	else if (FunctionId == TAG_FUNC_OPT_TTOT) {
		if (master && AtcMe) {
			addLogLine("TRIGGER - TAG_FUNC_OPT_TTOT");
			OpenPopupList(Area, "TTOT Options", 1);
			//CDT OPTIONS
			bool planeFound = false;
			Plane plane;
			for (size_t i = 0; i < slotList.size(); i++)
			{
				if (slotList[i].callsign == fp.GetCallsign()) {
					planeFound = true;
					plane = slotList[i];
				}
			}

			if (planeFound) {
				if (plane.hasManualCtot) {
					AddPopupListElement("Edit Custom CDT", "", TAG_FUNC_EDITCDT, false, 2, false);
				}
				else {
					AddPopupListElement("Set Custom CDT", "", TAG_FUNC_EDITCDT, false, 2, false);
				}
			}
			else {
				AddPopupListElement("Set Custom CDT", "", TAG_FUNC_EDITCDT, false, 2, false);
			}
		}
	}

	else if (FunctionId == TAG_FUNC_OPT) {
		if (master && AtcMe) {
			addLogLine("TRIGGER - TAG_FUNC_OPT");
			OpenPopupList(Area, "CDM - Options", 1);
			//EOBT OPTIONS
			AddPopupListElement("Edit EOBT", "", TAG_FUNC_EDITEOBT, false, 2, false);
			AddPopupListElement("----------------", "", -1, false, 2, false);

			//TOBT OPTIONS
			if ((string)fp.GetGroundState() != "STUP" && (string)fp.GetGroundState() != "ST-UP" && (string)fp.GetGroundState() != "PUSH" && (string)fp.GetGroundState() != "TAXI" && (string)fp.GetGroundState() != "DEPA") {
				AddPopupListElement("Ready TOBT", "", TAG_FUNC_READYTOBT, false, 2, false);
				AddPopupListElement("Edit TOBT", "", TAG_FUNC_EDITTOBT, false, 2, false);
				AddPopupListElement("----------------", "", -1, false, 2, false);
			}

			//TSAC OPTIONS
			string tsacvalue = getFlightStripInfo(fp, 1);
			if (tsacvalue.empty()) {
				AddPopupListElement("Add TSAT to TSAC", "", TAG_FUNC_ADDTSAC, false, 2, false);
			}
			else {
				AddPopupListElement("Remove TSAC", "", TAG_FUNC_REMOVETSAC, false, 2, false);
			}
			AddPopupListElement("Edit TSAC", "", TAG_FUNC_EDITTSAC, false, 2, false);
			AddPopupListElement("----------------", "", -1, false, 2, false);

			//ASRT OPTIONS
			string asrtvalue = getFlightStripInfo(fp, 0);
			if (asrtvalue.empty()) {
				AddPopupListElement("Set RSTUP State", "", TAG_FUNC_READYSTARTUP, false, 2, false);
			}
			else {
				AddPopupListElement("Remove RSTUP State", "", TAG_FUNC_READYSTARTUP, false, 2, false);
			}
			AddPopupListElement("----------------", "", -1, false, 2, false);

			//CDT OPTIONS
			bool planeFound = false;
			Plane plane;
			for (size_t i = 0; i < slotList.size(); i++)
			{
				if (slotList[i].callsign == fp.GetCallsign()) {
					planeFound = true;
					plane = slotList[i];
				}
			}

			if (planeFound) {
				if (plane.hasManualCtot) {
					AddPopupListElement("Edit Custom CDT", "", TAG_FUNC_EDITCDT, false, 2, false);
				}
				else {
					AddPopupListElement("Set Custom CDT", "", TAG_FUNC_EDITCDT, false, 2, false);
				}
			}
			else {
				AddPopupListElement("Set Custom CDT", "", TAG_FUNC_EDITCDT, false, 2, false);
			}

			//CTOT OPTIONS
			if (!plane.hasManualCtot || plane.ctot != "") {
				AddPopupListElement("Set Manual CTOT", "", TAG_FUNC_EDITMANCTOT, false, 2, false);
				if (flightHasCtotDisabled(fp.GetCallsign())) {
					AddPopupListElement("Enable CDM-Network", "", TAG_FUNC_ENABLECTOT, false, 2, false);
				}
				else {
					AddPopupListElement("Disable CDM-Network", "", TAG_FUNC_DISABLECTOT, false, 2, false);
				}
			}
			else if (plane.ctot == "" && plane.hasManualCtot) {
				AddPopupListElement("Remove Manual CTOT", "", TAG_FUNC_REMOVEMANCTOT, false, 2, false);
				if (flightHasCtotDisabled(fp.GetCallsign())) {
					AddPopupListElement("Enable CDM-Network", "", TAG_FUNC_ENABLECTOT, false, 2, false);
				}
				else {
					AddPopupListElement("Disable CDM-Network", "", TAG_FUNC_DISABLECTOT, false, 2, false);
				}
			}
		}
	}

	else if (FunctionId == TAG_FUNC_OPT_TOBT) {
		if (AtcMe) {
			if ((string)fp.GetGroundState() != "STUP" && (string)fp.GetGroundState() != "ST-UP" && (string)fp.GetGroundState() != "PUSH" && (string)fp.GetGroundState() != "TAXI" && (string)fp.GetGroundState() != "DEPA") {
				addLogLine("TRIGGER - TAG_FUNC_OPT_TOBT");
				OpenPopupList(Area, "TOBT Options", 1);
				AddPopupListElement("Ready TOBT", "", TAG_FUNC_READYTOBT, false, 2, false);
				AddPopupListElement("Edit TOBT", "", TAG_FUNC_EDITTOBT, false, 2, false);
			}
		}
	}

	else if (FunctionId == TAG_FUNC_OPT_EOBT) {
		//Can be modified as non-master
		if (AtcMe) {
			addLogLine("TRIGGER - TAG_FUNC_OPT_EOBT");
			OpenPopupList(Area, "EOBT Options", 1);
			AddPopupListElement("Edit EOBT", "", TAG_FUNC_EDITEOBT, false, 2, false);
		}
	}

	else if (FunctionId == TAG_FUNC_OPT_ETOBT) {
		addLogLine("TRIGGER - TAG_FUNC_OPT_ETOBT");
		bool isCDMairport = false;
		for (string a : CDMairports)
		{
			if (fp.GetFlightPlanData().GetOrigin() == a) {
				isCDMairport = true;
			}
		}
		if (!isCDMairport && AtcMe) {
			addLogLine("TRIGGER - EOBT options");
			OpenPopupList(Area, "E/TOBT Options", 1);
			AddPopupListElement("Edit EOBT", "", TAG_FUNC_EDITEOBT, false, 2, false);
		}
		else if (AtcMe) {
			addLogLine("TRIGGER - TOBT options");
			OpenPopupList(Area, "E/TOBT Options", 1);
			AddPopupListElement("Ready TOBT", "", TAG_FUNC_READYTOBT, false, 2, false);
			AddPopupListElement("Edit TOBT", "", TAG_FUNC_EDITTOBT, false, 2, false);
		}
	}

	else if (FunctionId == TAG_FUNC_OPT_TSAC) {
		if (master && AtcMe) {
			addLogLine("TRIGGER - TAG_FUNC_OPT_TSAC");
			OpenPopupList(Area, "TSAC Options", 1);
			string tsacvalue = getFlightStripInfo(fp, 1);
			AddPopupListElement("Add TSAT to TSAC", "", TAG_FUNC_ADDTSAC, false, 2, false);
			if (!tsacvalue.empty()) {
				AddPopupListElement("Remove TSAC", "", TAG_FUNC_REMOVETSAC, false, 2, false);
			}
			AddPopupListElement("Edit TSAC", "", TAG_FUNC_EDITTSAC, false, 2, false);
		}
	}

	else if (FunctionId == TAG_FUNC_OPT_EvCTOT) {
	if (master && AtcMe) {
		addLogLine("TRIGGER - TAG_FUNC_OPT_EvCTOT");
		OpenPopupList(Area, "Event CTOT Options", 1);
		AddPopupListElement("Add Event CTOT as MAN CTOT", "", TAG_FUNC_EvCTOTtoCTOT, false, 2, false);
	}
	}

	else if (FunctionId == TAG_FUNC_READYTOBT) {
		try{
		if ((string)fp.GetGroundState() != "STUP" && (string)fp.GetGroundState() != "ST-UP" && (string)fp.GetGroundState() != "PUSH" && (string)fp.GetGroundState() != "TAXI" && (string)fp.GetGroundState() != "DEPA") {
				if (master && AtcMe) {
				addLogLine("TRIGGER - TAG_FUNC_READYTOBT");
				//SET SU_WAIT WHEN OPTION ENABLED
				if (option_su_wait) {
					suWaitList.push_back(fp.GetCallsign());
				}

				if (getFlightStripInfo(fp, 2) != formatTime(GetActualTime())) {

					setFlightStripInfo(fp, formatTime(GetActualTime()), 2);

					//Get Time now
					time_t rawtime;
					struct tm ptm;
					time(&rawtime);
					gmtime_s(&ptm, &rawtime);
					string hour = to_string(ptm.tm_hour % 24);
					string min = to_string(ptm.tm_min);

					if (stoi(min) < 10) {
						min = "0" + min;
					}
					if (stoi(hour) < 10) {
						hour = "0" + hour.substr(0, 1);
					}

					string annotAsrt = getFlightStripInfo(fp, 0);
					if (annotAsrt.empty()) {
						setFlightStripInfo(fp, (hour + min), 0);
					}

					//Update TOBT-setBy
					bool found = false;
					for (int a = reqTobtTypes.size() - 1; a >= 0; --a) {
						if (reqTobtTypes[a][0] == fp.GetCallsign()) {
							found = true;
							if (reqTobtTypes[a][1] != "ATC") {
								reqTobtTypes[a][1] = "ATC";
							}
						}
					}
					if (!found) {
						reqTobtTypes.push_back({ fp.GetCallsign(), "ATC" });
					}

					//Set TSAC 9999 to later set the correct TSAT when calculation of TSAT completed
					if (readySetTsac) setFlightStripInfo(fp, "9999", 1);

					//Set REA Status
					std::thread t99(&CDM::setCdmSts, this, fp.GetCallsign(), "REA/1");
					t99.detach();
				}
			}
			else if (AtcMe) {
				addLogLine("TRIGGER - TAG_FUNC_READYTOBT_SLAVE");
				if (getFlightStripInfo(fp, 2) != formatTime(GetActualTime())) {

					setFlightStripInfo(fp, formatTime(GetActualTime()), 2);

					//Get Time now
					time_t rawtime;
					struct tm ptm;
					time(&rawtime);
					gmtime_s(&ptm, &rawtime);
					string hour = to_string(ptm.tm_hour % 24);
					string min = to_string(ptm.tm_min);

					if (stoi(min) < 10) {
						min = "0" + min;
					}
					if (stoi(hour) < 10) {
						hour = "0" + hour.substr(0, 1);
					}

					//Set REQ TOBT
					std::thread t99(&CDM::setCdmSts, this, fp.GetCallsign(), "REQTOBT/" + hour + min + "/ATC");
					t99.detach();
				}
			}
		}
		}
		catch (const std::exception& ex) {
			addLogLine(string("EXCEPTION in TAG_FUNC_READYTOBT: ") + ex.what());
		}
		catch (...) {
			addLogLine("UNKNOWN EXCEPTION in TAG_FUNC_READYTOBT");
		}
	}

	else if (FunctionId == TAG_FUNC_EDITCDT) {
		if (master && AtcMe) {
			addLogLine("TRIGGER - TAG_FUNC_EDITCDT");
			bool found = false;
			string ttot;
			for (size_t i = 0; i < slotList.size(); i++)
			{
				if (slotList[i].callsign == fp.GetCallsign()) {
					if (!slotList[i].ttot.empty()) {
						found = true;
						ttot = slotList[i].ttot.substr(0, 4);
					}
				}
			}
			if (found) {
				OpenPopupEdit(Area, TAG_FUNC_TRY_TO_SET_CDT, ttot.c_str());
			}
			else {
				OpenPopupEdit(Area, TAG_FUNC_TRY_TO_SET_CDT, "");
			}
		}
	}

	else if (FunctionId == TAG_FUNC_TRY_TO_SET_CDT) {
		if (master && AtcMe) {
			addLogLine("TRIGGER - TAG_FUNC_TRY_TO_SET_CDT");
			//only before start-up/push back
			const string groundState = fp.GetGroundState();
			if (groundState != "STUP" && groundState != "ST-UP" && groundState != "PUSH" && groundState != "TAXI" && groundState != "DEPA") {
				string editedCDT = ItemString;
				bool hasNoNumber = true;
				if (editedCDT.length() == 4) {
					for (size_t i = 0; i < editedCDT.length(); i++) {
						if (isdigit(editedCDT[i]) == false) {
							hasNoNumber = false;
						}
					}
					if (hasNoNumber) {
						//First, Re-order main list
						slotList = recalculateSlotList(slotList);

						string callsign = fp.GetCallsign();
						string depRwy = fp.GetFlightPlanData().GetDepartureRwy(); boost::to_upper(depRwy);
						if (RadarTargetSelect(callsign.c_str()).IsValid() && depRwy.length() > 0) {
							double lat = RadarTargetSelect(callsign.c_str()).GetPosition().GetPosition().m_Latitude;
							double lon = RadarTargetSelect(callsign.c_str()).GetPosition().GetPosition().m_Longitude;
							int deIceTime = addDeIceTime(callsign, fp.GetFlightPlanData().GetAircraftWtc());
							string myTaxiTime = getTaxiTime(lat, lon, fp.GetFlightPlanData().GetOrigin(), depRwy, deIceTime, callsign);
							string calculatedTOBT = calculateLessTime(editedCDT + "00", stod(myTaxiTime));
							// at the earlierst at present time + EXOT
							if (stoi(calculatedTOBT) > stoi(GetTimeNow())) {
								setFlightStripInfo(fp, calculatedTOBT.substr(0, 4), 2);
								for (size_t i = 0; i < slotList.size(); i++)
								{
									if ((string)fp.GetCallsign() == slotList[i].callsign && !slotList[i].hasManualCtot) {
										slotList[i].ttot = editedCDT + "00";
									}
								}
							}
						}
					}
				}
			}
		}
	}

	else if (FunctionId == TAG_FUNC_EvCTOTtoCTOT) {
		if (master && AtcMe) {
			addLogLine("TRIGGER - TAG_FUNC_EvCTOTtoCTOT");
			//only before start-up/push back
			const string groundState = fp.GetGroundState();
			if (groundState != "STUP" && groundState != "ST-UP" && groundState != "PUSH" && groundState != "TAXI" && groundState != "DEPA") {
				bool hasEvCTOT = false;
				string editedCTOT = "";
				for (size_t i = 0; i < evCtots.size(); i++) {
					if (evCtots[i][0] == fp.GetCallsign()) {
						if (evCtots[i][1] != "") {
							hasEvCTOT = true;
							editedCTOT = evCtots[i][1];
						}
					}
				}

				if (hasEvCTOT) {
					bool hasNoNumber = true;
					if (editedCTOT.length() == 4) {
						for (size_t i = 0; i < editedCTOT.length(); i++) {
							if (isdigit(editedCTOT[i]) == false) {
								hasNoNumber = false;
							}
						}

						if (hasNoNumber) {
							slotList = recalculateSlotList(slotList);
							string callsign = fp.GetCallsign();
							string depRwy = fp.GetFlightPlanData().GetDepartureRwy(); boost::to_upper(depRwy);
							if (RadarTargetSelect(callsign.c_str()).IsValid() && depRwy.length() > 0) {
								double lat = RadarTargetSelect(callsign.c_str()).GetPosition().GetPosition().m_Latitude;
								double lon = RadarTargetSelect(callsign.c_str()).GetPosition().GetPosition().m_Longitude;
								int deIceTime = addDeIceTime(callsign, fp.GetFlightPlanData().GetAircraftWtc());
								string myTaxiTime = getTaxiTime(lat, lon, fp.GetFlightPlanData().GetOrigin(), depRwy, deIceTime, callsign);
								string calculatedTOBT = calculateLessTime(editedCTOT + "00", stod(myTaxiTime));
								// at the earlierst at present time + EXOT
								if (stoi(calculatedTOBT) > stoi(GetTimeNow())) {
									setFlightStripInfo(fp, calculatedTOBT.substr(0, 4), 2);
									setFlightStripInfo(fp, "1", 7);
									for (size_t i = 0; i < slotList.size(); i++)
									{
										if (slotList[i].callsign == fp.GetCallsign()) {
											slotList[i].hasManualCtot = true;
											addTimeToListForSpecificAirportAndRunway(10, calculateTime(GetTimeNow(), 5), fp.GetFlightPlanData().GetOrigin(), fp.GetFlightPlanData().GetDepartureRwy());
										}
									}
									//Update times to slaves
									countTime = std::time(nullptr) - refreshTime;
									countTimeNonCdm = std::time(nullptr) - refreshTime;
								}
							}
						}
					}
				}
			}
		}
	}

	else if (FunctionId == TAG_FUNC_DISABLECTOT) {
		if (master && AtcMe) {
			addLogLine("TRIGGER - TAG_FUNC_DISABLECTOT");
			bool found = false;
			for (string callsign : disabledCtots) {
				if (callsign == fp.GetCallsign()) {
					found = true;
				}
			}
			if (!found) {
				addLogLine("Disabled CTOT for: " + (string)fp.GetCallsign());
				disabledCtots.push_back(fp.GetCallsign());
			}
		}
	}
	else if (FunctionId == TAG_FUNC_ENABLECTOT) {
	if (master && AtcMe) {
		addLogLine("TRIGGER - TAG_FUNC_ENABLECTOT");
		bool found = false;
		for (int i = 0; i < disabledCtots.size(); i++) {
			if (disabledCtots[i] == fp.GetCallsign()) {
				disabledCtots.erase(disabledCtots.begin() + i);
			}
		}
	}
	}
	else if (FunctionId == TAG_FUNC_EDITMANCTOT) {
		if (master && AtcMe) {
			addLogLine("TRIGGER - TAG_FUNC_EDITMANCTOT");
			bool found = false;
			for (size_t i = 0; i < slotList.size(); i++)
			{
				if (slotList[i].callsign == fp.GetCallsign()) {
					if (!slotList[i].ttot.empty()) {
						found = true;
					}
				}
			}
			if (found) {
				OpenPopupEdit(Area, TAG_FUNC_MODIFYMANCTOT, "");
			}
			else {
				OpenPopupEdit(Area, TAG_FUNC_MODIFYMANCTOT, "");
			}
		}
	}

	else if (FunctionId == TAG_FUNC_MODIFYMANCTOT) {
		if (master && AtcMe) {
			addLogLine("TRIGGER - TAG_FUNC_MODIFYMANCTOT");
			//only before start-up/push back
			const string groundState = fp.GetGroundState();
			if (groundState != "STUP" && groundState != "ST-UP" && groundState != "PUSH" && groundState != "TAXI" && groundState != "DEPA") {
				string editedCTOT = ItemString;
				bool hasNoNumber = true;
				if (editedCTOT.length() == 4) {
					for (size_t i = 0; i < editedCTOT.length(); i++) {
						if (isdigit(editedCTOT[i]) == false) {
							hasNoNumber = false;
						}
					}
					if (hasNoNumber) {
						//First, Re-order main list
						slotList = recalculateSlotList(slotList);

						string callsign = fp.GetCallsign();
						string depRwy = fp.GetFlightPlanData().GetDepartureRwy(); boost::to_upper(depRwy);
						if (RadarTargetSelect(callsign.c_str()).IsValid() && depRwy.length() > 0) {
							double lat = RadarTargetSelect(callsign.c_str()).GetPosition().GetPosition().m_Latitude;
							double lon = RadarTargetSelect(callsign.c_str()).GetPosition().GetPosition().m_Longitude;
							int deIceTime = addDeIceTime(callsign, fp.GetFlightPlanData().GetAircraftWtc());
							string myTaxiTime = getTaxiTime(lat, lon, fp.GetFlightPlanData().GetOrigin(), depRwy, deIceTime, callsign);
							string calculatedTOBT = calculateLessTime(editedCTOT + "00", stod(myTaxiTime));
							// at the earlierst at present time + EXOT
							if (stoi(calculatedTOBT) > stoi(GetTimeNow())) {
								setFlightStripInfo(fp, calculatedTOBT.substr(0, 4), 2);
								setFlightStripInfo(fp, "1", 7);
								for (size_t i = 0; i < slotList.size(); i++)
								{
									if ((string)fp.GetCallsign() == slotList[i].callsign && !slotList[i].hasManualCtot) {
										slotList[i].hasManualCtot = true;
										slotList[i].ttot = editedCTOT + "00";
										//Delay all aircraft to adjust sequence.
										//addTimeToListForSpecificAirportAndRunway(10, calculateTime(GetTimeNow(), 5), fp.GetFlightPlanData().GetOrigin(), fp.GetFlightPlanData().GetDepartureRwy());
									}
								}
							}
							//Update times to slaves
							countTime = std::time(nullptr) - refreshTime;
							countTimeNonCdm = std::time(nullptr) - refreshTime;
						}
					}
				}
			}
		}
	}

	else if (FunctionId == TAG_FUNC_REMOVEMANCTOT) {
		if (master && AtcMe) {
			addLogLine("TRIGGER - TAG_FUNC_REMOVEMANCTOT");
			for (size_t i = 0; i < slotList.size(); i++)
			{
				if (slotList[i].callsign == fp.GetCallsign()) {
					if (slotList[i].hasManualCtot) {
						slotList[i].hasManualCtot = false;
						if (serverEnabled) {
							for (size_t a = 0; a < slotList.size(); a++) {
								if (slotList[a].callsign == (string)fp.GetCallsign()) {
									slotList[a].showData = false;
								}
							}
							//Check API
							std::thread t(&CDM::setOBTApi, this, slotList[i].callsign, slotList[i].tsat, true, false);
							t.detach();
						}

						if (!isCdmAirport(fp.GetFlightPlanData().GetOrigin())) {
							slotList[i].ttot = "";
						}
					}
				}
			}
		}
	}

	else if (FunctionId == TAG_FUNC_EDITTOBT) {
		if (AtcMe) {
			if ((string)fp.GetGroundState() != "STUP" && (string)fp.GetGroundState() != "ST-UP" && (string)fp.GetGroundState() != "PUSH" && (string)fp.GetGroundState() != "TAXI" && (string)fp.GetGroundState() != "DEPA") {
				addLogLine("TRIGGER - TAG_FUNC_EDITTOBT");
				OpenPopupEdit(Area, TAG_FUNC_NEWTOBT, getFlightStripInfo(fp, 2).c_str());
			}
		}
	}
	else if (FunctionId == TAG_FUNC_NEWTOBT) {
		try {
		if ((string)fp.GetGroundState() != "STUP" && (string)fp.GetGroundState() != "ST-UP" && (string)fp.GetGroundState() != "PUSH" && (string)fp.GetGroundState() != "TAXI" && (string)fp.GetGroundState() != "DEPA") {
			addLogLine("TRIGGER - TAG_FUNC_NEWTOBT");
			string editedTOBT = ItemString;
			string setBy = "NONE";
			if (getFlightStripInfo(fp, 2) != editedTOBT) {
				bool hasNoNumber = true;
				if (editedTOBT.length() == 4) {
					for (size_t i = 0; i < editedTOBT.length(); i++) {
						if (isdigit(editedTOBT[i]) == false) {
							hasNoNumber = false;
						}
					}

					if (hasNoNumber) {
						if (!master) {
							if (editedTOBT.length() == 4) {
								//Set REQ TOBT
								std::thread t99(&CDM::setCdmSts, this, fp.GetCallsign(), "REQTOBT/" + editedTOBT + "/ATC");
								t99.detach();
							}
						} else {
							int hours = stoi(editedTOBT.substr(0, 2));
							int minutes = stoi(editedTOBT.substr(2, 2));
							if (hours >= 0 && hours <= 23 && minutes >= 0 && minutes <= 59) {
								bool found = false;
								for (size_t i = 0; i < slotList.size(); i++)
								{
									if (slotList[i].callsign == fp.GetCallsign()) {
										found = true;
										setFlightStripInfo(fp, editedTOBT, 2);
									}
								}
								if (!found) {
									setFlightStripInfo(fp, editedTOBT, 2);
								}
								setBy = "ATC";
							}
						}
					}
				}
				else if (editedTOBT.empty() && master && AtcMe) {
					setFlightStripInfo(fp, "", 0);
					setFlightStripInfo(fp, "", 2);
					for (size_t i = 0; i < slotList.size(); i++) {
						if ((string)fp.GetCallsign() == slotList[i].callsign) {
							slotList.erase(slotList.begin() + i);
							//Update times to slaves
							setFlightStripInfo(fp, "", 3);
							setFlightStripInfo(fp, "", 4);
							PushToOtherControllers(fp);
						}
					}

					//if (!realMode) {
						//Check API
					setBy = "";

					if (serverEnabled) {
						//Hide calculation
						for (size_t a = 0; a < slotList.size(); a++) {
							if (slotList[a].callsign == (string)fp.GetCallsign()) {
								slotList[a].showData = false;
							}
						}
						std::thread t(&CDM::setOBTApi, this, (string)fp.GetCallsign(), "", true, false);
						t.detach();
					}
					//}
				}

				//Update TOBT-setBy
				if (setBy != "NONE" && master && AtcMe) {
					bool found = false;
					for (int a = reqTobtTypes.size() - 1; a >= 0; --a) {
						if (reqTobtTypes[a][0] == fp.GetCallsign()) {
							found = true;
							if (reqTobtTypes[a][1] != setBy) {
								reqTobtTypes[a][1] = setBy;
							}
						}
					}
					if (!found) {
						reqTobtTypes.push_back({ fp.GetCallsign(), setBy });
					}
				}
			}
		}
		}
		catch (const std::exception& ex) {
			addLogLine(string("EXCEPTION in TAG_FUNC_NEWTOBT: ") + ex.what());
		}
		catch (...) {
			addLogLine("UNKNOWN EXCEPTION in TAG_FUNC_NEWTOBT");
		}
	}

	}
	catch (const std::exception& e) {
		addLogLine("ERROR: Unhandled exception OnFunctionCall: " + (string)e.what());
	}
	catch (...) {
		addLogLine("ERROR: Unhandled exception OnFunctionCall");
	}
}

void CDM::OnFlightPlanDisconnect(CFlightPlan FlightPlan) {
	if (FlightPlan.IsValid()) {
		string tsat = getFlightStripInfo(FlightPlan, 3);
		if (tsat != "") {
			disconnectionList.push_back(FlightPlan.GetCallsign());
			countTfcDisconnection = 1;
			countTfcDisconnectionTime = std::time(nullptr);
		}
		else {
			RemoveDataFromTfc(FlightPlan.GetCallsign());
		}
	}
}


void CDM::OnGetTagItem(CFlightPlan FlightPlan, CRadarTarget RadarTarget, int ItemCode, int TagData, char sItemString[16], int* pColorCode, COLORREF* pRGB, double* pFontSize)
{
	try {
	if (ItemCode == TAG_ITEM_TSAT || ItemCode == TAG_ITEM_TSAT_TOBT_DIFF) {
		//Check master status
		if (myAtcCallsign != ControllerMyself().GetCallsign()) {
			if (myAtcCallsign == "") {
				myAtcCallsign = ControllerMyself().GetCallsign();
			}
			else {
				RemoveMasterAirports();
			}
		}
	}

	COLORREF ItemRGB = 0xFFFFFFFF;
	string callsign = FlightPlan.GetCallsign();

	string origin = FlightPlan.GetFlightPlanData().GetOrigin(); boost::to_upper(origin);
	string destination = FlightPlan.GetFlightPlanData().GetDestination(); boost::to_upper(destination);

	string depRwy = FlightPlan.GetFlightPlanData().GetDepartureRwy(); boost::to_upper(depRwy);
	bool isVfr = false;
	if (strcmp(FlightPlan.GetFlightPlanData().GetPlanType(), "V") > -1) {
		isVfr = true;
	}

	if (ItemCode == TAG_ITEM_OPTIONS) {
		ItemRGB = TAG_GREY;
		strcpy_s(sItemString, 16, "->");
	}

	std::vector<std::vector<std::string>> myNetworkStatus;
	{
		std::lock_guard<std::mutex> lock(networkStatusMutex);
		myNetworkStatus = networkStatus;
	}


	if (!isVfr) {

		//Refresh ecfmpData every 5 min
		time_t timeNow = std::time(nullptr);
		if ((timeNow - countEcfmpTime) > 300 && !refresh2) {
			refresh2 = true;
			countEcfmpTime = timeNow;
			std::thread t(&CDM::refreshActions2, this);
			t.detach();
			if (debugMode) {
				sendMessage("[DEBUG MESSAGE] - REFRESHING FLOW DATA");
			}
		}
		//Refresh ecfmpData every <refreshTime> min
		if ((timeNow - countNetworkTobt) > refreshTime) {
			countNetworkTobt = timeNow;
			std::thread t(&CDM::getNetworkTobt, this);
			t.detach();
			if (debugMode) {
				sendMessage("[DEBUG MESSAGE] - REFRESHING FLOW DATA");
			}
		}
		if ((timeNow - countFetchServerTime) > 15 && !refresh3) {
			refresh3 = true;
			countFetchServerTime = timeNow;
			std::thread t(&CDM::refreshActions3, this);
			t.detach();
			if (debugMode) {
				sendMessage("[DEBUG MESSAGE] - REFRESHING CDM API DATA 1");
			}
		}
		if ((timeNow - countRefreshActions4Time) > 10 && !refresh4) {
			refresh4 = true;
			countRefreshActions4Time = timeNow;
			std::thread t(&CDM::refreshActions4, this);
			t.detach();
			if (debugMode) {
				sendMessage("[DEBUG MESSAGE] - REFRESHING CDM API DATA 2");
			}
		}

		bool isCDMairport = false;
		for (string a : CDMairports)
		{
			if (origin == a) {
				isCDMairport = true;
			}
		}

		if (isCDMairport)
		{
			//If aircraft is in aircraftFind Base vector
			int pos = -1;
			bool aircraftFind = false;
			for (size_t i = 0; i < slotList.size(); i++) {
				if (callsign == slotList[i].callsign) {
					aircraftFind = true;
					pos = i;
				}
			}
			const char* EOBT = "";
			const char* TSAT = "";
			const char* TTOT = "";
			int taxiTime = defTaxiTime;

			//Check if update in the queue
			std::vector<Plane> localPlaneQueue;
			{
				std::lock_guard<std::mutex> lock(apiQueueResponseMutex);
				localPlaneQueue.swap(apiQueueResponse);
				apiQueueResponse.clear();
			}

			if (!localPlaneQueue.empty()) {
				for (const Plane p : localPlaneQueue) {
					for (int t = 0; t < slotList.size(); t++) {
						if (p.callsign == slotList[t].callsign) {
							slotList[t] = p;
						}
					}
				}
			}

			// Get Taxi times
			int TaxiTimePos = 0;
			bool planeHasTaxiTimeAssigned = false;
			for (size_t j = 0; j < taxiTimesList.size(); j++)
			{
				if (taxiTimesList[j].substr(0, taxiTimesList[j].find(",")) == callsign) {
					planeHasTaxiTimeAssigned = true;
					TaxiTimePos = j;
				}
			}

			// Check if runway changed
			if (aircraftFind && (ItemCode == TAG_ITEM_TSAT || ItemCode == TAG_ITEM_TSAT_TOBT_DIFF)) {
				if (planeHasTaxiTimeAssigned) {
					if (taxiTimesList[TaxiTimePos].substr(taxiTimesList[TaxiTimePos].find(",") + 3, 1) == ",") {
						if (depRwy != taxiTimesList[TaxiTimePos].substr(taxiTimesList[TaxiTimePos].find(",") + 1, 2)) {
							planeHasTaxiTimeAssigned = false;
							taxiTimesList.erase(taxiTimesList.begin() + TaxiTimePos);
							slotList.erase(slotList.begin() + pos);
							aircraftFind = false;
						}
					}
					else if (taxiTimesList[TaxiTimePos].substr(taxiTimesList[TaxiTimePos].find(",") + 4, 1) == ",") {
						if (depRwy != taxiTimesList[TaxiTimePos].substr(taxiTimesList[TaxiTimePos].find(",") + 1, 3)) {
							planeHasTaxiTimeAssigned = false;
							taxiTimesList.erase(taxiTimesList.begin() + TaxiTimePos);
						}
					}
				}
			}

			if (!planeHasTaxiTimeAssigned) {
				if (RadarTargetSelect(callsign.c_str()).IsValid() && depRwy.length() > 0) {
					double lat = RadarTargetSelect(callsign.c_str()).GetPosition().GetPosition().m_Latitude;
					double lon = RadarTargetSelect(callsign.c_str()).GetPosition().GetPosition().m_Longitude;
					if (debugMode) {
						sendMessage("[DEBUG MESSAGE] - " + callsign + " LAT: " + to_string(lat) + " LON: " + to_string(lon) + " DEP RWY: " + depRwy);
					}
					int deIceTime = addDeIceTime(callsign, FlightPlan.GetFlightPlanData().GetAircraftWtc());
					string myTaxiTime = getTaxiTime(lat, lon, origin, depRwy, deIceTime, callsign);
					taxiTimesList.push_back(callsign + "," + depRwy + "," + myTaxiTime);
					planeHasTaxiTimeAssigned = true;
					TaxiTimePos = taxiTimesList.size() - 1;
				}
			}

			if (planeHasTaxiTimeAssigned) {
				if (taxiTimesList[TaxiTimePos].substr(taxiTimesList[TaxiTimePos].length() - 2, 1) == ",") {
					taxiTime = stoi(taxiTimesList[TaxiTimePos].substr(taxiTimesList[TaxiTimePos].length() - 1, 1));
				}
				else {
					taxiTime = stoi(taxiTimesList[TaxiTimePos].substr(taxiTimesList[TaxiTimePos].length() - 2, 2));
				}
			}

			//Check if update in the queue
			std::vector<vector<string>> localTobtTypesQueue;
			{
				std::lock_guard<std::mutex> lock(reqTobtTypesQueueMutex);
				localTobtTypesQueue.swap(reqTobtTypesQueue);				
				reqTobtTypesQueue.clear();
			}

			if (!localTobtTypesQueue.empty()) {
				bool found = false;
				for (vector<string> s : localTobtTypesQueue) {
					found = false;
					for (int a = reqTobtTypes.size() - 1; a >= 0; --a) {
						if (reqTobtTypes[a][0] == s[0]) {
							found = true;
							if (reqTobtTypes[a][1] != s[1] && s[1] != "") {
								reqTobtTypes[a][1] = s[1];
							}
						}
					}
					if (!found) {
						if (s[1] != "") {
							reqTobtTypes.push_back(s);
						}
					}
				}
			}

			if (ctotCid) {
				bool evCtotFound = false;
				for (size_t i = 0; i < evCtots.size(); i++) {
					if (evCtots[i][0] == callsign) {
						evCtotFound = true;
					}
				}
				if (!evCtotFound) {
					evCtots.push_back({ callsign, "" });
					std::thread t(&CDM::setEvCtot, this, callsign);
					t.detach();
				}
			}

			bool isValidToCalculateEventMode = false;
			string tobt = getFlightStripInfo(FlightPlan, 2);

			//If realMode is activated, then it will set TOBT from the EOBT auromatically
			if (realMode) {
				isValidToCalculateEventMode = true;
			}

			//If relaMode is NOT activated, then it'll wait to press the READY TOBT Function to activate the variable "isValidToCalculateEventMode"
			if (tobt.length() == 4) {
				isValidToCalculateEventMode = true;
			}

			if (!isValidToCalculateEventMode) {
				for (size_t i = 0; i < disconnectionList.size(); i++) {
					if (disconnectionList[i] == callsign) {
						disconnectionList.erase(disconnectionList.begin() + i);
						if (aircraftFind) {
							setFlightStripInfo(FlightPlan, formatTime(slotList[pos].eobt), 2);
						}
						isValidToCalculateEventMode = true;
					}
				}
			}

			bool hasManualCtot = false;
			if (aircraftFind) {
				if (slotList[pos].hasManualCtot || getFlightStripInfo(FlightPlan, 7) == "1") {
					hasManualCtot = true;
				}
			}
			else if (getFlightStripInfo(FlightPlan, 7) == "1") {
				hasManualCtot = true;
			}

			//It'll calculate pilot's times after pressing READY TOBT Function
			if (isValidToCalculateEventMode) {
				bool hasEcfmpRestriction = false;
				EcfmpRestriction myEcfmp;
				if (!aircraftFind) {
					for (size_t z = 0; z < ecfmpData.size(); z++)
					{
						bool destFound = false;
						for (string s : ecfmpData[z].ADES) {

							if (s.find(destination) != string::npos) {
								destFound = true;
							}
							else if (s.substr(2, 2) == "**") {
								if (destination.substr(0, 2) == s.substr(0, 2)) {
									destFound = true;
								}
								else if (s.substr(0, 2) == "**") {
									destFound = true;
								}
							}
						}
						if (destFound) {
							//Chech origin
							bool depaFound = false;
							for (string s : ecfmpData[z].ADEP) {
								if (s.find(origin) != string::npos) {
									depaFound = true;
								}
								else if (s.substr(2, 2) == "**") {
									if (origin.substr(0, 2) == s.substr(0, 2)) {
										depaFound = true;
									}
									else if (s.substr(0, 2) == "**") {
										depaFound = true;
									}
								}
							}

							if (depaFound) {
								bool waypointFound = false;
								if (ecfmpData[z].waypoints.empty()) {
									waypointFound = true;
								}
								for (string s : ecfmpData[z].waypoints) {
									string item15 = FlightPlan.GetFlightPlanData().GetRoute();
									if (item15.find(s) != string::npos) {
										waypointFound = true;
									}
								}

								if (waypointFound) {
									//Check day && Month
									string dayMonth = GetDateMonthNow();
									if (stoi(dayMonth.substr(0, dayMonth.find("-"))) == stoi(ecfmpData[z].valid_date.substr(0, ecfmpData[z].valid_date.find("/"))) && stoi(dayMonth.substr(dayMonth.find("-") + 1)) == stoi(ecfmpData[z].valid_date.substr(ecfmpData[z].valid_date.find("/") + 1))) {
										//Check valid time
										int timeNow = stoi(GetActualTime());
										if (stoi(ecfmpData[z].valid_time.substr(0, ecfmpData[z].valid_time.find("-"))) <= timeNow && stoi(ecfmpData[z].valid_time.substr(ecfmpData[z].valid_time.find("-") + 1)) >= timeNow) {
											hasEcfmpRestriction = 1;
											myEcfmp = ecfmpData[z];
										}
									}
								}
							}
						}
					}
				}
				//EOBT
				EOBT = FlightPlan.GetFlightPlanData().GetEstimatedDepartureTime();
				string EOBTstring = EOBT;
				string EOBTfinal = formatTime(EOBTstring);

				if (tobt.length() == 4) {
					EOBTfinal = tobt;
				}

				EOBTfinal += "00";
				EOBT = EOBTfinal.c_str();
				bool stillOutOfTsat = false;
				int stillOutOfTsatPos;

				//Get Time NOW
				time_t rawtime;
				struct tm ptm;
				time(&rawtime);
				gmtime_s(&ptm, &rawtime);
				string hour = to_string(ptm.tm_hour % 24);
				string min = to_string(ptm.tm_min);

				if (stoi(min) < 10) {
					min = "0" + min;
				}
				if (stoi(hour) < 10) {
					hour = "0" + hour.substr(0, 1);
				}
				
				bool stsDepa = false;
				if ((string)FlightPlan.GetGroundState() == "DEPA") {
					stsDepa = true;
					setFlightStripInfo(FlightPlan, "", 3);
					setFlightStripInfo(FlightPlan, "", 4);
				}

				//Get airport
				bool aptFind = false;
				for (size_t i = 0; i < planeAiportList.size(); i++)
				{
					if (planeAiportList[i].substr(0, planeAiportList[i].find(",")) == callsign) {
						aptFind = true;
						if (planeAiportList[i].substr(planeAiportList[i].find(",") + 1, 4) != origin) {
							planeAiportList[i] = callsign + "," + origin;
						}
					}
				}

				if (!aptFind) {
					planeAiportList.push_back(callsign + "," + origin);
				}

				bool master = false;
				for (string apt : masterAirports)
				{
					if (apt == origin) {
						master = true;
					}
				}

				bool SU_ISSET = false;
				if ((string)FlightPlan.GetGroundState() == "STUP" || (string)FlightPlan.GetGroundState() == "ST-UP" || (string)FlightPlan.GetGroundState() == "PUSH" || (string)FlightPlan.GetGroundState() == "TAXI" || (string)FlightPlan.GetGroundState() == "DEPA") {
					SU_ISSET = true;
				}

				if (master) {

					if (realMode) {
						if (tobt.length() > 0 == false) {
							string mySetEobt = formatTime(FlightPlan.GetFlightPlanData().GetEstimatedDepartureTime());
							setFlightStripInfo(FlightPlan, mySetEobt, 2);
						}
					}

					//Atot check
					if (atotEnabled) {
						if ((string)FlightPlan.GetGroundState() == "DEPA") {
							bool atotFound = false;
							for (size_t i = 0; i < atotSet.size(); i++)
							{
								if (callsign == atotSet[i]) {
									atotFound = true;
								}
							}
							if (!atotFound) {
								atotSet.push_back(callsign);
								//set TTOT to now
								if (aircraftFind) {
									slotList[pos].ttot = GetTimeNow();
								}
							}
						}
					}

					bool gndStatusSet = false;
					if ((string)FlightPlan.GetGroundState() == "STUP" || (string)FlightPlan.GetGroundState() == "ST-UP" || (string)FlightPlan.GetGroundState() == "PUSH" || (string)FlightPlan.GetGroundState() == "TAXI" || (string)FlightPlan.GetGroundState() == "DEPA") {
						gndStatusSet = true;
						bool aicraftInFinalTimesList = false;
						for (string aircraft : finalTimesList) {
							if (aircraft == callsign) {
								aicraftInFinalTimesList = true;
							}
						}
						if (!aicraftInFinalTimesList) {
							finalTimesList.push_back(callsign);
						}
					}
					else {
						for (size_t i = 0; i < finalTimesList.size(); i++) {
							if (finalTimesList[i] == callsign) {
								finalTimesList.erase(finalTimesList.begin() + i);
							}
						}
					}

					string outOfTsatString = "";

					for (size_t i = 0; i < OutOfTsat.size(); i++)
					{
						bool networkSuspended = false;
						if (callsign == OutOfTsat[i][0]) {
								for (size_t s = 0; s < myNetworkStatus.size(); s++) {
									if (myNetworkStatus[s][0] == callsign) {
										if (myNetworkStatus[s][1].find("FLS") != string::npos && myNetworkStatus[s][1].find("CDM") == string::npos) {
											networkSuspended = true;
										}
									}
								}

								if (EOBTfinal == OutOfTsat[i][1] || networkSuspended) {
									stillOutOfTsat = true;
									stillOutOfTsatPos = i;
									outOfTsatString = OutOfTsat[i][2];
									if (outOfTsatString.length() > 4) outOfTsatString = outOfTsatString.substr(0, 4);
								}
								else {
									OutOfTsat.erase(OutOfTsat.begin() + i);
								}
							}
					}

					if (stillOutOfTsat && !gndStatusSet) {
						//Remove ACFT Find
						if (aircraftFind) {
							PushToOtherControllers(FlightPlan);
							if (aircraftFind) {
								slotList.erase(slotList.begin() + pos);
							}
						}
						//Show basic lists with no info, only EOBT, TOBT, ASAT and *TSAC*
						bool notYetEOBT = false;
						bool actualTOBT = false;

						string completeEOBT = (string)EOBT;
						string EOBThour = completeEOBT.substr(completeEOBT.length() - 6, 2);
						string EOBTmin = completeEOBT.substr(completeEOBT.length() - 4, 2);

						if (hour != "00") {
							if (EOBThour == "00") {
								EOBThour = "24";
							}
						}

						//ASAT
						bool ASATFound = false;
						int ASATpos = 0;
						bool correctState = false;
						string ASATtext = " ";
						for (size_t x = 0; x < asatList.size(); x++)
						{
							string actualListCallsign = asatList[x].substr(0, asatList[x].find(","));
							if (actualListCallsign == callsign) {
								ASATFound = true;
								ASATpos = x;
							}
						}

						if ((string)FlightPlan.GetGroundState() == "STUP" || (string)FlightPlan.GetGroundState() == "ST-UP" || (string)FlightPlan.GetGroundState() == "PUSH" || (string)FlightPlan.GetGroundState() == "TAXI" || (string)FlightPlan.GetGroundState() == "DEPA") {
							correctState = true;
						}

						if (!ASATFound) {
							if (correctState) {
								ASATtext = formatTime(hour + min);
								asatList.push_back(callsign + "," + ASATtext.substr(0, 4));
								std::thread t(&CDM::setCdmSts, this, callsign, "AOBT/" + ASATtext);
								t.detach();
								ASATFound = true;
							}
						}
						else {
							if (correctState) {
								ASATtext = asatList[ASATpos].substr(asatList[ASATpos].length() - 4, 4);
							}
							else if (!correctState) {
								asatList.erase(asatList.begin() + ASATpos);
								std::thread t(&CDM::setCdmSts, this, callsign, "AOBT/NULL");
								t.detach();
								ASATFound = false;
							}
						}

						if (ItemCode == TAG_ITEM_EOBT)
						{
							string myeobt = FlightPlan.GetFlightPlanData().GetEstimatedDepartureTime();
							string ShowEOBT = formatTime(myeobt);
							ItemRGB = TAG_EOBT;
							for (size_t i = 0; i < myNetworkStatus.size(); i++) {
								if (myNetworkStatus[i][0] == callsign) {
									if (myNetworkStatus[i][1].find("FLS") != string::npos) {
										ItemRGB = TAG_RED;
									}
									break;
								}
							}
							strcpy_s(sItemString, 16, ShowEOBT.c_str());
						}
						else if (ItemCode == TAG_ITEM_TOBT)
						{
							string ShowEOBT = (string)EOBT;
							ItemRGB = TAG_RED;
							strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
						}
						else if (ItemCode == TAG_ITEM_ETOBT)
						{
							string ShowEOBT = (string)EOBT;
							ItemRGB = TAG_RED;
							strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
						}
						else if (ItemCode == TAG_ITEM_TSAT)
						{
							ItemRGB = TAG_RED;
							strcpy_s(sItemString, 16, outOfTsatString.c_str());
						}
						else if (ItemCode == TAG_ITEM_TSAC)
						{
							ItemRGB = TAG_GREEN;
							if (SU_ISSET) ItemRGB = SU_SET_COLOR;
							strcpy_s(sItemString, 16, "____");
						}
						else if (ItemCode == TAG_ITEM_TSAC_SIMPLE)
						{
							string annotTSAC = getFlightStripInfo(FlightPlan, 1);
							if (!annotTSAC.empty()) {
								ItemRGB = TAG_GREEN;
								if (SU_ISSET) ItemRGB = SU_SET_COLOR;
								strcpy_s(sItemString, 16, "\xA4");
							}
							else {
								ItemRGB = TAG_GREEN;
								if (SU_ISSET) ItemRGB = SU_SET_COLOR;
								strcpy_s(sItemString, 16, "\xAC");
							}
						}
						else if (ItemCode == TAG_ITEM_ASAT)
						{
							ItemRGB = TAG_GREEN;
							if (ASATFound)
								strcpy_s(sItemString, 16, ASATtext.c_str());
							else
								strcpy_s(sItemString, 16, " ");
						}
						else if (ItemCode == TAG_ITEM_E)
						{
							ItemRGB = TAG_RED;
							strcpy_s(sItemString, 16, "I");
						}
						else if (ItemCode == TAG_ITEM_EV_CTOT) {
							bool inEvCtotsList = false;
							string slot = "";
							for (size_t i = 0; i < evCtots.size(); i++) {
								if (evCtots[i][0] == callsign) {
									inEvCtotsList = true;
									slot = evCtots[i][1];
								}
							}
							if (inEvCtotsList) {
								ItemRGB = TAG_GREY;
								strcpy_s(sItemString, 16, slot.c_str());
							}
						}
						else if (ItemCode == TAG_ITEM_NETWORK_STATUS) {
							string status = "";
							for (size_t i = 0; i < myNetworkStatus.size(); i++) {
								if (myNetworkStatus[i][0] == callsign) {
									status = myNetworkStatus[i][1];
								}
							}
							if (status != "") {
								ItemRGB = TAG_YELLOW;
								if (status == "REA") {
									ItemRGB = TAG_YELLOW;
									strcpy_s(sItemString, 16, status.c_str());
								}
								else if (status.find("FLS") != string::npos) {
									ItemRGB = TAG_RED;
									status = GetTimedStatus(status);
									strcpy_s(sItemString, 16, status.c_str());
								}
							}
						}
						else if (ItemCode == TAG_ITEM_NETWORK_STATUS_AIRBORNE) {
							string status = "";
							for (size_t i = 0; i < myNetworkStatus.size(); i++) {
								if (myNetworkStatus[i][0] == callsign) {
									status = myNetworkStatus[i][1];
								}
							}
							if (status != "") {
								ItemRGB = TAG_YELLOW;
								if (status == "COMPLY") {
									ItemRGB = TAG_GREEN;
									status = "C";
									strcpy_s(sItemString, 16, status.c_str());
								}
								else if (status == "AIRB") {
									ItemRGB = TAG_RED;
									status = "A";
									strcpy_s(sItemString, 16, status.c_str());
								}
							}
						}
						else if (ItemCode == TAG_ITEM_ON_TIME_STATUS) {
							string status = "";
							for (size_t i = 0; i < onTimeStatus.size(); i++) {
								if (onTimeStatus[i][0] == callsign) {
									status = onTimeStatus[i][1];
								}
							}
							if (status != "") {
								ItemRGB = TAG_YELLOW;
								if (status.find("+") != string::npos) {
									ItemRGB = TAG_RED;
								}
								else if (status.find("-") != string::npos) {
									ItemRGB = TAG_GREEN;
								}
								else {
									status = "";
								}
								strcpy_s(sItemString, 16, status.c_str());
							}
						}
						else if (ItemCode == TAG_ITEM_SEND_STATUS) {
							ItemRGB = TAG_RED;
							for (string flt : messagesSent) {
								if (flt == callsign) {
									ItemRGB = TAG_GREEN;
								}
							}
							strcpy_s(sItemString, 16, "SEND");
						}
						else if (ItemCode == TAG_ITEM_DEICE) {
							string status = "";
							for (vector<string> deice : deiceList) {
								if (deice[0] == callsign) {
									status = deice[1];
								}
							}
							ItemRGB = TAG_YELLOW;
							strcpy_s(sItemString, 16, status.c_str());
						}
						else if (ItemCode == TAG_ITEM_TOBT_SETBY) {
							string status = "";
							for (vector<string> reqTobtType : reqTobtTypes) {
								if (reqTobtType[0] == callsign) {
									status = reqTobtType[1];
								}
							}
							ItemRGB = TAG_GREEN;
							strcpy_s(sItemString, 16, status.c_str());
						}
					}
					else {

							if (addTime) {
								string myHour = myTimeToAdd.substr(0, 2);
								if (myHour == "00") {
									myHour = "24";
								}
								string myMin = myTimeToAdd.substr(2, 2);
								if (GetdifferenceTime(hour, min, myHour, myMin) >= 0) {
									addTime = false;
								}
							}

							string TSATfinal = "";
							string TTOTFinal = "";
							bool recalculate = false;
							if (aircraftFind) {
								string tempEOBT = EOBT;
								if (tempEOBT != slotList[pos].eobt) {
									//sendMessage(slotList[pos].callsign + " EOBT: " + tempEOBT + " != TOBT: " + slotList[pos].eobt);
									recalculate = true;
									//Update times to slaves
									countTime = std::time(nullptr) - refreshTime;
								}
								else if (readySetTsac) {
									//Update TSAC
									string myTsac = getFlightStripInfo(FlightPlan, 1);
									if (slotList[pos].showData && myTsac == "9999") {
										myTsac = slotList[pos].tsat;
										myTsac = (myTsac.length() >= 4) ? myTsac.substr(0, 4) : "";
										setFlightStripInfo(FlightPlan, myTsac, 1);
									}
								}
							}

							if (!aircraftFind || recalculate) {
									TSAT = EOBT;
									//TSAT
									string TSATstring = TSAT;
									TSATfinal = formatTime(TSATstring) + "00";

									//TTOT
									TTOTFinal = calculateTime(TSATstring, taxiTime);

									bool tempAddTime_DELAY_TSAT = false;
									bool tempAddTime_DELAY_TTOT = false;
									string myTimeToAddTemp_DELAY = "";
									for (Delay d : delayList) {
										if (d.airport == origin && d.rwy == depRwy) {
											if (d.type == "ttot") {
												tempAddTime_DELAY_TTOT = true;
												myTimeToAddTemp_DELAY = d.time + "00";
												break;
											}
											else if (d.type == "tsat") {
												tempAddTime_DELAY_TSAT = true;
												myTimeToAddTemp_DELAY = d.time + "00";
												break;
											}
										}
									}

									if (addTime || tempAddTime_DELAY_TSAT || tempAddTime_DELAY_TTOT) {
										//USE DELAY GIVEN TIMES OTHERWISE USE THE DEFAULT DELAY FUNC
										if (tempAddTime_DELAY_TSAT || tempAddTime_DELAY_TTOT) {
											string timeToUse = myTimeToAddTemp_DELAY;
											if (tempAddTime_DELAY_TTOT) {
												timeToUse = calculateLessTime(myTimeToAddTemp_DELAY, taxiTime);
											}
											string timeToAddHour = timeToUse.substr(0, 2);
											string timeToAddMin = timeToUse.substr(2, 2);
											if (hour != "00") {
												if (timeToAddHour == "00") {
													timeToAddHour = "24";
												}
											}

											string myTSATHour = TSATfinal.substr(0, 2);
											string myTSATMin = TSATfinal.substr(2, 2);
											if (hour != "00") {
												if (myTSATHour == "00") {
													myTSATHour = "24";
												}
											}

											int difTime = GetdifferenceTime(timeToAddHour, timeToAddMin, myTSATHour, myTSATMin);
											bool fixTime = true;
											if (hour != timeToAddHour) {
												if (difTime > 40) {
													fixTime = false;
												}
											}
											else {
												if (difTime > 0) {
													fixTime = false;
												}
											}

											if (!fixTime) {
												TSATfinal = timeToUse;
												TTOTFinal = calculateTime(timeToUse, taxiTime);
											}
										}
										else {
											string timeToAddHour = myTimeToAdd.substr(0, 2);
											string timeToAddMin = myTimeToAdd.substr(2, 2);
											if (hour != "00") {
												if (timeToAddHour == "00") {
													timeToAddHour = "24";
												}
											}

											string myTSATHour = TSATfinal.substr(0, 2);
											string myTSATMin = TSATfinal.substr(2, 2);
											if (hour != "00") {
												if (myTSATHour == "00") {
													myTSATHour = "24";
												}
											}

											int difTime = GetdifferenceTime(timeToAddHour, timeToAddMin, myTSATHour, myTSATMin);
											bool fixTime = true;
											if (hour != timeToAddHour) {
												if (difTime > 40) {
													fixTime = false;
												}
											}
											else {
												if (difTime > 0) {
													fixTime = false;
												}
											}

											if (!fixTime) {
												TSATfinal = myTimeToAdd;
												TTOTFinal = calculateTime(myTimeToAdd, taxiTime);
											}
										}
									}
									TSAT = TSATfinal.c_str();
									TTOT = TTOTFinal.c_str();
							}
							else {
								//TSAT
								string TSATstring = slotList[pos].tsat;
								TSATfinal = formatTime(TSATstring) + "00";
								TSAT = TSATfinal.c_str();

								//TTOT
								TTOTFinal = slotList[pos].ttot;
								TTOT = TTOTFinal.c_str();
							}

							bool equalTTOT = true;
							bool correctTTOT = true;
							bool equalTempoTTOT = true;
							bool alreadySetTOStd = false;

							if (!aircraftFind || recalculate) {
								//Calculate Rate
								int rate;

								Rate dataRate = rateForRunway(origin, depRwy);
								if (dataRate.airport == "-1") {
									if (!lvo) {
										rate = stoi(rateString);
									}
									else {
										rate = stoi(lvoRateString);
									}
								}
								else {
									int a = 0;
									int dataRatePos = 0;
									if (dataRate.rates.size() > 1) {
										for (string dr : dataRate.depRwyYes) {
											if (dr == depRwy) {
												dataRatePos = a;
											}
											a++;
										}
									}
									if (!lvo) {
										rate = stoi(dataRate.rates[dataRatePos]);
									}
									else {
										rate = stoi(dataRate.ratesLvo[dataRatePos]);
									}
								}

								double rateHour = (double)60 / rate;
								bool sameOrDependantRwys = false;
								string mySid = FlightPlan.GetFlightPlanData().GetSidName();

								while (equalTTOT) {
									correctTTOT = true;
									if (!hasManualCtot) {
										if (bmiMode) {
											TTOTFinal = getCorrectTTOT_Windowed(TTOTFinal, hasManualCtot, slotList, rate, callsign, origin, depRwy, GetActualTime() + "00", taxiTime, mySid);
										} else {
										for (size_t t = 0; t < slotList.size(); t++)
										{
											string listTTOT;
											string listCallsign = slotList[t].callsign;
											string listDepRwy = "";
											CFlightPlan listFp = FlightPlanSelect(listCallsign.c_str());
											if (!listFp.IsValid()) {
												continue;
											}
											string listSid = listFp.GetFlightPlanData().GetSidName();
											bool depRwyFound = false;
											for (size_t i = 0; i < taxiTimesList.size(); i++)
											{
												if (listCallsign == taxiTimesList[i].substr(0, taxiTimesList[i].find(","))) {
													if (taxiTimesList[i].substr(taxiTimesList[i].find(",") + 3, 1) == ",") {
														listDepRwy = taxiTimesList[i].substr(taxiTimesList[i].find(",") + 1, 2);
														depRwyFound = true;
													}
													else if (taxiTimesList[i].substr(taxiTimesList[i].find(",") + 4, 1) == ",") {
														listDepRwy = taxiTimesList[i].substr(taxiTimesList[i].find(",") + 1, 3);
														depRwyFound = true;
													}
												}
											}
											string listAirport;
											for (size_t i = 0; i < planeAiportList.size(); i++)
											{
												if (listCallsign == planeAiportList[i].substr(0, planeAiportList[i].find(","))) {
													listAirport = planeAiportList[i].substr(planeAiportList[i].find(",") + 1, 4);
												}
											}

											if (!depRwyFound) {
												listDepRwy = depRwy;
											}

											sameOrDependantRwys = false;

											if (depRwy == listDepRwy) {
												sameOrDependantRwys = true;
											}

											if (dataRate.airport != "-1" && !sameOrDependantRwys) {
												for (string testRwy : dataRate.dependentRwy) {
													if (testRwy == listDepRwy) {
														sameOrDependantRwys = true;
													}
												}
											}

											
											bool found = false;
											while (!found) {
												found = true;
												listTTOT = slotList[t].ttot;

												if (slotList[t].tsat == "999999") {
													listDepRwy = depRwy;
													listAirport = origin;
												}

												if (TTOTFinal == listTTOT && callsign != listCallsign && sameOrDependantRwys && listAirport == origin) {
													found = false;
													if (alreadySetTOStd) {
														TTOTFinal = calculateTime(TTOTFinal, 0.5);
														correctTTOT = false;
													}
													else {
														TTOTFinal = calculateTime(listTTOT, 0.5);
														correctTTOT = false;
														alreadySetTOStd = true;
													}
												}
												else if ((stoi(TTOTFinal) < stoi(calculateTime(listTTOT, rateHour))) && (stoi(TTOTFinal) > stoi(calculateLessTime(listTTOT, rateHour))) && callsign != listCallsign && sameOrDependantRwys && listAirport == origin) {
													found = false;
													if (alreadySetTOStd) {
														TTOTFinal = calculateTime(TTOTFinal, 0.5);
														correctTTOT = false;
													}
													else {
														TTOTFinal = calculateTime(listTTOT, 0.5);
														correctTTOT = false;
														alreadySetTOStd = true;
													}
												}
											}
											//Check SID Interval
											if (correctTTOT && sidIntervalEnabled && callsign != listCallsign) {
												listTTOT = slotList[t].ttot;
												double interval = getSidInterval(mySid, listSid, origin, depRwy);
												if (interval > 0) {
													bool found = false;
													while (!found) {
														found = true;
														if ((stoi(TTOTFinal) < stoi(calculateTime(listTTOT, interval))) && (stoi(TTOTFinal) > stoi(calculateLessTime(listTTOT, interval)))) {
															found = false;
															if (alreadySetTOStd) {
																TTOTFinal = calculateTime(TTOTFinal, 0.5);
																correctTTOT = false;
															}
															else {
																TTOTFinal = calculateTime(listTTOT, 0.5);
																correctTTOT = false;
																alreadySetTOStd = true;
															}
														}
													}
												}
											}
										}
									}
									}

									if (correctTTOT) {
										bool correctFlowTTOT = true;
										vector<Plane> sameDestList;
										sameDestList.clear();

										//Check flow measures if exists
										if (hasEcfmpRestriction) {
											sameDestList.clear();
											int seperationFlow = myEcfmp.value;
											for (size_t z = 0; z < slotList.size(); z++)
											{
												CFlightPlan fpSelected = FlightPlanSelect(slotList[z].callsign.c_str());
												if (!fpSelected.IsValid()) {
													continue;
												}
												string destFound = fpSelected.GetFlightPlanData().GetDestination();
												string routeFound = fpSelected.GetFlightPlanData().GetRoute();
												bool validToAdd = false;
												for (string apt : myEcfmp.ADES) {
													if (apt.find(destFound) != string::npos) {
														validToAdd = true;
													}
													else if (apt.substr(2, 2) == "**") {
														if (destFound.substr(0, 2) == apt.substr(0, 2)) {
															validToAdd = true;
														}
														else if (apt.substr(0, 2) == "**") {
															validToAdd = true;
														}
													}
												}
												if (validToAdd) {
													validToAdd = false;
													if (myEcfmp.waypoints.empty()) {
														validToAdd = true;
													}
													for (string wpt : myEcfmp.waypoints) {
														if (routeFound.find(wpt) != string::npos) {
															validToAdd = true;
														}
													}
													if (validToAdd) {
														sameDestList.push_back(slotList[z]);
													}
												}
											}

											for (size_t z = 0; z < sameDestList.size(); z++)
											{
												CFlightPlan fpList = FlightPlanSelect(slotList[z].callsign.c_str());
												if (!fpList.IsValid()) {
													continue;
												}
												bool found = false;
												string listTTOT = sameDestList[z].ttot;
												string listCallsign = sameDestList[z].callsign;
												string listDepRwy = fpList.GetFlightPlanData().GetDepartureRwy();
												string listAirport = fpList.GetFlightPlanData().GetOrigin();
												while (!found) {
													found = true;
													if (TTOTFinal == listTTOT && callsign != listCallsign && sameOrDependantRwys && listAirport == origin) {
														found = false;
														TTOTFinal = calculateTime(TTOTFinal, 1);
														correctFlowTTOT = false;
													}
													else if ((stoi(TTOTFinal) < stoi(calculateTime(listTTOT, seperationFlow))) && (stoi(TTOTFinal) > stoi(calculateLessTime(listTTOT, seperationFlow))) && callsign != listCallsign && sameOrDependantRwys && listAirport == origin) {
														found = false;
														TTOTFinal = calculateTime(TTOTFinal, 1);
														correctFlowTTOT = false;
													}
												}
											}
										}
										if (correctFlowTTOT) {
											equalTTOT = false;
											TSATfinal = calculateLessTime(TTOTFinal, taxiTime);
											/* START Check stand de-ice */
											bool standDeice = false;
											for (vector<string> deice : deiceList) {
												if (deice[0] == callsign) {
													if (deice[1] == "STND") {
														standDeice = true;
													}
												}
											}
											if (standDeice) {
												int deIceTime = getDeIceTime(FlightPlan.GetFlightPlanData().GetAircraftWtc(), 0);
												TSATfinal = calculateTime(TSATfinal, deIceTime);
											}
											/* END Check stand de-ice */
											TSAT = TSATfinal.c_str();
											TTOT = TTOTFinal.c_str();
											bool doRequest = false;
											if (aircraftFind) {
												if (TTOT != slotList[pos].ttot || EOBT != slotList[pos].eobt) {
													Plane p(callsign, EOBT, TSAT, TTOT, slotList[pos].ctot, slotList[pos].flowReason, myEcfmp, hasEcfmpRestriction, hasManualCtot, true, true);
													doRequest = true;
													slotList[pos] = p;
													setFlightStripInfo(FlightPlan, p.tsat, 3);
													setFlightStripInfo(FlightPlan, p.ttot, 4);
												}
											}
											else {
												Plane p(callsign, EOBT, TSAT, TTOT, "", "", myEcfmp, hasEcfmpRestriction, hasManualCtot, true, true);
												doRequest = true;
												slotList.push_back(p);
												pos = getPlanePosition(callsign);
												setFlightStripInfo(FlightPlan, p.tsat, 3);
												setFlightStripInfo(FlightPlan, p.ttot, 4);
											}
											//Check API
											if (doRequest) {
												if (serverEnabled) {
													string myTSATApi = TSAT;
													//Hide calculation
													for (size_t a = 0; a < slotList.size(); a++) {
														if (slotList[a].callsign == callsign) {
															slotList[a].showData = false;
														}
													}
													if (slotList[pos].hasManualCtot && slotList[pos].ctot != "" && slotList[pos].ttot.length() >= 4) {
														string myTTOT = TTOT;
														myTTOT = myTTOT.substr(0, 4);
														if (stoi(myTTOT) > stoi(slotList[pos].ctot) && stoi(myTTOT + "00") <= stoi(calculateTime(slotList[pos].ctot + "00", 7))) {
															//Update TOBT API with TSAT if TTOT is greater than CTOT but less or equal to CTOT+7
															string myCOBT = calculateLessTime(slotList[pos].ctot + "00", taxiTime);
															std::thread t(&CDM::setOBTApi, this, callsign, myCOBT, true, false);
															t.detach();
														}
														else {
															std::thread t(&CDM::setOBTApi, this, callsign, myTSATApi, true, false);
															t.detach();
														}

													}
													else {
														std::thread t(&CDM::setOBTApi, this, callsign, myTSATApi, true, false);
														t.detach();
													}
												}
											}
										}
									}
								}
							}

							if (ItemCode == TAG_ITEM_TSAT || ItemCode == TAG_ITEM_TSAT_TOBT_DIFF) {
								//Sync TTOT
								if (aircraftFind) {
									if (string(TSAT) != slotList[pos].tsat || string(TTOT) != slotList[pos].ttot) {
										setFlightStripInfo(FlightPlan, slotList[pos].tsat, 3);
										setFlightStripInfo(FlightPlan, slotList[pos].ttot, 4);
									}
								}
								else if (aircraftFind && !stsDepa) {
									string myTSAT = TSAT;
									string myTTOT = TTOT;
									setFlightStripInfo(FlightPlan, myTSAT, 3);
									setFlightStripInfo(FlightPlan, myTTOT, 4);
								}

								//Set ASRT if SU_ISSET
								if (SU_ISSET) {
									string myASRTText = getFlightStripInfo(FlightPlan, 0);
									if (myASRTText.empty()) {
										//Get Time now
										time_t rawtime;
										struct tm ptm;
										time(&rawtime);
										gmtime_s(&ptm, &rawtime);
										string hour = to_string(ptm.tm_hour % 24);
										string min = to_string(ptm.tm_min);

										if (stoi(min) < 10) {
											min = "0" + min;
										}
										if (stoi(hour) < 10) {
											hour = "0" + hour.substr(0, 1);
										}

										setFlightStripInfo(FlightPlan, (hour + min), 0);
									}
								}
							}

						string ASRTtext = getFlightStripInfo(FlightPlan, 0);

						//If oldTOBT
						bool oldTOBT = false;
						if (ASRTtext.empty()) {
							string TOBThour = EOBTfinal.substr(0, 2);
							string TOBTmin = EOBTfinal.substr(2, 2);

							if (hour != "00") {
								if (TOBThour == "00") {
									TOBTmin = "24";
								}
							}

							int difTime = GetdifferenceTime(hour, min, TOBThour, TOBTmin);

							if (hour != TOBThour) {
								if (difTime > 45) {
									oldTOBT = true;
								}
							}
							else {
								if (difTime > 5) {
									oldTOBT = true;
								}
							}

							string getTTOT = getFlightStripInfo(FlightPlan, 4);
							if (oldTOBT && !getTTOT.empty() && invalidateTOBT_Option) {
								OutOfTsat.push_back({callsign,EOBT,TSAT});
								setFlightStripInfo(FlightPlan, "", 0);
								setFlightStripInfo(FlightPlan, "", 3);
								setFlightStripInfo(FlightPlan, "", 4);
								//Update CDM-API
								std::thread t(&CDM::setCdmSts, this, callsign, "SUSP");
								t.detach();
							}
						}

						//If oldTSAT
						string TSAThour = TSATfinal.substr(TSATfinal.length() - 6, 2);
						string TSATmin = TSATfinal.substr(TSATfinal.length() - 4, 2);

						bool oldTSAT = false;
						bool moreLessFive = false;
						bool lastMinute = false;
						bool firstMinute = false;
						bool lastMinuteTOBT = false;
						bool notYetEOBT = false;
						bool actualTOBT = false;

						//Hour(LOCAL), Min(LOCAL), Hour(PLANE), Min(PLANE)

						if (hour != "00") {
							if (TSAThour == "00") {
								TSAThour = "24";
							}
						}

						int difTime = GetdifferenceTime(hour, min, TSAThour, TSATmin);

						if (hour != TSAThour) {
							if (difTime == -45) {
								firstMinute = true;
							}
							else if (difTime >= 44 && difTime <= 45) {
								lastMinute = true;
							}
							else if (difTime >= -45 && difTime <= 45) {
								moreLessFive = true;
							}
							else if (difTime > 45) {
								oldTSAT = true;
							}
						}
						else {
							if (difTime == -5) {
								firstMinute = true;
							}
							else if (difTime > 5) {
								oldTSAT = true;
							}
							else if (difTime >= 4 && difTime <= 5) {
								lastMinute = true;
							}
							else if (difTime >= -5 && difTime <= 5) {
								moreLessFive = true;
							}
						}

						bool correctState = false;
						if ((string)FlightPlan.GetGroundState() == "STUP" || (string)FlightPlan.GetGroundState() == "ST-UP" || (string)FlightPlan.GetGroundState() == "PUSH" || (string)FlightPlan.GetGroundState() == "TAXI" || (string)FlightPlan.GetGroundState() == "DEPA") {
							correctState = true;
						}

						string getTTOT = getFlightStripInfo(FlightPlan, 4);
						if (oldTSAT && !correctState && (!oldTOBT || !invalidateTOBT_Option) && invalidateTSAT_Option && !getTTOT.empty() && ((invalidateTSAT_Option_asrt && ASRTtext.empty()) || (!invalidateTSAT_Option_asrt))) {
							OutOfTsat.push_back({ callsign,EOBT,TSAT });
							setFlightStripInfo(FlightPlan, "", 0);
							setFlightStripInfo(FlightPlan, "", 3);
							setFlightStripInfo(FlightPlan, "", 4);
							
							//Update CDM-API
							std::thread t(&CDM::setCdmSts, this, callsign, "SUSP");
							t.detach();
						}

						//If suspended by network Status, mark it as Invalid (I)
						if (!stillOutOfTsat) {
							for (size_t i = 0; i < myNetworkStatus.size(); i++) {
								if (myNetworkStatus[i][0] == callsign) {
									//Check for any FLS status (except "FLS-CDM")
									if (myNetworkStatus[i][1].find("FLS") != string::npos && myNetworkStatus[i][1].find("CDM") == string::npos) {
										OutOfTsat.push_back({ callsign,EOBT,TSAT });
										setFlightStripInfo(FlightPlan, "", 0);
										setFlightStripInfo(FlightPlan, "", 3);
										setFlightStripInfo(FlightPlan, "", 4);
									}
								}
								//Do NOT Update CDM-API (As we are SUSP because of network status)
							}
						}

						//EOBT
						string completeEOBT = (string)EOBT;
						string EOBThour = completeEOBT.substr(completeEOBT.length() - 6, 2);
						string EOBTmin = completeEOBT.substr(completeEOBT.length() - 4, 2);

						if (hour != "00") {
							if (EOBThour == "00") {
								EOBThour = "24";
							}
						}

						int EOBTdifTime = GetdifferenceTime(hour, min, EOBThour, EOBTmin);
						if (hour != EOBThour) {
							if (EOBTdifTime < -75) {
								notYetEOBT = true;
							}
						}
						else {
							if (EOBTdifTime < -35) {
								notYetEOBT = true;
							}
						}

						if (hour != EOBThour) {
							if (EOBTdifTime >= -45) {
								actualTOBT = true;
							}
						}
						else {
							if (EOBTdifTime >= -5) {
								actualTOBT = true;
							}
						}

						if (hour != EOBThour) {
							if (EOBTdifTime == 45) {
								lastMinuteTOBT = true;
							}
						}
						else {
							if (EOBTdifTime == 5) {
								lastMinuteTOBT = true;
							}
						}

						time_t now = time(nullptr);

						//ASAT
						bool ASATFound = false;
						bool ASATPlusFiveLessTen = false;
						int ASATpos = 0;
						string ASATtext = " ";
						for (size_t x = 0; x < asatList.size(); x++)
						{
							string actualListCallsign = asatList[x].substr(0, asatList[x].find(","));
							if (actualListCallsign == callsign) {
								ASATFound = true;
								ASATpos = x;
							}
						}

						if (!ASATFound) {
							if (correctState) {
								ASATtext = hour + min;
								asatList.push_back(callsign + "," + ASATtext);
								std::thread t(&CDM::setCdmSts, this, callsign, "AOBT/" + ASATtext);
								t.detach();
								ASATFound = true;
							}
						}
						else {
							if (correctState) {
								ASATtext = asatList[ASATpos].substr(asatList[ASATpos].length() - 4, 4);
							}
							else if (!correctState) {
								asatList.erase(asatList.begin() + ASATpos);
								std::thread t(&CDM::setCdmSts, this, callsign, "AOBT/NULL");
								t.detach();
								ASATFound = false;
							}
						}

						//Check disply of items
						bool showData = true;
						if (aircraftFind) {
							if (!slotList[pos].showData) {
								showData = false;
							}
						}

						if (aircraftFind && showData && option_su_wait) {
							for (size_t a = 0; a < suWaitList.size(); a++)
							{
								if (suWaitList[a] == FlightPlan.GetCallsign()) {
									suWaitList.erase(suWaitList.begin() + a);
									FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(4, "SU_WAIT");
								}
							}
						}

						if (ItemCode == TAG_ITEM_EOBT)
						{
							string ShowEOBT = formatTime(FlightPlan.GetFlightPlanData().GetEstimatedDepartureTime());
							//*pColorCode = TAG_COLOR_RGB_DEFINED;
							ItemRGB = TAG_EOBT;
							if (tobt.length() > 0) {
								string mySetEobt = formatTime(FlightPlan.GetFlightPlanData().GetEstimatedDepartureTime());
								if (mySetEobt != tobt && realMode) {
									ItemRGB = TAG_ORANGE;
								}
							}
							for (size_t i = 0; i < myNetworkStatus.size(); i++) {
								if (myNetworkStatus[i][0] == callsign) {
									if (myNetworkStatus[i][1].find("FLS") != string::npos) {
										ItemRGB = TAG_RED;
									}
									break;
								}
							}
							strcpy_s(sItemString, 16, ShowEOBT.c_str());
						}
						else if (ItemCode == TAG_ITEM_TOBT)
						{
							string ShowEOBT = (string)EOBT;
							if (showData) {
								if (SU_ISSET) {
									ItemRGB = SU_SET_COLOR;
									strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
								}
								else if (notYetEOBT) {
									//*pColorCode = TAG_COLOR_RGB_DEFINED;
									ItemRGB = TAG_GREENNOTACTIVE;
									strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
								}
								else if (!actualTOBT) {
									//*pColorCode = TAG_COLOR_RGB_DEFINED;
									ItemRGB = TAG_GREENNOTACTIVE;
									strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
								}
								else if (lastMinuteTOBT && ASRTtext == "" && invalidateTOBT_Option) {
									//*pColorCode = TAG_COLOR_RGB_DEFINED;
									bool toggle = (now % 2) == 0;
									if (toggle || !flashingTOBTend) {
										ItemRGB = TAG_YELLOW;
									}
									else {
										ItemRGB = TAG_GREEN;
									}
									strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
								}
								else {
									//*pColorCode = TAG_COLOR_RGB_DEFINED;
									ItemRGB = TAG_GREEN;
									strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
								}
							}
							else
							{
								ItemRGB = TAG_RED;
								strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
							}
						}
						else if (ItemCode == TAG_ITEM_ETOBT)
						{
							string ShowEOBT = (string)EOBT;
							if (showData) {
								if (SU_ISSET) {
									ItemRGB = SU_SET_COLOR;
									strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
								}
								else if (notYetEOBT) {
									//*pColorCode = TAG_COLOR_RGB_DEFINED;
									ItemRGB = TAG_GREENNOTACTIVE;
									strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
								}
								else if (!actualTOBT) {
									//*pColorCode = TAG_COLOR_RGB_DEFINED;
									ItemRGB = TAG_GREENNOTACTIVE;
									strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
								}
								else if (lastMinuteTOBT && ASRTtext == "" && invalidateTOBT_Option) {
									//*pColorCode = TAG_COLOR_RGB_DEFINED;
									bool toggle = (now % 2) == 0;
									if (toggle || !flashingTOBTend) {
										ItemRGB = TAG_YELLOW;
									}
									else {
										ItemRGB = TAG_GREEN;
									}
									strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
								}
								else {
									//*pColorCode = TAG_COLOR_RGB_DEFINED;
									ItemRGB = TAG_GREEN;
									strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
								}
							}
							else
							{
								ItemRGB = TAG_RED;
								strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
							}
						}
						else if (ItemCode == TAG_ITEM_TSAC)
						{
							//TSAC
							bool TSACNotTSAT = false;
							string annotTSAC = getFlightStripInfo(FlightPlan, 1);
							if (annotTSAC != "9999") {
								if (!annotTSAC.empty()) {
									string TSAChour = annotTSAC.substr(annotTSAC.length() - 4, 2);
									string TSACmin = annotTSAC.substr(annotTSAC.length() - 2, 2);

									int TSACDif = GetdifferenceTime(TSAThour, TSATmin, TSAChour, TSACmin);
									if (TSAThour == TSAChour) {
										if (TSACDif > 5 || TSACDif < -5) {
											TSACNotTSAT = true;
										}
									}
									else {
										if (TSACDif > 45 || TSACDif < -45) {
											TSACNotTSAT = true;
										}
									}
								}

								if (TSACNotTSAT) {
									//*pColorCode = TAG_COLOR_RGB_DEFINED;
									ItemRGB = TAG_ORANGE;
									if (SU_ISSET) ItemRGB = SU_SET_COLOR;
									strcpy_s(sItemString, 16, annotTSAC.c_str());
								}
								else if (!annotTSAC.empty()) {
									//*pColorCode = TAG_COLOR_RGB_DEFINED;
									ItemRGB = TAG_GREEN;
									if (SU_ISSET) ItemRGB = SU_SET_COLOR;
									strcpy_s(sItemString, 16, annotTSAC.c_str());
								}
								else {
									//*pColorCode = TAG_COLOR_RGB_DEFINED;
									ItemRGB = TAG_GREEN;
									if (SU_ISSET) ItemRGB = SU_SET_COLOR;
									strcpy_s(sItemString, 16, "____");
								}
							}
						}
						else if (ItemCode == TAG_ITEM_TSAC_SIMPLE)
						{
							//TSAC
							bool TSACNotTSAT = false;
							string annotTSAC = getFlightStripInfo(FlightPlan, 1);

							if (!annotTSAC.empty()) {
								string TSAChour = annotTSAC.substr(annotTSAC.length() - 4, 2);
								string TSACmin = annotTSAC.substr(annotTSAC.length() - 2, 2);

								int TSACDif = GetdifferenceTime(TSAThour, TSATmin, TSAChour, TSACmin);
								if (TSAThour == TSAChour) {
									if (TSACDif > 5 || TSACDif < -5) {
										TSACNotTSAT = true;
									}
								}
								else {
									if (TSACDif > 45 || TSACDif < -45) {
										TSACNotTSAT = true;
									}
								}
							}

							if (!annotTSAC.empty()) {
								if (SU_ISSET) {
									ItemRGB = SU_SET_COLOR;
								} else if (TSACNotTSAT) {
									ItemRGB = TAG_ORANGE;
								}
								else {
									//*pColorCode = TAG_COLOR_RGB_DEFINED;
									ItemRGB = TAG_GREEN;
									if (SU_ISSET) ItemRGB = SU_SET_COLOR;
								}
								strcpy_s(sItemString, 16, "\xA4");
							}
							else {
								ItemRGB = TAG_GREEN;
								if (SU_ISSET) ItemRGB = SU_SET_COLOR;
								strcpy_s(sItemString, 16, "\xAC");
							}
						}
						else if (ItemCode == TAG_ITEM_TSAT)
						{
							if (showData) {
								if (aircraftFind) {
									string ShowTSAT = (string)TSAT;
									ShowTSAT = ShowTSAT.substr(0, ShowTSAT.length() - 2);

									if (SU_ISSET) {
										ItemRGB = SU_SET_COLOR;
										strcpy_s(sItemString, 16, ShowTSAT.c_str());
									}
									else if (notYetEOBT) {
										//*pColorCode = TAG_COLOR_RGB_DEFINED;
										ItemRGB = TAG_GREY;
										strcpy_s(sItemString, 16, "~");
									}
									else if (firstMinute && flashingTSATstart) {
										bool toggle = (now % 2) == 0;
										if (toggle) {
											ItemRGB = TAG_ORANGE;
										}
										else {
											ItemRGB = TAG_GREEN;
										}
										strcpy_s(sItemString, 16, ShowTSAT.c_str());
									}
									else if (lastMinute) {
										//*pColorCode = TAG_COLOR_RGB_DEFINED;
										bool toggle = (now % 2) == 0;
										if (toggle || !flashingTSATend) {
											ItemRGB = TAG_YELLOW;
										}
										else {
											ItemRGB = TAG_GREEN;
										}
										strcpy_s(sItemString, 16, ShowTSAT.c_str());
									}
									else if (moreLessFive) {
										//*pColorCode = TAG_COLOR_RGB_DEFINED;
										ItemRGB = TAG_GREEN;
										strcpy_s(sItemString, 16, ShowTSAT.c_str());
									}
									else if (oldTSAT) {
										//*pColorCode = TAG_COLOR_RGB_DEFINED;
										ItemRGB = TAG_GREEN;
										if (!invalidateTSAT_Option) {
											ItemRGB = TAG_YELLOW;
										}
										strcpy_s(sItemString, 16, ShowTSAT.c_str());
									}
									else {
										//*pColorCode = TAG_COLOR_RGB_DEFINED;
										ItemRGB = TAG_GREENNOTACTIVE;
										strcpy_s(sItemString, 16, ShowTSAT.c_str());
									}
								}
							}
							else
							{
								ItemRGB = TAG_GREY;
								strcpy_s(sItemString, 16, "....");
							}
						}
						else if (ItemCode == TAG_ITEM_TSAT_TOBT_DIFF)
						{
							if (showData) {
								if (aircraftFind) {
									string value = slotList[pos].tsat.substr(0, 4) + getDiffTOBTTSAT(slotList[pos].tsat, slotList[pos].eobt);

									if (SU_ISSET) {
										ItemRGB = SU_SET_COLOR;
										strcpy_s(sItemString, 16, value.c_str());
									}
									else if (notYetEOBT) {
										//*pColorCode = TAG_COLOR_RGB_DEFINED;
										ItemRGB = TAG_GREY;
										strcpy_s(sItemString, 16, "~");
									}
									else if (lastMinute) {
										ItemRGB = TAG_GREEN;
										strcpy_s(sItemString, 16, value.c_str());
									}
									else if (moreLessFive) {
										//*pColorCode = TAG_COLOR_RGB_DEFINED;
										ItemRGB = TAG_GREEN;
										strcpy_s(sItemString, 16, value.c_str());
									}
									else if (oldTSAT) {
										//*pColorCode = TAG_COLOR_RGB_DEFINED;
										ItemRGB = TAG_GREEN;
										if (!invalidateTSAT_Option) {
											ItemRGB = TAG_YELLOW;
										}
										strcpy_s(sItemString, 16, value.c_str());
									}
									else {
										//*pColorCode = TAG_COLOR_RGB_DEFINED;
										ItemRGB = TAG_GREENNOTACTIVE;
										strcpy_s(sItemString, 16, value.c_str());
									}
								}
							}
							else
							{
								ItemRGB = TAG_GREY;
								strcpy_s(sItemString, 16, "....");
							}
						}
						else if (ItemCode == NOW_TSAT_DIFF)
						{
							if (showData) {
								if (aircraftFind) {
									string value = getDiffNowTime(slotList[pos].tsat);

									if (SU_ISSET) {
										ItemRGB = SU_SET_COLOR;
										strcpy_s(sItemString, 16, value.c_str());
									}
									else if (notYetEOBT) {
										//*pColorCode = TAG_COLOR_RGB_DEFINED;
										ItemRGB = TAG_GREY;
										strcpy_s(sItemString, 16, "~");
									}
									else if (lastMinute) {
										ItemRGB = TAG_GREEN;
										strcpy_s(sItemString, 16, value.c_str());
									}
									else if (moreLessFive) {
										//*pColorCode = TAG_COLOR_RGB_DEFINED;
										ItemRGB = TAG_GREEN;
										strcpy_s(sItemString, 16, value.c_str());
									}
									else if (oldTSAT) {
										//*pColorCode = TAG_COLOR_RGB_DEFINED;
										ItemRGB = TAG_GREEN;
										if (!invalidateTSAT_Option) {
											ItemRGB = TAG_YELLOW;
										}
										strcpy_s(sItemString, 16, value.c_str());
									}
									else {
										//*pColorCode = TAG_COLOR_RGB_DEFINED;
										ItemRGB = TAG_GREENNOTACTIVE;
										strcpy_s(sItemString, 16, value.c_str());
									}
								}
							}
						}
						else if (ItemCode == TAG_ITEM_TTOT)
						{
							if (showData) {
								string ShowTTOT = (string)TTOT;
								if (notYetEOBT) {
									//*pColorCode = TAG_COLOR_RGB_DEFINED;
									ItemRGB = TAG_GREY;
									strcpy_s(sItemString, 16, "~");
								}
								else if ((moreLessFive || lastMinute) && ShowTTOT.length() >= 4) {
									//*pColorCode = TAG_COLOR_RGB_DEFINED;
									ItemRGB = TAG_TTOT;
									strcpy_s(sItemString, 16, ShowTTOT.substr(0, ShowTTOT.length() - 2).c_str());
								}
								else if (ShowTTOT.length() >= 4) {
									//*pColorCode = TAG_COLOR_RGB_DEFINED;
									ItemRGB = TAG_TTOT;
									strcpy_s(sItemString, 16, ShowTTOT.substr(0, ShowTTOT.length() - 2).c_str());
								}
							}
							else
							{
								ItemRGB = TAG_GREY;
								strcpy_s(sItemString, 16, "....");
							}
						}
						else if (ItemCode == NOW_TTOT_DIFF)
						{
							if (showData) {
								string value = getDiffNowTime(slotList[pos].ttot);
								if (notYetEOBT) {
									//*pColorCode = TAG_COLOR_RGB_DEFINED;
									ItemRGB = TAG_GREY;
									strcpy_s(sItemString, 16, "~");
								}
								else if (moreLessFive || lastMinute) {
									//*pColorCode = TAG_COLOR_RGB_DEFINED;
									ItemRGB = TAG_TTOT;
									strcpy_s(sItemString, 16, value.c_str());
								}
								else {
									//*pColorCode = TAG_COLOR_RGB_DEFINED;
									ItemRGB = TAG_TTOT;
									strcpy_s(sItemString, 16, value.c_str());
								}
							}
						}
						else if (ItemCode == TAG_ITEM_ASAT)
						{
							if (ASATFound) {
								string ASATHour = ASATtext.substr(0, 2);
								string ASATMin = ASATtext.substr(2, 2);
								if (hour != "00") {
									if (ASATHour == "00") {
										ASATHour = "24";
									}
								}

								int ASATDifTIme = GetdifferenceTime(hour, min, ASATHour, ASATMin);
								if ((string)FlightPlan.GetGroundState() == "STUP" || (string)FlightPlan.GetGroundState() == "ST-UP" || (string)FlightPlan.GetGroundState() == "") {
									if (hour == ASATHour) {
										if (ASATDifTIme >= 5) {
											ASATPlusFiveLessTen = true;
										}
									}
									else {
										if (ASATDifTIme >= 45) {
											ASATPlusFiveLessTen = true;
										}
									}
								}
							}
							if (ASATFound) {
								if ((string)FlightPlan.GetGroundState() == "" || (string)FlightPlan.GetGroundState() == "STUP" || (string)FlightPlan.GetGroundState() == "ST-UP") {
									if (ASATPlusFiveLessTen) {
										ItemRGB = TAG_YELLOW;
										strcpy_s(sItemString, 16, ASATtext.c_str());
									}
									else {
										ItemRGB = TAG_GREEN;
										strcpy_s(sItemString, 16, ASATtext.c_str());
									}
								}
								else {
									ItemRGB = SU_SET_COLOR;
									strcpy_s(sItemString, 16, ASATtext.c_str());
								}
							}
							else {
								//*pColorCode = TAG_COLOR_RGB_DEFINED;
								ItemRGB = TAG_GREEN;
								strcpy_s(sItemString, 16, " ");
							}
						}
						else if (ItemCode == TAG_ITEM_ASRT)
						{
							string ASRTtext = getFlightStripInfo(FlightPlan, 0);
							if (!ASRTtext.empty()) {
								//*pColorCode = TAG_COLOR_RGB_DEFINED;
								if (SU_ISSET) {
									ItemRGB = SU_SET_COLOR;
								}
								else {
									ItemRGB = TAG_ASRT;
								}
								strcpy_s(sItemString, 16, ASRTtext.c_str());
							}
							else {
								//*pColorCode = TAG_COLOR_RGB_DEFINED;
								ItemRGB = TAG_ASRT;
								strcpy_s(sItemString, 16, " ");
							}
						}
						else if (ItemCode == TAG_ITEM_READYSTARTUP)
						{
							string ASRTtext = getFlightStripInfo(FlightPlan, 0);
							if (!ASRTtext.empty()) {
								ItemRGB = TAG_GREEN;
								strcpy_s(sItemString, 16, "RSTUP");
							}
							else {
								ItemRGB = TAG_RED;
								strcpy_s(sItemString, 16, "RSTUP");
							}
						}
						else if (ItemCode == TAG_ITEM_E)
						{
							if (notYetEOBT) {
								//*pColorCode = TAG_COLOR_RGB_DEFINED;
								ItemRGB = TAG_GREEN;
								strcpy_s(sItemString, 16, "P");
							}
							else {
								//*pColorCode = TAG_COLOR_RGB_DEFINED;
								ItemRGB = TAG_GREEN;
								strcpy_s(sItemString, 16, "C");
							}
						}
						else if (ItemCode == TAG_ITEM_FLOW_MESSAGE) {
							if (showData) {
								if (aircraftFind) {
									if (slotList[pos].hasManualCtot) {
										string message = "MAN ACT";
										if (slotList[pos].ctot != "") {
											message = slotList[pos].flowReason;
										}
										ItemRGB = TAG_YELLOW;
										strcpy_s(sItemString, 16, message.c_str());
									}
									else if (slotList[pos].hasEcfmpRestriction) {
										ItemRGB = TAG_YELLOW;
										string message = slotList[pos].ecfmpRestriction.ident;
										strcpy_s(sItemString, 16, message.c_str());
									}
								}
							}
							else
							{
								ItemRGB = TAG_GREY;
								strcpy_s(sItemString, 16, "....");
							}
						}
						else if (ItemCode == TAG_ITEM_CTOT)
						{
							if (showData) {
								if (aircraftFind) {
									if (slotList[pos].hasManualCtot) {
										string value = "";
										if (slotList[pos].ctot == "") {
											value = slotList[pos].ttot.substr(0, 4);
											ItemRGB = TAG_ORANGE;
										}
										else {
											value = slotList[pos].ctot;
											ItemRGB = TAG_CTOT;
											for (size_t i = 0; i < myNetworkStatus.size(); i++) {
												if (myNetworkStatus[i][0] == callsign && myNetworkStatus[i][1] == "REA" && !ASATFound) {
													ItemRGB = TAG_YELLOW;
												}
											}
										}
										strcpy_s(sItemString, 16, value.c_str());
									}
									else if (slotList[pos].hasEcfmpRestriction) {
										ItemRGB = TAG_RED;
										strcpy_s(sItemString, 16, slotList[pos].ttot.substr(0, 4).c_str());
									}
								}
							}
							else
							{
								ItemRGB = TAG_GREY;
								strcpy_s(sItemString, 16, "....");
							}
						}
						else if (ItemCode == NOW_CTOT_DIFF)
						{
							if (showData) {
								bool inreaList = false;
								for (string s : reaCTOTSent) {
									if (s == callsign) {
										inreaList = true;
									}
								}

								if (aircraftFind) {
									if (slotList[pos].hasManualCtot) {
										string value = getDiffNowTime(slotList[pos].ttot);
										if (slotList[pos].ctot == "") {
											ItemRGB = TAG_ORANGE;
										}
										else {
											ItemRGB = TAG_CTOT;
											for (size_t i = 0; i < myNetworkStatus.size(); i++) {
												if (myNetworkStatus[i][0] == callsign && myNetworkStatus[i][1] == "REA" && !ASATFound) {
													ItemRGB = TAG_YELLOW;
												}
											}
										}
										strcpy_s(sItemString, 16, value.c_str());
									}
								}
							}
						}
						else if (ItemCode == TAG_ITEM_EV_CTOT) {
							bool inEvCtotsList = false;
							string slot = "";
							for (size_t i = 0; i < evCtots.size(); i++) {
								if (evCtots[i][0] == callsign) {
									inEvCtotsList = true;
									slot = evCtots[i][1];
								}
							}
							if (inEvCtotsList) {
								ItemRGB = TAG_GREY;
								strcpy_s(sItemString, 16, slot.c_str());
							}
						}
						else if (ItemCode == TAG_ITEM_NETWORK_STATUS) {
						string status = "";
						for (size_t i = 0; i < myNetworkStatus.size(); i++) {
							if (myNetworkStatus[i][0] == callsign) {
								status = myNetworkStatus[i][1];
							}
						}
						if (status != "") {
							ItemRGB = TAG_YELLOW;
							if (status == "REA") {
								ItemRGB = TAG_YELLOW;
								strcpy_s(sItemString, 16, status.c_str());
							}
							else if (status.find("FLS") != string::npos) {
								ItemRGB = TAG_RED;
								status = GetTimedStatus(status);
								strcpy_s(sItemString, 16, status.c_str());
							}
						}
						}
						else if (ItemCode == TAG_ITEM_NETWORK_STATUS_AIRBORNE) {
							string status = "";
							for (size_t i = 0; i < myNetworkStatus.size(); i++) {
								if (myNetworkStatus[i][0] == callsign) {
									status = myNetworkStatus[i][1];
								}
							}
							if (status != "") {
								ItemRGB = TAG_YELLOW;
								if (status == "COMPLY") {
									ItemRGB = TAG_GREEN;
									status = "C";
									strcpy_s(sItemString, 16, status.c_str());
								}
								else if (status == "AIRB") {
									ItemRGB = TAG_RED;
									status = "A";
									strcpy_s(sItemString, 16, status.c_str());
								}
							}
							}
						else if (ItemCode == TAG_ITEM_ON_TIME_STATUS) {
							string status = "";
							for (size_t i = 0; i < onTimeStatus.size(); i++) {
								if (onTimeStatus[i][0] == callsign) {
									status = onTimeStatus[i][1];
								}
							}
							if (status != "") {
								ItemRGB = TAG_YELLOW;
								if (status.find("+") != string::npos) {
									ItemRGB = TAG_RED;
								}
								else if (status.find("-") != string::npos) {
									ItemRGB = TAG_GREEN;
								}
								else {
									status = "";
								}
								strcpy_s(sItemString, 16, status.c_str());
							}
						}
						else if (ItemCode == TAG_ITEM_SEND_STATUS) {
							ItemRGB = TAG_RED;
							for (string flt : messagesSent) {
								if (flt == callsign) {
									ItemRGB = TAG_GREEN;
								}
							}
							strcpy_s(sItemString, 16, "SEND");
						}
						else if (ItemCode == TAG_ITEM_DEICE) {
							string status = "";
							for (vector<string> deice : deiceList) {
								if (deice[0] == callsign) {
									status = deice[1];
								}
							}
							ItemRGB = TAG_YELLOW;
							strcpy_s(sItemString, 16, status.c_str());
						}
						else if (ItemCode == TAG_ITEM_TOBT_SETBY) {
							string status = "";
							for (vector<string> reqTobtType : reqTobtTypes) {
								if (reqTobtType[0] == callsign) {
									status = reqTobtType[1];
								}
							}
							ItemRGB = TAG_GREEN;
							strcpy_s(sItemString, 16, status.c_str());
							}

						//Update ECFMP to Slaves
						if (aircraftFind) {
							string stripEcfmp = getFlightStripInfo(FlightPlan, 6);
							if (stripEcfmp == "" && slotList[pos].hasEcfmpRestriction) {
								setFlightStripInfo(FlightPlan, slotList[pos].ecfmpRestriction.ident, 6);
							}
							else if (stripEcfmp != "" && !slotList[pos].hasEcfmpRestriction) {
								//Make ecfmp empty for slaves
								setFlightStripInfo(FlightPlan, "", 6);
							}
						}

						//Update Manual CTOT to Slaves
						if (aircraftFind) {
							string stripManualCtot = getFlightStripInfo(FlightPlan, 7);
							if (stripManualCtot == "" && slotList[pos].hasManualCtot && slotList[pos].ctot == "") {
								setFlightStripInfo(FlightPlan, "1", 7);
							}
							else if (stripManualCtot != "" && !slotList[pos].hasManualCtot) {
								//Make ecfmp empty for slaves
								setFlightStripInfo(FlightPlan, "", 7);
							}
						}

						//Refresh CDM API every 30 seconds
						time_t timeNow = std::time(nullptr);

						//Remove disconnected planes after 5 min disconnected
						if (countTfcDisconnection != -1) {
							if ((timeNow - countTfcDisconnectionTime) > 300) {
								countTfcDisconnectionTime = timeNow;
								countTfcDisconnection = -1;
								disconnectTfcs();
								pos = getPlanePosition(callsign);
								if (pos == -1) {
									aircraftFind = false;
								}
							}
						}

						//Check readyToUpdateList;
						if (readyToUpdateList && !refresh1) {
							addLogLine("[AUTO] - Updating slotList with latest update...");
							for (Plane p : slotListToUpdate) {
								for (int d = 0; d < slotList.size(); d++) {
									if (p.callsign == slotList[d].callsign && p.eobt == slotList[d].eobt && p.ctot == slotList[d].ctot) {
										p.showData = slotList[d].showData;
										slotList[d] = p;
									}
								}
							}
							addLogLine("[AUTO] - SlotList list updated succesfully");
							slotListToUpdate.clear();
							readyToUpdateList = false;
						}

						//Refresh times every x sec
						if ((timeNow - countTime) > refreshTime && !refresh1) {
							refresh1 = true;
							countTime = timeNow;
							addLogLine("[AUTO] - REFRESH CDM INTERNAL DATA");
							//Order list according TTOT
							slotList = recalculateSlotList(slotList);
							pos = getPlanePosition(callsign);

							for (size_t t = 0; t < slotList.size(); t++) {
								CFlightPlan fpSelected = FlightPlanSelect(slotList[t].callsign.c_str());
								PushToOtherControllers(fpSelected);
							}
							if (debugMode) {
								sendMessage("[DEBUG MESSAGE] - REFRESHING");
								sendMessage("[DEBUG MESSAGE] - " + to_string(slotList.size()) + " Planes in the list");
							}

							std::thread t(&CDM::refreshActions1, this);
							t.detach();
						}
					}
				}
				else {
					//Remove disconnected planes after 5 min disconnected
					if (countTfcDisconnection != -1) {
						if ((timeNow - countTfcDisconnectionTime) > 300) {
							countTfcDisconnectionTime = timeNow;
							countTfcDisconnection = -1;
							disconnectTfcs();
							pos = getPlanePosition(callsign);
							if (pos == -1) {
								aircraftFind = false;
							}
						}
					}

					bool TSATFind = true;
					string TSATString = getFlightStripInfo(FlightPlan, 3);
					string TTOTString = getFlightStripInfo(FlightPlan, 4);
					if (TSATString == "") {
						TSATFind = false;
					}

					if (aircraftFind) {
						if (!TSATFind) {
							if (pos < slotList.size()) { // Check if pos is within bounds
								slotList.erase(slotList.begin() + pos);
							}
						}
						else if (TSATString != slotList[pos].tsat || TTOTString != slotList[pos].ttot) {
							if (pos < slotList.size()) { // Check if pos is within bounds
								slotList[pos].tsat = TSATString;
								slotList[pos].ttot = TTOTString;
							}
						}
					}
					else {
						if (TSATFind) {
							Plane p(callsign, EOBT, TSATString, TTOTString, "", "", myEcfmp, hasEcfmpRestriction, hasManualCtot, true, true);
							slotList.push_back(p);
						}
					}

					//Update de-ice status
					if (aircraftFind) {
						string deIce = getFlightStripInfo(FlightPlan, 5);
						bool found = false;
						for (int z = 0; z < deiceList.size(); z++)
						{
							if (deiceList[z][0] == callsign) {
								found = true;
								if (deIce == "") {
									//Remove from list
									deiceList.erase(deiceList.begin() + z);
								}
								else if (deIce != deiceList[z][1]) {
									//Modify list value
									deiceList[z] = { callsign, deIce };
								}
							}
						}

						if (!found && deIce != "") {
							//Add to main de-ice list
							deiceList.push_back({ callsign, deIce });
						}
					}

					//Update ECFMP from Master
					if (aircraftFind) {
						string ecfmpIdent = getFlightStripInfo(FlightPlan, 6);
						if (ecfmpIdent != "") {
							for (int y = 0; y < ecfmpData.size(); y++)
							{
								if (ecfmpData[y].ident == ecfmpIdent) {
									slotList[pos].hasEcfmpRestriction = 1;
									slotList[pos].ecfmpRestriction = ecfmpData[y];
								}
							}
						}
						else if (ecfmpIdent == "" && slotList[pos].hasEcfmpRestriction)
						{
							slotList[pos].hasEcfmpRestriction = 0;
						}
					}

					//Update Manual CTOT from Master
					if (aircraftFind) {
						string manualCtot = getFlightStripInfo(FlightPlan, 7);
						if (manualCtot != "" && !slotList[pos].hasManualCtot) {
							slotList[pos].hasManualCtot = 1;
						}
						else if (manualCtot == "" && slotList[pos].hasManualCtot)
						{
							slotList[pos].hasManualCtot = 0;
						}
					}

					// If oldTSAT
					if (TSATFind) {
						string TSAThour = TSATString.substr(TSATString.length() - 6, 2);
						string TSATmin = TSATString.substr(TSATString.length() - 4, 2);

						bool oldTSAT = false;
						bool moreLessFive = false;
						bool lastMinute = false;
						bool firstMinute = false;
						bool lastMinuteTOBT = false;
						bool notYetEOBT = false;
						bool actualTOBT = false;

						if (hour != "00" && TSAThour == "00") {
							TSAThour = "24";
						}

						int difTime = GetdifferenceTime(hour, min, TSAThour, TSATmin);

						if (hour != TSAThour) {
							if (difTime == -45) {
								firstMinute = true;
							}
							else if (difTime >= 44 && difTime <= 45) {
								lastMinute = true;
							}
							else if (difTime >= -45 && difTime <= 45) {
								moreLessFive = true;
							}
							else if (difTime > 45) {
								oldTSAT = true;
							}
						}
						else {
							if (difTime == -5) {
								firstMinute = true;
							}
							else if (difTime > 5) {
								oldTSAT = true;
							}
							else if (difTime >= 4 && difTime <= 5) {
								lastMinute = true;
							}
							else if (difTime >= -5 && difTime <= 5) {
								moreLessFive = true;
							}
						}

						bool correctState = false;
						string groundState = (string)FlightPlan.GetGroundState();
						if (groundState == "STUP" || groundState == "ST-UP" || groundState == "PUSH" || groundState == "TAXI" || groundState == "DEPA") {
							correctState = true;
						}

						if (oldTSAT && !correctState) {
							bool alreadyInList = false;
							for (size_t i = 0; i < OutOfTsat.size(); i++)
							{
								if (callsign == OutOfTsat[i][0]) {
									alreadyInList = true;
								}
							}

							if (!alreadyInList) {
								OutOfTsat.push_back({callsign,EOBT,TSAT});
							}
						}

						string completeEOBT = (string)EOBT;
						string EOBThour = completeEOBT.substr(completeEOBT.length() - 6, 2);
						string EOBTmin = completeEOBT.substr(completeEOBT.length() - 4, 2);

						if (hour != "00" && EOBThour == "00") {
							EOBThour = "24";
						}

						int EOBTdifTime = GetdifferenceTime(hour, min, EOBThour, EOBTmin);
						if (hour != EOBThour) {
							if (EOBTdifTime < -75) {
								notYetEOBT = true;
							}
						}
						else {
							if (EOBTdifTime < -35) {
								notYetEOBT = true;
							}
						}

						if (hour != EOBThour) {
							if (EOBTdifTime >= -45) {
								actualTOBT = true;
							}
						}
						else {
							if (EOBTdifTime >= -5) {
								actualTOBT = true;
							}
						}

						if (hour != EOBThour) {
							if (EOBTdifTime == 45) {
								lastMinuteTOBT = true;
							}
						}
						else {
							if (EOBTdifTime == 5) {
								lastMinuteTOBT = true;
							}
						}

						//ASRT
						string ASRTtext = getFlightStripInfo(FlightPlan, 0);

						// TSAC
						bool TSACNotTSAT = false;
						string annotTSAC = getFlightStripInfo(FlightPlan, 1);

						if (!annotTSAC.empty()) {
							string TSAChour = annotTSAC.substr(annotTSAC.length() - 4, 2);
							string TSACmin = annotTSAC.substr(annotTSAC.length() - 2, 2);

							int TSACDif = GetdifferenceTime(TSAThour, TSATmin, TSAChour, TSACmin);
							if (TSAThour == TSAChour) {
								if (TSACDif > 5 || TSACDif < -5) {
									TSACNotTSAT = true;
								}
							}
							else {
								if (TSACDif > 45 || TSACDif < -45) {
									TSACNotTSAT = true;
								}
							}
						}

						time_t now = time(nullptr);

						// ASAT
						bool ASATFound = false;
						bool ASATPlusFiveLessTen = false;
						int ASATpos = 0;
						string ASATtext = " ";
						for (size_t x = 0; x < asatList.size(); x++) {
							string actualListCallsign = asatList[x].substr(0, asatList[x].find(","));
							if (actualListCallsign == callsign) {
								ASATFound = true;
								ASATpos = x;
								break; // Break the loop once found
							}
						}

						if (!ASATFound) {
							if (correctState) {
								ASATtext = hour + min;
								asatList.push_back(callsign + "," + ASATtext);
								ASATFound = true;
							}
						}
						else {
							if (correctState) {
								ASATtext = asatList[ASATpos].substr(asatList[ASATpos].length() - 4, 4);
							}
							else if (!correctState) {
								if (ASATpos < asatList.size()) { // Check if ASATpos is within bounds
									asatList.erase(asatList.begin() + ASATpos);
									ASATFound = false;
								}
							}
						}

						if (ASATFound) {
							string ASATHour = ASATtext.substr(0, 2);
							string ASATMin = ASATtext.substr(2, 2);
							if (hour != "00" && ASATHour == "00") {
								ASATHour = "24";
							}

							int ASATDifTIme = GetdifferenceTime(hour, min, ASATHour, ASATMin);
							string groundState = (string)FlightPlan.GetGroundState();
							if (groundState == "STUP" || groundState == "ST-UP" || groundState == "") {
								if (hour == ASATHour) {
									if (ASATDifTIme >= 5) {
										ASATPlusFiveLessTen = true;
									}
								}
								else {
									if (ASATDifTIme >= 45) {
										ASATPlusFiveLessTen = true;
									}
								}
							}
						}

						if (ItemCode == TAG_ITEM_EOBT) {
							string ShowEOBT = formatTime(FlightPlan.GetFlightPlanData().GetEstimatedDepartureTime());
							ItemRGB = TAG_EOBT;
							for (size_t i = 0; i < myNetworkStatus.size(); i++) {
								if (myNetworkStatus[i][0] == callsign) {
									if (myNetworkStatus[i][1].find("FLS") != string::npos) {
										ItemRGB = TAG_RED;
									}
									break;
								}
							}
							strcpy_s(sItemString, 16, ShowEOBT.c_str());
						}
						else if (ItemCode == TAG_ITEM_TOBT)
						{
							string ShowEOBT = (string)EOBT;
							if (SU_ISSET) {
								ItemRGB = SU_SET_COLOR;
								strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
							}
							else if (notYetEOBT) {
								ItemRGB = TAG_GREY;
								strcpy_s(sItemString, 16, "~");
							}
							else if (!actualTOBT) {
								ItemRGB = TAG_GREENNOTACTIVE;
								strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
							}
							else if (lastMinuteTOBT && ASRTtext == "" && invalidateTOBT_Option) {
								//*pColorCode = TAG_COLOR_RGB_DEFINED;
								bool toggle = (now % 2) == 0;
								if (toggle || !flashingTOBTend) {
									ItemRGB = TAG_YELLOW;
								}
								else {
									ItemRGB = TAG_GREEN;
								}
								strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
							}
							else {
								ItemRGB = TAG_GREEN;
								strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
							}
						}
						else if (ItemCode == TAG_ITEM_ETOBT)
						{
							string ShowEOBT = (string)EOBT;
							if (SU_ISSET) {
								ItemRGB = SU_SET_COLOR;
								strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
							}
							else if (notYetEOBT) {
								ItemRGB = TAG_GREY;
								strcpy_s(sItemString, 16, "~");
							}
							else if (!actualTOBT) {
								ItemRGB = TAG_GREENNOTACTIVE;
								strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
							}
							else if (lastMinuteTOBT && ASRTtext == "" && invalidateTOBT_Option) {
								//*pColorCode = TAG_COLOR_RGB_DEFINED;
								bool toggle = (now % 2) == 0;
								if (toggle || !flashingTOBTend) {
									ItemRGB = TAG_YELLOW;
								}
								else {
									ItemRGB = TAG_GREEN;
								}
								strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
							}
							else {
								ItemRGB = TAG_GREEN;
								strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
							}
						}
						else if (ItemCode == TAG_ITEM_TSAC)
						{
							if (TSACNotTSAT) {
								ItemRGB = TAG_ORANGE;
								if (SU_ISSET) ItemRGB = SU_SET_COLOR;
								strcpy_s(sItemString, 16, annotTSAC.c_str());
							}
							else if (!annotTSAC.empty()) {
								ItemRGB = TAG_GREEN;
								if (SU_ISSET) ItemRGB = SU_SET_COLOR;
								strcpy_s(sItemString, 16, annotTSAC.c_str());
							}
							else {
								ItemRGB = TAG_GREEN;
								if (SU_ISSET) ItemRGB = SU_SET_COLOR;
								strcpy_s(sItemString, 16, "____");
							}
						}
						else if (ItemCode == TAG_ITEM_TSAC_SIMPLE)
						{
							string annotTSAC = getFlightStripInfo(FlightPlan, 1);
							if (!annotTSAC.empty()) {
								if (TSACNotTSAT) {
									ItemRGB = TAG_ORANGE;
									if (SU_ISSET) ItemRGB = SU_SET_COLOR;
								}
								else {
									//*pColorCode = TAG_COLOR_RGB_DEFINED;
									ItemRGB = TAG_GREEN;
									if (SU_ISSET) ItemRGB = SU_SET_COLOR;
								}
								strcpy_s(sItemString, 16, "\xA4");
							}
							else {
								ItemRGB = TAG_GREEN;
								if (SU_ISSET) ItemRGB = SU_SET_COLOR;
								strcpy_s(sItemString, 16, "\xAC");
							}
						}
						else if (ItemCode == TAG_ITEM_TSAT)
						{
							if (TSATString.length() > 0 && aircraftFind) {
								TSATString = TSATString.substr(0, 4);
								if (SU_ISSET) {
									ItemRGB = SU_SET_COLOR;
									strcpy_s(sItemString, 16, TSATString.c_str());
								}
								else if (notYetEOBT) {
									ItemRGB = TAG_GREY;
									strcpy_s(sItemString, 16, "~");
								}
								else if (firstMinute && flashingTSATstart) {
									bool toggle = (now % 2) == 0;
									if (toggle) {
										ItemRGB = TAG_ORANGE;
									}
									else {
										ItemRGB = TAG_GREEN;
									}
									strcpy_s(sItemString, 16, TSATString.c_str());
								}
								else if (lastMinute) {
									//*pColorCode = TAG_COLOR_RGB_DEFINED;
									bool toggle = (now % 2) == 0;
									if (toggle || !flashingTSATend) {
										ItemRGB = TAG_YELLOW;
									}
									else {
										ItemRGB = TAG_GREEN;
									}
									strcpy_s(sItemString, 16, TSATString.c_str());
								}
								else if (moreLessFive) {
									ItemRGB = TAG_GREEN;
									strcpy_s(sItemString, 16, TSATString.c_str());
								}
								else if (oldTSAT) {
									ItemRGB = TAG_GREEN;
									if (!invalidateTSAT_Option) {
										ItemRGB = TAG_YELLOW;
									}
									strcpy_s(sItemString, 16, TSATString.c_str());
								}
								else {
									ItemRGB = TAG_GREENNOTACTIVE;
									strcpy_s(sItemString, 16, TSATString.c_str());
								}
							}
						}
						else if (ItemCode == TAG_ITEM_TSAT_TOBT_DIFF)
						{
							if (aircraftFind) {
								string value = slotList[pos].tsat.substr(0, 4) + getDiffTOBTTSAT(slotList[pos].tsat, slotList[pos].eobt);
								if (SU_ISSET) {
									ItemRGB = SU_SET_COLOR;
									strcpy_s(sItemString, 16, value.c_str());
								}
								else if (notYetEOBT) {
									//*pColorCode = TAG_COLOR_RGB_DEFINED;
									ItemRGB = TAG_GREY;
									strcpy_s(sItemString, 16, "~");
								}
								else if (lastMinute) {
									//*pColorCode = TAG_COLOR_RGB_DEFINED;
									ItemRGB = TAG_YELLOW;
									strcpy_s(sItemString, 16, value.c_str());
								}
								else if (moreLessFive) {
									//*pColorCode = TAG_COLOR_RGB_DEFINED;
									ItemRGB = TAG_GREEN;
									strcpy_s(sItemString, 16, value.c_str());
								}
								else if (oldTSAT) {
									//*pColorCode = TAG_COLOR_RGB_DEFINED;
									ItemRGB = TAG_GREEN;
									if (!invalidateTSAT_Option) {
										ItemRGB = TAG_YELLOW;
									}
									strcpy_s(sItemString, 16, value.c_str());
								}
								else {
									//*pColorCode = TAG_COLOR_RGB_DEFINED;
									ItemRGB = TAG_GREENNOTACTIVE;
									strcpy_s(sItemString, 16, value.c_str());
								}
							}
						}
						else if (ItemCode == NOW_TSAT_DIFF)
						{
							if (aircraftFind) {
								string value = getDiffNowTime(slotList[pos].tsat);
								if (SU_ISSET) {
									ItemRGB = SU_SET_COLOR;
									strcpy_s(sItemString, 16, value.c_str());
								}
								else if (notYetEOBT) {
									//*pColorCode = TAG_COLOR_RGB_DEFINED;
									ItemRGB = TAG_GREY;
									strcpy_s(sItemString, 16, "~");
								}
								else if (lastMinute) {
									//*pColorCode = TAG_COLOR_RGB_DEFINED;
									ItemRGB = TAG_YELLOW;
									strcpy_s(sItemString, 16, value.c_str());
								}
								else if (moreLessFive) {
									//*pColorCode = TAG_COLOR_RGB_DEFINED;
									ItemRGB = TAG_GREEN;
									strcpy_s(sItemString, 16, value.c_str());
								}
								else if (oldTSAT) {
									//*pColorCode = TAG_COLOR_RGB_DEFINED;
									ItemRGB = TAG_GREEN;
									if (!invalidateTSAT_Option) {
										ItemRGB = TAG_YELLOW;
									}
									strcpy_s(sItemString, 16, value.c_str());
								}
								else {
									//*pColorCode = TAG_COLOR_RGB_DEFINED;
									ItemRGB = TAG_GREENNOTACTIVE;
									strcpy_s(sItemString, 16, value.c_str());
								}
							}
						}
						else if (ItemCode == TAG_ITEM_TTOT)
						{
							if (TTOTString.length() > 0) {
								if (notYetEOBT) {
									ItemRGB = TAG_GREY;
									strcpy_s(sItemString, 16, "~");
								}
								else if (moreLessFive || lastMinute) {
									ItemRGB = TAG_TTOT;
									strcpy_s(sItemString, 16, TTOTString.substr(0, 4).c_str());
								}
								else {
									ItemRGB = TAG_TTOT;
									strcpy_s(sItemString, 16, TTOTString.substr(0, 4).c_str());
								}
							}
						}
						else if (ItemCode == NOW_TTOT_DIFF)
						{
							if (TTOTString.length() >= 4) {
								string value = getDiffNowTime(TTOTString.substr(0,4));
								if (notYetEOBT) {
									ItemRGB = TAG_GREY;
									strcpy_s(sItemString, 16, "~");
								}
								else if (moreLessFive || lastMinute) {
									ItemRGB = TAG_TTOT;
									strcpy_s(sItemString, 16, value.c_str());
								}
								else {
									ItemRGB = TAG_TTOT;
									strcpy_s(sItemString, 16, value.c_str());
								}
							}
						}
						else if (ItemCode == TAG_ITEM_ASAT)
						{
							if (ASATFound) {
								if ((string)FlightPlan.GetGroundState() == "" || (string)FlightPlan.GetGroundState() == "STUP" || (string)FlightPlan.GetGroundState() == "ST-UP") {
									if (ASATPlusFiveLessTen) {
										ItemRGB = TAG_YELLOW;
										strcpy_s(sItemString, 16, ASATtext.c_str());
									}
									else {
										ItemRGB = TAG_GREEN;
										strcpy_s(sItemString, 16, ASATtext.c_str());
									}
								}
								else {
									ItemRGB = SU_SET_COLOR;
									strcpy_s(sItemString, 16, ASATtext.c_str());
								}
							}
							else {
								ItemRGB = TAG_GREEN;
								strcpy_s(sItemString, 16, " ");
							}
						}
						else if (ItemCode == TAG_ITEM_ASRT)
						{
							string ASRTtext = getFlightStripInfo(FlightPlan, 0);
							if (!ASRTtext.empty()) {
								//*pColorCode = TAG_COLOR_RGB_DEFINED;
								if (SU_ISSET) {
									ItemRGB = SU_SET_COLOR;
								}
								else {
									ItemRGB = TAG_ASRT;
								}
								strcpy_s(sItemString, 16, ASRTtext.c_str());
							}
							else {
								//*pColorCode = TAG_COLOR_RGB_DEFINED;
								ItemRGB = TAG_ASRT;
								strcpy_s(sItemString, 16, " ");
							}
						}
						else if (ItemCode == TAG_ITEM_READYSTARTUP)
						{
							string ASRTtext = getFlightStripInfo(FlightPlan, 0);
							if (!ASRTtext.empty()) {
								ItemRGB = TAG_GREEN;
								strcpy_s(sItemString, 16, "RSTUP");
							}
							else {
								ItemRGB = TAG_RED;
								strcpy_s(sItemString, 16, "RSTUP");
							}
						}
						else if (ItemCode == TAG_ITEM_E)
						{
							if (notYetEOBT) {
								ItemRGB = TAG_GREEN;
								strcpy_s(sItemString, 16, "P");
							}
							else {
								ItemRGB = TAG_GREEN;
								strcpy_s(sItemString, 16, "C");
							}
						}
						else if (ItemCode == TAG_ITEM_FLOW_MESSAGE) {
							for (ServerRestricted sr : serverRestrictedPlanes) {
								if (sr.callsign == (string)FlightPlan.GetCallsign()) {
									ItemRGB = TAG_YELLOW;
									strcpy_s(sItemString, 50, sr.reason.c_str());
								}
							}
						}
						else if (ItemCode == TAG_ITEM_CTOT)
						{
							for (ServerRestricted sr : serverRestrictedPlanes) {
								if (sr.callsign == (string)FlightPlan.GetCallsign()) {
									ItemRGB = TAG_CTOT;
									for (size_t i = 0; i < myNetworkStatus.size(); i++) {
										if (myNetworkStatus[i][0] == callsign && myNetworkStatus[i][1] == "REA" && !ASATFound) {
											ItemRGB = TAG_YELLOW;
										}
									}
									strcpy_s(sItemString, 16, sr.ctot.c_str());
								}
							}
						}
						else if (ItemCode == NOW_CTOT_DIFF)
						{
							for (ServerRestricted sr : serverRestrictedPlanes) {
								if (sr.callsign == (string)FlightPlan.GetCallsign()) {
									ItemRGB = TAG_CTOT;
									for (size_t i = 0; i < myNetworkStatus.size(); i++) {
										if (myNetworkStatus[i][0] == callsign && myNetworkStatus[i][1] == "REA" && !ASATFound) {
											ItemRGB = TAG_YELLOW;
										}
									}
									string value = getDiffNowTime(sr.ctot);
									strcpy_s(sItemString, 16, value.c_str());
								}
							}
						}
						else if (ItemCode == TAG_ITEM_EV_CTOT) {
							bool inEvCtotsList = false;
							string slot = "";
							for (size_t i = 0; i < evCtots.size(); i++) {
								if (evCtots[i][0] == callsign) {
									inEvCtotsList = true;
									slot = evCtots[i][1];
								}
							}
							if (inEvCtotsList) {
								ItemRGB = TAG_GREY;
								strcpy_s(sItemString, 16, slot.c_str());
							}
						}
						else if (ItemCode == TAG_ITEM_NETWORK_STATUS) {
						string status = "";
						for (size_t i = 0; i < myNetworkStatus.size(); i++) {
							if (myNetworkStatus[i][0] == callsign) {
								status = myNetworkStatus[i][1];
							}
						}
						if (status != "") {
							ItemRGB = TAG_YELLOW;
							if (status == "REA") {
								ItemRGB = TAG_YELLOW;
								strcpy_s(sItemString, 16, status.c_str());
							}
							else if (status.find("FLS") != string::npos) {
								ItemRGB = TAG_RED;
								status = GetTimedStatus(status);
								strcpy_s(sItemString, 16, status.c_str());
							}
						}
						}
						else if (ItemCode == TAG_ITEM_NETWORK_STATUS_AIRBORNE) {
							string status = "";
							for (size_t i = 0; i < myNetworkStatus.size(); i++) {
								if (myNetworkStatus[i][0] == callsign) {
									status = myNetworkStatus[i][1];
								}
							}
							if (status != "") {
								ItemRGB = TAG_YELLOW;
								if (status == "COMPLY") {
									ItemRGB = TAG_GREEN;
									status = "C";
									strcpy_s(sItemString, 16, status.c_str());
								}
								else if (status == "AIRB") {
									ItemRGB = TAG_RED;
									status = "A";
									strcpy_s(sItemString, 16, status.c_str());
								}
							}
							}
						else if (ItemCode == TAG_ITEM_ON_TIME_STATUS) {
							string status = "";
							for (size_t i = 0; i < onTimeStatus.size(); i++) {
								if (onTimeStatus[i][0] == callsign) {
									status = onTimeStatus[i][1];
								}
							}
							if (status != "") {
								ItemRGB = TAG_YELLOW;
								if (status.find("+") != string::npos) {
									ItemRGB = TAG_RED;
								}
								else if (status.find("-") != string::npos) {
									ItemRGB = TAG_GREEN;
								}
								else {
									status = "";
								}
								strcpy_s(sItemString, 16, status.c_str());
							}
						}
						else if (ItemCode == TAG_ITEM_SEND_STATUS) {
								ItemRGB = TAG_RED;
								for (string flt : messagesSent) {
									if (flt == callsign) {
										ItemRGB = TAG_GREEN;
									}
								}
								strcpy_s(sItemString, 16, "SEND");
						}
						else if (ItemCode == TAG_ITEM_DEICE) {
							string status = "";
							for (vector<string> deice : deiceList) {
								if (deice[0] == callsign) {
									status = deice[1];
								}
							}
							ItemRGB = TAG_YELLOW;
							strcpy_s(sItemString, 16, status.c_str());
						}
						else if (ItemCode == TAG_ITEM_TOBT_SETBY) {
							string status = "";
							for (vector<string> reqTobtType : reqTobtTypes) {
								if (reqTobtType[0] == callsign) {
									status = reqTobtType[1];
								}
							}
							ItemRGB = TAG_GREEN;
							strcpy_s(sItemString, 16, status.c_str());
						}
					}
					else
					{
						if (ItemCode == TAG_ITEM_EOBT)
						{
							string ShowEOBT = formatTime(FlightPlan.GetFlightPlanData().GetEstimatedDepartureTime());
							ItemRGB = TAG_EOBT;
							for (size_t i = 0; i < myNetworkStatus.size(); i++) {
								if (myNetworkStatus[i][0] == callsign) {
									if (myNetworkStatus[i][1].find("FLS") != string::npos) {
										ItemRGB = TAG_RED;
									}
									break;
								}
							}
							strcpy_s(sItemString, 16, ShowEOBT.c_str());
						}
						else if (ItemCode == TAG_ITEM_TOBT)
						{
							ItemRGB = TAG_GREY;
							strcpy_s(sItemString, 16, "----");
						}
						else if (ItemCode == TAG_ITEM_ETOBT)
						{
							ItemRGB = TAG_GREY;
							strcpy_s(sItemString, 16, "----");
						}
						else if (ItemCode == TAG_ITEM_FLOW_MESSAGE) {
							if (aircraftFind) {
								if (slotList[pos].hasManualCtot) {
									string message = "MAN ACT";
									if (slotList[pos].ctot != "") {
										message = slotList[pos].flowReason;
									}
									ItemRGB = TAG_YELLOW;
									strcpy_s(sItemString, 16, message.c_str());
								}
								else if (slotList[pos].hasEcfmpRestriction) {
									ItemRGB = TAG_YELLOW;
									string message = slotList[pos].ecfmpRestriction.ident;
									strcpy_s(sItemString, 16, message.c_str());
								}
							}
							else {
								for (ServerRestricted sr : serverRestrictedPlanes) {
									if (sr.callsign == (string)FlightPlan.GetCallsign()) {
										ItemRGB = TAG_YELLOW;
										strcpy_s(sItemString, 50, sr.reason.c_str());
									}
								}
							}
						}
						else if (ItemCode == TAG_ITEM_CTOT)
						{
							if (aircraftFind) {
								if (slotList[pos].hasManualCtot) {
									string value = "";
									if (slotList[pos].ctot == "") {
										value = slotList[pos].ttot.substr(0, 4);
										ItemRGB = TAG_ORANGE;
									}
									else {
										value = slotList[pos].ctot;
										ItemRGB = TAG_CTOT;
										for (size_t i = 0; i < myNetworkStatus.size(); i++) {
											if (myNetworkStatus[i][0] == callsign && myNetworkStatus[i][1] == "REA") {
												ItemRGB = TAG_YELLOW;
											}
										}
									}
									strcpy_s(sItemString, 16, value.c_str());
								}
								else if (slotList[pos].hasEcfmpRestriction) {
									ItemRGB = TAG_RED;
									strcpy_s(sItemString, 16, slotList[pos].ttot.substr(0, 4).c_str());
								}
							}
							else {
								for (ServerRestricted sr : serverRestrictedPlanes) {
									if (sr.callsign == (string)FlightPlan.GetCallsign()) {
										ItemRGB = TAG_CTOT;
										for (size_t i = 0; i < myNetworkStatus.size(); i++) {
											if (myNetworkStatus[i][0] == callsign && myNetworkStatus[i][1] == "REA") {
												ItemRGB = TAG_YELLOW;
											}
										}
										strcpy_s(sItemString, 16, sr.ctot.c_str());
									}
								}
							}
						}
						else if (ItemCode == NOW_CTOT_DIFF)
						{
							bool inreaList = false;
							for (string s : reaCTOTSent) {
								if (s == callsign) {
									inreaList = true;
								}
							}

							if (aircraftFind) {
								if (slotList[pos].hasManualCtot || slotList[pos].hasEcfmpRestriction) {
									string value = getDiffNowTime(slotList[pos].ttot);
									if (slotList[pos].ctot == "") {
										ItemRGB = TAG_ORANGE;
									}
									else {
										ItemRGB = TAG_CTOT;
										for (size_t i = 0; i < myNetworkStatus.size(); i++) {
											if (myNetworkStatus[i][0] == callsign && myNetworkStatus[i][1] == "REA") {
												ItemRGB = TAG_YELLOW;
											}
										}
									}
									strcpy_s(sItemString, 16, value.c_str());
								}
							}
						}
						else if (ItemCode == TAG_ITEM_EV_CTOT) {
							bool inEvCtotsList = false;
							string slot = "";
							for (size_t i = 0; i < evCtots.size(); i++) {
								if (evCtots[i][0] == callsign) {
									inEvCtotsList = true;
									slot = evCtots[i][1];
								}
							}
							if (inEvCtotsList) {
								ItemRGB = TAG_GREY;
								strcpy_s(sItemString, 16, slot.c_str());
							}
						}
						else if (ItemCode == TAG_ITEM_NETWORK_STATUS) {
							string status = "";
							for (size_t i = 0; i < myNetworkStatus.size(); i++) {
								if (myNetworkStatus[i][0] == callsign) {
									status = myNetworkStatus[i][1];
								}
							}
							if (status != "") {
								ItemRGB = TAG_YELLOW;
								if (status == "REA") {
									ItemRGB = TAG_YELLOW;
									strcpy_s(sItemString, 16, status.c_str());
								}
								else if (status.find("FLS") != string::npos) {
									ItemRGB = TAG_RED;
									status = GetTimedStatus(status);
									strcpy_s(sItemString, 16, status.c_str());
								}
							}
						}
						else if (ItemCode == TAG_ITEM_NETWORK_STATUS_AIRBORNE) {
							string status = "";
							for (size_t i = 0; i < myNetworkStatus.size(); i++) {
								if (myNetworkStatus[i][0] == callsign) {
									status = myNetworkStatus[i][1];
								}
							}
							if (status != "") {
								ItemRGB = TAG_YELLOW;
								if (status == "COMPLY") {
									ItemRGB = TAG_GREEN;
									status = "C";
									strcpy_s(sItemString, 16, status.c_str());
								}
								else if (status == "AIRB") {
									ItemRGB = TAG_RED;
									status = "A";
									strcpy_s(sItemString, 16, status.c_str());
								}
							}
						}
						else if (ItemCode == TAG_ITEM_ON_TIME_STATUS) {
							string status = "";
							for (size_t i = 0; i < onTimeStatus.size(); i++) {
								if (onTimeStatus[i][0] == callsign) {
									status = onTimeStatus[i][1];
								}
							}
							if (status != "") {
								ItemRGB = TAG_YELLOW;
								if (status.find("+") != string::npos) {
									ItemRGB = TAG_RED;
								}
								else if (status.find("-") != string::npos) {
									ItemRGB = TAG_GREEN;
								}
								else {
									status = "";
								}
								strcpy_s(sItemString, 16, status.c_str());
							}
						}
						else if (ItemCode == TAG_ITEM_SEND_STATUS) {
							ItemRGB = TAG_RED;
							for (string flt : messagesSent) {
								if (flt == callsign) {
									ItemRGB = TAG_GREEN;
								}
							}
							strcpy_s(sItemString, 16, "SEND");
						}
						else if (ItemCode == TAG_ITEM_DEICE) {
							string status = "";
							for (vector<string> deice : deiceList) {
								if (deice[0] == callsign) {
									status = deice[1];
								}
							}
							ItemRGB = TAG_YELLOW;
							strcpy_s(sItemString, 16, status.c_str());
						}
						else if (ItemCode == TAG_ITEM_TOBT_SETBY) {
							string status = "";
							for (vector<string> reqTobtType : reqTobtTypes) {
								if (reqTobtType[0] == callsign) {
									status = reqTobtType[1];
								}
							}
							ItemRGB = TAG_GREEN;
							strcpy_s(sItemString, 16, status.c_str());
						}
					}
				}
			}
			else
			{
				string EOBTstring = FlightPlan.GetFlightPlanData().GetEstimatedDepartureTime();
				string EOBTfinal = formatTime(EOBTstring);

				if (ItemCode == TAG_ITEM_EOBT)
				{
					ItemRGB = TAG_EOBT;
					for (size_t i = 0; i < myNetworkStatus.size(); i++) {
						if (myNetworkStatus[i][0] == callsign) {
							if (myNetworkStatus[i][1].find("FLS") != string::npos) {
								ItemRGB = TAG_RED;
							}
							break;
						}
					}
					strcpy_s(sItemString, 16, EOBTfinal.c_str());
				}
				else if (ItemCode == TAG_ITEM_TOBT)
				{
					ItemRGB = TAG_GREY;
					strcpy_s(sItemString, 16, "----");
				}
				else if (ItemCode == TAG_ITEM_ETOBT)
				{
					ItemRGB = TAG_GREY;
					strcpy_s(sItemString, 16, "----");
				}
				else if (ItemCode == TAG_ITEM_FLOW_MESSAGE) {
					if (aircraftFind) {
						if (slotList[pos].hasManualCtot) {
							string message = "MAN ACT";
							if (slotList[pos].ctot != "") {
								message = slotList[pos].flowReason;
							}
							ItemRGB = TAG_YELLOW;
							strcpy_s(sItemString, 16, message.c_str());
						}
						else if (slotList[pos].hasEcfmpRestriction) {
							ItemRGB = TAG_YELLOW;
							string message = slotList[pos].ecfmpRestriction.ident;
							strcpy_s(sItemString, 16, message.c_str());
						}
					}
					else {
						for (ServerRestricted sr : serverRestrictedPlanes) {
							if (sr.callsign == (string)FlightPlan.GetCallsign()) {
								ItemRGB = TAG_YELLOW;
								strcpy_s(sItemString, 50, sr.reason.c_str());
							}
						}
					}
				}
				else if (ItemCode == TAG_ITEM_CTOT)
				{
					if (aircraftFind) {
						if (slotList[pos].hasManualCtot) {
							string value = "";
							if (slotList[pos].ctot == "") {
								value = slotList[pos].ttot.substr(0, 4);
								ItemRGB = TAG_ORANGE;
							}
							else {
								value = slotList[pos].ctot;
								ItemRGB = TAG_CTOT;
								for (size_t i = 0; i < myNetworkStatus.size(); i++) {
									if (myNetworkStatus[i][0] == callsign && myNetworkStatus[i][1] == "REA") {
										ItemRGB = TAG_YELLOW;
									}
								}
							}
							strcpy_s(sItemString, 16, value.c_str());
						}
						else if (slotList[pos].hasEcfmpRestriction) {
							ItemRGB = TAG_RED;
							strcpy_s(sItemString, 16, slotList[pos].ttot.substr(0, 4).c_str());
						}
					}
					else {
						for (ServerRestricted sr : serverRestrictedPlanes) {
							if (sr.callsign == (string)FlightPlan.GetCallsign()) {
								ItemRGB = TAG_CTOT;
								for (size_t i = 0; i < myNetworkStatus.size(); i++) {
									if (myNetworkStatus[i][0] == callsign && myNetworkStatus[i][1] == "REA") {
										ItemRGB = TAG_YELLOW;
									}
								}
								strcpy_s(sItemString, 16, sr.ctot.c_str());
							}
						}
					}
				}
				else if (ItemCode == NOW_CTOT_DIFF)
				{
					bool inreaList = false;
					for (string s : reaCTOTSent) {
						if (s == callsign) {
							inreaList = true;
						}
					}

					if (aircraftFind) {
						if (slotList[pos].hasManualCtot || slotList[pos].hasEcfmpRestriction) {
							string value = getDiffNowTime(slotList[pos].ttot);
							if (slotList[pos].ctot == "") {
								ItemRGB = TAG_ORANGE;
							}
							else {
								ItemRGB = TAG_CTOT;
								for (size_t i = 0; i < myNetworkStatus.size(); i++) {
									if (myNetworkStatus[i][0] == callsign && myNetworkStatus[i][1] == "REA") {
										ItemRGB = TAG_YELLOW;
									}
								}
							}
							strcpy_s(sItemString, 16, value.c_str());
						}
					}
				}
				else if (ItemCode == TAG_ITEM_EV_CTOT) {
					bool inEvCtotsList = false;
					string slot = "";
					for (size_t i = 0; i < evCtots.size(); i++) {
						if (evCtots[i][0] == callsign) {
							inEvCtotsList = true;
							slot = evCtots[i][1];
						}
					}
					if (inEvCtotsList) {
						ItemRGB = TAG_GREY;
						strcpy_s(sItemString, 16, slot.c_str());
					}
				}
				else if (ItemCode == TAG_ITEM_NETWORK_STATUS) {
					string status = "";
					for (size_t i = 0; i < myNetworkStatus.size(); i++) {
						if (myNetworkStatus[i][0] == callsign) {
							status = myNetworkStatus[i][1];
						}
					}
					if (status != "") {
						ItemRGB = TAG_YELLOW;
						if (status == "REA") {
							ItemRGB = TAG_YELLOW;
							strcpy_s(sItemString, 16, status.c_str());
						}
						else if (status.find("FLS") != string::npos) {
							ItemRGB = TAG_RED;
							status = GetTimedStatus(status);
							strcpy_s(sItemString, 16, status.c_str());
						}
					}
				}
				else if (ItemCode == TAG_ITEM_NETWORK_STATUS_AIRBORNE) {
					string status = "";
					for (size_t i = 0; i < myNetworkStatus.size(); i++) {
						if (myNetworkStatus[i][0] == callsign) {
							status = myNetworkStatus[i][1];
						}
					}
					if (status != "") {
						ItemRGB = TAG_YELLOW;
						if (status == "COMPLY") {
							ItemRGB = TAG_GREEN;
							status = "C";
							strcpy_s(sItemString, 16, status.c_str());
						}
						else if (status == "AIRB") {
							ItemRGB = TAG_RED;
							status = "A";
							strcpy_s(sItemString, 16, status.c_str());
						}
					}
						}
				else if (ItemCode == TAG_ITEM_ON_TIME_STATUS) {
					string status = "";
					for (size_t i = 0; i < onTimeStatus.size(); i++) {
						if (onTimeStatus[i][0] == callsign) {
							status = onTimeStatus[i][1];
						}
					}
					if (status != "") {
						ItemRGB = TAG_YELLOW;
						if (status.find("+") != string::npos) {
							ItemRGB = TAG_RED;
						}
						else if (status.find("-") != string::npos) {
							ItemRGB = TAG_GREEN;
						}
						else {
							status = "";
						}
						strcpy_s(sItemString, 16, status.c_str());
					}
				}
				else if (ItemCode == TAG_ITEM_SEND_STATUS) {
					ItemRGB = TAG_RED;
					for (string flt : messagesSent) {
						if (flt == callsign) {
							ItemRGB = TAG_GREEN;
						}
					}
					strcpy_s(sItemString, 16, "SEND");
				}
				else if (ItemCode == TAG_ITEM_DEICE) {
					string status = "";
					for (vector<string> deice : deiceList) {
						if (deice[0] == callsign) {
							status = deice[1];
						}
					}
					ItemRGB = TAG_YELLOW;
					strcpy_s(sItemString, 16, status.c_str());
				}
				else if (ItemCode == TAG_ITEM_TOBT_SETBY) {
					string status = "";
					for (vector<string> reqTobtType : reqTobtTypes) {
						if (reqTobtType[0] == callsign) {
							status = reqTobtType[1];
						}
					}
					ItemRGB = TAG_GREEN;
					strcpy_s(sItemString, 16, status.c_str());
				}
			}

			if (ItemRGB != 0xFFFFFFFF)
			{
				*pColorCode = TAG_COLOR_RGB_DEFINED;
				*pRGB = ItemRGB;
			}
		}
		else {

			//Check if update in the queue
			std::vector<Plane> localPlaneQueue;
			{
				std::lock_guard<std::mutex> lock(apiQueueResponseMutex);
				localPlaneQueue.swap(apiQueueResponse);
				apiQueueResponse.clear();
			}

			if (!localPlaneQueue.empty()) {
				for (const Plane p : localPlaneQueue) {
					for (int t = 0; t < slotList.size(); t++) {
						if (p.callsign == slotList[t].callsign) {
							slotList[t] = p;
						}
					}
				}
			}

			bool master = false;
			for (string apt : masterAirports)
			{
				if (apt == FlightPlan.GetFlightPlanData().GetOrigin()) {
					master = true;
				}
			}

			string EOBTstring = FlightPlan.GetFlightPlanData().GetEstimatedDepartureTime();
			string EOBTfinal = formatTime(EOBTstring);

			int slotListPos = -1;
			for (int i = 0; i < slotList.size(); i++) {
				if (slotList[i].callsign == (string)FlightPlan.GetCallsign()) {
					slotListPos = i;
				}
			}

			if (slotListPos == -1) {
				EcfmpRestriction myEcfmp;
				Plane p(callsign, EOBTfinal, EOBTfinal, EOBTfinal, "", "", myEcfmp, false, false, true, false);
				slotList.push_back(p);
			}

			if (master) {
				//Sync data
				if (slotListPos != -1) {
					//Update TTOT to Slaves
					if (slotList[slotListPos].ttot != getFlightStripInfo(FlightPlan, 4)) {
						setFlightStripInfo(FlightPlan, slotList[slotListPos].ttot, 4);
					}
					//Update Manual CTOT to Slaves
					if (slotList[slotListPos].hasManualCtot == true && getFlightStripInfo(FlightPlan, 7) != "1") {
						setFlightStripInfo(FlightPlan, "1", 7);
					}
					else if (slotList[slotListPos].hasManualCtot == false && getFlightStripInfo(FlightPlan, 7) != "") {
						setFlightStripInfo(FlightPlan, "", 7);
					}

					//Push to other ATCs
					if ((timeNow - countTimeNonCdm) > refreshTime) {
						countTimeNonCdm = timeNow;
						addLogLine("[AUTO] - REFRESH CDM INTERNAL DATA (Non-CDM)");
						for (size_t t = 0; t < slotList.size(); t++) {
							CFlightPlan fpSelected = FlightPlanSelect(slotList[t].callsign.c_str());
							PushToOtherControllers(fpSelected);
						}
					}
				}
			}

			//Get Time NOW
			time_t rawtime;
			struct tm ptm;
			time(&rawtime);
			gmtime_s(&ptm, &rawtime);
			string hour = to_string(ptm.tm_hour % 24);
			string min = to_string(ptm.tm_min);

			//Set/Remove AOBT automaically base on state
			bool ASATFound = false;
			int ASATpos = 0;
			bool correctState = false;
			string ASATtext = " ";
			for (size_t x = 0; x < asatList.size(); x++)
			{
				string actualListCallsign = asatList[x].substr(0, asatList[x].find(","));
				if (actualListCallsign == callsign) {
					ASATFound = true;
					ASATpos = x;
				}
			}

			if ((string)FlightPlan.GetGroundState() == "STUP" || (string)FlightPlan.GetGroundState() == "ST-UP" || (string)FlightPlan.GetGroundState() == "PUSH" || (string)FlightPlan.GetGroundState() == "TAXI" || (string)FlightPlan.GetGroundState() == "DEPA") {
				correctState = true;
			}

			if (!ASATFound) {
				if (correctState) {
					ASATtext = formatTime(hour + min);
					asatList.push_back(callsign + "," + ASATtext.substr(0, 4));
					std::thread t(&CDM::setCdmSts, this, callsign, "AOBT/" + ASATtext);
					t.detach();
					ASATFound = true;
				}
			}
			else {
				if (correctState) {
					ASATtext = asatList[ASATpos].substr(asatList[ASATpos].length() - 4, 4);
				}
				else if (!correctState) {
					asatList.erase(asatList.begin() + ASATpos);
					std::thread t(&CDM::setCdmSts, this, callsign, "AOBT/NULL");
					t.detach();
					ASATFound = false;
				}
			}

			if (ItemCode == TAG_ITEM_EOBT)
			{
				ItemRGB = TAG_EOBT;
				for (size_t i = 0; i < myNetworkStatus.size(); i++) {
					if (myNetworkStatus[i][0] == callsign) {
						if (myNetworkStatus[i][1].find("FLS") != string::npos) {
							ItemRGB = TAG_RED;
						}
						break;
					}
				}
				string eobtValue = EOBTfinal;
				for (vector<string> obtItem : obtList) {
					if (obtItem[0] == callsign && obtItem[1] != "") {
						eobtValue = obtItem[1];
						break;
					}
				}
				strcpy_s(sItemString, 16, eobtValue.c_str());
			}
			if (ItemCode == TAG_ITEM_ETOBT)
			{
				ItemRGB = TAG_EOBT;
				for (size_t i = 0; i < myNetworkStatus.size(); i++) {
					if (myNetworkStatus[i][0] == callsign) {
						if (myNetworkStatus[i][1].find("FLS") != string::npos) {
							ItemRGB = TAG_RED;
						}
						break;
					}
				}
				string eobtValue = EOBTfinal;
				for (vector<string> obtItem : obtList) {
					if (obtItem[0] == callsign && obtItem[1] != "") {
						eobtValue = obtItem[1];
						break;
					}
				}
				strcpy_s(sItemString, 16, eobtValue.c_str());
			}
			if (ItemCode == TAG_ITEM_CTOT)
			{
				for (ServerRestricted sr : serverRestrictedPlanes) {
					if (sr.callsign == (string)FlightPlan.GetCallsign()) {
						ItemRGB = TAG_CTOT;
						for (size_t i = 0; i < myNetworkStatus.size(); i++) {
							if (myNetworkStatus[i][0] == callsign && myNetworkStatus[i][1] == "REA" && !ASATFound) {
								ItemRGB = TAG_YELLOW;
							}
						}
						strcpy_s(sItemString, 16, sr.ctot.c_str());
					}
				}
			}
			else if (ItemCode == NOW_CTOT_DIFF)
			{
				for (ServerRestricted sr : serverRestrictedPlanes) {
					if (sr.callsign == (string)FlightPlan.GetCallsign()) {
						string value = getDiffNowTime(sr.ctot);
						ItemRGB = TAG_CTOT;
						for (size_t i = 0; i < myNetworkStatus.size(); i++) {
							if (myNetworkStatus[i][0] == callsign && myNetworkStatus[i][1] == "REA" && !ASATFound) {
								ItemRGB = TAG_YELLOW;
							}
						}
						strcpy_s(sItemString, 16, value.c_str());
					}
				}
			}
			if (ItemCode == TAG_ITEM_FLOW_MESSAGE)
			{
				for (ServerRestricted sr : serverRestrictedPlanes) {
					if (sr.callsign == (string)FlightPlan.GetCallsign()) {
						ItemRGB = TAG_YELLOW;
						strcpy_s(sItemString, 50, sr.reason.c_str());
					}
				}
			}
			else if (ItemCode == TAG_ITEM_NETWORK_STATUS) {
				string status = "";
				for (size_t i = 0; i < myNetworkStatus.size(); i++) {
					if (myNetworkStatus[i][0] == callsign) {
						status = myNetworkStatus[i][1];
					}
				}
				if (status != "") {
					ItemRGB = TAG_YELLOW;
					if (status == "REA") {
						ItemRGB = TAG_YELLOW;
						strcpy_s(sItemString, 16, status.c_str());
					}
					else if (status.find("FLS") != string::npos) {
						ItemRGB = TAG_RED;
						status = GetTimedStatus(status);
						strcpy_s(sItemString, 16, status.c_str());
					}
				}
			}
			else if (ItemCode == TAG_ITEM_NETWORK_STATUS_AIRBORNE) {
				string status = "";
				for (size_t i = 0; i < myNetworkStatus.size(); i++) {
					if (myNetworkStatus[i][0] == callsign) {
						status = myNetworkStatus[i][1];
					}
				}
				if (status != "") {
					ItemRGB = TAG_YELLOW;
					if (status == "COMPLY") {
						ItemRGB = TAG_GREEN;
						status = "C";
						strcpy_s(sItemString, 16, status.c_str());
					}
					else if (status == "AIRB") {
						ItemRGB = TAG_RED;
						status = "A";
						strcpy_s(sItemString, 16, status.c_str());
					}
				}
			}
			else if (ItemCode == TAG_ITEM_ON_TIME_STATUS) {
				string status = "";
				for (size_t i = 0; i < onTimeStatus.size(); i++) {
					if (onTimeStatus[i][0] == callsign) {
						status = onTimeStatus[i][1];
					}
				}
				if (status != "") {
					ItemRGB = TAG_YELLOW;
					if (status.find("+") != string::npos) {
						ItemRGB = TAG_RED;
					}
					else if (status.find("-") != string::npos) {
						ItemRGB = TAG_GREEN;
					}
					else {
						status = "";
					}
					strcpy_s(sItemString, 16, status.c_str());
				}
			}
			else if (ItemCode == TAG_ITEM_SEND_STATUS) {
				ItemRGB = TAG_RED;
				for (string flt : messagesSent) {
					if (flt == callsign) {
						ItemRGB = TAG_GREEN;
					}
				}
				strcpy_s(sItemString, 16, "SEND");
			}
			if (ItemRGB != 0xFFFFFFFF)
			{
				*pColorCode = TAG_COLOR_RGB_DEFINED;
				*pRGB = ItemRGB;
			}
		}
	}
	}
	catch (const std::exception& e) {
		addLogLine("ERROR: Unhandled exception OnGetTagItem: " + (string)e.what());
	}
	catch (...) {
		addLogLine("ERROR: Unhandled exception OnGetTagItem");
	}
}

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

bool CDM::getRateFromUrl(string url) {
	vector<Rate> myRates;
	CURL* curl;
	CURLcode result = CURLE_FAILED_INIT;
	string readBuffer;
	long responseCode = 0;
	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
		result = curl_easy_perform(curl);
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
		curl_easy_cleanup(curl);
	}

	if (responseCode == 404 || responseCode == 401 || responseCode == 502 || CURLE_OK != result) {
		// handle error 404
		addLogLine("UNABLE TO LOAD TaxiZones URL...");
	}
	else {
		std::istringstream is(readBuffer);

		//Get data from .txt file
		bool found;
		string lineValue;
		while (getline(is, lineValue))
		{
			auto firstNonSpace = lineValue.find_first_not_of(" \t\r\n");
			if (firstNonSpace == std::string::npos)
				continue;

			if (lineValue[firstNonSpace] == '#')
				continue;

			vector<string> data = explode(lineValue, ':');
			if (data.size() == 9) {
				string myRateDataAirport = data[0];
				vector<string> ArrRwyList = explode(data[2], ',');
				if (ArrRwyList.size() == 0) {
					ArrRwyList.push_back(data[2]);
				}
				vector<string> NotArrRwyList = explode(data[3], ',');
				if (NotArrRwyList.size() == 0) {
					NotArrRwyList.push_back(data[3]);
				}
				vector<string> DepRwyList = explode(data[5], ',');
				if (DepRwyList.size() == 0) {
					DepRwyList.push_back(data[5]);
				}
				vector<string> NotDepRwyList = explode(data[6], ',');
				if (NotDepRwyList.size() == 0) {
					NotDepRwyList.push_back(data[6]);
				}
				vector<string> DependentRwyList = explode(data[7], ',');
				if (DependentRwyList.size() == 0) {
					DependentRwyList.push_back(data[7]);
				}
				vector<string> ratesFromData = explode(data[8], ',');
				vector<string> ratesFromDataRate;
				vector<string> ratesFromDataRateLvo;
				if (ratesFromData.size() == 0) {
					ratesFromData.push_back(data[8]);
				}
				else {
					for (string completeRate : ratesFromData) {
						vector<string> tempCompleteRate = explode(completeRate, '_');
						ratesFromDataRate.push_back(tempCompleteRate[0]);
						ratesFromDataRateLvo.push_back(tempCompleteRate[1]);

					}
				}

				Rate myRate(myRateDataAirport, ArrRwyList, NotArrRwyList, DepRwyList, NotDepRwyList, DependentRwyList, ratesFromDataRate, ratesFromDataRateLvo);

				myRates.push_back(myRate);

				found = false;
				for (string airport : CDMairports) {
					if (airport == myRateDataAirport) {
						found = true;
					}
				}
				if (!found) {
					CDMairports.push_back(myRateDataAirport);
				}
				
			}
		}
	}
	vector<Rate> ratesChanged;
	for (Rate r1 : rate) {
		for (Rate r2 : myRates) {
			if (r1.airport == r2.airport && r1.arrRwyNo == r2.arrRwyNo && r1.arrRwyYes == r2.arrRwyYes && r1.dependentRwy == r2.dependentRwy
				&& r1.depRwyNo == r2.depRwyNo && r1.depRwyYes == r2.depRwyYes && r1.rates.size() == r2.rates.size() && r1.ratesLvo.size() == r2.ratesLvo.size()) {
				for (size_t i = 0; i < r1.rates.size(); i++) {
					if (r1.rates[i] != r2.rates[i]) {
						ratesChanged.push_back(r2);
					}
				}
			}
		}
	}
	for (Rate r : ratesChanged) {
		for (size_t z = 0; z < slotList.size(); z++) {
			CFlightPlan fp = FlightPlanSelect(slotList[z].callsign.c_str());
			if (!fp.IsValid()) {
				continue;
			}
			if ((string)fp.GetGroundState() != "STUP" && (string)fp.GetGroundState() != "ST-UP" && (string)fp.GetGroundState() != "PUSH" && (string)fp.GetGroundState() != "TAXI" && (string)fp.GetGroundState() != "DEPA") {
				Rate dataRate = rateForRunway(fp.GetFlightPlanData().GetOrigin(), fp.GetFlightPlanData().GetDepartureRwy());
				if (r.airport == dataRate.airport && r.arrRwyNo == dataRate.arrRwyNo && r.arrRwyYes == dataRate.arrRwyYes && r.dependentRwy == dataRate.dependentRwy
					&& r.depRwyNo == dataRate.depRwyNo && r.depRwyYes == dataRate.depRwyYes && r.rates.size() == dataRate.rates.size() && r.ratesLvo.size() == dataRate.ratesLvo.size()) {
					for (string dr : dataRate.depRwyYes) {
						int myPos = 0;
						if (dataRate.rates.size() > 1) {
							for (size_t i = 0; i < dataRate.rates.size(); i++) {
								if (dataRate.rates[i] != r.rates[i]) {
									myPos = i;
								}
							}
						}
						//Increase time according the rate change
						slotList[z].tsat = calculateTime(slotList[z].tsat, (60.0 / stoi(r.rates[myPos])));
						slotList[z].ttot = calculateTime(slotList[z].ttot, (60.0 / stoi(r.rates[myPos])));
					}
				}
			}
		}
	}

	rate = myRates;
	initialRate = rate;

	return true;
}

bool CDM::getRate() {
	vector<Rate> myRates;
	//Get data from rate.txt file
	fstream rateFile;
	string lineValue;
	rateFile.open(rfad.c_str(), std::ios::in);
	if (!rateFile.is_open()) {
		sendMessage("Error", "rate.txt not found. Place rate.txt next to CDM.dll.");
		addLogLine("ERROR: rate.txt not found at " + rfad);
		return false;
	}
	bool found;
	while (getline(rateFile, lineValue))
	{
		vector<string> data = explode(lineValue, ':');
		if (data.size() == 9) {
			string myRateDataAirport = data[0];
			vector<string> ArrRwyList = explode(data[2], ',');
			if (ArrRwyList.size() == 0) {
				ArrRwyList.push_back(data[2]);
			}
			vector<string> NotArrRwyList = explode(data[3], ',');
			if (NotArrRwyList.size() == 0) {
				NotArrRwyList.push_back(data[3]);
			}
			vector<string> DepRwyList = explode(data[5], ',');
			if (DepRwyList.size() == 0) {
				DepRwyList.push_back(data[5]);
			}
			vector<string> NotDepRwyList = explode(data[6], ',');
			if (NotDepRwyList.size() == 0) {
				NotDepRwyList.push_back(data[6]);
			}
			vector<string> DependentRwyList = explode(data[7], ',');
			if (DependentRwyList.size() == 0) {
				DependentRwyList.push_back(data[7]);
			}
			vector<string> ratesFromData = explode(data[8], ',');
			vector<string> ratesFromDataRate;
			vector<string> ratesFromDataRateLvo;
			if (ratesFromData.size() == 0) {
				ratesFromData.push_back(data[8]);
			}
			else {
				for (string completeRate : ratesFromData) {
					vector<string> tempCompleteRate = explode(completeRate, '_');
					ratesFromDataRate.push_back(tempCompleteRate[0]);
					ratesFromDataRateLvo.push_back(tempCompleteRate[1]);

				}
			}

			Rate myRate(myRateDataAirport, ArrRwyList, NotArrRwyList, DepRwyList, NotDepRwyList, DependentRwyList, ratesFromDataRate, ratesFromDataRateLvo);

			myRates.push_back(myRate);

			found = false;
			for (string airport : CDMairports) {
				if (airport == myRateDataAirport) {
					found = true;
				}
			}
			if (!found) {
				CDMairports.push_back(myRateDataAirport);
			}

		}
	}
	vector<Rate> ratesChanged;
	for (Rate r1 : rate) {
		for (Rate r2 : myRates) {
			if (r1.airport == r2.airport && r1.arrRwyNo == r2.arrRwyNo && r1.arrRwyYes == r2.arrRwyYes && r1.dependentRwy == r2.dependentRwy
				&& r1.depRwyNo == r2.depRwyNo && r1.depRwyYes == r2.depRwyYes && r1.rates.size() == r2.rates.size() && r1.ratesLvo.size() == r2.ratesLvo.size()) {
				for (size_t i = 0; i < r1.rates.size(); i++) {
					if (r1.rates[i] != r2.rates[i]) {
						ratesChanged.push_back(r2);
					}
				}
			}
		}
	}
	for (Rate r : ratesChanged) {
		for (size_t z = 0; z < slotList.size(); z++) {
			CFlightPlan fp = FlightPlanSelect(slotList[z].callsign.c_str());
			if (!fp.IsValid()) {
				continue;
			}
			if ((string)fp.GetGroundState() != "STUP" && (string)fp.GetGroundState() != "ST-UP" && (string)fp.GetGroundState() != "PUSH" && (string)fp.GetGroundState() != "TAXI" && (string)fp.GetGroundState() != "DEPA") {
				Rate dataRate = rateForRunway(fp.GetFlightPlanData().GetOrigin(), fp.GetFlightPlanData().GetDepartureRwy());
				if (r.airport == dataRate.airport && r.arrRwyNo == dataRate.arrRwyNo && r.arrRwyYes == dataRate.arrRwyYes && r.dependentRwy == dataRate.dependentRwy
					&& r.depRwyNo == dataRate.depRwyNo && r.depRwyYes == dataRate.depRwyYes && r.rates.size() == dataRate.rates.size() && r.ratesLvo.size() == dataRate.ratesLvo.size()) {
					for (string dr : dataRate.depRwyYes) {
						int myPos = 0;
						if (dataRate.rates.size() > 1) {
							for (size_t i = 0; i < dataRate.rates.size(); i++) {
								if (dataRate.rates[i] != r.rates[i]) {
									myPos = i;
								}
							}
						}
						//Increase time according the rate change
						slotList[z].tsat = calculateTime(slotList[z].tsat, (60.0 / stoi(r.rates[myPos])));
						slotList[z].ttot = calculateTime(slotList[z].ttot, (60.0 / stoi(r.rates[myPos])));
					}
				}
			}
		}
	}

	rate = myRates;
	initialRate = rate;

	return true;
}

Rate CDM::rateForRunway(string airport, string depRwy) {
	string lineAirport, lineDepRwy;

	vector<string> myActiveRwysDep;
	vector<string> myActiveRwysArr;
	string myairport;
	for (CSectorElement runway = this->SectorFileElementSelectFirst(SECTOR_ELEMENT_RUNWAY);
		runway.IsValid();
		runway = this->SectorFileElementSelectNext(runway, SECTOR_ELEMENT_RUNWAY)) {
		if (runway.IsElementActive(false, 1)) {
			myairport = runway.GetAirportName();
			if (myairport.substr(0, 4) == airport) {
				myActiveRwysArr.push_back(runway.GetRunwayName(1));
			}
		}
		if (runway.IsElementActive(false, 0)) {
			myairport = runway.GetAirportName();
			if (myairport.substr(0, 4) == airport) {
				myActiveRwysArr.push_back(runway.GetRunwayName(0));
			}
		}
		if (runway.IsElementActive(true, 1)) {
			myairport = runway.GetAirportName();
			if (myairport.substr(0, 4) == airport) {
				myActiveRwysDep.push_back(runway.GetRunwayName(1));
			}
		}
		if (runway.IsElementActive(true, 0)) {
			myairport = runway.GetAirportName();
			if (myairport.substr(0, 4) == airport) {
				myActiveRwysDep.push_back(runway.GetRunwayName(0));
			}
		}
	}

	for (Rate r : rate) {
		if (r.airport == airport) {
			bool found = false;
			for (string dr : r.depRwyYes) {
				if (depRwy == dr) {
					found = true;
				}
				else if (dr == "*") {
					found = true;
				}
			}

			if (found) {
				bool foundArrRwyYes = false;
				for (string ar : r.arrRwyYes) {
					if (ar == "*") {
						foundArrRwyYes = true;
					}
					else {
						for (string arrRwy : myActiveRwysArr) {
							if (arrRwy == ar) {
								foundArrRwyYes = true;
							}
						}
					}
				}

				bool foundArrRwyNo = true;
				for (string arn : r.arrRwyNo) {
					if (arn == "*") {
						foundArrRwyNo = true;
					}
					else {
						bool foundIt = false;
						for (string arrRwy : myActiveRwysArr) {
							if (arrRwy != arn) {
								foundIt = true;
							}
						}
						if (!foundIt) {
							foundArrRwyNo = false;
						}
						else if (myActiveRwysArr.size() != r.arrRwyNo.size()) {
							foundArrRwyNo = false;
						}
					}
				}

				bool foundDepRwyYes = true;
				for (string dr : r.depRwyYes) {
					if (dr == "*") {
						foundDepRwyYes = true;
					}
					else {
						for (string myRwy : myActiveRwysDep) {
							if (myRwy == dr) {
								foundDepRwyYes = true;
							}
						}
					}
				}

				bool foundDepRwyNo = true;
				for (string drn : r.depRwyNo) {
					if (drn == "*") {
						foundDepRwyYes = true;
					}
					else {
						bool foundIt = false;
						for (string myRwy : myActiveRwysDep) {
							if (myRwy != drn) {
								foundIt = true;
							}
						}
						if (!foundIt) {
							foundDepRwyYes = false;
						}
						else if (myActiveRwysDep.size() != r.depRwyNo.size()) {
							foundDepRwyYes = false;
						}
					}
				}

				//Check if ok to be valid rate
				if (foundArrRwyYes && foundArrRwyNo && foundDepRwyYes && foundDepRwyNo) {
					return r;
				}
			}
		}
	}
	return Rate("-1");
}

void CDM::RemoveMasterAirports() {
	if (!masterAirports.empty()) {
		string ATC_Position = myAtcCallsign;
		if (ATC_Position.size() < 2) {
			ATC_Position = ControllerMyself().GetCallsign();
		}
		sendMessage("Removed master airports from previous connection.");
		std::thread t(&CDM::removeAllMasterAirports, this, ATC_Position);
		t.detach();
		myAtcCallsign = ControllerMyself().GetCallsign();
	}
}

vector<Plane> CDM::backgroundProcess_recaulculate() {
	addLogLine("Called backgroundProcess_recaulculate...");
	try{
		vector<Plane> tempSlotList;
		addLogLine("[AUTO] - Starting CDM recalculation process");
		//Get Time NOW
		time_t rawtime;
		struct tm ptm;
		time(&rawtime);
		gmtime_s(&ptm, &rawtime);
		string hour = to_string(ptm.tm_hour % 24);
		string min = to_string(ptm.tm_min);
		for (size_t i = 0; i < slotList.size(); i++)
		{

			//Update TSAT in scratchpad if enabled remarksOption
			if (remarksOption || remarksOptionCtot) {
				CFlightPlan fplSelect = FlightPlanSelect(slotList[i].callsign.c_str());
				if (!fplSelect.IsValid()) {
					continue;
				}
				string testTsat = slotList[i].tsat;
				string testCtot = slotList[i].ctot;
				if (
					(string)fplSelect.GetGroundState() != "STUP" &&
					(string)fplSelect.GetGroundState() != "ST-UP" &&
					(string)fplSelect.GetGroundState() != "PUSH" &&
					(string)fplSelect.GetGroundState() != "TAXI" &&
					(string)fplSelect.GetGroundState() != "DEPA")
				{
					if (testCtot.length() >= 4 && remarksOptionCtot) fplSelect.GetControllerAssignedData().SetScratchPadString(std::string(".C" + testCtot.substr(0, 4)).c_str());
					if (testTsat.length() >= 4 && remarksOption) fplSelect.GetControllerAssignedData().SetScratchPadString(testTsat.substr(0, 4).c_str());
				}
			}

			string myCallsign = slotList[i].callsign;

			string myTTOT, myTSAT, myEOBT, myAirport, myDepRwy = "";
			int myTTime = defTaxiTime;

			//Check if aircraft has state and no need to recalculate
			bool aicraftInFinalTimesList = false;
			for (string aircraft : finalTimesList) {
				if (aircraft == myCallsign) {
					aicraftInFinalTimesList = true;
				}
			}

			//Do not calculate if has CTOT
			//if (slotList[i].hasManualCtot && slotList[i].ctot != "") aicraftInFinalTimesList = false;

			if (!aicraftInFinalTimesList) {
				CFlightPlan myFlightPlan = FlightPlanSelect(myCallsign.c_str());
				if (!myFlightPlan.IsValid()) {
					continue;
				}

				for (size_t s = 0; s < planeAiportList.size(); s++)
				{
					if (myCallsign == planeAiportList[s].substr(0, planeAiportList[s].find(","))) {
						myAirport = planeAiportList[s].substr(planeAiportList[s].find(",") + 1, 4);
					}
				}

				bool depRwyFound = false;
				for (size_t t = 0; t < taxiTimesList.size(); t++)
				{
					if (myCallsign == taxiTimesList[t].substr(0, taxiTimesList[t].find(","))) {
						if (taxiTimesList[t].substr(taxiTimesList[t].find(",") + 3, 1) == ",") {
							myDepRwy = taxiTimesList[t].substr(taxiTimesList[t].find(",") + 1, 2);
							depRwyFound = true;
						}
						else if (taxiTimesList[t].substr(taxiTimesList[t].find(",") + 4, 1) == ",") {
							myDepRwy = taxiTimesList[t].substr(taxiTimesList[t].find(",") + 1, 3);
							depRwyFound = true;
						}

						if (taxiTimesList[t].substr(taxiTimesList[t].length() - 2, 1) == ",") {
							myTTime = stoi(taxiTimesList[t].substr(taxiTimesList[t].length() - 1, 1));
						}
						else {
							myTTime = stoi(taxiTimesList[t].substr(taxiTimesList[t].length() - 2, 2));
						}
					}
				}

				myEOBT = slotList[i].eobt;

				Rate dataRate = rateForRunway(myFlightPlan.GetFlightPlanData().GetOrigin(), myFlightPlan.GetFlightPlanData().GetDepartureRwy());

				bool tempAddTime_DELAY_TSAT = false;
				bool tempAddTime_DELAY_TTOT = false;
				string myTimeToAddTemp_DELAY = "";
				for (Delay d : delayList) {
					if (d.airport == myFlightPlan.GetFlightPlanData().GetOrigin() && d.rwy == myFlightPlan.GetFlightPlanData().GetDepartureRwy()) {
						if (d.type == "ttot") {
							tempAddTime_DELAY_TTOT = true;
							myTimeToAddTemp_DELAY = d.time + "00";
							break;
						}
						else if (d.type == "tsat") {
							tempAddTime_DELAY_TSAT = true;
							myTimeToAddTemp_DELAY = d.time + "00";
							break;
						}
					}
				}

				myTSAT = myEOBT;
				myTTOT = calculateTime(myEOBT, myTTime);
				if (addTime || tempAddTime_DELAY_TSAT || tempAddTime_DELAY_TTOT) {
					if (tempAddTime_DELAY_TSAT || tempAddTime_DELAY_TTOT) {
						string timeToUse = myTimeToAddTemp_DELAY;
						if (tempAddTime_DELAY_TTOT) {
							timeToUse = calculateLessTime(myTimeToAddTemp_DELAY, myTTime);
						}
						string timeToAddHour = timeToUse.substr(0, 2);
						string timeToAddMin = timeToUse.substr(2, 2);
						if (hour != "00") {
							if (timeToAddHour == "00") {
								timeToAddHour = "24";
							}
						}

						string myTSATHour = myTSAT.substr(0, 2);
						string myTSATMin = myTSAT.substr(2, 2);
						if (hour != "00") {
							if (myTSATHour == "00") {
								myTSATHour = "24";
							}
						}

						int difTime = GetdifferenceTime(timeToAddHour, timeToAddMin, myTSATHour, myTSATMin);
						bool fixTime = true;
						if (hour != timeToAddHour) {
							if (difTime > 40) {
								fixTime = false;
							}
						}
						else {
							if (difTime > 0) {
								fixTime = false;
							}
						}

						if (!fixTime) {
							myTSAT = timeToUse;
							myTTOT = calculateTime(timeToUse, myTTime);
						}
					}
					else {
						string timeToAddHour = myTimeToAdd.substr(0, 2);
						string timeToAddMin = myTimeToAdd.substr(2, 2);
						if (hour != "00") {
							if (timeToAddHour == "00") {
								timeToAddHour = "24";
							}
						}

						string myTSATHour = myTSAT.substr(0, 2);
						string myTSATMin = myTSAT.substr(2, 2);
						if (hour != "00") {
							if (myTSATHour == "00") {
								myTSATHour = "24";
							}
						}

						int difTime = GetdifferenceTime(timeToAddHour, timeToAddMin, myTSATHour, myTSATMin);
						bool fixTime = true;
						if (hour != timeToAddHour) {
							if (difTime > 40) {
								fixTime = false;
							}
						}
						else {
							if (difTime > 0) {
								fixTime = false;
							}
						}

						if (!fixTime) {
							myTSAT = myTimeToAdd;
							myTTOT = calculateTime(myTimeToAdd, myTTime);
						}
					}
				}

				if (slotList[i].hasManualCtot && slotList[i].ctot != "") {
					double diffTime = GetDifferenceTimeHHMMSS(slotList[i].tsat, slotList[i].ttot);
					int candidateTTOT = stoi(calculateTime(slotList[i].eobt, diffTime));
					int maxValue = max(candidateTTOT, stoi(slotList[i].ctot + "00"));
					myTTOT = to_string(maxValue);
					if (myTTOT.length() == 1) {
						myTTOT = "00000" + myTTOT;
					} else if (myTTOT.length() == 2) {
						myTTOT = "0000" + myTTOT;
					} else if (myTTOT.length() == 3) {
						myTTOT = "000" + myTTOT;
					} else if (myTTOT.length() == 4) {
						myTTOT = "00" + myTTOT;
					} else if (myTTOT.length() == 5) {
						myTTOT = "0" + myTTOT;
					}
				}

				Plane item = refreshTimes(slotList[i], tempSlotList, myFlightPlan, myCallsign, myEOBT, myTSAT, myTTOT, myAirport, myTTime, myDepRwy, dataRate, true);
				tempSlotList.push_back(item);
				//refreshTimes(myFlightPlan, myCallsign, myEOBT, myTSAT, myTTOT, myAirport, myTTime, myRemarks, myDepRwy, dataRate, myhasCTOT, myCtotPos, i, true);
		}
		else {
			tempSlotList.push_back(slotList[i]);
		}
	}

	addLogLine("[AUTO] - Finished CDM recalculation process");
	return tempSlotList;
	}
	catch (const std::exception& e) {
		addLogLine("ERROR: Unhandled exception backgroundProcess_recaulculate: " + (string)e.what());
	}
	catch (...) {
		addLogLine("ERROR: Unhandled exception backgroundProcess_recaulculate");
	}
	return slotList;
}

Plane CDM::refreshTimes(Plane plane, vector<Plane> planes, CFlightPlan FlightPlan, string callsign, string EOBT, string TSATfinal, string TTOTFinal, string origin, int taxiTime, string depRwy, Rate dataRate, bool aircraftFind) {
	try {
		bool equalTTOT = true;
		bool correctTTOT = true;
		bool equalTempoTTOT = true;
		bool alreadySetTOStd = false;
		bool okToLook = false;
		string timeNow = GetActualTime() + "00";
		string myFlow = "";
		EcfmpRestriction myEcfmp;
		bool hasEcfmpRestriction = false;

		
		myFlow = plane.flowReason;
		myEcfmp = plane.ecfmpRestriction;
		hasEcfmpRestriction = plane.hasEcfmpRestriction;

		//Calculate Rate
		int rate;
		if (dataRate.airport == "-1") {
			if (!lvo) {
				rate = stoi(rateString);
			}
			else {
				rate = stoi(lvoRateString);
			}
		}
		else {
			int a = 0;
			int dataRatePos = 0;
			if (dataRate.rates.size() > 1) {
				for (string dr : dataRate.depRwyYes) {
					if (dr == depRwy) {
						dataRatePos = a;
					}
					a++;
				}
			}
			if (!lvo) {
				rate = stoi(dataRate.rates[dataRatePos]);
			}
			else {
				rate = stoi(dataRate.ratesLvo[dataRatePos]);
			}
		}

		double rateHour = (double)60 / rate;

		string checkedTSAT = plane.tsat;
		if (stoi(checkedTSAT) >= stoi(timeNow)) {
			okToLook = true;
		}

		if (okToLook) {
			bool sameOrDependantRwys = false;
			string mySid = FlightPlan.GetFlightPlanData().GetSidName();

			while (equalTTOT) {
				correctTTOT = true;
				if (bmiMode) {
					TTOTFinal = getCorrectTTOT_Windowed(TTOTFinal, plane.hasManualCtot, planes, rate, plane.callsign, origin, depRwy, timeNow, taxiTime, mySid);
				}
				else {
				for (size_t t = 0; t < planes.size(); t++)
				{
					string listTTOT;
					string listCallsign = planes[t].callsign;
					string listDepRwy = "";
					CFlightPlan listFlightPlan = FlightPlanSelect(listCallsign.c_str());
					if (!listFlightPlan.IsValid()) {
						continue;
					}
					string listSid = listFlightPlan.GetFlightPlanData().GetSidName();
					bool depRwyFound = false;
					for (size_t i = 0; i < taxiTimesList.size(); i++)
					{
						if (listCallsign == taxiTimesList[i].substr(0, taxiTimesList[i].find(","))) {
							if (taxiTimesList[i].substr(taxiTimesList[i].find(",") + 3, 1) == ",") {
								listDepRwy = taxiTimesList[i].substr(taxiTimesList[i].find(",") + 1, 2);
								depRwyFound = true;
							}
							else if (taxiTimesList[i].substr(taxiTimesList[i].find(",") + 4, 1) == ",") {
								listDepRwy = taxiTimesList[i].substr(taxiTimesList[i].find(",") + 1, 3);
								depRwyFound = true;
							}
						}
					}
					string listAirport;
					for (size_t i = 0; i < planeAiportList.size(); i++)
					{
						if (listCallsign == planeAiportList[i].substr(0, planeAiportList[i].find(","))) {
							listAirport = planeAiportList[i].substr(planeAiportList[i].find(",") + 1, 4);
						}
					}

					if (!depRwyFound) {
						listDepRwy = depRwy;
					}

					sameOrDependantRwys = false;

					if (depRwy == listDepRwy) {
						sameOrDependantRwys = true;
					}

					if (dataRate.airport != "-1" && !sameOrDependantRwys) {
						for (string testRwy : dataRate.dependentRwy) {
							if (testRwy == listDepRwy) {
								sameOrDependantRwys = true;
							}
						}
					}

					if (plane.hasManualCtot) {
						bool found = false;
						while (!found) {
							found = true;
							if (planes[t].hasManualCtot) {

								listTTOT = planes[t].ttot;

								if (TTOTFinal == listTTOT && callsign != listCallsign && sameOrDependantRwys && listAirport == origin) {
									found = false;
									if (alreadySetTOStd) {
										TTOTFinal = calculateTime(TTOTFinal, 0.5);
										correctTTOT = false;
									}
									else {
										TTOTFinal = calculateTime(listTTOT, 0.5);
										correctTTOT = false;
										alreadySetTOStd = true;
									}
								}
								else if (callsign != listCallsign && sameOrDependantRwys && listAirport == origin) {
									if ((stoi(TTOTFinal) < stoi(calculateTime(listTTOT, rateHour))) && (stoi(TTOTFinal) > stoi(calculateLessTime(listTTOT, rateHour)))) {
										found = false;
										if (alreadySetTOStd) {
											TTOTFinal = calculateTime(TTOTFinal, 0.5);
											correctTTOT = false;
										}
										else {
											TTOTFinal = calculateTime(listTTOT, 0.5);
											correctTTOT = false;
											alreadySetTOStd = true;
										}
									}
								}
							}
						}
					}
					else {
						bool found = false;
						while (!found) {
							found = true;
							listTTOT = planes[t].ttot;

							if (planes[t].tsat == "999999") {
								listDepRwy = depRwy;
								listAirport = origin;
							}

							if (TTOTFinal == listTTOT && callsign != listCallsign && sameOrDependantRwys && listAirport == origin) {
								found = false;
								if (alreadySetTOStd) {
									TTOTFinal = calculateTime(TTOTFinal, 0.5);
									correctTTOT = false;
								}
								else {
									TTOTFinal = calculateTime(listTTOT, 0.5);
									correctTTOT = false;
									alreadySetTOStd = true;
								}
							}
							else if (callsign != listCallsign && sameOrDependantRwys && listAirport == origin) {
								if ((stoi(TTOTFinal) < stoi(calculateTime(listTTOT, rateHour))) && (stoi(TTOTFinal) > stoi(calculateLessTime(listTTOT, rateHour)))) {
									found = false;
									if (alreadySetTOStd) {
										TTOTFinal = calculateTime(TTOTFinal, 0.5);
										correctTTOT = false;
									}
									else {
										TTOTFinal = calculateTime(listTTOT, 0.5);
										correctTTOT = false;
										alreadySetTOStd = true;
									}
								}
							}

							if (correctTTOT) {
								string calculatedTSATNow = calculateLessTime(TTOTFinal, taxiTime);
								if (calculatedTSATNow.substr(0, 2) == "00") {
									calculatedTSATNow = "24" + calculatedTSATNow.substr(2, 4);
								}
								if (stoi(calculatedTSATNow) < stoi(timeNow)) {
									TTOTFinal = calculateTime(TTOTFinal, 0.5);
									correctTTOT = false;
								}
							}
						}
					}
					//Check SID Interval
					if (correctTTOT && sidIntervalEnabled && callsign != listCallsign) {
						listTTOT = planes[t].ttot;
						double interval = getSidInterval(mySid, listSid, origin, depRwy);
						if (interval > 0) {
							bool found = false;
							while (!found) {
								found = true;
								if ((stoi(TTOTFinal) < stoi(calculateTime(listTTOT, interval))) && (stoi(TTOTFinal) > stoi(calculateLessTime(listTTOT, interval)))) {
									found = false;
									if (alreadySetTOStd) {
										TTOTFinal = calculateTime(TTOTFinal, 0.5);
										correctTTOT = false;
									}
									else {
										TTOTFinal = calculateTime(listTTOT, 0.5);
										correctTTOT = false;
										alreadySetTOStd = true;
									}
								}
							}
						}
					}
				}
				}

				if (correctTTOT) {
					bool correctFlowTTOT = true;
					bool correctCAD = true;
					vector<Plane> sameDestList;
					sameDestList.clear();

					//Check flow measures if exists
					if (hasEcfmpRestriction) {
						sameDestList.clear();
						int seperationFlow = myEcfmp.value;
						for (size_t z = 0; z < planes.size(); z++)
						{
							CFlightPlan fpSelected = FlightPlanSelect(planes[z].callsign.c_str());
							if (!fpSelected.IsValid()) {
								continue;
							}
							string destFound = fpSelected.GetFlightPlanData().GetDestination();
							string routeFound = fpSelected.GetFlightPlanData().GetRoute();
							bool validToAdd = false;
							for (string apt : myEcfmp.ADES) {
								if (apt.find(destFound) != string::npos) {
									validToAdd = true;
								}
								else if (apt.substr(2, 2) == "**") {
									if (destFound.substr(0, 2) == apt.substr(0, 2)) {
										validToAdd = true;
									}
									else if (apt.substr(0, 2) == "**") {
										validToAdd = true;
									}
								}
							}
							if (validToAdd) {
								validToAdd = false;
								if (myEcfmp.waypoints.empty()) {
									validToAdd = true;
								}
								for (string wpt : myEcfmp.waypoints) {
									if (routeFound.find(wpt) != string::npos) {
										validToAdd = true;
									}
								}
								if (validToAdd) {
									sameDestList.push_back(planes[z]);
								}
							}
						}

						for (size_t z = 0; z < sameDestList.size(); z++)
						{
							CFlightPlan fpList = FlightPlanSelect(planes[z].callsign.c_str());
							if (!fpList.IsValid()) {
								continue;
							}
							bool found = false;
							string listTTOT = sameDestList[z].ttot;
							string listCallsign = sameDestList[z].callsign;
							string listDepRwy = fpList.GetFlightPlanData().GetDepartureRwy();
							string listAirport = fpList.GetFlightPlanData().GetOrigin();
							while (!found) {
								found = true;
								if (TTOTFinal == listTTOT && callsign != listCallsign && sameOrDependantRwys && listAirport == origin) {
									found = false;
									TTOTFinal = calculateTime(TTOTFinal, 1);
									correctFlowTTOT = false;
								}
								else if ((stoi(TTOTFinal) < stoi(calculateTime(listTTOT, seperationFlow))) && (stoi(TTOTFinal) > stoi(calculateLessTime(listTTOT, seperationFlow))) && callsign != listCallsign && sameOrDependantRwys && listAirport == origin) {
									found = false;
									TTOTFinal = calculateTime(TTOTFinal, 1);
									correctFlowTTOT = false;
								}
							}
						}
					}
					if (correctFlowTTOT) {
						bool doRequest = false;
						equalTTOT = false;
						TSATfinal = calculateLessTime(TTOTFinal, taxiTime);
						/* START Check stand de-ice */
						bool standDeice = false;
						for (vector<string> deice : deiceList) {
							if (deice[0] == callsign) {
								if (deice[1] == "STND") {
									standDeice = true;
								}
							}
						}
						if (standDeice) {
							int deIceTime = getDeIceTime(FlightPlan.GetFlightPlanData().GetAircraftWtc(), 0);
							TSATfinal = calculateTime(TSATfinal, deIceTime);
						}
						/* END Check stand de-ice */
						string TSAT = TSATfinal.c_str();
						string TTOT = TTOTFinal.c_str();
						if (plane.hasManualCtot) {
							if (aircraftFind) {
								if (TTOT != plane.ttot && TTOT.length() >= 4) {
									Plane p(callsign, EOBT, TSAT, TTOT, plane.ctot, myFlow, myEcfmp, hasEcfmpRestriction, plane.hasManualCtot, true, true);
									plane = p;
									doRequest = true;
									setFlightStripInfo(FlightPlan, p.tsat, 3);
									setFlightStripInfo(FlightPlan, p.ttot, 4);
								}
							}
							else if (TTOT.length() >= 4) {
								Plane p(callsign, EOBT, TSAT, TTOT, plane.ctot, myFlow, myEcfmp, hasEcfmpRestriction, plane.hasManualCtot, true, true);
								plane = p;
								setFlightStripInfo(FlightPlan, p.tsat, 3);
								setFlightStripInfo(FlightPlan, p.ttot, 4);
							}
						}
						else {
							if (aircraftFind) {
								if (TTOT != plane.ttot && TTOT.length() >= 4) {
									Plane p(callsign, EOBT, TSAT, TTOT, plane.ctot, myFlow, myEcfmp, hasEcfmpRestriction, plane.hasManualCtot, true, true);
									plane = p;
									doRequest = true;
									setFlightStripInfo(FlightPlan, p.tsat, 3);
									setFlightStripInfo(FlightPlan, p.ttot, 4);
								}
							}
							else if (TTOT.length() >= 4) {
								Plane p(callsign, EOBT, TSAT, TTOT, plane.ctot, myFlow, myEcfmp, hasEcfmpRestriction, plane.hasManualCtot, true, true);
								plane = p;
								setFlightStripInfo(FlightPlan, p.tsat, 3);
								setFlightStripInfo(FlightPlan, p.ttot, 4);
							}
						}
						//Check API
						if (doRequest && TSATfinal.length() >= 4) {
							if (serverEnabled) {
								string myTSATApi = TSAT;
								if (plane.hasManualCtot && plane.ctot != "" && plane.ttot.length() >= 4) {
									string myTTOT = TTOT;
									myTTOT = myTTOT.substr(0, 4);
									if (stoi(myTTOT) > stoi(plane.ctot) && stoi(myTTOT + "00") <= stoi(calculateTime(plane.ctot + "00", 7))) {
										//Update TOBT API with TSAT if TTOT is greater than CTOT but less or equal to CTOT+7
										string myCOBT = calculateLessTime(plane.ctot + "00", taxiTime);
										setOBTApi(callsign, myCOBT, false, false);
									}
									else {
										setOBTApi(callsign, myTSATApi, false, false);
									}

								}
								else {
									setOBTApi(callsign, myTSATApi, false, false);
								}
							}
						}
					}
				}
			}
		}
		return plane;
	}
	catch (const std::exception& e) {
		addLogLine("ERROR: Unhandled exception refreshTimes: " + (string)e.what());
		return plane;
	}
	catch (...) {
		addLogLine("ERROR: Unhandled exception refreshTimes");
		return plane;
	}
}

string CDM::getCorrectTTOT_Windowed(
	string TTOTInitial,
	bool hasManualCtot,
	const vector<Plane>& planes,
	int rateHour,
	const string& callsign,
	const string& origin,
	const string& depRwy,
	const string& timeNow,
	double taxiTime,
	const string& mySid
) {
	string TTOTFinal = TTOTInitial;
	bool correctTTOT = true;
	bool alreadySetTOStd = false;

	const int windowMinutes = 10;
	const int windowsPerHour = 60 / windowMinutes;

	// Instead of ceil -> distribute remainder across windows
	const int baseCap = rateHour / windowsPerHour;      // e.g. 40/6 = 6
	const int remainder = rateHour % windowsPerHour;    // e.g. 40%6 = 4  -> 4 windows get +1

	auto getWindowStartFromTTOT = [&](const string& ttot) -> string {
		int v = stoi(ttot);
		int hh = v / 10000;
		int mm = (v / 100) % 100;

		int mmStart = (mm / windowMinutes) * windowMinutes;
		int startVal = (hh * 10000) + (mmStart * 100);

		string s = to_string(startVal);
		while ((int)s.size() < 6) s = "0" + s;
		return s;
		};

	auto capForWindowStart = [&](const string& windowStartTTOT) -> int {
		int v = stoi(windowStartTTOT);
		int mmStart = (v / 100) % 100;
		int windowIndex = mmStart / windowMinutes;
		return baseCap + ((windowIndex < remainder) ? 1 : 0);
		};

	auto inSameWindow = [&](const string& a, const string& b) -> bool {
		return getWindowStartFromTTOT(a) == getWindowStartFromTTOT(b);
		};

	auto bumpToNextWindowStart = [&](const string& ttot) -> string {
		string winStart = getWindowStartFromTTOT(ttot);
		string next = calculateTime(winStart, 10.0);
		return getWindowStartFromTTOT(next);
		};

	auto countInWindow = [&](const string& windowTTOT, bool manualMode) -> int {
		int count = 0;
		for (int t = 0; t < (int)planes.size(); t++) {
			if (planes[t].callsign == callsign) continue;
			if (manualMode && !planes[t].hasManualCtot) continue;
			if (!manualMode && planes[t].hasManualCtot) continue;

			string listCallsign = planes[t].callsign;
			string listDepRwy = "";

			CFlightPlan listFlightPlan = FlightPlanSelect(listCallsign.c_str());
			if (!listFlightPlan.IsValid()) continue;

			string listSid = listFlightPlan.GetFlightPlanData().GetSidName();

			bool depRwyFound = false;
			for (size_t i = 0; i < taxiTimesList.size(); i++) {
				if (listCallsign == taxiTimesList[i].substr(0, taxiTimesList[i].find(","))) {
					if (taxiTimesList[i].substr(taxiTimesList[i].find(",") + 3, 1) == ",") {
						listDepRwy = taxiTimesList[i].substr(taxiTimesList[i].find(",") + 1, 2);
						depRwyFound = true;
					}
					else if (taxiTimesList[i].substr(taxiTimesList[i].find(",") + 4, 1) == ",") {
						listDepRwy = taxiTimesList[i].substr(taxiTimesList[i].find(",") + 1, 3);
						depRwyFound = true;
					}
				}
			}

			string listAirport;
			for (size_t i = 0; i < planeAiportList.size(); i++) {
				if (listCallsign == planeAiportList[i].substr(0, planeAiportList[i].find(","))) {
					listAirport = planeAiportList[i].substr(planeAiportList[i].find(",") + 1, 4);
				}
			}

			if (!depRwyFound) listDepRwy = depRwy;

			bool sameOrDependantRwys = (depRwy == listDepRwy);

			if (!(listAirport == origin)) continue;
			if (!sameOrDependantRwys) continue;

			if (inSameWindow(windowTTOT, planes[t].ttot)) {
				count++;
			}
		}
		return count;
		};

	bool found = false;
	while (!found) {
		found = true;

		string currentWindowStart = getWindowStartFromTTOT(TTOTFinal);

		int used = countInWindow(currentWindowStart, hasManualCtot);
		int capThisWindow = capForWindowStart(currentWindowStart);

		if (used >= capThisWindow) {
			found = false;

			TTOTFinal = bumpToNextWindowStart(TTOTFinal);

			correctTTOT = false;
			alreadySetTOStd = true;
		}

		if (found && correctTTOT) {
			string calculatedTSATNow = calculateLessTime(TTOTFinal, taxiTime);
			if (calculatedTSATNow.substr(0, 2) == "00") {
				calculatedTSATNow = "24" + calculatedTSATNow.substr(2, 4);
			}
			if (stoi(calculatedTSATNow) < stoi(timeNow)) {
				found = false;

				TTOTFinal = bumpToNextWindowStart(TTOTFinal);

				correctTTOT = false;
				alreadySetTOStd = true;
			}
		}

		if (found && sidIntervalEnabled) {
			for (int t = 0; t < (int)planes.size(); t++) {
				if (planes[t].callsign == callsign) continue;
				string listAirport;
				for (size_t i = 0; i < planeAiportList.size(); i++) {
					if (planes[t].callsign == planeAiportList[i].substr(0, planeAiportList[i].find(","))) {
						listAirport = planeAiportList[i].substr(planeAiportList[i].find(",") + 1, 4);
					}
				}
				if (listAirport != origin) continue;
				string listCallsign = planes[t].callsign;
				string listTTOT = planes[t].ttot;
				CFlightPlan listFlightPlan = FlightPlanSelect(listCallsign.c_str());
				if (!listFlightPlan.IsValid()) continue;
				string listSid = listFlightPlan.GetFlightPlanData().GetSidName();
				double interval = getSidInterval(mySid, listSid, origin, depRwy);
				if (interval <= 0) continue;
				int ttotFinalInt = stoi(TTOTFinal);
				int listTTOTInt = stoi(listTTOT);
				int requiredTTOT = stoi(calculateTime(listTTOT, interval));
				if (ttotFinalInt < requiredTTOT) {
					found = false;
					TTOTFinal = calculateTime(listTTOT, interval);
					correctTTOT = false;
					alreadySetTOStd = true;
					break;
				}
			}
		}
	}

	while ((int)TTOTFinal.size() < 6) TTOTFinal = "0" + TTOTFinal;
	if ((int)TTOTFinal.size() > 6) TTOTFinal = TTOTFinal.substr(0, 6);

	return TTOTFinal;
}

/*
* Mehod to push FlightStrip Data to other controllers (old Amend)
*/
void CDM::PushToOtherControllers(CFlightPlan fp) {
	string callsign = "";
	for (CController c = ControllerSelectFirst(); c.IsValid(); c = ControllerSelectNext(c)) {
		if (c.IsController()) {
			callsign = c.GetCallsign();
			if (callsign.size() > 3) {
				if (callsign.find("_DEL") != string::npos || callsign.find("_GND") != string::npos || callsign.find("_TWR") != string::npos || callsign.find("_APP") != string::npos) {
					fp.PushFlightStrip(c.GetCallsign());
				}
			}
		}
		else if (callsign.find("OBS") != string::npos) {
			fp.PushFlightStrip(c.GetCallsign());
		}
	}
}

string CDM::GetActualTime()
{
	//Get Time now
	time_t rawtime;
	struct tm ptm;
	time(&rawtime);
	gmtime_s(&ptm, &rawtime);
	string hour = to_string(ptm.tm_hour % 24);
	string min = to_string(ptm.tm_min);

	if (stoi(min) < 10) {
		min = "0" + min;
	}
	if (stoi(hour) < 10) {
		hour = "0" + hour.substr(0, 1);
	}
	return hour + min;
}

string CDM::GetDateMonthNow() {
	//Get Time now
	time_t rawtime;
	struct tm ptm;
	time(&rawtime);
	gmtime_s(&ptm, &rawtime);
	string day = to_string(ptm.tm_mday);
	string month = to_string(ptm.tm_mon + 1);
	return day + "-" + month;
}

string CDM::EobtPlusTime(string EOBT, int addedTime) {
	time_t rawtime;
	struct tm ptm;
	time(&rawtime);
	gmtime_s(&ptm, &rawtime);
	string hour = to_string(ptm.tm_hour % 24);
	string min = to_string(ptm.tm_min);

	if (stoi(min) < 10) {
		min = "0" + min;
	}
	if (stoi(hour) < 10) {
		hour = "0" + hour.substr(0, 1);
	}

	return calculateTime(hour + min + "00", addedTime);
}

void CDM::deleteFlightStrips(string callsign)
{
	addLogLine("Called deleteFlightStrips...");
	try {
		CFlightPlan fp = FlightPlanSelect(callsign.c_str());
		if (fp.IsValid()) {
			setFlightStripInfo(fp, "", 0);
			setFlightStripInfo(fp, "", 1);
			setFlightStripInfo(fp, "", 2);
			setFlightStripInfo(fp, "", 3);
			setFlightStripInfo(fp, "", 4);
		}
	}
	catch (const std::exception& e) {
		addLogLine("ERROR: Unhandled exception deleteFlightStrips: " + (string)e.what());
	}
	catch (...) {
		addLogLine("ERROR: Unhandled exception deleteFlightStrips");
	}
}

vector<string> split(const std::string& s, char delimiter) {
	std::vector<std::string> tokens;
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delimiter)) {
		tokens.push_back(item);
	}
	return tokens;
}

string CDM::getTaxiTime(double lat, double lon, string origin, string depRwy, int deIceTime, string callsign) {
	string line, TxtOrigin, TxtDepRwy, TxtTime;
	CPosition p1, p2, p3, p4;
	smatch match;

	try
	{
		for (size_t t = 0; t < TxtTimesVector.size(); t++)
		{
			line = TxtTimesVector[t];
			if (
				regex_match(TxtTimesVector[t], match,
					regex("([A-Z]{4}):(\\d{2}[LRC]?):([^:]+):([^:]+):([^:]+):([^:]+):([^:]+):([^:]+):([^:]+):([^:]+):(\\d+)$",
						regex::icase)) ||

				regex_match(TxtTimesVector[t], match,
					regex("([A-Z]{4}):(\\d{2}[LRC]?):([^:]+):([^:]+):([^:]+):([^:]+):([^:]+):([^:]+):([^:]+):([^:]+):(\\d+):([^:]+)$",
						regex::icase)) ||

				regex_match(TxtTimesVector[t], match,
					regex("([A-Z]{4}):(\\d{2}[LRC]?):([^:]+):([^:]+):([^:]+):([^:]+):([^:]+):([^:]+):([^:]+):([^:]+):(\\d+):([^:]+):(\\d+)$",
						regex::icase))
				)
			{
				if (origin != match[1])
					continue;

				if (depRwy != match[2])
					continue;

				p1 = readPosition(match[3], match[4]);
				p2 = readPosition(match[5], match[6]);
				p3 = readPosition(match[7], match[8]);
				p4 = readPosition(match[9], match[10]);

				double LatArea[] = { p1.m_Latitude,p2.m_Latitude,p3.m_Latitude,p4.m_Latitude };
				double LonArea[] = { p1.m_Longitude,p2.m_Longitude,p3.m_Longitude,p4.m_Longitude };

				int remId = getDeIceId(callsign);

				vector<string> times;
				if (match.size() > 12 && match[12].matched && match[12].length() > 0) {
					times = splitString(match[12], ',');
				}

				int eventAdd = 0;
				if (eventMode) {
					if (match.size() > 13 && match[13].matched && match[13].length() > 0) {
						if (isNumber(match[13])) {
							eventAdd = stoi(match[13]);
						}
						else {
							addLogLine("ERROR: Non-numeric EVENT_TAXI in line: " + line);
							eventAdd = eventModeTime;
						}
					}
					else {
						eventAdd = eventModeTime;
					}
				}

				if (inPoly(4, LatArea, LonArea, lat, lon) % 2 != 0) {
					if (remId > 0 && times.size() >= (size_t)remId) {
						if (isNumber(times[remId - 1])) {
							return to_string(deIceTime + stoi(times[remId - 1]) + eventAdd);
						}
						else {
							addLogLine("ERROR: Non-numeric REM time in line: " + line);
						}
					}
					return to_string((isNumber(match[11]) ? stoi(match[11]) : defTaxiTime) + deIceTime + eventAdd);
				}
			}
		}
	}
	catch (std::runtime_error const& e)
	{
		DisplayUserMessage(MY_PLUGIN_NAME, "Error", e.what(), true, true, false, true, false);
		DisplayUserMessage(MY_PLUGIN_NAME, "Error", line.c_str(), true, true, false, true, false);
		addLogLine("ERROR: Unhandled exception getTaxiTime: " + (string)e.what());
	}
	catch (...)
	{
		DisplayUserMessage(MY_PLUGIN_NAME, "Error", std::to_string(GetLastError()).c_str(), true, true, false, true, false);
		DisplayUserMessage(MY_PLUGIN_NAME, "Error", line.c_str(), true, true, false, true, false);
		addLogLine("ERROR: Unhandled exception getTaxiTime");
	}

	return to_string(defTaxiTime);
}

int CDM::inPoly(int nvert, double* vertx, double* verty, double testx, double testy)
{
	int i, j, c = 0;
	for (i = 0, j = nvert - 1; i < nvert; j = i++) {
		if (((verty[i] > testy) != (verty[j] > testy)) &&
			(testx < (vertx[j] - vertx[i]) * (testy - verty[i]) / (verty[j] - verty[i]) + vertx[i]))
			c = !c;
	}
	return c;
}


string CDM::formatTime(string timeString) {
	try {
		if (timeString.length() <= 0) {
			timeString = "0000" + timeString;
			return timeString;
		}
		else if (timeString.length() <= 1) {
			timeString = "000" + timeString;
			return timeString;
		}
		else if (timeString.length() <= 2) {
			timeString = "00" + timeString;
			return timeString;
		}
		else if (timeString.length() <= 3) {
			timeString = "0" + timeString;
			return timeString;
		}
		else if (timeString.length() == 4) {
			return timeString;
		}
		else if (timeString.length() >= 5) {
			timeString = timeString.substr(0, 4);
			return timeString;
		}
		else {
			return timeString;
		}
	}
	catch (const std::exception& e) {
		addLogLine("ERROR: Unhandled exception formatTime: " + (string)e.what());
		return timeString;
	}
	catch (...) {
		addLogLine("ERROR: Unhandled exception formatTime");
		return timeString;
	}
}


string CDM::calculateTime(string timeString, double minsToAdd) {
	try {
		if (timeString.length() < 4 || timeString.length() == 5) {
			timeString = "000000";
		}
		else if (timeString.length() == 4) {
			timeString = timeString + "00";
		}
		int hours = stoi(timeString.substr(0, 2));
		int mins = stoi(timeString.substr(2, 2));
		int sec = stoi(timeString.substr(4, 2));

		int movTime = minsToAdd * 60;
		while (movTime > 0) {
			sec += 1;
			if (sec > 59) {
				sec = 0;
				mins += 1;
				if (mins > 59) {
					mins = 0;
					hours += 1;
					if (hours > 23) {
						hours = 0;
					}
				}
			}
			movTime -= 1;
		};

		//calculate hours
		string hourFinal;
		if (hours < 10) {
			hourFinal = "0" + to_string(hours);
		}
		else {
			hourFinal = to_string(hours);
		}
		//calculate mins
		string minsFinal;
		if (mins < 10) {
			minsFinal = "0" + to_string(mins);
		}
		else {
			minsFinal = to_string(mins);
		}
		//calculate sec
		string secFinal;
		if (sec < 10) {
			secFinal = "0" + to_string(sec);
		}
		else {
			secFinal = to_string(sec);
		}
		string timeFinal = hourFinal + minsFinal + secFinal;

		return timeFinal;
	}
	catch (const std::exception& e) {
		addLogLine("ERROR: Unhandled exception calculateTime: " + (string)e.what());
		return timeString;
	}
	catch (...) {
		addLogLine("ERROR: Unhandled exception calculateTime");
		return timeString;
	}
}

void CDM::toggleReaMsg(CFlightPlan fp, bool deleteIfExist)
{
	bool inreaList = false;
	int i = 0;
	for (string s : reaCTOTSent) {
		if (s == fp.GetCallsign()) {
			inreaList = true;
			if (deleteIfExist) {
				reaCTOTSent.erase(reaCTOTSent.begin() + i);
			}
		}
		i++;
	}
	if (!inreaList) {
		reaCTOTSent.push_back(fp.GetCallsign());
	}

	//Update times to slaves
	countTime = std::time(nullptr) - (refreshTime);
}

void CDM::addTimeToList(int timeToAdd, string minTSAT) {
	try {
		vector<Plane> mySlotList = slotList;

		for (size_t i = 0; i < mySlotList.size(); i++) {
			if (!mySlotList[i].hasManualCtot) {
				CFlightPlan myFp = FlightPlanSelect(mySlotList[i].callsign.c_str());
				if (!myFp.IsValid()) {
					continue;
				}
				if ((string)myFp.GetGroundState() != "STUP" && (string)myFp.GetGroundState() != "ST-UP" && (string)myFp.GetGroundState() != "PUSH" && (string)myFp.GetGroundState() != "TAXI" && (string)myFp.GetGroundState() != "DEPA") {
					int difTime = GetdifferenceTime(mySlotList[i].tsat.substr(0, 2), mySlotList[i].tsat.substr(2, 2), minTSAT.substr(0, 2), minTSAT.substr(2, 2));
					bool ok = false;
					if (minTSAT.substr(0, 2) == mySlotList[i].tsat.substr(0, 2)) {
						if (difTime >= 0) {
							ok = true;
						}
					}
					else {
						if (difTime >= 40) {
							ok = true;
						}
					}
					if (ok) {
						mySlotList[i].tsat = calculateTime(mySlotList[i].tsat, timeToAdd);
						mySlotList[i].ttot = calculateTime(mySlotList[i].ttot, timeToAdd);
					}
				}
			}
		}

		for (Plane p : mySlotList) {
			for (int d = 0; d < slotList.size(); d++) {
				if (p.callsign == slotList[d].callsign) {
					slotList[d] = p;
				}
			}
		}
	}
	catch (const std::exception& e) {
		addLogLine("ERROR: Unhandled exception addTimeToList: " + (string)e.what());
	}
	catch (...) {
		addLogLine("ERROR: Unhandled exception addTimeToList");
	}
}

void CDM::addTimeToListForSpecificAirportAndRunway(int timeToAdd, string minTSAT, string airport, string runway) {
	try{
		vector<Plane> mySlotList = slotList;

		for (size_t i = 0; i < mySlotList.size(); i++) {
			if (!mySlotList[i].hasManualCtot) {
				CFlightPlan myFp = FlightPlanSelect(mySlotList[i].callsign.c_str());
				if (!myFp.IsValid()) {
					continue;
				}
				if (myFp.GetFlightPlanData().GetDepartureRwy() == runway && myFp.GetFlightPlanData().GetOrigin() == airport) {
					if ((string)myFp.GetGroundState() != "STUP" && (string)myFp.GetGroundState() != "ST-UP" && (string)myFp.GetGroundState() != "PUSH" && (string)myFp.GetGroundState() != "TAXI" && (string)myFp.GetGroundState() != "DEPA") {
						int difTime = GetdifferenceTime(mySlotList[i].tsat.substr(0, 2), mySlotList[i].tsat.substr(2, 2), minTSAT.substr(0, 2), minTSAT.substr(2, 2));
						bool ok = false;
						if (minTSAT.substr(0, 2) == mySlotList[i].tsat.substr(0, 2)) {
							if (difTime >= 0) {
								ok = true;
							}
						}
						else {
							if (difTime >= 40) {
								ok = true;
							}
						}
						if (ok) {
							mySlotList[i].tsat = calculateTime(mySlotList[i].tsat, timeToAdd);
							mySlotList[i].ttot = calculateTime(mySlotList[i].ttot, timeToAdd);
						}
					}
				}
			}
		}

		for (Plane p : mySlotList) {
			for (int d = 0; d < slotList.size(); d++) {
				if (p.callsign == slotList[d].callsign) {
					slotList[d] = p;
				}
			}
		}
	}
	catch (const std::exception& e) {
		addLogLine("ERROR: Unhandled exception addTimeToListForSpecificAirportAndRunway: " + (string)e.what());
	}
	catch (...) {
		addLogLine("ERROR: Unhandled exception addTimeToListForSpecificAirportAndRunway");
	}
}

std::vector<Plane> CDM::recalculateSlotList(std::vector<Plane> mySlotList)
{
	addLogLine("Called recalculateSlotList...");

	try {
		std::sort(mySlotList.begin(), mySlotList.end(),
			[](const Plane& a, const Plane& b)
			{
				// 1. Manual CTOT first
				if (a.hasManualCtot != b.hasManualCtot)
					return a.hasManualCtot > b.hasManualCtot;

				// 2. Both manual CTOT AND both CTOTs present → order by CTOT
				if (a.hasManualCtot && b.hasManualCtot &&
					!a.ctot.empty() && !b.ctot.empty())
				{
					return std::stoi(a.ctot) < std::stoi(b.ctot);
				}

				// 3. Fallback → order by TTOT
				return std::stoi(a.ttot) < std::stoi(b.ttot);
			});
	}
	catch (const std::exception& e) {
		addLogLine("ERROR: Unhandled exception recalculateSlotList: " + std::string(e.what()));
	}
	catch (...) {
		addLogLine("ERROR: Unhandled exception recalculateSlotList");
	}

	return cleanUpSlotListVector(mySlotList);
}

vector<Plane> CDM::cleanUpSlotListVector(vector<Plane> mySlotList) {
	vector<Plane> finalSlotList;
	string lastCallsign = "";
	for (Plane p : mySlotList) {
		if (lastCallsign != p.callsign) {
			finalSlotList.push_back(p);
			lastCallsign = p.callsign;
		}
	}

	return finalSlotList;
}

string CDM::calculateLessTime(string timeString, double minsToAdd) {
	try{
	if (timeString.length() < 4 || timeString.length() == 5) {
		timeString = "000000";
	}
	else if (timeString.length() == 4) {
		timeString = timeString + "00";
	}
	int hours = stoi(timeString.substr(0, 2));
	int mins = stoi(timeString.substr(2, 2));
	int sec = stoi(timeString.substr(4, 2));

	int movTime = minsToAdd * 60;
	while (movTime > 0) {
		sec -= 1;
		if (sec < 0) {
			sec = 59;
			mins -= 1;
			if (mins < 0) {
				mins = 59;
				hours -= 1;
				if (hours < 0) {
					hours = 23;
				}
			}
		}
		movTime -= 1;
	};

	//calculate hours
	string hourFinal;
	if (hours < 10) {
		hourFinal = "0" + to_string(hours);
	}
	else {
		hourFinal = to_string(hours);
	}
	//calculate mins
	string minsFinal;
	if (mins < 10) {
		minsFinal = "0" + to_string(mins);
	}
	else {
		minsFinal = to_string(mins);
	}
	//calculate sec
	string secFinal;
	if (sec < 10) {
		secFinal = "0" + to_string(sec);
	}
	else {
		secFinal = to_string(sec);
	}
	string timeFinal = hourFinal + minsFinal + secFinal;

	return timeFinal;
	}
	catch (const std::exception& e) {
		addLogLine("ERROR: Unhandled exception calculateLessTime: " + (string)e.what());
		return timeString;
	}
	catch (...) {
		addLogLine("ERROR: Unhandled exception calculateLessTime");
		return timeString;
	}
}

bool CDM::checkIsNumber(string str) {
	bool hasNoNumber = true;
	for (size_t i = 0; i < str.length(); i++) {
		if (isdigit(str[i]) == false) {
			hasNoNumber = false;
		}
	}
	if (hasNoNumber) {
		return true;
	}
	else {
		return false;
	}
}

void CDM::setDeice(string remText, CFlightPlan fp, string index) {
	addLogLine("TRIGGER - TAG_FUNC_DEICE_" +  remText);

	bool found = false;
	for (size_t i = 0; i < deiceList.size(); i++) {
		if (deiceList[i][0] == fp.GetCallsign()) {
			found = true;
		}
	}

	if (found) {
		//Remove plane from deice list
		for (size_t i = 0; i < deiceList.size(); i++) {
			if (deiceList[i][0] == fp.GetCallsign()) {
				deiceList.erase(deiceList.begin() + i);
			}
		}
	}

	deiceList.push_back({ fp.GetCallsign(), remText, index });
	setFlightStripInfo(fp, remText, 5);

	//Remove plane from taxiTimesList
	for (size_t j = 0; j < taxiTimesList.size(); j++)
	{
		if (taxiTimesList[j].substr(0, taxiTimesList[j].find(",")) == fp.GetCallsign()) {
			taxiTimesList.erase(taxiTimesList.begin() + j);
		}
	}

	//Remove plane from slotlist to recalculate times
	for (size_t i = 0; i < slotList.size(); i++)
	{
		if (slotList[i].callsign == fp.GetCallsign()) {
			slotList.erase(slotList.begin() + i);
		}
	}
}

void CDM::disconnectTfcs() {
	addLogLine("Called disconnectTfcs...");
	try{
		for (string callsign : disconnectionList)
		{
			RemoveDataFromTfc(callsign);
		}
		disconnectionList.clear();
	}
	catch (const std::exception& e) {
		addLogLine("ERROR: Unhandled exception disconnectTfcs: " + (string)e.what());
	}
	catch (...) {
		addLogLine("ERROR: Unhandled exception disconnectTfcs");
	}
}

void CDM::RemoveDataFromTfc(string callsign) {
	addLogLine("Called RemoveDataFromTfc...");
	try{
	//Delete from vector
	for (size_t i = 0; i < slotList.size(); i++) {
		if (callsign == slotList[i].callsign) {
			if (debugMode) {
				sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 1");
			}
			slotList.erase(slotList.begin() + i);
		}
	}
	//Delete from reaSent list
	for (size_t i = 0; i < reaSent.size(); i++) {
		if (callsign == reaSent[i]) {
			if (debugMode) {
				sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 2");
			}
			reaSent.erase(reaSent.begin() + i);
		}
	}
	//Delete from reaCTOTSent list
	for (size_t i = 0; i < reaCTOTSent.size(); i++) {
		if (callsign == reaCTOTSent[i]) {
			if (debugMode) {
				sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 3");
			}
			reaCTOTSent.erase(reaCTOTSent.begin() + i);
		}
	}
	//Remove Plane From airport List
	for (size_t j = 0; j < planeAiportList.size(); j++)
	{
		if (planeAiportList[j].substr(0, planeAiportList[j].find(",")) == callsign) {
			if (debugMode) {
				sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 5");
			}
			planeAiportList.erase(planeAiportList.begin() + j);
		}
	}

	//Remove Plane From finalTimesList
	for (size_t i = 0; i < finalTimesList.size(); i++) {
		if (finalTimesList[i] == callsign) {
			if (debugMode) {
				sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 6");
			}
			finalTimesList.erase(finalTimesList.begin() + i);
		}
	}

	//Remove Taxi Times List
	for (size_t j = 0; j < taxiTimesList.size(); j++)
	{
		if (taxiTimesList[j].substr(0, taxiTimesList[j].find(",")) == callsign) {
			if (debugMode) {
				sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 7");
			}
			taxiTimesList.erase(taxiTimesList.begin() + j);
		}
	}

	//Remove ctotCheck
	for (size_t i = 0; i < CTOTcheck.size(); i++)
	{
		if (CTOTcheck[i] == callsign) {
			if (debugMode) {
				sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 8");
			}
			CTOTcheck.erase(CTOTcheck.begin() + i);
		}
	}

	//Remove ASAT
	for (size_t x = 0; x < asatList.size(); x++)
	{
		string actualListCallsign = asatList[x].substr(0, asatList[x].find(","));
		if (actualListCallsign == callsign) {
			if (debugMode) {
				sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 9");
			}
			asatList.erase(asatList.begin() + x);
		}
	}
	//Remove from OutOfTsat
	for (size_t i = 0; i < OutOfTsat.size(); i++)
	{
		if (callsign == OutOfTsat[i][0]) {
			if (debugMode) {
				sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 10");
			}
			OutOfTsat.erase(OutOfTsat.begin() + i);
		}
	}
	//Remove from setOBTlater
	{
		std::lock_guard<std::mutex> lock(later1Mutex);
		for (size_t i = 0; i < setOBTlater.size(); i++)
		{
			if (callsign == setOBTlater[i].callsign) {
				if (debugMode) {
					sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 11");
				}
				setOBTlater.erase(setOBTlater.begin() + i);
			}
		}
	}
	//Remove from suWaitList
	for (size_t i = 0; i < suWaitList.size(); i++)
	{
		if (callsign == suWaitList[i]) {
			if (debugMode) {
				sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 12");
			}
			suWaitList.erase(suWaitList.begin() + i);
		}
	}
	//Remove from setCdmStslater
	{
		std::lock_guard<std::mutex> lock(later2Mutex);
		for (size_t i = 0; i < setCdmStslater.size(); i++)
		{
			if (callsign == setCdmStslater[i][0]) {
				if (debugMode) {
					sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 13");
				}
				setCdmStslater.erase(setCdmStslater.begin() + i);
			}
		}
	}
	//Remove from setCdmDatalater
	{
		std::lock_guard<std::mutex> lock(later4Mutex);
		for (size_t i = 0; i < setCdmDatalater.size(); i++)
		{
			if (callsign == setCdmDatalater[i].callsign) {
				if (debugMode) {
					sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 14");
				}
				setCdmDatalater.erase(setCdmDatalater.begin() + i);
			}
		}
	}
	//Remove from disabledCtots
	for (size_t i = 0; i < disabledCtots.size(); i++)
	{
		if (callsign == disabledCtots[i]) {
			if (debugMode) {
				sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 15");
			}
			disabledCtots.erase(disabledCtots.begin() + i);
		}
	}
	//Remove from checkCIDLater
	{
		std::lock_guard<std::mutex> lock(later3Mutex);
		for (size_t i = 0; i < checkCIDLater.size(); i++)
		{
			if (callsign == checkCIDLater[i]) {
				if (debugMode) {
					sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 16");
				}
				checkCIDLater.erase(checkCIDLater.begin() + i);
			}
		}
	}
	
	//Remove from deiceList
	for (size_t i = 0; i < deiceList.size(); i++)
	{
		if (callsign == deiceList[i][0]) {
			if (debugMode) {
				sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 16");
			}
			deiceList.erase(deiceList.begin() + i);
		}
	}
	//Remove from atotSet
	for (size_t i = 0; i < atotSet.size(); i++)
	{
		if (callsign == atotSet[i]) {
			if (debugMode) {
				sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 17");
			}
			atotSet.erase(atotSet.begin() + i);
		}
	}
	//Remove from reqTobtTypes
	for (size_t i = 0; i < reqTobtTypes.size(); i++)
	{
		if (callsign == reqTobtTypes[i][0]) {
			if (debugMode) {
				sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 18");
			}
			reqTobtTypes.erase(reqTobtTypes.begin() + i);
		}
	}
	//Remove from dataSaved
	for (size_t i = 0; i < dataSaved.size(); i++)
	{
		if (callsign == dataSaved[i][0]) {
			if (debugMode) {
				sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 19");
			}
			dataSaved.erase(dataSaved.begin() + i);
		}
	}
	//Remove from obtList
	for (size_t i = 0; i < obtList.size(); i++)
	{
		if (callsign == obtList[i][0]) {
			if (debugMode) {
				sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 20");
			}
			obtList.erase(obtList.begin() + i);
		}
	}

	deleteFlightStrips(callsign);
	}
	catch (const std::exception& e) {
		addLogLine("ERROR: Unhandled exception removeDataFromTfc: " + (string)e.what());
	}
	catch (...) {
		addLogLine("ERROR: Unhandled exception removeDataFromTfc");
	}
}

bool CDM::flightHasCtotDisabled(string callsign) {
	for (string cs : disabledCtots) {
		if (cs == callsign) {
			return true;
		}
	}
	return false;
}

void CDM::getEcfmpData() {
	addLogLine("Called getEcfmpData...");
	try {
	if (!flowRestrictionsUrl.empty()) {
		addLogLine("AUTO - Call getEcfmpData");
		vector<EcfmpRestriction> flowDataTemp;
		CURL* curl;
		CURLcode result = CURLE_FAILED_INIT;
		std::string readBuffer;
		long responseCode = 0;
		curl = curl_easy_init();
		if (curl) {
			curl_easy_setopt(curl, CURLOPT_URL, flowRestrictionsUrl.c_str());
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
			curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
			curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
			result = curl_easy_perform(curl);
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
			curl_easy_cleanup(curl);
		}

		if (responseCode == 404 || responseCode == 401 || responseCode == 502 || CURLE_OK != result) {
			// handle error 404
			sendMessage("UNABLE TO LOAD ECFMP DATA...");
			addLogLine("UNABLE TO LOAD ECFMP DATA: rc=" + to_string(responseCode) + " result=" + to_string(result));
		}
		else {
			Json::Reader reader;
			Json::Value obj;
			Json::FastWriter fastWriter;
			reader.parse(readBuffer, obj);

			const Json::Value& measures = obj["flow_measures"];
			for (size_t i = 0; i < measures.size(); i++) {
				//Get Id
				int id = stoi(fastWriter.write(measures[i]["id"]));
				//Get Ident
				string ident = fastWriter.write(measures[i]["ident"]);
				ident.erase(std::remove(ident.begin(), ident.end(), '"'));
				//Get Event Id
				string myEvent_id = fastWriter.write(measures[i]["event_id"]);
				int event_id = -1;
				if (myEvent_id.find("null") == std::string::npos) {
					event_id = stoi(fastWriter.write(measures[i]["event_id"]));
				}
				//Get reason
				string reason = fastWriter.write(measures[i]["reason"]);
				reason.erase(std::remove(reason.begin(), reason.end(), '"'));
				//Get valid_time
				string timeStart = fastWriter.write(measures[i]["starttime"]);
				timeStart.erase(std::remove(timeStart.begin(), timeStart.end(), '"'));
				string timeEnd = fastWriter.write(measures[i]["endtime"]);
				timeEnd.erase(std::remove(timeEnd.begin(), timeEnd.end(), '"'));
				string valid_time = timeStart.substr(timeStart.length() - 11, 2) + timeStart.substr(timeStart.length() - 8, 2) + "-" + timeEnd.substr(timeEnd.length() - 11, 2) + timeEnd.substr(timeEnd.length() - 8, 2);
				string valid_date = timeStart.substr(8, 2) + "/" + timeStart.substr(5, 2);
				//Get withdrawn_at
				string withdrawn = fastWriter.write(measures[i]["withdrawn_at"]);
				withdrawn.erase(std::remove(withdrawn.begin(), withdrawn.end(), '"'));
				bool isWithdrawn = false;
				if (withdrawn.length() > 5) {
					isWithdrawn = true;
				}
				//Get type
				string typeMeasure = fastWriter.write(measures[i]["measure"]["type"]);
				typeMeasure.erase(std::remove(typeMeasure.begin(), typeMeasure.end(), '"'));
				//Get Value
				double valueMeasure = 0;
				if (typeMeasure.find("minimum_departure_interval") != std::string::npos) {
					string valueMeasureString = fastWriter.write(measures[i]["measure"]["value"]);
					if (isNumber(valueMeasureString)) {
						valueMeasure = stoi(valueMeasureString) / 60.0;
					}
				}
				if (typeMeasure.find("per_hour") != std::string::npos) {
					string valueMeasureString = fastWriter.write(measures[i]["measure"]["value"]);
					if (isNumber(valueMeasureString)) {
						valueMeasure = 60.0 / stoi(valueMeasureString);
					}
				}
				//Get Filters
				vector<string> ADEP;
				vector<string> ADES;
				vector<string> waypoints;

				for (size_t a = 0; a < measures[i]["filters"].size(); a++) {
					string typeMeasureFilter = fastWriter.write(measures[i]["filters"][a]["type"]);
					typeMeasureFilter.erase(std::remove(typeMeasureFilter.begin(), typeMeasureFilter.end(), '"'));
					if (typeMeasureFilter.find("ADEP") != std::string::npos) {
						for (size_t z = 0; z < measures[i]["filters"][a]["value"].size(); z++) {
							string myApt = fastWriter.write(measures[i]["filters"][a]["value"][z]);
							myApt.erase(std::remove(myApt.begin(), myApt.end(), '"'));
							boost::to_upper(myApt);
							ADEP.push_back(myApt);
						}
					}
					else if (typeMeasureFilter.find("ADES") != std::string::npos) {
						for (size_t z = 0; z < measures[i]["filters"][a]["value"].size(); z++) {
							string myApt = fastWriter.write(measures[i]["filters"][a]["value"][z]);
							myApt.erase(std::remove(myApt.begin(), myApt.end(), '"'));
							boost::to_upper(myApt);
							ADES.push_back(myApt);
						}
					}
					else if (typeMeasureFilter.find("waypoint") != std::string::npos) {
						for (size_t z = 0; z < measures[i]["filters"][a]["value"].size(); z++) {
							string waypoint = fastWriter.write(measures[i]["filters"][a]["value"][z]);
							waypoint.erase(std::remove(waypoint.begin(), waypoint.end(), '"'));
							boost::to_upper(waypoint);
							if (waypoint.size() > 2) {
								waypoint = waypoint.substr(0, waypoint.size() - 2);
							}
							waypoints.push_back(waypoint);
						}
					}
				}

				EcfmpRestriction flow(id, ident, event_id, reason, valid_time, valid_date, typeMeasure, valueMeasure, ADEP, ADES, waypoints);
				if ((flow.type.find("minimum_departure_interval") != std::string::npos || flow.type.find("per_hour") != std::string::npos) && isWithdrawn == false) {
					flowDataTemp.push_back(flow);
				}
			}

			ecfmpData = flowDataTemp;
		}
	}
	addLogLine("AUTO - FINISHED getEcfmpData");
	}
	catch (const std::exception& e) {
		addLogLine("ERROR: Unhandled exception getEcfmpData: " + (string)e.what());
	}
	catch (...) {
		addLogLine("ERROR: Unhandled exception getEcfmpData");
	}
}

void CDM::getSidIntervalValuesUrl(string url)
{
	addLogLine("Called getSidIntervalValuesUrl...");
	CURL* curl;
	CURLcode result = CURLE_FAILED_INIT;
	string readBuffer;
	long responseCode = 0;
	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
		result = curl_easy_perform(curl);
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
		curl_easy_cleanup(curl);
	}

	if (responseCode == 404 || responseCode == 401 || responseCode == 502 || CURLE_OK != result) {
		// handle error 404
		sendMessage("UNABLE TO LOAD SidInterval URL...");
		addLogLine("UNABLE TO LOAD SidInterval DATA: rc=" + to_string(responseCode) + " result=" + to_string(result));
	}
	else {
		std::istringstream is(readBuffer);

		//Get data from .txt file
		string lineValue;
		while (getline(is, lineValue))
		{
			if (!lineValue.empty()) {
				if (lineValue.substr(0, 1) != "#") {
					vector<string> tempList = explode(lineValue, ',');
					if (tempList.size() == 5) {
						sidInterval si = sidInterval(tempList[0], tempList[1], tempList[2], tempList[3], stod(tempList[4]));
						sidIntervalList.push_back(si);
					}
				}
			}
		}
	}
	addLogLine("FINISHED getSidIntervalValuesUrl");
}

double CDM::getSidInterval(string mySid, string listSid, string depAirport, string depRwy)
{
	if (mySid.length() > 3 && listSid.length() > 3) {
		//substr to get only the SID point
		string sid1 = mySid.substr(0, mySid.length() - 2);
		string sid2 = listSid.substr(0, listSid.length() - 2);
		for (sidInterval si : sidIntervalList) {
			if (si.airport == depAirport) {
				if (((si.sid1 == sid1 && si.sid2 == sid2) || (si.sid2 == sid1 && si.sid1 == sid2)) && depRwy == si.rwy) {
					return si.value;
				}
			}
		}
	}
	return -1;
}

bool CDM::isCdmAirport(string airport) {
	bool cdmAirport = false;
	for (string a : CDMairports)
	{
		if (airport == a) {
			return true;
		}
	}
	return false;
}

void CDM::saveData() {
	addLogLine("Called saveData...");
	try {
		bool found = false;
		vector<Plane> mySlotList = slotList;
		for (Plane plane : mySlotList) {
			found = false;
			bool foundData = false;
			string dataString = "";
			for (Plane planeSaved : slotListSaved) {
				if (plane.callsign == planeSaved.callsign && plane.isCdmAirport) {
					found = true;
					bool diffDataStored = false;
					CFlightPlan fp = FlightPlanSelect(plane.callsign.c_str());
					if (fp.IsValid()) {
						dataString = getFlightStripInfo(fp, 0) + "/" + fp.GetFlightPlanData().GetDepartureRwy() + "/" + fp.GetFlightPlanData().GetSidName();
					}
					for (int z = 0; z < dataSaved.size(); z++) {
						if (dataSaved[z].size() == 2) {
							if (plane.callsign == dataSaved[z][0]) {
								foundData = true;
								if (dataString != dataSaved[z][1]) {
									dataSaved[z][1] = dataString;
									diffDataStored = true;
								}
								break;
							}
						}
					}
					if (plane.ttot != planeSaved.ttot || plane.tsat != planeSaved.tsat || plane.eobt != planeSaved.eobt || plane.flowReason != planeSaved.flowReason || diffDataStored) {
						updateCdmDataApi(plane);
					}
					break;
				}
			}
			if (!found && plane.isCdmAirport) {
				//Plane is new in the slotList (not found int he latest slotListSaved)
				updateCdmDataApi(plane);
			}
			if (!foundData) {
				dataSaved.push_back({ plane.callsign, dataString });
			}
		}

		//Set empty times as the plane is not anymore in the slotList
		for (Plane planeSaved : slotListSaved) {
			found = false;
			for (Plane plane : mySlotList) {
				if (plane.callsign == planeSaved.callsign) {
					found = true;
					break;
				}
			}
			if (!found) {
				Plane myPlane(planeSaved.callsign, "", "", "", "", "", EcfmpRestriction(), false, false, false, true);
				updateCdmDataApi(myPlane);
			}
		}
		slotListSaved = slotList;
	if (!ftpHost.empty()) {
		if (!mySlotList.empty()) {
			for (string airport : masterAirports) {
				//Type2 -> https://fs.nool.ee/MSFS/VDGS/Specs/DATALINK.txt
				if (vdgsFileType == "2" || vdgsFileType == "3") {
					string fileName = dfad + "_" + airport + ".json";
					createJsonVDGS(mySlotList, fileName, airport);
				}
				if (vdgsFileType == "1" || vdgsFileType == "3") {
					ofstream myfile;
					string fileName = dfad + "_" + airport + ".txt";
					myfile.open(fileName, std::ofstream::out | std::ofstream::trunc);
					for (Plane plane : mySlotList) {
						if (myfile.is_open())
						{
							CFlightPlan fp = FlightPlanSelect(plane.callsign.c_str());
							if (!fp.IsValid()) {
								continue;
							}
							if (airport == fp.GetFlightPlanData().GetOrigin()) {

								string str;
								if (plane.hasManualCtot && plane.ctot != "" && plane.ttot.length() >= 4) {
									str = plane.callsign + "," + plane.eobt + "," + plane.tsat + "," + plane.ttot + "," + plane.ctot.substr(0, 4) + "," + plane.flowReason + ",";
								} else if (plane.hasManualCtot && plane.ttot.length() >= 4){
									str = plane.callsign + "," + plane.eobt + "," + plane.tsat + "," + plane.ttot + "," + plane.ttot.substr(0,4) + ",MAN CTOT" + ",";
								}
								else {
									str = plane.callsign + "," + plane.eobt + "," + plane.tsat + "," + plane.ttot + ",,flowRestriction" + ",";
								}
								myfile << str << endl;
							}
						}
					}
					myfile.close();
					upload(fileName, airport, ".txt");
				}
			}
		}
	}
	}
	catch (const std::exception& e) {
		addLogLine("ERROR: Unhandled exception saveData " + (string)e.what());
	}
	catch (...) {
		addLogLine("ERROR: Unhandled exception saveData");
	}
}

int CDM::getPlanePosition(string callsign) {
	try {
		for (int i = 0; i < slotList.size(); i++) {
			if (slotList[i].callsign == callsign) {
				return i;
			}
		}
		return -1;
	}
	catch (const std::exception& e) {
		addLogLine("ERROR: Unhandled exception getPlanePosition: " + (string)e.what());
		return -1;
	}
	catch (...) {
		addLogLine("ERROR: Unhandled exception getPlanePosition");
		return -1;
	}
}


void CDM::createJsonVDGS(vector<Plane> slotList, string fileName, string airport) {
	addLogLine("Called createJsonVDGS...");
	try {
		Document document;
		document.SetObject();
		Value version;
		version.SetInt(1);
		document.AddMember("version", version, document.GetAllocator());

		Value flightsArray(kArrayType);

		for (Plane plane : slotList) {
			CFlightPlan fp = FlightPlanSelect(plane.callsign.c_str());
			if (!fp.IsValid()) {
				continue;
			}
			if (fp.GetFlightPlanData().GetOrigin() == airport) {
				string tobtString = "", tsatString = "";
				if (plane.eobt.length() >= 4) {
					tobtString = plane.eobt;
				}
				if (plane.tsat.length() >= 4) {
					tsatString = plane.tsat;
				}
				Value flight(kObjectType);
				Value lat;
				lat.SetDouble(RadarTargetSelect(plane.callsign.c_str()).GetPosition().GetPosition().m_Latitude);
				Value lon;
				lon.SetDouble(RadarTargetSelect(plane.callsign.c_str()).GetPosition().GetPosition().m_Longitude);
				Value icao_type(RadarTargetSelect(plane.callsign.c_str()).GetCorrelatedFlightPlan().GetFlightPlanData().GetAircraftFPType(), document.GetAllocator());
				Value callsign(plane.callsign.c_str(), document.GetAllocator());
				Value destination(RadarTargetSelect(plane.callsign.c_str()).GetCorrelatedFlightPlan().GetFlightPlanData().GetDestination(), document.GetAllocator());
				Value tobt(tobtString.substr(0, 4).c_str(), document.GetAllocator());
				Value tsat(tsatString.substr(0, 4).c_str(), document.GetAllocator());

				flight.AddMember("lat", lat, document.GetAllocator());
				flight.AddMember("lon", lon, document.GetAllocator());
				flight.AddMember("icao_type", icao_type, document.GetAllocator());
				flight.AddMember("callsign", callsign, document.GetAllocator());
				flight.AddMember("destination", destination, document.GetAllocator());
				flight.AddMember("tobt", tobt, document.GetAllocator());
				flight.AddMember("tsat", tsat, document.GetAllocator());
				string slot = "";
				if (plane.hasManualCtot && plane.ttot.length() >= 4) {
					Value ctot(plane.ttot.substr(0, 4).c_str(), document.GetAllocator());
					flight.AddMember("ctot", ctot, document.GetAllocator());
				}
				Value runway(RadarTargetSelect(plane.callsign.c_str()).GetCorrelatedFlightPlan().GetFlightPlanData().GetDepartureRwy(), document.GetAllocator());
				Value sid(RadarTargetSelect(plane.callsign.c_str()).GetCorrelatedFlightPlan().GetFlightPlanData().GetSidName(), document.GetAllocator());
				flight.AddMember("runway", runway, document.GetAllocator());
				flight.AddMember("sid", sid, document.GetAllocator());
				flightsArray.PushBack(flight, document.GetAllocator());
			}
		}

		document.AddMember("flights", flightsArray, document.GetAllocator());

		// Convert the document to a JSON string
		StringBuffer buffer;
		Writer<StringBuffer> writer(buffer);
		document.Accept(writer);

		ofstream outFile;
		outFile.open(fileName, std::ofstream::out | std::ofstream::trunc);
		if (outFile.is_open()) {
			outFile << buffer.GetString() << std::endl;
			outFile.close();
		}

		upload(fileName, airport, ".json");
	}
	catch (const std::exception& e) {
		addLogLine("ERROR: Unhandled exception createJsonVDGS: " + (string)e.what());
	}
	catch (...) {
		addLogLine("ERROR: Unhandled exception createJsonVDGS");
	}
}

bool CDM::isNumber(string s)
{
	return std::any_of(s.begin(), s.end(), ::isdigit);
}

void CDM::upload(string fileName, string airport, string type)
{
	if (sftpConnection) {
		uploadSftp(fileName, airport, type);
	}
	else {
		uploadFtp(fileName, airport, type);
	}
}

void CDM::uploadSftp(string fileName, string airport, string type)
{
	addLogLine("Called uploadSftp...");
	string saveName = "/CDM_data_" + airport + type;
	int response = UploadFileFTPS(ftpHost, ftpUser, ftpPassword, fileName, saveName);
	if (response != 0) {
		sendMessage("FTP error: " + response);
	}
}

void CDM::uploadFtp(string fileName, string airport, string type)
{
	addLogLine("Called uploadFtp...");
	try {
		string saveName = "/CDM_data_" + airport + type;
		HINTERNET hInternet = InternetOpen(NULL, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
		HINTERNET hFtpSession = InternetConnect(hInternet, ftpHost.c_str(), INTERNET_DEFAULT_FTP_PORT, ftpUser.c_str(), ftpPassword.c_str(), INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0);
		FtpPutFile(hFtpSession, fileName.c_str(), saveName.c_str(), FTP_TRANSFER_TYPE_BINARY, 0);
		InternetCloseHandle(hFtpSession);
		InternetCloseHandle(hInternet);
	}
	catch (const std::exception& e) {
		addLogLine("ERROR: Unhandled exception upload: " + (string)e.what());
	}
	catch (...) {
		addLogLine("ERROR: Unhandled exception upload");
	}
}

void CDM::addLogLine(string text) {
	text = GetTimeNow() + ": " + text;
	std::ofstream file(tfad, std::ios::app);

	if (file.is_open()) {
		file << text << std::endl;
	}
	file.close();
}

void CDM::removeLog() {
	// Remove the file
	remove(tfad.c_str());
}

int CDM::GetVersion() {
	CURL* curl;
	CURLcode result = CURLE_FAILED_INIT;
	std::string readBuffer = "";
	curl = curl_easy_init();
	if (curl) {
		string url = "https://raw.githubusercontent.com/rpuig2001/CDM/master/version.txt";
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
		result = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
	}

	//Check if it is not a beta version
	/*if (string(MY_PLUGIN_VERSION).find("b") == std::string::npos) {
		//Check version
		if (!readBuffer.empty() && readBuffer.find(MY_PLUGIN_VERSION) == std::string::npos) {
			string DisplayMsg = "Please UPDATE YOUR CDM PLUGIN, version " + readBuffer + " is OUT! You have version " + MY_PLUGIN_VERSION " installed, download it from vats.im/CDM";
			DisplayUserMessage(MY_PLUGIN_NAME, "UPDATE", DisplayMsg.c_str(), true, false, false, false, false);
		}
	}*/

	return -1;
}

bool CDM::getCtotsFromUrl(string code)
{
	addLogLine("Called getCtotsFromUrl...");
	try {
		evCtots.clear();
		slotFile.clear();
		string vatcanUrl = code;
		CURL* curl;
		CURLcode result = CURLE_FAILED_INIT;
		std::string readBuffer;
		long responseCode = 0;
		curl = curl_easy_init();
		if (curl) {
			curl_easy_setopt(curl, CURLOPT_URL, vatcanUrl.c_str());
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
			curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
			curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
			result = curl_easy_perform(curl);
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
			curl_easy_cleanup(curl);
		}

		if (responseCode == 404 || responseCode == 401 || responseCode == 502 || CURLE_OK != result) {
			// handle error 404
			sendMessage("UNABLE TO LOAD CTOTs FROM VATCAN...");
		}
		else {
			std::istringstream is(readBuffer);

			//Get data from .txt file
			string lineValue;
			int i = 0;
			while (getline(is, lineValue))
			{
				addVatcanCtotToEvCTOT(lineValue);
				i++;
			}
			if (i <= 1) {
				ctotCid = false;
				addLogLine("No Ctots in file, disabling EvCTOT...");
			}
		}
	}
	catch (const std::exception& e) {
		addLogLine("ERROR: Unhandled exception getCtotsFromUrl: " + (string)e.what());
	}
	catch (...) {
		addLogLine("ERROR: Unhandled exception getCtotsFromUrl");
	}

	return true;
}

bool CDM::getTaxiZonesFromUrl(string url) {
	try {
		CURL* curl;
		CURLcode result = CURLE_FAILED_INIT;
		string readBuffer;
		long responseCode = 0;
		curl = curl_easy_init();
		if (curl) {
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
			curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
			curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
			result = curl_easy_perform(curl);
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
			curl_easy_cleanup(curl);
		}

		if (responseCode == 404 || responseCode == 401 || responseCode == 502 || CURLE_OK != result) {
			// handle error 404
			sendMessage("UNABLE TO LOAD TaxiZones URL...");
		}
		else {
			std::istringstream is(readBuffer);

			//Get data from .txt file
			string lineValue;
			while (getline(is, lineValue))
			{
				if (!lineValue.empty()) {
					if (lineValue.substr(0, 1) != "#") {
						if (lineValue.length() > 1) {
							if (isdigit(lineValue[lineValue.length() - 1])) {
								TxtTimesVector.push_back(lineValue);
							}
							else {
								TxtTimesVector.push_back(lineValue.substr(0, lineValue.length() - 1));
							}
						}
					}
				}
			}
		}
	}
	catch (const std::exception& e) {
		addLogLine("ERROR: Unhandled exception getTaxiZonesFromUrl: " + (string)e.what());
	}
	catch (...) {
		addLogLine("ERROR: Unhandled exception getTaxiZonesFromUrl");
	}

	return true;
}

int CDM::GetdifferenceTime(string hour1, string min1, string hour2, string min2) {

	string stringHour1 = hour1;
	string stringMin1 = min1;
	string stringTime1 = stringHour1 + stringMin1;

	string stringHour2 = hour2;
	string stringMin2 = min2;
	string stringTime2 = stringHour2 + stringMin2;

	int time1 = stoi(stringTime1);
	int time2 = stoi(stringTime2);

	return time1 - time2;
}

int CDM::GetDifferenceTimeHHMMSS(const std::string& time1, const std::string& time2) {
	if (time1.length() != 6 || time2.length() != 6) {
		return 0;
	}

	int h1 = std::stoi(time1.substr(0, 2));
	int m1 = std::stoi(time1.substr(2, 2));
	int s1 = std::stoi(time1.substr(4, 2));

	int h2 = std::stoi(time2.substr(0, 2));
	int m2 = std::stoi(time2.substr(2, 2));
	int s2 = std::stoi(time2.substr(4, 2));

	int totalSec1 = h1 * 3600 + m1 * 60 + s1;
	int totalSec2 = h2 * 3600 + m2 * 60 + s2;

	return std::abs(totalSec1 - totalSec2) / 60;
}

string CDM::GetTimeNow() {
	time_t rawtime;
	struct tm ptm;
	time(&rawtime);
	gmtime_s(&ptm, &rawtime);
	string hour = to_string(ptm.tm_hour % 24);
	if (stoi(hour) < 10) {
		hour = "0" + hour;
	}
	string min = to_string(ptm.tm_min);
	if (stoi(min) < 10) {
		min = "0" + min;
	}
	string sec = to_string(ptm.tm_sec);
	if (stoi(sec) < 10) {
		sec = "0" + sec;
	}

	return hour + min + sec;
}

//Multithread Run Functions
void CDM::multithread(void (CDM::* f)()) {
	try {
		thread* mythread = new thread(f, this);
		mythread->detach();
	}
	catch (std::exception e) {
		cout << "Failed to multi-thread function";
	}
}


//Get Data from the xml file
string CDM::getFromXml(string xpath)
{
	xml_document doc;

	// load the XML file
	doc.load_file(pfad.c_str());

	pugi::xpath_node_set altPugi = doc.select_nodes(xpath.c_str());

	std::vector<std::string> result;
	for (auto xpath_node : altPugi) {
		if (xpath_node.attribute() != nullptr)
			result.push_back(xpath_node.attribute().value());
		else
			result.push_back(xpath_node.node().child_value());
	}

	if (result.size() > 0)
	{
		return result[0];
	}
	else {
		return "";
	}
}

int CDM::getDeIceId(string callsign) {
	for (vector<string> deice : deiceList) {
		if (deice[0] == callsign) {
			if (deice[2].find("REM") != std::string::npos && deice[2].length() == 4) {
				return stoi(deice[2].substr(3, 1));
			}
			else if (deice[1] == "STND") {
				return 0;
			}
		}
	}
	return -1;
}

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
	}
	else if (wtc == 'M') {
		if (remNum == 1) return deIceTimeM + deIceTaxiRem1;
		if (remNum == 2) return deIceTimeM + deIceTaxiRem2;
		if (remNum == 3) return deIceTimeM + deIceTaxiRem3;
		if (remNum == 4) return deIceTimeM + deIceTaxiRem4;
		if (remNum == 5) return deIceTimeM + deIceTaxiRem5;
		return deIceTimeM;
	}
	else if (wtc == 'H') {
		if (remNum == 1) return deIceTimeH + deIceTaxiRem1;
		if (remNum == 2) return deIceTimeH + deIceTaxiRem2;
		if (remNum == 3) return deIceTimeH + deIceTaxiRem3;
		if (remNum == 4) return deIceTimeH + deIceTaxiRem4;
		if (remNum == 5) return deIceTimeH + deIceTaxiRem5;
		return deIceTimeH;
	}
	else if (wtc == 'J') {
		if (remNum == 1) return deIceTimeJ + deIceTaxiRem1;
		if (remNum == 2) return deIceTimeJ + deIceTaxiRem2;
		if (remNum == 3) return deIceTimeJ + deIceTaxiRem3;
		if (remNum == 4) return deIceTimeJ + deIceTaxiRem4;
		if (remNum == 5) return deIceTimeJ + deIceTaxiRem5;
		return deIceTimeJ;
	}
	
	return deIceTimeM;
}

string CDM::getDiffTOBTTSAT(string TSAT, string TOBT) {
	try {
		if (TSAT.length() < 4 || TOBT.length() < 4) {
			return "";
		}
		if (TSAT.substr(0, 4) == TOBT.substr(0, 4)) {
			return "";
		}

		int tsat_hours = stoi(TSAT.substr(0, 2));
		int tsat_minutes = stoi(TSAT.substr(2, 2));
		int tobt_hours = stoi(TOBT.substr(0, 2));
		int tobt_minutes = stoi(TOBT.substr(2, 2));

		int tsat_total_minutes = tsat_hours * 60 + tsat_minutes;
		int tobt_total_minutes = tobt_hours * 60 + tobt_minutes;

		return "/" + to_string(tsat_total_minutes - tobt_total_minutes);
	}
	catch (const std::exception& e) {
		addLogLine("ERROR: Unhandled exception getDiffTOBTTSAT: " + (string)e.what());
		return "";
	}
	catch (...) {
		addLogLine("ERROR: Unhandled exception getDiffTOBTTSAT");
		return "";
	}
}

string CDM::getDiffNowTime(string time) {
	try {
		string timeNow = GetTimeNow();
		if (time == "") {
			return "";
		}
		if (time.length() < 4 || timeNow.length() < 4) {
			return "0";
		}
		if (timeNow.substr(0, 4) == time.substr(0, 4)) {
			return "0";
		}

		int time_hours = stoi(time.substr(0, 2));
		int time_minutes = stoi(time.substr(2, 2));
		int timeNow_hours = stoi(timeNow.substr(0, 2));
		int timeNow_minutes = stoi(timeNow.substr(2, 2));

		int time_total_minutes = time_hours * 60 + time_minutes;
		int timeNow_total_minutes = timeNow_hours * 60 + timeNow_minutes;

		if (time > timeNow) {
			return to_string(timeNow_total_minutes - time_total_minutes);
		}
		return "+" + to_string(timeNow_total_minutes - time_total_minutes);
	}
	catch (const std::exception& e) {
		addLogLine("ERROR: Unhandled exception getDiffNowTTOT: " + (string)e.what());
		return "0";
	}
	catch (...) {
		addLogLine("ERROR: Unhandled exception getDiffNowTTOT");
		return "0";
	}
}

string CDM::GetTimedStatus(string status)
{
	size_t dashPos = status.find('-');
	if (dashPos == string::npos)
		return status;

	// Use current time only (no static state)
	auto now = std::chrono::steady_clock::now();
	auto seconds =
		std::chrono::duration_cast<std::chrono::seconds>(
			now.time_since_epoch()
		).count();

	// Toggle every 2 seconds
	bool firstPart = ((seconds / 2) % 2) == 0;

	return firstPart
		? status.substr(0, dashPos)
		: status.substr(dashPos + 1);
}

void CDM::addVatcanCtotToEvCTOT(string line) {
	slotFile.push_back({ line.substr(0,line.find(",")), line.substr(line.find(",")+1, 4)});
}

bool CDM::getPanelStatus() {
	return showPanel;
}

bool CDM::getAtfcmList() {
	return showAtfcmList;
}

vector<string> CDM::explode(std::string const& s, char delim)
{
	std::vector<std::string> result;
	std::istringstream iss(s);

	for (std::string token; std::getline(iss, token, delim); )
	{
		result.push_back(std::move(token));
	}

	return result;
}

bool CDM::OnCompileCommand(const char* sCommandLine) {
	if (startsWith(".cdm ecfmp", sCommandLine)) {
		addLogLine(sCommandLine);
		sendMessage("Reloading ECFMP data...");
		multithread(&CDM::getEcfmpData);
		return true;
	}

	if (startsWith(".cdm refresh", sCommandLine)) {
		addLogLine(sCommandLine);
		sendMessage("Refreshing Now...");
		countTime = std::time(nullptr) - refreshTime;
		countFetchServerTime = std::time(nullptr) - 60;
		return true;
	}

	if (startsWith(".cdm refreshtime", sCommandLine)) {
		try {
			addLogLine(sCommandLine);
			string line = sCommandLine;
			if (line.substr(line.length() - 3, 1) == " ") {
				refreshTime = stoi(line.substr(line.length() - 2)) * 50;
				sendMessage("Refresh Time set to: " + line.substr(line.length() - 2));
			}
			else if (line.substr(line.length() - 2, 1) == " ") {
				refreshTime = stoi(line.substr(line.length() - 1));
				sendMessage("Refresh Time set to: " + line.substr(line.length() - 1));
			}
			else {
				sendMessage("INCORRECT REFRESH TIME VALUE...");
			}
			return true;
		}
		catch (const std::exception& e) {
			addLogLine("ERROR: Unhandled exception .cdm refreshtime: " + (string)e.what());
			return true;
		}
		catch (...) {
			addLogLine("ERROR: Unhandled exception .cdm refreshtime");
			return true;
		}
	}

	if (startsWith(".cdm help", sCommandLine))
	{
		addLogLine(sCommandLine);
		sendMessage("CDM Commands: .cdm ctot - .cdm panel - .cdm master {airport} - .cdm slave {airport} - .cdm refreshtime {seconds} - .cdm startupdelay {icao}/{rwy} {start_time} - .cdm departuredelay {icao}/{rwy} {start_time} - .cdm lvo - .cdm realmode - .cdm server - .cdm remarks - .cdm rate - .cdm help");
		return true;
	}

	if (startsWith(".cdm realmode", sCommandLine))
	{
		addLogLine(sCommandLine);
		if (realMode) {
			realMode = false;
			sendMessage("Real Mode set to OFF");
		}
		else {
			realMode = true;
			sendMessage("Real Mode set to ON");
		}
		return true;
	}

	if (startsWith(".cdm event", sCommandLine))
	{
		addLogLine(sCommandLine);
		if (eventMode) {
			eventMode = false;
			sendMessage("Event Mode set to OFF");
		}
		else {
			eventMode = true;
			sendMessage("Event Mode set to ON");
		}
		return true;
	}

	if (startsWith(".cdm panel", sCommandLine))
	{
		addLogLine(sCommandLine);
		if (showPanel) {
			showPanel = false;
		}
		else {
			showPanel = true;
		}
		return true;
	}

	if (startsWith(".cdm atfcm", sCommandLine))
	{
		addLogLine(sCommandLine);
		if (showAtfcmList) {
			showAtfcmList = false;
		}
		else {
			showAtfcmList = true;
			std::thread t73(&CDM::getCdmServerRelevantFlights, this);
			t73.detach();
		}
		return true;
	}

	if (startsWith(".cdm server", sCommandLine))
	{
		addLogLine(sCommandLine);
		if (serverEnabled) {
			serverEnabled = false;
			sendMessage("Server Disabled");
		}
		else {
			serverEnabled = true;
			sendMessage("Server Enabled");
		}
		return true;
	}

	if (startsWith(".cdm debug", sCommandLine))
	{
		addLogLine(sCommandLine);
		if (debugMode) {
			debugMode = false;
			sendMessage("Debug Mode set to OFF");
		}
		else {
			debugMode = true;
			sendMessage("Debug Mode set to ON");
		}
		return true;
	}

	if (startsWith(".cdm rate", sCommandLine))
	{
		addLogLine(sCommandLine);
		sendMessage("Reloading rates....");
		if (rateUrl.length() <= 1) {
			if (debugMode) {
				sendMessage("[DEBUG MESSAGE] - USING RATE FROM LOCAL TXT FILE");
			}
			getRate();
		}
		else {
			if (debugMode) {
				sendMessage("[DEBUG MESSAGE] - USING TAXIZONES FROM URL");
			}
			getRateFromUrl(rateUrl);
		}
		return true;
	}

	if (startsWith(".cdm remarks", sCommandLine))
	{
		addLogLine(sCommandLine);
		if (remarksOption) {
			remarksOption = false;
			sendMessage("Set TSAT to Scratchpad to OFF");
		}
		else {
			remarksOption = true;
			sendMessage("Set TSAT to Scratchpad to ON");
		}
		return true;
	}

	if (startsWith(".cdm rmkctot", sCommandLine))
	{
		addLogLine(sCommandLine);
		if (remarksOptionCtot) {
			remarksOptionCtot = false;
			sendMessage("Set CTOT to Scratchpad to OFF");
		}
		else {
			remarksOptionCtot = true;
			sendMessage("Set CTOT to Scratchpad to ON");
		}
		return true;
	}

	if (startsWith(".cdm startupdelay", sCommandLine))
	{
		try {
			addLogLine(sCommandLine);
			string line = sCommandLine;
			string apt = line.substr(line.find("/") - 4, 4);
			boost::to_upper(apt);
			string rwy = "";
			if (line.substr(line.find("/") + 3, 1) == " ") {
				rwy = line.substr(line.find("/") + 1, 2);
			}
			else if (line.substr(line.find("/") + 4, 1) == " ") {
				rwy = line.substr(line.find("/") + 1, 3);
			}

			bool isTimeOk = false;
			std::istringstream iss(line);
			std::vector<std::string> customDelayValues;
			std::string token;
			while (std::getline(iss, token, ' ')) {
				customDelayValues.push_back(token);
			}
			for (const auto& substring : customDelayValues) {
				std::cout << substring << std::endl;
			}
			string myTime = customDelayValues[customDelayValues.size() - 1];

			if (myTime.length() == 4 && isNumber(myTime)) {
				isTimeOk = true;
			}
			else {
				if ((myTime.length() == 2 || myTime.length() == 1) && isNumber(myTime)) {
					isTimeOk = true;
					myTime = (calculateTime(GetTimeNow(), stoi(myTime)).substr(0, 4));
				}
			}

			if (isTimeOk) {
				//use myTime 9999 to remove delay for APT/RWY config
				if (myTime == "9999") {
					for (size_t i = 0; i < delayList.size(); i++) {
						if (delayList[i].airport == apt && delayList[i].rwy == rwy) {
							sendMessage("REMOVING START-UP DELAY " + apt + "/" + rwy);
							delayList.erase(delayList.begin() + i);
						}
					}
				}
				else {
					Delay d = Delay(apt, rwy, myTime, "tsat");

					//Get Time now
					time_t rawtime;
					struct tm ptm;
					time(&rawtime);
					gmtime_s(&ptm, &rawtime);
					string hour = to_string(ptm.tm_hour % 24);
					string min = to_string(ptm.tm_min);

					int difTime = difftime(stoi(d.time), stoi(hour + min));

					if (difTime > 0) {
						// Remove existing delay for the same airport and runway
						for (size_t i = 0; i < delayList.size(); i++) {
							if (delayList[i].airport == apt && delayList[i].rwy == rwy) {
								delayList.erase(delayList.begin() + i);
							}
						}
						sendMessage("Adding START-UP DELAY for " + apt + " rwy: " + rwy + " from time: " + myTime + "z.");
						delayList.push_back(d);
						//Update times to slaves
						countTime = std::time(nullptr) - refreshTime;
						//addTimeToListForSpecificAirportAndRunway(difTime, GetTimeNow(), d.airport, d.rwy);
					}
					else {
						sendMessage("START-UP DELAY NOT ADDED. Time must be in the future");
					}
				}
			}
			else {
				sendMessage("Wrong time formatting to add delay. Please use time in 4 digits format (1234) or minutes with 1 or 2 digits codes (12 or 1)");
			}

			return true;
		}
		catch (const std::exception& e) {
			addLogLine("ERROR: Unhandled exception .cdm startupdelay: " + (string)e.what());
			return true;
		}
		catch (...) {
			addLogLine("ERROR: Unhandled exception .cdm startupdelay");
			return true;
		}
	}

	if (startsWith(".cdm departuredelay", sCommandLine))
	{
		try {
			addLogLine(sCommandLine);
			string line = sCommandLine;
			string apt = line.substr(line.find("/") - 4, 4);
			boost::to_upper(apt);
			string rwy = "";
			if (line.substr(line.find("/") + 3, 1) == " ") {
				rwy = line.substr(line.find("/") + 1, 2);
			}
			else if (line.substr(line.find("/") + 4, 1) == " ") {
				rwy = line.substr(line.find("/") + 1, 3);
			}

			bool isTimeOk = false;
			std::istringstream iss(line);
			std::vector<std::string> customDelayValues;
			std::string token;
			while (std::getline(iss, token, ' ')) {
				customDelayValues.push_back(token);
			}
			for (const auto& substring : customDelayValues) {
				std::cout << substring << std::endl;
			}
			string myTime = customDelayValues[customDelayValues.size() - 1];

			if (myTime.length() == 4 && isNumber(myTime)) {
				isTimeOk = true;
			}
			else {
				if ((myTime.length() == 2 || myTime.length() == 1) && isNumber(myTime)) {
					isTimeOk = true;
					myTime = (calculateTime(GetTimeNow(), stoi(myTime)).substr(0, 4));
				}
			}

			if (isTimeOk) {
				//use myTime 9999 to remove delay for APT/RWY config
				if (myTime == "9999") {
					for (size_t i = 0; i < delayList.size(); i++) {
						if (delayList[i].airport == apt && delayList[i].rwy == rwy) {
							sendMessage("REMOVING DEPARTURE DELAY " + apt + "/" + rwy);
							delayList.erase(delayList.begin() + i);
						}
					}
				}
				else {
					Delay d = Delay(apt, rwy, myTime, "ttot");

					//Get Time now
					time_t rawtime;
					struct tm ptm;
					time(&rawtime);
					gmtime_s(&ptm, &rawtime);
					string hour = to_string(ptm.tm_hour % 24);
					string min = to_string(ptm.tm_min);

					int difTime = difftime(stoi(d.time), stoi(hour + min));

					if (difTime > 0) {
						// Remove existing delay for the same airport and runway
						for (size_t i = 0; i < delayList.size(); i++) {
							if (delayList[i].airport == apt && delayList[i].rwy == rwy) {
								delayList.erase(delayList.begin() + i);
							}
						}
						sendMessage("Adding DEPARTURE DELAY for " + apt + " rwy: " + rwy + " from time: " + myTime + "z.");
						delayList.push_back(d);
						//Update times to slaves
						countTime = std::time(nullptr) - refreshTime;
						//addTimeToListForSpecificAirportAndRunway(difTime, GetTimeNow(), d.airport, d.rwy);
					}
					else {
						sendMessage("DEPARTURE DELAY NOT ADDED. Time must be in the future");
					}
				}
			}
			else {
				sendMessage("Wrong time formatting to add delay. Please use time in 4 digits format (1234) or minutes with 1 or 2 digits codes (12 or 1)");
			}

			return true;
		}
		catch (const std::exception& e) {
			addLogLine("ERROR: Unhandled exception .cdm departuredelay: " + (string)e.what());
			return true;
		}
		catch (...) {
			addLogLine("ERROR: Unhandled exception .cdm departuredelay");
			return true;
		}
	}

	if (startsWith(".cdm save", sCommandLine))
	{
		try {
			addLogLine(sCommandLine);
			sendMessage("Saving CDM data....");
			//save data to file
			ofstream outfile(sfad.c_str());

			for (Plane pl : slotList)
			{
				outfile << pl.callsign + "," + pl.eobt + "," + pl.tsat + "," + pl.ttot << std::endl;
			}

			outfile.close();
			sendMessage("Done");
			return true;
		}
		catch (const std::exception& e) {
			addLogLine("ERROR: Unhandled exception .cdm save: " + (string)e.what());
			return true;
		}
		catch (...) {
			addLogLine("ERROR: Unhandled exception .cdm save");
			return true;
		}
	}
	
	if (startsWith(".cdm nvo", sCommandLine))
	{
		addLogLine(sCommandLine);
		rateString = getFromXml("/CDM/rate/@ops");
		sendMessage("Normal Visibility Operations Rate Set: " + rateString);
		return true;
	}

	if (startsWith(".cdm load", sCommandLine))
	{
		addLogLine(sCommandLine);
		sendMessage("Loading CDM data....");
		/*slotList.clear();
		//load data from file
		fstream file;
		string lineValue;
		file.open(sfad.c_str(), std::ios::in);
		while (getline(file, lineValue))
		{
			slotList.push_back(lineValue);
		}*/
		sendMessage("Command disabled");
		return true;
	}

	if (startsWith(".cdm ctot", sCommandLine))
	{
		addLogLine(sCommandLine);
		sendMessage("Loading CTOTs data....");

		if (ctotURL.length() <= 1) {
			ctotCid = false;
			if (debugMode) {
				sendMessage("[DEBUG MESSAGE] - NOT SHOWING EVCTOTs");
			}
			//Get data from .txt file
			/*fstream fileCtot;
			string lineValueCtot;
			fileCtot.open(cfad.c_str(), std::ios::in);
			while (getline(fileCtot, lineValueCtot))
			{
				addCtotToMainList(lineValueCtot);
			}*/
		}
		else {
			ctotCid = true;
			if (debugMode) {
				sendMessage("[DEBUG MESSAGE] - SHOWING EVCTOTs");
			}
			getCtotsFromUrl(ctotURL);
		}

		return true;
	}

	if (startsWith(".cdm ctotTime", sCommandLine))
	{
		addLogLine(sCommandLine);
		sendMessage("Reloading Ctot Expired time....");
		expiredCTOTTime = stoi(getFromXml("/CDM/expiredCtot/@time"));
		sendMessage("Done");
		return true;
	}

	if (startsWith(".cdm lvo", sCommandLine))
	{
		addLogLine(sCommandLine);
		if (lvo) {
			sendMessage("Low Visibility Operations desactivated");
			lvo = false;
		}
		else {
			sendMessage("Low Visibility Operations activated");
			lvo = true;
		}
		return true;
	}

	if (startsWith(".cdm master", sCommandLine))
	{
		try {
			addLogLine(sCommandLine);
			string line = sCommandLine; boost::to_upper(line);
			vector<string> lineAirports = explode(line, ' ');

			if (lineAirports.size() > 2) {
				string ATC_Position = ControllerMyself().GetCallsign();
				for (size_t i = 2; i < lineAirports.size(); i++) {
					string addedAirport = lineAirports[i];
					bool found = false;
					for (string apt : masterAirports)
					{
						if (apt == addedAirport) {
							found = true;
						}
					}
					if (!found) {
						std::thread t(&CDM::setMasterAirport, this, addedAirport, ATC_Position);
						t.detach();
					}
				}
			}
			else {
				sendMessage("NO AIRPORT SET");
			}
			return true;
		}
		catch (const std::exception& e) {
			addLogLine("ERROR: Unhandled exception .cdm master: " + (string)e.what());
			return true;
		}
		catch (...) {
			addLogLine("ERROR: Unhandled exception .cdm master");
			return true;
		}
	}

	if (startsWith(".cdm slave", sCommandLine))
	{
		try {
			addLogLine(sCommandLine);
			string line = sCommandLine; boost::to_upper(line);
			vector<string> lineAirports = explode(line, ' ');

			if (lineAirports.size() > 2) {
				string ATC_Position = ControllerMyself().GetCallsign();
				for (size_t i = 2; i < lineAirports.size(); i++) {
					string addedAirport = lineAirports[i];
					bool found = false;
					int a = 0;
					for (string apt : masterAirports)
					{
						if (apt == addedAirport) {
							std::thread t(&CDM::removeMasterAirport, this, addedAirport, ATC_Position);
							t.detach();
							found = true;
						}
						a++;
					}
					if (!found) {
						sendMessage("AIRPORT " + addedAirport + " NOT FOUND");
					}
				}
			}
			else {
				sendMessage("NO AIRPORT SET");
			}

			return true;
		}
		catch (const std::exception& e) {
			addLogLine("ERROR: Unhandled exception .cdm slave: " + (string)e.what());
			return true;
		}
		catch (...) {
			addLogLine("ERROR: Unhandled exception .cdm slave");
			return true;
		}
	}

	if (startsWith(".cdm resetmaster", sCommandLine))
	{
		try {
			addLogLine(sCommandLine);
			string line = sCommandLine; boost::to_upper(line);
			vector<string> lineAirports = explode(line, ' ');

			if (lineAirports.size() > 2) {
				string ATC_Position = ControllerMyself().GetCallsign();
				for (size_t i = 2; i < lineAirports.size(); i++) {
					string addedAirport = lineAirports[i];
					std::thread t(&CDM::removeAllMasterAirportsByAirport, this, addedAirport);
					t.detach();
				}
			}
			else {
				sendMessage("NO AIRPORT SET");
			}

			return true;
		}
		catch (const std::exception& e) {
			addLogLine("ERROR: Unhandled exception .cdm resetmaster: " + (string)e.what());
			return true;
		}
		catch (...) {
			addLogLine("ERROR: Unhandled exception .cdm resetmaster");
			return true;
		}
	}

	if (startsWith(".cdm recover", sCommandLine))
	{
		try {
			addLogLine(sCommandLine);
			string line = sCommandLine; boost::to_upper(line);
			vector<string> lineAirports = explode(line, ' ');

			if (lineAirports.size() > 2) {
				string ATC_Position = ControllerMyself().GetCallsign();
				for (size_t i = 2; i < lineAirports.size(); i++) {
					string addedAirport = lineAirports[i];
					std::thread t(&CDM::removeAllMasterAirportsByAirport, this, addedAirport);
					t.detach();
					copyServerSavedData(addedAirport);
				}
			}
			else {
				sendMessage("NO AIRPORT SET");
			}
			return true;
		}
		catch (const std::exception& e) {
			addLogLine("ERROR: Unhandled exception .cdm recover: " + (string)e.what());
			return true;
		}
		catch (...) {
			addLogLine("ERROR: Unhandled exception .cdm recover");
			return true;
		}
	}

	if (startsWith(".cdm data", sCommandLine))
	{
		addLogLine(sCommandLine);
		string planes = "";
		if (slotList.size() > 0) {
			for (Plane p : slotList)
			{
				planes += p.callsign + " ";
			}
			sendMessage("PLANES: " + planes);
		}
		else {
			sendMessage("NO PLANES IN THE LIST");
		}

		return true;
	}

	if (startsWith(".cdm status", sCommandLine))
	{
		addLogLine(sCommandLine);
		string apts = "";
		if (masterAirports.size() > 0) {
			for (string apt : masterAirports)
			{
				apts += apt + " ";
			}
			sendMessage("MASTER AIRPORTS: " + apts);
		}
		else {
			sendMessage("NO MASTER AIRPORTS");
		}
		sendMessage("DEFAULT RATE: " + rateString);

		return true;
	}
	return false;
}

void CDM::OnTimer(int Counter) {

	FuncBuffer = 0;

	blink = !blink;

	if (blink) {
		if (disCount < 3) {
			disCount++;
		}
		else {
			disCount = 0;
		}
	}
}

vector<string> CDM::splitString(const std::string& str, char delimiter) {
	std::vector<std::string> tokens;
	std::istringstream stream(str);
	std::string token;

	while (std::getline(stream, token, delimiter)) {
		tokens.push_back(token);
	}

	return tokens;
}

string CDM::getFlightStripInfo(CFlightPlan FlightPlan, int position) {
	if (position >= 0 && position <= 7 && FlightPlan.IsValid()) {
		//   0    1   2     3   4     5       6       7
		// ASRT/TSAC/TOBT/TSAT/TTOT/deIce/ecfmpId/manualCtot/
		string annotation = FlightPlan.GetControllerAssignedData().GetFlightStripAnnotation(0);
		vector<string> values = split(annotation, '/');

		if (position < values.size()) {
			return values[position];
		}
	}
	return "";
}

void CDM::setFlightStripInfo(CFlightPlan FlightPlan, string text, int position) {
	if (position >= 0 && position <= 7 && FlightPlan.IsValid()) {
		//   0    1   2     3   4     5       6       7
		// ASRT/TSAC/TOBT/TSAT/TTOT/deIce/ecfmpId/manualCtot/
		string annotation = FlightPlan.GetControllerAssignedData().GetFlightStripAnnotation(0);
		if (annotation == "") {
			annotation = "/////////";
		}
		vector<string> values = split(annotation, '/');

		if (position < values.size()) {
			values[position] = text;

			string finalString = "";
			for (int i = 0; i < values.size(); i++) {
				finalString += values[i];
				if (i < values.size()) {
					finalString += "/";
				}
			}
			FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(0, finalString.c_str());
		}
	}
}

void CDM::refreshActions1() {
	refresh1 = true;
	readyToUpdateList = false;
	saveData();
	//Execute background process in the background
	slotListToUpdate = backgroundProcess_recaulculate();
	//Check rates
	getNetworkRates();
	readyToUpdateList = true;
	refresh1 = false;
}

void CDM::refreshActions2() {
	addLogLine("[AUTO] - REFRESH ECFMP");
	getEcfmpData();
	refresh2 = false;
}

void CDM::refreshActions3() {
	addLogLine("[AUTO] - REFRESH API 1");
	getCdmServerRestricted(slotList);
	getCdmServerMasterAirports();
	getCdmServerRelevantFlights();
	getCdmServerOnTime();
	refresh3 = false;
}

void CDM::refreshActions4() {
	addLogLine("[AUTO] - REFRESH API 2");
	getCdmServerStatus();
	getIffOffBlockTimes();
	refresh4 = false;
}

//API requests

bool CDM::setMasterAirport(string airport, string position) {
	addLogLine("Called setMasterAirport...");
	if (serverEnabled) {
		try {
			addLogLine("Call - Set Master airport " + airport + "(" + position + ")");
			CURL* curl;
			CURLcode result = CURLE_FAILED_INIT;
			string readBuffer;
			long responseCode = 0;
			curl = curl_easy_init();
			if (curl) {
				string url = cdmServerUrl + "/airport/setMaster?airport=" + airport + "&position=" + position;
				string apiKeyHeader = "x-api-key: " + apikey;
				struct curl_slist* headers = NULL;
				headers = curl_slist_append(headers, apiKeyHeader.c_str());
				curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
				curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
				curl_easy_setopt(curl, CURLOPT_POST, 1L);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
				curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
				curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
				curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
				result = curl_easy_perform(curl);
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
				curl_easy_cleanup(curl);
			}

			if ((responseCode == 404 || CURLE_OK != result) && responseCode != 401) {
				addLogLine("UNABLE TO CONNECT CDM-API... Master not set.");
			}
			else {
				std::istringstream is(readBuffer);
				string lineValue;
				while (getline(is, lineValue))
				{
					if (lineValue == "true") {
						for (int attempt = 0; attempt < 10; ++attempt) {  // Retry up to 3 times
							try {
								masterAirports.push_back(airport);
								sendMessage("Successfully set master airport " + airport);
								addLogLine("Successfully set master airport " + airport);
								lastAddedIcao = "";
								//Update server master list
								std::thread t99(&CDM::getCdmServerMasterAirports, this);
								t99.detach();
								return true;
							}
							catch (const std::system_error& e) {
								addLogLine("ERROR: Unhandled exception setMasterAirport: " + (string)e.what());
							}
						}
					}
					sendMessage("Unable to set master airport " + airport);
					addLogLine("Unable to set master airport " + airport);
				}
			}
		}
		catch (const std::exception& e) {
			addLogLine("ERROR: Unhandled exception setMasterAirport: " + (string)e.what());
			lastAddedIcao = "";
			return false;
		}
		catch (...) {
			addLogLine("ERROR: Unhandled exception setMasterAirport");
			lastAddedIcao = "";
			return false;
		}
	}
	else {
		masterAirports.push_back(airport);
		sendMessage("Successfully set master airport (Locally only) " + airport);
		addLogLine("Successfully set master airport (Locally only) " + airport);
	}

	lastAddedIcao = "";
	return false;
}

bool CDM::removeMasterAirport(string airport, string position) {
	addLogLine("Call - Remove Master airport " + airport + "(" + position + ")");
	if (serverEnabled) {
		CURL* curl;
		CURLcode result = CURLE_FAILED_INIT;
		string readBuffer;
		long responseCode = 0;
		curl = curl_easy_init();
		if (curl) {
			string url = cdmServerUrl + "/airport/removeMaster?airport=" + airport + "&position=" + position;
			string apiKeyHeader = "x-api-key: " + apikey;
			struct curl_slist* headers = NULL;
			headers = curl_slist_append(headers, apiKeyHeader.c_str());
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
			curl_easy_setopt(curl, CURLOPT_POST, true);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
			curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
			curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
			result = curl_easy_perform(curl);
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
			curl_easy_cleanup(curl);
		}

		if (responseCode == 404 || responseCode == 401 || responseCode == 502 || CURLE_OK != result) {
			addLogLine("UNABLE TO CONNECT CDM-API...");
			for (int a = 0; a < masterAirports.size(); a++)
			{
				if (masterAirports[a] == airport) {
					masterAirports.erase(masterAirports.begin() + a);
					addLogLine("Successfully removed master airport (Locally only) " + airport);
					sendMessage("Successfully removed master airport (Locally only) " + airport);
					return true;
				}
			}
		}
		else {
			std::istringstream is(readBuffer);
			//Get data from .txt file
			string lineValue;
			while (getline(is, lineValue))
			{
				if (lineValue == "true") {
					addLogLine("Successfully removed master airport " + airport);
					sendMessage("Successfully removed master airport " + airport);
				}
				else {
					addLogLine("Successfully removed master airport (Locally only) " + airport);
					sendMessage("Successfully removed master airport (Locally only) " + airport);
				}
			}
			for (int a = 0; a < masterAirports.size(); a++)
			{
				if (masterAirports[a] == airport) {
					masterAirports.erase(masterAirports.begin() + a);
					//Update server master list
					std::thread t99(&CDM::getCdmServerMasterAirports, this);
					t99.detach();
					return true;
				}
			}
		}
	}
	else {
		for (int a = 0; a < masterAirports.size(); a++)
		{
			if (masterAirports[a] == airport) {
				masterAirports.erase(masterAirports.begin() + a);
				addLogLine("Successfully removed master airport (Locally only) " + airport);
				sendMessage("Successfully removed master airport (Locally only) " + airport);
				return true;
			}
		}
	}
	return false;
}

bool CDM::removeAllMasterAirports(string position) {
	addLogLine("Called removeAllMasterAirports...");
	if (serverEnabled) {
		try {
			addLogLine("Call - Remove all masters for " + position);
			CURL* curl;
			CURLcode result = CURLE_FAILED_INIT;
			string readBuffer;
			long responseCode = 0;
			curl = curl_easy_init();
			if (curl) {
				string url = cdmServerUrl + "/airport/removeAllMasterByPosition?position=" + position;
				string apiKeyHeader = "x-api-key: " + apikey;
				struct curl_slist* headers = NULL;
				headers = curl_slist_append(headers, apiKeyHeader.c_str());
				curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
				curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
				curl_easy_setopt(curl, CURLOPT_POST, true);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
				curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
				curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
				curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
				result = curl_easy_perform(curl);
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
				curl_easy_cleanup(curl);
			}

			if (responseCode == 404 || responseCode == 401 || responseCode == 502 || CURLE_OK != result) {
				addLogLine("UNABLE TO CONNECT CDM-API...");
			}
			else {
				std::istringstream is(readBuffer);
				string lineValue;
				while (getline(is, lineValue))
				{
					if (lineValue == "true") {
						addLogLine("Successfully removed all master airports for " + position);
						sendMessage("Successfully removed all master airports for " + position);
						masterAirports.clear();
						//Update server master list
						std::thread t99(&CDM::getCdmServerMasterAirports, this);
						t99.detach();
						return true;
					}
				}
			}
			return false;
		}
		catch (const std::exception& e) {
			addLogLine("ERROR: Unhandled exception removeAllMasterAirports: " + (string)e.what());
			return false;
		}
		catch (...) {
			addLogLine("ERROR: Unhandled exception removeAllMasterAirports");
			return false;
		}
	}
	else {
		addLogLine("Successfully removed all master airports for " + position);
		sendMessage("Successfully removed all master airports for " + position);
		masterAirports.clear();
		return true;
	}
}

void CDM::removeAllMasterAirportsByAirport(string airport) {
	if (serverEnabled) {
		addLogLine("Called removeAllMasterAirportsByAirport...");
		try {
			addLogLine("Call - Remove all masters from " + airport);
			CURL* curl;
			CURLcode result = CURLE_FAILED_INIT;
			string readBuffer;
			long responseCode = 0;
			curl = curl_easy_init();
			if (curl) {
				string url = cdmServerUrl + "/airport/removeAllMasterByAirport?airport=" + airport;
				string apiKeyHeader = "x-api-key: " + apikey;
				struct curl_slist* headers = NULL;
				headers = curl_slist_append(headers, apiKeyHeader.c_str());
				curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
				curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
				curl_easy_setopt(curl, CURLOPT_POST, true);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
				curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
				curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
				curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
				result = curl_easy_perform(curl);
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
				curl_easy_cleanup(curl);
			}

			if (responseCode == 404 || responseCode == 401 || responseCode == 502 || CURLE_OK != result) {
				addLogLine("UNABLE TO CONNECT CDM-API...");
			}
			else {
				addLogLine("Removed masters for airport " + airport);
				//Update server master list
				std::thread t99(&CDM::getCdmServerMasterAirports, this);
				t99.detach();
			}
		}
		catch (const std::exception& e) {
			addLogLine("ERROR: Unhandled exception removeAllMasterAirportsByAirport: " + (string)e.what());
		}
		catch (...) {
			addLogLine("ERROR: Unhandled exception removeAllMasterAirportsByAirport");
		}
	}
}

bool CDM::setEvCtot(string callsign) {
	if (serverEnabled) {
		addLogLine("Called setEvCtot...");
		try {
			addLogLine("Call - Set Event CTOT for " + callsign);
			CURL* curl;
			CURLcode result = CURLE_FAILED_INIT;
			string readBuffer;
			long responseCode = 0;
			curl = curl_easy_init();
			if (curl) {
				string url = cdmServerUrl + "/ifps/cidCheck?callsign=" + callsign;
				string apiKeyHeader = "x-api-key: " + apikey;
				struct curl_slist* headers = NULL;
				headers = curl_slist_append(headers, apiKeyHeader.c_str());
				curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
				curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
				curl_easy_setopt(curl, CURLOPT_HTTPGET, true);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
				curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
				curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
				curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
				result = curl_easy_perform(curl);
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
				curl_easy_cleanup(curl);
			}

			if (responseCode == 404 || responseCode == 401 || responseCode == 502 || CURLE_OK != result) {
				std::lock_guard<std::mutex> lock(later3Mutex);
				checkCIDLater.push_back(callsign);
				addLogLine("UNABLE TO CONNECT CDM-API...");
			}
			else {
				std::istringstream is(readBuffer);
				//Get data from .txt file
				string cid = "";
				while (getline(is, cid))
				{
					if (cid.length() > 4) {
						for (int i = 0; i < slotFile.size(); i++) {
							if (slotFile[i].size() > 1) {
								if (slotFile[i][0] == cid) {
									addLogLine(callsign + " linked with EvCTOT " + slotFile[i][1]);
									sendMessage(callsign + " linked with EvCTOT " + slotFile[i][1]);
									for (int a = 0; a < evCtots.size(); a++) {
										if (evCtots[a].size() > 0) {
											if (evCtots[a][0] == callsign) {
												evCtots[a] = { callsign, slotFile[i][1] };
												return true;
											};
										}
									}
								}
							}
						}
					}
				}
				if (cid == "") {
					std::lock_guard<std::mutex> lock(later3Mutex);
					checkCIDLater.push_back(callsign);
				}
			}
			return false;
		}
		catch (const std::exception& e) {
			std::lock_guard<std::mutex> lock(later3Mutex);
			checkCIDLater.push_back(callsign);
			addLogLine("ERROR: Unhandled exception setEvCtot: " + (string)e.what());
			return false;
		}
		catch (...) {
			std::lock_guard<std::mutex> lock(later3Mutex);
			checkCIDLater.push_back(callsign);
			addLogLine("ERROR: Unhandled exception setEvCtot");
			return false;
		}
	}
	return false;
}

void CDM::getCdmServerRestricted(vector<Plane> slotListTemp) {
	if (serverEnabled) {
		addLogLine("Called getCdmServerRestricted...");
		try {
			vector<ServerRestricted> serverRestrictedPlanesTemp;
			vector<Plane> initialslotListTemp = slotListTemp;
			addLogLine("Call - Fetching CTOTs");
			//sendMessage("Fetching CTOTs...");
			CURL* curl;
			CURLcode result = CURLE_FAILED_INIT;
			std::string readBuffer;
			long responseCode = 0;
			curl = curl_easy_init();
			if (curl) {
				string url = cdmServerUrl + "/etfms/restricted";
				if (customRestrictedUrl != "") url = customRestrictedUrl;
				string apiKeyHeader = "x-api-key: " + apikey;
				struct curl_slist* headers = NULL;
				headers = curl_slist_append(headers, apiKeyHeader.c_str());
				curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
				curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
				curl_easy_setopt(curl, CURLOPT_HTTPGET, true);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
				curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
				curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
				curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
				result = curl_easy_perform(curl);
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
				curl_easy_cleanup(curl);
			}

			if (responseCode == 404 || responseCode == 401 || responseCode == 502 || CURLE_OK != result) {
				// handle error 404
				addLogLine("UNABLE TO LOAD CDM-API URL...");
			}
			else {
				Json::Reader reader;
				Json::Value obj;
				Json::FastWriter fastWriter;
				reader.parse(readBuffer, obj);


				//Reset all CTOTs
				for (size_t i = 0; i < slotListTemp.size(); i++) {
					if (slotListTemp[i].ctot != "") {
						slotListTemp[i].hasManualCtot = false;
					}
					slotListTemp[i].ctot = "";
					if (!slotListTemp[i].hasEcfmpRestriction) {
						slotListTemp[i].flowReason = "";
					}
				}

				serverRestrictedPlanesTemp.clear();

				const Json::Value& restricted = obj;
				for (size_t i = 0; i < restricted.size(); i++) {
					if (restricted[i].isMember("callsign") && restricted[i].isMember("ctot") &&  restricted[i].isMember("mostPenalisingRegulation")) {
						//Get callsign 
						string callsign = fastWriter.write(restricted[i]["callsign"]);
						callsign.erase(std::remove(callsign.begin(), callsign.end(), '"'));
						callsign.erase(std::remove(callsign.begin(), callsign.end(), '\n'));
						callsign.erase(std::remove(callsign.begin(), callsign.end(), '\n'));

						//Get CTOT
						string ctot = fastWriter.write(restricted[i]["ctot"]);
						ctot.erase(std::remove(ctot.begin(), ctot.end(), '"'));
						ctot.erase(std::remove(ctot.begin(), ctot.end(), '\n'));
						ctot.erase(std::remove(ctot.begin(), ctot.end(), '\n'));

						//Get reason
						string reason = fastWriter.write(restricted[i]["mostPenalisingRegulation"]);
						reason.erase(std::remove(reason.begin(), reason.end(), '"'));
						reason.erase(std::remove(reason.begin(), reason.end(), '\n'));
						reason.erase(std::remove(reason.begin(), reason.end(), '\n'));

						serverRestrictedPlanesTemp.push_back({ callsign,ctot,reason });

						if (ctot.size() == 4) {
							for (size_t z = 0; z < slotListTemp.size(); z++) {
								if (slotListTemp[z].callsign == callsign && !flightHasCtotDisabled(callsign) && !slotListTemp[z].hasEcfmpRestriction) {
									slotListTemp[z] = {
										callsign,
										slotListTemp[z].eobt,
										slotListTemp[z].tsat,
										slotListTemp[z].ttot,
										ctot,
										reason,
										slotListTemp[z].ecfmpRestriction,
										slotListTemp[z].hasEcfmpRestriction,
										true,
										true,
										true
									};
								}
							}
						}
					}
				}
				serverRestrictedPlanes = serverRestrictedPlanesTemp;
				sendWaitingTOBT();
				sendWaitingCdmSts();
				sendCheckCIDLater();
				sendWaitingCdmData();
				std::vector<Plane> toAdd;
				for (Plane p : slotListTemp) {
					for (int d = 0; d < initialslotListTemp.size(); d++) {
						if (p.callsign == initialslotListTemp[d].callsign) {
							if (initialslotListTemp[d].ctot != p.ctot || initialslotListTemp[d].flowReason != p.flowReason || initialslotListTemp[d].hasManualCtot != p.hasManualCtot) {
								{
									toAdd.push_back(p);
								}
							}
						}
					}
				}
				if (!toAdd.empty()) {
					std::lock_guard<std::mutex> lock(apiQueueResponseMutex);
					apiQueueResponse.insert(apiQueueResponse.end(), toAdd.begin(), toAdd.end());
				}
			}
			addLogLine("COMPLETED - Fetching CTOTs");
		}
		catch (const std::exception& e) {
			addLogLine("ERROR: Unhandled exception Fetching CTOTs: " + (string)e.what());
		}
		catch (...) {
			addLogLine("ERROR: Unhandled exception Fetching CTOTs");
		}
	}
}

void CDM::sendWaitingTOBT() {
	try {
		vector<Plane> setOBTlaterTemp;
		{
			addLogLine("Call sendWaitingTOBT - " + to_string(setOBTlater.size()));
			addLogLine("Called sendWaitingTOBT...");
			std::lock_guard<std::mutex> lock(later1Mutex);
			setOBTlaterTemp = setOBTlater;
			setOBTlater.clear();
		}

		vector<Plane> alreadyProcessed;
		bool found = false;

		for (int i = 0; i < setOBTlaterTemp.size(); i++) {
			found = false;
			for (Plane p : alreadyProcessed) {
				if (p.callsign == setOBTlaterTemp[i].callsign) {
					found = true;
				}
			}

			if (!found) {
				alreadyProcessed.push_back(setOBTlaterTemp[i]);
				addLogLine("sendWaitingTOBT - " + setOBTlaterTemp[i].callsign);
				if (serverEnabled) {
					setOBTApi(
					setOBTlaterTemp[i].callsign,
					setOBTlaterTemp[i].tsat,
					setOBTlaterTemp[i].showData, /*Used as manualTrigger*/
					setOBTlaterTemp[i].isCdmAirport /*Used as useEobt*/
					);
				}
			}
		}
	}
	catch (const std::exception& e) {
		addLogLine("ERROR: Unhandled exception sendWaitingTOBT: " + (string)e.what());
	}
	catch (...) {
		addLogLine("Error occurred parsing data from the cdm-api (sendWaitingTOBT)");
	}
}

void CDM::sendWaitingCdmSts() {
	vector<vector<string>> callsignsToProcess;
	{
		addLogLine("Call sendWaitingCdmSts - " + to_string(setCdmStslater.size()));
		std::lock_guard<std::mutex> lock(later2Mutex);
		callsignsToProcess = setCdmStslater;
		setCdmStslater.clear();
	}

	for (int i = 0; i < callsignsToProcess.size(); i++) {
		addLogLine("sendWaitingCdmSts - " + callsignsToProcess[i][0]);
		setCdmSts(callsignsToProcess[i][0], callsignsToProcess[i][1]);
	}
}

void CDM::sendWaitingCdmData() {
	vector<Plane> callsignsToProcess;
	{
		addLogLine("Call sendWaitingCdmData - " + to_string(setCdmDatalater.size()));
		std::lock_guard<std::mutex> lock(later4Mutex);
		callsignsToProcess = setCdmDatalater;
		setCdmDatalater.clear();
	}

	for (const Plane& plane : callsignsToProcess) {
		addLogLine("sendWaitingCdmData - " + plane.callsign);
		updateCdmDataApi(plane);
	}
}

void CDM::sendCheckCIDLater() {
	vector<string> callsignsToProcess;
	{
		addLogLine("Call sendCheckCIDLater - " + to_string(checkCIDLater.size()));
		std::lock_guard<std::mutex> lock(later3Mutex);
		callsignsToProcess = checkCIDLater;
		checkCIDLater.clear();
	}

	for (const string& callsign : callsignsToProcess) {
		addLogLine("sendCheckCIDLater - " + callsign);
		setEvCtot(callsign);
	}
}

	void CDM::updateCdmDataApi(Plane p) {
		if (serverEnabled) {
			addLogLine("Called updateCdmDataApi...");
			try {
				CURL* curl;
				CURLcode result = CURLE_FAILED_INIT;
				string readBuffer;
				long responseCode = 0;
				string str;
				CFlightPlan fp = FlightPlanSelect(p.callsign.c_str());
				if (p.hasManualCtot && p.ctot != "" && p.ttot.length() >= 4) {
					str = "callsign=" + p.callsign + "&tobt=" + p.eobt + "&tsat=" + p.tsat + "&ttot=" + p.ttot + "&ctot=" + p.ctot.substr(0, 4) + "&reason=" + p.flowReason;
					if (fp.IsValid()) {
						str += "&asrt=" + getFlightStripInfo(fp, 0) + "&depInfo=" + fp.GetFlightPlanData().GetDepartureRwy() + "/" + fp.GetFlightPlanData().GetSidName();
					}
				}
				else if (p.hasManualCtot && p.ttot.length() >= 4) {
					str = "callsign=" + p.callsign + "&tobt=" + p.eobt + "&tsat=" + p.tsat + "&ttot=" + p.ttot + "&ctot=" + p.ttot.substr(0, 4) + "&reason=MANUAL";
					if (fp.IsValid()) {
						str += "&asrt=" + getFlightStripInfo(fp, 0) + "&depInfo=" + fp.GetFlightPlanData().GetDepartureRwy() + "/" + fp.GetFlightPlanData().GetSidName();
					}
				}
				else {
					str = "callsign=" + p.callsign + "&tobt=" + p.eobt + "&tsat=" + p.tsat + "&ttot=" + p.ttot + "&ctot=&reason=";
					if (fp.IsValid()) {
						str += "&asrt=" + getFlightStripInfo(fp, 0) + "&depInfo=" + fp.GetFlightPlanData().GetDepartureRwy() + "/" + fp.GetFlightPlanData().GetSidName();
					}
				}
				result = CURLE_FAILED_INIT;
				curl = curl_easy_init();
				if (curl) {											
					string url = cdmServerUrl + "/ifps/setCdmData?" + str;
					string apiKeyHeader = "x-api-key: " + apikey;
					struct curl_slist* headers = curl_slist_append(NULL, apiKeyHeader.c_str());
					curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
					curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
					curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
					curl_easy_setopt(curl, CURLOPT_POST, 1L);
					curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
					curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
					curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
					curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
					result = curl_easy_perform(curl);
					curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
					curl_easy_cleanup(curl);
				}

				if (responseCode == 404 || responseCode == 401 || responseCode == 502 || CURLE_OK != result) {
					std::lock_guard<std::mutex> lock(later4Mutex);
					setCdmDatalater.push_back(p);
					addLogLine("UNABLE TO CONNECT CDM-API...");
				}
				else {
					std::istringstream is(readBuffer);
					//Get data from .txt file
					string lineValue;
					while (getline(is, lineValue))
					{
						if (lineValue != "true") {
							addLogLine("setCdmData RESPONSE: " + lineValue);
							std::lock_guard<std::mutex> lock(later4Mutex);
							setCdmDatalater.push_back(p);
						}
					}
				}
				addLogLine("COMPLETED - updateCdmDataApi");
			}
			catch (const std::exception& e) {
				std::lock_guard<std::mutex> lock(later4Mutex);
				setCdmDatalater.push_back(p);
				addLogLine("ERROR: Unhandled exception updateCdmDataApi: " + (string)e.what());
			}
			catch (...) {
				std::lock_guard<std::mutex> lock(later4Mutex);
				setCdmDatalater.push_back(p);
				addLogLine("ERROR: Unhandled exception updateCdmDataApi");
			}
		}
	}

void CDM::setOBTApi(string callsign, string obt, bool triggeredByUser, bool useEobt) {
		addLogLine("Called setOBTApi...");
		try {
			vector<Plane> slotListTemp; // Local copy of the slotList
			{
				slotListTemp = slotList; // Copy the slotList
			}

			addLogLine("Call - Set OBT (" + obt + ") for " + callsign + " with triggeredByUser=" + (triggeredByUser ? "true" : "false") + " and useEobt=" + (useEobt ? "true" : "false"));
			bool createRequest = false;

			if (obt != "") {
				for (Plane p : slotListTemp) {
					if (p.callsign == callsign) {
						//Only create request if TOBT is manually triggered (or initially triggered or when no ctot), to avoid update set TSAT when syncing from CTOT
						if ((p.ctot != "" && triggeredByUser) || p.ctot == "") {
							createRequest = true;
							break;
						}
						else {
							createRequest = false;
							break;
						}
					}
				}
			}
			else {
				createRequest = true;
			}

			if (isFligthSusp(callsign)) createRequest = false;

			if (createRequest) {
				obt = (obt.length() >= 4) ? obt.substr(0, 4) : "";
				string taxiTime = getTaxiTime(callsign);

				CURL* curl;
				CURLcode result = CURLE_FAILED_INIT;
				string readBuffer;
				long responseCode = 0;
				curl = curl_easy_init();

				if (curl) {
					addLogLine("Requesting OBT (" + obt + ") for " + callsign);
					string url = cdmServerUrl + "/ifps/dpi?callsign=" + callsign + "&value=OBT/" + obt + "/" + taxiTime;
					if (useEobt) url = cdmServerUrl + "/ifps/dpi?callsign=" + callsign + "&value=EOBT/" + obt;
					string apiKeyHeader = "x-api-key: " + apikey;
					struct curl_slist* headers = curl_slist_append(NULL, apiKeyHeader.c_str());
					curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
					curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
					curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
					curl_easy_setopt(curl, CURLOPT_POST, 1L);
					curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
					curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
					curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
					curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
					result = curl_easy_perform(curl);
					curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
					curl_easy_cleanup(curl);
				}

				if (responseCode == 404 || responseCode == 401 || responseCode == 502 || CURLE_OK != result) {
					Plane plane(callsign, "", obt, "", "", "", EcfmpRestriction(), false, false, triggeredByUser, useEobt);
					{
						std::lock_guard<std::mutex> lock(later1Mutex);
						setOBTlater.push_back(plane); // Safely modify setOBTlater
					}
					addLogLine("UNABLE TO CONNECT CDM-API...");
				}
				else {
					Json::Reader reader;
					Json::Value obj;
					Json::FastWriter fastWriter;
					reader.parse(readBuffer, obj);
					if (obj.isMember("callsign") && obj.isMember("ctot") && obj.isMember("atfcmData") && obj["atfcmData"].isMember("mostPenalisingRegulation")) {
						string apiCallsign = fastWriter.write(obj["callsign"]);
						apiCallsign.erase(remove(apiCallsign.begin(), apiCallsign.end(), '"'), apiCallsign.end());
						apiCallsign.erase(remove(apiCallsign.begin(), apiCallsign.end(), '\n'), apiCallsign.end());

						string ctot = fastWriter.write(obj["ctot"]);
						ctot.erase(remove(ctot.begin(), ctot.end(), '"'), ctot.end());
						ctot.erase(remove(ctot.begin(), ctot.end(), '\n'), ctot.end());

						string reason = fastWriter.write(obj["atfcmData"]["mostPenalisingRegulation"]);
						reason.erase(remove(reason.begin(), reason.end(), '"'), reason.end());
						reason.erase(remove(reason.begin(), reason.end(), '\n'), reason.end());

						for (size_t i = 0; i < slotListTemp.size(); i++) {
							if (slotListTemp[i].callsign == apiCallsign) {
								addLogLine(apiCallsign + " returned with CTOT: [" + ctot + "] and reason: [" + reason + "]");
								if (!ctot.empty() && !flightHasCtotDisabled(apiCallsign)) {
									// Update with thread-safe access
									{
										slotListTemp[i] = {
											apiCallsign,
											slotListTemp[i].eobt,
											slotListTemp[i].tsat,
											slotListTemp[i].ttot,
											ctot,
											reason,
											slotListTemp[i].ecfmpRestriction,
											slotListTemp[i].hasEcfmpRestriction,
											true,
											true,
											true
										};
									}
								}
								else {
									if (slotListTemp[i].ctot != "") {
										// Reset CTOT
										{
											slotListTemp[i].ctot = "";
											slotListTemp[i].flowReason = "";
											slotListTemp[i].hasManualCtot = false;
											slotListTemp[i].showData = true;
										}
									}
								}
							}
						}
					}
					else {
						Plane plane(callsign, "", obt, "", "", "", EcfmpRestriction(), false, false, triggeredByUser, useEobt);
						{
							std::lock_guard<std::mutex> lock(later1Mutex);
							setOBTlater.push_back(plane);
						}
					}
				}
			}

			// Add to queue
			std::vector<Plane> toAdd;
			for (Plane p : slotListTemp) {
				for (int d = 0; d < slotList.size(); d++) {
					if (p.callsign == slotList[d].callsign) {
						p.showData = true;
						toAdd.push_back(p);
					}
				}
			}

			if (!toAdd.empty()) {
				std::lock_guard<std::mutex> lock(apiQueueResponseMutex);
				apiQueueResponse.insert(apiQueueResponse.end(), toAdd.begin(), toAdd.end());
			}

			addLogLine("COMPLETED - setOBTApi for " + callsign);
		}
		catch (const std::exception& e) {
			addLogLine("ERROR: Unhandled exception setOBTApi: " + (string)e.what());
			{
				for (size_t a = 0; a < slotList.size(); a++) {
					if (slotList[a].callsign == callsign) {
						slotList[a].showData = true;
					}
				}
			}
		}
		catch (...) {
			addLogLine("ERROR: Unhandled exception setOBTApi");
			{
				for (size_t a = 0; a < slotList.size(); a++) {
					if (slotList[a].callsign == callsign) {
						slotList[a].showData = true;
					}
				}
			}
		}
}

string CDM::getTaxiTime(string callsign) {
	addLogLine("Call - getTaxiTime");
	string taxiTime = "15";
	for (size_t j = 0; j < taxiTimesList.size(); j++)
	{
		if (taxiTimesList[j].substr(0, taxiTimesList[j].find(",")) == callsign) {
			if (taxiTimesList[j].substr(taxiTimesList[j].length() - 2, 1) == ",") {
				taxiTime = taxiTimesList[j].substr(taxiTimesList[j].length() - 1, 1);
			}
			else {
				taxiTime = taxiTimesList[j].substr(taxiTimesList[j].length() - 2, 2);
			}
		}
	}
	addLogLine("getTaxiTime: " + taxiTime);
	return taxiTime;
}

void CDM::setCdmSts(string callsign, string cdmSts) {
	if (serverEnabled) {
		addLogLine("Called setCdmSts...");
		try {
			addLogLine("Call - Set DPI:" + cdmSts + " - for " + callsign);
			CURL* curl;
			CURLcode result = CURLE_FAILED_INIT;
			string readBuffer;
			long responseCode = 0;
			curl = curl_easy_init();
			if (curl) {
				string url = cdmServerUrl + "/ifps/dpi?callsign=" + callsign + "&value=" + cdmSts;
				string apiKeyHeader = "x-api-key: " + apikey;
				struct curl_slist* headers = NULL;
				headers = curl_slist_append(headers, apiKeyHeader.c_str());
				curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
				curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
				curl_easy_setopt(curl, CURLOPT_POST, 1L);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
				curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
				curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
				curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
				result = curl_easy_perform(curl);
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
				curl_easy_cleanup(curl);
			}

			if (responseCode == 404 || responseCode == 401 || responseCode == 502 || CURLE_OK != result) {
				std::lock_guard<std::mutex> lock(later2Mutex);
				setCdmStslater.push_back({ callsign, cdmSts });
				addLogLine("UNABLE TO CONNECT CDM-API...");
			}
			else {
				std::istringstream is(readBuffer);
				string lineValue;
				while (getline(is, lineValue))
				{
					if (lineValue != "true") {
						std::lock_guard<std::mutex> lock(later2Mutex);
						addLogLine("setCdmSts: true not received. Retrying later... Received: " + lineValue);
						setCdmStslater.push_back({ callsign, cdmSts });
					}
				}
			}
			std::thread t59(&CDM::getCdmServerStatus, this);
			t59.detach();

			addLogLine("COMPLETED setCdmSts...");
		}
		catch (const std::exception& e) {
			std::lock_guard<std::mutex> lock(later2Mutex);
			setCdmStslater.push_back({ callsign, cdmSts });
			addLogLine("ERROR: Unhandled exception setCdmSts: " + (string)e.what());
		}
		catch (...) {
			std::lock_guard<std::mutex> lock(later2Mutex);
			setCdmStslater.push_back({ callsign, cdmSts });
			addLogLine("ERROR: Unhandled exception setCdmSts");
		}
	}
}

bool CDM::isFligthSusp(string callsign) {
	addLogLine("Call - isFligthSusp");
	bool outOfTsat = false;
	for (size_t i = 0; i < OutOfTsat.size(); i++)
	{
		if (callsign == OutOfTsat[i][0]) {
			outOfTsat = true;
		}
	}

	if (outOfTsat) {
		addLogLine("Flight is SUSP");
		return true;
	}
	addLogLine("Flight not SUSP: ");
	return false;
}

void CDM::getCdmServerStatus() {
	if (serverEnabled) {
		addLogLine("Called getCdmServerStatus...");
		try {
			vector<vector<string>> networkStatusTemp;
			CURL* curl;
			CURLcode result = CURLE_FAILED_INIT;
			std::string readBuffer;
			long responseCode = 0;
			curl = curl_easy_init();
			if (curl) {
				string url = cdmServerUrl + "/ifps/allStatus";
				string apiKeyHeader = "x-api-key: " + apikey;
				struct curl_slist* headers = NULL;
				headers = curl_slist_append(headers, apiKeyHeader.c_str());
				curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
				curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
				curl_easy_setopt(curl, CURLOPT_HTTPGET, true);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
				curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
				curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
				curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
				result = curl_easy_perform(curl);
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
				curl_easy_cleanup(curl);
			}

			if (responseCode == 404 || responseCode == 401 || responseCode == 502 || CURLE_OK != result) {
				// handle error 404
				addLogLine("UNABLE TO LOAD CDM-API URL...");
			}
			else {
				Json::Reader reader;
				Json::Value obj;
				Json::FastWriter fastWriter;
				reader.parse(readBuffer, obj);

				networkStatusTemp.clear();

				const Json::Value& data = obj;
				for (size_t i = 0; i < data.size(); i++) {
					if (data[i].isMember("callsign") && data[i].isMember("cdmSts")) {
						//Get callsign 
						string callsign = fastWriter.write(data[i]["callsign"]);
						callsign.erase(std::remove(callsign.begin(), callsign.end(), '"'));
						callsign.erase(std::remove(callsign.begin(), callsign.end(), '\n'));
						callsign.erase(std::remove(callsign.begin(), callsign.end(), '\n'));

						//Get CTOT
						string cdmSts = fastWriter.write(data[i]["cdmSts"]);
						cdmSts.erase(std::remove(cdmSts.begin(), cdmSts.end(), '"'));
						cdmSts.erase(std::remove(cdmSts.begin(), cdmSts.end(), '\n'));
						cdmSts.erase(std::remove(cdmSts.begin(), cdmSts.end(), '\n'));

						//Only keep sts if not affected by ecfmp restriction
						bool hasEcfmpRestriction = false;
						for (int i = 0; i < slotList.size(); i++)
						{
							if (slotList[i].callsign == callsign && slotList[i].hasEcfmpRestriction) {
								hasEcfmpRestriction = true;
							}
						}
						if (!hasEcfmpRestriction) {
							networkStatusTemp.push_back({ callsign, cdmSts });
						}
					}
				}
			}
			{
				std::lock_guard<std::mutex> lock(networkStatusMutex);
				networkStatus = std::move(networkStatusTemp);
			}
			addLogLine("COMPLETED - getCdmServerStatus");
		}
		catch (const std::exception& e) {
			addLogLine("ERROR: Unhandled exception getCdmServerStatus: " + (string)e.what());
		}
		catch (...) {
			addLogLine("ERROR: Unhandled exception getCdmServerStatus");
		}
	}
}

void CDM::getCdmServerOnTime() {
	if (serverEnabled) {
		addLogLine("Called getCdmServerOnTime...");
		try {
			vector<vector<string>> onTimeStatusTemp;
			CURL* curl;
			CURLcode result = CURLE_FAILED_INIT;
			std::string readBuffer;
			long responseCode = 0;
			curl = curl_easy_init();
			if (curl) {
				string url = cdmServerUrl + "/ifps/allOnTime";
				string apiKeyHeader = "x-api-key: " + apikey;
				struct curl_slist* headers = NULL;
				headers = curl_slist_append(headers, apiKeyHeader.c_str());
				curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
				curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
				curl_easy_setopt(curl, CURLOPT_HTTPGET, true);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
				curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
				curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
				curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
				result = curl_easy_perform(curl);
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
				curl_easy_cleanup(curl);
			}

			if (responseCode == 404 || responseCode == 401 || responseCode == 502 || CURLE_OK != result) {
				// handle error 404
				addLogLine("UNABLE TO LOAD CDM-API URL...");
			}
			else {
				Json::Reader reader;
				Json::Value obj;
				Json::FastWriter fastWriter;
				reader.parse(readBuffer, obj);

				onTimeStatusTemp.clear();

				const Json::Value& data = obj;
				for (size_t i = 0; i < data.size(); i++) {
					if (data[i].isMember("callsign") && data[i].isMember("onTime")) {
						//Get callsign 
						string callsign = fastWriter.write(data[i]["callsign"]);
						callsign.erase(std::remove(callsign.begin(), callsign.end(), '"'));
						callsign.erase(std::remove(callsign.begin(), callsign.end(), '\n'));
						callsign.erase(std::remove(callsign.begin(), callsign.end(), '\n'));

						//Get CTOT
						string onTime = fastWriter.write(data[i]["onTime"]);
						onTime.erase(std::remove(onTime.begin(), onTime.end(), '"'));
						onTime.erase(std::remove(onTime.begin(), onTime.end(), '\n'));
						onTime.erase(std::remove(onTime.begin(), onTime.end(), '\n'));

						//Only keep sts if not affected by ecfmp restriction
						bool hasEcfmpRestriction = false;
						for (int i = 0; i < slotList.size(); i++)
						{
							if (slotList[i].callsign == callsign && slotList[i].hasEcfmpRestriction) {
								hasEcfmpRestriction = true;
							}
						}
						if (!hasEcfmpRestriction) {
							onTimeStatusTemp.push_back({ callsign, onTime });
						}
					}
				}
			}
			onTimeStatus = onTimeStatusTemp;
			addLogLine("COMPLETED - getCdmServerStatus");
		}
		catch (const std::exception& e) {
			addLogLine("ERROR: Unhandled exception getCdmServerStatus: " + (string)e.what());
		}
		catch (...) {
			addLogLine("ERROR: Unhandled exception getCdmServerStatus");
		}
	}
}

void CDM::getCdmServerMasterAirports() {
	if (serverEnabled) {
		addLogLine("Called getCdmServerMasterAirports...");
		try {
			vector<vector<string>> serverMasterAirportsTemp;
			CURL* curl;
			CURLcode result = CURLE_FAILED_INIT;
			std::string readBuffer;
			long responseCode = 0;
			curl = curl_easy_init();
			if (curl) {
				string url = cdmServerUrl + "/airport";
				string apiKeyHeader = "x-api-key: " + apikey;
				struct curl_slist* headers = NULL;
				headers = curl_slist_append(headers, apiKeyHeader.c_str());
				curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
				curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
				curl_easy_setopt(curl, CURLOPT_HTTPGET, true);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
				curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
				curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
				curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
				result = curl_easy_perform(curl);
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
				curl_easy_cleanup(curl);
			}

			if (responseCode == 404 || responseCode == 401 || responseCode == 502 || CURLE_OK != result) {
				// handle error 404
				addLogLine("UNABLE TO LOAD CDM-API URL...");
			}
			else {
				Json::Reader reader;
				Json::Value obj;
				Json::FastWriter fastWriter;
				reader.parse(readBuffer, obj);

				serverMasterAirportsTemp.clear();

				const Json::Value& data = obj;
				for (size_t i = 0; i < data.size(); i++) {
					if (data[i].isMember("icao") && data[i].isMember("position")) {
						//Get callsign 
						string icao = fastWriter.write(data[i]["icao"]);
						icao.erase(std::remove(icao.begin(), icao.end(), '"'));
						icao.erase(std::remove(icao.begin(), icao.end(), '\n'));
						icao.erase(std::remove(icao.begin(), icao.end(), '\n'));

						//Get CTOT
						string position = fastWriter.write(data[i]["position"]);
						position.erase(std::remove(position.begin(), position.end(), '"'));
						position.erase(std::remove(position.begin(), position.end(), '\n'));
						position.erase(std::remove(position.begin(), position.end(), '\n'));

						//Only keep sts if not affected by ecfmp restriction
						serverMasterAirportsTemp.push_back({ icao, position });
					}
				}
			}
			serverMasterAirports = serverMasterAirportsTemp;
			addLogLine("COMPLETED - getCdmServerMasterAirports");
		}
		catch (const std::exception& e) {
			addLogLine("ERROR: Unhandled exception getCdmServerMasterAirports: " + (string)e.what());
		}
		catch (...) {
			addLogLine("ERROR: Unhandled exception getCdmServerMasterAirports");
		}
	}
}


void CDM::getNetworkRates() {
	if (serverEnabled) {
		addLogLine("Called getNetworkRates...");
		try {
			vector<Rate> tempRate = initialRate;
			CURL* curl;
			CURLcode result = CURLE_FAILED_INIT;
			std::string readBuffer;
			long responseCode = 0;
			curl = curl_easy_init();
			if (curl) {
				string url = cdmServerUrl + "/etfms/restrictions?type=DEP";
				string apiKeyHeader = "x-api-key: " + apikey;
				struct curl_slist* headers = NULL;
				headers = curl_slist_append(headers, apiKeyHeader.c_str());
				curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
				curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
				curl_easy_setopt(curl, CURLOPT_HTTPGET, true);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
				curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
				curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
				curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
				result = curl_easy_perform(curl);
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
				curl_easy_cleanup(curl);
			}

			if (responseCode == 404 || responseCode == 401 || responseCode == 502 || CURLE_OK != result) {
				// handle error 404
				addLogLine("UNABLE TO LOAD CDM-API URL...");
			}
			else {
				Json::Reader reader;
				Json::Value obj;
				Json::FastWriter fastWriter;
				reader.parse(readBuffer, obj);

				const Json::Value& data = obj;
				for (size_t i = 0; i < data.size(); i++) {
					if (data[i].isMember("type") && data[i].isMember("airspace") && data[i].isMember("capacity") && data[i].isMember("runway")) {

						//Get airspace name
						string airspace = fastWriter.write(data[i]["airspace"]);
						airspace.erase(std::remove(airspace.begin(), airspace.end(), '"'));
						airspace.erase(std::remove(airspace.begin(), airspace.end(), '\n'));
						airspace.erase(std::remove(airspace.begin(), airspace.end(), '\n'));
						bool aptFound = false;
						for (string apt : masterAirports)
						{
							if (apt == airspace) {
								aptFound = true;
							}
						}
						if (aptFound) {
							//Get callsign 
							string type = fastWriter.write(data[i]["type"]);
							type.erase(std::remove(type.begin(), type.end(), '"'));
							type.erase(std::remove(type.begin(), type.end(), '\n'));
							type.erase(std::remove(type.begin(), type.end(), '\n'));

							if (type == "DEP") {
								//Get capacity
								string capacity = fastWriter.write(data[i]["capacity"]);
								capacity.erase(std::remove(capacity.begin(), capacity.end(), '"'));
								capacity.erase(std::remove(capacity.begin(), capacity.end(), '\n'));
								capacity.erase(std::remove(capacity.begin(), capacity.end(), '\n'));

								//Get runway
								string runway = fastWriter.write(data[i]["runway"]);
								runway.erase(std::remove(runway.begin(), runway.end(), '"'));
								runway.erase(std::remove(runway.begin(), runway.end(), '\n'));
								runway.erase(std::remove(runway.begin(), runway.end(), '\n'));

								for (int i = 0; i < tempRate.size(); i++) {
									if (tempRate[i].airport == airspace) {
										for (int s = 0; s < tempRate[i].depRwyYes.size(); s++) {
											if (tempRate[i].depRwyYes[s] == runway || tempRate[i].depRwyYes[s] == "*") {
												if (tempRate[i].rates.size() > 1) {
													tempRate[i].rates[s] = capacity;
												} else if (tempRate[i].rates.size() == 1) {
													tempRate[i].rates[0] = capacity;
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
			rate = tempRate;
			addLogLine("COMPLETED - getNetworkRates");
		}
		catch (const std::exception& e) {
			addLogLine("ERROR: Unhandled exception getNetworkRates: " + (string)e.what());
		}
		catch (...) {
			addLogLine("ERROR: Unhandled exception getNetworkRates");
		}
	}
}

vector<vector<string>> CDM::getDepAirportPlanes(string airport) {
	vector<vector<string>> planes;
	if (serverEnabled) {
		addLogLine("Called getDepAirportPlanes...");
		try {
			vector<Rate> tempRate = initialRate;
			CURL* curl;
			CURLcode result = CURLE_FAILED_INIT;
			std::string readBuffer;
			long responseCode = 0;
			curl = curl_easy_init();
			if (curl) {
				string url = cdmServerUrl + "/ifps/depAirport?airport=" + airport;
				string apiKeyHeader = "x-api-key: " + apikey;
				struct curl_slist* headers = NULL;
				headers = curl_slist_append(headers, apiKeyHeader.c_str());
				curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
				curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
				curl_easy_setopt(curl, CURLOPT_HTTPGET, true);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
				curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
				curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
				curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
				result = curl_easy_perform(curl);
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
				curl_easy_cleanup(curl);
			}

			if (responseCode == 404 || responseCode == 401 || responseCode == 502 || CURLE_OK != result) {
				// handle error 404
				addLogLine("UNABLE TO LOAD CDM-API URL...");
			}
			else {
				Json::Reader reader;
				Json::Value obj;
				Json::FastWriter fastWriter;
				reader.parse(readBuffer, obj);

				const Json::Value& data = obj;
				for (size_t i = 0; i < data.size(); i++) {
					if (data[i].isMember("cdmData") && data[i]["cdmData"].isMember("reqTobt") && data[i]["cdmData"].isMember("reqTobtType") && data[i]["cdmData"].isMember("reqAsrt") && data[i].isMember("callsign") && data[i].isMember("atot")) {

						string callsign = fastWriter.write(data[i]["callsign"]);
						callsign.erase(std::remove(callsign.begin(), callsign.end(), '"'));
						callsign.erase(std::remove(callsign.begin(), callsign.end(), '\n'));
						callsign.erase(std::remove(callsign.begin(), callsign.end(), '\n'));

						string tobt = fastWriter.write(data[i]["cdmData"]["reqTobt"]);
						tobt.erase(std::remove(tobt.begin(), tobt.end(), '"'));
						tobt.erase(std::remove(tobt.begin(), tobt.end(), '\n'));
						tobt.erase(std::remove(tobt.begin(), tobt.end(), '\n'));

						string type = fastWriter.write(data[i]["cdmData"]["reqTobtType"]);
						type.erase(std::remove(type.begin(), type.end(), '"'));
						type.erase(std::remove(type.begin(), type.end(), '\n'));
						type.erase(std::remove(type.begin(), type.end(), '\n'));

						string asrt = fastWriter.write(data[i]["cdmData"]["reqAsrt"]);
						asrt.erase(std::remove(asrt.begin(), asrt.end(), '"'));
						asrt.erase(std::remove(asrt.begin(), asrt.end(), '\n'));
						asrt.erase(std::remove(asrt.begin(), asrt.end(), '\n'));

						string atot = fastWriter.write(data[i]["atot"]);
						atot.erase(std::remove(atot.begin(), atot.end(), '"'));
						atot.erase(std::remove(atot.begin(), atot.end(), '\n'));
						atot.erase(std::remove(atot.begin(), atot.end(), '\n'));

						if (atot == "") {
							planes.push_back({ callsign, tobt, type, asrt });
						}
					}
				}
			}
		}
		catch (const std::exception& e) {
			addLogLine("ERROR: Unhandled exception getDepAirportPlanes: " + (string)e.what());
		}
		catch (...) {
			addLogLine("ERROR: Unhandled exception getDepAirportPlanes");
		}
	}
	return planes;
}

void CDM::getIffOffBlockTimes() {
	vector<vector<string>> myObts;
	if (serverEnabled) {
		vector<Plane> mySlotList = slotList;
		vector<string> airports;
		for (int i = 0; i < mySlotList.size(); i++) {
			CFlightPlan fp1 = FlightPlanSelect(mySlotList[i].callsign.c_str());
			if (fp1.IsValid()) {
				string depAirport = fp1.GetFlightPlanData().GetOrigin();
				if (find(CDMairports.begin(), CDMairports.end(), depAirport) == CDMairports.end()) airports.push_back(depAirport);
			}
		}
		sort(airports.begin(), airports.end());
		airports.erase(unique(airports.begin(), airports.end()), airports.end());

		addLogLine("Called getIffOffBlockTimes...");
		try {
			for (string apt : airports)
			{
				CURL* curl;
				CURLcode result = CURLE_FAILED_INIT;
				std::string readBuffer;
				long responseCode = 0;
				curl = curl_easy_init();
				if (curl) {
					string url = cdmServerUrl + "/ifps/depAirport?airport=" + apt;
					string apiKeyHeader = "x-api-key: " + apikey;
					struct curl_slist* headers = NULL;
					headers = curl_slist_append(headers, apiKeyHeader.c_str());
					curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
					curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
					curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
					curl_easy_setopt(curl, CURLOPT_HTTPGET, true);
					curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
					curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
					curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
					curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
					result = curl_easy_perform(curl);
					curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
					curl_easy_cleanup(curl);
				}

				if (responseCode == 404 || responseCode == 401 || responseCode == 502 || CURLE_OK != result) {
					// handle error 404
					addLogLine("UNABLE TO LOAD CDM-API URL...");
				}
				else {
					Json::Reader reader;
					Json::Value obj;
					Json::FastWriter fastWriter;
					reader.parse(readBuffer, obj);

					const Json::Value& data = obj;
					for (size_t i = 0; i < data.size(); i++) {
						if (data[i].isMember("obt") && data[i].isMember("callsign") && data[i].isMember("atot")) {

							string callsign = fastWriter.write(data[i]["callsign"]);
							callsign.erase(std::remove(callsign.begin(), callsign.end(), '"'));
							callsign.erase(std::remove(callsign.begin(), callsign.end(), '\n'));
							callsign.erase(std::remove(callsign.begin(), callsign.end(), '\n'));

							string obt = fastWriter.write(data[i]["obt"]);
							obt.erase(std::remove(obt.begin(), obt.end(), '"'));
							obt.erase(std::remove(obt.begin(), obt.end(), '\n'));
							obt.erase(std::remove(obt.begin(), obt.end(), '\n'));

							string atot = fastWriter.write(data[i]["atot"]);
							atot.erase(std::remove(atot.begin(), atot.end(), '"'));
							atot.erase(std::remove(atot.begin(), atot.end(), '\n'));
							atot.erase(std::remove(atot.begin(), atot.end(), '\n'));

							if (atot == "") {
								myObts.push_back({ callsign, obt });
							}
						}
					}
				}
			}
		}
		catch (const std::exception& e) {
			addLogLine("ERROR: Unhandled exception getIffOffBlockTimes: " + (string)e.what());
		}
		catch (...) {
			addLogLine("ERROR: Unhandled exception getIffOffBlockTimes");
		}
	}
	obtList = myObts;
}

void CDM::getNetworkTobt() {
	if (serverEnabled && pilotTobt) {
		addLogLine("Called getNetworkTobt...");
		vector<vector<string>> planes;
		for (string airport : masterAirports)
		{
			vector<vector<string>> newplanes = getDepAirportPlanes(airport);
			planes.insert(planes.end(), newplanes.begin(), newplanes.end());
		}

		vector<Plane> mySlotList = slotList;
		std::vector<vector<string>> toAdd;

		for (vector<string> plane : planes) {
			bool updated = false;
			if (plane.size() == 4) {
				if (plane[3] != "" && plane[0] != "") {
					CFlightPlan fp1 = FlightPlanSelect(plane[0].c_str());
					if (fp1.IsValid()) {
						//Update ASRT
						string prevAsrt = getFlightStripInfo(fp1, 0);
						if ((string)fp1.GetGroundState() != "STUP" && (string)fp1.GetGroundState() != "ST-UP" && (string)fp1.GetGroundState() != "PUSH" && (string)fp1.GetGroundState() != "TAXI" && (string)fp1.GetGroundState() != "DEPA") {
							addLogLine("Updating ASRT for: " + plane[0] + " Old: " + prevAsrt + " New: " + plane[3] + "00");
							setFlightStripInfo(fp1, plane[3], 0);
							setCdmSts(plane[0], "REQASRT/NULL");
						}
					}
				}
				if (plane[1] != "" && plane[0] != "") {
					bool found = false;
					for (int i = 0; i < mySlotList.size(); i++)
					{
						if (plane[0] == mySlotList[i].callsign) {
							if (!mySlotList[i].showData) {
								found = true;
							}
							//Check if not manual CTOT assigned
							else if ((!mySlotList[i].hasManualCtot && mySlotList[i].ctot == "") || (mySlotList[i].hasManualCtot && mySlotList[i].ctot != "")) {
								found = true;
								CFlightPlan fp = FlightPlanSelect(mySlotList[i].callsign.c_str());
								if (!fp.IsValid()) {
									continue;
								}

								//Update TOBT
								string annotAsrt = getFlightStripInfo(fp, 0);
								if (annotAsrt.empty() && (string)fp.GetGroundState() != "STUP" && (string)fp.GetGroundState() != "ST-UP" && (string)fp.GetGroundState() != "PUSH" && (string)fp.GetGroundState() != "TAXI" && (string)fp.GetGroundState() != "DEPA") {
									addLogLine("Updating TOBT for: " + mySlotList[i].callsign + " Old: " + mySlotList[i].eobt + " New: " + plane[1] + "00");
									/*int posPlane = getPlanePosition(mySlotList[i].callsign);
									if (posPlane != -1) {
										slotList.erase(slotList.begin() + posPlane);
									}*/
									setFlightStripInfo(fp, plane[1], 2);
									setCdmSts(plane[0], "REQTOBT/NULL/NULL");
									//Trigger TOBT update to update TAXI TIME
									//setOBTApi(plane[0], plane[1], true, false);
									updated = true;
								}
							}
						}
					}
					if (!found) {
						if (plane[1] != "") {
							addLogLine("Updating TOBT for: " + plane[0] + " Old: outdated New: " + plane[1] + "00");
							CFlightPlan fp = FlightPlanSelect(plane[0].c_str());
							if (!fp.IsValid()) {
								continue;
							}
							setFlightStripInfo(fp, plane[1], 2);
							//setCdmSts(plane[0], "REQTOBT/NULL/NULL");
							//Trigger TOBT update to update TAXI TIME
							setOBTApi(plane[0], plane[1], true, false);
							updated = true;
						}
					}

					if (plane[0] != "") {
						toAdd.push_back({ plane[0], plane[2]});
					}
				}
			}
		}

		if (!toAdd.empty()) {
			std::lock_guard<std::mutex> lock(reqTobtTypesQueueMutex);
			reqTobtTypesQueue.insert(reqTobtTypesQueue.end(), toAdd.begin(), toAdd.end());
		}
		addLogLine("COMPLETED - getNetworkTobt");
		//Update times to slaves
		countTime = std::time(nullptr) - refreshTime;
	}
}

vector<vector<string>> CDM::getAirportPlanesCdmDataSection(string airport) {
	vector<vector<string>> planes;
	if (serverEnabled) {
		addLogLine("Called getAirportPlanesCdmDataSection...");
		try {
			vector<Rate> tempRate = initialRate;
			CURL* curl;
			CURLcode result = CURLE_FAILED_INIT;
			std::string readBuffer;
			long responseCode = 0;
			curl = curl_easy_init();
			if (curl) {
				string url = cdmServerUrl + "/ifps/depAirport?airport=" + airport;
				string apiKeyHeader = "x-api-key: " + apikey;
				struct curl_slist* headers = NULL;
				headers = curl_slist_append(headers, apiKeyHeader.c_str());
				curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
				curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
				curl_easy_setopt(curl, CURLOPT_HTTPGET, true);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
				curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
				curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
				curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
				result = curl_easy_perform(curl);
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
				curl_easy_cleanup(curl);
			}

			if (responseCode == 404 || responseCode == 401 || responseCode == 502 || CURLE_OK != result) {
				// handle error 404
				addLogLine("UNABLE TO LOAD CDM-API URL...");
			}
			else {
				Json::Reader reader;
				Json::Value obj;
				Json::FastWriter fastWriter;
				reader.parse(readBuffer, obj);

				const Json::Value& data = obj;
				for (size_t i = 0; i < data.size(); i++) {
					if (data[i].isMember("cdmData") && data[i]["cdmData"].isMember("tobt") && data[i]["cdmData"].isMember("tsat") && data[i]["cdmData"].isMember("ttot")) {

						string callsign = fastWriter.write(data[i]["callsign"]);
						callsign.erase(std::remove(callsign.begin(), callsign.end(), '"'));
						callsign.erase(std::remove(callsign.begin(), callsign.end(), '\n'));
						callsign.erase(std::remove(callsign.begin(), callsign.end(), '\n'));

						string tobt = fastWriter.write(data[i]["cdmData"]["tobt"]);
						tobt.erase(std::remove(tobt.begin(), tobt.end(), '"'));
						tobt.erase(std::remove(tobt.begin(), tobt.end(), '\n'));
						tobt.erase(std::remove(tobt.begin(), tobt.end(), '\n'));

						string tsat = fastWriter.write(data[i]["cdmData"]["tsat"]);
						tsat.erase(std::remove(tsat.begin(), tsat.end(), '"'));
						tsat.erase(std::remove(tsat.begin(), tsat.end(), '\n'));
						tsat.erase(std::remove(tsat.begin(), tsat.end(), '\n'));

						string ttot = fastWriter.write(data[i]["cdmData"]["ttot"]);
						ttot.erase(std::remove(ttot.begin(), ttot.end(), '"'));
						ttot.erase(std::remove(ttot.begin(), ttot.end(), '\n'));
						ttot.erase(std::remove(ttot.begin(), ttot.end(), '\n'));

						if (ttot != "" && tobt != "" && tsat != "") {
							planes.push_back({ callsign, tobt, tsat, ttot });
						}
					}
				}
			}
		}
		catch (const std::exception& e) {
			addLogLine("ERROR: Unhandled exception getDepAirportPlanes: " + (string)e.what());
		}
		catch (...) {
			addLogLine("ERROR: Unhandled exception getDepAirportPlanes");
		}
	}
	return planes;
}

void CDM::copyServerSavedData(string airport) {
	if (serverEnabled) {
		addLogLine("Called copyServerSavedData...");
		for (string apt : masterAirports)
		{
			if (apt == airport) {
				sendMessage("Airport: " + airport + ". is already MASTER. Sync not possible.");
				return;
			}
		}

		/* [callisgn, tobt, tsat, ttot] */
		vector<vector<string>> newplanes = getAirportPlanesCdmDataSection(airport);

		vector<Plane> mySlotList = slotList;
		std::vector<vector<string>> toAdd;

		for (vector<string> newplane : newplanes) {
			bool updated = false;
			CFlightPlan fp = FlightPlanSelect(newplane[0].c_str());
			if (!fp.IsValid()) {
				continue;
			}
			for (Plane plane : slotList) {
				if (plane.callsign == newplane[0]) {
					updated = true;
					plane.eobt = newplane[1];
					plane.tsat = newplane[2];
					plane.ttot = newplane[3];
					setFlightStripInfo(fp, formatTime(plane.eobt), 2);
					setFlightStripInfo(fp, plane.tsat, 3);
					setFlightStripInfo(fp, plane.ttot, 4);
				}
			}
			if (!updated) {
				Plane plane = Plane(newplane[0], newplane[1], newplane[2], newplane[3], "", "", EcfmpRestriction(), false, false, false, true);
				setFlightStripInfo(fp, formatTime(plane.eobt), 2);
				setFlightStripInfo(fp, plane.tsat, 3);
				setFlightStripInfo(fp, plane.ttot, 4);
			}
		}
		addLogLine("COMPLETED - copyServerSavedData");
		//Update times to slaves
		countTime = std::time(nullptr) - refreshTime;
	}
}

bool CDM::addMasterAirport(string icao)
{
	try {
		if (icao.length() == 4) {
			string ATC_Position = ControllerMyself().GetCallsign();
			bool found = false;
			for (string apt : masterAirports)
			{
				if (apt == icao) {
					found = true;
				}
			}
			if (!found) {
				std::thread t(&CDM::setMasterAirport, this, icao, ATC_Position);
				t.detach();
			}
		}
		else {
			sendMessage("NO AIRPORT SET");
		}
		return true;
	}
	catch (const std::exception& e) {
		addLogLine("ERROR: Unhandled exception .cdm master: " + (string)e.what());
		return true;
	}
	catch (...) {
		addLogLine("ERROR: Unhandled exception .cdm master");
		return true;
	}
}

bool CDM::clearMasterAirport(string icao)
{
	try {
		if (icao.length() == 4) {
			string ATC_Position = ControllerMyself().GetCallsign();
			bool found = false;
			int a = 0;
			for (string apt : masterAirports)
			{
				if (apt == icao) {
					std::thread t(&CDM::removeMasterAirport, this, icao, ATC_Position);
					t.detach();
					found = true;
				}
				a++;
			}
			if (!found) {
				sendMessage("AIRPORT " + icao + " NOT FOUND");
			}
		}
		else {
			sendMessage("NO AIRPORT SET");
		}
		return true;
	}
	catch (const std::exception& e) {
		addLogLine("ERROR: Unhandled exception .cdm slave: " + (string)e.what());
		return true;
	}
	catch (...) {
		addLogLine("ERROR: Unhandled exception .cdm slave");
		return true;
	}
}

void CDM::getCdmServerRelevantFlights() {
	if (serverEnabled && showAtfcmList) {
		addLogLine("Called getCdmServerRelevantFlights...");
		try {
			vector<vector<string>> relevantFlightsTemp;
			string callsign = ControllerMyself().GetCallsign();
			if (callsign.length() > 4) {
				callsign = callsign.substr(0, 2);
			}
			else {
				//return;
			}
			CURL* curl;
			CURLcode result = CURLE_FAILED_INIT;
			std::string readBuffer;
			long responseCode = 0;
			curl = curl_easy_init();
			if (curl) {
				string url = cdmServerUrl + "/etfms/relevant" /* ? filter = " + callsign */;
				string apiKeyHeader = "x-api-key: " + apikey;
				struct curl_slist* headers = NULL;
				headers = curl_slist_append(headers, apiKeyHeader.c_str());
				curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
				curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
				curl_easy_setopt(curl, CURLOPT_HTTPGET, true);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
				curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
				curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
				curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
				result = curl_easy_perform(curl);
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
				curl_easy_cleanup(curl);
			}

			if (responseCode == 404 || responseCode == 401 || responseCode == 502 || CURLE_OK != result) {
				// handle error 404
				addLogLine("UNABLE TO LOAD CDM-API URL...");
			}
			else {
				Json::Reader reader;
				Json::Value obj;
				Json::FastWriter fastWriter;
				reader.parse(readBuffer, obj);

				relevantFlightsTemp.clear();

				const Json::Value& data = obj;
				for (size_t i = 0; i < data.size(); i++) {
					if (data[i].isMember("callsign") && data[i].isMember("cid") &&
						data[i].isMember("departure") && data[i].isMember("arrival") &&
						data[i].isMember("eobt") && data[i].isMember("atfcmStatus") &&
						data[i].isMember("tobt") && data[i].isMember("taxi") &&
						data[i].isMember("ctot") && data[i].isMember("aobt") &&
						data[i].isMember("atot") && data[i].isMember("eta") &&
						data[i].isMember("atfcmData") && data[i]["atfcmData"].isMember("mostPenalisingRegulation") &&
						data[i].isMember("informed") && data[i].isMember("isCdm")) {

						auto cleanString = [&](const Json::Value& val) -> std::string {
							std::string s = fastWriter.write(val);
							s.erase(std::remove(s.begin(), s.end(), '"'), s.end());
							s.erase(std::remove(s.begin(), s.end(), '\n'));
							s.erase(std::remove(s.begin(), s.end(), '\n'));
							return s;
							};

						// Extract all fields
						std::string callsign = cleanString(data[i]["callsign"]);
						std::string departure = cleanString(data[i]["departure"]);
						std::string arrival = cleanString(data[i]["arrival"]);
						std::string eobt = cleanString(data[i]["eobt"]);
						std::string tobt = cleanString(data[i]["tobt"]);
						std::string taxi = cleanString(data[i]["taxi"]);
						std::string ctot = cleanString(data[i]["ctot"]);
						std::string aobt = cleanString(data[i]["aobt"]);
						std::string eta = cleanString(data[i]["eta"]);
						std::string mostPenalisingRegulation = cleanString(data[i]["atfcmData"]["mostPenalisingRegulation"]);
						std::string atfcmStatus = cleanString(data[i]["atfcmStatus"]);
						std::string informed = cleanString(data[i]["informed"]);
						std::string isCdm = cleanString(data[i]["isCdm"]);
						const Json::Value& atfcm = data[i]["atfcmData"];
						std::string isExcluded = cleanString(atfcm["excluded"]);
						std::string isRea = cleanString(atfcm["isRea"]);
						std::string isSir = cleanString(atfcm["SIR"]);

						if (mostPenalisingRegulation.length() <= 2 && ctot.length() > 2) {
							mostPenalisingRegulation = "N/A";
						}

						//Only keep sts if not affected by ecfmp restriction
						relevantFlightsTemp.push_back({ callsign, departure, arrival, eobt, tobt, taxi, ctot, aobt, eta, mostPenalisingRegulation, atfcmStatus, informed, isCdm, isExcluded, isRea, isSir });
					}
				}
			}
			relevantFlights = relevantFlightsTemp;
			addLogLine("COMPLETED - getCdmServerRelevantFlights");
		}
		catch (const std::exception& e) {
			addLogLine("ERROR: Unhandled exception getCdmServerRelevantFlights: " + (string)e.what());
		}
		catch (...) {
			addLogLine("ERROR: Unhandled exception getCdmServerRelevantFlights");
		}
	}
}

vector<string> CDM::getCDMAirports() {
	return CDMairports;
}

string CDM::getFilterFlightsText() {
	return flightsFilterText;
}

vector<vector<string>> CDM::returnRelevantFlights() {
	return relevantFlights;
}

vector<string> CDM::getMasterAirports() {
	return masterAirports;
}

vector<vector<string>> CDM::getServerMasterAirports() {
	return serverMasterAirports;
}

void CDM::fetchRelevantFlights() {
	std::thread t78(&CDM::getCdmServerRelevantFlights, this);
	t78.detach();
}

bool CDM::setCdmServerStatusFromDialog(std::vector<std::string> flight, string request) {
	string requestToDo = "";
	if (request == "EXCL") {
		if (flight[13] == "true") requestToDo = "EXCLUDED/0";
		else requestToDo = "EXCLUDED/1";

		for (int i = 0; i < (int)relevantFlights.size(); i++)
		{
			if (relevantFlights[i][0] == flight[0])
				if (requestToDo == "EXCLUDED/1") relevantFlights[i][13] = "true";
				else relevantFlights[i][13] = "false";
		}
	}
	if (request == "REA") {
		if (flight[14] == "true") requestToDo = "REA/0";
		else requestToDo = "REA/1";

		for (int i = 0; i < (int)relevantFlights.size(); i++)
		{
			if (relevantFlights[i][0] == flight[0])
				if (requestToDo == "REA/1") relevantFlights[i][14] = "true";
				else relevantFlights[i][14] = "false";
		}
	}
	if (request == "SIR") {
		if (flight[15] == "true") requestToDo = "SIR/0";
		else requestToDo = "SIR/1";

		for (int i = 0; i < (int)relevantFlights.size(); i++)
		{
			if (relevantFlights[i][0] == flight[0])
				if (requestToDo == "SIR/1") relevantFlights[i][15] = "true";
				else relevantFlights[i][15] = "false";
		}
	}
	if (request == "SWM") {
		if (flight[16] == "true") requestToDo = "SWM/0";
		else requestToDo = "SWM/1";

		for (int i = 0; i < (int)relevantFlights.size(); i++)
		{
			if (relevantFlights[i][0] == flight[0])
				if (requestToDo == "SWM/1") relevantFlights[i][16] = "true";
				else relevantFlights[i][16] = "false";
		}
	}
	std::thread t6(&CDM::setCdmSts, this, flight[0], requestToDo);
	t6.detach();
	return true;
}

static void SendUnicodeChar(wchar_t ch)
{
	INPUT in[2]{};
	in[0].type = INPUT_KEYBOARD;
	in[0].ki.wScan = ch;
	in[0].ki.dwFlags = KEYEVENTF_UNICODE;

	in[1] = in[0];
	in[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;

	SendInput(2, in, sizeof(INPUT));
}

static void SendEnter()
{
	INPUT in[2]{};
	in[0].type = INPUT_KEYBOARD;
	in[0].ki.wVk = VK_RETURN;

	in[1] = in[0];
	in[1].ki.dwFlags = KEYEVENTF_KEYUP;

	SendInput(2, in, sizeof(INPUT));
}

// Types a string using Unicode input (reliable across layouts)
static void TypeTextInstant(const std::string& text)
{
	// Convert to UTF-16
	int wlen = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
	if (wlen <= 1) return;

	std::vector<wchar_t> wbuf((size_t)wlen);
	MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, wbuf.data(), wlen);

	// Each character needs 2 INPUT events (down + up)
	std::vector<INPUT> inputs;
	inputs.reserve((wlen - 1) * 2);

	for (int i = 0; i < wlen - 1; ++i)
	{
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

void CDM::sendAtfcmPrivateMessageToPilotCon(std::vector<std::string> flight)
{
	for (int i = 0; i < (int)relevantFlights.size(); i++)
	{
		if (relevantFlights[i][0] == flight[0])
			relevantFlights[i][11] = "true";
	}

	std::thread t457(&CDM::sendAtfcmPrivateMessageToPilot, this, flight);
	t457.detach();
}

bool CDM::sendAtfcmPrivateMessageToPilot(std::vector<std::string> flight)
{
	string callsign = "";
	bool correctPosition = false;
	if (ControllerMyself().IsValid()) {
		if (ControllerMyself().IsController()) {
			callsign = ControllerMyself().GetCallsign();
			if (callsign.size() > 3) {
				if (callsign.find("_DEL") != string::npos || callsign.find("_GND") != string::npos || callsign.find("_TWR") != string::npos || callsign.find("_APP") != string::npos || callsign.find("_CTR") != string::npos || callsign.find("_FMP") != string::npos) {
					correctPosition = true;
				}
			}
		}
	}

	if (!correctPosition) {
		sendMessage("You are not in a position able to send ATFCM messages to pilots.");
		return false;
	}
	std::thread t9(&CDM::setCdmSts, this, flight[0], "INFORMED/1");
	t9.detach();

	std::string message;

	const std::string& status = flight[10];

	if (status.find("FLS") != std::string::npos)
	{
		message = ".msg " + flight[0] + " [ATFCM MSG] OFF-BLOCK TIME EXPIRED - " + flight[0] + " (" + flight[1] + " - " + flight[2] +
			"). PLEASE, UPDATE YOUR NEW OFF-BLOCK TIME IN https://vats.im/vdgs AND MONITOR THE VDGS PANEL FOR FUTHER UPDATES. [END OF ATFCM MSG - TRIAL IN PROGRESS]";
	}
	else if (flight[6] != "")
	{
		message = ".msg " + flight[0] + " [ATFCM MSG] SLOT ALLOCATION MESSAGE - " + flight[0] + " (" + flight[1] + " - " + flight[2] +
			") CTOT:" + flight[6] + " REGUL:" + flight[9] +
			" RMK:PLEASE, MONITOR https://vats.im/vdgs FOR FURTHER CTOT UPDATES AND START-UP TIME INFORMATION. [END OF ATFCM MSG - TRIAL IN PROGRESS]";
	}
	else
	{
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
				if (position.find("_DEL") != string::npos || position.find("_GND") != string::npos || position.find("_TWR") != string::npos || position.find("_APP") != string::npos || position.find("_CTR") != string::npos || position.find("_FMP") != string::npos) {
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

	string msg = ".msg " + callsign + " [CDM MSG] PLEASE, MONITOR https://vats.im/vdgs FOR CDM AND ATFCM UPDATES. [END OF CDM MSG]";

	std::thread t458(&CDM::sendCdmPrivateMessageToPilot, this, msg);
	t458.detach();
}

bool CDM::sendCdmPrivateMessageToPilot(string message)
{
	TypeTextInstant(message);
	SendEnter();

	return true;
}

