/*
	PSPRadio / Music streaming client for the PSP. (Initial Release: Sept. 2005)
	PSPRadio Copyright (C) 2005 Rafael Cabezas a.k.a. Raf
	TextUI3D Copyright (C) 2005 Jesper Sandberg & Raf


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
#include <PSPApp.h>
#include <PSPSound.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <malloc.h>
#include <math.h>
#include <time.h>
#include <iniparser.h>
#include <Tools.h>
#include <stdarg.h>
#include <Logging.h>

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <psprtc.h>
#include <psppower.h>

#include "TextUI3D.h"
#include "TextUI3D_Panel.h"
#include "jsaTextureCache.h"

#define RGB2BGR(x) (((x>>16)&0xFF) | (x&0xFF00) | ((x<<16)&0xFF0000))

/* Global texture cache */
jsaTextureCache		tcache;

/* Settings */
Settings 	LocalSettings;
gfx_sizes	GfxSizes;

CTextUI3D::CTextUI3D()
{
	ModuleLog(LOG_VERYLOW, "CTextUI3D: created");
	m_state	= CScreenHandler::PSPRADIO_SCREENSHOT_NOT_ACTIVE;
	m_title[0] = 0;
	m_option_list = m_option_list_tail = NULL;
	m_list_list = m_list_list_tail = NULL;
	m_entry_list = m_entry_list_tail = NULL;
}

CTextUI3D::~CTextUI3D()
{
	if (m_Settings)
	{
		delete(m_Settings);
		m_Settings = NULL;
	}
	ModuleLog(LOG_VERYLOW, "CTextUI3D: destroyed.");
}

int CTextUI3D::Initialize(char *strCWD)
{
char *strCfgFile = NULL;

	/* Load settings from config file */
	strCfgFile = (char *)malloc(strlen(strCWD) + strlen("TextUI3D/TextUI3D.cfg") + 10);
	sprintf(strCfgFile, "%s/%s", strCWD, "TextUI3D/TextUI3D.cfg");
	m_Settings = new CIniParser(strCfgFile);
	free (strCfgFile), strCfgFile = NULL;
	memset(&LocalSettings, 0, sizeof(LocalSettings));
	GetSettings();
	ModuleLog(LOG_VERYLOW, "CTextUI3D:Settings read");

	/* Allocate space in VRAM for 2 displaybuffer and the Zbuffer */
	jsaVRAMManager::jsaVRAMManagerInit((unsigned long)0x154000);

	/* Initialize WindowManager */
	m_wmanager.Initialize(strCWD);


	sceKernelDcacheWritebackAll();

	return 0;
}

void CTextUI3D::GetConfigPair(char *strKey, int *x, int *y)
{
 	sscanf(m_Settings->GetStr(strKey), "%d,%d", x, y);
}

int CTextUI3D::GetConfigColor(char *strKey)
{
	int iRet;
	sscanf(m_Settings->GetStr(strKey), "%x", &iRet);

	iRet = RGB2BGR(iRet);
	return iRet | 0xFF000000;
}

