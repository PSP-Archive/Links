/* 
	PSPRadio / Music streaming client for the PSP. (Initial Release: Sept. 2005)
	Copyright (C) 2005  Rafael Cabezas a.k.a. Raf
	
	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include <list>
#include <pspdisplay.h>
#include <PSPApp.h>
#include <PSPSound.h>
#include <stdio.h>
#include <unistd.h> 
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <malloc.h>
#include <iniparser.h>
#include <Tools.h>
#include <stdarg.h>
#include <Screen.h>
#include "TextUI.h"
#include <psputility_sysparam.h>

#define MAX_ROWS 		m_Screen->GetNumberOfTextRows()
#define MAX_COL 		m_Screen->GetNumberOfTextColumns()
#define PIXEL_TO_ROW(y)	((y)/m_Screen->GetFontHeight())
#define PIXEL_TO_COL(x) ((x)/m_Screen->GetFontWidth())
#define COL_TO_PIXEL(c) ((c)*m_Screen->GetFontWidth())
#define ROW_TO_PIXEL(r) ((r)*m_Screen->GetFontHeight())


#define TEXT_UI_CFG_FILENAME "TextUI/TextUI.cfg"

#define RGB2BGR(x) (((x>>16)&0xFF) | (x&0xFF00) | ((x<<16)&0xFF0000))

#define TextUILog ModuleLog

CTextUI::CTextUI()
{
	TextUILog(LOG_VERYLOW, "CtextUI: Constructor start");
	
	m_Config = NULL;
	m_lockprint = NULL;
	m_lockclear = NULL;
	m_CurrentScreen = CScreenHandler::PSPRADIO_SCREEN_PLAYLIST;
	m_Screen = new CScreen;
	m_strTitle = strdup("PSPRadio by Raf");
	
	m_lockprint = new CLock("Print_Lock");
	m_lockclear = new CLock("Clear_Lock");

	m_isdirty = false;
	m_LastBatteryPercentage = 0;
	sceRtcGetCurrentClockLocalTime(&m_LastLocalTime);
	
	TextUILog(LOG_VERYLOW, "CtextUI: Constructor end.");
}

CTextUI::~CTextUI()
{
	TextUILog(LOG_VERYLOW, "~CTextUI(): Start");
	if (m_lockprint)
	{
		delete(m_lockprint);
		m_lockprint = NULL;
	}
	if (m_lockclear)
	{
		delete(m_lockclear);
		m_lockclear = NULL;
	}
	if (m_Config)
	{
		delete(m_Config);
		m_Config = NULL;
	}
	if (m_strCWD)
	{
		free(m_strCWD), m_strCWD = NULL;
	}
	
	delete(m_Screen), m_Screen = NULL;
	TextUILog(LOG_VERYLOW, "~CTextUI(): End");
}

int CTextUI::Initialize(char *strCWD)
{
	TextUILog(LOG_VERYLOW, "CTextUI::Initialize Start");
	m_strCWD = strdup(strCWD);
	//m_Screen->Init();
	
	TextUILog(LOG_VERYLOW, "CTextUI::Initialize End");
	
	return 0;
}

int CTextUI::GetConfigColor(char *strKey)
{
	int iRet;
	sscanf(m_Config->GetStr(strKey), "%x", &iRet);
	
	return RGB2BGR(iRet);
}

void CTextUI::GetConfigPair(char *strKey, int *x, int *y)
{
	sscanf(m_Config->GetStr(strKey), "%d,%d", x, y);
}

void CTextUI::Terminate()
{
}

extern "C" 
{
	int module_stop(int args, void *argp);
	void __psp_libc_init(int argc, char *argv[]);
}

void CTextUI::LoadConfigSettings(IScreen *Screen)
{
	char *strCfgFile = NULL;

	pspDebugScreenInit();
	TextUILog(LOG_LOWLEVEL, "LoadConfigSettings() start");
	
	if (Screen->GetConfigFilename())
	{
		if (m_Config)
		{
			delete(m_Config);
		}
		strCfgFile = (char *)malloc(strlen(m_strCWD) + strlen(Screen->GetConfigFilename()) + 64);
		sprintf(strCfgFile, "%s/TextUI/%s", m_strCWD, Screen->GetConfigFilename());
	
		TextUILog(LOG_LOWLEVEL, "LoadConfigSettings(): Using '%s' config file", strCfgFile);
		
		m_Config = new CIniParser(strCfgFile);
		
		TextUILog(LOG_VERYLOW, "After instantiating the iniparser");
		
		free (strCfgFile), strCfgFile = NULL;
		
		memset(&m_ScreenConfig, 0, sizeof(m_ScreenConfig));
		
		/** General */
		m_ScreenConfig.ClockFormat = m_Config->GetInteger("GENERAL:CLOCK_FORMAT", 0);
		switch (m_ScreenConfig.ClockFormat)
		{
		case 12:
			m_ScreenConfig.ClockFormat = PSP_SYSTEMPARAM_TIME_FORMAT_12HR;
			break;
		case 24:
			m_ScreenConfig.ClockFormat = PSP_SYSTEMPARAM_TIME_FORMAT_24HR;
			break;
		case 0:
		default:
			sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_TIME_FORMAT, &m_ScreenConfig.ClockFormat);
			break;
		}
		//sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_TIME_FORMAT, &m_ScreenConfig.ClockFormat);
		
		m_ScreenConfig.FontMode   = (CScreen::textmode)m_Config->GetInteger("SCREEN_SETTINGS:FONT_MODE", 0);
		m_ScreenConfig.FontWidth  = m_Config->GetInteger("SCREEN_SETTINGS:FONT_WIDTH", 7);
		m_ScreenConfig.FontHeight = m_Config->GetInteger("SCREEN_SETTINGS:FONT_HEIGHT", 8);
		m_ScreenConfig.strBackground = m_Config->GetString("SCREEN_SETTINGS:BACKGROUND", NULL);
		m_ScreenConfig.BgColor = GetConfigColor("SCREEN_SETTINGS:BG_COLOR");
		m_ScreenConfig.FgColor = GetConfigColor("SCREEN_SETTINGS:FG_COLOR");
		GetConfigPair("SCREEN_SETTINGS:CONTAINERLIST_X_RANGE", 
								&m_ScreenConfig.ContainerListRangeX1, &m_ScreenConfig.ContainerListRangeX2);
		GetConfigPair("SCREEN_SETTINGS:CONTAINERLIST_Y_RANGE", 
								&m_ScreenConfig.ContainerListRangeY1, &m_ScreenConfig.ContainerListRangeY2);
		GetConfigPair("SCREEN_SETTINGS:ENTRIESLIST_X_RANGE", 
								&m_ScreenConfig.EntriesListRangeX1, &m_ScreenConfig.EntriesListRangeX2);
		GetConfigPair("SCREEN_SETTINGS:ENTRIESLIST_Y_RANGE", 
								&m_ScreenConfig.EntriesListRangeY1, &m_ScreenConfig.EntriesListRangeY2);
		GetConfigPair("SCREEN_SETTINGS:BUFFER_PERCENTAGE_XY", 
								&m_ScreenConfig.BufferPercentageX, &m_ScreenConfig.BufferPercentageY);
		m_ScreenConfig.BufferPercentageColor = GetConfigColor("SCREEN_SETTINGS:BUFFER_PERCENTAGE_COLOR");
		m_ScreenConfig.MetadataX1 = m_Config->GetInteger("SCREEN_SETTINGS:METADATA_X", 7);
		m_ScreenConfig.MetadataLength = m_Config->GetInteger("SCREEN_SETTINGS:METADATA_LEN", 50);
		GetConfigPair("SCREEN_SETTINGS:METADATA_Y_RANGE", 
								&m_ScreenConfig.MetadataRangeY1, &m_ScreenConfig.MetadataRangeY2);
		m_ScreenConfig.MetadataColor = GetConfigColor("SCREEN_SETTINGS:METADATA_COLOR");
		m_ScreenConfig.MetadataTitleColor = GetConfigColor("SCREEN_SETTINGS:METADATA_TITLE_COLOR");
		m_ScreenConfig.ListsTitleColor = GetConfigColor("SCREEN_SETTINGS:LISTS_TITLE_COLOR");
		m_ScreenConfig.EntriesListColor = GetConfigColor("SCREEN_SETTINGS:ENTRIESLIST_COLOR");
		m_ScreenConfig.SelectedEntryColor = GetConfigColor("SCREEN_SETTINGS:SELECTED_ENTRY_COLOR");
		m_ScreenConfig.PlayingEntryColor = GetConfigColor("SCREEN_SETTINGS:PLAYING_ENTRY_COLOR");
		GetConfigPair("SCREEN_SETTINGS:PROGRAM_VERSION_XY", 
								&m_ScreenConfig.ProgramVersionX, &m_ScreenConfig.ProgramVersionY);
		m_ScreenConfig.ProgramVersionColor = GetConfigColor("SCREEN_SETTINGS:PROGRAM_VERSION_COLOR");
		GetConfigPair("SCREEN_SETTINGS:STREAM_OPENING_XY", 
								&m_ScreenConfig.StreamOpeningX, &m_ScreenConfig.StreamOpeningY);
		GetConfigPair("SCREEN_SETTINGS:STREAM_OPENING_ERROR_XY", 
								&m_ScreenConfig.StreamOpeningErrorX, &m_ScreenConfig.StreamOpeningErrorY);
		GetConfigPair("SCREEN_SETTINGS:STREAM_OPENING_SUCCESS_XY", 
								&m_ScreenConfig.StreamOpeningSuccessX, &m_ScreenConfig.StreamOpeningSuccessY);
		m_ScreenConfig.StreamOpeningColor = GetConfigColor("SCREEN_SETTINGS:STREAM_OPENING_COLOR");
		m_ScreenConfig.StreamOpeningErrorColor = GetConfigColor("SCREEN_SETTINGS:STREAM_OPENING_ERROR_COLOR");
		m_ScreenConfig.StreamOpeningSuccessColor = GetConfigColor("SCREEN_SETTINGS:STREAM_OPENING_SUCCESS_COLOR");
		GetConfigPair("SCREEN_SETTINGS:CLEAN_ON_NEW_STREAM_Y_RANGE", 
								&m_ScreenConfig.CleanOnNewStreamRangeY1, &m_ScreenConfig.CleanOnNewStreamRangeY2);
		GetConfigPair("SCREEN_SETTINGS:ACTIVE_COMMAND_XY", 
								&m_ScreenConfig.ActiveCommandX, &m_ScreenConfig.ActiveCommandY);
		GetConfigPair("SCREEN_SETTINGS:ERROR_MESSAGE_XY", 
								&m_ScreenConfig.ErrorMessageX, &m_ScreenConfig.ErrorMessageY);
		m_ScreenConfig.ActiveCommandColor = GetConfigColor("SCREEN_SETTINGS:ACTIVE_COMMAND_COLOR");
		m_ScreenConfig.ErrorMessageColor = GetConfigColor("SCREEN_SETTINGS:ERROR_MESSAGE_COLOR");
		GetConfigPair("SCREEN_SETTINGS:NETWORK_ENABLING_XY", 
								&m_ScreenConfig.NetworkEnablingX, &m_ScreenConfig.NetworkEnablingY);
		GetConfigPair("SCREEN_SETTINGS:NETWORK_DISABLING_XY", 
								&m_ScreenConfig.NetworkDisablingX, &m_ScreenConfig.NetworkDisablingY);
		GetConfigPair("SCREEN_SETTINGS:NETWORK_READY_XY", 
								&m_ScreenConfig.NetworkReadyX, &m_ScreenConfig.NetworkReadyY);
		m_ScreenConfig.NetworkEnablingColor = GetConfigColor("SCREEN_SETTINGS:NETWORK_ENABLING_COLOR");
		m_ScreenConfig.NetworkDisablingColor = GetConfigColor("SCREEN_SETTINGS:NETWORK_DISABLING_COLOR");
		m_ScreenConfig.NetworkReadyColor = GetConfigColor("SCREEN_SETTINGS:NETWORK_READY_COLOR");
		GetConfigPair("SCREEN_SETTINGS:CLOCK_XY", 
								&m_ScreenConfig.ClockX, &m_ScreenConfig.ClockY);
		m_ScreenConfig.ClockColor = GetConfigColor("SCREEN_SETTINGS:CLOCK_COLOR");
		GetConfigPair("SCREEN_SETTINGS:BATTERY_XY", 
								&m_ScreenConfig.BatteryX, &m_ScreenConfig.BatteryY);
		m_ScreenConfig.BatteryColor = GetConfigColor("SCREEN_SETTINGS:BATTERY_COLOR");
		GetConfigPair("SCREEN_SETTINGS:CONTAINERLIST_TITLE_XY", 
			&m_ScreenConfig.ContainerListTitleX, &m_ScreenConfig.ContainerListTitleY);
		m_ScreenConfig.ContainerListTitleUnselectedColor =
			GetConfigColor("SCREEN_SETTINGS:CONTAINERLIST_TITLE_UNSELECTED_COLOR");
		m_ScreenConfig.strContainerListTitleUnselected =
			m_Config->GetString("SCREEN_SETTINGS:CONTAINERLIST_TITLE_UNSELECTED_STRING", "List");
		m_ScreenConfig.strContainerListTitleSelected =
			m_Config->GetString("SCREEN_SETTINGS:CONTAINERLIST_TITLE_SELECTED_STRING", "*List*");
		m_ScreenConfig.ContainerListTitleSelectedColor =
			GetConfigColor("SCREEN_SETTINGS:CONTAINERLIST_TITLE_SELECTED_COLOR");
		GetConfigPair("SCREEN_SETTINGS:ENTRIESLIST_TITLE_XY", 
			&m_ScreenConfig.EntriesListTitleX, &m_ScreenConfig.EntriesListTitleY);
		m_ScreenConfig.EntriesListTitleUnselectedColor =
			GetConfigColor("SCREEN_SETTINGS:ENTRIESLIST_TITLE_UNSELECTED_COLOR");
		m_ScreenConfig.strEntriesListTitleUnselected =
			m_Config->GetString("SCREEN_SETTINGS:ENTRIESLIST_TITLE_UNSELECTED_STRING", "Entries");
		m_ScreenConfig.strEntriesListTitleSelected =
			m_Config->GetString("SCREEN_SETTINGS:ENTRIESLIST_TITLE_SELECTED_STRING", "*Entries*");
		m_ScreenConfig.EntriesListTitleSelectedColor =
			GetConfigColor("SCREEN_SETTINGS:ENTRIESLIST_TITLE_SELECTED_COLOR");
			
		GetConfigPair("SCREEN_SETTINGS:TIME_XY", 
			&m_ScreenConfig.TimeX, &m_ScreenConfig.TimeY);
		m_ScreenConfig.TimeColor = GetConfigColor("SCREEN_SETTINGS:TIME_COLOR");
			
		m_ScreenConfig.ContainerListTitleLen = max(strlen(m_ScreenConfig.strContainerListTitleSelected), 
													strlen(m_ScreenConfig.strContainerListTitleUnselected));
		m_ScreenConfig.EntriesListTitleLen = max(strlen(m_ScreenConfig.strEntriesListTitleSelected), 
													strlen(m_ScreenConfig.strEntriesListTitleUnselected));
	}
	
	TextUILog(LOG_LOWLEVEL, "LoadConfigSettings() end");
}

