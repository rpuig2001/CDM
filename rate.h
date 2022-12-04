#pragma once
#include <iostream>
#include <string>
#include "Flow.h"
using namespace std;

class Rate {
public:
	string airport;
	vector<string> arrRwyYes;
	vector<string> arrRwyNo;
	vector<string> depRwyYes;
	vector<string> depRwyNo;
	vector<string> dependentRwy;
	vector<string> rates;
	vector<string> ratesLvo;

	Rate(string myairport, vector<string> myarrRwyYes, vector<string> myarrRwyNo, vector<string> mydepRwyYes, vector<string> mydepRwyNo, vector<string> mydependentRwy, vector<string> myrates, vector<string> myratesLvo) :
		airport(myairport), arrRwyYes(myarrRwyYes), arrRwyNo(myarrRwyNo), depRwyYes(mydepRwyYes), depRwyNo(mydepRwyNo), dependentRwy(mydependentRwy), rates(myrates), ratesLvo(myratesLvo) { }
	Rate(string myairport) : airport(myairport) {}
};
