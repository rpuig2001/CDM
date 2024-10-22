#include "stdafx.h"
#include "CDMSingle.hpp"
#include "pugixml.hpp"
#include "pugixml.cpp"
#include <thread>
#include "Delay.h"
#include "EcfmpRestriction.h"

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
int countTime;
int countFetchServerTime;
int countEcfmpTime;
int countTfcDisconnection;
int refreshTime;
bool addTime;
bool lvo;
bool ctotCid;
bool realMode;
bool remarksOption;
bool invalidateTSAT_Option;
string myTimeToAdd;
string rateUrl;
string taxiZonesUrl;
string ctotURL;
string cdmServerUrl;
int defTaxiTime;
string flowRestrictionsUrl;
string cdm_api;
string myAtcCallsign;
bool option_su_wait;
string apikey;

//Ftp data
string ftpHost;
string ftpUser;
string ftpPassword;
string vdgsFileType;

vector<Plane> slotList;
vector<EcfmpRestriction> ecfmpData;
vector<Plane> apiCtots;
vector<string> asatList;
vector<string> taxiTimesList;
vector<string> TxtTimesVector;
vector<string> OutOfTsat;
vector<string> colors;
vector<Rate> rate;
vector<string> planeAiportList;
vector<string> masterAirports;
vector<string> CDMairports;
vector<string> CTOTcheck;
vector<string> finalTimesList;
vector<string> disconnectionList;
vector<string> difeobttobtList;
vector<string> reaSent;
vector<string> reaCTOTSent;
vector<vector<string>> slotFile;
vector<vector<string>> evCtots;
vector<Delay> delayList;
vector<ServerRestricted> serverRestrictedPlanes;
vector<Plane> setTSATlater;
vector<string> setCdmStslater;
vector<string> suWaitList;
vector<string> checkCIDLater;
vector<string> disabledCtots;

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

	// Register Tag Item "CDM-TSAT"
	RegisterTagItemType("TSAT", TAG_ITEM_TSAT);
	RegisterTagItemType("TSAT/TOBT-DIFF", TAG_ITEM_TSAT_TOBT_DIFF);
	RegisterTagItemFunction("TSAT Delay", TAG_FUNC_CUSTOMTSAT);

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

	// Register Tag Item "CDM-CTOT"
	RegisterTagItemType("CTOT", TAG_ITEM_CTOT);
	RegisterTagItemFunction("CTOT Options", TAG_FUNC_CTOTOPTIONS);
	RegisterTagItemFunction("Get FM as text", TAG_FUNC_FMASTEXT);

	// Register Tag Item "CDM-EVENT-CTOT"
	RegisterTagItemType("EV-CTOT", TAG_ITEM_EV_CTOT);
	RegisterTagItemFunction("EvCTOT Options", TAG_FUNC_OPT_EvCTOT);
	RegisterTagItemFunction("EvCTOT to MANUAL CTOT", TAG_FUNC_EvCTOTtoCTOT);

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

	countTime = stoi(GetTimeNow());
	countFetchServerTime = stoi(GetTimeNow());
	countEcfmpTime = stoi(GetTimeNow());
	//countTime = stoi(GetTimeNow()) - refreshTime;
	addTime = false;

	countTfcDisconnection = -1;

	GetVersion();

	//Get data from xml config file
	//airport = getFromXml("/CDM/apt/@icao");
	//airport = getFromXml("/CDM/apt/@icao");
	defTaxiTime = stoi(getFromXml("/CDM/DefaultTaxiTime/@minutes"));
	refreshTime = stoi(getFromXml("/CDM/RefreshTime/@seconds"));
	expiredCTOTTime = stoi(getFromXml("/CDM/expiredCtot/@time"));
	string realModeStr = getFromXml("/CDM/realMode/@mode");
	rateString = getFromXml("/CDM/rate/@ops");
	lvoRateString = getFromXml("/CDM/rateLvo/@ops");
	rateUrl = getFromXml("/CDM/Rates/@url");
	taxiZonesUrl = getFromXml("/CDM/Taxizones/@url");
	ctotURL = getFromXml("/CDM/Ctot/@url");
	string invalidateTSAT_OptionStr = getFromXml("/CDM/invalidateAtTsat/@mode");
	string stringDebugMode = getFromXml("/CDM/Debug/@mode");
	flowRestrictionsUrl = getFromXml("/CDM/FlowRestrictions/@url");
	vdgsFileType = getFromXml("/CDM/vdgsFileType/@type");
	ftpHost = getFromXml("/CDM/ftpHost/@host");
	ftpUser = getFromXml("/CDM/ftpUser/@user");
	ftpPassword = getFromXml("/CDM/ftpPassword/@password");
	string opt_su_wait = getFromXml("/CDM/Su_Wait/@mode");

	option_su_wait = false;
	if (opt_su_wait == "true") {
		option_su_wait = true;
	}

	debugMode = false;
	if (stringDebugMode == "true") {
		debugMode = true;
		sendMessage("[DEBUG MESSAGE] - USING DEBUG MODE");
	}

	realMode = false;
	if (realModeStr == "true") {
		realMode = true;
	}

	//Invalidate FP at TSAT+6
	invalidateTSAT_Option = true;
	if (invalidateTSAT_OptionStr == "false") {
		invalidateTSAT_Option = false;
	}

	//Flow Data
	std::thread t(&CDM::getEcfmpData, this);
	t.detach();

	//CDM-Server
	cdmServerUrl = "https://cdm-server-production.up.railway.app";

	//CDM-Server Fetch restricted
	getCdmServerRestricted();

	//Init reamrksOption
	remarksOption = false;

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
	bool AtcMe = false;
	bool master = false;

	for (string apt : masterAirports)
	{
		if (apt == fp.GetFlightPlanData().GetOrigin()) {
			master = true;
		}
	}
	AtcMe = true;
	/*if (master) {
		if (fp.GetTrackingControllerIsMe() || strlen(fp.GetTrackingControllerId()) == 0) {
			AtcMe = true;
		}
	}*/

	if (FunctionId == TAG_FUNC_EDITEOBT)
	{
		if (master && AtcMe) {
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
			}
		}
	}
	else if (FunctionId == TAG_FUNC_EOBTTOTOBT) {
		addLogLine("TRIGGER - TAG_FUNC_EOBTTOTOBT");
		fp.GetControllerAssignedData().SetFlightStripAnnotation(0, formatTime(fp.GetFlightPlanData().GetEstimatedDepartureTime()).c_str());
		//Remove if added to not modify TOBT if EOBT changes List
		for (size_t i = 0; i < difeobttobtList.size(); i++) {
			if ((string)fp.GetCallsign() == difeobttobtList[i]) {
				difeobttobtList.erase(difeobttobtList.begin() + i);
			}
		}
	}
	else if (FunctionId == TAG_FUNC_ADDTSAC) {
		addLogLine("TRIGGER - TAG_FUNC_ADDTSAC");
		string annotTSAC = fp.GetControllerAssignedData().GetFlightStripAnnotation(2);
		string completeTOBT = (string)fp.GetControllerAssignedData().GetFlightStripAnnotation(0);
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
							fp.GetControllerAssignedData().SetFlightStripAnnotation(2, getTSAT.substr(0,4).c_str());
						}
					}
				}
			}
		}
		else {
			fp.GetControllerAssignedData().SetFlightStripAnnotation(2, "");
		}
	}

	else if (FunctionId == TAG_FUNC_EDITTSAC) {
		addLogLine("TRIGGER - TAG_FUNC_EDITTSAC");
		OpenPopupEdit(Area, TAG_FUNC_NEWTSAC, fp.GetControllerAssignedData().GetFlightStripAnnotation(2));
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
					fp.GetControllerAssignedData().SetFlightStripAnnotation(2, editedTSAC.c_str());
				}
			}
		}
	}

	else if (FunctionId == TAG_FUNC_TOGGLEASRT || FunctionId == TAG_FUNC_READYSTARTUP) {
		if (master && AtcMe) {
			addLogLine("TRIGGER - TAG_FUNC_READYSTARTUP");
			string annotAsrt = fp.GetControllerAssignedData().GetFlightStripAnnotation(1);
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

				fp.GetControllerAssignedData().SetFlightStripAnnotation(1, (hour + min).c_str());
			}
			else {
				fp.GetControllerAssignedData().SetFlightStripAnnotation(1, "");
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
		countTime = stoi(GetTimeNow()) - (refreshTime+5);
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
			string tsacvalue = fp.GetControllerAssignedData().GetFlightStripAnnotation(2);
			if (tsacvalue.empty()) {
				AddPopupListElement("Add TSAT to TSAC", "", TAG_FUNC_ADDTSAC, false, 2, false);
			}
			else {
				AddPopupListElement("Remove TSAC", "", TAG_FUNC_ADDTSAC, false, 2, false);
			}
			AddPopupListElement("Edit TSAC", "", TAG_FUNC_EDITTSAC, false, 2, false);
			AddPopupListElement("----------------", "", -1, false, 2, false);

			//ASRT OPTIONS
			string asrtvalue = fp.GetControllerAssignedData().GetFlightStripAnnotation(1);
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
		if (master && AtcMe) {
			addLogLine("TRIGGER - TAG_FUNC_OPT_EOBT");
			OpenPopupList(Area, "EOBT Options", 1);
			AddPopupListElement("Edit EOBT", "", TAG_FUNC_EDITEOBT, false, 2, false);
		}
	}

	else if (FunctionId == TAG_FUNC_OPT_TSAC) {
		if (master && AtcMe) {
			addLogLine("TRIGGER - TAG_FUNC_OPT_TSAC");
			OpenPopupList(Area, "TSAC Options", 1);
			string tsacvalue = fp.GetControllerAssignedData().GetFlightStripAnnotation(2);
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
		if (master && AtcMe) {
			addLogLine("TRIGGER - TAG_FUNC_READYTOBT");
			//SET SU_WAIT WHEN OPTION ENABLED
			if (option_su_wait) {
				suWaitList.push_back(fp.GetCallsign());
			}

			fp.GetControllerAssignedData().SetFlightStripAnnotation(0, formatTime(GetActualTime()).c_str());

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

			string annotAsrt = fp.GetControllerAssignedData().GetFlightStripAnnotation(1);
			if (annotAsrt.empty()) {
				fp.GetControllerAssignedData().SetFlightStripAnnotation(1, (hour + min).c_str());
			}

			//Add to not modify TOBT if EOBT changes List
			bool foundInEobtTobtList = false;
			for (size_t i = 0; i < difeobttobtList.size(); i++) {
				if ((string)fp.GetCallsign() == difeobttobtList[i]) {
					foundInEobtTobtList = true;
				}
			}
			if (!foundInEobtTobtList) {
				difeobttobtList.push_back(fp.GetCallsign());
			}
		}

		//Reset CTOT by CDM-Network
		for (size_t i = 0; i < slotList.size(); i++)
		{
			if (slotList[i].callsign == fp.GetCallsign()) {
				if (slotList[i].ctot != "") {
					slotList[i].ctot = "";
					slotList[i].flowReason = "";
					slotList[i].hasManualCtot = false;
					slotList[i].showData = true;
				}
			}
		}

		//Update times to slaves
		countTime = stoi(GetTimeNow()) - refreshTime;
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
							string myTaxiTime = getTaxiTime(lat, lon, fp.GetFlightPlanData().GetOrigin(), depRwy);
							string calculatedTOBT = calculateLessTime(editedCDT + "00", stod(myTaxiTime));
							// at the earlierst at present time + EXOT
							if (stoi(calculatedTOBT) > stoi(GetTimeNow())) {
								fp.GetControllerAssignedData().SetFlightStripAnnotation(0, calculatedTOBT.substr(0, 4).c_str());
								for (size_t i = 0; i < slotList.size(); i++)
								{
									if ((string)fp.GetCallsign() == slotList[i].callsign && !slotList[i].hasManualCtot) {
										slotList[i].ttot = editedCDT + "00";
									}
								}

								//Add to not modify TOBT if EOBT changes List
								bool foundInEobtTobtList = false;
								for (size_t i = 0; i < difeobttobtList.size(); i++) {
									if ((string)fp.GetCallsign() == difeobttobtList[i]) {
										foundInEobtTobtList = true;
									}
								}
								if (!foundInEobtTobtList) {
									difeobttobtList.push_back(fp.GetCallsign());
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
								string myTaxiTime = getTaxiTime(lat, lon, fp.GetFlightPlanData().GetOrigin(), depRwy);
								string calculatedTOBT = calculateLessTime(editedCTOT + "00", stod(myTaxiTime));
								// at the earlierst at present time + EXOT
								if (stoi(calculatedTOBT) > stoi(calculateTime(GetTimeNow(), 5))) {
									fp.GetControllerAssignedData().SetFlightStripAnnotation(0, calculatedTOBT.substr(0, 4).c_str());
									for (size_t i = 0; i < slotList.size(); i++)
									{
										if (slotList[i].callsign == fp.GetCallsign()) {
											slotList[i].hasManualCtot = true;
											addTimeToListForSpecificAirportAndRunway(10, calculateTime(GetTimeNow(), 5), fp.GetFlightPlanData().GetOrigin(), fp.GetFlightPlanData().GetDepartureRwy());
										}
									}
									//Add to not modify TOBT if EOBT changes List
									bool foundInEobtTobtList = false;
									for (size_t i = 0; i < difeobttobtList.size(); i++) {
										if ((string)fp.GetCallsign() == difeobttobtList[i]) {
											foundInEobtTobtList = true;
										}
									}
									if (!foundInEobtTobtList) {
										difeobttobtList.push_back(fp.GetCallsign());
									}
									//Update times to slaves
									countTime = stoi(GetTimeNow()) - refreshTime;
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
							string myTaxiTime = getTaxiTime(lat, lon, fp.GetFlightPlanData().GetOrigin(), depRwy);
							string calculatedTOBT = calculateLessTime(editedCTOT + "00", stod(myTaxiTime));
							// at the earlierst at present time (now + 5min) + EXOT
							if (stoi(calculatedTOBT) > stoi(calculateTime(GetTimeNow(), 5))) {
								fp.GetControllerAssignedData().SetFlightStripAnnotation(0, calculatedTOBT.substr(0, 4).c_str());
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
							//Add to not modify TOBT if EOBT changes List
							bool foundInEobtTobtList = false;
							for (size_t i = 0; i < difeobttobtList.size(); i++) {
								if ((string)fp.GetCallsign() == difeobttobtList[i]) {
									foundInEobtTobtList = true;
								}
							}
							if (!foundInEobtTobtList) {
								difeobttobtList.push_back(fp.GetCallsign());
							}
							//Update times to slaves
							countTime = stoi(GetTimeNow()) - refreshTime;
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
						//Check API
						slotList[i].showData = false;
						std::thread t(&CDM::setTSATApi, this, slotList[i].callsign, slotList[i].tsat);
						t.detach();
					}
				}
			}
		}
	}

	else if (FunctionId == TAG_FUNC_EDITTOBT) {
		if (master && AtcMe) {
			addLogLine("TRIGGER - TAG_FUNC_EDITTOBT");
			OpenPopupEdit(Area, TAG_FUNC_NEWTOBT, fp.GetControllerAssignedData().GetFlightStripAnnotation(0));
		}
	}
	else if (FunctionId == TAG_FUNC_NEWTOBT) {
		addLogLine("TRIGGER - TAG_FUNC_NEWTOBT");
		string editedTOBT = ItemString;
		if (fp.GetControllerAssignedData().GetFlightStripAnnotation(0) != editedTOBT) {
			bool hasNoNumber = true;
			if (editedTOBT.length() == 4) {
				for (size_t i = 0; i < editedTOBT.length(); i++) {
					if (isdigit(editedTOBT[i]) == false) {
						hasNoNumber = false;
					}
				}
				if (hasNoNumber) {
					bool found = false;
					for (size_t i = 0; i < slotList.size(); i++)
					{
						if (slotList[i].callsign == fp.GetCallsign()) {
							found = true;
							slotList.erase(slotList.begin() + i);
							fp.GetControllerAssignedData().SetFlightStripAnnotation(0, editedTOBT.c_str());
							//Update times to slaves
							countTime = stoi(GetTimeNow()) - refreshTime;
						}
					}
					if (!found) {
						fp.GetControllerAssignedData().SetFlightStripAnnotation(0, editedTOBT.c_str());
						//Update times to slaves
						countTime = stoi(GetTimeNow()) - refreshTime;
					}

					//Add to not modify TOBT if EOBT changes List
					bool foundInEobtTobtList = false;
					for (size_t i = 0; i < difeobttobtList.size(); i++) {
						if ((string)fp.GetCallsign() == difeobttobtList[i]) {
							foundInEobtTobtList = true;
						}
					}
					if (!foundInEobtTobtList) {
						difeobttobtList.push_back(fp.GetCallsign());
					}
				}
			}
			else if (editedTOBT.empty()) {
				fp.GetControllerAssignedData().SetFlightStripAnnotation(0, "");
				fp.GetControllerAssignedData().SetFlightStripAnnotation(1, "");
				for (size_t i = 0; i < slotList.size(); i++) {
					if ((string)fp.GetCallsign() == slotList[i].callsign) {
						slotList.erase(slotList.begin() + i);
						//Update times to slaves
						fp.GetControllerAssignedData().SetFlightStripAnnotation(3, "");
						PushToOtherControllers(fp);
					}
				}

				//Remove if added to not modify TOBT if EOBT changes List
				for (size_t i = 0; i < difeobttobtList.size(); i++) {
					if ((string)fp.GetCallsign() == difeobttobtList[i]) {
						difeobttobtList.erase(difeobttobtList.begin() + i);
					}
				}
				
				if (!realMode) {
					//Check API
					std::thread t(&CDM::setTSATApi, this, (string)fp.GetCallsign(), "");
					t.detach();
				}
			}
		}
	}

	else if (FunctionId == TAG_FUNC_CUSTOMTSAT) {
		addLogLine("TRIGGER - TAG_FUNC_CUSTOMTSAT");
		if (_stricmp(fp.GetFlightPlanData().GetDepartureRwy(), "") == 0) return;
		if (master)
			OpenPopupEdit(Area, TAG_FUNC_EDITFIRSTTSAT, "");
	}

	else if (FunctionId == TAG_FUNC_EDITFIRSTTSAT) {
		addLogLine("TRIGGER - TAG_FUNC_EDITFIRSTTSAT");
		string rwy = fp.GetFlightPlanData().GetDepartureRwy();
		string apt = fp.GetFlightPlanData().GetOrigin();
		string myTime = ItemString;

		//use myTime 9999 to remove delay for APT/RWY config
		if (myTime == "9999") {
			for (size_t i = 0; i < delayList.size(); i++) {
				if (delayList[i].airport == apt && delayList[i].rwy == rwy) {
					sendMessage("REMOVING DELAY " + apt + "/" + rwy);
					delayList.erase(delayList.begin() + i);
				}
			}
		}
		else {
			try {
				Delay d = Delay(apt, rwy, myTime);

				//Get Time now
				time_t rawtime;
				struct tm ptm;
				time(&rawtime);
				gmtime_s(&ptm, &rawtime);
				string hour = to_string(ptm.tm_hour % 24);
				string min = to_string(ptm.tm_min);

				int difTime = difftime(stoi(d.time), stoi(hour + min));

				if (difTime > 0) {
					sendMessage("Adding DELAY for " + apt + " rwy: " + rwy + " from time: " + myTime + "z.");
					delayList.push_back(d);
					addTimeToListForSpecificAirportAndRunway(difTime, GetTimeNow(), d.airport, d.rwy);
				}
				else {
					sendMessage("DELAY NOT ADDED. Time must be in the future");
				}
			}
			catch (...)
			{
				sendMessage("DELAY NOT ADDED. An error occured. Check time format.");
			}
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
	string tsat = FlightPlan.GetControllerAssignedData().GetFlightStripAnnotation(3);
	if (tsat.find("%") != string::npos) {
		disconnectionList.push_back(FlightPlan.GetCallsign());
		countTfcDisconnection = stoi(GetTimeNow());
	}
	else {
		RemoveDataFromTfc(FlightPlan.GetCallsign());
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

	string data3ToSend = FlightPlan.GetControllerAssignedData().GetFlightStripAnnotation(3);
	string remarks = FlightPlan.GetControllerAssignedData().GetFlightStripAnnotation(3);

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
			string tobt = FlightPlan.GetControllerAssignedData().GetFlightStripAnnotation(0);

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
							FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(0, formatTime(slotList[pos].eobt).c_str());
						}
						isValidToCalculateEventMode = true;
					}
				}
			}

			bool hasManualCtot = false;
			if (aircraftFind) {
				if (slotList[pos].hasManualCtot) {
					hasManualCtot = true;
				}
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
					if (remarks.find("%") != string::npos) {
						string stringToAdd = remarks.substr(0, remarks.find("%") - 1);
						FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(3, stringToAdd.c_str());
						remarks = stringToAdd;
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
						string myTaxiTime = getTaxiTime(lat, lon, origin, depRwy);
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
							FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(0, mySetEobt.c_str());
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
						if (callsign == OutOfTsat[i].substr(0, OutOfTsat[i].find(","))) {
							if (EOBTfinal.substr(0, 4) == OutOfTsat[i].substr(OutOfTsat[i].find(",") + 1, 4)) {
								stillOutOfTsat = true;
								stillOutOfTsatPos = i;
							}
							else {
								OutOfTsat.erase(OutOfTsat.begin() + i);
							}
						}
					}

					//Sync TOBT if different than EOBT
					if (!SU_ISSET) {
						if (tobt.length() > 0) {
							string mySetEobt = formatTime(FlightPlan.GetFlightPlanData().GetEstimatedDepartureTime());
							if (mySetEobt != tobt) {
								bool foundInEobtTobtList = false;
								for (size_t i = 0; i < difeobttobtList.size(); i++) {
									if (callsign == difeobttobtList[i]) {
										foundInEobtTobtList = true;
									}
								}

								if (!foundInEobtTobtList) {
									FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(0, formatTime(FlightPlan.GetFlightPlanData().GetEstimatedDepartureTime()).c_str());
								}
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
								ASATFound = true;
							}
						}
						else {
							if (correctState) {
								ASATtext = asatList[ASATpos].substr(asatList[ASATpos].length() - 4, 4);
							}
							else if (!correctState) {
								asatList.erase(asatList.begin() + ASATpos);
								ASATFound = false;
							}
						}

						if (ItemCode == TAG_ITEM_EOBT)
						{
							string myeobt = FlightPlan.GetFlightPlanData().GetEstimatedDepartureTime();
							string ShowEOBT = formatTime(myeobt);
							ItemRGB = TAG_EOBT;
							strcpy_s(sItemString, 16, ShowEOBT.c_str());
						}
						else if (ItemCode == TAG_ITEM_TOBT)
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
							string annotTSAC = FlightPlan.GetControllerAssignedData().GetFlightStripAnnotation(2);
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
						else if (ItemCode == TAG_ITEM_CTOT)
						{
							bool inreaList = false;
							for (string s : reaCTOTSent) {
								if (s == callsign) {
									inreaList = true;
								}
							}

							if (aircraftFind) {
								if (slotList[pos].hasManualCtot) {
									if (slotList[pos].ctot == "") {
										ItemRGB = TAG_ORANGE;
										strcpy_s(sItemString, 16, slotList[pos].ttot.substr(0, 4).c_str());
									}
									else {
										ItemRGB = TAG_CTOT;
										strcpy_s(sItemString, 16, slotList[pos].ttot.substr(0, 4).c_str());
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

							if (aircraftFind) {
								string tempEOBT = EOBT;
								if (tempEOBT != slotList[pos].eobt) {
									//aircraftFind false to recalculate Times due to fp change
									slotList.erase(slotList.begin() + pos);
									aircraftFind = false;
									if (remarks.find("%") != string::npos) {
										string stringToAdd = remarks.substr(0, remarks.find("%") - 1);
										FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(3, stringToAdd.c_str());
										remarks = stringToAdd;
									}
								}
							}

							if (!aircraftFind) {
									TSAT = EOBT;
									//TSAT
									string TSATstring = TSAT;
									TSATfinal = formatTime(TSATstring) + "00";

									//TTOT
									TTOTFinal = calculateTime(TSATstring, taxiTime);

									bool tempAddTime_DELAY = false;
									string myTimeToAddTemp_DELAY = "";
									for (Delay d : delayList) {
										if (d.airport == origin && d.rwy == depRwy) {
											tempAddTime_DELAY = true;
											myTimeToAddTemp_DELAY = d.time + "00";
										}
									}

									if (addTime || tempAddTime_DELAY) {
										//USE DELAY GIVEN TIMES OTHERWISE USE THE DEFAULT DELAY FUNC
										if (tempAddTime_DELAY) {
											string timeToAddHour = myTimeToAddTemp_DELAY.substr(0, 2);
											string timeToAddMin = myTimeToAddTemp_DELAY.substr(2, 2);
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
												TSATfinal = myTimeToAddTemp_DELAY;
												TTOTFinal = calculateTime(myTimeToAddTemp_DELAY, taxiTime);
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

							if (!aircraftFind) {
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

								while (equalTTOT) {
									correctTTOT = true;
									if (!hasManualCtot) {
										for (size_t t = 0; t < slotList.size(); t++)
										{
											string listTTOT;
											string listCallsign = slotList[t].callsign;
											string listDepRwy = "";
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
												string destFound = FlightPlanSelect(slotList[z].callsign.c_str()).GetFlightPlanData().GetDestination();
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
													sameDestList.push_back(slotList[z]);
												}
											}

											for (size_t z = 0; z < sameDestList.size(); z++)
											{
												CFlightPlan fpList = FlightPlanSelect(slotList[z].callsign.c_str());
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
											TSAT = TSATfinal.c_str();
											TTOT = TTOTFinal.c_str();
											bool doRequest = false;
											if (aircraftFind) {
												if (TTOTFinal != slotList[pos].ttot) {
													Plane p(callsign, EOBT, TSAT, TTOT, slotList[pos].ctot, slotList[pos].flowReason, myEcfmp, hasEcfmpRestriction, hasManualCtot, true);
													doRequest = true;
													slotList[pos] = p;
													remarks = setCTOTremarks(remarks, slotList[pos], FlightPlan);
												}
											}
											else {
												Plane p(callsign, EOBT, TSAT, TTOT, "", "", myEcfmp, hasEcfmpRestriction, hasManualCtot, true);
												doRequest = true;
												slotList.push_back(p);
												pos = getPlanePosition(callsign);
												remarks = setCTOTremarks(remarks, slotList[pos], FlightPlan);
											}
											//Check API
											if (doRequest) {
												for (size_t a = 0; a < slotList.size(); a++)
												{
													if (slotList[a].callsign == callsign) {
														slotList[a].showData = false;
													}
												}
												std::thread t(&CDM::setTSATApi, this, callsign, TSATfinal);
												t.detach();
											}
										}
									}
								}
							}

							if (ItemCode == TAG_ITEM_TSAT || ItemCode == TAG_ITEM_TSAT_TOBT_DIFF) {
								//Sync TTOT
								if (aircraftFind && remarks.find("%") != string::npos) {
									string TTOT1 = slotList[pos].ttot;
									string TTOT2 = remarks.substr(remarks.find("%") + 8, 6);
									if (TTOT1 != TTOT2) {
										string stringToAdd = "%" + slotList[pos].tsat + "|" + slotList[pos].ttot;
										FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(3, stringToAdd.c_str());
										remarks = stringToAdd;
									}
								}
								else if (aircraftFind && !stsDepa) {
									string myTSAT = TSAT;
									string myTTOT = TTOT;
									string stringToAdd = "%" + myTSAT + "|" + myTTOT;
									FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(3, stringToAdd.c_str());
									remarks = stringToAdd;
								}

								//Set ASRT if SU_ISSET
								if (SU_ISSET) {
									string myASRTText = FlightPlan.GetControllerAssignedData().GetFlightStripAnnotation(1);
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

										FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(1, (hour + min).c_str());
									}
								}
							}

						string ASRTtext = FlightPlan.GetControllerAssignedData().GetFlightStripAnnotation(1);

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

							if (oldTOBT) {
								OutOfTsat.push_back(callsign + "," + EOBT);
								FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(1, "");
								if (remarks.find("%") != string::npos) {
									string stringToAdd = remarks.substr(0, remarks.find("%") - 1);
									FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(3, stringToAdd.c_str());
									remarks = stringToAdd;
								}
								//Update CDM-API
								std::thread t(&CDM::setCdmSts, this, callsign, "I");
								t.detach();
							}
						}

						//If oldTSAT
						string TSAThour = TSATfinal.substr(TSATfinal.length() - 6, 2);
						string TSATmin = TSATfinal.substr(TSATfinal.length() - 4, 2);

						bool oldTSAT = false;
						bool moreLessFive = false;
						bool lastMinute = false;
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

						if (oldTSAT && !correctState && !oldTOBT && invalidateTSAT_Option) {
							OutOfTsat.push_back(callsign + "," + EOBT);
							FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(1, "");
							if (remarks.find("%") != string::npos) {
								string stringToAdd = remarks.substr(0, remarks.find("%") - 1);
								FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(3, stringToAdd.c_str());
								remarks = stringToAdd;
							}
							//Update CDM-API
							std::thread t(&CDM::setCdmSts, this, callsign, "I");
							t.detach();
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
							string annotTSAC = FlightPlan.GetControllerAssignedData().GetFlightStripAnnotation(2);

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
							string annotTSAC = FlightPlan.GetControllerAssignedData().GetFlightStripAnnotation(2);

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
									if (slotList[pos].hasEcfmpRestriction) {
										ShowTSAT += "R";
									}

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
									string value = getDiffNowTSAT(slotList[pos].tsat);

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
						else if (ItemCode == TAG_ITEM_ASAT)
						{
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
								ASATFound = true;
							}
						}
						else {
							if (correctState) {
								ASATtext = asatList[ASATpos].substr(asatList[ASATpos].length() - 4, 4);
							}
							else if (!correctState) {
								asatList.erase(asatList.begin() + ASATpos);
								ASATFound = false;
							}
						}

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
							string ASRTtext = FlightPlan.GetControllerAssignedData().GetFlightStripAnnotation(1);
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
							string ASRTtext = FlightPlan.GetControllerAssignedData().GetFlightStripAnnotation(1);
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
								bool inreaList = false;
								for (string s : reaCTOTSent) {
									if (s == callsign) {
										inreaList = true;
									}
								}

								if (aircraftFind) {
									if (slotList[pos].hasManualCtot) {
										if (slotList[pos].ctot == "") {
											ItemRGB = TAG_ORANGE;
										}
										else {
											ItemRGB = TAG_CTOT;
										}
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

						//Refresh CDM API every 30 seconds
						int myNow = stoi(GetTimeNow());

						//Remove disconnected planes after 5 min disconnected
						if (countTfcDisconnection != -1) {
							if (myNow - countTfcDisconnection > 300) {
								countTfcDisconnection = -1;
								disconnectTfcs();
								pos = getPlanePosition(callsign);
								if (pos == -1) {
									aircraftFind = false;
								}
							}
						}

						if (myNow - countFetchServerTime > 30) {
							countFetchServerTime = myNow;
							addLogLine("[AUTO] - REFRESH API");
							std::thread t(&CDM::getCdmServerRestricted, this);
							t.detach();
							if (debugMode) {
								sendMessage("[DEBUG MESSAGE] - REFRESHING CDM API DATA");
							}
						}

						//Refresh ecfmpData every 5 min
						if (myNow - countEcfmpTime > 300) {
							countEcfmpTime = myNow;
							addLogLine("[AUTO] - REFRESH ECFMP");
							std::thread t(&CDM::getEcfmpData, this);
							t.detach();
							if (debugMode) {
								sendMessage("[DEBUG MESSAGE] - REFRESHING FLOW DATA");
							}
						}

						//Refresh times every x sec
						if (myNow - countTime > refreshTime) {
							countTime = myNow;
							addLogLine("[AUTO] - REFRESH CDM INTERNAL DATA");
							//Order list according TTOT
							slotList = recalculateSlotList(slotList);
							pos = getPlanePosition(callsign);

							for (size_t t = 0; t < slotList.size(); t++) {
								PushToOtherControllers(FlightPlanSelect(slotList[t].callsign.c_str()));
							}

							std::thread t(&CDM::saveData, this);
							t.detach();
							if (debugMode) {
								sendMessage("[DEBUG MESSAGE] - REFRESHING");
								sendMessage("[DEBUG MESSAGE] - " + to_string(slotList.size()) + " Planes in the list");
							}

							//Execute background process in the background
							std::thread t1(&CDM::backgroundProcess_recaulculate, this);
							t1.detach();
						}
					}
				}
				else {
					// Refresh FlowData every 30 seconds
					int myNow = stoi(GetTimeNow());
					if (myNow - countFetchServerTime > 30) {
						countFetchServerTime = myNow;
						std::thread t(&CDM::getCdmServerRestricted, this);
						t.detach();
						if (debugMode) {
							sendMessage("[DEBUG MESSAGE] - REFRESHING FLOW DATA");
						}
					}

					bool TSATFind = false;
					string TSATString, TTOTString;
					size_t posPercent = remarks.find("%");
					if (posPercent != string::npos) {
						TSATString = remarks.substr(posPercent + 1, 6);
						TTOTString = remarks.substr(posPercent + 8, 6);
						TSATFind = true;
					}

					if (aircraftFind) {
						if (!TSATFind) {
							if (pos < slotList.size()) { // Check if pos is within bounds
								slotList.erase(slotList.begin() + pos);
							}
						}
						else if (TSATString != slotList[pos].tsat) {
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

					bool foundIndifeobttobtList = false;
					for (const string& s : difeobttobtList) {
						if (s == callsign) {
							foundIndifeobttobtList = true;
							break; // Break the loop once found
						}
					}
					string checkEOBT = formatTime(FlightPlan.GetFlightPlanData().GetEstimatedDepartureTime());
					string checkTOBT = FlightPlan.GetControllerAssignedData().GetFlightStripAnnotation(0);
					if (!foundIndifeobttobtList) {
						if (checkEOBT != checkTOBT && !checkTOBT.empty()) {
							difeobttobtList.push_back(FlightPlan.GetCallsign());
						}
					}
					else {
						if (checkEOBT == checkTOBT) {
							difeobttobtList.erase(
								remove(difeobttobtList.begin(), difeobttobtList.end(), callsign),
								difeobttobtList.end()
							);
						}
					}

					// If oldTSAT
					if (TSATFind) {
						string TSAThour = TSATString.substr(TSATString.length() - 6, 2);
						string TSATmin = TSATString.substr(TSATString.length() - 4, 2);

						bool oldTSAT = false;
						bool moreLessFive = false;
						bool lastMinute = false;
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
							OutOfTsat.push_back(callsign + "," + EOBT);
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

						// TSAC
						bool TSACNotTSAT = false;
						string annotTSAC = FlightPlan.GetControllerAssignedData().GetFlightStripAnnotation(2);

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
							string annotTSAC = FlightPlan.GetControllerAssignedData().GetFlightStripAnnotation(2);
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
								if (slotList[pos].hasEcfmpRestriction) {
									TSATString += "R";
								}
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
								string value = getDiffNowTSAT(slotList[pos].tsat);
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
							string ASRTtext = FlightPlan.GetControllerAssignedData().GetFlightStripAnnotation(1);
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
							string ASRTtext = FlightPlan.GetControllerAssignedData().GetFlightStripAnnotation(1);
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
							if (aircraftFind) {
								if (slotList[pos].hasManualCtot) {
									string message = "MAN ACT";
									if (slotList[pos].ctot != "") {
										message = slotList[pos].flowReason;
									}
									ItemRGB = TAG_YELLOW;
									strcpy_s(sItemString, 16, message.c_str());
								}
							}
						}
						else if (ItemCode == TAG_ITEM_CTOT)
						{
							bool inreaList = false;
							for (string s : reaCTOTSent) {
								if (s == callsign) {
									inreaList = true;
								}
							}

							if (aircraftFind) {
								if (slotList[pos].hasManualCtot) {
									if (slotList[pos].ctot == "") {
										ItemRGB = TAG_ORANGE;
									}
									else {
										ItemRGB = TAG_CTOT;
									}
									strcpy_s(sItemString, 16, slotList[pos].ttot.substr(0, 4).c_str());
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
					}
					else
					{
						if (ItemCode == TAG_ITEM_EOBT)
						{
							string ShowEOBT = formatTime(FlightPlan.GetFlightPlanData().GetEstimatedDepartureTime());
							ItemRGB = TAG_EOBT;
							strcpy_s(sItemString, 16, ShowEOBT.c_str());
						}
						else if (ItemCode == TAG_ITEM_TOBT)
						{
							ItemRGB = TAG_GREY;
							strcpy_s(sItemString, 16, "----");
						}
						else if (ItemCode == TAG_ITEM_CTOT)
						{
							bool inreaList = false;
							for (string s : reaCTOTSent) {
								if (s == callsign) {
									inreaList = true;
								}
							}

							if (aircraftFind) {
								if (slotList[pos].hasManualCtot) {
									if (slotList[pos].ctot == "") {
										ItemRGB = TAG_ORANGE;
									}
									else {
										ItemRGB = TAG_CTOT;
									}
									strcpy_s(sItemString, 16, slotList[pos].ttot.substr(0, 4).c_str());
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
					strcpy_s(sItemString, 16, EOBTfinal.c_str());
				}
				else if (ItemCode == TAG_ITEM_TOBT)
				{
					ItemRGB = TAG_GREY;
					strcpy_s(sItemString, 16, "----");
				}
				else if (ItemCode == TAG_ITEM_CTOT)
				{
					bool inreaList = false;
					for (string s : reaCTOTSent) {
						if (s == callsign) {
							inreaList = true;
						}
					}

					if (aircraftFind) {
						if (slotList[pos].hasManualCtot) {
							if (slotList[pos].ctot == "") {
								ItemRGB = TAG_ORANGE;
							}
							else {
								ItemRGB = TAG_CTOT;
							}
							strcpy_s(sItemString, 16, slotList[pos].ttot.substr(0, 4).c_str());
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
			}

			if (ItemRGB != 0xFFFFFFFF)
			{
				*pColorCode = TAG_COLOR_RGB_DEFINED;
				*pRGB = ItemRGB;
			}
		}
		else {
			string EOBTstring = FlightPlan.GetFlightPlanData().GetEstimatedDepartureTime();
			string EOBTfinal = formatTime(EOBTstring);

			if (ItemCode == TAG_ITEM_EOBT)
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
			if (ItemCode == TAG_ITEM_FLOW_MESSAGE)
			{
				for (ServerRestricted sr : serverRestrictedPlanes) {
					if (sr.callsign == (string)FlightPlan.GetCallsign()) {
						ItemRGB = TAG_YELLOW;
						strcpy_s(sItemString, 50, sr.reason.c_str());
					}
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
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);
		result = curl_easy_perform(curl);
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
		curl_easy_cleanup(curl);
	}

	if (responseCode == 404 || responseCode == 401 || CURLE_OK != result) {
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

void CDM::backgroundProcess_recaulculate() {
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
				string testTsat = slotList[i].tsat;
				if (testTsat.length() >= 4) {
					if (
						(string)FlightPlanSelect(slotList[i].callsign.c_str()).GetGroundState() != "STUP" &&
						(string)FlightPlanSelect(slotList[i].callsign.c_str()).GetGroundState() != "ST-UP" &&
						(string)FlightPlanSelect(slotList[i].callsign.c_str()).GetGroundState() != "PUSH" &&
						(string)FlightPlanSelect(slotList[i].callsign.c_str()).GetGroundState() != "TAXI" &&
						(string)FlightPlanSelect(slotList[i].callsign.c_str()).GetGroundState() != "DEPA")
					{
						FlightPlanSelect(slotList[i].callsign.c_str()).GetControllerAssignedData().SetScratchPadString(testTsat.substr(0, 4).c_str());
					}
				}
			}

			string myCallsign = slotList[i].callsign;

			string myTTOT, myTSAT, myEOBT, myAirport, myDepRwy = "", myRemarks;
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

				myRemarks = myFlightPlan.GetControllerAssignedData().GetFlightStripAnnotation(3);

				myEOBT = slotList[i].eobt;

				Rate dataRate = rateForRunway(myFlightPlan.GetFlightPlanData().GetOrigin(), myFlightPlan.GetFlightPlanData().GetDepartureRwy());

				bool tempAddTime_DELAY = false;
				string myTimeToAddTemp_DELAY = "";
				for (Delay d : delayList) {
					if (d.airport == myFlightPlan.GetFlightPlanData().GetOrigin() && d.rwy == myFlightPlan.GetFlightPlanData().GetDepartureRwy()) {
						tempAddTime_DELAY = true;
						myTimeToAddTemp_DELAY = d.time + "00";
					}
				}

				myTSAT = myEOBT;
				myTTOT = calculateTime(myEOBT, myTTime);
				if (addTime || tempAddTime_DELAY) {
					if (tempAddTime_DELAY) {
						string timeToAddHour = myTimeToAddTemp_DELAY.substr(0, 2);
						string timeToAddMin = myTimeToAddTemp_DELAY.substr(2, 2);
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
							myTSAT = myTimeToAddTemp_DELAY;
							myTTOT = calculateTime(myTimeToAddTemp_DELAY, myTTime);
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

				Plane item = refreshTimes(slotList[i], tempSlotList, myFlightPlan, myCallsign, myEOBT, myTSAT, myTTOT, myAirport, myTTime, myRemarks, myDepRwy, dataRate, true);
				tempSlotList.push_back(item);
				//refreshTimes(myFlightPlan, myCallsign, myEOBT, myTSAT, myTTOT, myAirport, myTTime, myRemarks, myDepRwy, dataRate, myhasCTOT, myCtotPos, i, true);
		}
		else {
			tempSlotList.push_back(slotList[i]);
		}
	}
	for (Plane p : tempSlotList) {
		for (int d = 0; d < slotList.size(); d++) {
			if (p.callsign == slotList[d].callsign) {
				slotList[d] = p;
			}
		}
	}
	addLogLine("[AUTO] - Finished CDM recalculation process");
	}
	catch (const std::exception& e) {
		addLogLine("ERROR: Unhandled exception backgroundProcess_recaulculate: " + (string)e.what());
	}
	catch (...) {
		addLogLine("ERROR: Unhandled exception backgroundProcess_recaulculate");
	}
}

Plane CDM::refreshTimes(Plane plane, vector<Plane> planes, CFlightPlan FlightPlan, string callsign, string EOBT, string TSATfinal, string TTOTFinal, string origin, int taxiTime, string remarks, string depRwy, Rate dataRate, bool aircraftFind) {
	addLogLine("Called refreshTimes...");
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

			while (equalTTOT) {
				correctTTOT = true;
				for (size_t t = 0; t < planes.size(); t++)
				{
					string listTTOT;
					string listCallsign = planes[t].callsign;
					string listDepRwy = "";
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
							string destFound = FlightPlanSelect(planes[z].callsign.c_str()).GetFlightPlanData().GetDestination();
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
								sameDestList.push_back(planes[z]);
							}
						}

						for (size_t z = 0; z < sameDestList.size(); z++)
						{
							CFlightPlan fpList = FlightPlanSelect(planes[z].callsign.c_str());
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
						string TSAT = TSATfinal.c_str();
						string TTOT = TTOTFinal.c_str();
						if (plane.hasManualCtot) {
							if (aircraftFind) {
								if (TTOTFinal != plane.ttot && TTOTFinal.length() >= 4) {
									Plane p(callsign, EOBT, TSAT, TTOT, plane.ctot, myFlow, myEcfmp, hasEcfmpRestriction, plane.hasManualCtot, true);
									plane = p;
									doRequest = true;
									remarks = setCTOTremarks(remarks, p, FlightPlan);
								}
							}
							else if (TTOTFinal.length() >= 4) {
								Plane p(callsign, EOBT, TSAT, TTOT, plane.ctot, myFlow, myEcfmp, hasEcfmpRestriction, plane.hasManualCtot, true);
								plane = p;
								remarks = setCTOTremarks(remarks, p, FlightPlan);
							}
						}
						else {
							if (aircraftFind) {
								if (TTOTFinal != plane.ttot && TTOTFinal.length() >= 4) {
									Plane p(callsign, EOBT, TSAT, TTOT, plane.ctot, myFlow, myEcfmp, hasEcfmpRestriction, plane.hasManualCtot, true);
									plane = p;
									doRequest = true;
									remarks = setCTOTremarks(remarks, p, FlightPlan);
								}
							}
							else if (TTOTFinal.length() >= 4) {
								Plane p(callsign, EOBT, TSAT, TTOT, plane.ctot, myFlow, myEcfmp, hasEcfmpRestriction, plane.hasManualCtot, true);
								plane = p;
								remarks = setCTOTremarks(remarks, p, FlightPlan);
							}
						}
						//Check API
						if (doRequest && TSATfinal.length() >= 4) {
							std::thread t(&CDM::setTSATApi, this, callsign, TSATfinal);
							t.detach();
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
			fp.GetControllerAssignedData().SetFlightStripAnnotation(0, "");
			fp.GetControllerAssignedData().SetFlightStripAnnotation(1, "");
			fp.GetControllerAssignedData().SetFlightStripAnnotation(2, "");
			fp.GetControllerAssignedData().SetFlightStripAnnotation(3, "");
		}
	}
	catch (const std::exception& e) {
		addLogLine("ERROR: Unhandled exception deleteFlightStrips: " + (string)e.what());
	}
	catch (...) {
		addLogLine("ERROR: Unhandled exception deleteFlightStrips");
	}
}

string CDM::getTaxiTime(double lat, double lon, string origin, string depRwy) {
	string line, TxtOrigin, TxtDepRwy, TxtTime;
	CPosition p1, p2, p3, p4;
	smatch match;

	try
	{
		for (size_t t = 0; t < TxtTimesVector.size(); t++)
		{
			if (regex_match(TxtTimesVector[t], match, regex("([A-Z]{4}):(\\d{2}[LRC]?):([^:]+):([^:]+):([^:]+):([^:]+):([^:]+):([^:]+):([^:]+):([^:]+):(\\d+)", regex::icase)))
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

				if (inPoly(4, LatArea, LonArea, lat, lon) % 2 != 0)
					return match[11];
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
		if (timeString.length() <= 0) {
			timeString = "0000" + timeString;
		}
		else if (timeString.length() <= 1) {
			timeString = "000" + timeString;
		}
		else if (timeString.length() <= 2) {
			timeString = "00" + timeString;
		}
		else if (timeString.length() <= 3) {
			timeString = "0" + timeString;
		}
		int hours = stoi(timeString.substr(0, 2));
		int mins = stoi(timeString.substr(2, 2));
		int sec = stoi(timeString.substr(4, timeString.length() - 1));

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
	countTime = stoi(GetTimeNow()) - (refreshTime);
}

void CDM::addTimeToList(int timeToAdd, string minTSAT) {
	try {
		vector<Plane> mySlotList = slotList;

		for (size_t i = 0; i < mySlotList.size(); i++) {
			if (!mySlotList[i].hasManualCtot) {
				CFlightPlan myFp = FlightPlanSelect(mySlotList[i].callsign.c_str());
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

		slotList = mySlotList;
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

		slotList = mySlotList;
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
	if (timeString.length() <= 0) {
		timeString = "0000" + timeString;
	}
	else if (timeString.length() <= 1) {
		timeString = "000" + timeString;
	}
	else if (timeString.length() <= 2) {
		timeString = "00" + timeString;
	}
	else if (timeString.length() <= 3) {
		timeString = "0" + timeString;
	}
	int hours = stoi(timeString.substr(0, 2));
	int mins = stoi(timeString.substr(2, 2));
	int sec = stoi(timeString.substr(4, timeString.length() - 1));

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
	//Remove if added to not modify TOBT if EOBT changes List
	for (size_t i = 0; i < difeobttobtList.size(); i++) {
		if (callsign == difeobttobtList[i]) {
			if (debugMode) {
				sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 4");
			}
			difeobttobtList.erase(difeobttobtList.begin() + i);
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
			//Update CDM-API
			std::thread t(&CDM::setCdmSts, this, callsign, "");
			t.detach();
		}
	}
	//Remove from setTSATlater
	for (size_t i = 0; i < setTSATlater.size(); i++)
	{
		if (callsign == setTSATlater[i].callsign) {
			if (debugMode) {
				sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 11");
			}
			setTSATlater.erase(setTSATlater.begin() + i);
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
	for (size_t i = 0; i < setCdmStslater.size(); i++)
	{
		if (callsign == setCdmStslater[i]) {
			if (debugMode) {
				sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 13");
			}
			setCdmStslater.erase(setCdmStslater.begin() + i);
		}
	}
	//Remove from disabledCtots
	for (size_t i = 0; i < disabledCtots.size(); i++)
	{
		if (callsign == disabledCtots[i]) {
			if (debugMode) {
				sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 14");
			}
			disabledCtots.erase(disabledCtots.begin() + i);
		}
	}
	//Remove from checkCIDLater
	for (size_t i = 0; i < checkCIDLater.size(); i++)
	{
		if (callsign == checkCIDLater[i]) {
			if (debugMode) {
				sendMessage("[DEBUG MESSAGE] - " + callsign + " REMOVED 15");
			}
			checkCIDLater.erase(checkCIDLater.begin() + i);
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
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);
			result = curl_easy_perform(curl);
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
			curl_easy_cleanup(curl);
		}

		if (responseCode == 404 || responseCode == 401 || CURLE_OK != result) {
			// handle error 404
			sendMessage("UNABLE TO LOAD ECFMP DATA...");
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
						valueMeasure = stoi(valueMeasureString);
					}
				}
				//Get Filters
				vector<string> ADEP;
				vector<string> ADES;

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
				}

				EcfmpRestriction flow(id, ident, event_id, reason, valid_time, valid_date, typeMeasure, valueMeasure, ADEP, ADES);
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

void CDM::saveData() {
	addLogLine("Called saveData...");
	try{
	if (!ftpHost.empty()) {
		if (!slotList.empty()) {
			for (string airport : masterAirports) {
				//Type2 -> https://fs.nool.ee/MSFS/VDGS/Specs/DATALINK.txt
				if (vdgsFileType == "2" || vdgsFileType == "3") {
					string fileName = dfad + "_" + airport + ".json";
					createJsonVDGS(slotList, fileName, airport);
				}
				if (vdgsFileType == "1" || vdgsFileType == "3") {
					ofstream myfile;
					string fileName = dfad + "_" + airport + ".txt";
					myfile.open(fileName, std::ofstream::out | std::ofstream::trunc);
					for (Plane plane : slotList) {
						if (myfile.is_open())
						{
							if (airport == FlightPlanSelect(plane.callsign.c_str()).GetFlightPlanData().GetOrigin()) {

								string str;
								if (plane.hasManualCtot && plane.ctot != "" && plane.ttot.length() >= 4) {
									str = plane.callsign + "," + plane.eobt + "," + plane.tsat + "," + plane.ttot + "," + plane.ttot.substr(0, 4) + "," + plane.flowReason + ",";
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
			if (FlightPlanSelect(plane.callsign.c_str()).GetFlightPlanData().GetOrigin() == airport) {
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
	addLogLine("Called upload...");
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
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);
		result = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
	}

	if (!readBuffer.empty() && readBuffer.find(MY_PLUGIN_VERSION) == std::string::npos) {
		string DisplayMsg = "Please UPDATE YOUR CDM PLUGIN, version " + readBuffer + " is OUT! You have version " + MY_PLUGIN_VERSION " installed, download it from vats.im/CDM";
		DisplayUserMessage(MY_PLUGIN_NAME, "UPDATE", DisplayMsg.c_str(), true, false, false, false, false);
	}

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
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);
			result = curl_easy_perform(curl);
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
			curl_easy_cleanup(curl);
		}

		if (responseCode == 404 || responseCode == 401 || CURLE_OK != result) {
			// handle error 404
			sendMessage("UNABLE TO LOAD CTOTs FROM VATCAN...");
		}
		else {
			std::istringstream is(readBuffer);

			//Get data from .txt file
			string lineValue;
			while (getline(is, lineValue))
			{
				addVatcanCtotToEvCTOT(lineValue);
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
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);
			result = curl_easy_perform(curl);
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
			curl_easy_cleanup(curl);
		}

		if (responseCode == 404 || responseCode == 401 || CURLE_OK != result) {
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

string CDM::setCTOTremarks(string remarks, Plane plane, CFlightPlan FlightPlan) {
	try{
	string stringToAdd = remarks;
	if (remarks.find("%") != string::npos) {
		stringToAdd = remarks.substr(0, remarks.find("%") - 1);
		remarks = stringToAdd;
	}

	stringToAdd = "%" + plane.tsat + "|" + plane.ttot;

	FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(3, stringToAdd.c_str());
	remarks = stringToAdd;
	return remarks;
	}
	catch (const std::exception& e) {
		addLogLine("ERROR: Unhandled exception setCTOTremarks: " + (string)e.what());
		return remarks;
	}
	catch (...) {
		addLogLine("ERROR: Unhandled exception setCTOTremarks");
		return remarks;
	}
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

string CDM::getDiffNowTSAT(string TSAT) {
	try {
		string timeNow = GetTimeNow();
		if (TSAT.length() < 4 || timeNow.length() < 4) {
			return "0";
		}
		if (timeNow.substr(0, 4) == TSAT.substr(0, 4)) {
			return "0";
		}

		int tsat_hours = stoi(TSAT.substr(0, 2));
		int tsat_minutes = stoi(TSAT.substr(2, 2));
		int timeNow_hours = stoi(timeNow.substr(0, 2));
		int timeNow_minutes = stoi(timeNow.substr(2, 2));

		int tsat_total_minutes = tsat_hours * 60 + tsat_minutes;
		int timeNow_total_minutes = timeNow_hours * 60 + timeNow_minutes;

		if (TSAT > timeNow) {
			return to_string(timeNow_total_minutes - tsat_total_minutes);
		}
		return "+" + to_string(timeNow_total_minutes - tsat_total_minutes);
	}
	catch (const std::exception& e) {
		addLogLine("ERROR: Unhandled exception getDiffNowTSAT: " + (string)e.what());
		return "0";
	}
	catch (...) {
		addLogLine("ERROR: Unhandled exception getDiffNowTSAT");
		return "0";
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
		countTime = stoi(GetTimeNow()) - refreshTime;
		countFetchServerTime = stoi(GetTimeNow()) - 60;
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
		sendMessage("CDM Commands: .cdm reload - .cdm ctot - .cdm master {airport} - .cdm slave {airport} - .cdm refreshtime {seconds} - .cdm customdelay {icao}/{rwy} {start_time} - .cdm lvo - .cdm realmode - .cdm remarks - .cdm rates - .cdm help");
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

	if (startsWith(".cdm customdelay", sCommandLine))
	{
		try {
			addLogLine(sCommandLine);
			string line = sCommandLine;
			string apt = line.substr(line.find("/") - 4, 4);
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
							sendMessage("REMOVING DELAY " + apt + "/" + rwy);
							delayList.erase(delayList.begin() + i);
						}
					}
				}
				else {
					Delay d = Delay(apt, rwy, myTime);

					//Get Time now
					time_t rawtime;
					struct tm ptm;
					time(&rawtime);
					gmtime_s(&ptm, &rawtime);
					string hour = to_string(ptm.tm_hour % 24);
					string min = to_string(ptm.tm_min);

					int difTime = difftime(stoi(d.time), stoi(hour + min));

					if (difTime > 0) {
						sendMessage("Adding DELAY for " + apt + " rwy: " + rwy + " from time: " + myTime + "z.");
						delayList.push_back(d);
						addTimeToListForSpecificAirportAndRunway(difTime, GetTimeNow(), d.airport, d.rwy);
					}
					else {
						sendMessage("DELAY NOT ADDED. Time must be in the future");
					}
				}
			}
			else {
				sendMessage("Wrong time formatting to add delay. Please use time in 4 digits format (1234) or minutes with 1 or 2 digits codes (12 or 1)");
			}

			return true;
		}
		catch (const std::exception& e) {
			addLogLine("ERROR: Unhandled exception .cdm customdelay: " + (string)e.what());
			return true;
		}
		catch (...) {
			addLogLine("ERROR: Unhandled exception .cdm customdelay");
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
	
	if (startsWith(".cdm delay", sCommandLine))
	{
		addLogLine(sCommandLine);
		sendMessage("Command disabled, please check the customdelay command from the documentation.");
		/*
		//Get Time NOW
		time_t rawtime;
		struct tm* ptm;
		time(&rawtime);
		ptm = gmtime(&rawtime);
		string hour = to_string(ptm->tm_hour % 24);
		string min = to_string(ptm->tm_min);

		if (stoi(min) < 10) {
			min = "0" + min;
		}
		if (stoi(hour) < 10) {
			hour = "0" + hour.substr(0, 1);
		}

		string line = sCommandLine, timeAdded;

		if (line.substr(line.length() - 2, 1) == " ") {
			timeAdded = line.substr(line.length() - 1);

			if (isNumber(timeAdded)) {
				myTimeToAdd = calculateTime(hour + min + "00", stoi(timeAdded));
				sendMessage("Delay added: " + timeAdded + " minutes. PLEASE WAIT UNTIL CDM REFRESH TO SEE THE CHANGES!");
				addTime = true;
				addTimeToList(stoi(timeAdded), hour + min + "00");
			}
		}
		else if(line.substr(line.length() - 3, 1) == " ") {
			timeAdded = line.substr(line.length() - 2);

			if (isNumber(timeAdded)) {
				myTimeToAdd = calculateTime(hour + min + "00", stoi(timeAdded));
				sendMessage("Delay added: " + timeAdded + " minutes. PLEASE WAIT UNTIL CDM REFRESH TO SEE THE CHANGES!");
				addTime = true;
				addTimeToList(stoi(timeAdded), hour + min + "00");
			}
		}*/
		return true;
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

//API requests

bool CDM::setMasterAirport(string airport, string position) {
	addLogLine("Called setMasterAirport...");
	try{
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
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);
		result = curl_easy_perform(curl);
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
		curl_easy_cleanup(curl);
	}

	if (responseCode == 404 || responseCode == 401 || CURLE_OK != result) {
		addLogLine("UNABLE TO CONNECT CDM-API...");
		masterAirports.push_back(airport);
		sendMessage("Successfully set master airport (Locally only) " + airport);
		addLogLine("Successfully set master airport (Locally only) " + airport);
	}
	else {
		std::istringstream is(readBuffer);
		//Get data from .txt file
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
						addLogLine("Exception in setMasterAirport()");
					}
				}
			}
			sendMessage("Unable to set master airport " + airport);
			addLogLine("Unable to set master airport " + airport);
		}
	}
	return false;
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

bool CDM::removeMasterAirport(string airport, string position) {
	addLogLine("Call - Remove Master airport " + airport + "(" + position + ")");
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
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);
		result = curl_easy_perform(curl);
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
		curl_easy_cleanup(curl);
	}

	if (responseCode == 404 || responseCode == 401 || CURLE_OK != result) {
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
	return false;
}

bool CDM::removeAllMasterAirports(string position) {
	addLogLine("Called removeAllMasterAirports...");
	try{
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
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);
		result = curl_easy_perform(curl);
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
		curl_easy_cleanup(curl);
	}

	if (responseCode == 404 || responseCode == 401 || CURLE_OK != result) {
		addLogLine("UNABLE TO CONNECT CDM-API...");
	}
	else {
		std::istringstream is(readBuffer);
		//Get data from .txt file
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

void CDM::removeAllMasterAirportsByAirport(string airport) {
	addLogLine("Called removeAllMasterAirportsByAirport...");
	try{
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
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);
		result = curl_easy_perform(curl);
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
		curl_easy_cleanup(curl);
	}

	if (responseCode == 404 || responseCode == 401 || CURLE_OK != result) {
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

bool CDM::setEvCtot(string callsign) {
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
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);
		result = curl_easy_perform(curl);
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
		curl_easy_cleanup(curl);
	}

	if (responseCode == 404 || responseCode == 401 || CURLE_OK != result) {
		addLogLine("UNABLE TO CONNECT CDM-API...");
	}
	else {
		std::istringstream is(readBuffer);
		//Get data from .txt file
		string cid;
		while (getline(is, cid))
		{
			if (cid.length() > 2) {
				for (int i = 0; i < slotFile.size(); i++) {
					if (slotFile[i].size() > 1) {
						if (slotFile[i][0] == cid) {
							addLogLine(callsign + " linked with EvCTOT " + slotFile[i][1]);
							sendMessage(callsign + " linked with EvCTOT " + slotFile[i][1]);
							bool found = false;
							for (int a = 0; a < evCtots.size(); a++) {
								if (evCtots[a].size() > 0) {
									if (evCtots[a][0] == callsign) {
										evCtots[a] = { callsign, slotFile[i][1] };
										found = true;
										return true;
									};
								}
							}
							evCtots.push_back({ callsign, slotFile[i][1] });
						}
					}
				}
			}
			/*for (int a = 0; a < evCtots.size(); a++) {
				if (evCtots[a][0] == callsign) {
					evCtots[a] = { callsign, "" };
					return true;
				};
			}*/
		}
		if(cid == "") {
			checkCIDLater.push_back(callsign);
		}
	}
	return false;
	}
	catch (const std::exception& e) {
		addLogLine("ERROR: Unhandled exception setEvCtot: " + (string)e.what());
		return false;
	}
	catch (...) {
		addLogLine("ERROR: Unhandled exception setEvCtot");
		return false;
	}
}

void CDM::getCdmServerRestricted() {
	addLogLine("Called getCdmServerRestricted...");
	try{
	addLogLine("Call - Fetching CTOTs");
	//sendMessage("Fetching CTOTs...");
	CURL* curl;
	CURLcode result = CURLE_FAILED_INIT;
	std::string readBuffer;
	long responseCode = 0;
	curl = curl_easy_init();
	if (curl) {
		string url = cdmServerUrl + "/slotService/restricted";
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

	if (responseCode == 404 || responseCode == 401 || CURLE_OK != result) {
		// handle error 404
		addLogLine("UNABLE TO LOAD CDM-API URL...");
	}
	else {
		Json::Reader reader;
		Json::Value obj;
		Json::FastWriter fastWriter;
		reader.parse(readBuffer, obj);

		
		//Reset all CTOTs
		for (size_t i = 0; i < slotList.size(); i++) {
			if (slotList[i].ctot != "") {
				slotList[i].hasManualCtot = false;
			}
			slotList[i].ctot = "";
			slotList[i].flowReason = "";
		}

		serverRestrictedPlanes.clear();

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

				serverRestrictedPlanes.push_back({ callsign,ctot,reason });

				if (ctot.size() == 4) {
					for (size_t z = 0; z < slotList.size(); z++) {
						if (slotList[z].callsign == callsign && !flightHasCtotDisabled(callsign)) {
							string taxiTime = getTaxiTime(callsign);
							slotList[z] = {
								callsign,
								slotList[z].eobt,
								calculateLessTime(ctot + "00", stod(taxiTime)),
								ctot + "00",
								ctot,
								reason,
								slotList[z].ecfmpRestriction,
								slotList[z].hasEcfmpRestriction,
								true,
								true
							};
							if (slotList[z].ttot != (ctot + "00")) {
								CFlightPlan fp = FlightPlanSelect(slotList[z].callsign.c_str());
								addTimeToListForSpecificAirportAndRunway(10, calculateTime(GetTimeNow(), 5), fp.GetFlightPlanData().GetOrigin(), fp.GetFlightPlanData().GetDepartureRwy());
							}
						}
					}
				}
			}
		}
		sendWaitingTSAT();
		sendWaitingCdmSts();
		sendCheckCIDLater();
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

void CDM::sendWaitingTSAT() {
	try {
		addLogLine("Called sendWaitingTSAT...");
		vector<Plane> setTSATlaterTemp = setTSATlater;

		for (int i = 0; i < setTSATlaterTemp.size(); ++i) {
			addLogLine("sendWaitingTSAT - " + setTSATlaterTemp[i].callsign);
			for (auto it = setTSATlater.begin(); it != setTSATlater.end();) {
				if (it->callsign == setTSATlaterTemp[i].callsign) {
					it = setTSATlater.erase(it);
				}
				else {
					++it;
				}
			}
			setTSATApi(setTSATlaterTemp[i].callsign, setTSATlaterTemp[i].tsat);
		}
	}
	catch (const std::exception& e) {
		addLogLine("ERROR: Unhandled exception sendWaitingTSAT: " + (string)e.what());
	}
	catch (...) {
		addLogLine("Error occurred parsing data from the cdm-api (sendWaitingTSAT)");
	}
}

void CDM::sendWaitingCdmSts() {
	string cdmSts = "";
	vector<string> setCdmStslaterTemp = setCdmStslater;
	for (int i = 0; i < setCdmStslaterTemp.size();) {
		string callsign = setCdmStslaterTemp[i];
		cdmSts = getCdmSts(callsign);
		addLogLine("sendWaitingCdmSts - " + callsign);
		for (int a = 0; a < setCdmStslater.size(); a++) {
			if (setCdmStslater[a] == callsign) {
				setCdmStslater.erase(setCdmStslater.begin() + a);
			}
		}
		setCdmSts(callsign, cdmSts);
	}
}

void CDM::sendCheckCIDLater() {
	vector<string> checkCIDLaterTemp = checkCIDLater;
	for (int i = 0; i < checkCIDLaterTemp.size();) {
		string callsign = checkCIDLaterTemp[i];
		addLogLine("sendCheckCIDLater - " + callsign);
		for (int a = 0; a < checkCIDLater.size(); a++) {
			if (checkCIDLater[a] == callsign) {
				checkCIDLater.erase(checkCIDLater.begin() + a);
			}
		}
		setEvCtot(callsign);
	}
}

void CDM::setTSATApi(string callsign, string tsat) {
	addLogLine("Called setTSATApi...");
	try{
	addLogLine("Call - Set TSAT (" + tsat + ") for " + callsign);
	bool createRequest = false;
	for (Plane p : slotList) {
		//Do not send API request if Manual CTOT is created by user.
		if ((p.hasManualCtot == false || (p.hasManualCtot && p.ctot != "")) && p.callsign == callsign) {
			createRequest = true;
		}
	}

	if (createRequest) {

		if (tsat.length() >= 4) {
			tsat = tsat.substr(0, 4);
		}
		else {
			tsat = "";
		}

		string cdmSts = getCdmSts(callsign);

		string taxiTime = getTaxiTime(callsign);

		CURL* curl;
		CURLcode result = CURLE_FAILED_INIT;
		string readBuffer;
		long responseCode = 0;
		curl = curl_easy_init();
		if (curl) {
			string url = cdmServerUrl + "/slotService/cdm?callsign=" + callsign + "&taxi=" + taxiTime + "&tsat=" + tsat + "&cdmSts=" + cdmSts;
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

		if (responseCode == 404 || responseCode == 401 || CURLE_OK != result) {
			Plane plane(callsign, "", tsat, "", "", "", EcfmpRestriction(), false, false, false);
			setTSATlater.push_back(plane);
			for (int i = 0; i < slotList.size(); i++) {
				//Show data again
				if (slotList[i].callsign == callsign) {
					slotList[i].showData = true;
				}
			}
			addLogLine("UNABLE TO CONNECT CDM-API...");
		}
		else {
			Json::Reader reader;
			Json::Value obj;
			Json::FastWriter fastWriter;
			reader.parse(readBuffer, obj);
			if (!obj.isNull()) {
				if (obj.isMember("callsign") && obj.isMember("ctot") && obj.isMember("mostPenalizingAirspace")) {
					//Get callsign 
					string callsign = fastWriter.write(obj["callsign"]);
					callsign.erase(std::remove(callsign.begin(), callsign.end(), '"'));
					callsign.erase(std::remove(callsign.begin(), callsign.end(), '"'));
					callsign.erase(std::remove(callsign.begin(), callsign.end(), '\n'));
					callsign.erase(std::remove(callsign.begin(), callsign.end(), '\n'));

					//Get CTOT
					string ctot = fastWriter.write(obj["ctot"]);
					ctot.erase(std::remove(ctot.begin(), ctot.end(), '"'));
					ctot.erase(std::remove(ctot.begin(), ctot.end(), '\n'));
					ctot.erase(std::remove(ctot.begin(), ctot.end(), '\n'));

					//Get reason
					string reason = fastWriter.write(obj["mostPenalizingAirspace"]);
					reason.erase(std::remove(reason.begin(), reason.end(), '"'));
					reason.erase(std::remove(reason.begin(), reason.end(), '\n'));
					reason.erase(std::remove(reason.begin(), reason.end(), '\n'));


					for (size_t i = 0; i < slotList.size(); i++) {
						if (slotList[i].callsign == callsign) {
							addLogLine(callsign + " returned with CTOT: [" + ctot + "] and reason: [" + reason + "]");
							//sendMessage(callsign + " returned with CTOT: [" + ctot + "] and reason: [" + reason + "]");
							if (ctot != "" && !flightHasCtotDisabled(callsign)) {
								slotList[i] = {
									callsign,
									slotList[i].eobt,
									calculateLessTime(ctot + "00", stod(taxiTime)),
									ctot + "00",
									ctot,
									reason,
									slotList[i].ecfmpRestriction,
									slotList[i].hasEcfmpRestriction,
									true,
									true
								};
							}
							else {
								if (slotList[i].ctot != "") {
									slotList[i].ctot = "";
									slotList[i].flowReason = "";
									slotList[i].hasManualCtot = false;
									slotList[i].showData = true;
								}
							}
						}
					}
				}
				else {
					Plane plane(callsign, "", tsat, "", "", "", EcfmpRestriction(), false, false, false);
					setTSATlater.push_back(plane);
				}
			}
			else {
				Plane plane(callsign, "", tsat, "", "", "", EcfmpRestriction(), false, false, false);
				setTSATlater.push_back(plane);
			}
		}
	}

	for (size_t a = 0; a < slotList.size(); a++)
	{
		if (slotList[a].callsign == callsign) {
			slotList[a].showData = true;
		}
	}
	addLogLine("COMPLETED - setTSATApi for " + callsign);
	}
	catch (const std::exception& e) {
		addLogLine("ERROR: Unhandled exception setTSATApi: " + (string)e.what());
		for (size_t a = 0; a < slotList.size(); a++)
		{
			if (slotList[a].callsign == callsign) {
				slotList[a].showData = true;
			}
		}
	}
	catch (...) {
		addLogLine("ERROR: Unhandled exception setTSATApi");
		for (size_t a = 0; a < slotList.size(); a++)
		{
			if (slotList[a].callsign == callsign) {
				slotList[a].showData = true;
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
	addLogLine("Called setCdmSts...");
	try {
		addLogLine("Call - Set CDM Sts (" + cdmSts + ") for " + callsign);
		CURL* curl;
		CURLcode result = CURLE_FAILED_INIT;
		string readBuffer;
		long responseCode = 0;
		curl = curl_easy_init();
		if (curl) {
			string url = cdmServerUrl + "/slotService/setCdmStatus?callsign=" + callsign + "&cdmSts=" + cdmSts;
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

		if (responseCode == 404 || responseCode == 401 || CURLE_OK != result) {
			setCdmStslater.push_back(callsign);
			addLogLine("UNABLE TO CONNECT CDM-API...");
		}
		else {
			std::istringstream is(readBuffer);
			//Get data from .txt file
			string lineValue;
			while (getline(is, lineValue))
			{
				if (lineValue != "true") {
					setCdmStslater.push_back(callsign);
				}
			}
		}
	}
	catch (const std::exception& e) {
		addLogLine("ERROR: Unhandled exception setCdmSts: " + (string)e.what());
	}
	catch (...) {
		addLogLine("ERROR: Unhandled exception setCdmSts");
	}
}

string CDM::getCdmSts(string callsign) {
	addLogLine("Call - getCdmSts");
	bool outOfTsat = false;
	for (size_t i = 0; i < OutOfTsat.size(); i++)
	{
		if (callsign == OutOfTsat[i].substr(0, OutOfTsat[i].find(","))) {
			outOfTsat = true;
		}
	}

	if (outOfTsat) {
		addLogLine("getCdmSts: I");
		return "I";
	}
	addLogLine("getCdmSts: ");
	return "";
}