void CTextUI::Initialize_Screen(IScreen *Screen)
{
	TextUILog(LOG_LOWLEVEL, "Inialize screen start");
	m_CurrentScreen = (CScreenHandler::Screen)Screen->GetId();

	LoadConfigSettings(Screen);
	TextUILog(LOG_LOWLEVEL, "After LoadConfigSettings");
	m_Screen->SetTextMode(m_ScreenConfig.FontMode);
	m_Screen->SetFontSize(m_ScreenConfig.FontWidth, m_ScreenConfig.FontHeight);
	m_Screen->SetBackColor(m_ScreenConfig.BgColor);
	m_Screen->SetTextColor(m_ScreenConfig.FgColor);
	if (m_ScreenConfig.strBackground)
	{
		char strPath[MAXPATHLEN+1];
		sprintf(strPath, "%s/%s", m_strCWD, m_ScreenConfig.strBackground);
		TextUILog(LOG_LOWLEVEL, "Calling SetBackgroundImage '%s'", strPath);
		m_Screen->SetBackgroundImage(strPath);
	}
	TextUILog(LOG_LOWLEVEL, "Calling m_Screen->Clear");
	m_Screen->Clear(); 
	
	TextUILog(LOG_LOWLEVEL, "Cleared");
	
	uiPrintf(m_ScreenConfig.ProgramVersionX, m_ScreenConfig.ProgramVersionY, 
			m_ScreenConfig.ProgramVersionColor, PSPRadioExport_GetProgramVersion());
	
	OnBatteryChange(m_LastBatteryPercentage);
	OnTimeChange(&m_LastLocalTime);
	
	TextUILog(LOG_LOWLEVEL, "Inialize screen end");
}

