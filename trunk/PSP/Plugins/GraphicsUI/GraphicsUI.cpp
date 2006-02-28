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
//#include <list>
#include <PSPApp.h>
#include <PSPSound.h>
#include <Tools.h>
#include <Logging.h>
#include "GraphicsUI.h"

#define THEME_FILE 		"./THEME/PSPRadio_AllStates.theme"

CGraphicsUI::CGraphicsUI()
{
}

CGraphicsUI::~CGraphicsUI()
{
}

int CGraphicsUI::Initialize(char *strCWD)
{	
//	char *szThemeFile = (char *)malloc(strlen(THEME_FILE) + strlen(strCWD));
//	sprintf(szThemeFile, "%s/%s", strCWD, THEME_FILE);
	if(-1 == m_theme.Initialize(THEME_FILE))
	{
		Log(LOG_ERROR, "Initialize: error initializing Theme");
//		SAFE_FREE(szThemeFile);
		return -1;
	}
	
//	SAFE_FREE(szThemeFile);

	return 0;
}


void CGraphicsUI::Terminate()
{
	m_theme.Terminate();
}

void CGraphicsUI::Initialize_Screen(IScreen *Screen)
{
	switch ((CScreenHandler::Screen)Screen->GetId())
	{
		case CScreenHandler::PSPRADIO_SCREEN_SHOUTCAST_BROWSER:
		{
			m_theme.DisplayShoutcastScreen();
		} break;
		
		case CScreenHandler::PSPRADIO_SCREEN_PLAYLIST:
		case CScreenHandler::PSPRADIO_SCREEN_LOCALFILES:
		{
			m_theme.DisplayMainScreen();
		} break;
			
		case CScreenHandler::PSPRADIO_SCREEN_OPTIONS:
		{
			m_theme.DisplaySettingScreen();
		} break;
	}
}

void CGraphicsUI::UpdateOptionsScreen(list<OptionsScreen::Options> &OptionsList, 
										 list<OptionsScreen::Options>::iterator &CurrentOptionIterator)
{
	list<OptionsScreen::Options>::iterator OptionIterator;
	OptionsScreen::Options	Option;
	
	
	if (OptionsList.size() > 0)
	{
		int nLineNumber = 1;
		for (OptionIterator = OptionsList.begin() ; OptionIterator != OptionsList.end() ; OptionIterator++)
		{
			char szTemp[100];
			
			Option = (*OptionIterator);

			if (OptionIterator == CurrentOptionIterator)
			{
				sprintf(szTemp, "[%s]", Option.strName); 
			}
			else
			{
				sprintf(szTemp, " %s ", Option.strName); 
			}
									
			PrintOption(nLineNumber, szTemp, Option.strStates, Option.iNumberOfStates, Option.iSelectedState, 
						Option.iActiveState);
			
			nLineNumber++;
		}
	}
}

void CGraphicsUI::PrintOption(int nLineNumber, char *strName, char *strStates[], int iNumberOfStates, int iSelectedState,
						  int iActiveState)
{	
	if (iNumberOfStates > 0)
	{
		char szLine[100];
		char szOpt[50];

		sprintf(szLine, "%-20s : ", strName); 
		
		for (int iStates = 0; iStates < iNumberOfStates ; iStates++)
		{
			if ((iStates+1 == iActiveState) && (iStates+1 == iSelectedState))
			{
				sprintf(szOpt, "[*%s*]", strStates[iStates]);
				strcat(szLine, szOpt);
			}
			else if (iStates+1 == iActiveState)
			{
				sprintf(szOpt, " *%s* ", strStates[iStates]);
				strcat(szLine, szOpt);
			}
			else if (iStates+1 == iSelectedState) /** 1-based */
			{
				sprintf(szOpt, "[ %s ]", strStates[iStates]);
				strcat(szLine, szOpt);
			}
			else
			{
				sprintf(szOpt, "  %s  ", strStates[iStates]);
				strcat(szLine, szOpt);
			}			
		}

		m_theme.DisplayString(szLine, OA_SETTINGS, nLineNumber);
	}
}

int CGraphicsUI::SetTitle(char *strTitle)
{
	return 0;
}

int CGraphicsUI::DisplayMessage_EnablingNetwork()
{
	m_theme.DisplayButton(BP_NETWORK, 1);
	return 0;
}

int CGraphicsUI::DisplayMessage_NetworkSelection(int iProfileID, char *strProfileName)
{
	m_theme.DisplayButton(BP_NETWORK, 0);
	return 0;
}

int CGraphicsUI::DisplayMessage_DisablingNetwork()
{
	m_theme.DisplayButton(BP_NETWORK, 3);
	return 0;
}

int CGraphicsUI::DisplayMessage_NetworkReady(char *strIP)
{
	m_theme.DisplayButton(BP_NETWORK, 2);
	return 0;
}

int CGraphicsUI::DisplayMainCommands()
{
	return 0;
}

int CGraphicsUI::DisplayActiveCommand(CPSPSound::pspsound_state playingstate)
{
	switch(playingstate)
	{
		case CPSPSound::STOP:
			m_theme.DisplayButton(BP_PLAY, 0);
			m_theme.DisplayButton(BP_PAUSE, 0);
			m_theme.DisplayButton(BP_STOP, 1);
			break;
		
		case CPSPSound::PLAY:
			m_theme.DisplayButton(BP_PLAY, 1);
			m_theme.DisplayButton(BP_PAUSE, 0);
			m_theme.DisplayButton(BP_STOP, 0);
			break;
		
		case CPSPSound::PAUSE:
			m_theme.DisplayButton(BP_PLAY, 0);
			m_theme.DisplayButton(BP_PAUSE, 1);
			m_theme.DisplayButton(BP_STOP, 0);
			break;
	}
	return 0;
}