void CTextUI3D::GetSettings()
{
	/* Settings */
	LocalSettings.EventThreadPrio = m_Settings->GetInteger("SETTINGS:EVENT_THREAD_PRIO", 80);
	GetConfigPair("SETTINGS:PROGRAM_VERSION_XY", &LocalSettings.VersionX, &LocalSettings.VersionY);
	GetConfigPair("SETTINGS:IP_XY", &LocalSettings.IPX, &LocalSettings.IPY);
	GetConfigPair("SETTINGS:BATTERY_ICON_XY", &LocalSettings.BatteryIconX, &LocalSettings.BatteryIconY);
	GetConfigPair("SETTINGS:WIFI_ICON_XY", &LocalSettings.WifiIconX, &LocalSettings.WifiIconY);
	GetConfigPair("SETTINGS:LIST_ICON_XY", &LocalSettings.ListIconX, &LocalSettings.ListIconY);
	GetConfigPair("SETTINGS:VOLUME_ICON_XY", &LocalSettings.VolumeIconX, &LocalSettings.VolumeIconY);
	GetConfigPair("SETTINGS:CLOCK_XY", &LocalSettings.ClockX, &LocalSettings.ClockY);
	LocalSettings.ClockFormat = m_Settings->GetInteger("SETTINGS:CLOCK_FORMAT", 24);
	GetConfigPair("SETTINGS:PLAYTIME_XY", &LocalSettings.PlayTimeX, &LocalSettings.PlayTimeY);
	GetConfigPair("SETTINGS:PROGRESS_BAR_XY", &LocalSettings.ProgressBarX, &LocalSettings.ProgressBarY);
	GetConfigPair("SETTINGS:OPTIONS_XY", &LocalSettings.OptionsX, &LocalSettings.OptionsY);
	LocalSettings.OptionsItemX = m_Settings->GetInteger("SETTINGS:OPTIONS_ITEM_X", 256);
	LocalSettings.OptionsLinespace = m_Settings->GetInteger("SETTINGS:OPTIONS_LINESPACE", 10);
	LocalSettings.OptionsColorNotSelected = GetConfigColor("SETTINGS:OPTIONS_COLOR_NOT_SELECTED");
	LocalSettings.OptionsColorSelected = GetConfigColor("SETTINGS:OPTIONS_COLOR_SELECTED");
	LocalSettings.OptionsColorSelectedActive = GetConfigColor("SETTINGS:OPTIONS_COLOR_SELECTED_ACTIVE");
	LocalSettings.OptionsColorActive = GetConfigColor("SETTINGS:OPTIONS_COLOR_ACTIVE");
	GetConfigPair("SETTINGS:LIST_XY", &LocalSettings.ListX, &LocalSettings.ListY);
	LocalSettings.ListLines = m_Settings->GetInteger("SETTINGS:LIST_LINES", 22);
	LocalSettings.ListLinespace = m_Settings->GetInteger("SETTINGS:LIST_LINESPACE", 8);
	LocalSettings.ListMaxChars = m_Settings->GetInteger("SETTINGS:LIST_MAX_CHARS", 40);
	LocalSettings.ListColorSelected = GetConfigColor("SETTINGS:LIST_COLOR_SELECTED");
	LocalSettings.ListColorPlaying = GetConfigColor("SETTINGS:LIST_COLOR_PLAYING");
	LocalSettings.ListColorNotSelected = GetConfigColor("SETTINGS:LIST_COLOR_NOT_SELECTED");
	LocalSettings.ShowFileExtension = m_Settings->GetInteger("SETTINGS:ShowFileExtension", 0);
	GetConfigPair("SETTINGS:SONGTITLE_XY", &LocalSettings.SongTitleX, &LocalSettings.SongTitleY);
	LocalSettings.SongTitleWidth = m_Settings->GetInteger("SETTINGS:SONGTITLE_WIDTH", 236);
	LocalSettings.SongTitleColor = GetConfigColor("SETTINGS:SONGTITLE_COLOR");
	GetConfigPair("SETTINGS:BUFFER_XY", &LocalSettings.BufferX, &LocalSettings.BufferY);
	GetConfigPair("SETTINGS:BITRATE_XY", &LocalSettings.BitrateX, &LocalSettings.BitrateY);
	GetConfigPair("SETTINGS:USB_ICON_XY", &LocalSettings.USBIconX, &LocalSettings.USBIconY);
	GetConfigPair("SETTINGS:PLAYER_STATE_XY", &LocalSettings.PlayerstateX, &LocalSettings.PlayerstateY);
	LocalSettings.ErrorTextColor = GetConfigColor("SETTINGS:ERROR_TEXT_COLOR");
	LocalSettings.MessageTextColor = GetConfigColor("SETTINGS:MESSAGE_TEXT_COLOR");
	LocalSettings.ErrorWindowColor = GetConfigColor("SETTINGS:ERROR_WINDOW_COLOR");
	LocalSettings.MessageWindowColor = GetConfigColor("SETTINGS:MESSAGE_WINDOW_COLOR");
	GetConfigPair("SETTINGS:VISUALIZER_XY", &LocalSettings.VisualizerX, &LocalSettings.VisualizerY);
	GetConfigPair("SETTINGS:VISUALIZER_WH", &LocalSettings.VisualizerW, &LocalSettings.VisualizerH);

	/* Graphics sizes */
	GetConfigPair("GRAPHICS:GFX_FONT_LIST_SIZE", &GfxSizes.FontWidth_List, &GfxSizes.FontHeight_List);
	GetConfigPair("GRAPHICS:GFX_FONT_SCROLLER_SIZE", &GfxSizes.FontWidth_Scroller, &GfxSizes.FontHeight_Scroller);
	GetConfigPair("GRAPHICS:GFX_FONT_STATIC_SIZE", &GfxSizes.FontWidth_Static, &GfxSizes.FontHeight_Static);
	GetConfigPair("GRAPHICS:GFX_FONT_MESSAGE_SIZE", &GfxSizes.FontWidth_Message, &GfxSizes.FontHeight_Message);
	GetConfigPair("GRAPHICS:GFX_WIFI_SIZE", &GfxSizes.wifi_w, &GfxSizes.wifi_h);
	GfxSizes.wifi_y = m_Settings->GetInteger("GRAPHICS:GFX_WIFI_H", 8);
	GetConfigPair("GRAPHICS:GFX_POWER_SIZE", &GfxSizes.power_w, &GfxSizes.power_h);
	GfxSizes.power_y = m_Settings->GetInteger("GRAPHICS:GFX_POWER_H", 8);
	GetConfigPair("GRAPHICS:GFX_VOLUME_SIZE", &GfxSizes.volume_w, &GfxSizes.volume_h);
	GfxSizes.volume_y = m_Settings->GetInteger("GRAPHICS:GFX_VOLUME_H", 16);
	GetConfigPair("GRAPHICS:GFX_ICONS_SIZE", &GfxSizes.icons_w, &GfxSizes.icons_h);
	GfxSizes.icons_y = m_Settings->GetInteger("GRAPHICS:GFX_ICONS_H", 40);
	GetConfigPair("GRAPHICS:GFX_PROGRESS_SIZE", &GfxSizes.progress_w, &GfxSizes.progress_h);
	GfxSizes.progress_y = m_Settings->GetInteger("GRAPHICS:GFX_PROGRESS_H", 8);
	GetConfigPair("GRAPHICS:GFX_USB_SIZE", &GfxSizes.usb_w, &GfxSizes.usb_h);
	GfxSizes.usb_y = m_Settings->GetInteger("GRAPHICS:GFX_USB_H", 16);
	GetConfigPair("GRAPHICS:GFX_PLAYSTATE_SIZE", &GfxSizes.playstate_w, &GfxSizes.playstate_h);
	GfxSizes.playstate_y = m_Settings->GetInteger("GRAPHICS:GFX_PLAYSTATE_H", 8);
}