int CTextUI::OnVBlank()
{
	if (m_isdirty)
	{
		//flip
		m_isdirty = false;
	}
	return 0;
}

void CTextUI::UpdateOptionsScreen(list<OptionsScreen::Options> &OptionsList, 
										 list<OptionsScreen::Options>::iterator &CurrentOptionIterator)
{
	list<OptionsScreen::Options>::iterator OptionIterator;
	OptionsScreen::Options	Option;
	
	int x=-1,y=m_Config->GetInteger("SCREEN_SETTINGS:FIRST_ENTRY_Y",40),c=0xFFFFFF;
	
	if (OptionsList.size() > 0)
	{
		for (OptionIterator = OptionsList.begin() ; OptionIterator != OptionsList.end() ; OptionIterator++)
		{
			if (OptionIterator == CurrentOptionIterator)
			{
				c = GetConfigColor("SCREEN_SETTINGS:COLOR_OPTION_NAME_TEXT");//0xFFFFFF;
			}
			else
			{
				c = GetConfigColor("SCREEN_SETTINGS:COLOR_OPTION_SELECTED_NAME_TEXT");//0x888888;
			}
			
			Option = (*OptionIterator);
			
			//ClearRows(y);
			PrintOption(x,y,c, Option.strName, Option.strStates, Option.iNumberOfStates, Option.iSelectedState, 
						Option.iActiveState);
			
			y+=m_Config->GetInteger("SCREEN_SETTINGS:Y_INCREMENT",16); //was 2*
		}
	}
}

