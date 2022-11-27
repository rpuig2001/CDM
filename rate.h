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
	int rate;
	int rateLvo;

	Rate(string myairport, vector<string> myarrRwyYes, vector<string> myarrRwyNo, vector<string> mydepRwyYes, vector<string> mydepRwyNo, int myrate, int myrateLvo) :
		airport(myairport), arrRwyYes(myarrRwyYes), arrRwyNo(myarrRwyNo), depRwyYes(mydepRwyYes), depRwyNo(mydepRwyNo), rate(myrate), rateLvo(myrateLvo) { }
};
