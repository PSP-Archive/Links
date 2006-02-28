#ifndef _PSPRADIOTEXTUI_
#define _PSPRADIOTEXTUI_

#include "IPSPRadio_UI.h"
#include "PSPRadio_Exports.h"
#include "Screen.h"

struct screenconfig
{
	CScreen::textmode FontMode;
	int FontWidth;
	int FontHeight;
	char *strBackground;
	int BgColor;
	int FgColor;
	int ContainerListRangeX1, ContainerListRangeX2, ContainerListRangeY1, ContainerListRangeY2;
	int EntriesListRangeX1, EntriesListRangeX2, EntriesListRangeY1, EntriesListRangeY2;
	int ContainerListTitleX, ContainerListTitleY, ContainerListTitleLen;
	int ContainerListTitleUnselectedColor, ContainerListTitleSelectedColor;
	char *strContainerListTitleUnselected, *strContainerListTitleSelected;
	int EntriesListTitleX, EntriesListTitleY, EntriesListTitleLen;
	int EntriesListTitleUnselectedColor, EntriesListTitleSelectedColor;
	char *strEntriesListTitleUnselected, *strEntriesListTitleSelected;
	int BufferPercentageX, BufferPercentageY, BufferPercentageColor;
	int MetadataX1, MetadataLength, MetadataRangeY1, MetadataRangeY2, MetadataColor, MetadataTitleColor;
	int ListsTitleColor;
	int EntriesListColor, SelectedEntryColor, PlayingEntryColor;
	int ProgramVersionX, ProgramVersionY, ProgramVersionColor;
	int StreamOpeningX, StreamOpeningY, StreamOpeningColor;
	int StreamOpeningErrorX, StreamOpeningErrorY, StreamOpeningErrorColor;
	int StreamOpeningSuccessX, StreamOpeningSuccessY, StreamOpeningSuccessColor;
	int CleanOnNewStreamRangeY1, CleanOnNewStreamRangeY2;
	int ActiveCommandX, ActiveCommandY, ActiveCommandColor;
	int ErrorMessageX, ErrorMessageY, ErrorMessageColor;
	int NetworkEnablingX, NetworkEnablingY;
	int NetworkDisablingX, NetworkDisablingY;
	int NetworkReadyX, NetworkReadyY;
	int NetworkEnablingColor,NetworkDisablingColor, NetworkReadyColor;
	int ClockX, ClockY, ClockColor, ClockFormat;
	int BatteryX, BatteryY, BatteryColor;
	int TimeX, TimeY, TimeColor;
	
};

class CTextUI : public IPSPRadio_UI
{
public:
	CTextUI();
	~CTextUI();
	
public:
	int Initialize(char *strCWD);
	void Terminate();

	int SetTitle(char *strTitle);
	int DisplayMessage_EnablingNetwork();
	int DisplayMessage_DisablingNetwork();
	int DisplayMessage_NetworkReady(char *strIP);
	int DisplayMainCommands();
	int DisplayActiveCommand(CPSPSound::pspsound_state playingstate);
	int DisplayErrorMessage(char *strMsg);
	int DisplayMessage(char *strMsg);
	int DisplayBufferPercentage(int a);
	int OnVBlank();

	/** these are listed in sequential order */
	int OnNewStreamStarted();
	int OnStreamOpening();
	int OnConnectionProgress();
	int OnStreamOpeningError();
	int OnStreamOpeningSuccess();
	int OnNewSongData(MetaData *pData);
	int OnStreamTimeUpdate(MetaData *pData);
	
	/** Screen Handling */
	void Initialize_Screen(IScreen *Screen);
	void UpdateOptionsScreen(list<OptionsScreen::Options> &OptionsList, 
										 list<OptionsScreen::Options>::iterator &CurrentOptionIterator);

	void DisplayContainers(CMetaDataContainer *Container);
	void DisplayElements(CMetaDataContainer *Container);
	void OnCurrentContainerSideChange(CMetaDataContainer *Container);
	
	void OnBatteryChange(int Percentage);
	void OnTimeChange(pspTime *LocalTime);
	
private:
	bool  m_isdirty;
	CLock *m_lockprint;
	CLock *m_lockclear;
	CIniParser *m_Config;
	char  *m_strTitle;
	CScreen *m_Screen;
	CScreenHandler::Screen m_CurrentScreen;
	screenconfig m_ScreenConfig;
	int 	m_LastBatteryPercentage;
	pspTime m_LastLocalTime;
	char *m_strCWD;
	
	//helpers
	enum uicolors
	{
		COLOR_BLACK = 0x00000000,
		COLOR_WHITE = 0x00FFFFFF,
		COLOR_RED   = 0x000000FF,
		COLOR_GREEN = 0x0000FF00,
		COLOR_BLUE  = 0x00FF0000,
		COLOR_CYAN  = 0x00AABB00,
		COLOR_YELLOW= 0x00559999
	};
	void uiPrintf(int x, int y, int color, char *strFormat, ...);
	void ClearRows(int iRowStart, int iRowEnd = -1);
	void ClearHalfRows(int iColStart, int iColEnd, int iRowStart, int iRowEnd = -1);

	int ClearErrorMessage();
	int GetConfigColor(char *strKey);
	void GetConfigPair(char *strKey, int *x, int *y);
	
	void PrintOption(int x, int y, int c, char *strName, char *strStates[], int iNumberOfStates, int iSelectedState, 
					int iActiveState);
	void LoadConfigSettings(IScreen *Screen);
};



#endif