void CTextUI::PrintOption(int x, int y, int c, char *strName, char *strStates[], int iNumberOfStates, int iSelectedState,
						  int iActiveState)
{
	int iTextPos = PIXEL_TO_COL(m_Config->GetInteger("SCREEN_SETTINGS:FIRST_ENTRY_X",40));
	int color = 0xFFFFFF;
	
	uiPrintf(COL_TO_PIXEL(iTextPos), y, c, "%s: ", strName);
	if (iNumberOfStates > 0)
	{
		iTextPos += strlen(strName)+2;
		for (int iStates = 0; iStates < iNumberOfStates ; iStates++)
		{
			if (iStates+1 == iActiveState)
			{
				color = GetConfigColor("SCREEN_SETTINGS:COLOR_ACTIVE_STATE");//0x0000FF;
			}
			else if (iStates+1 == iSelectedState) /** 1-based */
			{
				color = GetConfigColor("SCREEN_SETTINGS:COLOR_SELECTED_STATE");//0xFFFFFF;
			}
			else
			{
				color = GetConfigColor("SCREEN_SETTINGS:COLOR_NOT_SELECTED_STATE");//0x888888;
			}
			
			if ((iStates+1 == iActiveState) && (iStates+1 == iSelectedState))
			{
				color =  GetConfigColor("SCREEN_SETTINGS:COLOR_ACTIVE_AND_SELECTED_STATE");//0x9090E3;
			}
			
			uiPrintf(COL_TO_PIXEL(iTextPos),y,color, "%s ", strStates[iStates]);
			iTextPos += strlen(strStates[iStates])+1;
		}
	}	
}

void CTextUI::uiPrintf(int x, int y, int color, char *strFormat, ...)
{
	va_list args;
	char msg[70*5/** 5 lines worth of text...*/];

	/** -2  = Don't Print */
	if (x != -2)
	{
		va_start (args, strFormat);         /* Initialize the argument list. */
		
		vsprintf(msg, strFormat, args);
	
		if (msg[strlen(msg)-1] == 0x0A)
			msg[strlen(msg)-1] = 0; /** Remove LF 0D*/
		if (msg[strlen(msg)-1] == 0x0D) 
			msg[strlen(msg)-1] = 0; /** Remove CR 0A*/
	
		if (x == -1) /** CENTER */
		{
			x = PSP_SCREEN_WIDTH/2 - ((strlen(msg)/2)*m_Screen->GetFontWidth());
		}
		m_Screen->PrintText(x, y, color, msg);
		
		va_end (args);                  /* Clean up. */
	}
}

void CTextUI::ClearRows(int iRowStart, int iRowEnd)
{
	if (iRowEnd == -1)
		iRowEnd = iRowStart;
		
	m_lockclear->Lock();
	for (int iRow = iRowStart ; (iRow < PSP_SCREEN_HEIGHT) && (iRow <= iRowEnd); iRow+=m_Screen->GetFontHeight())
	{
		//m_Screen->ClearLine(PIXEL_TO_ROW(iRow));///m_Screen->GetFontHeight());
		m_Screen->ClearCharsAtYFromX1ToX2(iRow, 0, PSP_SCREEN_WIDTH);
	}
	m_lockclear->Unlock();
}

void CTextUI::ClearHalfRows(int iColStart, int iColEnd, int iRowStart, int iRowEnd)
{
	if (iRowEnd == -1)
		iRowEnd = iRowStart;
		
	m_lockclear->Lock();
	for (int iRow = iRowStart ; (iRow < PSP_SCREEN_HEIGHT) && (iRow <= iRowEnd); iRow+=m_Screen->GetFontHeight())
	{
		m_Screen->ClearCharsAtYFromX1ToX2(iRow, iColStart, iColEnd);
	}
	m_lockclear->Unlock();
}
	
	
int CTextUI::SetTitle(char *strTitle)
{
//	int x,y;
//	int c;
//	GetConfigPair("TEXT_POS:TITLE", &x, &y);
//	c = GetConfigColor("COLORS:TITLE");
//	uiPrintf(x,y, c, strTitle);
	
	if (m_strTitle)
	{
		free(m_strTitle);
	}
	
	m_strTitle = strdup(strTitle);
	
	return 0;
}

void CTextUI::OnBatteryChange(int Percentage)
{
	uiPrintf(m_ScreenConfig.BatteryX, m_ScreenConfig.BatteryY, m_ScreenConfig.BatteryColor, "B:%03d%%", Percentage);
	m_LastBatteryPercentage = Percentage;
}

