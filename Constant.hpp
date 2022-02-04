#pragma once
#include "stdafx.h"

const int TAG_ITEM_EOBT = 1;
const int TAG_ITEM_TSAT = 2;
const int TAG_ITEM_TTOT = 3;
const int TAG_ITEM_TOBT = 4;
const int TAG_ITEM_TSAC = 5;
const int TAG_ITEM_ASAT = 6;
const int TAG_ITEM_ASRT = 7;
const int TAG_ITEM_A = 8;
const int TAG_ITEM_E = 9;
const int TAG_ITEM_CTOT = 10;

const int TAG_FUNC_EDITEOBT = 100;
const int TAG_FUNC_NEWEOBT = 101;
const int TAG_FUNC_ON_OFF = 102;
const int TAG_FUNC_ADDTSAC = 103;
const int TAG_FUNC_EDITTSAC = 104;
const int TAG_FUNC_NEWTSAC = 105;
const int TAG_FUNC_TOGGLEA = 106;
const int TAG_FUNC_TOGGLEASRT = 107;
const int TAG_FUNC_CTOTOPTIONS = 108;
const int TAG_FUNC_REMOVECTOT = 109;
const int TAG_FUNC_ADDCTOTSELECTED = 110;
const int TAG_FUNC_ADDCTOT = 111;
const int TAG_FUNC_REAASRT = 112;
const int TAG_FUNC_REA = 113;
const int TAG_FUNC_READYTOBT = 114;
const int TAG_FUNC_EDITTOBT = 115;
const int TAG_FUNC_NEWTOBT = 116;

inline static bool startsWith(const char* pre, const char* str)
{
	size_t lenpre = strlen(pre), lenstr = strlen(str);
	return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
};