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
	bool hasCtot;

	Plane(string mycallsign, string myeobt, string mytsat, string myttot, bool myhasCtot) : callsign(mycallsign), eobt(myeobt), tsat(mytsat), ttot(myttot), hasCtot(myhasCtot) { }
};