void CTextUI::OnTimeChange(pspTime *LocalTime)
{
	if (m_ScreenConfig.ClockFormat == PSP_SYSTEMPARAM_TIME_FORMAT_24HR)
	{
		uiPrintf(m_ScreenConfig.ClockX, m_ScreenConfig.ClockY, m_ScreenConfig.ClockColor, "%02d:%02d", 
					LocalTime->hour, LocalTime->minutes);
	}
	else
	{
		bool bIsPM = (LocalTime->hour)>12;
		uiPrintf(m_ScreenConfig.ClockX, m_ScreenConfig.ClockY, m_ScreenConfig.ClockColor, "%02d:%02d%s", 
					bIsPM?(LocalTime->hour-12):(LocalTime->hour==0?12:LocalTime->hour), 
					LocalTime->minutes,
					bIsPM?"PM":"AM");
	}
	m_LastLocalTime = *LocalTime;
}

int CTextUI::DisplayMessage_EnablingNetwork()
{
	//int x,y,c;
	//GetConfigPair("TEXT_POS:NETWORK_ENABLING", &x, &y);
	//c = GetConfigColor("COLORS:NETWORK_ENABLING");
	
	//ClearErrorMessage();
	ClearRows(m_ScreenConfig.NetworkEnablingY);
	uiPrintf(m_ScreenConfig.NetworkEnablingX, m_ScreenConfig.NetworkEnablingY, m_ScreenConfig.NetworkEnablingColor, "Enabling Network");
	
	return 0;
}

int CTextUI::DisplayMessage_DisablingNetwork()
{
	//int x,y,c;
	//GetConfigPair("TEXT_POS:NETWORK_DISABLING", &x, &y);
	//c = GetConfigColor("COLORS:NETWORK_DISABLING");
	
	ClearRows(m_ScreenConfig.NetworkDisablingY);
	uiPrintf(m_ScreenConfig.NetworkDisablingX, m_ScreenConfig.NetworkDisablingY, m_ScreenConfig.NetworkDisablingColor, "Disabling Network");
	
	return 0;
}

int CTextUI::DisplayMessage_NetworkReady(char *strIP)
{
	//int x,y,c;
	//GetConfigPair("TEXT_POS:NETWORK_READY", &x, &y);
	//c = GetConfigColor("COLORS:NETWORK_READY");
	
	ClearRows(m_ScreenConfig.NetworkReadyY);
	uiPrintf(m_ScreenConfig.NetworkReadyX, m_ScreenConfig.NetworkReadyY, m_ScreenConfig.NetworkReadyColor, "Ready, IP %s", strIP);
	
	return 0;
}

int CTextUI::DisplayMainCommands()
{
//	int x,y,c;
//	GetConfigPair("TEXT_POS:MAIN_COMMANDS", &x, &y);
//	c = GetConfigColor("COLORS:MAIN_COMMANDS");
	
//	ClearRows(y);
//	uiPrintf(x, y, c, "X Play/Pause | [] Stop | L / R To Browse");
		
	return 0;
}

int CTextUI::DisplayActiveCommand(CPSPSound::pspsound_state playingstate)
{
	//int x,y,c;
	//GetConfigPair("TEXT_POS:ACTIVE_COMMAND", &x, &y);
	//c = GetConfigColor("COLORS:ACTIVE_COMMAND");
	
	ClearRows(m_ScreenConfig.ActiveCommandY);
	switch(playingstate)
	{
	case CPSPSound::STOP:
		{
			uiPrintf(m_ScreenConfig.ActiveCommandX, m_ScreenConfig.ActiveCommandY, m_ScreenConfig.ActiveCommandColor, "STOP");
			//int r1,r2;
			//GetConfigPair("TEXT_POS:METADATA_ROW_RANGE", &r1, &r2);
			ClearRows(m_ScreenConfig.MetadataRangeY1, m_ScreenConfig.MetadataRangeY2);
			//int px,py;
			//GetConfigPair("TEXT_POS:BUFFER_PERCENTAGE", &px, &py);
			ClearRows(m_ScreenConfig.BufferPercentageY);
			break;
		}
	case CPSPSound::PLAY:
		uiPrintf(m_ScreenConfig.ActiveCommandX, m_ScreenConfig.ActiveCommandY, m_ScreenConfig.ActiveCommandColor, "PLAY");
		break;
	case CPSPSound::PAUSE:
		uiPrintf(m_ScreenConfig.ActiveCommandX, m_ScreenConfig.ActiveCommandY, m_ScreenConfig.ActiveCommandColor, "PAUSE");
		break;
	}
	
	return 0;
}

int CTextUI::DisplayErrorMessage(char *strMsg)
{
	int x,y,c;
	//c = GetConfigColor("COLORS:ERROR_MESSAGE");
	x = m_ScreenConfig.ErrorMessageX;
	y = m_ScreenConfig.ErrorMessageY;
	c = m_ScreenConfig.ErrorMessageColor;
	
	#if 0
	switch (m_CurrentScreen)
	{
		case CScreenHandler::PSPRADIO_SCREEN_SHOUTCAST_BROWSER:
		case CScreenHandler::PSPRADIO_SCREEN_PLAYLIST:
			GetConfigPair("TEXT_POS:ERROR_MESSAGE", &x, &y);
			ClearErrorMessage();
			break;
		case CScreenHandler::PSPRADIO_SCREEN_SETTINGS:
			GetConfigPair("TEXT_POS:ERROR_MESSAGE_IN_OPTIONS", &x, &y);
			ClearRows(y);
			break;
	}
	#endif
	ClearErrorMessage();
	/** If message is longer than 1 lines, then truncate;
	The -10 is to accomodate for the "Error: " plus a bit.
	*/
	if (strlen(strMsg)>(size_t)(PIXEL_TO_COL(PSP_SCREEN_WIDTH) - 10))
	{
		strMsg[(PIXEL_TO_COL(PSP_SCREEN_WIDTH) - 10)] = 0;
	}
	uiPrintf(x, y, c, "Error: %s", strMsg);
	
	return 0;
}

