#pragma once
#include <iostream>
#include <string>
using namespace std;

class Flow {
public:
	string type;
	string time;
	string depa;
	string dest;
	string validDate;
	string validTime;
	string customMessage;

	Flow(string mytype, string mytime, string mydepa, string mydest, string myvalidDate, string myvalidTime, string mymessage) :
		type(mytype), time(mytime), depa(mydepa), dest(mydest), validDate(myvalidDate), validTime(myvalidTime), customMessage(mymessage) { }
	Flow() : type() { }
};