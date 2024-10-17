#pragma once
#include <iostream>
#include <string>
#include <vector>

class ServerRestricted {
public:
	string callsign;
	string ctot;
	string reason;

	ServerRestricted(string myCallsign, string myCtot, string myReason) :
		callsign(myCallsign), ctot(myCtot), reason(myReason) { }
	ServerRestricted() = default;
};