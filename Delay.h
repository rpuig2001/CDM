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

	Delay(string myAirport, string myRwy, string myTime) :
		airport(myAirport), rwy(myRwy), time(myTime) { }
	Delay() {};
};