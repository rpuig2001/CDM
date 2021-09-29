#include "stdafx.h"
#include "analyzeFP.hpp"
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
string airport;
string rateString;

vector<string> AircraftIgnore;
vector<string> slotList;
vector<string> tsacList;
vector<string> asrtList;
vector<string> taxiTimesList;
vector<string> TxtTimesVector;
vector<string> OutOfTsat;
vector<string> listA;

using namespace std;
using namespace EuroScopePlugIn;
using namespace pugi;

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

	// Register Tag Item "CDM-TSAT"
	RegisterTagItemType("TSAT", TAG_ITEM_TSAT);

	// Register Tag Item "CDM-TTOT"
	RegisterTagItemType("TTOT", TAG_ITEM_TTOT);

	// Register Tag Item "CDM-TSAC"
	RegisterTagItemType("TSAC", TAG_ITEM_TSAC);
	RegisterTagItemFunction("Add actual TSAT to TSAC", TAG_FUNC_ADDTSAC);
	RegisterTagItemFunction("Edit TSAC", TAG_FUNC_EDITTSAC);

	// Register Tag Item "CDM-ASRT"
	RegisterTagItemType("ASRT", TAG_ITEM_ASRT);

	// Register Tag Item "CDM-A"
	RegisterTagItemType("A", TAG_ITEM_A);
	RegisterTagItemFunction("Add to column A", TAG_FUNC_ADDA);

	// Register Tag Item "CDM-E"
	RegisterTagItemType("E", TAG_ITEM_E);

	GetModuleFileNameA(HINSTANCE(&__ImageBase), DllPathFile, sizeof(DllPathFile));
	pfad = DllPathFile;
	pfad.resize(pfad.size() - strlen("CDM.dll"));
	pfad += "CDMconfig.xml";

	lfad = DllPathFile;
	lfad.resize(lfad.size() - strlen("CDM.dll"));
	lfad += "taxizones.txt";

	debugMode = false;
	initialSidLoad = false;

	//Get data from xml config file
	airport = getAirportFromXml();
	rateString = getRateFromXml();

	//Get data from .txt file
	fstream file;
	string lineValue;
	file.open(lfad.c_str(), std::ios::in);
	while (getline(file, lineValue))
	{
		TxtTimesVector.push_back(lineValue);
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

	if (FunctionId == TAG_FUNC_EDITEOBT)
	{
		OpenPopupEdit(Area, TAG_FUNC_NEWEOBT, fp.GetFlightPlanData().GetEstimatedDepartureTime());
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
			if (editedTSAC.length() <= 4) {
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

	if (FunctionId == TAG_FUNC_ADDA) {
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

	if (FunctionId == TAG_FUNC_ON_OFF) {
		if (find(AircraftIgnore.begin(), AircraftIgnore.end(), fp.GetCallsign()) != AircraftIgnore.end())
			AircraftIgnore.erase(remove(AircraftIgnore.begin(), AircraftIgnore.end(), fp.GetCallsign()), AircraftIgnore.end());
		else
			AircraftIgnore.emplace_back(fp.GetCallsign());

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

	string depRwy = FlightPlan.GetFlightPlanData().GetDepartureRwy(); boost::to_upper(depRwy);
	bool isVfr = false;
	if (strcmp(FlightPlan.GetFlightPlanData().GetPlanType(), "V") > -1) {
		isVfr = true;
	}


	if (origin == airport && !isVfr) {

		const char* EOBT = "";
		const char* TSAT = "";
		const char* TTOT = "";
		int taxiTime = 15;

		//If aircraft is in aircraftFind Base vector
		int pos;
		bool aircraftFind = false;
		for (int i = 0; i < slotList.size(); i++) {
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

		//If column A set
		bool hasValueInA = false;
		for (int i = 0; i < listA.size(); i++)
		{
			if (listA[i] == (string)FlightPlan.GetCallsign()) {
				hasValueInA = true;
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
			//Show basic lists with no info, only EOBT, TOBT, ASRT and *TSAC*
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

			int EOBTdifTime = GetdifferenceTime(hour, min, EOBThour, EOBTmin);

			if (hour != EOBThour) {
				if (EOBTdifTime >= -45 && EOBTdifTime <= 45) {
					actualTOBT = true;
				}
			}
			else {
				if (EOBTdifTime >= -5 && EOBTdifTime <= 5) {
					actualTOBT = true;
				}
			}

			//ASRT
			bool ASRTFound = false;
			int ASRTpos = 0;
			bool correctState = false;
			string ASRTtext = " ";
			for (int x = 0; x < asrtList.size(); x++)
			{
				string actualListCallsign = asrtList[x].substr(0, asrtList[x].find(","));
				if (actualListCallsign == callsign) {
					ASRTFound = true;
					ASRTpos = x;
				}
			}

			if ((string)FlightPlan.GetGroundState() == "STUP" || (string)FlightPlan.GetGroundState() == "ST-UP" || (string)FlightPlan.GetGroundState() == "PUSH" || (string)FlightPlan.GetGroundState() == "TAXI" || (string)FlightPlan.GetGroundState() == "DEPA") {
				correctState = true;
			}

			if (!ASRTFound) {
				if (correctState) {
					ASRTtext = hour + min;
					asrtList.push_back(callsign + "," + ASRTtext);
					ASRTFound = true;
				}
			}
			else {
				if (correctState) {
					ASRTtext = asrtList[ASRTpos].substr(asrtList[ASRTpos].length() - 4, 4);
				}
				else if (!correctState) {
					asrtList.erase(asrtList.begin() + ASRTpos);
					ASRTFound = false;
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
				if (!actualTOBT) {
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
				*pColorCode = TAG_COLOR_RGB_DEFINED;
				*pRGB = TAG_GREEN;
				strcpy_s(sItemString, 16, "____");
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
			if (ItemCode == TAG_ITEM_E)
			{
				*pColorCode = TAG_COLOR_RGB_DEFINED;
				*pRGB = TAG_GREEN;
				strcpy_s(sItemString, 16, "I");
			}
		}
		else {

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

			string TSATfinal = "";
			string TTOTFinal = "";

			if (aircraftFind) {
				string tempEOBT = EOBT;
				if (tempEOBT != slotList[pos].substr(slotList[pos].find(",") + 1, 6)) {
					//aircraftFind false to recalculate Times due to fp change
					slotList.erase(slotList.begin() + pos);
					aircraftFind = false;
				}
			}

			TSAT = EOBT;
			//TSAT
			string TSATstring = TSAT;
			TSATfinal = formatTime(TSATstring);
			TSAT = TSATfinal.c_str();

			//TTOT
			TTOTFinal = calculateTime(TSATstring, taxiTime);
			TTOT = TTOTFinal.c_str();

			int rate = stoi(rateString);
			double rateHour = (double)60 / rate;

			bool equalTTOT = true;
			bool correctTTOT = true;
			bool equalTempoTTOT = true;
			bool alreadySetTOStd = false;

			if (aircraftFind) {
				slotList.erase(slotList.begin() + pos);
			}

			while (equalTTOT) {
				correctTTOT = true;
				for (int t = 0; t < slotList.size(); t++)
				{
					string listTTOT = slotList[t].substr(slotList[t].length() - 6, 6);
					string listCallsign = slotList[t].substr(0, slotList[t].find(","));

					if (TTOTFinal == listTTOT && callsign != listCallsign) {
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
					else if ((stoi(TTOTFinal) < stoi(calculateTime(listTTOT, rateHour))) && (stoi(TTOTFinal) > stoi(calculateLessTime(listTTOT, rateHour)))) {
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
					equalTTOT = false;
					TSATfinal = calculateLessTime(TTOTFinal, taxiTime);
					TSAT = TSATfinal.c_str();
					TTOT = TTOTFinal.c_str();
					string valueToAdd = callsign + "," + EOBT + "," + TSAT + "," + TTOT;
					slotList.push_back(valueToAdd);
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
			if ((string)FlightPlan.GetGroundState() == "STUP" ||(string)FlightPlan.GetGroundState() == "ST-UP" || (string)FlightPlan.GetGroundState() == "PUSH" || (string)FlightPlan.GetGroundState() == "TAXI" || (string)FlightPlan.GetGroundState() == "DEPA") {
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
				if (EOBTdifTime >= -45 && EOBTdifTime <= 45) {
					actualTOBT = true;
				}
			}
			else {
				if (EOBTdifTime >= -5 && EOBTdifTime <= 5) {
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
			int ASRTpos = 0;
			string ASRTtext = " ";
			for (int x = 0; x < asrtList.size(); x++)
			{
				string actualListCallsign = asrtList[x].substr(0, asrtList[x].find(","));
				if (actualListCallsign == callsign) {
					ASRTFound = true;
					ASRTpos = x;
				}
			}

			if (!ASRTFound) {
				if (correctState) {
					ASRTtext = hour + min;
					asrtList.push_back(callsign + "," + ASRTtext);
					ASRTFound = true;
				}
			}
			else {
				if (correctState) {
					ASRTtext = asrtList[ASRTpos].substr(asrtList[ASRTpos].length() - 4, 4);
				}
				else if (!correctState) {
					asrtList.erase(asrtList.begin() + ASRTpos);
					ASRTFound = false;
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
				if (notYetEOBT) {
					*pColorCode = TAG_COLOR_RGB_DEFINED;
					*pRGB = TAG_RED;
					strcpy_s(sItemString, 16, " ");
				}
				else if (lastMinute) {
					string ShowTSAT = (string)TSAT;
					*pColorCode = TAG_COLOR_RGB_DEFINED;
					*pRGB = TAG_YELLOW;
					strcpy_s(sItemString, 16, ShowTSAT.substr(0, ShowTSAT.length() - 2).c_str());
				}
				else if (moreLessFive) {
					string ShowTSAT = (string)TSAT;
					*pColorCode = TAG_COLOR_RGB_DEFINED;
					*pRGB = TAG_GREEN;
					strcpy_s(sItemString, 16, ShowTSAT.substr(0, ShowTSAT.length() - 2).c_str());
				}
				else {
					string ShowTSAT = (string)TSAT;
					*pColorCode = TAG_COLOR_RGB_DEFINED;
					*pRGB = TAG_GREENNOTACTIVE;
					strcpy_s(sItemString, 16, ShowTSAT.substr(0, ShowTSAT.length() - 2).c_str());
				}
			}

			if (ItemCode == TAG_ITEM_TTOT)
			{
				if (notYetEOBT) {
					*pColorCode = TAG_COLOR_RGB_DEFINED;
					*pRGB = TAG_GREEN;
					strcpy_s(sItemString, 16, " ");
				}
				else if (moreLessFive || lastMinute) {
					string ShowTTOT = (string)TTOT;
					*pColorCode = TAG_COLOR_RGB_DEFINED;
					*pRGB = TAG_GREEN;
					strcpy_s(sItemString, 16, ShowTTOT.substr(0, ShowTTOT.length() - 2).c_str());
				}
				else {
					string ShowTTOT = (string)TTOT;
					*pColorCode = TAG_COLOR_RGB_DEFINED;
					*pRGB = TAG_GREEN;
					strcpy_s(sItemString, 16, ShowTTOT.substr(0, ShowTTOT.length() - 2).c_str());
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
		}
	}
}

string CDM::getTaxiTime(double lat, double lon, string origin, string depRwy) {
	string line, bot_left_lat, bot_left_lon, top_right_lat, top_right_lon, TxtOrigin, TxtDepRwy, TxtTime;
	vector<int> separators;
	bool ZoneFound = false;

	for (int t = 0; t < TxtTimesVector.size(); t++)
	{
		for (int g = 0; g < TxtTimesVector[t].length(); g++)
		{
			if (TxtTimesVector[t].substr(g, 1) == ":") {
				separators.push_back(g);
			}
		}
		line = TxtTimesVector[t];
		TxtOrigin = line.substr(0, 4);
		TxtDepRwy = line.substr(separators[0] + 1, separators[1] - separators[0] - 1);
		bot_left_lat = line.substr(separators[1] + 1, separators[2] - separators[1] - 1);
		bot_left_lon = line.substr(separators[2] + 1, separators[3] - separators[2] - 1);
		top_right_lat = line.substr(separators[3] + 1, separators[4] - separators[3] - 1);
		top_right_lon = line.substr(separators[4] + 1, separators[5] - separators[4] - 1);
		TxtTime = line.substr(separators[5] + 1, line.length() - 1);
		if (TxtOrigin == origin) {
			if (TxtDepRwy == depRwy) {
				if (FindPoint(stod(bot_left_lat), stod(bot_left_lon), stod(top_right_lat), stod(top_right_lon), lat, lon) == true) {
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

bool CDM::FindPoint(double x1, double y1, double x2, double y2, double x, double y) {

	if (x > x1 && x < x2 && y > y1 && y < y2) {
		return true;
	}
	else {
		return false;
	}
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

//Get Airport from the xml file
string CDM::getAirportFromXml()
{
	xml_document doc;

	// load the XML file
	doc.load_file(pfad.c_str());

	string xpath = "/CDM/apt/@icao";
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

//Get Rate/Hour from the xml file
string CDM::getRateFromXml()
{
	xml_document doc;

	// load the XML file
	doc.load_file(pfad.c_str());

	string xpath = "/CDM/rate/@ops";
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


bool CDM::OnCompileCommand(const char* sCommandLine) {
	if (startsWith(".cdm reload", sCommandLine))
	{
		sendMessage("Reloading CDM....");
		slotList.clear();
		tsacList.clear();
		asrtList.clear();
		taxiTimesList.clear();
		TxtTimesVector.clear();
		OutOfTsat.clear();
		listA.clear();
		//Get data from xml config file
		airport = getAirportFromXml();
		rateString = getRateFromXml();

		//Get data from .txt file
		fstream file;
		string lineValue;
		file.open(lfad.c_str(), std::ios::in);
		while (getline(file, lineValue))
		{
			TxtTimesVector.push_back(lineValue);
		}
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