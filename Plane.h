#pragma once
#include <iostream>
#include <string>
#include "EcfmpRestriction.h"
using namespace std;

class Plane {
public:
	string callsign;
	string eobt;
	string tsat;
	string ttot;
	string ctot;
	string flowReason;
	EcfmpRestriction ecfmpRestriction;
	bool hasEcfmpRestriction;
	bool hasManualCtot;
	bool showData;

	Plane(string mycallsign, string myeobt, string mytsat, string myttot, string myctot, string myFlowReason, EcfmpRestriction myEcfmpRestriction, bool myHasEcfmpRestriction, bool myHasManualCtot, bool myShowData) :
		callsign(mycallsign), eobt(myeobt), tsat(mytsat), ttot(myttot), ctot(myctot), flowReason(myFlowReason), ecfmpRestriction(myEcfmpRestriction), hasEcfmpRestriction(myHasEcfmpRestriction), hasManualCtot(myHasManualCtot), showData(myShowData) { }
	Plane() {};
};