void CTextUI3D::PrepareShutdown()
{
	/* Prepare for shutdown -> Don't render anymore */
	ModuleLog(LOG_VERYLOW, "CTextUI3D: preparing for shutdown");
	m_state = CScreenHandler::PSPRADIO_SCREENSHOT_ACTIVE;
}

void CTextUI3D::Terminate()
{
	ModuleLog(LOG_VERYLOW, "CTextUI3D:Terminating");
	sceGuTerm();
}

int CTextUI3D::SetTitle(char *strTitle)
{
	sprintf(m_title, "PSPRadio version : %s", strTitle);
	return 0;
}

int CTextUI3D::DisplayMessage_EnablingNetwork()
{
	m_wmanager.Dispatch(WM_EVENT_NETWORK_ENABLE, NULL);
	return 0;
}

int CTextUI3D::DisplayMessage_DisablingNetwork()
{
	m_wmanager.Dispatch(WM_EVENT_NETWORK_DISABLE, NULL);
	return 0;
}

int CTextUI3D::DisplayMessage_NetworkReady(char *strIP)
{
	m_wmanager.Dispatch(WM_EVENT_NETWORK_IP, NULL);
	return 0;
}

int CTextUI3D::DisplayMainCommands()
{
	return 0;
}

int CTextUI3D::DisplayActiveCommand(CPSPSound::pspsound_state playingstate)
{
	switch(playingstate)
	{
		case CPSPSound::STOP:
			{
			m_wmanager.Dispatch(WM_EVENT_PLAYER_STOP, NULL);
			}
			break;
		case CPSPSound::PAUSE:
			{
			m_wmanager.Dispatch(WM_EVENT_PLAYER_PAUSE, NULL);
			}
			break;
		case CPSPSound::PLAY:
			{
			m_wmanager.Dispatch(WM_EVENT_PLAYER_START, NULL);
			}
			break;
	}
	return 0;
}


