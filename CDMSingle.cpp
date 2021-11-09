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
//string airport;
string rateString;
int expiredCTOTTime;
bool defaultRate;

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

using namespace std;
using namespace EuroScopePlugIn;
using namespace pugi;

COLORREF TAG_GREEN = RGB(0, 192, 0);
COLORREF TAG_GREENNOTACTIVE = RGB(143, 216, 148);
COLORREF TAG_GREY = RGB(182, 182, 182);
COLORREF TAG_ORANGE = RGB(212, 133, 46);
COLORREF TAG_YELLOW = RGB(212, 214, 7);
COLORREF TAG_DARKYELLOW = RGB(245, 239, 13);
COLORREF TAG_RED = RGB(190, 0, 0);

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

	//Get data from xml config file
	//airport = getFromXml("/CDM/apt/@icao");
	expiredCTOTTime = stoi(getFromXml("/CDM/expiredCtot/@time"));
	string rateOption = getFromXml("/CDM/defaultRate/@option");
	rateString = getFromXml("/CDM/rate/@ops");
	
	if (rateOption == "0") {
		getRateOpt0();
		defaultRate = false;
	}
	else {
		defaultRate = true;
	}

	//Get data from .txt file
	fstream file;
	string lineValue;
	file.open(lfad.c_str(), std::ios::in);
	while (getline(file, lineValue))
	{
		TxtTimesVector.push_back(lineValue);
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
	while (getline(fileColors, lineValueColors))
	{
		if (lineValueColors.size() > 1) {
			colors.push_back(lineValueColors.substr(7, lineValueColors.length() - 7));
			for (int g = 0; g < colors[colors.size() - 1].length(); g++)
			{
				if (colors[colors.size() - 1].substr(g, 1) == ",") {
					sep.push_back(g);
				}
			}
		}
	}

	TAG_GREEN = RGB(stoi(colors[0].substr(0, sep[0])), stoi(colors[0].substr(sep[0] + 1, sep[1] - (sep[0] + 1))), stoi(colors[0].substr(sep[1] + 1, colors[0].length() - (sep[1] + 1))));
	TAG_GREENNOTACTIVE = RGB(stoi(colors[1].substr(0, sep[2])), stoi(colors[1].substr(sep[2] + 1, sep[3] - (sep[2] + 1))), stoi(colors[1].substr(sep[3] + 1, colors[0].length() - (sep[3] + 1))));
	TAG_GREY = RGB(stoi(colors[2].substr(0, sep[4])), stoi(colors[2].substr(sep[4] + 1, sep[5] - (sep[4] + 1))), stoi(colors[2].substr(sep[5] + 1, colors[2].length() - (sep[5] + 1))));
	TAG_ORANGE = RGB(stoi(colors[3].substr(0, sep[6])), stoi(colors[3].substr(sep[6] + 1, sep[7] - (sep[6] + 1))), stoi(colors[3].substr(sep[7] + 1, colors[3].length() - (sep[7] + 1))));
	TAG_YELLOW = RGB(stoi(colors[4].substr(0, sep[8])), stoi(colors[4].substr(sep[8] + 1, sep[9] - (sep[8] + 1))), stoi(colors[4].substr(sep[9] + 1, colors[4].length() - (sep[9] + 1))));
	TAG_DARKYELLOW = RGB(stoi(colors[5].substr(0, sep[10])), stoi(colors[5].substr(sep[10] + 1, sep[11] - (sep[10] + 1))), stoi(colors[5].substr(sep[11] + 1, colors[5].length() - (sep[11] + 1))));
	TAG_RED = RGB(stoi(colors[6].substr(0, sep[12])), stoi(colors[6].substr(sep[12] + 1, sep[13] - (sep[12] + 1))), stoi(colors[6].substr(sep[13] + 1, colors[6].length() - (sep[13] + 1))));
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
		for (int i = 0; i < tsacList.size(); i++)
		{
			if (tsacList[i].substr(0, tsacList[i].find(",")) == fp.GetCallsign()) {
				tsacList.erase(tsacList.begin() + i);
			}
		}

		for (int a = 0; a < slotList.size(); a++)
		{
			if (slotList[a].substr(0, slotList[a].find(",")) == fp.GetCallsign()) {
				string getTSAT = slotList[a].substr(slotList[a].find(",") + 8, 6);
				string valuesToAdd = (string)fp.GetCallsign() + "," + getTSAT;
				tsacList.push_back(valuesToAdd);
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
		bool hasCTOT = false;
		int ctotPos = 0;
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

			bool gndStatusSet = false;
			if ((string)FlightPlan.GetGroundState() == "STUP" || (string)FlightPlan.GetGroundState() == "ST-UP" || (string)FlightPlan.GetGroundState() == "PUSH" || (string)FlightPlan.GetGroundState() == "TAXI" || (string)FlightPlan.GetGroundState() == "DEPA") {
				gndStatusSet = true;
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
					*pColorCode = TAG_COLOR_RGB_DEFINED;
					*pRGB = TAG_GREY;
					strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
				}
				if (ItemCode == TAG_ITEM_TOBT)
				{
					string ShowEOBT = (string)EOBT;
					*pColorCode = TAG_COLOR_RGB_DEFINED;
					*pRGB = TAG_GREEN;
					strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
				}
				if (ItemCode == TAG_ITEM_TSAC)
				{
					*pColorCode = TAG_COLOR_RGB_DEFINED;
					*pRGB = TAG_GREEN;
					strcpy_s(sItemString, 16, "____");
				}
				if (ItemCode == TAG_ITEM_ASAT)
				{
					if (ASATFound) {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_GREEN;
						strcpy_s(sItemString, 16, ASATtext.c_str());
					}
					else {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_GREEN;
						strcpy_s(sItemString, 16, " ");
					}
				}
				if (ItemCode == TAG_ITEM_E)
				{
					*pColorCode = TAG_COLOR_RGB_DEFINED;
					*pRGB = TAG_GREEN;
					strcpy_s(sItemString, 16, "I");
				}
				if (ItemCode == TAG_ITEM_CTOT)
				{
					if (hasCTOT) {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_GREEN;
						strcpy_s(sItemString, 16, ctotList[ctotPos].substr(ctotList[ctotPos].find(",") + 1, 4).c_str());
					}
				}
			}
			else {

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

				int rate;
				if (defaultRate) {
					rate = stoi(rateString);
				}
				else {
					rate = rateForRunway(origin, depRwy);
					if (rate == -1) {
						rate = stoi(rateString);
					}
				}
				double rateHour = (double)60 / rate;

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
								listAirport = planeAiportList[i].substr(planeAiportList[i].find(",")+1, 4);
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

				//Sync TTOT
				if (remarks.find("%") != string::npos) {
					if (TTOT != remarks.substr(remarks.find("%") + 8, 6)) {
						string stringToAdd = remarks.substr(0, remarks.find("%") - 1);
						FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
						FlightPlan.GetFlightPlanData().AmendFlightPlan();
						remarks = stringToAdd;
					}
				}
				else if(aircraftFind && !stsDepa){
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
					*pColorCode = TAG_COLOR_RGB_DEFINED;
					*pRGB = TAG_GREY;
					strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
				}

				if (ItemCode == TAG_ITEM_TOBT)
				{
					string ShowEOBT = (string)EOBT;
					if (notYetEOBT) {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_GREENNOTACTIVE;
						strcpy_s(sItemString, 16, " ");
					}
					else if (!actualTOBT) {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_GREENNOTACTIVE;
						strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
					}
					else {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_GREEN;
						strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
					}
				}

				if (ItemCode == TAG_ITEM_TSAC)
				{
					if (TSACNotTSAT) {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_ORANGE;
						strcpy_s(sItemString, 16, ThisTSAC.substr(0, ThisTSAC.length() - 2).c_str());
					}
					else if (TSACFound) {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_GREEN;
						strcpy_s(sItemString, 16, ThisTSAC.substr(0, ThisTSAC.length() - 2).c_str());
					}
					else {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_GREEN;
						strcpy_s(sItemString, 16, "____");
					}
				}

				if (ItemCode == TAG_ITEM_TSAT)
				{
					string ShowTSAT = (string)TSAT;
					if (notYetEOBT) {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_RED;
						strcpy_s(sItemString, 16, " ");
					}
					else if (lastMinute) {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_YELLOW;
						strcpy_s(sItemString, 16, ShowTSAT.substr(0, ShowTSAT.length() - 2).c_str());
					}
					else if (moreLessFive) {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_GREEN;
						strcpy_s(sItemString, 16, ShowTSAT.substr(0, ShowTSAT.length() - 2).c_str());
					}
					else if (oldTSAT) {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_GREEN;
						strcpy_s(sItemString, 16, ShowTSAT.substr(0, ShowTSAT.length() - 2).c_str());
					}
					else {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_GREENNOTACTIVE;
						strcpy_s(sItemString, 16, ShowTSAT.substr(0, ShowTSAT.length() - 2).c_str());
					}
				}

				if (ItemCode == TAG_ITEM_TTOT)
				{
					string ShowTTOT = (string)TTOT;
					if (notYetEOBT) {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_GREEN;
						strcpy_s(sItemString, 16, " ");
					}
					else if (moreLessFive || lastMinute) {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_GREEN;
						strcpy_s(sItemString, 16, ShowTTOT.substr(0, ShowTTOT.length() - 2).c_str());
					}
					else {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_GREEN;
						strcpy_s(sItemString, 16, ShowTTOT.substr(0, ShowTTOT.length() - 2).c_str());
					}
				}

				if (ItemCode == TAG_ITEM_ASAT)
				{
					if (ASATFound) {
						if (ASATPlusFiveLessTen) {
							*pColorCode = TAG_COLOR_RGB_DEFINED;
							*pRGB = TAG_YELLOW;
							strcpy_s(sItemString, 16, ASATtext.c_str());
						}
						else {
							*pColorCode = TAG_COLOR_RGB_DEFINED;
							*pRGB = TAG_GREEN;
							strcpy_s(sItemString, 16, ASATtext.c_str());
						}
					}
					else {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_GREEN;
						strcpy_s(sItemString, 16, " ");
					}
				}

				if (ItemCode == TAG_ITEM_ASRT)
				{
					if (ASRTFound) {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_GREEN;
						strcpy_s(sItemString, 16, ASRTtext.c_str());
					}
					else {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_GREEN;
						strcpy_s(sItemString, 16, " ");
					}
				}

				if (ItemCode == TAG_ITEM_A)
				{
					if (hasValueInA) {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_YELLOW;
						strcpy_s(sItemString, 16, "A");
					}
					else {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_GREEN;
						strcpy_s(sItemString, 16, " ");
					}
				}

				if (ItemCode == TAG_ITEM_E)
				{
					if (notYetEOBT) {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_GREEN;
						strcpy_s(sItemString, 16, "P");
					}
					else {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_GREEN;
						strcpy_s(sItemString, 16, "C");
					}
				}

				if (ItemCode == TAG_ITEM_CTOT)
				{
					if (hasCTOT) {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_GREEN;
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
					}else{
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
					*pColorCode = TAG_COLOR_RGB_DEFINED;
					*pRGB = TAG_GREY;
					strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
				}

				if (ItemCode == TAG_ITEM_TOBT)
				{
					string ShowEOBT = (string)EOBT;
					if (notYetEOBT) {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_GREENNOTACTIVE;
						strcpy_s(sItemString, 16, " ");
					}
					else if (!actualTOBT) {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_GREENNOTACTIVE;
						strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
					}
					else {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_GREEN;
						strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
					}
				}

				if (ItemCode == TAG_ITEM_TSAC)
				{
					if (TSACNotTSAT) {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_ORANGE;
						strcpy_s(sItemString, 16, ThisTSAC.substr(0, ThisTSAC.length() - 2).c_str());
					}
					else if (TSACFound) {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_GREEN;
						strcpy_s(sItemString, 16, ThisTSAC.substr(0, ThisTSAC.length() - 2).c_str());
					}
					else {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_GREEN;
						strcpy_s(sItemString, 16, "____");
					}
				}

				if (ItemCode == TAG_ITEM_TSAT)
				{
					if (TSATString.length() > 0) {
						if (notYetEOBT) {
							*pColorCode = TAG_COLOR_RGB_DEFINED;
							*pRGB = TAG_RED;
							strcpy_s(sItemString, 16, " ");
						}
						else if (lastMinute) {
							*pColorCode = TAG_COLOR_RGB_DEFINED;
							*pRGB = TAG_YELLOW;
							strcpy_s(sItemString, 16, TSATString.substr(0, 4).c_str());
						}
						else if (moreLessFive) {
							*pColorCode = TAG_COLOR_RGB_DEFINED;
							*pRGB = TAG_GREEN;
							strcpy_s(sItemString, 16, TSATString.substr(0, 4).c_str());
						}
						else if (oldTSAT) {
							*pColorCode = TAG_COLOR_RGB_DEFINED;
							*pRGB = TAG_GREEN;
							strcpy_s(sItemString, 16, TSATString.substr(0, 4).c_str());
						}
						else {
							*pColorCode = TAG_COLOR_RGB_DEFINED;
							*pRGB = TAG_GREENNOTACTIVE;
							strcpy_s(sItemString, 16, TSATString.substr(0, 4).c_str());
						}
					}
				}

				if (ItemCode == TAG_ITEM_TTOT)
				{
					if (TTOTString.length() > 0) {
						if (notYetEOBT) {
							*pColorCode = TAG_COLOR_RGB_DEFINED;
							*pRGB = TAG_GREEN;
							strcpy_s(sItemString, 16, " ");
						}
						else if (moreLessFive || lastMinute) {
							*pColorCode = TAG_COLOR_RGB_DEFINED;
							*pRGB = TAG_GREEN;
							strcpy_s(sItemString, 16, TTOTString.substr(0, 4).c_str());
						}
						else {
							*pColorCode = TAG_COLOR_RGB_DEFINED;
							*pRGB = TAG_GREEN;
							strcpy_s(sItemString, 16, TTOTString.substr(0, 4).c_str());
						}
					}
				}

				if (ItemCode == TAG_ITEM_ASAT)
				{
					if (ASATFound) {
						if (ASATPlusFiveLessTen) {
							*pColorCode = TAG_COLOR_RGB_DEFINED;
							*pRGB = TAG_YELLOW;
							strcpy_s(sItemString, 16, ASATtext.c_str());
						}
						else {
							*pColorCode = TAG_COLOR_RGB_DEFINED;
							*pRGB = TAG_GREEN;
							strcpy_s(sItemString, 16, ASATtext.c_str());
						}
					}
					else {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_GREEN;
						strcpy_s(sItemString, 16, " ");
					}
				}

				if (ItemCode == TAG_ITEM_ASRT)
				{
					if (ASRTFound) {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_GREEN;
						strcpy_s(sItemString, 16, ASRTtext.c_str());
					}
					else {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_GREEN;
						strcpy_s(sItemString, 16, " ");
					}
				}

				if (ItemCode == TAG_ITEM_A)
				{
					if (hasValueInA) {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_YELLOW;
						strcpy_s(sItemString, 16, "A");
					}
					else {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_GREEN;
						strcpy_s(sItemString, 16, " ");
					}
				}

				if (ItemCode == TAG_ITEM_E)
				{
					if (notYetEOBT) {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_GREEN;
						strcpy_s(sItemString, 16, "P");
					}
					else {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_GREEN;
						strcpy_s(sItemString, 16, "C");
					}
				}

				if (ItemCode == TAG_ITEM_CTOT)
				{
					if (hasCTOT) {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = TAG_GREEN;
						strcpy_s(sItemString, 16, ctotList[ctotPos].substr(ctotList[ctotPos].find(",") + 1, 4).c_str());
					}
				}
			}
			else {
				if (ItemCode == TAG_ITEM_EOBT)
				{
					string ShowEOBT = (string)EOBT;
					*pColorCode = TAG_COLOR_RGB_DEFINED;
					*pRGB = TAG_GREY;
					strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
				}
				if (ItemCode == TAG_ITEM_TSAC)
				{
					*pColorCode = TAG_COLOR_RGB_DEFINED;
					*pRGB = TAG_GREEN;
					strcpy_s(sItemString, 16, "____");
				}
				if (ItemCode == TAG_ITEM_TOBT)
				{
					*pColorCode = TAG_COLOR_RGB_DEFINED;
					*pRGB = TAG_RED;
					strcpy_s(sItemString, 16, "MAST");
				}
				if (ItemCode == TAG_ITEM_TSAT)
				{
					*pColorCode = TAG_COLOR_RGB_DEFINED;
					*pRGB = TAG_RED;
					strcpy_s(sItemString, 16, "MAST");
				}
				if (ItemCode == TAG_ITEM_TTOT)
				{
					*pColorCode = TAG_COLOR_RGB_DEFINED;
					*pRGB = TAG_RED;
					strcpy_s(sItemString, 16, "MAST");
				}
				if (ItemCode == TAG_ITEM_E)
				{
					*pColorCode = TAG_COLOR_RGB_DEFINED;
					*pRGB = TAG_GREEN;
					strcpy_s(sItemString, 16, "I");
				}
			}
		}
	}
}

