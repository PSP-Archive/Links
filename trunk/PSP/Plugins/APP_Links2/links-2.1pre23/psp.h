#ifndef psp_h
	#define psp_h

	#include <psptypes.h>
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

	/** If commented out, then 32bpp are used. */
	#define PSP_16BPP

	#ifdef PSP_16BPP
		#define PSP_PIXEL_FORMAT PSP_DISPLAY_PIXEL_FORMAT_565
		#define PSP_GU_PIXEL_FORMAT GU_PSM_5650
		#define PSP_PIXELSIZE	2
		#define FBFRAMESIZE (PSP_PIXELSIZE * PSP_SCREEN_HEIGHT * PSP_LINE_SIZE)
		typedef u16 pixel_type;
	#else
		#define PSP_PIXEL_FORMAT PSP_DISPLAY_PIXEL_FORMAT_8888
		#define PSP_GU_PIXEL_FORMAT GU_PSM_8888
		#define PSP_PIXELSIZE	4
		#define FBFRAMESIZE (PSP_PIXELSIZE * PSP_SCREEN_HEIGHT * PSP_LINE_SIZE)
		typedef u32 pixel_type;
	#endif

	extern volatile tBoolean g_PSPEnableRendering;
	extern volatile tBoolean g_PSPEnableInput;

	/* PSP Specific config items */
	typedef struct _psp_config_items_tag
	{
		tBoolean screen_zoom_factor;
		int mouse_speed_factor;
	} _psp_config_items;
	extern _psp_config_items  g_PSPConfig;
		
	typedef struct _extension_download_dirs 
	{
		char music[MAXPATHLEN+1];
		char mp4[MAXPATHLEN+1];
		char avcmp4[MAXPATHLEN+1];
		char videos[MAXPATHLEN+1];
		char images[MAXPATHLEN+1];
		char other[MAXPATHLEN+1];
	} extension_download_dirs;
	extern extension_download_dirs ext_dl_dir;
	
	/** Functions */
	void wait_for_triangle(char *str);
	/** Reconnect to selected wifi */
	void wifiReconnect();


#endif
