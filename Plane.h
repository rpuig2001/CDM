#pragma once
#include <iostream>
#include <string>
#include "Flow.h"
using namespace std;

class Plane {
public:
	string callsign;
	string eobt;
	string tsat;
	string ttot;
	bool hasCtot;
	string ctot;
	bool hasRestriction;
	Flow flowRestriction;

	Plane(string mycallsign, string myeobt, string mytsat, string myttot, bool myhasCtot, string myctot, bool myhasRestriction, Flow myFlowRestriction) :
		callsign(mycallsign), eobt(myeobt), tsat(mytsat), ttot(myttot), hasCtot(myhasCtot), ctot(myctot), hasRestriction(myhasRestriction), flowRestriction(myFlowRestriction) { }
};
