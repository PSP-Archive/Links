#ifndef _PSPRADIOGRAPHICSUI_
#define _PSPRADIOGRAPHICSUI_

#include "IPSPRadio_UI.h"
#include "GraphicsUITheme.h"

class CGraphicsUI : public IPSPRadio_UI
{
public:
	CGraphicsUI();
	~CGraphicsUI();
	
public:
	int Initialize(char *strCWD);
	void Terminate();

	int SetTitle(char *strTitle);
	int DisplayMessage_EnablingNetwork();
	int DisplayMessage_DisablingNetwork();
	int DisplayMessage_NetworkReady(char *strIP);
	int DisplayMessage_NetworkSelection(int iProfileID, char *strProfileName);
	int DisplayMainCommands();
	int DisplayActiveCommand(CPSPSound::pspsound_state playingstate);
	int DisplayErrorMessage(char *strMsg);
	int DisplayBufferPercentage(int a);

	/** these are listed in sequential order */
	int OnNewStreamStarted();
	int OnStreamOpening();
	int OnConnectionProgress();
	int OnStreamOpeningError();
	int OnStreamOpeningSuccess();
	int OnVBlank();
	int OnNewSongData(MetaData *pData);	
	void DisplayContainers(CMetaDataContainer *Container);
	void DisplayElements(CMetaDataContainer *Container);
	void OnCurrentContainerSideChange(CMetaDataContainer *Container){return;};

	/** Screen Handling */
	void Initialize_Screen(IScreen *Screen);
	void UpdateOptionsScreen(list<OptionsScreen::Options> &OptionsList, 
										 list<OptionsScreen::Options>::iterator &CurrentOptionIterator);
		
private:

	void PrintOption(int nLineNumber, char *strName, char *strStates[], int iNumberOfStates, int iSelectedState, 
					int iActiveState);
	
private:
	CGraphicsUITheme m_theme;	
};

#endif
