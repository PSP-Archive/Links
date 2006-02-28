#include "ThemeTool.h"
#include "Logging.h"

Uint32 OnVBlank_TimerCallback(Uint32 interval, void *param);

CPSPThemeTool::CPSPThemeTool(void)
{
	m_TimerID = 0;
}

CPSPThemeTool::~CPSPThemeTool(void)
{
	Terminate();
}

bool CPSPThemeTool::Initialize(char *szThemeFileName)
{
	if(0 != m_GUI.Initialize(szThemeFileName, false))
	{
		m_GUI.LogError("Error initilizing theme");
		return false;
	}

	m_TimerID = SDL_AddTimer(10, OnVBlank_TimerCallback, NULL);

	if(0 == m_TimerID)
	{
		m_GUI.LogError("Error initilizing timer [%s]", SDL_GetError());
		return false;
	}

	m_GUI.DisplayMainScreen();

	return true;
}

bool CPSPThemeTool::Terminate(void)
{
	SDL_RemoveTimer(m_TimerID);

	m_GUI.Terminate();
	return true;
}

bool CPSPThemeTool::Run(void)
{
	SDL_Event event;

	while(SDL_WaitEvent(&event) >= 0) 
	{
		switch (event.type) 
		{
			case SDL_USEREVENT:
			{
				m_GUI.OnVBlank();
			} break;

			case SDL_ACTIVEEVENT: 
			{
			} break;

			case SDL_KEYDOWN:
			{
				switch(event.key.keysym.sym)
				{
					case SDLK_F1: 
					{
						m_GUI.DisplayMainScreen();
					} break;

					case SDLK_F2:
					{
						m_GUI.DisplayShoutcastScreen();
					} break;

					case SDLK_F3:
					{
						m_GUI.DisplaySettingScreen();
					} break;

					case SDLK_1:
					{
						m_GUI.DisplayButton(BP_PLAY);
					} break;

					case SDLK_2:
					{
						m_GUI.DisplayButton(BP_PAUSE);
					} break;

					case SDLK_3:
					{
						m_GUI.DisplayButton(BP_STOP);
					} break;

					case SDLK_4:
					{
						m_GUI.DisplayButton(BP_LOAD);
					} break;

					case SDLK_5:
					{
						m_GUI.DisplayButton(BP_SOUND);
					} break;

					case SDLK_6:
					{
						m_GUI.DisplayButton(BP_VOLUME);
					} break;

					case SDLK_7:
					{
						m_GUI.DisplayButton(BP_BUFFER);
					} break;

					case SDLK_8:
					{
						m_GUI.DisplayButton(BP_STREAM);
					} break;

					case SDLK_9:
					{
						m_GUI.DisplayButton(BP_NETWORK);
					} break;

					case SDLK_0:
					{
					} break;


					case SDLK_a:
					{
						m_GUI.DisplayString("-- -- -- -- -- -- -- -- -- -- -- -- File Title -- -- -- -- -- -- -- -- -- -- -- --", SP_META_FILETITLE);
						m_GUI.DisplayString("-- Uri --",SP_META_URI);
						m_GUI.DisplayString("-- Url --",SP_META_URL);
						m_GUI.DisplayString("-- Sample Rate --",SP_META_SAMPLERATE);
						m_GUI.DisplayString("-- MPEG Layer --",SP_META_MPEGLAYER);
						m_GUI.DisplayString("-- Genre --",SP_META_GENRE);
						m_GUI.DisplayString("-- Author --",SP_META_SONGAUTHOR);
						m_GUI.DisplayString("-- Length --",SP_META_LENGTH);
						m_GUI.DisplayString("-- Bitrate --",SP_META_BITRATE);
						m_GUI.DisplayString("-- Channels --",SP_META_CHANNELS);
						m_GUI.DisplayString("-- This Is A Sample Error Message --",SP_ERROR);
						
					} break;

					case SDLK_b:
					{
						for(int x = 0; x < m_GUI.GetLineCount(OA_PLAYLIST); x++)
						{
							m_GUI.DisplayString("Play List Text 1", OA_PLAYLIST, x+1);
						}
					} break;

					case SDLK_c:
					{
						for(int x = 0; x < m_GUI.GetLineCount(OA_PLAYLISTITEM); x++)
						{
							m_GUI.DisplayString("Play List Item Text 1", OA_PLAYLISTITEM, x+1);
						}
					} break;

					case SDLK_d:
					{
						for(int x = 0; x < m_GUI.GetLineCount(OA_SETTINGS); x++)
						{
							m_GUI.DisplayString("OA_SETTINGS Settings go here", OA_SETTINGS, x+1);
						}
					} break;
					
					default:
					{
					} break;

				}

			} break;

			case SDL_QUIT: 
			{
				return true;
			} break;
			
			default:
			{
			} break;
		}
	}

	return false;
}

Uint32 OnVBlank_TimerCallback(Uint32 interval, void *param)
{
	SDL_Event event;
	SDL_UserEvent userevent;

	userevent.type = SDL_USEREVENT;
	userevent.code = 0;
	userevent.data1 = NULL;
	userevent.data2 = NULL;

	event.type = SDL_USEREVENT;
	event.user = userevent;

	SDL_PushEvent(&event);
	return(interval);
}

int main(int argc, char *argv[])
{
	CPSPThemeTool psp;
	char szThemeFile[255];
		
	if(argc <= 1)
	{
		strcpy(szThemeFile, "THEME/PSPRadio_AllStates.theme");
	}
	else
	{
		strcpy(szThemeFile, argv[1]);
	}
	
	Logging.Set("ThemeTool.log", LOG_VERYLOW);

	if(false == psp.Initialize("THEME/PSPRadio_AllStates.theme"))
	{
		printf("Error initializing PSPWrapper\n");
	}

	psp.Run();

	return 1;
}