int CTextUI3D::DisplayErrorMessage(char *strMsg)
{
	strcpy(m_error, strMsg);
	m_wmanager.Dispatch(WM_EVENT_TEXT_ERROR, m_error);
	return 0;
}

int CTextUI3D::DisplayMessage(char *strMsg)
{
	strcpy(m_message, strMsg);
	m_wmanager.Dispatch(WM_EVENT_TEXT_MESSAGE, m_message);
	return 0;
}

int CTextUI3D::DisplayBufferPercentage(int iPercentage)
{
	m_wmanager.SetBuffer(iPercentage);
	return 0;
}

int CTextUI3D::OnNewStreamStarted()
{
	m_wmanager.Dispatch(WM_EVENT_STREAM_START, NULL);
	return 0;
}

int CTextUI3D::OnStreamOpening()
{
	m_wmanager.Dispatch(WM_EVENT_STREAM_OPEN, NULL);
	return 0;
}

int CTextUI3D::OnConnectionProgress()
{
	m_wmanager.Dispatch(WM_EVENT_STREAM_CONNECTING, NULL);
	return 0;
}

int CTextUI3D::OnStreamOpeningError()
{
	m_wmanager.Dispatch(WM_EVENT_STREAM_ERROR, NULL);
	return 0;
}

int CTextUI3D::OnStreamOpeningSuccess()
{
	m_wmanager.Dispatch(WM_EVENT_STREAM_SUCCESS, NULL);
	return 0;
}

void CTextUI3D::OnScreenshot(CScreenHandler::ScreenShotState state)
{
	m_state = state;
}

int CTextUI3D::OnVBlank()
{
	if (m_state == CScreenHandler::PSPRADIO_SCREENSHOT_NOT_ACTIVE)
	{
		/* Render windows */
		m_wmanager.Dispatch(WM_EVENT_VBLANK, NULL);
	}
	return 0;
}

int CTextUI3D::OnNewSongData(MetaData *pData)
{
	if (strlen(pData->strTitle))
	{
	char	title[1024];

		/* If the artist is known then add it in front of the title */
		if (pData->strArtist && strlen(pData->strArtist))
		{
			sprintf(title, "%s - %s", pData->strArtist, pData->strTitle);
		}
		else
		{
			sprintf(title, "%s", pData->strTitle);
		}

		AddTitleText(LocalSettings.SongTitleX, LocalSettings.SongTitleY, LocalSettings.SongTitleColor, title);

	}
	else
	{
		if (pData->strURL && strlen(pData->strURL))
		{
		AddTitleText(LocalSettings.SongTitleX, LocalSettings.SongTitleY, LocalSettings.SongTitleColor, pData->strURL);
		}
		else
		{
		AddTitleText(LocalSettings.SongTitleX, LocalSettings.SongTitleY, LocalSettings.SongTitleColor, pData->strURI);
		}
	}
	m_wmanager.SetBitrate((pData->iBitRate/1000));

	return 0;
}

