#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>

#include <pspsdk.h>
#include <pspctrl.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <psprtc.h>
#include <psputility_netmodules.h>
#include <psputility_netparam.h> 

#include "psp.h"
#include <PSPRadio_Exports.h>

#include <links.h>

#define printf pspDebugScreenPrintf

volatile tBoolean g_PSPEnableRendering = truE;
volatile tBoolean g_PSPEnableInput = truE;

/* PSP Specific config items */
_psp_config_items  g_PSPConfig = {
	1, 				/* screen_zoom_factor */
	35				/* mouse_speed_factor */
};

extension_download_dirs ext_dl_dir;

int env_termsize(int *x, int *y)
{
	*x = PSP_SCREEN_WIDTH / 7;
	*y = PSP_SCREEN_HEIGHT / 8;

	return 0;
}

void app_init_progress(char *str)
{
	static int step = 0;

	printf("Init Step %d..%s\n", step, str);
    //int fb = getfreebytes();
	//printf("Available memory: %dbytes (%dKB or %dMB)\n", fb, fb/1024, fb/1024/1024);

	step++;
}

void wait_for_triangle(char *str)
{
	printf("%s\n", str);
	printf("** Press TRIANGLE **\n");
	SceCtrlData pad;
	SceCtrlLatch latch ; 
	int button = 0;
	g_PSPEnableRendering = falsE;
	for(;;) 
	{
		sceDisplayWaitVblankStart();
	//sceCtrlReadBufferPositive(&pad, 1);
		sceCtrlReadLatch(&latch);
	
		if (latch.uiMake)
		{
		// Button Pressed 
			button = latch.uiPress;
		}
		else if (latch.uiBreak) {/** Button Released */
			if (button & PSP_CTRL_TRIANGLE)
			{
				break;
			}
		}
	}
	g_PSPEnableRendering = truE;

	psp_reset_graphic_mode();
	sceDisplayWaitVblankStart();
}

/* Functions for a user connecting to Wifi */
#define multiselect(x,y,picks,size,selected,message); {\
		while (!done)\
		{\
			char msBuffer[100];\
			SceCtrlData pad;\
			int onepressed = 0;\
			int loop;\
				/*Print out current selection*/\
			pspDebugScreenSetXY(x,y);\
			pspDebugScreenPrintf(message"\n");\
			for (loop = 0; loop < size; loop++)\
			{\
				if (selected == loop)\
				{ pspDebugScreenPrintf("> %s\n",picks[loop].name); }\
				else\
				{ pspDebugScreenPrintf("  %s\n",picks[loop].name); }\
			}\
				/*now loop on input, let it fall through to redraw, if it is X then break*/\
			while (!onepressed)/*While havent pressed a key*/\
			{\
				sceCtrlReadBufferPositive(&pad, 1); \
				onepressed = ( (pad.Buttons & PSP_CTRL_CROSS) ||\
								(pad.Buttons & PSP_CTRL_UP) ||\
								(pad.Buttons & PSP_CTRL_DOWN));\
			}\
			/*Find the key and change based on it*/\
			if (pad.Buttons & PSP_CTRL_CROSS) done = 1;\
			if (pad.Buttons & PSP_CTRL_UP) selected = (selected + size - 1) % size;\
			if (pad.Buttons & PSP_CTRL_DOWN) selected = (selected+1) % size;\
			while (onepressed)/*Wait for Key Release*/\
			{\
				sceCtrlReadBufferPositive(&pad, 1); \
				onepressed = ( (pad.Buttons & PSP_CTRL_CROSS) ||\
								(pad.Buttons & PSP_CTRL_UP) ||\
								(pad.Buttons & PSP_CTRL_DOWN));\
			}\
		}\
	}


int getWifiAPFromUser()
{
	// enumerate connections
#define MAX_PICK 10
	struct
	{
		int index;
		char name[64];
	} picks[MAX_PICK+1];
	int pick_count = 0;
	char buffer[200];

	int iNetIndex;
	
	
	for (iNetIndex = 1; iNetIndex < 100; iNetIndex++) // skip the 0th connection
	{
		if (sceUtilityCheckNetParam(iNetIndex) != 0)
			continue;  // no good
		sceUtilityGetNetParam(iNetIndex, PSP_NETPARAM_NAME, (netData *)picks[pick_count].name);
		picks[pick_count].index = iNetIndex;
		pick_count++;
		if (pick_count >= MAX_PICK)
			break;  // no more room
	}

	picks[pick_count].index = 0;
	strlcpy(picks[pick_count].name, "None", 64);
	pick_count++;

	pspDebugScreenSetXY(0,7);
	if (pick_count == 0)
	{
		pspDebugScreenPrintf("No connections\n");
		pspDebugScreenPrintf("Please try Network Settings\n");
		sceKernelDelayThread(10*1000000); // 10sec to read before exit
		return -1;
	}
	
	iNetIndex = 0;
	if (pick_count > 1)
	{
		int done = 0;
		
		sceCtrlSetSamplingCycle(0);
		sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

		multiselect(0,7,picks,pick_count,iNetIndex,"Choose a connection and press X");
	}
	return picks[iNetIndex].index;
}

