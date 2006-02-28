#ifndef _PSPRADIOGRAPHICSUIDEFINES_
#define _PSPRADIOGRAPHICSUIDEFINES_

//*****************************************************************************
// DEFINES
//*****************************************************************************
#define SAFE_FREE_SURFACE(surface) if(NULL != surface) { SDL_FreeSurface(surface); surface = NULL; }
#define SAFE_DELETE(obj) if(NULL != obj) { delete obj; obj = NULL; }
#define SAFE_FREE(obj) if(NULL != obj) { free(obj); obj = NULL; }
#define REMOVE_TIMER(obj) if(0 != obj) { SDL_RemoveTimer(obj); obj = 0; }

//*****************************************************************************
// ENUMERATIONS
//*****************************************************************************
enum ScreenTypeEnum
{
	ST_MAINSCREEN = 0,
	ST_SHOUTCASTSCREEN,
	ST_SETTINGSSCREEN,
	
	ST_ITEM_COUNT
};

enum StringJustEnum
{
	JUST_LEFT = 0,
	JUST_RIGHT,
	JUST_CENTER,

	JUST_ITEM_COUNT
};

enum OutputAreaEnum
{
	OA_PLAYLIST = 0,
	OA_PLAYLISTITEM,
	OA_SETTINGS,

	OA_ITEM_COUNT
};

enum StringPosEnum
{
	SP_META_FILETITLE = 0,
	SP_META_URI,
	SP_META_URL,
	SP_META_SAMPLERATE,
	SP_META_MPEGLAYER,
	SP_META_GENRE,
	SP_META_SONGAUTHOR,
	SP_META_LENGTH,
	SP_META_BITRATE,
	SP_META_CHANNELS,
	SP_ERROR,

	SP_ITEM_COUNT
};

enum ButtonPosEnum
{
	BP_PLAY = 0,
	BP_PAUSE,
	BP_STOP,
	BP_LOAD,
	BP_SOUND,
	BP_VOLUME,
	BP_BUFFER,
	BP_NETWORK,
	BP_STREAM,

	BP_ITEM_COUNT
};

//*****************************************************************************
// Structures
//*****************************************************************************
struct StringPosType
{
	char szIniName[50];
	char *szStringValue;
	SDL_Rect rectPos;
	bool bEnabled;
	StringJustEnum fontJust;
	int nFontIndex;
	bool bRotate;
	int nCurrentXPos;
	int nCurrentYPos;
	SDL_Surface *pSurface;
	SDL_TimerID nTimerID;
	int nCurrentMovement;
};

struct ButtonPosType
{
	char szIniName[50];
	SDL_Rect *pSrcRect;
	SDL_Rect dstRect;
	int nCurrentState;
	bool bEnabled;
	int nButtonCount;
};

struct OutputAreaType
{
	char szIniName[50];
	int nLineCount;
	SDL_Rect srcRect;
	SDL_Rect dstRect;
	SDL_Rect lineSize;
	SDL_Rect selectorPos;
	bool bHasSelector;
	bool bEnabled;
	int nFontIndex;
};

#endif
