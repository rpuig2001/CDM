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
const int TAG_ITEM_CTOT = 9;

const int TAG_FUNC_EDITEOBT = 100;
const int TAG_FUNC_NEWEOBT = 101;
const int TAG_FUNC_ON_OFF = 102;
const int TAG_FUNC_ADDTSAC = 103;
const int TAG_FUNC_EDITTSAC = 104;
const int TAG_FUNC_NEWTSAC = 105;
const int TAG_FUNC_ADDA = 106;
const int TAG_FUNC_CTOTOPTIONS = 107;
const int TAG_FUNC_REMOVECTOT = 108;
const int TAG_FUNC_ADDCTOTSELECTED = 109;
const int TAG_FUNC_ADDCTOT = 110;
const int TAG_FUNC_EOBTACTUALTIME = 111;


//const COLORREF TAG_GREEN = RGB(0, 190, 0);
const COLORREF TAG_GREEN = RGB(0, 192, 0);
const COLORREF TAG_GREENNOTACTIVE = RGB(143, 216, 148);
const COLORREF TAG_GREY = RGB(182, 182, 182);
const COLORREF TAG_ORANGE = RGB(212, 133, 46);
const COLORREF TAG_YELLOW = RGB(212, 214, 7);
const COLORREF TAG_DARKYELLOW = RGB(245, 239, 13);
const COLORREF TAG_RED = RGB(190, 0, 0);

inline static bool startsWith(const char* pre, const char* str)
{
	size_t lenpre = strlen(pre), lenstr = strlen(str);
	return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
};