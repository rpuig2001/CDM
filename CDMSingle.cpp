#include "stdafx.h"
#include "CDMSingle.hpp"
#include "pugixml.hpp"
#include "pugixml.cpp"

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
string rateString;
string lvoRateString;
string ctotOption;
int expiredCTOTTime;
bool defaultRate;
int countTime;
int refreshTime;
bool addTime;
bool lvo;
bool ctotCid;
string myTimeToAdd;
string taxiZonesUrl;

vector<string> slotList;
vector<string> tsacList;
vector<string> asatList;
vector<string> asrtList;
vector<string> taxiTimesList;
vector<string> TxtTimesVector;
vector<string> OutOfTsat;
vector<string> listA;
vector<string> ctotList;
vector<string> colors;
vector<string> rate;
vector<string> planeAiportList;
vector<string> masterAirports;
vector<string> CDMairports;
vector<string> CTOTcheck;

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
	RegisterTagItemFunction("Send REA Message + Set ASRT", TAG_FUNC_REAASRT);
	RegisterTagItemFunction("Send REA Message", TAG_FUNC_REA);

	//Register Tag Item "CDM-TOBT"
	RegisterTagItemType("TOBT", TAG_ITEM_TOBT);

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

	// Register Tag Item "CDM-A"
	RegisterTagItemType("A", TAG_ITEM_A);
	RegisterTagItemFunction("Toggle A", TAG_FUNC_TOGGLEA);

	// Register Tag Item "CDM-E"
	RegisterTagItemType("E", TAG_ITEM_E);

	// Register Tag Item "CDM-CTOT"
	RegisterTagItemType("CTOT", TAG_ITEM_CTOT);
	RegisterTagItemFunction("Open CTOT Option list", TAG_FUNC_CTOTOPTIONS);

	GetModuleFileNameA(HINSTANCE(&__ImageBase), DllPathFile, sizeof(DllPathFile));
	pfad = DllPathFile;
	pfad.resize(pfad.size() - strlen("CDM.dll"));
	pfad += "CDMconfig.xml";

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

	countTime = 0;
	refreshTime = 30;
	addTime = false;

	GetVersion();

	//Get data from xml config file
	//airport = getFromXml("/CDM/apt/@icao");
	ctotOption = getFromXml("/CDM/ctot/@option");
	expiredCTOTTime = stoi(getFromXml("/CDM/expiredCtot/@time"));
	rateString = getFromXml("/CDM/rate/@ops");
	lvoRateString = getFromXml("/CDM/rateLvo/@ops");
	taxiZonesUrl = getFromXml("/CDM/Taxizones/@url");
	lvo = false;
	getRate();

	if (ctotOption == "cid") {
		ctotCid = true;
	}
	else {
		ctotCid = false;
	}


	if (taxiZonesUrl.length() <= 1) {
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

	if (FunctionId == TAG_FUNC_NEWEOBT) {
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
	if (FunctionId == TAG_FUNC_ADDTSAC) {
		//Get Time now
		long int timeNow = static_cast<long int>(std::time(nullptr));
		string completeTime = unixTimeToHumanReadable(timeNow);
		string hour = "";
		string min = "";

		hour = completeTime.substr(completeTime.find(":") - 2, 2);

		if (completeTime.substr(completeTime.find(":") + 3, 1) == ":") {
			min = completeTime.substr(completeTime.find(":") + 1, 2);
		}
		else {
			min = completeTime.substr(completeTime.find(":") + 1, 1);
		}

		if (stoi(min) < 10) {
			min = "0" + min;
		}
		if (stoi(hour) < 10) {
			hour = "0" + hour.substr(1, 1);
		}
		bool notYetEOBT = false;
		string completeEOBT = (string)fp.GetFlightPlanData().GetEstimatedDepartureTime();
		completeEOBT = formatTime(completeEOBT);
		string EOBThour = completeEOBT.substr(0, 2);
		string EOBTmin = completeEOBT.substr(2, 2);

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
		for (int i = 0; i < tsacList.size(); i++)
		{
			if (tsacList[i].substr(0, tsacList[i].find(",")) == fp.GetCallsign()) {
				tsacList.erase(tsacList.begin() + i);
			}
		}
		if (!notYetEOBT) {
			for (int a = 0; a < slotList.size(); a++)
			{
				if (slotList[a].substr(0, slotList[a].find(",")) == fp.GetCallsign()) {
					string getTSAT = slotList[a].substr(slotList[a].find(",") + 8, 6);
					string valuesToAdd = (string)fp.GetCallsign() + "," + getTSAT;
					tsacList.push_back(valuesToAdd);
				}
			}
		}
	}

	if (FunctionId == TAG_FUNC_EDITTSAC) {
		string tsacValue = "";
		for (int i = 0; i < tsacList.size(); i++)
		{
			if (tsacList[i].substr(0, tsacList[i].find(",")) == fp.GetCallsign()) {
				tsacValue = tsacList[i].substr(tsacList[i].find(",") + 1, 4);
			}
		}
		OpenPopupEdit(Area, TAG_FUNC_NEWTSAC, tsacValue.c_str());
	}

	if (FunctionId == TAG_FUNC_NEWTSAC) {
		for (int i = 0; i < tsacList.size(); i++)
		{
			if (tsacList[i].substr(0, tsacList[i].find(",")) == fp.GetCallsign()) {
				tsacList.erase(tsacList.begin() + i);
			}
		}
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
					string valuesToAdd = (string)fp.GetCallsign() + "," + (editedTSAC + "00");
					tsacList.push_back(valuesToAdd);
				}
			}
		}
	}

	if (FunctionId == TAG_FUNC_TOGGLEA) {
		bool callsignFound = false;
		int Apos;
		for (int i = 0; i < listA.size(); i++)
		{
			if (listA[i] == (string)fp.GetCallsign()) {
				callsignFound = true;
				Apos = i;
			}
		}
		if (!callsignFound) {
			listA.push_back(fp.GetCallsign());
		}
		else {
			listA.erase(listA.begin() + Apos);
		}
	}

	if (FunctionId == TAG_FUNC_TOGGLEASRT) {
		if (master && AtcMe) {
			string remarks = fp.GetFlightPlanData().GetRemarks();
			bool callsignFound = false;
			int ASRTpos;
			for (int i = 0; i < asrtList.size(); i++)
			{
				if ((string)fp.GetCallsign() == asrtList[i].substr(0, asrtList[i].find(","))) {
					callsignFound = true;
					ASRTpos = i;
				}
			}
			if (!callsignFound) {
				//Get Time now
				long int timeNow = static_cast<long int>(std::time(nullptr));
				string completeTime = unixTimeToHumanReadable(timeNow);
				string hour = "";
				string min = "";

				hour = completeTime.substr(completeTime.find(":") - 2, 2);

				if (completeTime.substr(completeTime.find(":") + 3, 1) == ":") {
					min = completeTime.substr(completeTime.find(":") + 1, 2);
				}
				else {
					min = completeTime.substr(completeTime.find(":") + 1, 1);
				}

				if (stoi(min) < 10) {
					min = "0" + min;
				}
				if (stoi(hour) < 10) {
					hour = "0" + hour.substr(1, 1);
				}

				asrtList.push_back((string)fp.GetCallsign() + "," + hour + min);
				if (remarks.find("ASRT") == string::npos) {
					if (remarks.find("CTOT") != string::npos) {
						string stringToAdd = remarks.substr(0, remarks.find("CTOT")) + "ASRT" + hour + min + " " + remarks.substr(remarks.find("CTOT"), remarks.length() - remarks.find("CTOT"));
						fp.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
						fp.GetFlightPlanData().AmendFlightPlan();
						remarks = stringToAdd;
					}
					else if (remarks.find("%") != string::npos) {
						string stringToAdd = remarks.substr(0, remarks.find("%")) + "ASRT" + hour + min + " " + remarks.substr(remarks.find("%"), remarks.length() - remarks.find("%"));
						fp.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
						fp.GetFlightPlanData().AmendFlightPlan();
						remarks = stringToAdd;
					}
					else {
						string stringToAdd = remarks + " ASRT" + hour + min;
						fp.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
						fp.GetFlightPlanData().AmendFlightPlan();
						remarks = stringToAdd;
					}
				}
			}
			else {
				asrtList.erase(asrtList.begin() + ASRTpos);
				if (remarks.find("ASRT") != string::npos) {
					if (remarks.find("CTOT") != string::npos) {
						string stringToAdd = remarks.substr(0, remarks.find("ASRT")) + remarks.substr(remarks.find("CTOT"), remarks.length() - remarks.find("CTOT"));
						fp.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
						fp.GetFlightPlanData().AmendFlightPlan();
						remarks = stringToAdd;
					}
					else if (remarks.find("%") != string::npos) {
						string stringToAdd = remarks.substr(0, remarks.find("ASRT")) + remarks.substr(remarks.find("%"), remarks.length() - remarks.find("%"));
						fp.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
						fp.GetFlightPlanData().AmendFlightPlan();
						remarks = stringToAdd;
					}
					else {
						string stringToAdd = remarks.substr(0, remarks.find("ASRT") - 1);
						fp.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
						fp.GetFlightPlanData().AmendFlightPlan();
						remarks = stringToAdd;
					}
				}
			}
		}
	}

	if (FunctionId == TAG_FUNC_CTOTOPTIONS) {
		if (master && AtcMe) {
			bool hasCTOT = false;
			for (int i = 0; i < ctotList.size(); i++)
			{
				if (ctotList[i].substr(0, ctotList[i].find(",")) == fp.GetCallsign()) {
					hasCTOT = true;
				}
			}
			if (hasCTOT) {
				OpenPopupList(Area, "CTOT Options", 1);
				AddPopupListElement("Remove CTOT", "", TAG_FUNC_REMOVECTOT, false, 2, false);
			}
		}
	}

	if (FunctionId == TAG_FUNC_REMOVECTOT) {
		for (int a = 0; a < slotList.size(); a++)
		{
			if (slotList[a].substr(0, slotList[a].find(",")) == fp.GetCallsign()) {
				slotList.erase(slotList.begin() + a);
			}
		}

		for (int i = 0; i < ctotList.size(); i++)
		{
			if (ctotList[i].substr(0, ctotList[i].find(",")) == fp.GetCallsign()) {
				ctotList.erase(ctotList.begin() + i);
				string remarks = fp.GetFlightPlanData().GetRemarks();
				if (remarks.find("CTOT") != string::npos) {
					if (remarks.find("%") != string::npos) {
						string stringToAdd = remarks.substr(0, remarks.find("CTOT")) + remarks.substr(remarks.find("%"), remarks.length() - remarks.find("%"));
						fp.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
						fp.GetFlightPlanData().AmendFlightPlan();
						remarks = stringToAdd;
					}
					else {
						string stringToAdd = remarks.substr(0, remarks.find("CTOT") - 1);
						fp.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
						fp.GetFlightPlanData().AmendFlightPlan();
						remarks = stringToAdd;
					}
				}
			}
		}
	}

	if (FunctionId == TAG_FUNC_REAASRT) {
		if (master && AtcMe) {
			fp.GetFlightPlanData().SetEstimatedDepartureTime(EobtPlusTime(fp.GetFlightPlanData().GetEstimatedDepartureTime(), stoi(getFromXml("/CDM/ReaMsg/@minutes"))).substr(0, 4).c_str());
			fp.GetFlightPlanData().AmendFlightPlan();

			string remarks = fp.GetFlightPlanData().GetRemarks();
			bool callsignFound = false;
			int ASRTpos;
			for (int i = 0; i < asrtList.size(); i++)
			{
				if ((string)fp.GetCallsign() == asrtList[i].substr(0, asrtList[i].find(","))) {
					callsignFound = true;
					ASRTpos = i;
				}
			}

			//Get Time now
			long int timeNow = static_cast<long int>(std::time(nullptr));
			string completeTime = unixTimeToHumanReadable(timeNow);
			string hour = "";
			string min = "";

			hour = completeTime.substr(completeTime.find(":") - 2, 2);

			if (completeTime.substr(completeTime.find(":") + 3, 1) == ":") {
				min = completeTime.substr(completeTime.find(":") + 1, 2);
			}
			else {
				min = completeTime.substr(completeTime.find(":") + 1, 1);
			}

			if (stoi(min) < 10) {
				min = "0" + min;
			}
			if (stoi(hour) < 10) {
				hour = "0" + hour.substr(1, 1);
			}

			if (!callsignFound) {
				asrtList.push_back((string)fp.GetCallsign() + "," + hour + min);
				if (remarks.find("ASRT") == string::npos) {
					if (remarks.find("CTOT") != string::npos) {
						string stringToAdd = remarks.substr(0, remarks.find("CTOT")) + "ASRT" + hour + min + " " + remarks.substr(remarks.find("CTOT"), remarks.length() - remarks.find("CTOT"));
						fp.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
						fp.GetFlightPlanData().AmendFlightPlan();
						remarks = stringToAdd;
					}
					else if (remarks.find("%") != string::npos) {
						string stringToAdd = remarks.substr(0, remarks.find("%")) + "ASRT" + hour + min + " " + remarks.substr(remarks.find("%"), remarks.length() - remarks.find("%"));
						fp.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
						fp.GetFlightPlanData().AmendFlightPlan();
						remarks = stringToAdd;
					}
					else {
						string stringToAdd = remarks + " ASRT" + hour + min;
						fp.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
						fp.GetFlightPlanData().AmendFlightPlan();
						remarks = stringToAdd;
					}
				}
			}
			else {
				asrtList[ASRTpos] = (string)fp.GetCallsign() + "," + hour + min;
				if (remarks.find("ASRT") != string::npos) {
					if (remarks.find("CTOT") != string::npos) {
						string stringToAdd = remarks.substr(0, remarks.find("ASRT")) + "ASRT" + hour + min + " " + remarks.substr(remarks.find("CTOT"), remarks.length() - remarks.find("CTOT"));
						fp.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
						fp.GetFlightPlanData().AmendFlightPlan();
						remarks = stringToAdd;
					}
					else if (remarks.find("%") != string::npos) {
						string stringToAdd = remarks.substr(0, remarks.find("ASRT")) + "ASRT" + hour + min + " " + remarks.substr(remarks.find("%"), remarks.length() - remarks.find("%"));
						fp.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
						fp.GetFlightPlanData().AmendFlightPlan();
						remarks = stringToAdd;
					}
					else {
						string stringToAdd = remarks + " ASRT" + hour + min;
						fp.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
						fp.GetFlightPlanData().AmendFlightPlan();
						remarks = stringToAdd;
					}
				}
			}
		}
	}

	if (FunctionId == TAG_FUNC_REA) {
		if (master && AtcMe) {
			fp.GetFlightPlanData().SetEstimatedDepartureTime(EobtPlusTime(fp.GetFlightPlanData().GetEstimatedDepartureTime(), stoi(getFromXml("/CDM/ReaMsg/@minutes"))).substr(0, 4).c_str());
			fp.GetFlightPlanData().AmendFlightPlan();
		}
	}
}

