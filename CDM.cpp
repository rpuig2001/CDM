// rStatus.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "EuroScopePlugIn.h"
#include "CDMSingle.hpp"

CDM* gpMyPlugin = NULL;

void    __declspec (dllexport)    EuroScopePlugInInit(EuroScopePlugIn::CPlugIn** ppPlugInInstance)
{
	// create the instance
	*ppPlugInInstance = gpMyPlugin = new CDM();
}


//---EuroScopePlugInExit-----------------------------------------------

void    __declspec (dllexport)    EuroScopePlugInExit(void)
{
	// delete the instance
	delete gpMyPlugin;
}