int CTextUI3D::OnStreamTimeUpdate(MetaData *pData)
{
	if (pData->lTotalTime > 0)
	{
		m_wmanager.SetPlayTime( (int)(pData->lCurrentTime / 60), (int)(pData->lCurrentTime % 60),
								(int)(pData->lTotalTime / 60), (int)(pData->lTotalTime % 60));
	}
	else
	{
		m_wmanager.SetPlayTime( (int)(pData->lCurrentTime / 60), (int)(pData->lCurrentTime % 60));
	}
	return 0;
}

void CTextUI3D::Initialize_Screen(IScreen *Screen)
{
	/* Pass to WindowManager */
	if (PSPRadioExport_GetProgramVersion())
	{
		SetTitle(PSPRadioExport_GetProgramVersion());
	}

	if (PSPRadioExport_GetMyIP())
	{
		DisplayMessage_NetworkReady(PSPRadioExport_GetMyIP());
	}

	switch ((CScreenHandler::Screen)Screen->GetId())
	{
		case CScreenHandler::PSPRADIO_SCREEN_SHOUTCAST_BROWSER:
			ModuleLog(LOG_VERYLOW, "CTextUI3D:Initialize shoutcast");
			m_wmanager.Dispatch(WM_EVENT_SHOUTCAST, NULL);
			break;
		case CScreenHandler::PSPRADIO_SCREEN_PLAYLIST:
			ModuleLog(LOG_VERYLOW, "CTextUI3D:Initialize playlist");
			m_wmanager.Dispatch(WM_EVENT_PLAYLIST, NULL);
			break;
		case CScreenHandler::PSPRADIO_SCREEN_LOCALFILES:
			ModuleLog(LOG_VERYLOW, "CTextUI3D:Initialize localfiles");
			m_wmanager.Dispatch(WM_EVENT_LOCALFILES, NULL);
			break;
		case CScreenHandler::PSPRADIO_SCREEN_OPTIONS:
			ModuleLog(LOG_VERYLOW, "CTextUI3D:Initialize options");
			m_wmanager.Dispatch(WM_EVENT_OPTIONS, NULL);
			break;
	}
}

void CTextUI3D::OnTimeChange(pspTime *LocalTime)
{
	m_wmanager.SetTime(LocalTime->hour, LocalTime->minutes);
}

void CTextUI3D::OnBatteryChange(int Percentage)
{
	m_wmanager.SetBatteryLevel(Percentage);
}

void CTextUI3D::OnUSBEnable()
{
	m_wmanager.SetUSBState(true);
}

void CTextUI3D::OnUSBDisable()
{
	m_wmanager.SetUSBState(false);
}

void CTextUI3D::NewPCMBuffer(short *PCMBuffer)
{
//	m_wmanager.Dispatch(WM_EVENT_PCM_BUFFER, PCMBuffer);
}