void CDM::OnFlightPlanDisconnect(CFlightPlan FlightPlan) {
	string callsign = FlightPlan.GetCallsign();
	//Delete from vector
	for (int i = 0; i < slotList.size(); i++) {
		if ((string)FlightPlan.GetCallsign() == slotList[i].substr(0, slotList[i].find(","))) {
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
	//Remove Taxi Times List
	for (int j = 0; j < taxiTimesList.size(); j++)
	{
		if (taxiTimesList[j].substr(0, taxiTimesList[j].find(",")) == callsign) {
			taxiTimesList.erase(taxiTimesList.begin() + j);
		}
	}
	//Remove TSAC
	for (int i = 0; i < tsacList.size(); i++)
	{
		if (tsacList[i].substr(0, tsacList[i].find(",")) == callsign) {
			tsacList.erase(tsacList.begin() + i);
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

	//Remove ASRT
	for (int x = 0; x < asrtList.size(); x++)
	{
		string actualListCallsign = asrtList[x].substr(0, asrtList[x].find(","));
		if (actualListCallsign == callsign) {
			asrtList.erase(asrtList.begin() + x);
		}
	}

	//Remove from listA
	for (int i = 0; i < listA.size(); i++)
	{
		if (listA[i] == callsign) {
			listA.erase(listA.begin() + i);
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


void CDM::OnGetTagItem(CFlightPlan FlightPlan, CRadarTarget RadarTarget, int ItemCode, int TagData, char sItemString[16], int* pColorCode, COLORREF* pRGB, double* pFontSize)
{
	COLORREF ItemRGB = 0xFFFFFFFF;
	string callsign = FlightPlan.GetCallsign();

	string origin = FlightPlan.GetFlightPlanData().GetOrigin(); boost::to_upper(origin);
	string destination = FlightPlan.GetFlightPlanData().GetDestination(); boost::to_upper(destination);
	string remarks = FlightPlan.GetFlightPlanData().GetRemarks();

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

		if(isCDMairport)
		{
			const char* EOBT = "";
			const char* TSAT = "";
			const char* TTOT = "";
			int taxiTime = 15;

			//If aircraft is in aircraftFind Base vector
			int pos;
			bool aircraftFind = false;
			for (int i = 0; i < slotList.size(); i++) {
				if (slotList[i].substr(slotList[i].length() - 1, 1) == "c") {
					if (expiredCtot(slotList[i])) {
						slotList.erase(slotList.begin() + i);
						for (int a = 0; a < ctotList.size(); a++)
						{
							if (ctotList[a].substr(0, ctotList[a].find(",")) == callsign) {
								ctotList.erase(ctotList.begin() + a);
							}
						}
					}
				}
				if (callsign == slotList[i].substr(0, slotList[i].find(","))) {
					aircraftFind = true;
					pos = i;
				}
			}


			//Check if has CTOT
			bool hasCTOT = false;
			int ctotPos = 0;

			//If opion is "cid"
			if (ctotCid) {
				bool ctotValidated = false;
				for (int i = 0; i < CTOTcheck.size(); i++) {
					if (callsign == CTOTcheck[i]) {
						ctotValidated = true;
					}
				}

				if (!ctotValidated && !aircraftFind) {
					string cid = getCidByCallsign(callsign);
					string savedCid;
					string ctotCallsign;
					for (int i = 0; i < slotList.size(); i++)
					{
						savedCid = slotList[i].substr(0, slotList[i].find(","));
						if (checkIsNumber(savedCid)) {
							if (stoi(cid) == stoi(savedCid)) {
								slotList[i] = callsign + slotList[i].substr(slotList[i].find(","), slotList[i].length() - slotList[i].find(","));
								pos = i;
								for (int a = 0; a < ctotList.size(); a++)
								{
									ctotCallsign = ctotList[a].substr(0, ctotList[a].find(","));
									if (checkIsNumber(ctotCallsign)) {
										if (stoi(cid) == stoi(ctotCallsign)) {
											ctotList[a] = callsign + ctotList[a].substr(ctotList[a].find(","), ctotList[a].length() - ctotList[a].find(","));
											hasCTOT = true;
											ctotPos = i;
										}
									}
								}
							}
						}
					}
					CTOTcheck.push_back(callsign);
				}
			}

			//EOBT
			EOBT = FlightPlan.GetFlightPlanData().GetEstimatedDepartureTime();
			string EOBTstring = EOBT;
			string EOBTfinal = formatTime(EOBTstring);
			EOBTfinal += "00";
			EOBT = EOBTfinal.c_str();
			bool stillOutOfTsat = false;
			int stillOutOfTsatPos;

			//If column A set
			bool hasValueInA = false;
			for (int i = 0; i < listA.size(); i++)
			{
				if (listA[i] == (string)FlightPlan.GetCallsign()) {
					hasValueInA = true;
				}
			}

			//CTOT
			for (int i = 0; i < ctotList.size(); i++)
			{
				if (callsign == ctotList[i].substr(0, ctotList[i].find(","))) {
					hasCTOT = true;
					ctotPos = i;
				}
			}

			//Get Time NOW
			long int timeNow = static_cast<long int>(std::time(nullptr));
			string completeTime = unixTimeToHumanReadable(timeNow);
			string hour = "";
			string min = "";

			hour = completeTime.substr(completeTime.find(":") - 2, 2);

			if (completeTime.substr(completeTime.find(":") + 3, 1) == ":") {
				min = completeTime.substr(completeTime.find(":") + 1, 2);
			}
			else {
				min = completeTime.substr(completeTime.find(":") + 1, 1);
			}

			if (stoi(min) < 10) {
				min = "0" + min;
			}
			if (stoi(hour) < 10) {
				hour = "0" + hour.substr(1, 1);
			}

			bool stsDepa = false;
			if ((string)FlightPlan.GetGroundState() == "DEPA") {
				stsDepa = true;
				if (remarks.find("%") != string::npos) {
					string stringToAdd = remarks.substr(0, remarks.find("%") - 1);
					FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
					FlightPlan.GetFlightPlanData().AmendFlightPlan();
					remarks = stringToAdd;
				}
				if (remarks.find("CTOT") != string::npos) {
					string stringToAdd = remarks.substr(0, remarks.find("CTOT") - 1);
					FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
					FlightPlan.GetFlightPlanData().AmendFlightPlan();
					remarks = stringToAdd;
				}
				if (remarks.find("ASRT") != string::npos) {
					string stringToAdd = remarks.substr(0, remarks.find("ASRT") - 1);
					FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
					FlightPlan.GetFlightPlanData().AmendFlightPlan();
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
							slotList.erase(slotList.begin() + pos);
							aircraftFind = false;
						}
					}
				}
			}

			if (!planeHasTaxiTimeAssigned) {
				if (RadarTargetSelect(callsign.c_str()).IsValid()) {
					double lat = RadarTargetSelect(callsign.c_str()).GetPosition().GetPosition().m_Latitude;
					double lon = RadarTargetSelect(callsign.c_str()).GetPosition().GetPosition().m_Longitude;
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

			if (master) {

				bool gndStatusSet = false;
				if ((string)FlightPlan.GetGroundState() == "STUP" || (string)FlightPlan.GetGroundState() == "ST-UP" || (string)FlightPlan.GetGroundState() == "PUSH" || (string)FlightPlan.GetGroundState() == "TAXI" || (string)FlightPlan.GetGroundState() == "DEPA") {
					gndStatusSet = true;
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
						string ShowEOBT = (string)EOBT;
						ItemRGB = TAG_EOBT;
						strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
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
						if (hasCTOT) {
							ItemRGB = TAG_CTOT;
							strcpy_s(sItemString, 16, ctotList[ctotPos].substr(ctotList[ctotPos].find(",") + 1, 4).c_str());
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

									if (hasCTOT) {
										tempTTOT = calculateTime(slotList[pos].substr(slotList[pos].length() - 8, 6), taxiTime);
										tempTSAT = calculateTime(slotList[pos].substr(slotList[pos].length() - 15, 6), taxiTime);
									}
									else {
										tempTTOT = calculateTime(slotList[pos].substr(slotList[pos].length() - 6, 6), taxiTime);
										tempTSAT = calculateTime(slotList[pos].substr(slotList[pos].length() - 13, 6), taxiTime);
									}

									slotList[pos] = callsign + "," + EOBT + "," + tempTSAT + "," + tempTTOT;
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
						if (tempEOBT != slotList[pos].substr(slotList[pos].find(",") + 1, 6) && !hasCTOT) {
							//aircraftFind false to recalculate Times due to fp change
							slotList.erase(slotList.begin() + pos);
							aircraftFind = false;
							if (remarks.find("%") != string::npos) {
								string stringToAdd = remarks.substr(0, remarks.find("%") - 1);
								FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
								FlightPlan.GetFlightPlanData().AmendFlightPlan();
								remarks = stringToAdd;
							}
						}
					}

					if (!aircraftFind) {
						if (hasCTOT) {
							//TTOT with CTOT
							TTOTFinal = formatTime(ctotList[ctotPos].substr(ctotList[ctotPos].find(",") + 1, 4) + "00");
							TTOT = TTOTFinal.c_str();

							//TSAT with CTOT
							TSATfinal = calculateLessTime(TTOTFinal, taxiTime);
							TSAT = TSATfinal.c_str();

							//IF EOBT+TaxiTime >= CTOT+10 THEN CTOT LOST
							if (stoi(calculateTime(EOBT, taxiTime)) > stoi(calculateTime(TTOTFinal, 10))) {
								hasCTOT = false;
								if (remarks.find("CTOT") != string::npos) {
									if (remarks.find("%") != string::npos) {
										string stringToAdd = remarks.substr(0, remarks.find("CTOT")) + remarks.substr(remarks.find("%"), remarks.length() - remarks.find("%"));
										FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
										FlightPlan.GetFlightPlanData().AmendFlightPlan();
										remarks = stringToAdd;
									}
									else {
										string stringToAdd = remarks.substr(0, remarks.find("CTOT") - 1);
										FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
										FlightPlan.GetFlightPlanData().AmendFlightPlan();
										remarks = stringToAdd;
									}
								}
							}
							else if ((stoi(calculateTime(EOBT, taxiTime)) <= stoi(calculateTime(TTOTFinal, 10))) && stoi(EOBT) > (stoi(TSATfinal))) {
								TSAT = EOBT;
								//TSAT
								string TSATstring = TSAT;
								TSATfinal = formatTime(TSATstring);
								TSAT = TSATfinal.c_str();

								//TTOT
								TTOTFinal = calculateTime(TSATstring, taxiTime);
								TTOT = TTOTFinal.c_str();
							}
						}

						if (!hasCTOT) {
							TSAT = EOBT;
							//TSAT
							string TSATstring = TSAT;
							TSATfinal = formatTime(TSATstring);
							TSAT = TSATfinal.c_str();

							//TTOT
							TTOTFinal = calculateTime(TSATstring, taxiTime);
							TTOT = TTOTFinal.c_str();
						}
					}
					else {
						if (hasCTOT) {
							//TSAT
							string TSATstring = slotList[pos].substr(slotList[pos].length() - 15, 6);
							TSATfinal = formatTime(TSATstring);
							TSAT = TSATfinal.c_str();

							//TTOT
							TTOTFinal = slotList[pos].substr(slotList[pos].length() - 8, 6);
							TTOT = TTOTFinal.c_str();
						}
						else {
							//TSAT
							string TSATstring = slotList[pos].substr(slotList[pos].length() - 13, 6);
							TSATfinal = formatTime(TSATstring);
							TSAT = TSATfinal.c_str();

							//TTOT
							TTOTFinal = slotList[pos].substr(slotList[pos].length() - 6, 6);
							TTOT = TTOTFinal.c_str();
						}
					}

				

					bool equalTTOT = true;
					bool correctTTOT = true;
					bool equalTempoTTOT = true;
					bool alreadySetTOStd = false;

					if (!aircraftFind) {
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

						while (equalTTOT) {
							correctTTOT = true;
							for (int t = 0; t < slotList.size(); t++)
							{
								string listTTOT;
								string listCallsign = slotList[t].substr(0, slotList[t].find(","));
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

								if (hasCTOT) {
									bool found = false;
									while (!found) {
										found = true;
										if (slotList[t].substr(slotList[t].length() - 1, 1) == "c") {

											listTTOT = slotList[t].substr(slotList[t].length() - 8, 6);

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
										if (slotList[t].substr(slotList[t].length() - 1, 1) == "c") {
											listTTOT = slotList[t].substr(slotList[t].length() - 8, 6);
										}
										else {
											listTTOT = slotList[t].substr(slotList[t].length() - 6, 6);
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
								equalTTOT = false;
								TSATfinal = calculateLessTime(TTOTFinal, taxiTime);
								TSAT = TSATfinal.c_str();
								TTOT = TTOTFinal.c_str();
								if (hasCTOT) {
									if (aircraftFind) {
										if (TTOTFinal != slotList[pos].substr(slotList[pos].length() - 8, 6)) {
											string valueToAdd = callsign + "," + EOBT + "," + TSAT + "," + TTOT + ",c";
											slotList[pos] = valueToAdd;

											if (remarks.find("CTOT") != string::npos) {
												string stringToAdd = remarks.substr(0, remarks.find("CTOT") - 1);
												FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
												remarks = stringToAdd;
											}
											else if (remarks.find("%") != string::npos) {
												string stringToAdd = remarks.substr(0, remarks.find("%") - 1);
												FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
												remarks = stringToAdd;
											}

											string stringToAdd = remarks + " CTOT" + ctotList[ctotPos].substr(ctotList[ctotPos].find(",") + 1, 4) + " %" + TSAT + "|" + TTOT;
											FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
											FlightPlan.GetFlightPlanData().AmendFlightPlan();
											remarks = stringToAdd;
										}
									}
									else {
										string valueToAdd = callsign + "," + EOBT + "," + TSAT + "," + TTOT + ",c";
										slotList.push_back(valueToAdd);

										if (remarks.find("CTOT") != string::npos) {
											string stringToAdd = remarks.substr(0, remarks.find("CTOT") - 1);
											FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
											remarks = stringToAdd;
										}
										else if (remarks.find("%") != string::npos) {
											string stringToAdd = remarks.substr(0, remarks.find("%") - 1);
											FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
											remarks = stringToAdd;
										}

										string stringToAdd = remarks + " CTOT" + ctotList[ctotPos].substr(ctotList[ctotPos].find(",") + 1, 4) + " %" + TSAT + "|" + TTOT;
										FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
										FlightPlan.GetFlightPlanData().AmendFlightPlan();
										remarks = stringToAdd;
									}
								}
								else {
									if (aircraftFind) {
										if (TTOTFinal != slotList[pos].substr(slotList[pos].length() - 6, 6)) {
											string valueToAdd = callsign + "," + EOBT + "," + TSAT + "," + TTOT;
											slotList[pos] = valueToAdd;

											if (remarks.find("%") != string::npos) {
												string stringToAdd = remarks.substr(0, remarks.find("%") - 1);
												FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
												remarks = stringToAdd;
											}

											string stringToAdd = remarks + " %" + TSAT + "|" + TTOT;
											FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
											FlightPlan.GetFlightPlanData().AmendFlightPlan();
											remarks = stringToAdd;
										}
									}
									else {
										string valueToAdd = callsign + "," + EOBT + "," + TSAT + "," + TTOT;
										slotList.push_back(valueToAdd);

										if (remarks.find("%") != string::npos) {
											string stringToAdd = remarks.substr(0, remarks.find("%") - 1);
											FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
											remarks = stringToAdd;
										}

										string stringToAdd = remarks + " %" + TSAT + "|" + TTOT;
										FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
										FlightPlan.GetFlightPlanData().AmendFlightPlan();
										remarks = stringToAdd;
									}
								}
							}
						}
					}

					countTime += 1;
					//Refresh times every x sec
					if (countTime > refreshTime * 1000) {
						countTime = 0;

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
							string myTTOT, myTSAT, myEOBT, myCallsign, myAirport, myDepRwy = "", myRemarks;
							int myTTime = 15;

							myCallsign = slotList[i].substr(0, slotList[i].find(","));
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

							myRemarks = myFlightPlan.GetFlightPlanData().GetRemarks();
							bool myhasCTOT = false;
							int myCtotPos = 0;
							for (int s = 0; s < ctotList.size(); s++)
							{
								if (myCallsign == ctotList[s].substr(0, ctotList[s].find(","))) {
									myhasCTOT = true;
									myCtotPos = s;
								}
							}

							myEOBT = slotList[i].substr(slotList[i].find(",") + 1, 6);

							if (myhasCTOT) {
								//TSAT and TTOT with CTOT
								myTTOT = formatTime(ctotList[myCtotPos].substr(ctotList[myCtotPos].find(",") + 1, 4) + "00");
								myTSAT = calculateLessTime(myTTOT, myTTime);
							}
							else {
								myTSAT = myEOBT;
								myTTOT = calculateTime(myEOBT, myTTime);
							}

							refreshTimes(myFlightPlan, myCallsign, myEOBT, myTSAT, myTTOT, myAirport, myTTime, myRemarks, myDepRwy, rateHour, myhasCTOT, myCtotPos, i, true);
						}
					}

					//Sync TTOT
					if (remarks.find("%") != string::npos) {
						if (TTOT != remarks.substr(remarks.find("%") + 8, 6)) {
							string stringToAdd = remarks.substr(0, remarks.find("%") - 1);
							FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
							FlightPlan.GetFlightPlanData().AmendFlightPlan();
							remarks = stringToAdd;
						}
					}
					else if (aircraftFind && !stsDepa) {
						string stringToAdd = remarks + " %" + TSAT + "|" + TTOT;
						FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
						FlightPlan.GetFlightPlanData().AmendFlightPlan();
						remarks = stringToAdd;
					}

					//Sync CTOT
					if (remarks.find("CTOT") != string::npos && hasCTOT) {
						string listCtot = ctotList[ctotPos].substr(ctotList[ctotPos].find(",") + 1, 4);
						if (listCtot != remarks.substr(remarks.find("CTOT") + 4, 4)) {
							if (remarks.find("%") != string::npos) {
								string stringToAdd = remarks.substr(0, remarks.find("%")) + "CTOT" + listCtot + " " + remarks.substr(remarks.find("%"), remarks.length() - remarks.find("%"));
								FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
								FlightPlan.GetFlightPlanData().AmendFlightPlan();
								remarks = stringToAdd;
							}
							else {
								string stringToAdd = remarks + " CTOT" + listCtot;
								FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
								FlightPlan.GetFlightPlanData().AmendFlightPlan();
								remarks = stringToAdd;
							}
						}
					}
					else if (hasCTOT && !stsDepa) {
						string listCtot = ctotList[ctotPos].substr(ctotList[ctotPos].find(",") + 1, 4);
						if (remarks.find("%") != string::npos) {
							string stringToAdd = remarks.substr(0, remarks.find("%")) + "CTOT" + listCtot + " " + remarks.substr(remarks.find("%"), remarks.length() - remarks.find("%"));
							FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
							FlightPlan.GetFlightPlanData().AmendFlightPlan();
							remarks = stringToAdd;
						}
						else {
							string stringToAdd = remarks + " CTOT" + listCtot;
							FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
							FlightPlan.GetFlightPlanData().AmendFlightPlan();
							remarks = stringToAdd;
						}
					}
					else if (remarks.find("CTOT") != string::npos && !hasCTOT) {
						if (remarks.find("%") != string::npos) {
							string stringToAdd = remarks.substr(0, remarks.find("CTOT")) + remarks.substr(remarks.find("%"), remarks.length() - remarks.find("%"));
							FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
							FlightPlan.GetFlightPlanData().AmendFlightPlan();
							remarks = stringToAdd;
						}
						else {
							string stringToAdd = remarks.substr(0, remarks.find("CTOT") - 1);
							FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
							FlightPlan.GetFlightPlanData().AmendFlightPlan();
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

					if (oldTSAT && !correctState) {
						OutOfTsat.push_back(callsign + "," + EOBT);
						for (int i = 0; i < asrtList.size(); i++)
						{
							if ((string)FlightPlan.GetCallsign() == asrtList[i].substr(0, asrtList[i].find(","))) {
								asrtList.erase(asrtList.begin() + i);
								if (remarks.find("ASRT") != string::npos) {
									string stringToAdd = remarks.substr(0, remarks.find("ASRT") - 1);
									FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
									remarks = stringToAdd;
								}
							}
						}
						if (remarks.find("CTOT") != string::npos) {
							string stringToAdd = remarks.substr(0, remarks.find("CTOT") - 1);
							FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
							remarks = stringToAdd;
						}
						if (remarks.find("%") != string::npos) {
							string stringToAdd = remarks.substr(0, remarks.find("%") - 1);
							FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
							remarks = stringToAdd;
						}
						FlightPlan.GetFlightPlanData().AmendFlightPlan();
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
					bool TSACFound = false;
					bool TSACNotTSAT = false;
					string ThisTSAC = "";
					for (int d = 0; d < tsacList.size(); d++)
					{
						if (callsign == tsacList[d].substr(0, tsacList[d].find(","))) {
							TSACFound = true;
							ThisTSAC = tsacList[d].substr(tsacList[d].find(",") + 1, 6);
						}
					}

					if (TSACFound) {
						string TSAChour = ThisTSAC.substr(ThisTSAC.length() - 6, 2);
						string TSACmin = ThisTSAC.substr(ThisTSAC.length() - 4, 2);

						int TSACDif = GetdifferenceTime(TSAThour, TSATmin, TSAChour, TSACmin);
						if (TSACDif > 5 || TSACDif < -5) {
							TSACNotTSAT = true;
						}
					}

					//ASRT
					bool ASRTFound = false;
					string ASRTtext = " ";
					for (int x = 0; x < asrtList.size(); x++)
					{
						string MyListCallsign = asrtList[x].substr(0, asrtList[x].find(","));
						if (MyListCallsign == callsign) {
							ASRTFound = true;
							ASRTtext = asrtList[x].substr(asrtList[x].find(",") + 1, 4);
						}
					}

					//Sync ASRT
					if (remarks.find("ASRT") != string::npos && ASRTFound) {
						if (ASRTtext != remarks.substr(remarks.find("ASRT") + 4, 4)) {
							if (remarks.find("CTOT") != string::npos) {
								string stringToAdd = remarks.substr(0, remarks.find("ASRT")) + "ASRT" + ASRTtext + " " + remarks.substr(remarks.find("CTOT"), remarks.length() - remarks.find("CTOT"));
								FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
								FlightPlan.GetFlightPlanData().AmendFlightPlan();
								remarks = stringToAdd;
							}
							else if (remarks.find("%") != string::npos) {
								string stringToAdd = remarks.substr(0, remarks.find("ASRT")) + "ASRT" + ASRTtext + " " + remarks.substr(remarks.find("%"), remarks.length() - remarks.find("%"));
								FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
								FlightPlan.GetFlightPlanData().AmendFlightPlan();
								remarks = stringToAdd;
							}
							else {
								string stringToAdd = remarks + " ASRT" + ASRTtext;
								FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
								FlightPlan.GetFlightPlanData().AmendFlightPlan();
								remarks = stringToAdd;
							}
						}
					}
					else if (ASRTFound && !stsDepa) {
						if (remarks.find("CTOT") != string::npos) {
							string stringToAdd = remarks.substr(0, remarks.find("CTOT")) + "ASRT" + ASRTtext + " " + remarks.substr(remarks.find("CTOT"), remarks.length() - remarks.find("CTOT"));
							FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
							FlightPlan.GetFlightPlanData().AmendFlightPlan();
							remarks = stringToAdd;
						}
						else if (remarks.find("%") != string::npos) {
							string stringToAdd = remarks.substr(0, remarks.find("%")) + "ASRT" + ASRTtext + " " + remarks.substr(remarks.find("%"), remarks.length() - remarks.find("%"));
							FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
							FlightPlan.GetFlightPlanData().AmendFlightPlan();
							remarks = stringToAdd;
						}
						else {
							string stringToAdd = remarks + " ASRT" + ASRTtext;
							FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
							FlightPlan.GetFlightPlanData().AmendFlightPlan();
							remarks = stringToAdd;
						}
					}
					else if (remarks.find("ASRT") != string::npos && !ASRTFound) {
						if (remarks.find("CTOT") != string::npos) {
							string stringToAdd = remarks.substr(0, remarks.find("ASRT")) + remarks.substr(remarks.find("CTOT"), remarks.length() - remarks.find("CTOT"));
							FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
							FlightPlan.GetFlightPlanData().AmendFlightPlan();
							remarks = stringToAdd;
						}
						else if (remarks.find("%") != string::npos) {
							string stringToAdd = remarks.substr(0, remarks.find("ASRT")) + remarks.substr(remarks.find("%"), remarks.length() - remarks.find("%"));
							FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
							FlightPlan.GetFlightPlanData().AmendFlightPlan();
							remarks = stringToAdd;
						}
						else {
							string stringToAdd = remarks.substr(0, remarks.find("ASRT") - 1);
							FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
							FlightPlan.GetFlightPlanData().AmendFlightPlan();
							remarks = stringToAdd;
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
						string ShowEOBT = (string)EOBT;
						//*pColorCode = TAG_COLOR_RGB_DEFINED;
						ItemRGB = TAG_EOBT;
						strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
					}
					else if (ItemCode == TAG_ITEM_TOBT)
					{
						string ShowEOBT = (string)EOBT;
						if (notYetEOBT) {
							//*pColorCode = TAG_COLOR_RGB_DEFINED;
							ItemRGB = TAG_GREENNOTACTIVE;
							strcpy_s(sItemString, 16, " ");
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
							strcpy_s(sItemString, 16, ThisTSAC.substr(0, ThisTSAC.length() - 2).c_str());
						}
						else if (TSACFound) {
							//*pColorCode = TAG_COLOR_RGB_DEFINED;
							ItemRGB = TAG_GREEN;
							strcpy_s(sItemString, 16, ThisTSAC.substr(0, ThisTSAC.length() - 2).c_str());
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
						if (notYetEOBT) {
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
							if (ASATPlusFiveLessTen) {
								//*pColorCode = TAG_COLOR_RGB_DEFINED;
								ItemRGB = TAG_YELLOW;
								strcpy_s(sItemString, 16, ASATtext.c_str());
							}
							else {
								//*pColorCode = TAG_COLOR_RGB_DEFINED;
								ItemRGB = TAG_GREEN;
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
						if (ASRTFound) {
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
					else if (ItemCode == TAG_ITEM_A)
					{
						if (hasValueInA) {
							//*pColorCode = TAG_COLOR_RGB_DEFINED;
							ItemRGB = TAG_YELLOW;
							strcpy_s(sItemString, 16, "A");
						}
						else {
							//*pColorCode = TAG_COLOR_RGB_DEFINED;
							ItemRGB = TAG_GREEN;
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
					else if (ItemCode == TAG_ITEM_CTOT)
					{
						if (hasCTOT) {
							//*pColorCode = TAG_COLOR_RGB_DEFINED;
							ItemRGB = TAG_CTOT;
							strcpy_s(sItemString, 16, ctotList[ctotPos].substr(ctotList[ctotPos].find(",") + 1, 4).c_str());
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
					else if (TSATString != slotList[pos].substr(slotList[pos].find(",") + 8, 6) && TSATFind) {
						if (hasCTOT) {
							string valueToAdd = callsign + "," + EOBT + "," + TSATString + "," + TTOTString + ",c";
							slotList[pos] = valueToAdd;
						}
						else {
							string valueToAdd = callsign + "," + EOBT + "," + TSATString + "," + TTOTString;
							slotList[pos] = valueToAdd;
						}
					}
				}
				else {
					if (TSATFind) {
						if (hasCTOT) {
							string valueToAdd = callsign + "," + EOBT + "," + TSATString + "," + TTOTString + ",c";
							slotList.push_back(valueToAdd.c_str());
						}
						else {
							string valueToAdd = callsign + "," + EOBT + "," + TSATString + "," + TTOTString;
							slotList.push_back(valueToAdd.c_str());
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

					if (oldTSAT && !correctState) {
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
						if (hasCTOT) {
							if (ctotList[ctotPos].substr(ctotList[ctotPos].find(",") + 4) != rmkCtot) {
								ctotList[ctotPos] == callsign + "," + rmkCtot;
								if (slotList[pos].substr(slotList[pos].length() - 1, 1) != "c") {
									slotList[pos] = slotList[pos] + ",c";
								}
							}
						}
						else {
							ctotList.push_back(callsign + "," + rmkCtot);
						}
					}
					else if (hasCTOT) {
						ctotList.erase(ctotList.begin() + ctotPos);
						if (slotList[pos].substr(slotList[pos].length() - 1, 1) == "c") {
							slotList[pos] = slotList[pos].substr(0, slotList[pos].length() - 2);
						}
					}

					//TSAC
					bool TSACFound = false;
					bool TSACNotTSAT = false;
					string ThisTSAC = "";
					for (int d = 0; d < tsacList.size(); d++)
					{
						if (callsign == tsacList[d].substr(0, tsacList[d].find(","))) {
							TSACFound = true;
							ThisTSAC = tsacList[d].substr(tsacList[d].find(",") + 1, 6);
						}
					}

					if (TSACFound) {
						string TSAChour = ThisTSAC.substr(ThisTSAC.length() - 6, 2);
						string TSACmin = ThisTSAC.substr(ThisTSAC.length() - 4, 2);

						int TSACDif = GetdifferenceTime(TSAThour, TSATmin, TSAChour, TSACmin);
						if (TSACDif > 5 || TSACDif < -5) {
							TSACNotTSAT = true;
						}
					}


					//ASRT
					bool ASRTFound = false;
					int ASRTPos;
					string ASRTtext = "";
					for (int x = 0; x < asrtList.size(); x++)
					{
						string MyListCallsign = asrtList[x].substr(0, asrtList[x].find(","));
						if (MyListCallsign == callsign) {
							ASRTFound = true;
							ASRTPos = x;
						}
					}

					if (ASRTFound) {
						if (remarks.find("ASRT") != string::npos) {
							//If ASRT changed
							if (asrtList[ASRTPos].substr(asrtList[ASRTPos].find(",") + 1, 4) != remarks.substr(remarks.find("ASRT") + 4, 4)) {
								ASRTtext = remarks.substr(remarks.find("ASRT") + 4, 4);
								asrtList[ASRTPos] = callsign + "," + ASRTtext;
							}
							else {
								ASRTtext = asrtList[ASRTPos].substr(asrtList[ASRTPos].find(",") + 1, 4);
							}
						}
						else {
							//If ASRT not in remarks but yes in list
							asrtList.erase(asrtList.begin() + ASRTPos);
							ASRTFound = false;
						}
					}
					else {
						if (remarks.find("ASRT") != string::npos) {
							//If Yes in remarks but not in list
							ASRTtext = remarks.substr(remarks.find("ASRT") + 4, 4);
							asrtList.push_back(callsign + "," + ASRTtext);
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
						string ShowEOBT = (string)EOBT;
						ItemRGB = TAG_EOBT;
						strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
					}
					else if (ItemCode == TAG_ITEM_TOBT)
					{
						string ShowEOBT = (string)EOBT;
						if (notYetEOBT) {
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
							strcpy_s(sItemString, 16, ThisTSAC.substr(0, ThisTSAC.length() - 2).c_str());
						}
						else if (TSACFound) {
							ItemRGB = TAG_GREEN;
							strcpy_s(sItemString, 16, ThisTSAC.substr(0, ThisTSAC.length() - 2).c_str());
						}
						else {
							ItemRGB = TAG_GREEN;
							strcpy_s(sItemString, 16, "____");
						}
					}
					else if (ItemCode == TAG_ITEM_TSAT)
					{
						if (TSATString.length() > 0) {
							if (notYetEOBT) {
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
							ItemRGB = TAG_GREEN;
							strcpy_s(sItemString, 16, " ");
						}
					}
					else if (ItemCode == TAG_ITEM_ASRT)
					{
						if (ASRTFound) {
							ItemRGB = TAG_ASRT;
							strcpy_s(sItemString, 16, ASRTtext.c_str());
						}
						else {
							ItemRGB = TAG_ASRT;
							strcpy_s(sItemString, 16, " ");
						}
					}
					else if (ItemCode == TAG_ITEM_A)
					{
						if (hasValueInA) {
							ItemRGB = TAG_YELLOW;
							strcpy_s(sItemString, 16, "A");
						}
						else {
							ItemRGB = TAG_GREEN;
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
					else if (ItemCode == TAG_ITEM_CTOT)
					{
						if (hasCTOT) {
							ItemRGB = TAG_CTOT;
							strcpy_s(sItemString, 16, ctotList[ctotPos].substr(ctotList[ctotPos].find(",") + 1, 4).c_str());
						}
					}
				}
				else
				{
					if (ItemCode == TAG_ITEM_EOBT)
					{
						string ShowEOBT = (string)EOBT;
						ItemRGB = TAG_EOBT;
						strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
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
				}
			}
		}
		else
		{
			if (ItemCode == TAG_ITEM_EOBT)
			{
				ItemRGB = TAG_EOBT;
				strcpy_s(sItemString, 16, FlightPlan.GetFlightPlanData().GetEstimatedDepartureTime());
			}
			else if (ItemCode == TAG_ITEM_TOBT)
			{
				ItemRGB = TAG_GREY;
				strcpy_s(sItemString, 16, FlightPlan.GetFlightPlanData().GetEstimatedDepartureTime());
			}
		}

		if (ItemRGB != 0xFFFFFFFF)
		{
			*pColorCode = TAG_COLOR_RGB_DEFINED;
			*pRGB = ItemRGB;
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
		myAirport = lineValue.substr(0,lineValue.find(":"));
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

	while (equalTTOT) {
		correctTTOT = true;
		for (int t = 0; t < slotList.size(); t++)
		{
			string listTTOT;
			string listCallsign = slotList[t].substr(0, slotList[t].find(","));
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

			if (hasCTOT) {
				bool found = false;
				while (!found) {
					found = true;
					if (slotList[t].substr(slotList[t].length() - 1, 1) == "c") {

						listTTOT = slotList[t].substr(slotList[t].length() - 8, 6);

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
					if (slotList[t].substr(slotList[t].length() - 1, 1) == "c") {
						listTTOT = slotList[t].substr(slotList[t].length() - 8, 6);
					}
					else {
						listTTOT = slotList[t].substr(slotList[t].length() - 6, 6);
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
					if (TTOTFinal != slotList[pos].substr(slotList[pos].length() - 8, 6)) {
						string valueToAdd = callsign + "," + EOBT + "," + TSAT + "," + TTOT + ",c";
						slotList[pos] = valueToAdd;

						if (remarks.find("CTOT") != string::npos) {
							string stringToAdd = remarks.substr(0, remarks.find("CTOT") - 1);
							FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
							remarks = stringToAdd;
						}
						else if (remarks.find("%") != string::npos) {
							string stringToAdd = remarks.substr(0, remarks.find("%") - 1);
							FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
							remarks = stringToAdd;
						}

						string stringToAdd = remarks + " CTOT" + ctotList[ctotPos].substr(ctotList[ctotPos].find(",") + 1, 4) + " %" + TSAT + "|" + TTOT;
						FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
						FlightPlan.GetFlightPlanData().AmendFlightPlan();
						remarks = stringToAdd;
					}
				}
				else {
					string valueToAdd = callsign + "," + EOBT + "," + TSAT + "," + TTOT + ",c";
					slotList.push_back(valueToAdd);

					if (remarks.find("CTOT") != string::npos) {
						string stringToAdd = remarks.substr(0, remarks.find("CTOT") - 1);
						FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
						remarks = stringToAdd;
					}
					else if (remarks.find("%") != string::npos) {
						string stringToAdd = remarks.substr(0, remarks.find("%") - 1);
						FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
						remarks = stringToAdd;
					}

					string stringToAdd = remarks + " CTOT" + ctotList[ctotPos].substr(ctotList[ctotPos].find(",") + 1, 4) + " %" + TSAT + "|" + TTOT;
					FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
					FlightPlan.GetFlightPlanData().AmendFlightPlan();
					remarks = stringToAdd;
				}
			}
			else {
				if (aircraftFind) {
					if (TTOTFinal != slotList[pos].substr(slotList[pos].length() - 6, 6)) {
						string valueToAdd = callsign + "," + EOBT + "," + TSAT + "," + TTOT;
						slotList[pos] = valueToAdd;

						if (remarks.find("%") != string::npos) {
							string stringToAdd = remarks.substr(0, remarks.find("%") - 1);
							FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
							remarks = stringToAdd;
						}

						string stringToAdd = remarks + " %" + TSAT + "|" + TTOT;
						FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
						FlightPlan.GetFlightPlanData().AmendFlightPlan();
						remarks = stringToAdd;
					}
				}
				else {
					string valueToAdd = callsign + "," + EOBT + "," + TSAT + "," + TTOT;
					slotList.push_back(valueToAdd);

					if (remarks.find("%") != string::npos) {
						string stringToAdd = remarks.substr(0, remarks.find("%") - 1);
						FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
						remarks = stringToAdd;
					}

					string stringToAdd = remarks + " %" + TSAT + "|" + TTOT;
					FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
					FlightPlan.GetFlightPlanData().AmendFlightPlan();
					remarks = stringToAdd;
				}
			}
		}
	}
}

string CDM::EobtPlusTime(string EOBT, int addedTime) {
	long int timeNow = static_cast<long int>(std::time(nullptr));
	string completeTime = unixTimeToHumanReadable(timeNow);
	string hour = "";
	string min = "";

	hour = completeTime.substr(completeTime.find(":") - 2, 2);

	if (completeTime.substr(completeTime.find(":") + 3, 1) == ":") {
		min = completeTime.substr(completeTime.find(":") + 1, 2);
	}
	else {
		min = completeTime.substr(completeTime.find(":") + 1, 1);
	}

	if (stoi(min) < 10) {
		min = "0" + min;
	}
	if (stoi(hour) < 10) {
		hour = "0" + hour.substr(1, 1);
	}

	return calculateTime(hour + min + "00", addedTime);
}

string CDM::getTaxiTime(double lat, double lon, string origin, string depRwy) {
	double x1, y1, x2, y2, x3, y3, x4, y4;
	string line, TxtOrigin, TxtDepRwy, TxtTime;
	vector<int> separators;
	bool ZoneFound = false;
	CPosition Pos;

	try
	{
		for (int t = 0; t < TxtTimesVector.size(); t++)
		{
			line = TxtTimesVector[t];
			if (!separators.empty()) {
				separators.clear();
			}
			for (int g = 0; g < TxtTimesVector[t].length(); g++)
			{
				if (line.substr(g, 1) == ":") {
					separators.push_back(g);
				}
			}


			TxtOrigin = line.substr(0, separators[0]);
			if (TxtOrigin == origin) {
				TxtDepRwy = line.substr(separators[0] + 1, separators[1] - separators[0] - 1);
				if (TxtDepRwy == depRwy) {
					if (Pos.LoadFromStrings(line.substr(separators[2] + 1, separators[3] - separators[2] - 1).c_str(), line.substr(separators[1] + 1, separators[2] - separators[1] - 1).c_str()))
					{
						x1 = Pos.m_Latitude;
						y1 = Pos.m_Longitude;
					}
					else
					{
						x1 = stod(line.substr(separators[1] + 1, separators[2] - separators[1] - 1));
						y1 = stod(line.substr(separators[2] + 1, separators[3] - separators[2] - 1));
					}

					if (Pos.LoadFromStrings(line.substr(separators[4] + 1, separators[5] - separators[4] - 1).c_str(), line.substr(separators[3] + 1, separators[4] - separators[3] - 1).c_str()))
					{
						x2 = Pos.m_Latitude;
						y2 = Pos.m_Longitude;
					}
					else
					{
						x2 = stod(line.substr(separators[3] + 1, separators[4] - separators[3] - 1));
						y2 = stod(line.substr(separators[4] + 1, separators[5] - separators[4] - 1));
					}

					if (Pos.LoadFromStrings(line.substr(separators[6] + 1, separators[7] - separators[6] - 1).c_str(), line.substr(separators[5] + 1, separators[6] - separators[5] - 1).c_str()))
					{
						x3 = Pos.m_Latitude;
						y3 = Pos.m_Longitude;
					}
					else
					{
						x3 = stod(line.substr(separators[5] + 1, separators[6] - separators[5] - 1));
						y3 = stod(line.substr(separators[6] + 1, separators[7] - separators[6] - 1));
					}

					if (Pos.LoadFromStrings(line.substr(separators[8] + 1, separators[9] - separators[8] - 1).c_str(), line.substr(separators[7] + 1, separators[8] - separators[7] - 1).c_str()))
					{
						x4 = Pos.m_Latitude;
						y4 = Pos.m_Longitude;
					}
					else
					{
						x4 = stod(line.substr(separators[7] + 1, separators[8] - separators[7] - 1));
						y4 = stod(line.substr(separators[8] + 1, separators[9] - separators[8] - 1));
					}

					if (FindPoint(x1, y1, x2, y2, x3, y3, x4, y4, lat, lon)) {
						TxtTime = line.substr(separators[9] + 1, line.length() - separators[9] - 1);
						return TxtTime;
						ZoneFound = true;
					}
				}
			}
		}
	}
	catch (std::runtime_error const& e)
	{
		DisplayUserMessage(MY_PLUGIN_NAME, "Error", e.what(), true, true, false, true, false);
		DisplayUserMessage(MY_PLUGIN_NAME, "Error", line.c_str(), true, true, false, true, false);
		return "15";
	}
	catch (...)
	{
		DisplayUserMessage(MY_PLUGIN_NAME, "Error", std::to_string(GetLastError()).c_str(), true, true, false, true, false);
		DisplayUserMessage(MY_PLUGIN_NAME, "Error", line.c_str(), true, true, false, true, false);
		return "15";
	}

	if (!ZoneFound) {
		return "15";
	}

	separators.clear();
}

bool CDM::FindPoint(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4, double pointx, double pointy) {
	double myX[] = { x1,x2,x3,x4 };
	double myY[] = { y1,y2,y3,y4 };

	int final = inPoly(4, myX, myY, pointx, pointy);

	if (final % 2 != 0) {
		return true;
	}
	return false;
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


bool CDM::expiredCtot(string line) {
	if (line.substr(line.find(",") + 1, 6) != "999999") {
		return false;
	}

	//Get Time now
	long int timeNow = static_cast<long int>(std::time(nullptr));
	string completeTime = unixTimeToHumanReadable(timeNow);
	string hour = "";
	string min = "";

	hour = completeTime.substr(completeTime.find(":") - 2, 2);

	if (completeTime.substr(completeTime.find(":") + 3, 1) == ":") {
		min = completeTime.substr(completeTime.find(":") + 1, 2);
	}
	else {
		min = completeTime.substr(completeTime.find(":") + 1, 1);
	}

	if (stoi(min) < 10) {
		min = "0" + min;
	}
	if (stoi(hour) < 10) {
		hour = "0" + hour.substr(1, 1);
	}

	bool oldCTOT = true;
	string CTOTHour = line.substr(line.length() - 8, 2);
	string CTOTMin = line.substr(line.length() - 6, 2);
	int difTime = GetdifferenceTime(hour, min, CTOTHour, CTOTMin);
	if (hour != CTOTHour) {
		if (difTime <= -expiredCTOTTime - 45) {
			oldCTOT = false;
		}
	}
	else {
		if (difTime <= -expiredCTOTTime) {
			oldCTOT = false;
		}
	}

	if (oldCTOT) {
		return true;
	}
	else {
		return false;
	}
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
		if (foundCallsign.substr(1, foundCallsign.length()-3) == callsign) {
			std::string myCid = fastWriter.write(pilots[i]["cid"]);
			return myCid;
		}
	}
	return "0";
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

	if (readBuffer != MY_PLUGIN_VERSION) {
		string DisplayMsg = "Please UPDATE YOUR CDM PLUGIN, version " + readBuffer + " is OUT! You have version " + MY_PLUGIN_VERSION " installed, install it in puigcloud.me/CDM";
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

string CDM::unixTimeToHumanReadable(long int seconds)
{

	// Save the time in Human
	// readable format
	string ans = "";

	// Number of days in month
	// in normal year
	int daysOfMonth[] = { 31, 28, 31, 30, 31, 30,
						  31, 31, 30, 31, 30, 31 };

	long int currYear, daysTillNow, extraTime,
		extraDays, index, date, month, hours,
		minutes, secondss, flag = 0;

	// Calculate total days unix time T
	daysTillNow = seconds / (24 * 60 * 60);
	extraTime = seconds % (24 * 60 * 60);
	currYear = 1970;

	// Calculating current year
	while (daysTillNow >= 365) {
		if (currYear % 400 == 0
			|| (currYear % 4 == 0
				&& currYear % 100 != 0)) {
			daysTillNow -= 366;
		}
		else {
			daysTillNow -= 365;
		}
		currYear += 1;
	}

	// Updating extradays because it
	// will give days till previous day
	// and we have include current day
	extraDays = daysTillNow + 1;

	if (currYear % 400 == 0
		|| (currYear % 4 == 0
			&& currYear % 100 != 0))
		flag = 1;

	// Calculating MONTH and DATE
	month = 0, index = 0;
	if (flag == 1) {
		while (true) {

			if (index == 1) {
				if (extraDays - 29 < 0)
					break;
				month += 1;
				extraDays -= 29;
			}
			else {
				if (extraDays
					- daysOfMonth[index]
					< 0) {
					break;
				}
				month += 1;
				extraDays -= daysOfMonth[index];
			}
			index += 1;
		}
	}
	else {
		while (true) {

			if (extraDays
				- daysOfMonth[index]
				< 0) {
				break;
			}
			month += 1;
			extraDays -= daysOfMonth[index];
			index += 1;
		}
	}

	// Current Month
	if (extraDays > 0) {
		month += 1;
		date = extraDays;
	}
	else {
		if (month == 2 && flag == 1)
			date = 29;
		else {
			date = daysOfMonth[month - 1];
		}
	}

	// Calculating HH:MM:YYYY
	hours = extraTime / 3600;
	minutes = (extraTime % 3600) / 60;
	secondss = (extraTime % 3600) % 60;

	ans += to_string(date);
	ans += "/";
	ans += to_string(month);
	ans += "/";
	ans += to_string(currYear);
	ans += " ";
	ans += to_string(hours);
	ans += ":";
	ans += to_string(minutes);
	ans += ":";
	ans += to_string(secondss);

	// Return the time
	return ans;
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
	//Get Time now
	long int timeNow = static_cast<long int>(std::time(nullptr));
	string completeTime = unixTimeToHumanReadable(timeNow);
	string hour = "";
	string min = "";

	hour = completeTime.substr(completeTime.find(":") - 2, 2);

	if (completeTime.substr(completeTime.find(":") + 3, 1) == ":") {
		min = completeTime.substr(completeTime.find(":") + 1, 2);
	}
	else {
		min = completeTime.substr(completeTime.find(":") + 1, 1);
	}

	if (stoi(min) < 10) {
		min = "0" + min;
	}
	if (stoi(hour) < 10) {
		hour = "0" + hour.substr(1, 1);
	}
	bool found = false;
	for (int i = 0; i < slotList.size(); i++)
	{
		if (slotList[i].substr(0, slotList[i].find(",")) == lineValue.substr(0, lineValue.find(","))) {
			bool oldCTOT = true;
			string CTOTHour = slotList[i].substr(slotList[i].length() - 8, 2);
			string CTOTMin = slotList[i].substr(slotList[i].length() - 6, 2);
			int difTime = GetdifferenceTime(hour, min, CTOTHour, CTOTMin);
			if (hour != CTOTHour) {
				if (difTime <= -expiredCTOTTime - 45) {
					oldCTOT = false;
				}
			}
			else {
				if (difTime <= -expiredCTOTTime) {
					oldCTOT = false;
				}
			}
			if (!oldCTOT) {
				slotList[i] = lineValue.substr(0, lineValue.find(",")) + ",999999,999999," + lineValue.substr(lineValue.find(",") + 1, 4) + "00,c";
				found = true;
				bool ctotFound = false;
				for (int i = 0; i < ctotList.size(); i++)
				{
					if (ctotList[i].substr(0, ctotList[i].find(",")) == lineValue.substr(0, lineValue.find(","))) {
						ctotList.erase(ctotList.begin() + i);
					}
				}
				ctotList.push_back(lineValue);
			}
		}
	}
	if (!found) {
		bool oldCTOT = true;
		string CTOTHour = lineValue.substr(lineValue.length() - 4, 2);
		string CTOTMin = lineValue.substr(lineValue.length() - 2, 2);
		int difTime = GetdifferenceTime(hour, min, CTOTHour, CTOTMin);
		if (hour != CTOTHour) {
			if (difTime <= -expiredCTOTTime - 45) {
				oldCTOT = false;
			}
		}
		else {
			if (difTime <= -expiredCTOTTime) {
				oldCTOT = false;
			}
		}
		if (!oldCTOT) {
			slotList.push_back(lineValue.substr(0, lineValue.find(",")) + ",999999,999999," + lineValue.substr(lineValue.find(",") + 1, 4) + "00,c");
			for (int i = 0; i < ctotList.size(); i++)
			{
				if (ctotList[i].substr(0, ctotList[i].find(",")) == lineValue.substr(0, lineValue.find(","))) {
					ctotList.erase(ctotList.begin() + i);
				}
			}
			ctotList.push_back(lineValue);
		}
	}
}

bool CDM::OnCompileCommand(const char* sCommandLine) {
	if (startsWith(".cdm reload", sCommandLine))
	{
		sendMessage("Reloading CDM....");
		slotList.clear();
		tsacList.clear();
		asatList.clear();
		asrtList.clear();
		taxiTimesList.clear();
		TxtTimesVector.clear();
		OutOfTsat.clear();
		listA.clear();
		ctotList.clear();
		//Get data from xml config file
		rateString = getFromXml("/CDM/rate/@ops");

		//Get data from .txt file
		fstream file;
		string lineValue;
		file.open(lfad.c_str(), std::ios::in);
		if (taxiZonesUrl.length() <= 1) {
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
			getTaxiZonesFromUrl(taxiZonesUrl);
		}
		sendMessage("Done");
		return true;
	}

	if (startsWith(".cdm refreshtime", sCommandLine)) {
		string line = sCommandLine;
		if (line.substr(line.length() - 3, 1) == " ") {
			refreshTime = stoi(line.substr(line.length() - 2));
			sendMessage("Refresh Time se to: " + to_string(refreshTime));
		}
		else if (line.substr(line.length() - 2, 1) == " ") {
			refreshTime = stoi(line.substr(line.length() - 1));
			sendMessage("Refresh Time se to: " + to_string(refreshTime));
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

		for (string line : slotList)
		{
			outfile << line << std::endl;
		}

		outfile.close();
		sendMessage("Done");
		return true;
	}

	if (startsWith(".cdm delay", sCommandLine))
	{
		//Get Time NOW
		long int timeNow = static_cast<long int>(std::time(nullptr));
		string completeTime = unixTimeToHumanReadable(timeNow);
		string hour = "";
		string min = "";

		hour = completeTime.substr(completeTime.find(":") - 2, 2);

		if (completeTime.substr(completeTime.find(":") + 3, 1) == ":") {
			min = completeTime.substr(completeTime.find(":") + 1, 2);
		}
		else {
			min = completeTime.substr(completeTime.find(":") + 1, 1);
		}

		if (stoi(min) < 10) {
			min = "0" + min;
		}
		if (stoi(hour) < 10) {
			hour = "0" + hour.substr(1, 1);
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
		slotList.clear();
		//load data from file
		fstream file;
		string lineValue;
		file.open(sfad.c_str(), std::ios::in);
		while (getline(file, lineValue))
		{
			slotList.push_back(lineValue);
		}
		sendMessage("Done");
		return true;
	}

	if (startsWith(".cdm ctot", sCommandLine))
	{
		sendMessage("Loading CTOTs data....");
		ctotList.clear();
		CTOTcheck.clear();
		//load data from file
		fstream file;
		string lineValue;
		file.open(cfad.c_str(), std::ios::in);
		while (getline(file, lineValue))
		{
			addCtotToMainList(lineValue);
		}
		sendMessage("Done");
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
