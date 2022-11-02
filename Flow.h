#pragma once
#include <iostream>
#include <string>
#include <vector>
using namespace std;

class Flow {
public:
	int id;
	string ident;
	int event_id;
	string reason;
	string valid_time;
	string valid_date;
	string type;
	double value;
	vector<string> ADEP;
	vector<string> ADES;

	Flow(int myId, string myIdent, int myEventId, string myReason, string myValid_time, string myValid_date, string myType, double myValue, vector<string> myADEP, vector<string> myADES) :
		id(myId), ident(myIdent), event_id(myEventId), reason(myReason), valid_time(myValid_time), valid_date(myValid_date), type(myType), value(myValue) , ADEP(myADEP), ADES(myADES) { }
	Flow() {};
};