bool CDM::getRateOpt0() {
	//Get data from rate.txt file
	fstream rateFile;
	string lineValue;
	rateFile.open(rfad.c_str(), std::ios::in);
	while (getline(rateFile, lineValue))
	{
		rate.push_back(lineValue);
	}
	return true;
}

int CDM::rateForRunway(string airport, string depRwy) {
	string lineAirport, lineDepRwy;
	for (string line : rate) {
		if (line.length() > 1) {
			lineAirport = line.substr(0, line.find(":"));
			if (lineAirport == airport) {
				lineDepRwy = line.substr(line.find(":") + 1, line.find("=") - line.find(":") - 1);
				if (lineDepRwy == depRwy) {
					return stoi(line.substr(line.find("=") + 1, line.length() - line.find("=")));
				}
			}
		}
	}
	return -1;
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
			TxtDepRwy = line.substr(separators[0] + 1, separators[1]- separators[0]-1);
			if (TxtDepRwy == depRwy) {
				x1 = stod(line.substr(separators[1] + 1, separators[2] - separators[1] - 1));
				y1 = stod(line.substr(separators[2] + 1, separators[3] - separators[2] - 1));

				x2 = stod(line.substr(separators[3] + 1, separators[4] - separators[3] - 1));
				y2 = stod(line.substr(separators[4] + 1, separators[5] - separators[4] - 1));

				x3 = stod(line.substr(separators[5] + 1, separators[6] - separators[5] - 1));
				y3 = stod(line.substr(separators[6] + 1, separators[7] - separators[6] - 1));

				x4 = stod(line.substr(separators[7] + 1, separators[8] - separators[7] - 1));
				y4 = stod(line.substr(separators[8] + 1, separators[9] - separators[8] - 1));

				if (FindPoint(x1, y1, x2, y2, x3, y3, x4, y4, lat, lon)) {
					TxtTime = line.substr(separators[9] + 1, line.length() - separators[9] - 1);
					return TxtTime;
					ZoneFound = true;
				}
			}
		}
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
		while (getline(file, lineValue))
		{
			TxtTimesVector.push_back(lineValue);
		}
		sendMessage("Done");
		return true;
	}

	if (startsWith(".cdm save", sCommandLine))
	{
		sendMessage("Saving CDM data....");
		//save data to file
		ofstream outfile(sfad.c_str());

		for (string line: slotList)
		{
			outfile << line << std::endl;
		}

		outfile.close();
		sendMessage("Done");
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
	
	if (startsWith(".cdm debug", sCommandLine))
	{
		if(debugMode) {
			sendMessage("Debug Mode Disable....");
			debugMode = false;
		}
		else {
			sendMessage("Debug Mode Enable....");
			debugMode = true;
		return true;
	}

	if (startsWith(".cdm rate", sCommandLine))
	{
		if (defaultRate) {
			string line = sCommandLine;
			rateString = line.substr(line.length() - 2);
			sendMessage("NEW Rate/Hour: " + rateString);
		}
		else {
			sendMessage("This command is only available when using the default rate.");
		}
		return true;
	}

	if (startsWith(".cdm lvo", sCommandLine))
	{
		if (defaultRate) {
			rateString = getFromXml("/CDM/rateLvo/@ops");
			sendMessage("Low Visibility Operations Rate Set: " + rateString);
		}
		else {
			sendMessage("This command is only available when using the default rate.");
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
		}else{
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
