#pragma once
#include "stdafx.h"

const int TAG_ITEM_EOBT = 1;
const int TAG_ITEM_TSAT = 2;
const int TAG_ITEM_TTOT = 3;
const int TAG_ITEM_TOBT = 4;
const int TAG_ITEM_TSAC = 5;
const int TAG_ITEM_ASRT = 6;
const int TAG_ITEM_A = 7;
const int TAG_ITEM_E = 8;

const int TAG_FUNC_EDITEOBT = 100;
const int TAG_FUNC_NEWEOBT = 101;
const int TAG_FUNC_ON_OFF = 102;
const int TAG_FUNC_ADDTSAC = 103;
const int TAG_FUNC_EDITTSAC = 104;
const int TAG_FUNC_NEWTSAC = 105;
const int TAG_FUNC_ADDA = 106;
const int TAG_FUNC_REMOVEA = 107;

//const COLORREF TAG_GREEN = RGB(0, 190, 0);
const COLORREF TAG_GREEN = RGB(95, 220, 1);
const COLORREF TAG_GREENNOTACTIVE = RGB(146, 205, 163);
const COLORREF TAG_GREY = RGB(128, 128, 128);
const COLORREF TAG_ORANGE = RGB(254, 189, 86);
const COLORREF TAG_YELLOW = RGB(255, 254, 107);
const COLORREF TAG_RED = RGB(190, 0, 0);

inline static bool startsWith(const char* pre, const char* str)
{
	size_t lenpre = strlen(pre), lenstr = strlen(str);
	return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
};