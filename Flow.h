#pragma once
#include <iostream>
#include <string>
#include <vector>
using namespace std;

class Flow {
public:
	string type;
	string time;
	vector<string> depa;
	vector<string> dest;
	string validDate;
	string validTime;
	string customMessage;

	Flow(string mytype, string mytime, vector<string> mydepa, vector<string> mydest, string myvalidDate, string myvalidTime, string mymessage) :
		type(mytype), time(mytime), depa(mydepa), dest(mydest), validDate(myvalidDate), validTime(myvalidTime), customMessage(mymessage) { }
	Flow() : type() { }
};