void CTextUI3D::UpdateOptionsScreen(list<OptionsScreen::Options> &OptionsList,
									 list<OptionsScreen::Options>::iterator &CurrentOptionIterator)
{
	bool active_item;
	int y = LocalSettings.OptionsY;
	list<OptionsScreen::Options>::iterator OptionIterator;
	OptionsScreen::Options	Option;
	WindowHandlerHSM::TextItem *new_list;

	ModuleLog(LOG_VERYLOW, "CTextUI3D:Updating options");

	if (OptionsList.size() > 0)
	{
		for (OptionIterator = OptionsList.begin() ; OptionIterator != OptionsList.end() ; OptionIterator++)
		{
			if (OptionIterator == CurrentOptionIterator)
			{
			active_item = true;
			}
			else
			{
			active_item = false;
			}
			Option = (*OptionIterator);
			StoreOption(y, active_item, Option.strName, Option.strStates, Option.iNumberOfStates, Option.iSelectedState, Option.iActiveState);
			y += LocalSettings.OptionsLinespace;
		}
		AddToOptionList(LocalSettings.OptionsX, y, LocalSettings.OptionsColorSelected, m_title);

		new_list = m_option_list;
		m_option_list = m_option_list_tail = NULL;
		m_wmanager.Dispatch(WM_EVENT_OPTIONS_TEXT, new_list);
	}
}

void CTextUI3D::StoreOption(int y, bool active_item, char *strName, char *strStates[], int iNumberOfStates, int iSelectedState, int iActiveState)
{
	int 			x = LocalSettings.OptionsX;
	unsigned int	color = LocalSettings.OptionsColorSelected;

	if (active_item)
	{
		color = LocalSettings.OptionsColorSelected;
	}
	else
	{
		color = LocalSettings.OptionsColorNotSelected;
	}
	/* The ID field are not used on the option screen */
	AddToOptionList(x, y, color, strName);

	if (iNumberOfStates > 0)
	{
		x = LocalSettings.OptionsItemX;
		for (int iStates = 0; iStates < iNumberOfStates ; iStates++)
		{
			if (iStates+1 == iSelectedState)
			{
				if (iStates+1 == iActiveState)
					{
					color =  LocalSettings.OptionsColorSelectedActive;
					}
				else
					{
					color = LocalSettings.OptionsColorSelected;
					}

				/* For the moment only render the selected item .. No space on screen */
				/* The ID field are not used on the option screen */
				AddToOptionList(x, y, color, strStates[iStates]);
				x += (strlen(strStates[iStates])+1) * 8;
				continue;
			}
		}
	}
}

void CTextUI3D::AddToOptionList(int x, int y, unsigned int color, char *text)
{
	WindowHandlerHSM::TextItem			*new_item;

	new_item = (WindowHandlerHSM::TextItem *) malloc(sizeof(WindowHandlerHSM::TextItem));

	new_item->x 		= x;
	new_item->y 		= y;
	new_item->color 	= color;
	new_item->ID 		= 0;
	new_item->next 		= NULL;
	strcpy(new_item->strText, text);
	strupr(new_item->strText);

	if (m_option_list == NULL)
		{
		m_option_list_tail = m_option_list = new_item;
		}
	else
		{
		m_option_list_tail->next = new_item;
		m_option_list_tail = new_item;
		}
}

