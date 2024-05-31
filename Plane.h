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
	int hasRestriction;
	string flowReason;
	bool hasManualCtot;

	Plane(string mycallsign, string myeobt, string mytsat, string myttot, string myctot, int myhasRestriction, string myFlowReason, bool myHasManualCtot) :
		callsign(mycallsign), eobt(myeobt), tsat(mytsat), ttot(myttot), ctot(myctot), hasRestriction(myhasRestriction), flowReason(myFlowReason), hasManualCtot(myHasManualCtot) { }
	Plane() {};
};