int CTextUI::DisplayMessage(char *strMsg)
{
	int x,y,c;
	//c = GetConfigColor("COLORS:ERROR_MESSAGE");
	x = m_ScreenConfig.ErrorMessageX;
	y = m_ScreenConfig.ErrorMessageY;
	c = m_ScreenConfig.ErrorMessageColor;
	#if 0
	switch (m_CurrentScreen)
	{
		case CScreenHandler::PSPRADIO_SCREEN_SHOUTCAST_BROWSER:
		case CScreenHandler::PSPRADIO_SCREEN_PLAYLIST:
			GetConfigPair("TEXT_POS:ERROR_MESSAGE", &x, &y);
			ClearErrorMessage();
			break;
		case CScreenHandler::PSPRADIO_SCREEN_SETTINGS:
			GetConfigPair("TEXT_POS:ERROR_MESSAGE_IN_OPTIONS", &x, &y);
			ClearRows(y);
			break;
	}
	#endif
	ClearErrorMessage();
	/** If message is longer than 1 lines, then truncate;
	 *  The -3 is just in case.
	 */
	if (strlen(strMsg)>(size_t)(PIXEL_TO_COL(PSP_SCREEN_WIDTH) - 3))
	{
		strMsg[(PIXEL_TO_COL(PSP_SCREEN_WIDTH) - 3)] = 0;
	}
	uiPrintf(-1, y, c, "%s", strMsg);
	
	return 0;
}

int CTextUI::ClearErrorMessage()
{
	//int x,y;
	//GetConfigPair("TEXT_POS:ERROR_MESSAGE_ROW_RANGE", &x, &y);
	//ClearRows(x, y);
	ClearRows(m_ScreenConfig.ErrorMessageY);
	return 0;
}

int CTextUI::DisplayBufferPercentage(int iPerc)
{
	//int c;

	if (CScreenHandler::PSPRADIO_SCREEN_OPTIONS != m_CurrentScreen)
	{
		//GetConfigPair("TEXT_POS:BUFFER_PERCENTAGE", &x, &y);
		//c = GetConfigColor("COLORS:BUFFER_PERCENTAGE");
	
		if (iPerc >= 95)
			iPerc = 100;
		if (iPerc < 2)
			iPerc = 0;

		//uiPrintf(x, y, c, "Buffer: %03d%c%c", iPerc, 37, 37/* 37='%'*/);
		uiPrintf(m_ScreenConfig.BufferPercentageX, m_ScreenConfig.BufferPercentageY, m_ScreenConfig.BufferPercentageColor, "Buffer: %03d%c", iPerc, 37/* 37='%'*/);
	}
	return 0;
}

int CTextUI::OnNewStreamStarted()
{
	return 0;
}

int CTextUI::OnStreamOpening()
{
	int x,y,c;
	//GetConfigPair("TEXT_POS:STREAM_OPENING", &x, &y);
	//c = GetConfigColor("COLORS:STREAM_OPENING");
	x = m_ScreenConfig.StreamOpeningX;
	y = m_ScreenConfig.StreamOpeningY;
	c = m_ScreenConfig.StreamOpeningColor;
	
	ClearErrorMessage(); /** Clear any errors */
	ClearRows(y);
	uiPrintf(x, y, c, "Opening Stream");
	return 0;
}

int CTextUI::OnConnectionProgress()
{
	int x,y,c;
	//GetConfigPair("TEXT_POS:STREAM_OPENING", &x, &y);
	//c = GetConfigColor("COLORS:STREAM_OPENING");
	char *strIndicator[] = {"OpEnInG StReAm", "oPeNiNg sTrEaM"};
	static int sIndex = -1;
	x = m_ScreenConfig.StreamOpeningX;
	y = m_ScreenConfig.StreamOpeningY;
	c = m_ScreenConfig.StreamOpeningColor;

	sIndex = (sIndex+1)%2;
	
	uiPrintf(x, y, c, "%s", strIndicator[sIndex]);
	

	return 0;
}

int CTextUI::OnStreamOpeningError()
{
	int x,y,c;
	//GetConfigPair("TEXT_POS:STREAM_OPENING_ERROR", &x, &y);
	//c = GetConfigColor("COLORS:STREAM_OPENING_ERROR");
	x = m_ScreenConfig.StreamOpeningErrorX;
	y = m_ScreenConfig.StreamOpeningErrorY;
	c = m_ScreenConfig.StreamOpeningErrorColor;
	
	ClearRows(y);
	uiPrintf(x, y, c, "Error Opening Stream");
	return 0;
}

int CTextUI::OnStreamOpeningSuccess()
{
	int x,y,c;
	//GetConfigPair("TEXT_POS:STREAM_OPENING_SUCCESS", &x, &y);
	//c = GetConfigColor("COLORS:STREAM_OPENING_SUCCESS");
	x = m_ScreenConfig.StreamOpeningSuccessX;
	y = m_ScreenConfig.StreamOpeningSuccessY;
	c = m_ScreenConfig.StreamOpeningSuccessColor;
	
	ClearRows(y);
	//uiPrintf(x, y, c, "Stream Opened");
	return 0;
}