void CTextUI3D::DisplayContainers(CMetaDataContainer *Container)
{
	int			 	list_cnt, render_cnt;
	int				current = 1;
	int				i = 0;
	unsigned int	color;
	int				y = LocalSettings.ListY;
	int				list_size = LocalSettings.ListLines;
	int				first_entry;
	char 			*strTemp = (char *)malloc (MAXPATHLEN);
	WindowHandlerHSM::TextItem *new_list;


	map< string, list<MetaData>* >::iterator ListIterator;
	map< string, list<MetaData>* >::iterator *CurrentHighlightedElement = Container->GetCurrentContainerIterator();
	map< string, list<MetaData>* >::iterator *CurrentPlayingElement = Container->GetPlayingContainerIterator();

	map< string, list<MetaData>* > *List = Container->GetContainerList();

	list_cnt = List->size();

	/*  Find number of current element */
	if (list_cnt > 0)
	{
		for (ListIterator = List->begin() ; ListIterator != List->end() ; ListIterator++, current++)
		{
			if (ListIterator == *CurrentHighlightedElement)
			{
			break;
			}
		}

		/* Calculate start element in list */
		first_entry = FindFirstEntry(list_cnt, current);

		/* Find number of elements to show */
		render_cnt = (list_cnt > list_size) ? list_size : list_cnt;

		/* Go to start element */
		for (ListIterator = List->begin() ; i < first_entry ; i++, ListIterator++);

		for (i = 0 ; i < list_size ; i++)
		{
			if (i < render_cnt)
			{
				if (ListIterator == *CurrentHighlightedElement)
				{
					color = LocalSettings.ListColorSelected;
				}
				else if (ListIterator == *CurrentPlayingElement)
				{
					color = LocalSettings.ListColorPlaying;
				}
				else
				{
					color = LocalSettings.ListColorNotSelected;
				}
				strlcpy(strTemp, ListIterator->first.c_str(), MAXPATHLEN);
				if (strlen(strTemp) > 4 && memcmp(strTemp, "ms0:", 4) == 0)
				{
					char *pStrTemp = basename(strTemp);
					pStrTemp[LocalSettings.ListMaxChars] = 0;
					AddToListList(LocalSettings.ListX, y, color, pStrTemp);
				}
				else
				{
					strTemp[LocalSettings.ListMaxChars] = 0;
					AddToListList(LocalSettings.ListX, y, color, strTemp);
				}
				y += LocalSettings.ListLinespace;
				ListIterator++;
			}
		}
		new_list = m_list_list;
		m_list_list = m_list_list_tail = NULL;
		m_wmanager.Dispatch(WM_EVENT_LIST_TEXT, new_list);
	}
	free (strTemp);
}

void CTextUI3D::AddToListList(int x, int y, unsigned int color, char *text)
{
	WindowHandlerHSM::TextItem			*new_item;

	new_item = (WindowHandlerHSM::TextItem *) malloc(sizeof(WindowHandlerHSM::TextItem));

	new_item->x 		= x;
	new_item->y 		= y;
	new_item->color 	= color;
	new_item->ID 		= 0;
	new_item->next 		= NULL;
	strcpy(new_item->strText, text);
	strupr(new_item->strText);

	if (m_list_list == NULL)
		{
		m_list_list_tail = m_list_list = new_item;
		}
	else
		{
		m_list_list_tail->next = new_item;
		m_list_list_tail = new_item;
		}
}

void CTextUI3D::DisplayElements(CMetaDataContainer *Container)
{
	int			 	list_cnt, render_cnt;
	int				current = 1;
	int				i = 0;
	unsigned int	color;
	int				y = LocalSettings.ListY;
	int				list_size = LocalSettings.ListLines;
	int				first_entry;
	char 			*strTemp = (char *)malloc (MAXPATHLEN+1);
	WindowHandlerHSM::TextItem *new_list;


	list<MetaData>::iterator ListIterator;
	list<MetaData>::iterator *CurrentHighlightedElement = Container->GetCurrentElementIterator();
	list<MetaData>::iterator *CurrentPlayingElement = Container->GetPlayingElementIterator();
	list<MetaData> *List = Container->GetElementList();


	list_cnt = List->size();

	/*  Find number of current element */
	if (list_cnt > 0)
	{
		for (ListIterator = List->begin() ; ListIterator != List->end() ; ListIterator++, current++)
		{
			if (ListIterator == *CurrentHighlightedElement)
			{
			break;
			}
		}

		/* Calculate start element in list */
		first_entry = FindFirstEntry(list_cnt, current);

		/* Find number of elements to show */
		render_cnt = (list_cnt > list_size) ? list_size : list_cnt;

		/* Go to start element */
		for (ListIterator = List->begin() ; i < first_entry ; i++, ListIterator++);

		for (i = 0 ; i < list_size ; i++)
		{
			if (i < render_cnt)
			{
				if (ListIterator == *CurrentHighlightedElement)
				{
					color = LocalSettings.ListColorSelected;
				}
				else if (ListIterator == *CurrentPlayingElement)
				{
					color = LocalSettings.ListColorPlaying;
				}
				else
				{
					color = LocalSettings.ListColorNotSelected;
				}

				if (strlen((*ListIterator).strTitle))
				{
					strlcpy(strTemp, (*ListIterator).strTitle, MAXPATHLEN);
				}
				else
				{
					strlcpy(strTemp, (*ListIterator).strURI, MAXPATHLEN);
				}

				if (strlen(strTemp) > 4 && memcmp(strTemp, "ms0:", 4) == 0)
				{
					char *pStrTemp = basename(strTemp);
					if (0 == LocalSettings.ShowFileExtension)
					{
						char *ext = strrchr(pStrTemp, '.');
						if(ext)
						{
							ext[0] = 0;
						}
					}
					pStrTemp[LocalSettings.ListMaxChars] = 0;
					AddToEntryList(LocalSettings.ListX, y, color, pStrTemp);
				}
				else
				{
					strTemp[LocalSettings.ListMaxChars] = 0;
					AddToEntryList(LocalSettings.ListX, y, color, strTemp);
				}

				y += LocalSettings.ListLinespace;
				ListIterator++;
			}
		}
		new_list = m_entry_list;
		m_entry_list = m_entry_list_tail = NULL;
		m_wmanager.Dispatch(WM_EVENT_ENTRY_TEXT, new_list);
	}
	free (strTemp);
}

