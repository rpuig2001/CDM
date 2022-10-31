#pragma once
#include <iostream>
#include <string>
#include <vector>
using namespace std;

class CAD {
public:
	string airport;
	int rate;

	CAD(string myAirport, int myRate) :
		airport(myAirport), rate(myRate) { }
	CAD() {};
};