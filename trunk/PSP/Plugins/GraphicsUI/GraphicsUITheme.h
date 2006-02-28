#ifndef _PSPRADIOGRAPHICSUITHEME_
#define _PSPRADIOGRAPHICSUITHEME_

#include <map>
#ifndef WIN32
	#include <SDL/SDL.h>
#else
	#include <SDL.h>
#endif

#include <Logging.h>
#include <iniparser.h>
#include "GraphicsUIDefines.h"

class CGraphicsUITheme
{
public:
	CGraphicsUITheme();
	~CGraphicsUITheme();
	
	int Initialize(char *szThemeFileName, bool bFullScreen=true);
	void Terminate();	

	void DisplayMainScreen();
	void DisplaySettingScreen();
	void DisplayShoutcastScreen();
	void DisplayString(char *szWord, StringPosEnum posEnum);
	void DisplayString(char *szWord, OutputAreaEnum posEnum, int nLineNumber, bool bHighlight=false);
	void ResetOutputArea(OutputAreaEnum posEnum);

	void DisplayButton(ButtonPosEnum posEnum);
	void DisplayButton(ButtonPosEnum posEnum, int nState);
	void OnVBlank();

	int GetLineCount(OutputAreaEnum posEnum);
	int GetButtonStateCount(ButtonPosEnum posEnum);

	
	void UpdateStringSurface(StringPosType *pPos);
	
	void LogError(char *szFormat, ...);
	void LogInfo(char *szFormat, ...);

private:
	int GetFonts();
	int GetStringPos();
	int GetButtonPos();
	int GetOutputAreaPos();
	
	int GetIniRect(char *szIniTag, SDL_Rect *pRect);
	int GetIniColor(char *szIniTag, SDL_Color *pColor);
	int GetIniStringPos(char *szIniTag, StringPosType *pPos);

	int StringToRect(char *szRect, SDL_Rect *pSdlRect);
	int StringToPoint(char *szPair, int *pItem1, int *pItem2);
	int StringToPoint(char *szPair, int *pItem1, int *pItem2, int *pItem3);
	int StringToStringPos(char *szPos, StringPosType *pPos);

	void DisplayStringSurface(StringPosType *pPos);
	SDL_Surface *GetStringSurface(char *szWord, int nFontIndex);

private:
	CIniParser *m_pIniTheme;
	char *m_szMsg;
	int m_nColorDepth;
	int m_nFlags;
	int m_nPSPResWidth;
	int m_nPSPResHeight;
	SDL_Surface *m_pPSPSurface;
	SDL_Surface *m_pImageSurface;

	std::map<char, SDL_Rect> m_FontMap[5];
	SDL_Rect m_FontSize[5];

	SDL_Rect m_CurrentScreenSrc;
	SDL_Color m_TransparencyColor;
};

#endif
