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
string airport;
string rateString;

vector<string> AircraftIgnore;
vector<string> slotList;

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
	//RegisterTagItemFunction("Add to CFL", TAG_FUNC_ADDTOCFL);

	// Register Tag Item "CDM-TSAT"
	RegisterTagItemType("TSAT", TAG_ITEM_TSAT);

	// Register Tag Item "CDM-TSAT"
	RegisterTagItemType("TTOT", TAG_ITEM_TTOT);

	// Get Path of the Sid.json
	GetModuleFileNameA(HINSTANCE(&__ImageBase), DllPathFile, sizeof(DllPathFile));
	pfad = DllPathFile;
	pfad.resize(pfad.size() - strlen("CDM.dll"));
	pfad += "CDMconfig.xml";

	debugMode = false;
	initialSidLoad = false;

	//Get data from xml config file
	airport = getAirportFromXml();
	rateString = getRateFromXml();

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

	/*if (FunctionId == TAG_FUNC_ADDTOCFL)
	{

	}*/
	if (FunctionId == TAG_FUNC_ON_OFF) {
		if (find(AircraftIgnore.begin(), AircraftIgnore.end(), fp.GetCallsign()) != AircraftIgnore.end())
			AircraftIgnore.erase(remove(AircraftIgnore.begin(), AircraftIgnore.end(), fp.GetCallsign()), AircraftIgnore.end());
		else
			AircraftIgnore.emplace_back(fp.GetCallsign());

	}
}

void CDM::OnFlightPlanDisconnect(CFlightPlan FlightPlan) {
	AircraftIgnore.erase(remove(AircraftIgnore.begin(), AircraftIgnore.end(), FlightPlan.GetCallsign()), AircraftIgnore.end());
}


