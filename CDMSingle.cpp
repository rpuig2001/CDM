#include "stdafx.h"
#include "CDMSingle.hpp"
#include "pugixml.hpp"
#include "pugixml.cpp"
#include "Plane.h"
#include "Flow.h"
#include <thread>

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
string rateString;
string lvoRateString;
string ctotOption;
int expiredCTOTTime;
bool defaultRate;
int countTime;
int countTfcDisconnection;
int refreshTime;
bool addTime;
bool lvo;
bool ctotCid;
string myTimeToAdd;
string taxiZonesUrl;
string flowRestrictionsUrl;
int defTaxiTime;

//Ftp data
string ftpHost;
string ftpUser;
string ftpPassword;

vector<Plane> slotList;
vector<Flow> flowData;
vector<string> asatList;
vector<string> taxiTimesList;
vector<string> TxtTimesVector;
vector<string> OutOfTsat;
vector<string> colors;
vector<string> rate;
vector<string> planeAiportList;
vector<string> masterAirports;
vector<string> CDMairports;
vector<string> CTOTcheck;
vector<string> finalTimesList;
vector<string> disconnectionList;

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

	// Register Tag Item "CDM-EOBT"
	RegisterTagItemType("EOBT", TAG_ITEM_EOBT);
	RegisterTagItemFunction("Edit EOBT", TAG_FUNC_EDITEOBT);

	//Register Tag Item "CDM-TOBT"
	RegisterTagItemType("TOBT", TAG_ITEM_TOBT);
	RegisterTagItemFunction("Ready TOBT", TAG_FUNC_READYTOBT);
	RegisterTagItemFunction("Edit TOBT", TAG_FUNC_EDITTOBT);
	RegisterTagItemFunction("EOBT to TOBT", TAG_FUNC_EOBTTOTOBT);

	// Register Tag Item "CDM-TSAT"
	RegisterTagItemType("TSAT", TAG_ITEM_TSAT);

	// Register Tag Item "CDM-TTOT"
	RegisterTagItemType("TTOT", TAG_ITEM_TTOT);

	// Register Tag Item "CDM-TSAC"
	RegisterTagItemType("TSAC", TAG_ITEM_TSAC);
	RegisterTagItemFunction("Add TSAT to TSAC", TAG_FUNC_ADDTSAC);
	RegisterTagItemFunction("Edit TSAC", TAG_FUNC_EDITTSAC);

	// Register Tag Item "CDM-ASAT"
	RegisterTagItemType("ASAT", TAG_ITEM_ASAT);

	// Register Tag Item "CDM-ASAT"
	RegisterTagItemType("ASRT", TAG_ITEM_ASRT);
	RegisterTagItemFunction("Toggle ASRT", TAG_FUNC_TOGGLEASRT);

	// Register Tag Item "CDM-E"
	RegisterTagItemType("E", TAG_ITEM_E);

	//Register
	RegisterTagItemType("Flow Message", TAG_ITEM_FLOW_MESSAGE);

	// Register Tag Item "CDM-CTOT"
	RegisterTagItemType("CTOT", TAG_ITEM_CTOT);
	RegisterTagItemFunction("Open CTOT Option list", TAG_FUNC_CTOTOPTIONS);

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

	debugMode = false;
	initialSidLoad = false;

	countTime = stoi(GetTimeNow());
	addTime = false;

	countTfcDisconnection = -1;

	GetVersion();

	//Get data from xml config file
	//airport = getFromXml("/CDM/apt/@icao");
	//airport = getFromXml("/CDM/apt/@icao");
	defTaxiTime = stoi(getFromXml("/CDM/DefaultTaxiTime/@minutes"));
	ctotOption = getFromXml("/CDM/ctot/@option");
	refreshTime = stoi(getFromXml("/CDM/RefreshTime/@seconds"));
	expiredCTOTTime = stoi(getFromXml("/CDM/expiredCtot/@time"));
	rateString = getFromXml("/CDM/rate/@ops");
	lvoRateString = getFromXml("/CDM/rateLvo/@ops");
	taxiZonesUrl = getFromXml("/CDM/Taxizones/@url");
	string stringDebugMode = getFromXml("/CDM/Debug/@mode");
	flowRestrictionsUrl = getFromXml("/CDM/FlowRestrictions/@url");
	ftpHost = getFromXml("/CDM/ftpHost/@host");
	ftpUser = getFromXml("/CDM/ftpUser/@user");
	ftpPassword = getFromXml("/CDM/ftpPassword/@password");
	debugMode = false;
	if (stringDebugMode == "true") {
		debugMode = true;
		sendMessage("[DEBUG MESSAGE] - USING DEBUG MODE");
	}

	lvo = false;
	getRate();

	if (ctotOption == "cid") {
		ctotCid = true;
	}
	else {
		ctotCid = false;
	}

	//Flow Data
	getFlowData();


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
			if (lineValue.substr(0, 1) != "#") {
				TxtTimesVector.push_back(lineValue);
			}
		}
	}
	else {
		if (debugMode) {
			sendMessage("[DEBUG MESSAGE] - USING TAXIZONES FROM URL");
		}
		getTaxiZonesFromUrl(taxiZonesUrl);
	}

	fstream fileCtot;
	string lineValueCtot;
	fileCtot.open(cfad.c_str(), std::ios::in);
	while (getline(fileCtot, lineValueCtot))
	{
		addCtotToMainList(lineValueCtot);
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
	DisplayUserMessage("Message", "CDM", message.c_str(), true, true, true, false, false);
}

