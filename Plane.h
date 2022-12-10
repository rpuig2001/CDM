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
	int hasRestriction;
	Flow flowRestriction;
	bool hasCdt;

	Plane(string mycallsign, string myeobt, string mytsat, string myttot, bool myhasCtot, string myctot, int myhasRestriction, Flow myFlowRestriction, bool myHasCdt) :
		callsign(mycallsign), eobt(myeobt), tsat(mytsat), ttot(myttot), hasCtot(myhasCtot), ctot(myctot), hasRestriction(myhasRestriction), flowRestriction(myFlowRestriction), hasCdt(myHasCdt) { }
	Plane() {};
};