int CTextUI::OnNewSongData(MetaData *pData)
{
	//int r1,r2,x1;

	if (CScreenHandler::PSPRADIO_SCREEN_OPTIONS != m_CurrentScreen)
	{

		//GetConfigPair("TEXT_POS:METADATA_ROW_RANGE", &r1, &r2);
		//x1 = m_Config->GetInteger("TEXT_POS:METADATA_START_COLUMN", 0);
		int y = m_ScreenConfig.MetadataRangeY1;
		int x = m_ScreenConfig.MetadataX1;
		int cTitle = m_ScreenConfig.MetadataTitleColor;
		int c = m_ScreenConfig.MetadataColor;
		int iLen = m_ScreenConfig.MetadataLength;
		ClearRows(y, m_ScreenConfig.MetadataRangeY2);
		
		if (strlen(pData->strTitle) >= (size_t)(m_Screen->GetNumberOfTextColumns()-10-PIXEL_TO_COL(x)))
			pData->strTitle[m_Screen->GetNumberOfTextColumns()-10-PIXEL_TO_COL(x)] = 0;
			
		if (strlen(pData->strURL) >= (size_t)(m_Screen->GetNumberOfTextColumns()-10-PIXEL_TO_COL(x)))
			pData->strURL[m_Screen->GetNumberOfTextColumns()-10-PIXEL_TO_COL(x)] = 0;
		
		if (0 != pData->iSampleRate)
		{
			uiPrintf(x, y, cTitle, "%lukbps %dHz (%d channels) stream",
					pData->iBitRate/1000, 
					pData->iSampleRate,
					pData->iNumberOfChannels);
					//pData->strMPEGLayer);
			y+=m_Screen->GetFontHeight();
		}
		if (pData->strURL && strlen(pData->strURL))
		{
			uiPrintf(x, y, cTitle,	"URL   : ");
			uiPrintf(x+COL_TO_PIXEL(8), y, c,	"%-*.*s ", iLen, iLen, pData->strURL);
		}
		else
		{
			uiPrintf(x , y,	cTitle,	"Stream: ");
			uiPrintf(x+COL_TO_PIXEL(8), y,	c,		"%-*.*s ", iLen, iLen, pData->strURI);
		}
		y+=m_Screen->GetFontHeight();
		
		uiPrintf(x , y,	cTitle,	"Title : ");
		uiPrintf(x+COL_TO_PIXEL(8), y,	c, 	"%-*.*s ", iLen, iLen, pData->strTitle);
		y+=m_Screen->GetFontHeight();
		
		if (pData->strArtist && strlen(pData->strArtist))
		{
			uiPrintf(x , y,	cTitle,	"Artist: ");
			uiPrintf(x+COL_TO_PIXEL(8), y,	c, 	"%-*.*s ", iLen, iLen, pData->strArtist);
		}
	}
	return 0;
}

int CTextUI::OnStreamTimeUpdate(MetaData *pData)
{
	int y = m_ScreenConfig.TimeY;
	int x = m_ScreenConfig.TimeX;
	int c = m_ScreenConfig.TimeColor;
	
	if (pData->lTotalTime > 0)
	{
		uiPrintf(x, y, c, "%02d:%02d / %02d:%02d",
					pData->lCurrentTime / 60, pData->lCurrentTime % 60,
					pData->lTotalTime / 60, pData->lTotalTime % 60);
	}
	else
	{
		uiPrintf(x, y, c, "%02d:%02d",
					pData->lCurrentTime / 60, pData->lCurrentTime % 60);
	}
	return 0;
}

void CTextUI::DisplayContainers(CMetaDataContainer *Container)
{
	int iColorNormal, iColorSelected, iColorTitle, iColor, iColorPlaying;
	int iNextRow = 0;
	char *strText = NULL;

	map< string, list<MetaData>* >::iterator ListIterator;
	map< string, list<MetaData>* >::iterator *CurrentHighlightedElement = Container->GetCurrentContainerIterator();
	map< string, list<MetaData>* >::iterator *CurrentPlayingElement = Container->GetPlayingContainerIterator();
	map< string, list<MetaData>* > *List = Container->GetContainerList();
	iColorNormal   = m_ScreenConfig.EntriesListColor;
	iColorTitle    = m_ScreenConfig.ListsTitleColor;
	iColorSelected = m_ScreenConfig.SelectedEntryColor;
	iColor = iColorNormal;
	iColorPlaying  = m_ScreenConfig.PlayingEntryColor;
	
	bool bShowFileExtension = m_Config->GetInteger("GENERAL:SHOW_FILE_EXTENSION", 0);

	ClearHalfRows(m_ScreenConfig.ContainerListRangeX1, m_ScreenConfig.ContainerListRangeX2, m_ScreenConfig.ContainerListRangeY1, m_ScreenConfig.ContainerListRangeY2);

	iNextRow = m_ScreenConfig.ContainerListRangeY1;
	
	strText = (char *)malloc (MAXPATHLEN+1);
	
	//TextUILog(LOG_VERYLOW, "DisplayContainers(): populating screen");
	if (List->size() > 0)
	{
		//TextUILog(LOG_VERYLOW, "DisplayContainers(): Setting iterator to middle of the screen");
		ListIterator = *CurrentHighlightedElement;
		for (int i = 0; i < PIXEL_TO_ROW(m_ScreenConfig.ContainerListRangeY2-m_ScreenConfig.ContainerListRangeY1)/2; i++)
		{
			if (ListIterator == List->begin())
				break;
			ListIterator--;
		
		}

		//TextUILog(LOG_VERYLOW, "DisplayPLEntries(): elements: %d", List->size());
		//TextUILog(LOG_VERYLOW, "DisplayContainers(): Populating Screen (total elements %d)", List->size());
		for (; ListIterator != List->end() ; ListIterator++)
		{
			if (iNextRow > m_ScreenConfig.ContainerListRangeY2)
			{
				break;
			}
			
			if (ListIterator == *CurrentHighlightedElement)
			{
				iColor = iColorSelected;
			}
			else if (ListIterator == *CurrentPlayingElement)
			{
				iColor = iColorPlaying;
			}
			else
			{
				iColor = iColorNormal;
			}
			
			strlcpy(strText, ListIterator->first.c_str(), MAXPATHLEN);
			
			if (strlen(strText) > 4 && memcmp(strText, "ms0:", 4) == 0)
			{
				char *pText = basename(strText);
				if (false == bShowFileExtension)
				{
					char *ext = strrchr(pText, '.');
					if(ext)
					{
						ext[0] = 0;
					}
				}
				pText[PIXEL_TO_COL(m_ScreenConfig.ContainerListRangeX2-m_ScreenConfig.ContainerListRangeX1)] = 0;
				uiPrintf(m_ScreenConfig.ContainerListRangeX1, iNextRow, iColor, pText);
			}
			else
			{
				strText[PIXEL_TO_COL(m_ScreenConfig.ContainerListRangeX2-m_ScreenConfig.ContainerListRangeX1)] = 0;
				uiPrintf(m_ScreenConfig.ContainerListRangeX1, iNextRow, iColor, strText);
			}
			iNextRow+=m_Screen->GetFontHeight();
		}
	}
	
	free(strText), strText = NULL;
}