void CDM::OnGetTagItem(CFlightPlan FlightPlan, CRadarTarget RadarTarget, int ItemCode, int TagData, char sItemString[16], int* pColorCode, COLORREF* pRGB, double* pFontSize)
{
	string callsign = FlightPlan.GetCallsign();
	string remarks = FlightPlan.GetFlightPlanData().GetRemarks();

	string origin = FlightPlan.GetFlightPlanData().GetOrigin(); boost::to_upper(origin);
	string destination = FlightPlan.GetFlightPlanData().GetDestination(); boost::to_upper(destination);

	string sid = FlightPlan.GetFlightPlanData().GetSidName(); boost::to_upper(sid);
	string depRwy = FlightPlan.GetFlightPlanData().GetDepartureRwy(); boost::to_upper(depRwy);

	string first_wp = sid.substr(0, sid.find_first_of("0123456789"));
	if (0 != first_wp.length())
		boost::to_upper(first_wp);
	string sid_suffix;
	if (first_wp.length() != sid.length()) {
		sid_suffix = sid.substr(sid.find_first_of("0123456789"), sid.length());
		boost::to_upper(sid_suffix);
	}

	const char* EOBT = "";
	const char* TSAT = "";
	const char* TTOT = "";
	string taxiTime;

	if (origin == airport) {

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

		/*remarks.find("&") != std::string::npos && aircraftFind*/
		if (false) {
			string getTSAT = remarks.substr(remarks.find("&") + 1, 6);
			TSAT = getTSAT.c_str();
			string TTOTstring = slotList[pos].substr(slotList[pos].find(",") + 8, slotList[pos].length() - 1);
			TTOT = TTOTstring.c_str();
		} else {
			if (aircraftFind) {
				TSAT = EOBT;
				aircraftFind = false;
				slotList.erase(slotList.begin() + pos);
			}
			else if (remarks.find("&") != std::string::npos) {
				TSAT = EOBT;
				remarks.erase(remarks.begin() + (remarks.length() - 10));
				FlightPlan.GetFlightPlanData().SetRemarks(remarks.c_str());
			}
			else {
				TSAT = EOBT;
			}

			//TSAT
			string TSATstring = TSAT;
			TSATfinal = formatTime(TSATstring);
			TSAT = TSATfinal.c_str();

			//TTOT
			TTOTFinal = calculateTime(TSATstring, 15);
			TTOT = TTOTFinal.c_str();
		}

		int rate = stoi(rateString);
		double rateHour = (double)60 / rate;

		if (!aircraftFind) {

			if (remarks.find("&") != std::string::npos) {
				remarks.erase(remarks.length() - 11, remarks.length() - 1);
				FlightPlan.GetFlightPlanData().SetRemarks(remarks.c_str());
			}
			bool equalTTOT = true;
			bool correctTTOT = true;
			bool equalTempoTTOT = true;
			bool alreadySetTOStd = false;

			while (equalTTOT) {
				correctTTOT = true;
				for (int t = 0; t < slotList.size(); t++)
				{
					string listTTOT = slotList[t].substr(slotList[t].find(",") + 8, slotList[t].length() - 1);
					string listCallsign = slotList[t].substr(0, slotList[t].find(","));

					if (TTOTFinal == listTTOT && callsign != listCallsign) {
						if (alreadySetTOStd) {
							TTOTFinal = calculateTime(TTOTFinal, rateHour);
							correctTTOT = false;
						}
						else {
							TTOTFinal = calculateTime(listTTOT, rateHour);
							correctTTOT = false;
							alreadySetTOStd = true;
						}
					}
					else if ((stoi(TTOTFinal) < stoi(calculateTime(listTTOT, rateHour))) && (stoi(TTOTFinal) > stoi(calculateLessTime(listTTOT, rateHour)))) {
						if (alreadySetTOStd) {
							TTOTFinal = calculateTime(TTOTFinal, rateHour);
							correctTTOT = false;
						}
						else {
							TTOTFinal = calculateTime(listTTOT, rateHour);
							correctTTOT = false;
							alreadySetTOStd = true;
						}
					}
				}

				if (correctTTOT) {
					equalTTOT = false;
					TSATfinal = calculateLessTime(TTOTFinal, 15);
					TSAT = TSATfinal.c_str();
					TTOT = TTOTFinal.c_str();
					string valueToAdd = callsign + "," + EOBT + "," + TTOT;
					slotList.push_back(valueToAdd);
					string stringToAdd = remarks + " TSAT&" + TSAT;
					FlightPlan.GetFlightPlanData().SetRemarks(stringToAdd.c_str());
				}
			}
		}

		//If oldTSAT
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

		string TSAThour = TSATfinal.substr(TSATfinal.length() - 6, 2);
		string TSATmin = TSATfinal.substr(TSATfinal.length() - 4, 2);

		bool oldTSAT = false;
		bool moreLessFive = false;

		//Hour(LOCAL), Min(LOCAL), Hour(PLANE), Min(PLANE)
		int difTime = GetdifferenceTime(hour, min, TSAThour, TSATmin);
		if (difTime > 5) {
			oldTSAT = true;
		}
		else if (difTime >= -5 && difTime <= 5) {
			moreLessFive = true;
		}


		if (ItemCode == TAG_ITEM_EOBT)
		{
			string ShowEOBT = (string)EOBT;
			*pColorCode = TAG_COLOR_RGB_DEFINED;
			*pRGB = TAG_GREY;
			strcpy_s(sItemString, 16, ShowEOBT.substr(0, ShowEOBT.length() - 2).c_str());
		}

		if (ItemCode == TAG_ITEM_TSAT)
		{
			if (oldTSAT) {
				*pColorCode = TAG_COLOR_RGB_DEFINED;
				*pRGB = TAG_RED;
				strcpy_s(sItemString, 16, "OLD");
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
				*pRGB = TAG_GREY;
				strcpy_s(sItemString, 16, ShowTSAT.substr(0, ShowTSAT.length() - 2).c_str());
			}
		}

		if (ItemCode == TAG_ITEM_TTOT)
		{
			if (oldTSAT) {
				*pColorCode = TAG_COLOR_RGB_DEFINED;
				*pRGB = TAG_RED;
				strcpy_s(sItemString, 16, "OLD");
			}
			else if (moreLessFive) {
				string ShowTTOT = (string)TTOT;
				*pColorCode = TAG_COLOR_RGB_DEFINED;
				*pRGB = TAG_GREEN;
				strcpy_s(sItemString, 16, ShowTTOT.substr(0, ShowTTOT.length() - 2).c_str());
			}
			else {
				string ShowTTOT = (string)TTOT;
				*pColorCode = TAG_COLOR_RGB_DEFINED;
				*pRGB = TAG_GREY;
				strcpy_s(sItemString, 16, ShowTTOT.substr(0, ShowTTOT.length() - 2).c_str());
			}
		}
	}
}

string CDM::getTaxiTime(double lat, double lon) {
	//x=lat y=lon
	double x1 = 2.712383, y1 = 39.543504, x2 = 2.719502, y2 = 39.548777;
	if (FindPoint(2.712383, 39.543504, 2.719502, 39.548777, lat, lon)) {
		return "15";
	}
	else {
		return "1";
	}
}

bool CDM::FindPoint(double x1, double y1, double x2, double y2, double x, double y) {

	if (x > x1 and x < x2 and y > y1 and y < y2) {
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

	// Loading proper Sids, when logged in
	if (GetConnectionType() != CONNECTION_TYPE_NO) {
		slotList.clear();
	}
}