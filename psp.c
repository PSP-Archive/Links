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
//#include <PSPRadio_Exports.h>

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

int connect_to_apctl(int config) {
  int err;
  int stateLast = -1;

  if (sceWlanGetSwitchState() != 1)
    pspDebugScreenClear();

  while (sceWlanGetSwitchState() != 1) {
    pspDebugScreenSetXY(0, 0);
    printf("Please enable WLAN to continue.\n");
    sceKernelDelayThread(1000 * 1000);
  }

  err = sceNetApctlConnect(config);
  if (err != 0) {
    printf("sceNetApctlConnect returns %08X\n", err);
    return 0;
  }

  printf("Connecting...\n");
  while (1) {
    int state;
    err = sceNetApctlGetState(&state);
    if (err != 0) {
      printf("sceNetApctlGetState returns $%x\n", err);
      break;
    }
    if (state != stateLast) {
      printf("  Connection state %d of 4.\n", state);
      stateLast = state;
    }
    if (state == 4) {
      break;
    }
    sceKernelDelayThread(50 * 1000);
  }
  printf("Connected!\n");
  sceKernelDelayThread(3000 * 1000);

  if (err != 0) {
    return 0;
  }

  return 1;
}

char *getconfname(int confnum) {
  static char confname[128];
  sceUtilityGetNetParam(confnum, PSP_NETPARAM_NAME, (netData *)confname);
  return confname;
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