//
void CDM::OnFunctionCall(int FunctionId, const char* ItemString, POINT Pt, RECT Area) {
	CFlightPlan fp = FlightPlanSelectASEL();
	bool AtcMe = false;
	bool master = false;

	for (string apt : masterAirports)
	{
		if (apt == fp.GetFlightPlanData().GetOrigin()) {
			master = true;
		}
	}

	if (master) {
		if (fp.GetTrackingControllerIsMe() || strlen(fp.GetTrackingControllerId()) == 0) {
			AtcMe = true;
		}
	}

	if (FunctionId == TAG_FUNC_EDITEOBT)
	{
		if (master && AtcMe) {
			OpenPopupEdit(Area, TAG_FUNC_NEWEOBT, fp.GetFlightPlanData().GetEstimatedDepartureTime());
		}
	}

	else if (FunctionId == TAG_FUNC_NEWEOBT) {
		string editedEOBT = ItemString;
		bool hasNoNumber = true;
		if (editedEOBT.length() <= 4) {

			for (int i = 0; i < editedEOBT.length(); i++) {
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
		fp.GetControllerAssignedData().SetFlightStripAnnotation(0, formatTime(fp.GetFlightPlanData().GetEstimatedDepartureTime()).c_str());
	}
	else if (FunctionId == TAG_FUNC_ADDTSAC) {
		string annotTSAC = fp.GetControllerAssignedData().GetFlightStripAnnotation(2);
		string completeTOBT = (string)fp.GetControllerAssignedData().GetFlightStripAnnotation(0);
		if (annotTSAC.empty() && !completeTOBT.empty()) {
			//Get Time now
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
				for (int a = 0; a < slotList.size(); a++)
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
		OpenPopupEdit(Area, TAG_FUNC_NEWTSAC, fp.GetControllerAssignedData().GetFlightStripAnnotation(2));
	}

	else if (FunctionId == TAG_FUNC_NEWTSAC) {
		string editedTSAC = ItemString;
		if (editedTSAC.length() > 0) {
			bool hasNoNumber = true;
			if (editedTSAC.length() == 4) {
				for (int i = 0; i < editedTSAC.length(); i++) {
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

	else if (FunctionId == TAG_FUNC_TOGGLEASRT) {
		if (master && AtcMe) {
			string annotAsrt = fp.GetControllerAssignedData().GetFlightStripAnnotation(1);
			if (annotAsrt.empty()) {
				//Get Time now
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

				fp.GetControllerAssignedData().SetFlightStripAnnotation(1, (hour + min).c_str());
			}
			else {
				fp.GetControllerAssignedData().SetFlightStripAnnotation(1, "");
			}
		}
	}

	else if (FunctionId == TAG_FUNC_CTOTOPTIONS) {
		if (master && AtcMe) {
			bool hasCTOT = false;
			bool hasRestriction = false;
			for (int i = 0; i < slotList.size(); i++)
			{
				if (slotList[i].callsign == fp.GetCallsign()) {
					if (slotList[i].hasCtot) {
						hasCTOT = true;
						if (slotList[i].hasRestriction) {
							hasRestriction = true;
						}
					}
				}
			}
			if (hasCTOT) {
				OpenPopupList(Area, "CTOT Options", 1);
				if (hasRestriction) {
					AddPopupListElement("Reload CTOT", "", TAG_FUNC_REMOVECTOT, false, 2, false);
				}
				else {
					AddPopupListElement("Remove CTOT", "", TAG_FUNC_REMOVECTOT, false, 2, false);
				}
			}
		}
	}
	else if (FunctionId == TAG_FUNC_REMOVECTOT) {
		for (int a = 0; a < slotList.size(); a++)
		{
			if (slotList[a].callsign == fp.GetCallsign()) {
				if (slotList[a].hasCtot) {
					slotList.erase(slotList.begin() + a);
				}
			}
		}

		for (int i = 0; i < slotList.size(); i++)
		{
			if (slotList[i].callsign == fp.GetCallsign()) {
				slotList[i].hasCtot = false;
				slotList[i].ctot = "";
				string remarks = fp.GetControllerAssignedData().GetFlightStripAnnotation(3);
				if (remarks.find("CTOT") != string::npos) {
					if (remarks.find("%") != string::npos) {
						string stringToAdd = remarks.substr(0, remarks.find("CTOT")) + remarks.substr(remarks.find("%"), remarks.length() - remarks.find("%"));
						fp.GetControllerAssignedData().SetFlightStripAnnotation(3, stringToAdd.c_str());
						remarks = stringToAdd;
					}
					else {
						string stringToAdd = remarks.substr(0, remarks.find("CTOT") - 1);
						fp.GetControllerAssignedData().SetFlightStripAnnotation(3, stringToAdd.c_str());
						remarks = stringToAdd;
					}
				}
			}
		}
	}

	else if (FunctionId == TAG_FUNC_READYTOBT) {
		if (master && AtcMe) {
			fp.GetControllerAssignedData().SetFlightStripAnnotation(0, formatTime(GetActualTime()).c_str());
			//Get Time now
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

			string annotAsrt = fp.GetControllerAssignedData().GetFlightStripAnnotation(1);
			if (annotAsrt.empty()) {
				fp.GetControllerAssignedData().SetFlightStripAnnotation(1, (hour + min).c_str());
			}
		}
	}

	else if (FunctionId == TAG_FUNC_EDITTOBT) {
		if (master && AtcMe) {
			OpenPopupEdit(Area, TAG_FUNC_NEWTOBT, fp.GetControllerAssignedData().GetFlightStripAnnotation(0));
		}
	}
	else if (FunctionId == TAG_FUNC_NEWTOBT) {
		string editedTOBT = ItemString;
		bool hasNoNumber = true;
		if (editedTOBT.length() == 4) {

			for (int i = 0; i < editedTOBT.length(); i++) {
				if (isdigit(editedTOBT[i]) == false) {
					hasNoNumber = false;
				}
			}
			if (hasNoNumber) {
				bool found = false;
				for (int i = 0; i < slotList.size(); i++)
				{
					if (slotList[i].callsign == fp.GetCallsign()) {
						slotList[i].eobt = editedTOBT;
						fp.GetControllerAssignedData().SetFlightStripAnnotation(0, editedTOBT.c_str());
						found = true;
					}
				}
				if (!found) {
					fp.GetControllerAssignedData().SetFlightStripAnnotation(0, editedTOBT.c_str());
				}
			}
		}
		else if (editedTOBT.empty()) {
			fp.GetControllerAssignedData().SetFlightStripAnnotation(0, "");
			for (int i = 0; i < slotList.size(); i++) {
				if ((string)fp.GetCallsign() == slotList[i].callsign) {
					slotList.erase(slotList.begin() + i);
				}
			}
		}
	}
}

void CDM::OnFlightPlanDisconnect(CFlightPlan FlightPlan) {
	string tobt = FlightPlan.GetControllerAssignedData().GetFlightStripAnnotation(0);
	if (tobt.length() > 0) {
		disconnectionList.push_back(FlightPlan.GetCallsign());
		countTfcDisconnection = stoi(GetTimeNow());
	}
}


void CDM::OnGetTagItem(CFlightPlan FlightPlan, CRadarTarget RadarTarget, int ItemCode, int TagData, char sItemString[16], int* pColorCode, COLORREF* pRGB, double* pFontSize)
{
	COLORREF ItemRGB = 0xFFFFFFFF;
	string callsign = FlightPlan.GetCallsign();

	string origin = FlightPlan.GetFlightPlanData().GetOrigin(); boost::to_upper(origin);
	string destination = FlightPlan.GetFlightPlanData().GetDestination(); boost::to_upper(destination);
	string remarks = FlightPlan.GetControllerAssignedData().GetFlightStripAnnotation(3);

	string depRwy = FlightPlan.GetFlightPlanData().GetDepartureRwy(); boost::to_upper(depRwy);
	bool isVfr = false;
	if (strcmp(FlightPlan.GetFlightPlanData().GetPlanType(), "V") > -1) {
		isVfr = true;
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
			const char* EOBT = "";
			const char* TSAT = "";
			const char* TTOT = "";
			int taxiTime = defTaxiTime;

			//If aircraft is in aircraftFind Base vector
			int pos;
			bool aircraftFind = false;
			for (int i = 0; i < slotList.size(); i++) {
				if (callsign == slotList[i].callsign) {
					aircraftFind = true;
					pos = i;
				}
			}

			//If opion is "cid"
			if (ctotCid) {
				bool ctotValidated = false;
				for (int i = 0; i < CTOTcheck.size(); i++) {
					if (callsign == CTOTcheck[i]) {
						ctotValidated = true;
					}
				}

				if (!ctotValidated) {
					string cid = getCidByCallsign(callsign);
					string savedCid;
					string ctotCallsign;
					for (int i = 0; i < slotList.size(); i++)
					{
						savedCid = slotList[i].callsign;
						if (checkIsNumber(savedCid)) {
							if (stoi(cid) == stoi(savedCid)) {
								slotList[i].callsign = callsign;
								pos = i;
								for (int a = 0; a < slotList.size(); a++)
								{
									ctotCallsign = slotList[a].callsign;
									if (checkIsNumber(ctotCallsign)) {
										if (stoi(cid) == stoi(ctotCallsign)) {
											slotList[pos].hasCtot = true;
										}
									}
								}
							}
						}
					}
					CTOTcheck.push_back(callsign);
				}
			}


			bool isValidToCalculateEventMode = false;
			string tobt = FlightPlan.GetControllerAssignedData().GetFlightStripAnnotation(0);
			if (tobt.length() > 0) {
				isValidToCalculateEventMode = true;
			}

			if (!isValidToCalculateEventMode) {
				for (int i = 0; i < disconnectionList.size(); i++) {
					if (disconnectionList[i] == callsign) {
						disconnectionList.erase(disconnectionList.begin() + i);
						if (aircraftFind) {
							FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(0, formatTime(slotList[pos].eobt).c_str());
						}
						isValidToCalculateEventMode = true;
					}
				}
			}

			bool hasCtot = false;
			if (aircraftFind) {
				if (slotList[pos].hasCtot) {
					hasCtot = true;
				}
			}

			//It'll calculate pilot's times after pressing READY TOBT Function
			if (isValidToCalculateEventMode) {
				//EOBT
				EOBT = FlightPlan.GetFlightPlanData().GetEstimatedDepartureTime();
				string EOBTstring = EOBT;
				string EOBTfinal = formatTime(EOBTstring);

				if (tobt.length() > 0) {
					EOBTfinal = tobt;
				}

				EOBTfinal += "00";
				EOBT = EOBTfinal.c_str();
				bool stillOutOfTsat = false;
				int stillOutOfTsatPos;

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

				bool stsDepa = false;
				if ((string)FlightPlan.GetGroundState() == "DEPA") {
					stsDepa = true;
					if (remarks.find("%") != string::npos) {
						string stringToAdd = remarks.substr(0, remarks.find("%") - 1);
						FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(3, stringToAdd.c_str());
						remarks = stringToAdd;
					}
					if (remarks.find("CTOT") != string::npos) {
						string stringToAdd = remarks.substr(0, remarks.find("CTOT") - 1);
						FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(3, stringToAdd.c_str());
						remarks = stringToAdd;
					}
					if (remarks.find("ASRT") != string::npos) {
						string stringToAdd = remarks.substr(0, remarks.find("ASRT") - 1);
						FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(3, stringToAdd.c_str());
						remarks = stringToAdd;
					}
				}

				// Get Taxi times
				int TaxiTimePos = 0;
				bool planeHasTaxiTimeAssigned = false;
				for (int j = 0; j < taxiTimesList.size(); j++)
				{
					if (taxiTimesList[j].substr(0, taxiTimesList[j].find(",")) == callsign) {
						planeHasTaxiTimeAssigned = true;
						TaxiTimePos = j;
					}
				}

				if (aircraftFind) {
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
								if (!hasCtot) {
									slotList.erase(slotList.begin() + pos);
									aircraftFind = false;
								}
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
				for (int i = 0; i < planeAiportList.size(); i++)
				{
					if (planeAiportList[i].substr(0, planeAiportList[i].find(",")) == callsign) {
						aptFind = true;
						if (planeAiportList[i].substr(planeAiportList[i].find(",") + 1, 4) != origin) {
							planeAiportList[i] = callsign + "," + origin;
						}
					}
				}

				//Get Restriction
				bool hasFlowMeasures = false;
				Flow myFlow;
				if (!aircraftFind) {
					for (int z = 0; z < flowData.size(); z++)
					{
						if (destination == flowData[z].dest.substr(0, 4)) {
							//Chech origin
							if ((origin == flowData[z].depa.substr(0, 4)) || ("ALL" == flowData[z].depa.substr(0, 3))) {
								//Check day && Month
								string dayMonth = GetDateMonthNow();
								if (stoi(dayMonth.substr(0, dayMonth.find("-"))) == stoi(flowData[z].validDate.substr(0, flowData[z].validDate.find("/"))) && stoi(dayMonth.substr(dayMonth.find("-") + 1)) == stoi(flowData[z].validDate.substr(flowData[z].validDate.find("/") + 1))) {
									//Check valid time
									int timeNow = stoi(GetActualTime());
									if (stoi(flowData[z].validTime.substr(0, flowData[z].validTime.find("-"))) <= timeNow && stoi(flowData[z].validTime.substr(flowData[z].validTime.find("-") + 1)) >= timeNow) {
										hasFlowMeasures = true;
										myFlow = flowData[z];
									}
								}
							}
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
						for (int i = 0; i < finalTimesList.size(); i++) {
							if (finalTimesList[i] == callsign) {
								finalTimesList.erase(finalTimesList.begin() + i);
							}
						}
					}

					for (int i = 0; i < OutOfTsat.size(); i++)
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

					if (stillOutOfTsat && !gndStatusSet) {
						//Remove ACFT Find
						if (aircraftFind) {
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
						for (int x = 0; x < asatList.size(); x++)
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
							ItemRGB = TAG_GREEN;
							strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
						}
						else if (ItemCode == TAG_ITEM_TSAC)
						{
							ItemRGB = TAG_GREEN;
							strcpy_s(sItemString, 16, "____");
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
							ItemRGB = TAG_GREEN;
							strcpy_s(sItemString, 16, "I");
						}
						else if (ItemCode == TAG_ITEM_CTOT)
						{
							if (hasCtot) {
								ItemRGB = TAG_CTOT;
								strcpy_s(sItemString, 16, slotList[pos].ctot.c_str());
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
							if (GetdifferenceTime(hour, min, myHour, myMin) <= 0) {
								if (stoi(myTimeToAdd) > stoi(EOBT) && !gndStatusSet) {
									EOBTfinal = myTimeToAdd;
									EOBT = EOBTfinal.c_str();
									FlightPlan.GetFlightPlanData().SetEstimatedDepartureTime(myTimeToAdd.c_str());
									FlightPlan.GetFlightPlanData().AmendFlightPlan();
									if (aircraftFind) {
										string tempTTOT, tempTSAT;

										if (hasCtot) {
											tempTTOT = calculateTime(slotList[pos].ttot, taxiTime);
											tempTSAT = calculateTime(slotList[pos].tsat, taxiTime);
										}
										else {
											tempTTOT = calculateTime(slotList[pos].ttot, taxiTime);
											tempTSAT = calculateTime(slotList[pos].tsat, taxiTime);
										}
										slotList[pos].ttot = tempTTOT;
										slotList[pos].ttot = tempTSAT;
									}
								}
							}
							else {
								addTime = false;
							}
						}

						string TSATfinal = "";
						string TTOTFinal = "";

						if (aircraftFind) {
							string tempEOBT = EOBT;
							if (tempEOBT != slotList[pos].eobt && !hasCtot) {
								//aircraftFind false to recalculate Times due to fp change
								slotList.erase(slotList.begin() + pos);
								aircraftFind = false;
								if (remarks.find("%") != string::npos) {
									string stringToAdd = remarks.substr(0, remarks.find("%") - 1);
									FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(3, stringToAdd.c_str());
									remarks = stringToAdd;
								}
							}
							else if (tempEOBT != slotList[pos].eobt && hasCtot) {

							}
						}


						/*if (hasCtot) {
								//IF EOBT+TaxiTime >= CTOT+10 THEN CTOT LOST
								string myTimeString = hour + min + "00";
								if (stoi(calculateTime(myTimeString, taxiTime)) > stoi(calculateTime(slotList[pos].ctot + "00", 10))) {
									slotList[pos].hasCtot = false;
									hasCtot = false;
									slotList[pos].ctot = "";
									if (remarks.find("CTOT") != string::npos) {
										if (remarks.find("%") != string::npos) {
											string stringToAdd = remarks.substr(0, remarks.find("CTOT")) + remarks.substr(remarks.find("%"), remarks.length() - remarks.find("%"));
											FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(3, stringToAdd.c_str());
											remarks = stringToAdd;
										}
										else {
											string stringToAdd = remarks.substr(0, remarks.find("CTOT") - 1);
											FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(3, stringToAdd.c_str());
											remarks = stringToAdd;
										}
									}
								}
							}*/

						if (!aircraftFind) {
							if (!hasCtot) {
								TSAT = EOBT;
								//TSAT
								string TSATstring = TSAT;
								TSATfinal = formatTime(TSATstring) + "00";
								TSAT = TSATfinal.c_str();

								//TTOT
								TTOTFinal = calculateTime(TSATstring, taxiTime);
								TTOT = TTOTFinal.c_str();
							}
						}
						else {
							if (hasCtot) {
								if (slotList[pos].eobt == "999999") {
									string temporalEOBT = FlightPlan.GetFlightPlanData().GetEstimatedDepartureTime();
									slotList[pos].eobt = temporalEOBT + "00";
									aircraftFind = false;
								}
								//TSAT
								string TSATstring = slotList[pos].tsat;
								TSATfinal = formatTime(TSATstring) + "00";
								TSAT = TSATfinal.c_str();

								//TTOT
								TTOTFinal = slotList[pos].ttot;
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
						}



						bool equalTTOT = true;
						bool correctTTOT = true;
						bool equalTempoTTOT = true;
						bool alreadySetTOStd = false;
						string myCtot = "";

						if (!aircraftFind) {
							//Calculate Rate
							int rate;
							bool flowChecked = false;

							rate = rateForRunway(origin, depRwy, lvo);
							if (rate == -1) {
								if (!lvo) {
									rate = stoi(rateString);
								}
								else {
									rate = stoi(lvoRateString);
								}
							}

							double rateHour = (double)60 / rate;

							while (equalTTOT) {
								correctTTOT = true;
								for (int t = 0; t < slotList.size(); t++)
								{
									string listTTOT;
									string listCallsign = slotList[t].callsign;
									string listDepRwy = "";
									bool depRwyFound = false;
									for (int i = 0; i < taxiTimesList.size(); i++)
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
									for (int i = 0; i < planeAiportList.size(); i++)
									{
										if (listCallsign == planeAiportList[i].substr(0, planeAiportList[i].find(","))) {
											listAirport = planeAiportList[i].substr(planeAiportList[i].find(",") + 1, 4);
										}
									}

									if (!depRwyFound) {
										listDepRwy = depRwy;
									}

									if (hasCtot) {
										bool found = false;
										while (!found) {
											found = true;
											if (hasCtot) {

												listTTOT = slotList[t].ttot;

												if (TTOTFinal == listTTOT && callsign != listCallsign && depRwy == listDepRwy && listAirport == origin) {
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
												else if ((stoi(TTOTFinal) < stoi(calculateTime(listTTOT, rateHour))) && (stoi(TTOTFinal) > stoi(calculateLessTime(listTTOT, rateHour))) && callsign != listCallsign && depRwy == listDepRwy && listAirport == origin) {
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
									else {
										bool found = false;
										while (!found) {
											found = true;
											if (hasCtot) {
												listTTOT = slotList[t].ttot;
											}
											else {
												listTTOT = slotList[t].ttot;
											}

											if (TTOTFinal == listTTOT && callsign != listCallsign && depRwy == listDepRwy && listAirport == origin) {
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
											else if ((stoi(TTOTFinal) < stoi(calculateTime(listTTOT, rateHour))) && (stoi(TTOTFinal) > stoi(calculateLessTime(listTTOT, rateHour))) && callsign != listCallsign && depRwy == listDepRwy && listAirport == origin) {
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

									if (hasFlowMeasures) {
										int seperationFlow = stoi(myFlow.time);
										for (int z = 0; z < slotList.size(); z++)
										{
											if (FlightPlanSelect(slotList[z].callsign.c_str()).GetFlightPlanData().GetDestination() == destination) {
												sameDestList.push_back(slotList[z]);
											}
										}

										for (int z = 0; z < sameDestList.size(); z++)
										{
											CFlightPlan fpList = FlightPlanSelect(slotList[z].callsign.c_str());
											bool found = false;
											string listTTOT = sameDestList[z].ttot;
											string listCallsign = sameDestList[z].callsign;
											string listDepRwy = fpList.GetFlightPlanData().GetDepartureRwy();
											string listAirport = fpList.GetFlightPlanData().GetOrigin();
											while (!found) {
												found = true;
												if (TTOTFinal == listTTOT && callsign != listCallsign && depRwy == listDepRwy && listAirport == origin) {
													found = false;
													TTOTFinal = calculateTime(TTOTFinal, 1);
													correctFlowTTOT = false;
												}
												else if ((stoi(TTOTFinal) < stoi(calculateTime(listTTOT, seperationFlow))) && (stoi(TTOTFinal) > stoi(calculateLessTime(listTTOT, seperationFlow))) && callsign != listCallsign && depRwy == listDepRwy && listAirport == origin) {
													found = false;
													TTOTFinal = calculateTime(TTOTFinal, 1);
													correctFlowTTOT = false;
												}
											}
										}
										if (aircraftFind) {
											if (!hasCtot) {
												hasCtot = true;
												slotList[pos].hasCtot = true;
												slotList[pos].ctot = formatTime(TTOTFinal);
											}
										}
										else {
											hasCtot = true;
											myCtot = formatTime(TTOTFinal);
										}
									}
									if (correctFlowTTOT) {
										equalTTOT = false;
										TSATfinal = calculateLessTime(TTOTFinal, taxiTime);
										TSAT = TSATfinal.c_str();
										TTOT = TTOTFinal.c_str();
										if (hasCtot) {
											if (aircraftFind) {
												if (TTOTFinal != slotList[pos].ttot) {
													Plane p(callsign, EOBT, TSAT, TTOT, true, slotList[pos].ctot, hasFlowMeasures, myFlow);
													slotList[pos] = p;

													if (remarks.find("CTOT") != string::npos) {
														string stringToAdd = remarks.substr(0, remarks.find("CTOT") - 1);
														remarks = stringToAdd;
													}
													else if (remarks.find("%") != string::npos) {
														string stringToAdd = remarks.substr(0, remarks.find("%") - 1);
														remarks = stringToAdd;
													}

													string stringToAdd = remarks + " CTOT" + slotList[pos].ctot + " %" + TSAT + "|" + TTOT;
													FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(3, stringToAdd.c_str());
													remarks = stringToAdd;
												}
											}
											else {
												if (myCtot.empty()) {
													Plane p(callsign, EOBT, TSAT, TTOT, true, slotList[pos].ctot, hasFlowMeasures, myFlow);
													slotList.push_back(p);
												}
												else {
													Plane p(callsign, EOBT, TSAT, TTOT, true, myCtot, hasFlowMeasures, myFlow);
													slotList.push_back(p);
												}

												if (remarks.find("CTOT") != string::npos) {
													string stringToAdd = remarks.substr(0, remarks.find("CTOT") - 1);
													remarks = stringToAdd;
												}
												else if (remarks.find("%") != string::npos) {
													string stringToAdd = remarks.substr(0, remarks.find("%") - 1);
													remarks = stringToAdd;
												}

												string stringToAdd = remarks + " CTOT" + slotList[pos].ctot + " %" + TSAT + "|" + TTOT;
												FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(3, stringToAdd.c_str());
												remarks = stringToAdd;
											}
										}
										else {
											if (aircraftFind) {
												if (TTOTFinal != slotList[pos].ttot) {
													Plane p(callsign, EOBT, TSAT, TTOT, false, "", hasFlowMeasures, myFlow);
													slotList[pos] = p;

													if (remarks.find("%") != string::npos) {
														string stringToAdd = remarks.substr(0, remarks.find("%") - 1);
														remarks = stringToAdd;
													}

													string stringToAdd = remarks + " %" + TSAT + "|" + TTOT;
													remarks = stringToAdd;
												}
											}
											else {
												Plane p(callsign, EOBT, TSAT, TTOT, false, "", hasFlowMeasures, myFlow);
												slotList.push_back(p);

												if (remarks.find("%") != string::npos) {
													string stringToAdd = remarks.substr(0, remarks.find("%") - 1);
													remarks = stringToAdd;
												}

												string stringToAdd = remarks + " %" + TSAT + "|" + TTOT;
												FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(3, stringToAdd.c_str());
												remarks = stringToAdd;
											}
										}
									}
								}
							}
						}

						//Remove disconnected planes after 5 min disconnected
						if (countTfcDisconnection != -1) {
							if (countTfcDisconnection - stoi(GetTimeNow()) < -300) {
								countTfcDisconnection = -1;
								disconnectTfcs();
							}
						}

						//Refresh times every x sec
						if (countTime - stoi(GetTimeNow()) < -refreshTime) {
							multithread(&CDM::getFlowData);
							multithread(&CDM::saveData);
							if (debugMode) {
								sendMessage("[DEBUG MESSAGE] - REFRESHING");
							}
							countTime = stoi(GetTimeNow());

							//Calculate Rate
							int rate;

							rate = rateForRunway(origin, depRwy, lvo);
							if (rate == -1) {
								if (!lvo) {
									rate = stoi(rateString);
								}
								else {
									rate = stoi(lvoRateString);
								}
							}

							double rateHour = (double)60 / rate;

							for (int i = 0; i < slotList.size(); i++)
							{
								if (!slotList[i].hasRestriction) {
									string myTTOT, myTSAT, myEOBT, myCallsign, myAirport, myDepRwy = "", myRemarks;
									int myTTime = defTaxiTime;

									myCallsign = slotList[i].callsign;
									//Check if aircraft has state and no need to recalculate
									bool aicraftInFinalTimesList = false;
									for (string aircraft : finalTimesList) {
										if (aircraft == myCallsign) {
											aicraftInFinalTimesList = true;
										}
									}
									if (!aicraftInFinalTimesList) {
										CFlightPlan myFlightPlan = FlightPlanSelect(myCallsign.c_str());

										for (int s = 0; s < planeAiportList.size(); s++)
										{
											if (myCallsign == planeAiportList[s].substr(0, planeAiportList[s].find(","))) {
												myAirport = planeAiportList[s].substr(planeAiportList[s].find(",") + 1, 4);
											}
										}

										bool depRwyFound = false;
										for (int t = 0; t < taxiTimesList.size(); t++)
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
										bool myhasCTOT = false;
										int myCtotPos = 0;
										for (int s = 0; s < slotList.size(); s++)
										{
											if (myCallsign == slotList[s].callsign) {
												if (slotList[s].hasCtot) {
													myhasCTOT = true;
													myCtotPos = s;
												}
											}
										}

										myEOBT = slotList[i].eobt;

										if (myhasCTOT) {
											//TSAT and TTOT with CTOT
											myTTOT = formatTime(slotList[pos].ctot) + "00";
											myTSAT = calculateLessTime(myTTOT, myTTime);
										}
										else {
											myTSAT = myEOBT;
											myTTOT = calculateTime(myEOBT, myTTime);
										}

										refreshTimes(myFlightPlan, myCallsign, myEOBT, myTSAT, myTTOT, myAirport, myTTime, myRemarks, myDepRwy, rateHour, myhasCTOT, myCtotPos, i, true);
									}
								}
							}
						}		

						//Sync TTOT
						if (remarks.find("%") != string::npos) {
							if (TTOT != remarks.substr(remarks.find("%") + 8, 6)) {
								string stringToAdd = remarks.substr(0, remarks.find("%") - 1);
								FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(3, stringToAdd.c_str());
								remarks = stringToAdd;
							}
						}
						else if (aircraftFind && !stsDepa) {
							string stringToAdd = remarks + " %" + TSAT + "|" + TTOT;
							FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(3, stringToAdd.c_str());
							remarks = stringToAdd;
						}

						//Sync CTOT
						if (remarks.find("CTOT") != string::npos && hasCtot) {
							string listCtot = slotList[pos].ctot;
							if (listCtot != remarks.substr(remarks.find("CTOT") + 4, 4)) {
								if (remarks.find("%") != string::npos) {
									string stringToAdd = remarks.substr(0, remarks.find("CTOT")) + "CTOT" + listCtot + " " + remarks.substr(remarks.find("%"), remarks.length() - remarks.find("%"));
									FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(3, stringToAdd.c_str());
									remarks = stringToAdd;
								}
								else {
									string stringToAdd = remarks.substr(0, remarks.find("CTOT")) + "CTOT" + listCtot;
									FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(3, stringToAdd.c_str());
									remarks = stringToAdd;
								}
							}
						}
						else if (hasCtot && !stsDepa) {
							string listCtot = slotList[pos].ctot;
							if (remarks.find("%") != string::npos) {
								string stringToAdd = remarks.substr(0, remarks.find("%")) + "CTOT" + listCtot + " " + remarks.substr(remarks.find("%"), remarks.length() - remarks.find("%"));
								FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(3, stringToAdd.c_str());
								remarks = stringToAdd;
							}
							else {
								string stringToAdd = remarks + " CTOT" + listCtot;
								FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(3, stringToAdd.c_str());
								remarks = stringToAdd;
							}
						}
						else if (remarks.find("CTOT") != string::npos && !hasCtot) {
							if (remarks.find("%") != string::npos) {
								string stringToAdd = remarks.substr(0, remarks.find("CTOT")) + remarks.substr(remarks.find("%"), remarks.length() - remarks.find("%"));
								FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(3, stringToAdd.c_str());
								remarks = stringToAdd;
							}
							else {
								string stringToAdd = remarks.substr(0, remarks.find("CTOT") - 1);
								FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(3, stringToAdd.c_str());
								remarks = stringToAdd;
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

						if (oldTSAT && !correctState && !hasCtot) {
							OutOfTsat.push_back(callsign + "," + EOBT);
							FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(1, "");
							if (remarks.find("CTOT") != string::npos) {
								string stringToAdd = remarks.substr(0, remarks.find("CTOT") - 1);
								FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(3, stringToAdd.c_str());
								remarks = stringToAdd;
							}
							if (remarks.find("%") != string::npos) {
								string stringToAdd = remarks.substr(0, remarks.find("%") - 1);
								FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(3, stringToAdd.c_str());
								remarks = stringToAdd;
							}
						}

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

						//TSAC
						bool TSACNotTSAT = false;
						string annotTSAC = FlightPlan.GetControllerAssignedData().GetFlightStripAnnotation(2);

						if (!annotTSAC.empty()) {
							string TSAChour = annotTSAC.substr(annotTSAC.length() - 4, 2);
							string TSACmin = annotTSAC.substr(annotTSAC.length() - 2, 2);

							int TSACDif = GetdifferenceTime(TSAThour, TSATmin, TSAChour, TSACmin);
							if (TSACDif > 5 || TSACDif < -5) {
								TSACNotTSAT = true;
							}
						}

						//ASAT
						bool ASATFound = false;
						bool ASATPlusFiveLessTen = false;
						int ASATpos = 0;
						string ASATtext = " ";
						for (int x = 0; x < asatList.size(); x++)
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


						if (ItemCode == TAG_ITEM_EOBT)
						{
							string ShowEOBT = formatTime(FlightPlan.GetFlightPlanData().GetEstimatedDepartureTime());
							//*pColorCode = TAG_COLOR_RGB_DEFINED;
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
						else if (ItemCode == TAG_ITEM_TSAC)
						{
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
						else if (ItemCode == TAG_ITEM_TSAT)
						{
							string ShowTSAT = (string)TSAT;
							if (SU_ISSET) {
								ItemRGB = SU_SET_COLOR;
								strcpy_s(sItemString, 16, ShowTSAT.substr(0, ShowTSAT.length() - 2).c_str());
							}
							else if (notYetEOBT) {
								//*pColorCode = TAG_COLOR_RGB_DEFINED;
								ItemRGB = TAG_RED;
								strcpy_s(sItemString, 16, " ");
							}
							else if (lastMinute) {
								//*pColorCode = TAG_COLOR_RGB_DEFINED;
								ItemRGB = TAG_YELLOW;
								strcpy_s(sItemString, 16, ShowTSAT.substr(0, ShowTSAT.length() - 2).c_str());
							}
							else if (moreLessFive) {
								//*pColorCode = TAG_COLOR_RGB_DEFINED;
								ItemRGB = TAG_GREEN;
								strcpy_s(sItemString, 16, ShowTSAT.substr(0, ShowTSAT.length() - 2).c_str());
							}
							else if (oldTSAT) {
								//*pColorCode = TAG_COLOR_RGB_DEFINED;
								ItemRGB = TAG_GREEN;
								strcpy_s(sItemString, 16, ShowTSAT.substr(0, ShowTSAT.length() - 2).c_str());
							}
							else {
								//*pColorCode = TAG_COLOR_RGB_DEFINED;
								ItemRGB = TAG_GREENNOTACTIVE;
								strcpy_s(sItemString, 16, ShowTSAT.substr(0, ShowTSAT.length() - 2).c_str());
							}
						}
						else if (ItemCode == TAG_ITEM_TTOT)
						{
							string ShowTTOT = (string)TTOT;
							if (notYetEOBT) {
								//*pColorCode = TAG_COLOR_RGB_DEFINED;
								ItemRGB = TAG_TTOT;
								strcpy_s(sItemString, 16, " ");
							}
							else if (moreLessFive || lastMinute) {
								//*pColorCode = TAG_COLOR_RGB_DEFINED;
								ItemRGB = TAG_TTOT;
								strcpy_s(sItemString, 16, ShowTTOT.substr(0, ShowTTOT.length() - 2).c_str());
							}
							else {
								//*pColorCode = TAG_COLOR_RGB_DEFINED;
								ItemRGB = TAG_TTOT;
								strcpy_s(sItemString, 16, ShowTTOT.substr(0, ShowTTOT.length() - 2).c_str());
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
								ItemRGB = TAG_ASRT;
								strcpy_s(sItemString, 16, ASRTtext.c_str());
							}
							else {
								//*pColorCode = TAG_COLOR_RGB_DEFINED;
								ItemRGB = TAG_ASRT;
								strcpy_s(sItemString, 16, " ");
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
							if (aircraftFind) {
								if (slotList[pos].hasRestriction) {
									    string message = slotList[pos].flowRestriction.customMessage;
										ItemRGB = TAG_YELLOW;
										//strcpy_s(sItemString, 16, message.c_str());
									}
								}
						}
						else if (ItemCode == TAG_ITEM_CTOT)
						{
							if (hasCtot) {
								//*pColorCode = TAG_COLOR_RGB_DEFINED;
								ItemRGB = TAG_CTOT;
								strcpy_s(sItemString, 16, slotList[pos].ctot.c_str());
							}
						}
					}
				}
				else {
					bool TSATFind = false;
					string TSATString, TTOTString;
					if (remarks.find("%") != string::npos) {
						TSATString = remarks.substr(remarks.find("%") + 1, 6);
						TTOTString = remarks.substr(remarks.find("%") + 8, 6);
						TSATFind = true;
					}

					if (aircraftFind) {
						if (!TSATFind) {
							slotList.erase(slotList.begin() + pos);
						}
						else if (TSATString != slotList[pos].tsat && TSATFind) {
							if (hasCtot) {
								Plane p(callsign, EOBT, TSATString, TTOTString, true, slotList[pos].ctot, hasFlowMeasures, myFlow);
								slotList[pos] = p;
							}
							else {
								Plane p(callsign, EOBT, TSATString, TTOTString, true, slotList[pos].ctot, hasFlowMeasures, myFlow);
								string valueToAdd = callsign + "," + EOBT + "," + TSATString + "," + TTOTString;
								slotList[pos] = p;
							}
						}
					}
					else {
						if (TSATFind) {
							if (hasCtot) {
								Plane p(callsign, EOBT, TSATString, TTOTString, true, slotList[pos].ctot, hasFlowMeasures, myFlow);
								slotList.push_back(p);
							}
							else {
								Plane p(callsign, EOBT, TSATString, TTOTString, false, "", hasFlowMeasures, myFlow);
								slotList.push_back(p);
							}
						}
					}

					//If oldTSAT
					if (TSATFind) {
						string TSAThour = TSATString.substr(TSATString.length() - 6, 2);
						string TSATmin = TSATString.substr(TSATString.length() - 4, 2);

						bool oldTSAT = false;
						bool moreLessFive = false;
						bool lastMinute = false;
						bool notYetEOBT = false;
						bool actualTOBT = false;

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

						if (oldTSAT && !correctState && !hasCtot) {
							OutOfTsat.push_back(callsign + "," + EOBT);
						}

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

						//CTOT
						if (remarks.find("CTOT") != string::npos) {
							string rmkCtot = remarks.substr(remarks.find("CTOT") + 4, 4);
							if (hasCtot) {
								if (slotList[pos].ctot != rmkCtot) {
									slotList[pos].ctot = rmkCtot;
									if (!hasCtot) {
										slotList[pos].hasCtot = true;
									}
								}
							}
							else {
								slotList[pos].hasCtot = true;
							}
						}
						else if (hasCtot) {
							slotList[pos].hasCtot = false;
						}

						//TSAC
						bool TSACNotTSAT = false;
						string annotTSAC = FlightPlan.GetControllerAssignedData().GetFlightStripAnnotation(2);

						if (!annotTSAC.empty()) {
							string TSAChour = annotTSAC.substr(annotTSAC.length() - 4, 2);
							string TSACmin = annotTSAC.substr(annotTSAC.length() - 2, 2);

							int TSACDif = GetdifferenceTime(TSAThour, TSATmin, TSAChour, TSACmin);
							if (TSACDif > 5 || TSACDif < -5) {
								TSACNotTSAT = true;
							}
						}

						//ASAT
						bool ASATFound = false;
						bool ASATPlusFiveLessTen = false;
						int ASATpos = 0;
						string ASATtext = " ";
						for (int x = 0; x < asatList.size(); x++)
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


						if (ItemCode == TAG_ITEM_EOBT)
						{
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
								ItemRGB = TAG_GREENNOTACTIVE;
								strcpy_s(sItemString, 16, " ");
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
						else if (ItemCode == TAG_ITEM_TSAT)
						{
							if (TSATString.length() > 0) {
								if (SU_ISSET) {
									ItemRGB = SU_SET_COLOR;
									strcpy_s(sItemString, 16, TSATString.substr(0, 4).c_str());
								}
								else if (notYetEOBT) {
									ItemRGB = TAG_RED;
									strcpy_s(sItemString, 16, " ");
								}
								else if (lastMinute) {
									ItemRGB = TAG_YELLOW;
									strcpy_s(sItemString, 16, TSATString.substr(0, 4).c_str());
								}
								else if (moreLessFive) {
									ItemRGB = TAG_GREEN;
									strcpy_s(sItemString, 16, TSATString.substr(0, 4).c_str());
								}
								else if (oldTSAT) {
									ItemRGB = TAG_GREEN;
									strcpy_s(sItemString, 16, TSATString.substr(0, 4).c_str());
								}
								else {
									ItemRGB = TAG_GREENNOTACTIVE;
									strcpy_s(sItemString, 16, TSATString.substr(0, 4).c_str());
								}
							}
						}
						else if (ItemCode == TAG_ITEM_TTOT)
						{
							if (TTOTString.length() > 0) {
								if (notYetEOBT) {
									ItemRGB = TAG_TTOT;
									strcpy_s(sItemString, 16, " ");
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
								ItemRGB = TAG_ASRT;
								strcpy_s(sItemString, 16, ASRTtext.c_str());
							}
							else {
								//*pColorCode = TAG_COLOR_RGB_DEFINED;
								ItemRGB = TAG_ASRT;
								strcpy_s(sItemString, 16, " ");
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
							if (slotList[pos].hasRestriction) {
								string message = slotList[pos].flowRestriction.customMessage;
								ItemRGB = TAG_YELLOW;
								//strcpy_s(sItemString, 16, message.c_str());
							}
						}
						}
						else if (ItemCode == TAG_ITEM_CTOT)
						{
							if (hasCtot) {
								ItemRGB = TAG_CTOT;
								strcpy_s(sItemString, 16, slotList[pos].ctot.c_str());
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
						else if (ItemCode == TAG_ITEM_TSAC)
						{
							ItemRGB = TAG_GREEN;
							strcpy_s(sItemString, 16, "____");
						}
						else if (ItemCode == TAG_ITEM_TOBT)
						{
							ItemRGB = TAG_RED;
							strcpy_s(sItemString, 16, "MAST");
						}
						else if (ItemCode == TAG_ITEM_TSAT)
						{
							ItemRGB = TAG_RED;
							strcpy_s(sItemString, 16, "MAST");
						}
						else if (ItemCode == TAG_ITEM_TTOT)
						{
							ItemRGB = TAG_RED;
							strcpy_s(sItemString, 16, "MAST");
						}
						else if (ItemCode == TAG_ITEM_E)
						{
							ItemRGB = TAG_GREEN;
							strcpy_s(sItemString, 16, "I");
						}
						else if (ItemCode == TAG_ITEM_CTOT)
						{
							if (hasCtot) {
								ItemRGB = TAG_CTOT;
								strcpy_s(sItemString, 16, slotList[pos].ctot.c_str());
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
					if (hasCtot) {
						ItemRGB = TAG_CTOT;
						strcpy_s(sItemString, 16, slotList[pos].ctot.c_str());
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
		}
	}
}

bool CDM::getRate() {
	//Get data from rate.txt file
	fstream rateFile;
	string lineValue, myAirport;
	rateFile.open(rfad.c_str(), std::ios::in);
	bool found;
	while (getline(rateFile, lineValue))
	{
		rate.push_back(lineValue);
		myAirport = lineValue.substr(0, lineValue.find(":"));
		found = false;
		for (string airport : CDMairports) {
			if (airport == myAirport) {
				found = true;
			}
		}
		if (!found) {
			CDMairports.push_back(myAirport);
		}
	}
	return true;
}

int CDM::rateForRunway(string airport, string depRwy, bool lvoActive) {
	string lineAirport, lineDepRwy;
	for (string line : rate) {
		if (line.length() > 1) {
			lineAirport = line.substr(0, line.find(":"));
			if (lineAirport == airport) {
				lineDepRwy = line.substr(line.find(":") + 1, line.find("=") - line.find(":") - 1);
				if (lineDepRwy == depRwy) {
					if (!lvoActive) {
						return stoi(line.substr(line.find("=") + 1, line.length() - line.find("=")));
					}
					else {
						return stoi(line.substr(line.find("_") + 1, line.length() - line.find("_")));
					}
				}
			}
		}
	}
	return -1;
}

bool CDM::refreshTimes(CFlightPlan FlightPlan, string callsign, string EOBT, string TSATfinal, string TTOTFinal, string origin, int taxiTime, string remarks, string depRwy, double rateHour, bool hasCTOT, int ctotPos, int pos, bool aircraftFind) {
	bool equalTTOT = true;
	bool correctTTOT = true;
	bool equalTempoTTOT = true;
	bool alreadySetTOStd = false;

	//Get Restriction
	Flow myFlow;

	while (equalTTOT) {
		correctTTOT = true;
		for (int t = 0; t < slotList.size(); t++)
		{

			string listTTOT;
			string listCallsign = slotList[t].callsign;
			string listDepRwy = "";
			bool depRwyFound = false;
			PushToOtherControllers(FlightPlanSelect(listCallsign.c_str()));
			for (int i = 0; i < taxiTimesList.size(); i++)
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
			for (int i = 0; i < planeAiportList.size(); i++)
			{
				if (listCallsign == planeAiportList[i].substr(0, planeAiportList[i].find(","))) {
					listAirport = planeAiportList[i].substr(planeAiportList[i].find(",") + 1, 4);
				}
			}

			if (!depRwyFound) {
				listDepRwy = depRwy;
			}

			if (hasCTOT) {
				bool found = false;
				while (!found) {
					found = true;
					if (hasCTOT) {

						listTTOT = slotList[t].ttot;

						if (TTOTFinal == listTTOT && callsign != listCallsign && depRwy == listDepRwy && listAirport == origin) {
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
						else if (callsign != listCallsign && depRwy == listDepRwy && listAirport == origin) {
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
					if (hasCTOT) {
						listTTOT = slotList[t].ttot;
					}
					else {
						listTTOT = slotList[t].ttot;
					}

					if (TTOTFinal == listTTOT && callsign != listCallsign && depRwy == listDepRwy && listAirport == origin) {
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
					else if (callsign != listCallsign && depRwy == listDepRwy && listAirport == origin) {
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

		if (correctTTOT) {
			equalTTOT = false;
			TSATfinal = calculateLessTime(TTOTFinal, taxiTime);
			string TSAT = TSATfinal.c_str();
			string TTOT = TTOTFinal.c_str();
			if (hasCTOT) {
				if (aircraftFind) {
					if (TTOTFinal != slotList[pos].ttot || slotList[pos].ttot == "999999") {
						Plane p(callsign, EOBT, TSAT, TTOT, true, slotList[pos].ctot, false, myFlow);
						slotList[pos] = p;

						if (remarks.find("CTOT") != string::npos) {
							string stringToAdd = remarks.substr(0, remarks.find("CTOT") - 1);
							remarks = stringToAdd;
						}
						else if (remarks.find("%") != string::npos) {
							string stringToAdd = remarks.substr(0, remarks.find("%") - 1);
							remarks = stringToAdd;
						}

						string stringToAdd = remarks + " CTOT" + slotList[pos].ctot + " %" + TSAT + "|" + TTOT;
						FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(3, stringToAdd.c_str());
						remarks = stringToAdd;
					}
				}
				else {
					Plane p(callsign, EOBT, TSAT, TTOT, true, slotList[pos].ctot, false, myFlow);
					slotList.push_back(p);

					if (remarks.find("CTOT") != string::npos) {
						string stringToAdd = remarks.substr(0, remarks.find("CTOT") - 1);
						remarks = stringToAdd;
					}
					else if (remarks.find("%") != string::npos) {
						string stringToAdd = remarks.substr(0, remarks.find("%") - 1);
						remarks = stringToAdd;
					}

					string stringToAdd = remarks + " CTOT" + slotList[pos].ctot + " %" + TSAT + "|" + TTOT;
					FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(3, stringToAdd.c_str());
					remarks = stringToAdd;
				}
			}
			else {
				if (aircraftFind) {
					if (TTOTFinal != slotList[pos].ttot) {
						Plane p(callsign, EOBT, TSAT, TTOT, false, "", false, myFlow);
						slotList[pos] = p;

						if (remarks.find("%") != string::npos) {
							string stringToAdd = remarks.substr(0, remarks.find("%") - 1);
							remarks = stringToAdd;
						}

						string stringToAdd = remarks + " %" + TSAT + "|" + TTOT;
						FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(3, stringToAdd.c_str());
						remarks = stringToAdd;
					}
				}
				else {
					Plane p(callsign, EOBT, TSAT, TTOT, false, "", false, myFlow);
					slotList.push_back(p);

					if (remarks.find("%") != string::npos) {
						string stringToAdd = remarks.substr(0, remarks.find("%") - 1);
						remarks = stringToAdd;
					}

					string stringToAdd = remarks + " %" + TSAT + "|" + TTOT;
					FlightPlan.GetControllerAssignedData().SetFlightStripAnnotation(3, stringToAdd.c_str());
					remarks = stringToAdd;
				}
			}
		}
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
				if (callsign.substr(callsign.size() - 3) == "DEL" || callsign.substr(callsign.size() - 3) == "GND" || callsign.substr(callsign.size() - 3) == "TWR" || callsign.substr(callsign.size() - 3) == "APP") {
					fp.PushFlightStrip(c.GetCallsign());
				}
			}
		}
	}
}
/*
* Method to calculate TSAT from TTOT

void CDM::CalculateTSAT(string TTOT) {

}
*/
/*
* Method to calculate TTOT

void CDM::CalculateAvailableTTOT(string TTOT) {

}
*/
/*
* Method to check if plane has CTOT

void CDM::CheckCtot(string TTOT) {

}
*/

string CDM::GetActualTime()
{
	//Get Time now
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
	return hour + min;
}

string CDM::GetDateMonthNow() {
	//Get Time now
	time_t rawtime;
	struct tm* ptm;
	time(&rawtime);
	ptm = gmtime(&rawtime);
	string day = to_string(ptm->tm_mday);
	string month = to_string(ptm->tm_mon + 1);
	return day + "-" + month;
}

string CDM::EobtPlusTime(string EOBT, int addedTime) {
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

	return calculateTime(hour + min + "00", addedTime);
}

void CDM::deleteFlightStrips(string callsign)
{
	CFlightPlan fp = FlightPlanSelect(callsign.c_str());
	fp.GetControllerAssignedData().SetFlightStripAnnotation(0, "");
	fp.GetControllerAssignedData().SetFlightStripAnnotation(1, "");
	fp.GetControllerAssignedData().SetFlightStripAnnotation(2, "");
	fp.GetControllerAssignedData().SetFlightStripAnnotation(3, "");
}

string CDM::getTaxiTime(double lat, double lon, string origin, string depRwy) {
	string line, TxtOrigin, TxtDepRwy, TxtTime;
	CPosition p1, p2, p3, p4;
	smatch match;

	try
	{
		for (int t = 0; t < TxtTimesVector.size(); t++)
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
	}
	catch (...)
	{
		DisplayUserMessage(MY_PLUGIN_NAME, "Error", std::to_string(GetLastError()).c_str(), true, true, false, true, false);
		DisplayUserMessage(MY_PLUGIN_NAME, "Error", line.c_str(), true, true, false, true, false);
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


string CDM::calculateTime(string timeString, double minsToAdd) {
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

string CDM::calculateLessTime(string timeString, double minsToAdd) {
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

bool CDM::checkIsNumber(string str) {
	bool hasNoNumber = true;
	for (int i = 0; i < str.length(); i++) {
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

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

void CDM::disconnectTfcs() {
	for (string callsign : disconnectionList)
	{
		RemoveDataFromTfc(callsign);
	}
	disconnectionList.clear();
}

void CDM::RemoveDataFromTfc(string callsign) {
	deleteFlightStrips(callsign);
	//Delete from vector
	for (int i = 0; i < slotList.size(); i++) {
		if (callsign == slotList[i].callsign) {
			slotList.erase(slotList.begin() + i);
		}
	}
	//Remove Plane From airport List
	for (int j = 0; j < planeAiportList.size(); j++)
	{
		if (planeAiportList[j].substr(0, planeAiportList[j].find(",")) == callsign) {
			planeAiportList.erase(planeAiportList.begin() + j);
		}
	}

	//Remove Plane From finalTimesList
	for (int i = 0; i < finalTimesList.size(); i++) {
		if (finalTimesList[i] == callsign) {
			finalTimesList.erase(finalTimesList.begin() + i);
		}
	}

	//Remove Taxi Times List
	for (int j = 0; j < taxiTimesList.size(); j++)
	{
		if (taxiTimesList[j].substr(0, taxiTimesList[j].find(",")) == callsign) {
			taxiTimesList.erase(taxiTimesList.begin() + j);
		}
	}

	//Remove ctotCheck
	for (int i = 0; i < CTOTcheck.size(); i++)
	{
		if (CTOTcheck[i] == callsign) {
			CTOTcheck.erase(CTOTcheck.begin() + i);
		}
	}

	//Remove ASAT
	for (int x = 0; x < asatList.size(); x++)
	{
		string actualListCallsign = asatList[x].substr(0, asatList[x].find(","));
		if (actualListCallsign == callsign) {
			asatList.erase(asatList.begin() + x);
		}
	}
	//Remove from OutOfTsat
	for (int i = 0; i < OutOfTsat.size(); i++)
	{
		if (callsign == OutOfTsat[i].substr(0, OutOfTsat[i].find(","))) {
			OutOfTsat.erase(OutOfTsat.begin() + i);
		}
	}
}

string CDM::getCidByCallsign(string callsign) {
	CURL* curl;
	CURLcode res;
	std::string readBuffer;
	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, "https://data.vatsim.net/v3/vatsim-data.json");
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
	}
	Json::Reader reader;
	Json::Value obj;
	Json::FastWriter fastWriter;
	reader.parse(readBuffer, obj);

	string foundCallsign;

	const Json::Value& pilots = obj["pilots"];
	for (int i = 0; i < pilots.size(); i++) {
		foundCallsign = fastWriter.write(pilots[i]["callsign"]);
		if (foundCallsign.substr(1, foundCallsign.length() - 3) == callsign) {
			std::string myCid = fastWriter.write(pilots[i]["cid"]);
			return myCid;
		}
	}
	return "0";
}

void CDM::getFlowData() {
	if (!flowRestrictionsUrl.empty()) {
		flowData.clear();
		CURL* curl;
		CURLcode res;
		std::string readBuffer;
		curl = curl_easy_init();
		if (curl) {
			curl_easy_setopt(curl, CURLOPT_URL, flowRestrictionsUrl);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
			res = curl_easy_perform(curl);
			curl_easy_cleanup(curl);
		}
		Json::Reader reader;
		Json::Value obj;
		Json::FastWriter fastWriter;
		reader.parse(readBuffer, obj);

		const Json::Value& measures = obj["MDI"];
		for (int i = 0; i < measures.size(); i++) {
			string type = "MDI";
			string time = fastWriter.write(measures[i]["TIME"]);
			string depa = fastWriter.write(measures[i]["DEPA"]);
			depa.erase(std::remove(depa.begin(), depa.end(), '"'));
			string dest = fastWriter.write(measures[i]["DEST"]);
			dest.erase(std::remove(dest.begin(), dest.end(), '"'));
			string valid_Date = fastWriter.write(measures[i]["VALIDDATE"]);
			valid_Date.erase(std::remove(valid_Date.begin(), valid_Date.end(), '"'));
			string valid_time = fastWriter.write(measures[i]["VALIDTIME"]);
			valid_time.erase(std::remove(valid_time.begin(), valid_time.end(), '"'));
			string message = fastWriter.write(measures[i]["MESSAGE"]);
			message.erase(std::remove(message.begin(), message.end(), '"'));
			Flow flow(type, time, depa, dest, valid_Date, valid_time, message);
			flowData.push_back(flow);
		}
	}
}

void CDM::saveData() {
	if (!ftpHost.empty()) {
		if (!slotList.empty()) {
			for (string airport : masterAirports) {
				ofstream myfile;
				string fileName = dfad + "_" + airport + ".txt";
				myfile.open(fileName, std::ofstream::out | std::ofstream::trunc);
				for (Plane plane : slotList) {
					if (myfile.is_open())
					{
						if (airport == FlightPlanSelect(plane.callsign.c_str()).GetFlightPlanData().GetOrigin()) {
							string str;
							if (plane.hasCtot) {
								if (plane.hasRestriction) {
									str = plane.callsign + "," + plane.eobt + "," + plane.tsat + "," + plane.ttot + "," + plane.ctot + "," + plane.flowRestriction.customMessage + ",";
								}
								else {
									str = plane.callsign + "," + plane.eobt + "," + plane.tsat + "," + plane.ttot + "," + plane.ctot + ",flowRestriction" + ",";
								}
							}
							else {
								if (plane.hasRestriction) {
									str = plane.callsign + "," + plane.eobt + "," + plane.tsat + "," + plane.ttot + ",ctot" + "," + plane.flowRestriction.customMessage + ",";
								}
								else {
									str = plane.callsign + "," + plane.eobt + "," + plane.tsat + "," + plane.ttot + ",ctot" + ",flowRestriction" + ",";
								}
							}
							myfile << str << endl;
						}
					}
				}
				myfile.close();
				upload(fileName, airport);
			}
		}
	}
}

void CDM::upload(string fileName, string airport)
{
	string saveName = "/CDM_data_" + airport + ".txt";
	HINTERNET hInternet = InternetOpen(NULL, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	HINTERNET hFtpSession = InternetConnect(hInternet, ftpHost.c_str(), INTERNET_DEFAULT_FTP_PORT, ftpUser.c_str(), ftpPassword.c_str(), INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0);
	FtpPutFile(hFtpSession, fileName.c_str(), saveName.c_str(), FTP_TRANSFER_TYPE_BINARY, 0);
	InternetCloseHandle(hFtpSession);
	InternetCloseHandle(hInternet);
}


int CDM::GetVersion() {
	CURL* curl;
	CURLcode result;
	std::string readBuffer = "";
	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, "https://raw.githubusercontent.com/rpuig2001/CDM/master/version.txt");
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
		result = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
	}

	if (readBuffer.find(MY_PLUGIN_VERSION) == std::string::npos) {
		string DisplayMsg = "Please UPDATE YOUR CDM PLUGIN, version " + readBuffer + " is OUT! You have version " + MY_PLUGIN_VERSION " installed, download it from puigcloud.me/CDM";
		DisplayUserMessage(MY_PLUGIN_NAME, "UPDATE", DisplayMsg.c_str(), true, false, false, false, false);
	}

	return -1;
}

bool CDM::getTaxiZonesFromUrl(string url) {
	CURL* curl;
	CURLcode result;
	string readBuffer;
	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
		result = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
	}

	std::istringstream is(readBuffer);

	//Get data from .txt file
	string lineValue;
	while (getline(is, lineValue))
	{
		if (lineValue.substr(0, 1) != "#") {
			TxtTimesVector.push_back(lineValue);
		}
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
	struct tm* ptm;
	time(&rawtime);
	ptm = gmtime(&rawtime);
	string hour = to_string(ptm->tm_hour % 24);
	string min = to_string(ptm->tm_min);
	string sec = to_string(ptm->tm_sec);

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

bool CDM::addCtotToMainList(string lineValue) {
	Flow myFlow;
	//Get Time now
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
	bool found = false;
	for (int i = 0; i < slotList.size(); i++)
	{
		if (slotList[i].callsign == lineValue.substr(0, lineValue.find(","))) {
			bool oldCTOT = true;
			string CTOTHour = slotList[i].ttot.substr(slotList[i].ttot.length() - 8, 2);
			string CTOTMin = slotList[i].ttot.substr(slotList[i].ttot.length() - 6, 2);
			int difTime = GetdifferenceTime(CTOTHour, CTOTMin, hour, min);
			if (hour != CTOTHour) {
				if (difTime >= expiredCTOTTime + 40) {
					oldCTOT = false;
				}
			}
			else {
				if (difTime >= expiredCTOTTime) {
					oldCTOT = false;
				}
			}
			if (!oldCTOT) {
				Plane p(lineValue.substr(0, lineValue.find(",")), "999999", "999999", lineValue.substr(lineValue.find(",") + 1, 4) + "00", true, lineValue.substr(lineValue.find(",") + 1, 4), false, myFlow);
				slotList[i] = p;
				found = true;
			}
		}
	}
	if (!found) {
		bool oldCTOT = true;
		string CTOTHour = lineValue.substr(lineValue.length() - 4, 2);
		string CTOTMin = lineValue.substr(lineValue.length() - 2, 2);
		int difTime = GetdifferenceTime(CTOTHour, CTOTMin, hour, min);
		if (hour != CTOTHour) {
			if (difTime >= expiredCTOTTime + 40) {
				oldCTOT = false;
			}
		}
		else {
			if (difTime >= expiredCTOTTime) {
				oldCTOT = false;
			}
		}
		if (!oldCTOT) {
			Plane p(lineValue.substr(0, lineValue.find(",")), "999999", "999999", lineValue.substr(lineValue.find(",") + 1, 4) + "00", true, lineValue.substr(lineValue.find(",") + 1, 4), false, myFlow);
			slotList.push_back(p);
		}
	}
}

bool CDM::OnCompileCommand(const char* sCommandLine) {
	if (startsWith(".cdm refresh", sCommandLine)) {
		sendMessage("Refreshing Now...");
		countTime = stoi(GetTimeNow()) - refreshTime;
		return true;
	}

	if (startsWith(".cdm reload", sCommandLine))
	{
		sendMessage("Reloading CDM....");
		for (Plane pl : slotList) {
			deleteFlightStrips(pl.callsign);
		}
		slotList.clear();
		asatList.clear();
		taxiTimesList.clear();
		TxtTimesVector.clear();
		OutOfTsat.clear();
		colors.clear();
		flowData.clear();
		rate.clear();
		planeAiportList.clear();
		masterAirports.clear();
		CDMairports.clear();
		CTOTcheck.clear();
		finalTimesList.clear();
		//Get data from xml config file
		defTaxiTime = stoi(getFromXml("/CDM/DefaultTaxiTime/@minutes"));
		ctotOption = getFromXml("/CDM/ctot/@option");
		refreshTime = stoi(getFromXml("/CDM/RefreshTime/@seconds"));
		expiredCTOTTime = stoi(getFromXml("/CDM/expiredCtot/@time"));
		rateString = getFromXml("/CDM/rate/@ops");
		lvoRateString = getFromXml("/CDM/rateLvo/@ops");
		taxiZonesUrl = getFromXml("/CDM/Taxizones/@url");
		string stringDebugMode = getFromXml("/CDM/Debug/@mode");
		flowRestrictionsUrl = getFromXml("/CDM/FlowRestrictions/@url");
		debugMode = false;
		if (stringDebugMode == "true") {
			debugMode = true;
			sendMessage("[DEBUG MESSAGE] - USING DEBUG MODE");
		}

		lvo = false;
		getRate();

		if (ctotOption == "cid") {
			ctotCid = true;
		}
		else {
			ctotCid = false;
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
				if (lineValue.substr(0, 1) != "#") {
					TxtTimesVector.push_back(lineValue);
				}
			}
		}
		else {
			if (debugMode) {
				sendMessage("[DEBUG MESSAGE] - USING TAXIZONES FROM URL");
			}
			getTaxiZonesFromUrl(taxiZonesUrl);
		}
		getFlowData();
		fstream fileCtot;
		string lineValueCtot;
		fileCtot.open(cfad.c_str(), std::ios::in);
		while (getline(fileCtot, lineValueCtot))
		{
			addCtotToMainList(lineValueCtot);
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
		sendMessage("Done");
		return true;
	}

	if (startsWith(".cdm refreshtime", sCommandLine)) {
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

	if (startsWith(".cdm help", sCommandLine))
	{
		sendMessage("CDM Commands: .cdm reload - .cdm ctot - .cdm save - .cdm load - .cdm master {airport} - .cdm slave {airport} - .cdm refreshtime {seconds} - .cdm delay {minutes} - .cdm lvo on - .cdm lvo off");
		return true;
	}

	if (startsWith(".cdm save", sCommandLine))
	{
		sendMessage("Saving CDM data....");
		//save data to file
		ofstream outfile(sfad.c_str());

		for (Plane pl : slotList)
		{
			if (pl.hasCtot) {
				outfile << pl.callsign + "," + pl.eobt + "," + pl.tsat + "," + pl.ttot + "," + pl.ctot + ",c" << std::endl;
			}
			else {
				outfile << pl.callsign + "," + pl.eobt + "," + pl.tsat + "," + pl.ttot << std::endl;
			}
		}

		outfile.close();
		sendMessage("Done");
		return true;
	}

	if (startsWith(".cdm delay", sCommandLine))
	{
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
			myTimeToAdd = calculateTime(hour + min + "00", stoi(line.substr(line.length() - 1)));
		}
		else {
			timeAdded = line.substr(line.length() - 2);
			myTimeToAdd = calculateTime(hour + min + "00", stoi(line.substr(line.length() - 2)));
		}
		sendMessage("Delay added: " + timeAdded + " minutes");
		addTime = true;
		return true;
	}

	if (startsWith(".cdm nvo", sCommandLine))
	{
		rateString = getFromXml("/CDM/rate/@ops");
		sendMessage("Normal Visibility Operations Rate Set: " + rateString);
		return true;
	}

	if (startsWith(".cdm load", sCommandLine))
	{
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
		sendMessage("Loading CTOTs data....");
		sendMessage("Command disabled...");
		/*
		ctotList.clear();
		CTOTcheck.clear();
		slotList.clear();
		//load data from file
		fstream file;
		string lineValue;
		file.open(cfad.c_str(), std::ios::in);
		while (getline(file, lineValue))
		{
			addCtotToMainList(lineValue);
		}
		sendMessage("Done");
		*/
		return true;
	}

	if (startsWith(".cdm ctotTime", sCommandLine))
	{
		sendMessage("Reloading Ctot Expired time....");
		expiredCTOTTime = stoi(getFromXml("/CDM/expiredCtot/@time"));
		sendMessage("Done");
		return true;
	}

	if (startsWith(".cdm lvo on", sCommandLine))
	{
		if (!lvo) {
			sendMessage("Low Visibility Operations activated");
			lvo = true;
		}
		else {
			sendMessage("Low Visibility Operations already activated");
		}
		return true;
	}

	if (startsWith(".cdm lvo off", sCommandLine))
	{
		if (lvo) {
			sendMessage("Low Visibility Operations desactivated");
			lvo = false;
		}
		else {
			sendMessage("Low Visibility Operations not activated");
		}
		return true;
	}

	if (startsWith(".cdm nvo", sCommandLine))
	{
		rateString = getFromXml("/CDM/rate/@ops");
		sendMessage("Normal Visibility Operations Rate Set: " + rateString);
		return true;
	}

	if (startsWith(".cdm master", sCommandLine))
	{
		string line = sCommandLine; boost::to_upper(line);
		if (line.substr(line.length() - 7, 1) == " ") {
			sendMessage("NO AIRPORT SET");
		}
		else {
			string addedAirport = line.substr(line.length() - 4, 4);
			bool found = false;
			for (string apt : masterAirports)
			{
				if (apt == addedAirport) {
					found = true;
				}
			}
			if (!found) {
				masterAirports.push_back(addedAirport);
				sendMessage("ADDED " + addedAirport + " TO MASTER AIRPORTS");
			}
			else {
				sendMessage("AIRPORT " + addedAirport + " ALREADY ADDED");
			}

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
		}
		return true;
	}

	if (startsWith(".cdm slave", sCommandLine))
	{
		string line = sCommandLine; boost::to_upper(line);
		if (line.substr(line.length() - 6, 1) == " ") {
			sendMessage("NO AIRPORT SET");
		}
		else {
			string addedAirport = line.substr(line.length() - 4, 4);
			int pos = 0;
			bool found = false;
			for (int i = 0; i < masterAirports.size(); i++)
			{
				if (masterAirports[i].substr(masterAirports[i].find(",") + 1, 4) == addedAirport) {
					pos = i;
					found = true;
				}
			}
			if (found) {
				masterAirports.erase(masterAirports.begin() + pos);
				sendMessage("REMOVED " + addedAirport + " TO MASTER AIPORTS LIST");
			}
			else {
				sendMessage("AIRPORT " + addedAirport + " NOT FOUND");
			}

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
		}
		return true;
	}

	if (startsWith(".cdm status", sCommandLine))
	{
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