/* Choose the wifi AP to connect to and connect to it */
void wifiChooseConnect()
{
	g_PSPEnableRendering = falsE;
	int iAP = 0;
	iAP = getWifiAPFromUser();
	
	if (iAP > 0)
	{
		connect_to_apctl(iAP);
	}
	g_PSPEnableRendering = truE;
//	sceDisplaySetMode(0, PSP_SCREEN_WIDTH, PSP_SCREEN_HEIGHT);
//	sceDisplaySetFrameBuf((u32 *) (0x40000000 | (u32) sceGeEdramGetAddr()), PSP_LINE_SIZE, PSP_PIXEL_FORMAT, 1);
}

#include <png.h>
#include <pspdisplay.h>
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 272
char *ScreenshotName(char *path);
void ScreenshotStore(char *filename);

void TakeScreenShot()
{
	char	path[MAXPATHLEN];
	char	*filename;
	char    m_strCWD[MAXPATHLEN+1];
	
	getcwd(m_strCWD, MAXPATHLEN);

	sprintf(path, "%s/Screenshots/", m_strCWD);

	filename = ScreenshotName(path);

	if  (filename)
	{
		ScreenshotStore(filename);
		ModuleLog(LOG_INFO, "Screenshot stored as : %s", filename);
		free(filename);
	}
	else
	{
		ModuleLog(LOG_INFO, "No screenshot taken..");
	}
}

char *ScreenshotName(char *path)
{
	char	*filename;
	int		image_number;
	FILE	*temp_handle;

	filename = (char *) malloc(MAXPATHLEN);
	if (filename)
	{
		for (image_number = 0 ; image_number < 1000 ; image_number++)
		{
			sprintf(filename, "%sPSPLinks2_Screen%03d.png", path, image_number);
			temp_handle = fopen(filename, "r");
			// If the file didn't exist we can use this current filename for the screenshot
			if (!temp_handle)
			{
				break;
			}
			fclose(temp_handle);
		}
	}
	return filename;
}

//The code below is take from an example for libpng.
void ScreenshotStore(char *filename)
{
	u32* vram32;
	u16* vram16;
	int bufferwidth;
	int pixelformat;
	int unknown;
	int i, x, y;
	png_structp png_ptr;
	png_infop info_ptr;
	FILE* fp;
	u8* line;
	fp = fopen(filename, "wb");
	if (!fp) return;
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) return;
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
		fclose(fp);
		return;
	}
	png_init_io(png_ptr, fp);
	png_set_IHDR(png_ptr, info_ptr, SCREEN_WIDTH, SCREEN_HEIGHT,
		8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png_ptr, info_ptr);
	line = (u8*) malloc(SCREEN_WIDTH * 3);
	sceDisplayWaitVblankStart();  // if framebuf was set with PSP_DISPLAY_SETBUF_NEXTFRAME, wait until it is changed
	sceDisplayGetFrameBuf((void**)&vram32, &bufferwidth, &pixelformat, &unknown);
	vram16 = (u16*) vram32;
	for (y = 0; y < SCREEN_HEIGHT; y++) {
		for (i = 0, x = 0; x < SCREEN_WIDTH; x++) {
			u32 color = 0;
			u8 r = 0, g = 0, b = 0;
			switch (pixelformat) {
				case PSP_DISPLAY_PIXEL_FORMAT_565:
								color = vram16[x + y * bufferwidth];
								r = (color & 0x1f) << 3;
								g = ((color >> 5) & 0x3f) << 2 ;
								b = ((color >> 11) & 0x1f) << 3 ;
								break;
				case PSP_DISPLAY_PIXEL_FORMAT_5551:
								color = vram16[x + y * bufferwidth];
								r = (color & 0x1f) << 3;
								g = ((color >> 5) & 0x1f) << 3 ;
								b = ((color >> 10) & 0x1f) << 3 ;
								break;
				case PSP_DISPLAY_PIXEL_FORMAT_4444:
								color = vram16[x + y * bufferwidth];
								r = (color & 0xf) << 4;
								g = ((color >> 4) & 0xf) << 4 ;
								b = ((color >> 8) & 0xf) << 4 ;
								break;
				case PSP_DISPLAY_PIXEL_FORMAT_8888:
								color = vram32[x + y * bufferwidth];
								r = color & 0xff;
								g = (color >> 8) & 0xff;
								b = (color >> 16) & 0xff;
								break;
			}
			line[i++] = r;
			line[i++] = g;
			line[i++] = b;
		}
		png_write_row(png_ptr, line);
	}
	free(line);
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
	fclose(fp);
}
