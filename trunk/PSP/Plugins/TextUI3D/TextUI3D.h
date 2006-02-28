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
#ifndef _PSPRADIOTEXTUI3D_
#define _PSPRADIOTEXTUI3D_

#include "IPSPRadio_UI.h"
#include "PSPRadio_Exports.h"
#include "jsaVRAMManager.h"
#include "TextUI3D_WindowManager_HSM.h"

 	struct Settings
		{
		int EventThreadPrio;
		int VersionX, VersionY;
		int IPX, IPY;
		int BatteryIconX, BatteryIconY;
		int WifiIconX, WifiIconY;
		int ListIconX, ListIconY;
		int VolumeIconX, VolumeIconY;
		int	ClockX, ClockY;
		int ClockFormat;
		int PlayTimeX, PlayTimeY;
		int OptionsX, OptionsY;
		int OptionsItemX;
		int	OptionsLinespace;
		int OptionsColorNotSelected;
		int OptionsColorSelected;
		int OptionsColorSelectedActive;
		int OptionsColorActive;
		int ProgressBarX, ProgressBarY;
		int ListX, ListY;
		int ListLines;
		int ListLinespace;
		int ListMaxChars;
		int ListColorSelected;
		int ListColorPlaying;
		int ListColorNotSelected;
		int ShowFileExtension;
		int SongTitleX, SongTitleY;
		int SongTitleWidth;
		int SongTitleColor;
		int BufferX, BufferY;
		int BitrateX, BitrateY;
		int USBIconX, USBIconY;
		int PlayerstateX, PlayerstateY;
		int ErrorTextColor;
		int MessageTextColor;
		int ErrorWindowColor;
		int MessageWindowColor;
		int VisualizerX, VisualizerY;
		int VisualizerW, VisualizerH;
		};

	struct gfx_sizes
		{
		int FontWidth_List, FontHeight_List;
		int FontWidth_Scroller, FontHeight_Scroller;
		int FontWidth_Static, FontHeight_Static;
		int FontWidth_Message, FontHeight_Message;
		int	wifi_w, wifi_h;
		int wifi_y;
		int	power_w, power_h;
		int power_y;
		int	volume_w, volume_h;
		int volume_y;
		int	icons_w, icons_h;
		int icons_y;
		int	progress_w, progress_h;
		int progress_y;
		int	usb_w, usb_h;
		int usb_y;
		int	playstate_w, playstate_h;
		int playstate_y;
		};

class CTextUI3D : public IPSPRadio_UI
{
public:

	struct Vertex
	{
		float u, v;
		unsigned int color;
		float x,y,z;
	};

public:
	CTextUI3D();
	~CTextUI3D();

public:
	int Initialize(char *strCWD);
	void Terminate();
	void PrepareShutdown();

	int SetTitle(char *strTitle);
	int DisplayMessage_EnablingNetwork();
	int DisplayMessage_DisablingNetwork();
	int DisplayMessage_NetworkReady(char *strIP);
	int DisplayMainCommands();
	int DisplayActiveCommand(CPSPSound::pspsound_state playingstate);
	int DisplayErrorMessage(char *strMsg);
	int DisplayBufferPercentage(int a);
	int DisplayMessage(char *strMsg);

	/** these are listed in sequential order */
	int OnNewStreamStarted();
	int OnStreamOpening();
	int OnConnectionProgress();
	int OnStreamOpeningError();
	int OnStreamOpeningSuccess();
	int OnVBlank();
	int OnNewSongData(MetaData *pData);
	int OnStreamTimeUpdate(MetaData *pData);
	void DisplayContainers(CMetaDataContainer *Container);
	void DisplayElements(CMetaDataContainer *Container);
	void OnCurrentContainerSideChange(CMetaDataContainer *Container);
	void OnTimeChange(pspTime *LocalTime);
	void OnBatteryChange(int Percentage);
	void OnUSBEnable();
	void OnUSBDisable();
	void NewPCMBuffer(short *PCMBuffer);

	/** Screen Handling */
	void Initialize_Screen(IScreen *Screen);
	void UpdateOptionsScreen(list<OptionsScreen::Options> &OptionsList,
										 list<OptionsScreen::Options>::iterator &CurrentOptionIterator);
	void OnScreenshot(CScreenHandler::ScreenShotState state);

private:
	void GetSettings();
	void GetConfigPair(char *strKey, int *x, int *y);
	int GetConfigColor(char *strKey);
	int FindFirstEntry(int list_cnt, int current);
	void StoreOption(int y, bool active_item, char *strName, char *strStates[], int iNumberOfStates, int iSelectedState, int iActiveState);
	void AddToOptionList(int x, int y, unsigned int color, char *text);
	void AddToListList(int x, int y, unsigned int color, char *text);
	void AddToEntryList(int x, int y, unsigned int color, char *text);
	void AddTitleText(int x, int y, unsigned int color, char *text);

private:
	CScreenHandler::ScreenShotState 	m_state;
	WindowHandlerHSM					m_wmanager;
	CIniParser 							*m_Settings;
	char								m_title[256];
	char								m_error[256];
	char								m_message[256];
	char								m_playtime[64];
	WindowHandlerHSM::TextItem			*m_option_list, *m_option_list_tail;
	WindowHandlerHSM::TextItem			*m_list_list, *m_list_list_tail;
	WindowHandlerHSM::TextItem			*m_entry_list, *m_entry_list_tail;
};

#endif