int CGraphicsUI::DisplayErrorMessage(char *strMsg)
{
	m_theme.DisplayString(strMsg, SP_ERROR);
	return 0;
}

int CGraphicsUI::DisplayBufferPercentage(int iPercentage)
{
	static int nPrevState = -1;

	int nState = (iPercentage * m_theme.GetButtonStateCount(BP_BUFFER)) / 100;

	if(nState != nPrevState)
	{
		m_theme.DisplayButton(BP_BUFFER, nState);
		nPrevState = nState;
	}

	return 0;
}

int CGraphicsUI::OnNewStreamStarted()
{
	m_theme.DisplayButton(BP_STREAM, 3);
	return 0;
}

int CGraphicsUI::OnStreamOpening()
{
	m_theme.DisplayButton(BP_STREAM, 3);
	return 0;
}

int CGraphicsUI::OnStreamOpeningError()
{
	m_theme.DisplayButton(BP_STREAM, 4);
	return 0;
}

int CGraphicsUI::OnStreamOpeningSuccess()
{
	m_theme.DisplayButton(BP_STREAM, 3);
	return 0;
}

int CGraphicsUI::OnVBlank()
{
	m_theme.OnVBlank();
	return 0;
}

int CGraphicsUI::OnNewSongData(MetaData *pData)
{
	char szTmp[25];

	m_theme.DisplayString(pData->strURI, SP_META_URI);
	m_theme.DisplayString(pData->strTitle, SP_META_FILETITLE);
	m_theme.DisplayString(pData->strURL, SP_META_URL);
	m_theme.DisplayString(pData->strArtist, SP_META_SONGAUTHOR);
	m_theme.DisplayString(pData->strGenre, SP_META_GENRE);

	sprintf(szTmp, "%d", pData->iLength);
	m_theme.DisplayString(szTmp, SP_META_LENGTH);
	
	sprintf(szTmp, "%d", pData->iSampleRate);
	m_theme.DisplayString(szTmp, SP_META_SAMPLERATE);
	
	sprintf(szTmp, "%d", pData->iBitRate);
	m_theme.DisplayString(szTmp, SP_META_BITRATE);
	
	sprintf(szTmp, "%d", pData->iNumberOfChannels);
	m_theme.DisplayString(szTmp, SP_META_CHANNELS);

	sprintf(szTmp, "%d", pData->iMPEGLayer);
	m_theme.DisplayString(szTmp, SP_META_MPEGLAYER);
	
	return 0;
}

void CGraphicsUI::DisplayContainers(CMetaDataContainer *Container)
{
	map< string, list<MetaData>* >::iterator dataIter = *Container->GetCurrentContainerIterator();
	char *strText = (char *)malloc (MAXPATHLEN);
	
	int nLineCount = m_theme.GetLineCount(OA_PLAYLIST);

	for (int i = 0; i < nLineCount/2; i++)
	{
		if (dataIter == Container->GetContainerList()->begin())
			break;
		dataIter--;
	}
	
	// Reset playlist area
	m_theme.ResetOutputArea(OA_PLAYLIST);

	for (int nLineNumber = 0; dataIter != Container->GetContainerList()->end() ; dataIter++)
	{
		bool bHighlight = false;

		if (nLineNumber > nLineCount-1)
		{
			break;
		}

		// Highling current item
		if(dataIter == *Container->GetCurrentContainerIterator())
		{
			bHighlight = true;
		}
		
		//m_theme.DisplayString(basename(dataIter->strURI), OA_PLAYLIST, nLineNumber+1, bHighlight);
		strlcpy(strText, dataIter->first.c_str(), MAXPATHLEN - 1);
		
		if ((strlen(strText) > 4) && 
			(0 == memcmp(strText, "ms0:", 4)) )
		{
			char *pText = basename(strText);
			m_theme.DisplayString(pText, OA_PLAYLIST, nLineNumber+1, bHighlight);
		}
		else
		{
			m_theme.DisplayString(strText, OA_PLAYLIST, nLineNumber+1, bHighlight);
		}
		
		nLineNumber++;
	}	

	free (strText), strText = NULL;
}

void CGraphicsUI::DisplayElements(CMetaDataContainer *Container)
{
	int nLineCount = m_theme.GetLineCount(OA_PLAYLISTITEM);
	
	list<MetaData>::iterator dataIter = *Container->GetCurrentElementIterator();
			
	for (int i = 0; i < nLineCount/2; i++)
	{
		if (dataIter == Container->GetElementList()->begin())
			break;
		dataIter--;
	}

	// Reset playlist area
	m_theme.ResetOutputArea(OA_PLAYLISTITEM);
	
	for (int nLineNumber = 0; dataIter != Container->GetElementList()->end() ; dataIter++)
	{
		bool bHighlight = false;

		if (nLineNumber > nLineCount-1)
		{
			break;
		}

		// Highlight item
		if(dataIter == *Container->GetCurrentElementIterator())
		{
			bHighlight = true;
		}


		if(0 < strlen(dataIter->strTitle))
		{
			m_theme.DisplayString(dataIter->strTitle, OA_PLAYLISTITEM, nLineNumber+1, bHighlight);
		}
		else
		{
			m_theme.DisplayString(dataIter->strURI, OA_PLAYLISTITEM, nLineNumber+1, bHighlight);
		}

		
		nLineNumber++;
	}	
}

int CGraphicsUI::OnConnectionProgress()
{
	return 0;
}
