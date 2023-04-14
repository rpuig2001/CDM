#pragma once
#include "stdafx.h"

const int TAG_ITEM_EOBT = 1;
const int TAG_ITEM_TSAT = 2;
const int TAG_ITEM_TTOT = 3;
const int TAG_ITEM_TOBT = 4;
const int TAG_ITEM_TSAC = 5;
const int TAG_ITEM_ASAT = 6;
const int TAG_ITEM_ASRT = 7;
const int TAG_ITEM_E = 9;
const int TAG_ITEM_CTOT = 10;
const int TAG_ITEM_FLOW_MESSAGE = 11;
const int TAG_ITEM_READYSTARTUP = 12;
const int TAG_ITEM_OPTIONS = 13;
const int TAG_ITEM_TSAC_SIMPLE = 14;
const int TAG_ITEM_EV_CTOT = 15;

const int TAG_FUNC_EDITEOBT = 100;
const int TAG_FUNC_NEWEOBT = 101;
const int TAG_FUNC_ON_OFF = 102;
const int TAG_FUNC_ADDTSAC = 103;
const int TAG_FUNC_EDITTSAC = 104;
const int TAG_FUNC_NEWTSAC = 105;
const int TAG_FUNC_READYSTARTUP = 106;
const int TAG_FUNC_TOGGLEASRT = 107;
const int TAG_FUNC_CTOTOPTIONS = 108;
const int TAG_FUNC_REMOVECTOT = 109;
const int TAG_FUNC_ADDCTOTSELECTED = 110;
const int TAG_FUNC_ADDCTOT = 111;
const int TAG_FUNC_READYTOBT = 114;
const int TAG_FUNC_EDITTOBT = 115;
const int TAG_FUNC_NEWTOBT = 116;
const int TAG_FUNC_EOBTTOTOBT = 117;
const int TAG_FUNC_TOGGLEREAMSG = 118;
const int TAG_FUNC_OPT_EOBT = 120;
const int TAG_FUNC_OPT_TOBT = 121;
const int TAG_FUNC_OPT_TSAC = 122;
const int TAG_FUNC_OPT = 123;
const int TAG_FUNC_TOGGLECDT = 124;
const int TAG_FUNC_SETCUSTOMCDT = 125;
const int TAG_FUNC_EDITCDT = 126;
const int TAG_FUNC_FMASTEXT = 127;
const int TAG_FUNC_OPT_TTOT = 128;
const int TAG_FUNC_EvCTOTtoCDT = 129;
const int TAG_FUNC_OPT_EvCTOT = 130;

inline static bool startsWith(const char* pre, const char* str)
{
	size_t lenpre = strlen(pre), lenstr = strlen(str);
	return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
};