void CTextUI3D::AddToEntryList(int x, int y, unsigned int color, char *text)
{
	WindowHandlerHSM::TextItem			*new_item;

	new_item = (WindowHandlerHSM::TextItem *) malloc(sizeof(WindowHandlerHSM::TextItem));

	new_item->x 		= x;
	new_item->y 		= y;
	new_item->color 	= color;
	new_item->ID 		= 0;
	new_item->next 		= NULL;
	strcpy(new_item->strText, text);
	strupr(new_item->strText);

	if (m_entry_list == NULL)
		{
		m_entry_list_tail = m_entry_list = new_item;
		}
	else
		{
		m_entry_list_tail->next = new_item;
		m_entry_list_tail = new_item;
		}
}

int CTextUI3D::FindFirstEntry(int list_cnt, int current)
{
	int		first_entry;
	int		list_size = LocalSettings.ListLines;
	int		list_margin = (((list_size-1) / 2) + 1);

	/* Handle start of list */
	if ((list_cnt <= list_size) || (current < list_margin))
	{
		first_entry = 0;
	}
	/* Handle end of list */
	else if ((list_cnt > list_size) && ((list_cnt - current) < list_margin))
	{
		first_entry = list_cnt - list_size;
	}
	/* Handle rest of list */
	else
	{
		first_entry = current - list_margin;
	}

	return first_entry;
}

void CTextUI3D::OnCurrentContainerSideChange(CMetaDataContainer *Container)
{
	/* Pass to WindowManager */

	switch (Container->GetCurrentSide())
	{
		case	CMetaDataContainer::CONTAINER_SIDE_CONTAINERS:
			m_wmanager.Dispatch(WM_EVENT_SELECT_LIST, NULL);
			break;
		case	CMetaDataContainer::CONTAINER_SIDE_ELEMENTS:
			m_wmanager.Dispatch(WM_EVENT_SELECT_ENTRIES, NULL);
			break;
		default:
			break;
	}
}

void CTextUI3D::AddTitleText(int x, int y, unsigned int color, char *text)
{
	WindowHandlerHSM::TextItem	*item;

	item = (WindowHandlerHSM::TextItem *) malloc(sizeof(WindowHandlerHSM::TextItem));
	item->x 	= x;
	item->y 	= y;
	item->color 	= color;
	strcpy(item->strText, text);
	strupr(item->strText);
	item->ID = 0;
	m_wmanager.Dispatch(WM_EVENT_TEXT_SONGTITLE, item);
}
