#ifndef psp_h
	#define psp_h

	#include <unistd.h>

	#ifndef tBoolean
	#define tBoolean int
	#define truE 1
	#define falsE 0
	#define KEY_BACKSPACE 8
	#endif
		

	#define PSP_SCREEN_WIDTH 480
	#define PSP_SCREEN_HEIGHT 272
	#define PSP_LINE_SIZE 512
	#define PSP_PIXEL_FORMAT PSP_DISPLAY_PIXEL_FORMAT_8888
	//#define PSP_PIXEL_FORMAT PSP_DISPLAY_PIXEL_FORMAT_565

	extern volatile tBoolean g_PSPEnableRendering;
	extern volatile tBoolean g_PSPEnableInput;

	/* PSP Specific config items */
	extern tBoolean g_PSPConfig_BB_TO_FB_FACTOR;


	typedef struct _extension_download_dirs 
	{
		char music[MAXPATHLEN+1];
		char mp4[MAXPATHLEN+1];
		char avcmp4[MAXPATHLEN+1];
		char videos[MAXPATHLEN+1];
		char images[MAXPATHLEN+1];
		char other[MAXPATHLEN+1];
	}extension_download_dirs;

	extern extension_download_dirs ext_dl_dir;

#endif
