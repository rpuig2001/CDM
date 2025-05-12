#pragma once
#include <iostream>
#include <string>
#include <vector>
using namespace std;

class sidInterval {
public:
	string airport;
	string rwy;
	string sid1;
	string sid2;
	double value;

	sidInterval(string myAirport, string myRwy, string mySid1, string mySid2, double myValue) :
		airport(myAirport), rwy(myRwy), sid1(mySid1), sid2(mySid2), value(myValue) { }
	sidInterval() {};
};