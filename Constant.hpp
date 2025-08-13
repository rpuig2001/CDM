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
const int TAG_ITEM_TSAT_TOBT_DIFF = 16;
const int NOW_TSAT_DIFF = 17;
const int TAG_ITEM_NETWORK_STATUS = 18;
const int TAG_ITEM_DEICE = 19;
const int NOW_TTOT_DIFF = 20;
const int NOW_CTOT_DIFF = 21;
const int TAG_ITEM_ETOBT = 22;

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
const int TAG_FUNC_REMOVEMANCTOT = 124;
const int TAG_FUNC_EDITMANCTOT = 125;
const int TAG_FUNC_EDITCDT = 126;
const int TAG_FUNC_FMASTEXT = 127;
const int TAG_FUNC_OPT_TTOT = 128;
const int TAG_FUNC_EvCTOTtoCTOT = 129;
const int TAG_FUNC_OPT_EvCTOT = 130;
const int TAG_FUNC_MODIFYMANCTOT = 131;
const int TAG_FUNC_TRY_TO_SET_CDT = 132;
const int TAG_FUNC_CUSTOMTSAT = 133;
const int TAG_FUNC_EDITFIRSTTSAT = 134;
const int TAG_FUNC_DISABLECTOT = 135;
const int TAG_FUNC_ENABLECTOT = 136;
const int TAG_FUNC_NETWORK_STATUS_OPTIONS = 137;
const int TAG_FUNC_NETWORK_REMOVE_STATUS = 138;
const int TAG_FUNC_NETWORK_SET_REA = 139;
const int TAG_FUNC_NETWORK_SET_PRIO = 140;
const int TAG_FUNC_OPT_DEICE = 141;
const int TAG_FUNC_DEICE_NONE = 142;
const int TAG_FUNC_DEICE_STAND = 143;
const int TAG_FUNC_DEICE_REMOTE = 144;
const int TAG_ITEM_TOBT_SETBY = 145;

inline static bool startsWith(const char* pre, const char* str)
{
	size_t lenpre = strlen(pre), lenstr = strlen(str);
	return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
};