#pragma once
#include <iostream>
#include <string>
#include <vector>
using namespace std;

class Delay {
public:
	string airport;
	string rwy;
	string time;
	string type;

	Delay(string myAirport, string myRwy, string myTime, string myType) :
		airport(myAirport), rwy(myRwy), time(myTime), type(myType) { }
	Delay() {};
};