void CTextUI::DisplayElements(CMetaDataContainer *Container)
{
	int iNextRow;
	int iColorNormal,iColorTitle,iColorSelected, iColor, iColorPlaying;
	char *strText = NULL;
	
	list<MetaData>::iterator ListIterator;
	list<MetaData>::iterator *CurrentHighlightedElement = Container->GetCurrentElementIterator();
	list<MetaData>::iterator *CurrentPlayingElement = Container->GetPlayingElementIterator();
	list<MetaData> *List = Container->GetElementList();
	iColorNormal = m_ScreenConfig.EntriesListColor;
	iColorSelected = m_ScreenConfig.SelectedEntryColor;
	iColorPlaying  = m_ScreenConfig.PlayingEntryColor;
	iColorTitle = m_ScreenConfig.ListsTitleColor;
	iColor = iColorNormal;

	bool bShowFileExtension = m_Config->GetInteger("GENERAL:SHOW_FILE_EXTENSION", 0);
	
	ClearHalfRows(m_ScreenConfig.EntriesListRangeX1,m_ScreenConfig.EntriesListRangeX2,
				  m_ScreenConfig.EntriesListRangeY1,m_ScreenConfig.EntriesListRangeY2);
	
	iNextRow = m_ScreenConfig.EntriesListRangeY1;
	
	strText = (char *)malloc (MAXPATHLEN+1);
	memset(strText, 0, MAXPATHLEN);
	
	//TextUILog(LOG_VERYLOW, "DisplayPLEntries(): populating screen");
	if (List->size() > 0)
	{
		ListIterator = *CurrentHighlightedElement;
		for (int i = 0; i < PIXEL_TO_ROW(m_ScreenConfig.EntriesListRangeY2-m_ScreenConfig.EntriesListRangeY1)/2; i++)
		{
			if (ListIterator == List->begin())
				break;
			ListIterator--;
		
		}
		//TextUILog(LOG_VERYLOW, "DisplayPLEntries(): elements: %d", List->size());
		for (; ListIterator != List->end() ; ListIterator++)
		{
			if (iNextRow > m_ScreenConfig.EntriesListRangeY2)
			{
				break;
			}
			
			if (ListIterator == *CurrentHighlightedElement)
			{
				iColor = iColorSelected;
			}
			else if (ListIterator == *CurrentPlayingElement)
			{
				iColor = iColorPlaying;
			}
			else
			{
				iColor = iColorNormal;
			}
			
			
			char *pText = strText;
			if (strlen((*ListIterator).strTitle))
			{
				//TextUILog(LOG_VERYLOW, "DisplayPLEntries(): Using strTitle='%s'", (*ListIterator).strTitle);
				strlcpy(strText, (*ListIterator).strTitle, MAXPATHLEN);
			}
			else
			{
				strlcpy(strText, (*ListIterator).strURI, MAXPATHLEN);
				
				if (strlen(strText) > 4 && memcmp(strText, "ms0:", 4) == 0)
				{
					pText = basename(strText);
					if (false == bShowFileExtension)
					{
						char *ext = strrchr(pText, '.');
						if(ext)
						{
							ext[0] = 0;
						}
					}
				}
			}
		
			pText[PIXEL_TO_COL(m_ScreenConfig.EntriesListRangeX2-m_ScreenConfig.EntriesListRangeX1)] = 0;
			uiPrintf(m_ScreenConfig.EntriesListRangeX1, iNextRow, iColor, pText);
			iNextRow+=m_Screen->GetFontHeight();
		}
	}
	
	free(strText), strText = NULL;
}

void CTextUI::OnCurrentContainerSideChange(CMetaDataContainer *Container)
{
	ClearHalfRows(m_ScreenConfig.ContainerListTitleX,
				m_ScreenConfig.ContainerListTitleX + COL_TO_PIXEL(m_ScreenConfig.ContainerListTitleLen),
				m_ScreenConfig.ContainerListTitleY,m_ScreenConfig.ContainerListTitleY);
	
	ClearHalfRows(m_ScreenConfig.EntriesListTitleX,
				m_ScreenConfig.EntriesListTitleX + COL_TO_PIXEL(m_ScreenConfig.EntriesListTitleLen),
				m_ScreenConfig.EntriesListTitleY,m_ScreenConfig.EntriesListTitleY);
	
	
	switch (Container->GetCurrentSide())
	{
		case CMetaDataContainer::CONTAINER_SIDE_CONTAINERS:
			uiPrintf(m_ScreenConfig.ContainerListTitleX, m_ScreenConfig.ContainerListTitleY,
					m_ScreenConfig.ContainerListTitleSelectedColor, m_ScreenConfig.strContainerListTitleSelected);
			uiPrintf(m_ScreenConfig.EntriesListTitleX, m_ScreenConfig.EntriesListTitleY, 
					m_ScreenConfig.EntriesListTitleUnselectedColor, m_ScreenConfig.strEntriesListTitleUnselected);
			break;
		
		case CMetaDataContainer::CONTAINER_SIDE_ELEMENTS:
			uiPrintf(m_ScreenConfig.ContainerListTitleX, m_ScreenConfig.ContainerListTitleY, 	
					m_ScreenConfig.ContainerListTitleUnselectedColor, m_ScreenConfig.strContainerListTitleUnselected);
			uiPrintf(m_ScreenConfig.EntriesListTitleX, m_ScreenConfig.EntriesListTitleY, 
					m_ScreenConfig.EntriesListTitleSelectedColor, m_ScreenConfig.strEntriesListTitleSelected);
			break;
	
	}
}
