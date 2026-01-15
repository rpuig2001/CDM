#include "stdafx.h"
#include "CDMSingle.hpp"
#include "pugixml.hpp"
#include "pugixml.cpp"
#include <thread>
#include "Delay.h"
#include "EcfmpRestriction.h"
#include "SFTP.h"

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
bool ctotCid;
bool realMode;
bool pilotTobt;
bool atotEnabled;
bool remarksOption;
bool invalidateTSAT_Option;
bool invalidateTOBT_Option;
bool sidIntervalEnabled;
bool readyToUpdateList;
string myTimeToAdd;
string rateUrl;
string taxiZonesUrl;
string ctotURL;
string cdmServerUrl;
string sidIntervalUrl;
int defTaxiTime;
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
vector<EcfmpRestriction> ecfmpData;
vector<Plane> apiCtots;
vector<string> asatList;
vector<string> taxiTimesList;
vector<string> TxtTimesVector;
vector<string> OutOfTsat;
vector<string> colors;
vector<Rate> rate;
vector<Rate> initialRate;
vector<string> planeAiportList;
vector<string> masterAirports;
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
vector<Plane> setTOBTlater;
vector<vector<string>> setCdmStslater;
vector<Plane> setCdmDatalater;
vector<string> suWaitList;
vector<string> checkCIDLater;
vector<string> disabledCtots;
vector<vector<string>> networkStatus;
vector<Plane> apiQueueResponse;
std::mutex apiQueueResponseMutex;
vector<vector<string>> deiceList;
vector<sidInterval> sidIntervalList;
vector<string> atotSet;
vector<vector<string>> reqTobtTypes;
vector<vector<string>> reqTobtTypesQueue;
std::mutex reqTobtTypesQueueMutex;
std::mutex later1Mutex;
std::mutex later2Mutex;
std::mutex later3Mutex;
std::mutex later4Mutex;

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
	addLogLine(loadingMessage);

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
	RegisterTagItemFunction("Edit TSAC", TAG_FUNC_EDITTSAC);
	RegisterTagItemFunction("TSAC Options", TAG_FUNC_OPT_TSAC);

	// Register Tag Item "CDM-ASAT"
	RegisterTagItemType("ASAT", TAG_ITEM_ASAT);

	// Register Tag Item "CDM-ASAT"
	RegisterTagItemType("ASRT", TAG_ITEM_ASRT);
	RegisterTagItemFunction("Toggle ASRT", TAG_FUNC_TOGGLEASRT);

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
	RegisterTagItemFunction("Network Sts Options", TAG_FUNC_NETWORK_STATUS_OPTIONS);

	//Register Tag Item "CDM-DEICE"
	RegisterTagItemType("DE-ICE", TAG_ITEM_DEICE);
	RegisterTagItemFunction("DE-ICE Options", TAG_FUNC_OPT_DEICE);

	//Register Tag Item "REQTOBT-TYPE"
	RegisterTagItemType("TOBT-SET-BY", TAG_ITEM_TOBT_SETBY);

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

	tfad = DllPathFile;
	tfad.resize(tfad.size() - strlen("CDM.dll"));
	tfad += "log.txt";

	debugMode = false;
	initialSidLoad = false;

	removeLog();

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
	string realModeStr = getFromXml("/CDM/realMode/@mode");
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
	string stringDebugMode = getFromXml("/CDM/Debug/@mode");
	flowRestrictionsUrl = getFromXml("/CDM/FlowRestrictions/@url");
	vdgsFileType = getFromXml("/CDM/vdgsFileType/@type");
	ftpHost = getFromXml("/CDM/ftpHost/@host");
	ftpUser = getFromXml("/CDM/ftpUser/@user");
	ftpPassword = getFromXml("/CDM/ftpPassword/@password");
	string sftpConnectionString = getFromXml("/CDM/sftpConnection/@mode");
	string cdmserver = getFromXml("/CDM/Server/@mode");
	string opt_su_wait = getFromXml("/CDM/Su_Wait/@mode");

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
	if (invalidateTSAT_OptionStr == "false") {
		invalidateTSAT_Option = false;
	}

	invalidateTOBT_Option = true;
	if (invalidateTOBT_OptionStr == "false") {
		invalidateTOBT_Option = false;
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

	//CDM-Server
	cdmServerUrl = "https://cdm-server-production.up.railway.app";

	//CDM-Server Fetch restricted
	getCdmServerRestricted(slotList);

	apikey = "test";
	if (ftpPassword == "") {
		ftpPassword = "test";
	}

	//Init reamrksOption
	remarksOption = false;

	//Init refreshActions
	refresh1 = false;
	refresh2 = false;
	refresh3 = false;
	refresh4 = false;

	//Initialize with empty callsign
	myAtcCallsign = "";

	lvo = false;
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
	if (fp.GetTrackingControllerIsMe() || strlen(fp.GetTrackingControllerId()) == 0) {
		AtcMe = true;
	}

	if (FunctionId == TAG_FUNC_EDITEOBT)
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
					//Set EOBT in API
					std::thread t(&CDM::setCdmSts, this, fp.GetCallsign(), "EOBT/" + editedEOBT);
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
		string annotTSAC = getFlightStripInfo(fp, 1);
		string completeTOBT = getFlightStripInfo(fp, 2);
		if (annotTSAC.empty() && !completeTOBT.empty()) {
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
		else {
			setFlightStripInfo(fp, "", 1);
		}
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

	else if (FunctionId == TAG_FUNC_TOGGLEASRT || FunctionId == TAG_FUNC_READYSTARTUP) {
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
			}
			else {
				setFlightStripInfo(fp, "", 0);
			}
		}
	}

	else if (FunctionId == TAG_FUNC_FMASTEXT) {
		if (master) {
			addLogLine("TRIGGER - TAG_FUNC_FMASTEXT");
			for (size_t i = 0; i < slotList.size(); i++)
			{
				if (slotList[i].callsign == fp.GetCallsign()) {
					if (slotList[i].hasManualCtot) {
						sendMessage(slotList[i].callsign + " FM -> " + slotList[i].flowReason);
					}
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
		if (master && AtcMe) {
			addLogLine("TRIGGER - TAG_FUNC_NETWORK_STATUS_OPTIONS");

			//Get actual status
			string status = "";
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

			if (status == "") {
				OpenPopupList(Area, "CDM-Network", 1);
				if (status != "REA" && hasCtot) {
					AddPopupListElement("Set REA", "", TAG_FUNC_NETWORK_SET_REA, false, 2, false);
				}
			} else if (status == "REA") {
				OpenPopupList(Area, "CDM-Network", 1);
				AddPopupListElement("Remove REA", "", TAG_FUNC_NETWORK_REMOVE_REA, false, 2, false);
			}
		}
	}
	else if (FunctionId == TAG_FUNC_NETWORK_SET_REA) {
		if (master && AtcMe) {
			addLogLine("TRIGGER - TAG_FUNC_NETWORK_SET_REA");
			std::thread t3(&CDM::setCdmSts, this, fp.GetCallsign(), "REA/1");
			t3.detach();
		}
	}
	else if (FunctionId == TAG_FUNC_NETWORK_REMOVE_REA) {
		if (master && AtcMe) {
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
			AddPopupListElement("Ready TOBT", "", TAG_FUNC_READYTOBT, false, 2, false);
			AddPopupListElement("Edit TOBT", "", TAG_FUNC_EDITTOBT, false, 2, false);
			AddPopupListElement("----------------", "", -1, false, 2, false);

			//TSAC OPTIONS
			string tsacvalue = getFlightStripInfo(fp, 1);
			if (tsacvalue.empty()) {
				AddPopupListElement("Add TSAT to TSAC", "", TAG_FUNC_ADDTSAC, false, 2, false);
			}
			else {
				AddPopupListElement("Remove TSAC", "", TAG_FUNC_ADDTSAC, false, 2, false);
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
		if (master && AtcMe) {
			addLogLine("TRIGGER - TAG_FUNC_OPT_TOBT");
			OpenPopupList(Area, "TOBT Options", 1);
			AddPopupListElement("Ready TOBT", "", TAG_FUNC_READYTOBT, false, 2, false);
			AddPopupListElement("Edit TOBT", "", TAG_FUNC_EDITTOBT, false, 2, false);
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

	else if (FunctionId == TAG_FUNC_OPT_TSAC) {
		if (master && AtcMe) {
			addLogLine("TRIGGER - TAG_FUNC_OPT_TSAC");
			OpenPopupList(Area, "TSAC Options", 1);
			string tsacvalue = getFlightStripInfo(fp, 1);
			if (tsacvalue.empty()) {
				AddPopupListElement("Add TSAT to TSAC", "", TAG_FUNC_ADDTSAC, false, 2, false);
			}
			else {
				AddPopupListElement("Remove TSAC", "", TAG_FUNC_ADDTSAC, false, 2, false);
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

					//Set REA Status
					std::thread t99(&CDM::setCdmSts, this, fp.GetCallsign(), "REA/1");
					t99.detach();
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
			if ((string)fp.GetGroundState() != "STUP" || (string)fp.GetGroundState() != "ST-UP" || (string)fp.GetGroundState() != "PUSH" || (string)fp.GetGroundState() != "TAXI" || (string)fp.GetGroundState() != "DEPA") {
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
			if ((string)fp.GetGroundState() != "STUP" || (string)fp.GetGroundState() != "ST-UP" || (string)fp.GetGroundState() != "PUSH" || (string)fp.GetGroundState() != "TAXI" || (string)fp.GetGroundState() != "DEPA") {
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
			if ((string)fp.GetGroundState() != "STUP" || (string)fp.GetGroundState() != "ST-UP" || (string)fp.GetGroundState() != "PUSH" || (string)fp.GetGroundState() != "TAXI" || (string)fp.GetGroundState() != "DEPA") {
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
							std::thread t(&CDM::setTOBTApi, this, slotList[i].callsign, slotList[i].tsat, true);
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
		if (master && AtcMe) {
			addLogLine("TRIGGER - TAG_FUNC_EDITTOBT");
			OpenPopupEdit(Area, TAG_FUNC_NEWTOBT, getFlightStripInfo(fp, 2).c_str());
		}
	}
	else if (FunctionId == TAG_FUNC_NEWTOBT) {
		try {
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
				else if (editedTOBT.empty()) {
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
						std::thread t(&CDM::setTOBTApi, this, (string)fp.GetCallsign(), "", true);
						t.detach();
					}
					//}
				}

				//Update TOBT-setBy
				if (setBy != "NONE") {
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


	if (!isVfr) {

		//Refresh ecfmpData every 5 min
		time_t timeNow = std::time(nullptr);
		if ((timeNow - countEcfmpTime) > 300 && !refresh2) {
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
		if ((timeNow - countFetchServerTime) > 30 && !refresh3) {
			countFetchServerTime = timeNow;
			std::thread t(&CDM::refreshActions3, this);
			t.detach();
			if (debugMode) {
				sendMessage("[DEBUG MESSAGE] - REFRESHING CDM API DATA 1");
			}
		}
		if ((timeNow - countRefreshActions4Time) > 10 && !refresh4) {
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

					for (size_t i = 0; i < OutOfTsat.size(); i++)
					{
						bool networkSuspended = false;
						if (callsign == OutOfTsat[i].substr(0, OutOfTsat[i].find(","))) {
							for (size_t i = 0; i < networkStatus.size(); i++) {
								if (networkStatus[i][0] == callsign) {
									if (networkStatus[i][1].find("FLS") != string::npos && networkStatus[i][1].find("CDM") == string::npos) {
										networkSuspended = true;
									}
								}
							}

							if (EOBTfinal.substr(0, 4) == OutOfTsat[i].substr(OutOfTsat[i].find(",") + 1, 4) || networkSuspended) {
								stillOutOfTsat = true;
								stillOutOfTsatPos = i;
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
							for (size_t i = 0; i < networkStatus.size(); i++) {
								if (networkStatus[i][0] == callsign) {
									if (networkStatus[i][1].find("FLS") != string::npos) {
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
							ItemRGB = TAG_GREENNOTACTIVE;
							strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
						}
						else if (ItemCode == TAG_ITEM_ETOBT)
						{
							string ShowEOBT = (string)EOBT;
							ItemRGB = TAG_GREENNOTACTIVE;
							strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
						}
						else if (ItemCode == TAG_ITEM_TSAC)
						{
							ItemRGB = TAG_GREEN;
							strcpy_s(sItemString, 16, "____");
						}
						else if (ItemCode == TAG_ITEM_TSAC_SIMPLE)
						{
							string annotTSAC = getFlightStripInfo(FlightPlan, 1);
							if (!annotTSAC.empty()) {
								ItemRGB = TAG_GREEN;
								strcpy_s(sItemString, 16, "\xA4");
							}
							else {
								ItemRGB = TAG_GREEN;
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
							for (size_t i = 0; i < networkStatus.size(); i++) {
								if (networkStatus[i][0] == callsign) {
									status = networkStatus[i][1];
								}
							}
							if (status != "") {
								ItemRGB = TAG_YELLOW;
								if (status == "REA") {
									ItemRGB = TAG_YELLOW;
								}
								else if (status.find("FLS") != string::npos) {
									ItemRGB = TAG_RED;
									status = GetTimedStatus(status);
								}
								else if (status == "COMPLY") {
									ItemRGB = TAG_GREEN;
								}
								strcpy_s(sItemString, 16, status.c_str());
							}
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
											if (correctTTOT && sidIntervalEnabled) {
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
												if (TTOT != slotList[pos].ttot) {
													Plane p(callsign, EOBT, TSAT, TTOT, slotList[pos].ctot, slotList[pos].flowReason, myEcfmp, hasEcfmpRestriction, hasManualCtot, true);
													doRequest = true;
													slotList[pos] = p;
													setFlightStripInfo(FlightPlan, p.tsat, 3);
													setFlightStripInfo(FlightPlan, p.ttot, 4);
												}
											}
											else {
												Plane p(callsign, EOBT, TSAT, TTOT, "", "", myEcfmp, hasEcfmpRestriction, hasManualCtot, true);
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
													std::thread t(&CDM::setTOBTApi, this, callsign, myTSATApi, true);
													t.detach();
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
								OutOfTsat.push_back(callsign + "," + EOBT);
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
							if (difTime >= 44 && difTime <= 45) {
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
							if (difTime > 5) {
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

						if (oldTSAT && aircraftFind) {
							if (slotList[pos].hasManualCtot) {
								oldTSAT = false;
							}
						}

						string getTTOT = getFlightStripInfo(FlightPlan, 4);
						if (oldTSAT && !correctState && (!oldTOBT || !invalidateTOBT_Option) && invalidateTSAT_Option && !getTTOT.empty()) {
							OutOfTsat.push_back(callsign + "," + EOBT);
							setFlightStripInfo(FlightPlan, "", 0);
							setFlightStripInfo(FlightPlan, "", 3);
							setFlightStripInfo(FlightPlan, "", 4);
							
							//Update CDM-API
							std::thread t(&CDM::setCdmSts, this, callsign, "SUSP");
							t.detach();
						}

						//If suspended by network Status, mark it as Invalid (I)
						for (size_t i = 0; i < networkStatus.size(); i++) {
							if (networkStatus[i][0] == callsign) {
								//Check for any FLS status (except "FLS-CDM")
								if (networkStatus[i][1].find("FLS") != string::npos && networkStatus[i][1].find("CDM") == string::npos) {
									OutOfTsat.push_back(callsign + "," + EOBT);
									setFlightStripInfo(FlightPlan, "", 0);
									setFlightStripInfo(FlightPlan, "", 3);
									setFlightStripInfo(FlightPlan, "", 4);
								}
							}
							//Do NOT Update CDM-API (As we are SUSP because of network status)
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
								if (mySetEobt != tobt) {
									ItemRGB = TAG_ORANGE;
								}
							}
							for (size_t i = 0; i < networkStatus.size(); i++) {
								if (networkStatus[i][0] == callsign) {
									if (networkStatus[i][1].find("FLS") != string::npos) {
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
									ItemRGB = TAG_YELLOW;
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
									ItemRGB = TAG_YELLOW;
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
								strcpy_s(sItemString, 16, annotTSAC.c_str());
							}
							else if (!annotTSAC.empty()) {
								//*pColorCode = TAG_COLOR_RGB_DEFINED;
								ItemRGB = TAG_GREEN;
								strcpy_s(sItemString, 16, annotTSAC.c_str());
							}
							else {
								//*pColorCode = TAG_COLOR_RGB_DEFINED;
								ItemRGB = TAG_GREEN;
								strcpy_s(sItemString, 16, "____");
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
								if (TSACNotTSAT) {
									ItemRGB = TAG_ORANGE;
								}
								else {
									//*pColorCode = TAG_COLOR_RGB_DEFINED;
									ItemRGB = TAG_GREEN;
								}
								strcpy_s(sItemString, 16, "\xA4");
							}
							else {
								ItemRGB = TAG_GREEN;
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
									else if (lastMinute) {
										//*pColorCode = TAG_COLOR_RGB_DEFINED;
										ItemRGB = TAG_YELLOW;
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
						for (size_t i = 0; i < networkStatus.size(); i++) {
							if (networkStatus[i][0] == callsign) {
								status = networkStatus[i][1];
							}
						}
						if (status != "") {
							ItemRGB = TAG_YELLOW;
							if (status == "REA") {
								ItemRGB = TAG_YELLOW;
							}
							else if (status.find("FLS") != string::npos) {
								ItemRGB = TAG_RED;
								status = GetTimedStatus(status);
							}
							else if (status == "COMPLY") {
								ItemRGB = TAG_GREEN;
							}
							strcpy_s(sItemString, 16, status.c_str());
						}
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
							Plane p(callsign, EOBT, TSATString, TTOTString, "", "", myEcfmp, hasEcfmpRestriction, hasManualCtot, true);
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
						bool lastMinuteTOBT = false;
						bool notYetEOBT = false;
						bool actualTOBT = false;

						if (hour != "00" && TSAThour == "00") {
							TSAThour = "24";
						}

						int difTime = GetdifferenceTime(hour, min, TSAThour, TSATmin);

						if (hour != TSAThour) {
							if (difTime >= 44 && difTime <= 45) {
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
							if (difTime > 5) {
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
								if (callsign == OutOfTsat[i].substr(0, OutOfTsat[i].find(","))) {
									alreadyInList = true;
								}
							}

							if (!alreadyInList) {
								OutOfTsat.push_back(callsign + "," + EOBT);
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
							for (size_t i = 0; i < networkStatus.size(); i++) {
								if (networkStatus[i][0] == callsign) {
									if (networkStatus[i][1].find("FLS") != string::npos) {
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
								ItemRGB = TAG_YELLOW;
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
								ItemRGB = TAG_YELLOW;
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
								strcpy_s(sItemString, 16, annotTSAC.c_str());
							}
							else if (!annotTSAC.empty()) {
								ItemRGB = TAG_GREEN;
								strcpy_s(sItemString, 16, annotTSAC.c_str());
							}
							else {
								ItemRGB = TAG_GREEN;
								strcpy_s(sItemString, 16, "____");
							}
						}
						else if (ItemCode == TAG_ITEM_TSAC_SIMPLE)
						{
							string annotTSAC = getFlightStripInfo(FlightPlan, 1);
							if (!annotTSAC.empty()) {
								if (TSACNotTSAT) {
									ItemRGB = TAG_ORANGE;
								}
								else {
									//*pColorCode = TAG_COLOR_RGB_DEFINED;
									ItemRGB = TAG_GREEN;
								}
								strcpy_s(sItemString, 16, "\xA4");
							}
							else {
								ItemRGB = TAG_GREEN;
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
								else if (lastMinute) {
									ItemRGB = TAG_YELLOW;
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
									strcpy_s(sItemString, 16, sr.ctot.c_str());
								}
							}
						}
						else if (ItemCode == NOW_CTOT_DIFF)
						{
							for (ServerRestricted sr : serverRestrictedPlanes) {
								if (sr.callsign == (string)FlightPlan.GetCallsign()) {
									ItemRGB = TAG_CTOT;
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
						for (size_t i = 0; i < networkStatus.size(); i++) {
							if (networkStatus[i][0] == callsign) {
								status = networkStatus[i][1];
							}
						}
						if (status != "") {
							ItemRGB = TAG_YELLOW;
							if (status == "REA") {
								ItemRGB = TAG_YELLOW;
							}
							else if (status.find("FLS") != string::npos) {
								ItemRGB = TAG_RED;
								status = GetTimedStatus(status);
							}
							else if (status == "COMPLY") {
								ItemRGB = TAG_GREEN;
							}
							strcpy_s(sItemString, 16, status.c_str());
						}
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
							for (size_t i = 0; i < networkStatus.size(); i++) {
								if (networkStatus[i][0] == callsign) {
									if (networkStatus[i][1].find("FLS") != string::npos) {
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
							for (size_t i = 0; i < networkStatus.size(); i++) {
								if (networkStatus[i][0] == callsign) {
									status = networkStatus[i][1];
								}
							}
							if (status != "") {
								ItemRGB = TAG_YELLOW;
								if (status == "REA") {
									ItemRGB = TAG_YELLOW;
								}
								else if (status.find("FLS") != string::npos) {
									ItemRGB = TAG_RED;
									status = GetTimedStatus(status);
								}
								else if (status == "COMPLY") {
									ItemRGB = TAG_GREEN;
								}
								strcpy_s(sItemString, 16, status.c_str());
							}
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
					for (size_t i = 0; i < networkStatus.size(); i++) {
						if (networkStatus[i][0] == callsign) {
							if (networkStatus[i][1].find("FLS") != string::npos) {
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
					for (size_t i = 0; i < networkStatus.size(); i++) {
						if (networkStatus[i][0] == callsign) {
							status = networkStatus[i][1];
						}
					}
					if (status != "") {
						ItemRGB = TAG_YELLOW;
						if (status == "REA") {
							ItemRGB = TAG_YELLOW;
						}
						else if (status.find("FLS") != string::npos) {
							ItemRGB = TAG_RED;
							status = GetTimedStatus(status);
						}
						else if (status == "COMPLY") {
							ItemRGB = TAG_GREEN;
						}
						strcpy_s(sItemString, 16, status.c_str());
					}
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
				Plane p(callsign, EOBTfinal, EOBTfinal, EOBTfinal, "", "", myEcfmp, false, false, true);
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
				for (size_t i = 0; i < networkStatus.size(); i++) {
					if (networkStatus[i][0] == callsign) {
						if (networkStatus[i][1].find("FLS") != string::npos) {
							ItemRGB = TAG_RED;
						}
						break;
					}
				}
				strcpy_s(sItemString, 16, EOBTfinal.c_str());
			}
			if (ItemCode == TAG_ITEM_ETOBT)
			{
				ItemRGB = TAG_EOBT;
				strcpy_s(sItemString, 16, EOBTfinal.c_str());
			}
			if (ItemCode == TAG_ITEM_CTOT)
			{
				for (ServerRestricted sr : serverRestrictedPlanes) {
					if (sr.callsign == (string)FlightPlan.GetCallsign()) {
						ItemRGB = TAG_CTOT;
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
				for (size_t i = 0; i < networkStatus.size(); i++) {
					if (networkStatus[i][0] == callsign) {
						status = networkStatus[i][1];
					}
				}
				if (status != "") {
					ItemRGB = TAG_YELLOW;
					if (status == "REA") {
						ItemRGB = TAG_YELLOW;
					}
					else if (status.find("FLS") != string::npos) {
						ItemRGB = TAG_RED;
						status = GetTimedStatus(status);
					}
					else if (status == "COMPLY") {
						ItemRGB = TAG_GREEN;
					}
					strcpy_s(sItemString, 16, status.c_str());
				}
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
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
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
			if (remarksOption) {
				CFlightPlan fplSelect = FlightPlanSelect(slotList[i].callsign.c_str());
				if (!fplSelect.IsValid()) {
					continue;
				}
				string testTsat = slotList[i].tsat;
				if (testTsat.length() >= 4) {
					if (
						(string)fplSelect.GetGroundState() != "STUP" &&
						(string)fplSelect.GetGroundState() != "ST-UP" &&
						(string)fplSelect.GetGroundState() != "PUSH" &&
						(string)fplSelect.GetGroundState() != "TAXI" &&
						(string)fplSelect.GetGroundState() != "DEPA")
					{
						fplSelect.GetControllerAssignedData().SetScratchPadString(testTsat.substr(0, 4).c_str());
					}
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
					myTTOT = slotList[i].ctot + "00";
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
					if (correctTTOT && sidIntervalEnabled) {
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
									Plane p(callsign, EOBT, TSAT, TTOT, plane.ctot, myFlow, myEcfmp, hasEcfmpRestriction, plane.hasManualCtot, true);
									plane = p;
									doRequest = true;
									setFlightStripInfo(FlightPlan, p.tsat, 3);
									setFlightStripInfo(FlightPlan, p.ttot, 4);
								}
							}
							else if (TTOT.length() >= 4) {
								Plane p(callsign, EOBT, TSAT, TTOT, plane.ctot, myFlow, myEcfmp, hasEcfmpRestriction, plane.hasManualCtot, true);
								plane = p;
								setFlightStripInfo(FlightPlan, p.tsat, 3);
								setFlightStripInfo(FlightPlan, p.ttot, 4);
							}
						}
						else {
							if (aircraftFind) {
								if (TTOT != plane.ttot && TTOT.length() >= 4) {
									Plane p(callsign, EOBT, TSAT, TTOT, plane.ctot, myFlow, myEcfmp, hasEcfmpRestriction, plane.hasManualCtot, true);
									plane = p;
									doRequest = true;
									setFlightStripInfo(FlightPlan, p.tsat, 3);
									setFlightStripInfo(FlightPlan, p.ttot, 4);
								}
							}
							else if (TTOT.length() >= 4) {
								Plane p(callsign, EOBT, TSAT, TTOT, plane.ctot, myFlow, myEcfmp, hasEcfmpRestriction, plane.hasManualCtot, true);
								plane = p;
								setFlightStripInfo(FlightPlan, p.tsat, 3);
								setFlightStripInfo(FlightPlan, p.ttot, 4);
							}
						}
						//Check API
						if (doRequest && TSATfinal.length() >= 4) {
							if (serverEnabled) {
								string myTSATApi = TSAT;
								setTOBTApi(callsign, myTSATApi, false);
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

/*
* Mehod to push FlightStrip Data to other controllers (old Amend)
*/
void CDM::PushToOtherControllers(CFlightPlan fp) {
	string callsign = "";
	for (CController c = ControllerSelectFirst(); c.IsValid(); c = ControllerSelectNext(c)) {
		if (c.IsController()) {
			callsign = c.GetCallsign();
			if (callsign.size() > 3) {
				if (callsign.find("DEL") != string::npos || callsign.find("GND") != string::npos || callsign.find("TWR") != string::npos || callsign.find("APP") != string::npos) {
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
			if (regex_match(TxtTimesVector[t], match, regex("([A-Z]{4}):(\\d{2}[LRC]?):([^:]+):([^:]+):([^:]+):([^:]+):([^:]+):([^:]+):([^:]+):([^:]+):(\\d+):([^:]+)", regex::icase)))
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

				if (inPoly(4, LatArea, LonArea, lat, lon) % 2 != 0) {
					if (isNumber(match[11])) {
						// Check for REM pad times only if present and index is valid
						if (remId > 0 && times.size() >= remId) {
							if (isNumber(times[remId - 1])) {
								return to_string(stoi(match[11]) + deIceTime + stoi(times[remId - 1]));
							}
						}
						// Else, just use taxi time + deIceTime
						return to_string(stoi(match[11]) + deIceTime);
					}
					return match[11];
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

vector<Plane> CDM::recalculateSlotList(vector<Plane> mySlotList) {
	addLogLine("Called recalculateSlotList...");
	try {
		int slotListLength = mySlotList.size();
		bool ordered = false;
		string value1 = "", value2 = "";
		bool swap = false;
		while (!ordered) {
			ordered = true;
			for (int i = 0; i < slotListLength; i++) {
				if (i < slotListLength - 1) {
					swap = false;
					value1 = mySlotList[i].ttot;
					value2 = mySlotList[i + 1].ttot;
					if (stoi(value1) > stoi(value2)) {
						if (mySlotList[i].hasManualCtot && mySlotList[i + 1].hasManualCtot) {
							swap = true;
						}
						else if (!mySlotList[i].hasManualCtot && !mySlotList[i + 1].hasManualCtot) {
							swap = true;
						}
					}
					//swap if previous no ctot and after has ctot. Otherwise, calculation maks same TTOT...
					else if (stoi(value1) == stoi(value2) && !mySlotList[i].hasManualCtot && mySlotList[i + 1].hasManualCtot) {
						swap = true;
					}
					if (swap) {
						ordered = false;
						Plane saved1 = mySlotList[i];
						Plane saved2 = mySlotList[i + 1];
						mySlotList[i] = saved2;
						mySlotList[i + 1] = saved1;
					}
				}
			}
		}
	}
	catch (const std::exception& e) {
		addLogLine("ERROR: Unhandled exception recalculateSlotList: " + (string)e.what());
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
		if (callsign == OutOfTsat[i].substr(0, OutOfTsat[i].find(","))) {
			if (debugMode) {
				sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 10");
			}
			OutOfTsat.erase(OutOfTsat.begin() + i);
		}
	}
	//Remove from setTOBTlater
	{
		std::lock_guard<std::mutex> lock(later1Mutex);
		for (size_t i = 0; i < setTOBTlater.size(); i++)
		{
			if (callsign == setTOBTlater[i].callsign) {
				if (debugMode) {
					sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 11");
				}
				setTOBTlater.erase(setTOBTlater.begin() + i);
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
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20);
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
		bool updated = false;
		bool found = false;
		vector<Plane> mySlotList = slotList;
		for (Plane plane : mySlotList) {
			found = false;
			for (Plane planeSaved : slotListSaved) {
				if (plane.callsign == planeSaved.callsign) {
					found = true;
					if (plane.ctot != planeSaved.ctot || plane.ttot != planeSaved.ttot || plane.tsat != planeSaved.tsat || plane.eobt != planeSaved.eobt || plane.flowReason != planeSaved.flowReason) {
						updateCdmDataApi(plane);
						updated = true;
					}
				}
			}
			if (!found) {
				//Plane is new in the slotList (not found int he latest slotListSaved)
				updated = true;
				updateCdmDataApi(plane);
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
				Plane myPlane(planeSaved.callsign, "", "", "", "", "", EcfmpRestriction(), false, false, false);
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
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
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
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
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
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
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
			if (deice[2].find("REM") != std::string::npos && deice[1].length() == 4) {
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
	// If no '-' exists, return as-is
	size_t dashPos = status.find('-');
	if (dashPos == std::string::npos) {
		return status;
	}

	static auto startTime = std::chrono::steady_clock::now();

	auto now = std::chrono::steady_clock::now();
	auto elapsedSeconds =
		std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();

	// 0–1 = first part, 2–3 = second part, repeat
	bool firstPart = ((elapsedSeconds / 2) % 2) == 0;

	if (firstPart) {
		return status.substr(0, dashPos);
	}
	else {
		return status.substr(dashPos + 1);
	}
}

void CDM::addVatcanCtotToEvCTOT(string line) {
	slotFile.push_back({ line.substr(0,line.find(",")), line.substr(line.find(",")+1, 4)});
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
		sendMessage("CDM Commands: .cdm ctot - .cdm master {airport} - .cdm slave {airport} - .cdm refreshtime {seconds} - .cdm startupdelay {icao}/{rwy} {start_time} - .cdm departuredelay {icao}/{rwy} {start_time} - .cdm lvo - .cdm realmode - .cdm server - .cdm remarks - .cdm rate - .cdm help");
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
	refresh2 = true;
	addLogLine("[AUTO] - REFRESH ECFMP");
	getEcfmpData();
	refresh2 = false;
}

void CDM::refreshActions3() {
	refresh3 = true;
	addLogLine("[AUTO] - REFRESH API 1");
	getCdmServerRestricted(slotList);
	refresh3 = false;
}

void CDM::refreshActions4() {
	refresh4 = true;
	addLogLine("[AUTO] - REFRESH API 2");
	getCdmServerStatus();
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
				curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
				result = curl_easy_perform(curl);
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
				curl_easy_cleanup(curl);
			}

			if ((responseCode == 404 || CURLE_OK != result) && responseCode != 401) {
				addLogLine("UNABLE TO CONNECT CDM-API...");
				masterAirports.push_back(airport);
				sendMessage("Successfully set master airport (Locally only) " + airport);
				addLogLine("Successfully set master airport (Locally only) " + airport);
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
			return false;
		}
		catch (...) {
			addLogLine("ERROR: Unhandled exception setMasterAirport");
			return false;
		}
	}
	else {
		masterAirports.push_back(airport);
		sendMessage("Successfully set master airport (Locally only) " + airport);
		addLogLine("Successfully set master airport (Locally only) " + airport);
	}
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
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
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
				curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
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
				curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
				result = curl_easy_perform(curl);
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
				curl_easy_cleanup(curl);
			}

			if (responseCode == 404 || responseCode == 401 || responseCode == 502 || CURLE_OK != result) {
				addLogLine("UNABLE TO CONNECT CDM-API...");
			}
			else {
				addLogLine("Removed masters for airport " + airport);
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
				string url = cdmServerUrl + "/plane/cidCheck?callsign=" + callsign;
				string apiKeyHeader = "x-api-key: " + apikey;
				struct curl_slist* headers = NULL;
				headers = curl_slist_append(headers, apiKeyHeader.c_str());
				curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
				curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
				curl_easy_setopt(curl, CURLOPT_HTTPGET, true);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
				curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
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
				string apiKeyHeader = "x-api-key: " + apikey;
				struct curl_slist* headers = NULL;
				headers = curl_slist_append(headers, apiKeyHeader.c_str());
				curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
				curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
				curl_easy_setopt(curl, CURLOPT_HTTPGET, true);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
				curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20);
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
					if (restricted[i].isMember("callsign") && restricted[i].isMember("ctot") && restricted[i].isMember("mostPenalizingAirspace")) {
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
						string reason = fastWriter.write(restricted[i]["mostPenalizingAirspace"]);
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
		vector<Plane> setTOBTlaterTemp;
		{
			addLogLine("Call sendWaitingTOBT - " + to_string(setTOBTlater.size()));
			addLogLine("Called sendWaitingTOBT...");
			std::lock_guard<std::mutex> lock(later1Mutex);
			setTOBTlaterTemp = setTOBTlater;
			setTOBTlater.clear();
		}

		vector<Plane> alreadyProcessed;
		bool found = false;

		for (int i = 0; i < setTOBTlaterTemp.size(); i++) {
			found = false;
			for (Plane p : alreadyProcessed) {
				if (p.callsign == setTOBTlaterTemp[i].callsign) {
					found = true;
				}
			}

			if (!found) {
				alreadyProcessed.push_back(setTOBTlaterTemp[i]);
				addLogLine("sendWaitingTOBT - " + setTOBTlaterTemp[i].callsign);
				if (serverEnabled) {
					setTOBTApi(setTOBTlaterTemp[i].callsign, setTOBTlaterTemp[i].tsat, false);
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
		addLogLine("Call sendWaitingCdmSts - " + to_string(setCdmDatalater.size()));
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
				if (p.hasManualCtot && p.ctot != "" && p.ttot.length() >= 4) {
					str = "callsign=" + p.callsign + "&tobt=" + p.eobt + "&tsat=" + p.tsat + "&ttot=" + p.ttot + "&ctot=" + p.ctot.substr(0, 4) + "&reason=" + p.flowReason;
				}
				else if (p.hasManualCtot && p.ttot.length() >= 4) {
					str = "callsign=" + p.callsign + "&tobt=" + p.eobt + "&tsat=" + p.tsat + "&ttot=" + p.ttot + "&ctot=" + p.ttot.substr(0, 4) + "&reason=MANUAL";
				}
				else {
					str = "callsign=" + p.callsign + "&tobt=" + p.eobt + "&tsat=" + p.tsat + "&ttot=" + p.ttot + "&ctot=&reason=";
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
					curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20);
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

void CDM::setTOBTApi(string callsign, string tobt, bool triggeredByUser) {
		addLogLine("Called setTOBTApi...");
		try {
			vector<Plane> slotListTemp; // Local copy of the slotList
			{
				slotListTemp = slotList; // Copy the slotList
			}

			addLogLine("Call - Set TOBT (" + tobt + ") for " + callsign);
			bool createRequest = false;

			if (tobt != "") {
				for (Plane p : slotListTemp) {
					if (p.callsign == callsign) {
						createRequest = true;
						//Only create request if TOBT is manually triggered (or initially triggered or when no ctot), to avoid update set TSAT when syncing from CTOT
						if ((p.ctot != "" && triggeredByUser) || p.ctot == "") {
							createRequest = true;
						}
						else {
							createRequest = false;
						}
					}
				}
			}
			else {
				createRequest = true;
			}

			if (isFligthSusp(callsign)) createRequest = false;

			if (createRequest) {
				tobt = (tobt.length() >= 4) ? tobt.substr(0, 4) : "";
				string taxiTime = getTaxiTime(callsign);

				CURL* curl;
				CURLcode result = CURLE_FAILED_INIT;
				string readBuffer;
				long responseCode = 0;
				curl = curl_easy_init();

				if (curl) {
					addLogLine("Requesting TOBT (" + tobt + ") for " + callsign);
					string url = cdmServerUrl + "/ifps/dpi?callsign=" + callsign + "&value=TOBT/" + tobt + "/" + taxiTime;
					string apiKeyHeader = "x-api-key: " + apikey;
					struct curl_slist* headers = curl_slist_append(NULL, apiKeyHeader.c_str());
					curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
					curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
					curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
					curl_easy_setopt(curl, CURLOPT_POST, 1L);
					curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
					curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20);
					result = curl_easy_perform(curl);
					curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
					curl_easy_cleanup(curl);
				}

				if (responseCode == 404 || responseCode == 401 || responseCode == 502 || CURLE_OK != result) {
					Plane plane(callsign, "", tobt, "", "", "", EcfmpRestriction(), false, false, false);
					{
						std::lock_guard<std::mutex> lock(later1Mutex);
						setTOBTlater.push_back(plane); // Safely modify setTOBTlater
					}
					addLogLine("UNABLE TO CONNECT CDM-API...");
				}
				else {
					Json::Reader reader;
					Json::Value obj;
					Json::FastWriter fastWriter;
					reader.parse(readBuffer, obj);
					if (obj.isMember("callsign") && obj.isMember("ctot") && obj.isMember("mostPenalizingAirspace")) {
						string apiCallsign = fastWriter.write(obj["callsign"]);
						apiCallsign.erase(remove(apiCallsign.begin(), apiCallsign.end(), '"'), apiCallsign.end());
						apiCallsign.erase(remove(apiCallsign.begin(), apiCallsign.end(), '\n'), apiCallsign.end());

						string ctot = fastWriter.write(obj["ctot"]);
						ctot.erase(remove(ctot.begin(), ctot.end(), '"'), ctot.end());
						ctot.erase(remove(ctot.begin(), ctot.end(), '\n'), ctot.end());

						string reason = fastWriter.write(obj["mostPenalizingAirspace"]);
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
											calculateLessTime(ctot, stod(getTaxiTime(apiCallsign))),
											slotListTemp[i].ttot,
											ctot,
											reason,
											slotListTemp[i].ecfmpRestriction,
											slotListTemp[i].hasEcfmpRestriction,
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
						Plane plane(callsign, "", tobt, "", "", "", EcfmpRestriction(), false, false, false);
						{
							std::lock_guard<std::mutex> lock(later1Mutex);
							setTOBTlater.push_back(plane);
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

			addLogLine("COMPLETED - setTOBTApi for " + callsign);
		}
		catch (const std::exception& e) {
			addLogLine("ERROR: Unhandled exception setTOBTApi: " + (string)e.what());
			{
				for (size_t a = 0; a < slotList.size(); a++) {
					if (slotList[a].callsign == callsign) {
						slotList[a].showData = true;
					}
				}
			}
		}
		catch (...) {
			addLogLine("ERROR: Unhandled exception setTOBTApi");
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
				curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20);
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
				//Get data from .txt file
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
		if (callsign == OutOfTsat[i].substr(0, OutOfTsat[i].find(","))) {
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
				curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20);
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
			networkStatus = networkStatusTemp;
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
				curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20);
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
					if (data[i].isMember("type") && data[i].isMember("airspace") && data[i].isMember("capacity")) {

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
								//Get CTOT
								string capacity = fastWriter.write(data[i]["capacity"]);
								capacity.erase(std::remove(capacity.begin(), capacity.end(), '"'));
								capacity.erase(std::remove(capacity.begin(), capacity.end(), '\n'));
								capacity.erase(std::remove(capacity.begin(), capacity.end(), '\n'));

								for (int i = 0; i < tempRate.size(); i++) {
									if (tempRate[i].airport == airspace) {
										for (int a = 0; a < tempRate[i].rates.size(); a++) {
											tempRate[i].rates[a] = capacity;
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
				curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20);
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
					if (data[i].isMember("cdmData") && data[i]["cdmData"].isMember("reqTobt") && data[i]["cdmData"].isMember("reqTobtType") && data[i].isMember("callsign") && data[i].isMember("atot")) {

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

						string atot = fastWriter.write(data[i]["atot"]);
						atot.erase(std::remove(atot.begin(), atot.end(), '"'));
						atot.erase(std::remove(atot.begin(), atot.end(), '\n'));
						atot.erase(std::remove(atot.begin(), atot.end(), '\n'));

						if (atot == "") {
							planes.push_back({ callsign, tobt, type });
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
			if (plane.size() == 3) {
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
								string annotAsrt = getFlightStripInfo(fp, 0);
								if (annotAsrt.empty() && (string)fp.GetGroundState() != "STUP" && (string)fp.GetGroundState() != "ST-UP" && (string)fp.GetGroundState() != "PUSH" && (string)fp.GetGroundState() != "TAXI" && (string)fp.GetGroundState() != "DEPA") {
									addLogLine("Updating TOBT for: " + mySlotList[i].callsign + " Old: " + mySlotList[i].eobt + " New: " + plane[1] + "00");
									/*int posPlane = getPlanePosition(mySlotList[i].callsign);
									if (posPlane != -1) {
										slotList.erase(slotList.begin() + posPlane);
									}*/
									setFlightStripInfo(fp, plane[1], 2);
									setCdmSts(plane[0], "REQTOBT/NULL/NULL");
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
							setCdmSts(plane[0], "REQTOBT/NULL/NULL");
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
				curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20);
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
				Plane plane = Plane(newplane[0], newplane[1], newplane[2], newplane[3], "", "", EcfmpRestriction(), false, false, false);
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