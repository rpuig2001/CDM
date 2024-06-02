#pragma once
#include <iostream>
#include <string>
using namespace std;

class Plane {
public:
	string callsign;
	string eobt;
	string tsat;
	string ttot;
	string ctot;
	string flowReason;
	bool hasManualCtot;
	bool showData;

	Plane(string mycallsign, string myeobt, string mytsat, string myttot, string myctot, string myFlowReason, bool myHasManualCtot, bool myShowData) :
		callsign(mycallsign), eobt(myeobt), tsat(mytsat), ttot(myttot), ctot(myctot), flowReason(myFlowReason), hasManualCtot(myHasManualCtot), showData(myShowData) { }
	Plane() {};
};
