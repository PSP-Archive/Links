#ifndef __PSPTHEMEWRAPPER__
#define __PSPTHEMEWRAPPER__

#include "GraphicsUITheme.h"

class CPSPThemeTool
{
public:
	CPSPThemeTool(void);
	~CPSPThemeTool(void);

	bool Initialize(char *szThemeFileName);
	bool Terminate(void);
	bool Run(void);

private:
	CGraphicsUITheme m_GUI;
	SDL_TimerID m_TimerID;
};

#endif
