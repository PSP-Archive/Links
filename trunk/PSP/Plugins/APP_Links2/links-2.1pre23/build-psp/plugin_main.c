/*
 * Links2 Port for PSPRadio
 * -----------------------------------------------------------------------
 *
 * plugin_main.c - Based on parts of: Simple PRX example by James Forshaw <tyranid@gmail.com>
 * by: Raf 2006.
 *
 */
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>

#include <pspctrl.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <psprtc.h>

#include <openssl/rand.h>

#include <pthread.h>
#include <Tools.h>
#include <PSPRadio_Exports.h>
#include <APP_Exports.h>
#include <Common.h>
#include <malloc.h>
	
#include <links.h>

#ifdef STAND_ALONE_APP
	#ifdef DEBUGMODE
		PSP_MODULE_INFO("DebugLinks2", 0x0, 0, 1); 	 
		PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER); 
		PSP_MAIN_THREAD_PRIORITY(80);
	#else
		PSP_MODULE_INFO("Links2", 0x1000, 1, 1);
		PSP_MAIN_THREAD_ATTR(0);
	#endif
	void CallbackThread(void *argp);
	void StartNetworkThread(void *argp);
//	PSP_HEAP_SIZE_KB(20*1024);
#else
	PSP_MODULE_INFO("APP_Links2", 0, 1, 1);
	PSP_HEAP_SIZE_KB(1024*6);
#endif

#define printf pspDebugScreenPrintf
void app_plugin_main();
void wait_for_triangle(char *str);
int main_loop(int argc, char** argv);
int connect_to_apctl(int config);
void wifiChooseConnect();

//Set to truE by the network thread once it has connected.
tBoolean networkThreadInitialized = falsE;

/** Plugin code */
int ModuleStartAPP()
{
	pthread_t pthid;
	pthread_attr_t pthattr;
	struct sched_param shdparam;
	
	sleep(1);

	pspDebugScreenInit();
	
	SceSize am = sceKernelTotalFreeMemSize();
	ModuleLog(LOG_INFO, "ModuleStartApp(): Available memory: %dbytes (%dKB or %dMB)", am, am/1024, am/1024/1024);

	pthread_attr_init(&pthattr);
	shdparam.sched_policy = SCHED_OTHER;
	shdparam.sched_priority = 45;
	pthread_attr_setschedparam(&pthattr, &shdparam);
	pthread_create(&pthid, &pthattr, app_plugin_main, NULL);

	return 0;
}

int ModuleContinueApp()
{
	PSPRadioExport_RequestExclusiveAccess(PLUGIN_APP);
	pspDebugScreenInit();

	g_PSPEnableInput = truE;
	g_PSPEnableRendering = truE;
	cls_redraw_all_terminals();
	return 0;
}

int CreateHomepage(char *file)
{
	FILE *fp = fopen(file, "w");
	
	if (fp)
	{
		fprintf(fp, "<html><head><title>Links2 On PSP</title></head><body bgcolor=\"white\"><h3 align=\"center\"><b>Links2 For PSPRadio</b></h3>\n");
	
		fprintf(fp, "<p>PSP Port by Raf. Thanks to Danzel for OSK!<br> Thanks to Sandberg for upcoming GU driver :)<br>");
#ifdef STAND_ALONE_APP
		fprintf(fp, "This port is a stand alone application.<br>", PSPRadioExport_GetVersion());
#else
		fprintf(fp, "This port is a plugin for PSPRadio Version %s.<br>", PSPRadioExport_GetVersion());
#endif
		fprintf(fp, "Visit us at <a href=\"http://pspradio.sourceforge.net\">PSPRadio Forums</a> Or <a href=\"http://rafpsp.blogspot.com\">PSPRadio HomePage</a>.</p>\n");
		
		/** Load tips from tips file */
		{
			FILE *fTips = NULL;
			char strLine[512];
			#ifdef STAND_ALONE_APP
				fTips = fopen(".links/tips.html", "r");
			#else
				fTips = fopen("APP_Links2/tips.html", "r");
			#endif
			if (fTips)
			{
				while (!feof(fTips))
				{
					if (fgets(strLine, 512, fTips) != NULL)
					{
						fprintf(fp, strLine);
					}
					else
					{
						break;
					}
				}
				fclose(fTips), fTips = NULL;
			}
			else
			{
				fprintf(fp, "<p><br><br>PSP Port Tips: <br>Input mode: <b>START</b> = Enter. <b>START</b> = Exit.<br><b>L+UP</b> = Page up <b>L+Down</b> = Page Down.<br><b>CIRCLE</b> = Yes / OK / Enter<br><b>SQUARE</b> = No / Cancel<br><b>CROSS</b> = Right Mouse Click. <b>TRIANGLE</b> = Left Mouse Click.<br><b>SELECT</b> = Take Screenshot. <b>CROSS+SELECT</b> = Network Reconnect.</p><br><br>\n");
			}
		}
		
		fprintf(fp, "\n</body></html>\n");
		
		fclose(fp), fp = NULL;
		return 0;
	}
	
	return -1;
}

//static char *argv[] = { "APP_Links2", "-g", "-driver", "pspsdl", "-mode", "480x272", LINKS_HOMEPAGE_URL, NULL };	
static char *argv[] = { "APP_Links2", "-g", "-driver", "pspgu", "-mode", "480x272", LINKS_HOMEPAGE_URL, NULL };	

void app_plugin_main()
{
	static int argc = sizeof(argv)/sizeof(char *)-1; 	/* idea from scummvm psp port */
	char str[128];
	int ret;
	pspTime time;
	
	PSPRadioExport_RequestExclusiveAccess(PLUGIN_APP);
//	pspDebugScreenInit();

	g_PSPEnableInput = truE;
	g_PSPEnableRendering = truE;

	pspDebugScreenPrintf("- Seeding SSL random generator...\n");
	sceDisplayWaitVblankStart();
	
	sceRtcGetCurrentClockLocalTime(&time);
	RAND_seed(&time, sizeof(pspTime));
		
	ret = main_loop(argc, (char **)&argv);

	if (ret != 0) 
	{
		sprintf(str, "Application returns %d", ret);
		wait_for_triangle(str);
	}

#ifdef STAND_ALONE_APP
	sceKernelExitGame();
#endif
	
	pspDebugScreenInit();
	
	///PSPRadioExport_GiveUpExclusiveAccess(); /** PluginExits gives up access too */
	PSPRadioExport_PluginExits(PLUGIN_APP); /** Notify PSPRadio, so it can unload the plugin */
}

/* Connect to an access point */
int connect_to_apctl(int config)
{
	int err;
	int stateLast = -1;

	/* Connect using the first profile */
	err = sceNetApctlConnect(config);
	if (err != 0)
	{
		printf(": sceNetApctlConnect returns %08X\n", err);
		return 0;
	}

	printf(": Connecting...\n");
	while (1)
	{
		int state;
		err = sceNetApctlGetState(&state);
		if (err != 0)
		{
			printf(": sceNetApctlGetState returns $%x\n", err);
			break;
		}
		if (state > stateLast)
		{
			printf("  connection state %d of 5\n", state + 1);
			stateLast = state;
		}
		if (state == 4)
			break;  // connected with static IP

	// wait a little before polling again
		sceKernelDelayThread(50*1000); // 50ms
	}
	printf(": Connected!\n");

	if(err != 0)
	{
		return 0;
	}

	return 1;
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
}


int getfreebytes()
{
   	struct mallinfo mi;
   	//mi = mallinfo();
	//return ((20*1024*1024) - mi.arena + mi.fordblks;
}

void app_init_progress(char *str)
{
	static int step = 0;

	printf("Init Step %d..%s\n", step, str);
    //int fb = getfreebytes();
	//printf("Available memory: %dbytes (%dKB or %dMB)\n", fb, fb/1024, fb/1024/1024);

	step++;
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
		sceUtilityGetNetParam(iNetIndex, 0, picks[pick_count].name);
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
	sceDisplaySetMode(0, PSP_SCREEN_WIDTH, PSP_SCREEN_HEIGHT);
	sceDisplaySetFrameBuf((u32 *) (0x40000000 | (u32) sceGeEdramGetAddr()), PSP_LINE_SIZE, PSP_PIXEL_FORMAT, 1);
}

/** Stand alone code: */
#ifdef STAND_ALONE_APP
/** -- Exception handler */
void MyExceptionHandler(PspDebugRegBlock *regs)
{
	static int bFirstTime = 1;

	g_PSPEnableRendering = falsE;
	if (bFirstTime)
	{
		pspDebugScreenInit();
		pspDebugScreenSetBackColor(0x000000FF);
		pspDebugScreenSetTextColor(0xFFFFFFFF);
		pspDebugScreenClear();

		pspDebugScreenPrintf("Links2 -- Exception Caught:\n");
		pspDebugScreenPrintf("Please provide the following information when filing a bug report:\n\n");
		pspDebugScreenPrintf("Exception Details:\n");
		pspDebugDumpException(regs);
		pspDebugScreenPrintf("\nHolding select to capture a shot of this screen for reference\n");
		pspDebugScreenPrintf("may or may not work at this point.\n");
		pspDebugScreenPrintf("\nPlease Use the Home Menu to return to the VSH.\n");
		pspDebugScreenPrintf("-----------------------------------------------------------------\n");

		bFirstTime = 0;
	}
	pspDebugScreenPrintf("******* Important Registers: epc=0x%x ra=0x%x\n", regs->epc, regs->r[31]);

	wait_for_triangle("");
	sceKernelExitGame();
}

int main(int argc, char **argv)
{
	pthread_t pthid;
	pthread_attr_t pthattr;
	struct sched_param shdparam;
#ifndef DEBUGMODE	
	int kmode_is_available = (sceKernelDevkitVersion() < 0x02000010); /** Copied from SDL_psp_main.c */
#endif

	pthread_attr_init(&pthattr);
	shdparam.sched_policy = SCHED_OTHER;
	
	shdparam.sched_priority = 32;
	pthread_attr_setschedparam(&pthattr, &shdparam);
	pthread_create(&pthid, &pthattr, CallbackThread, NULL);
	
	pspDebugScreenInit();
	pspDebugScreenPrintf("Links2 For PSP\n\n");
	
#ifndef DEBUGMODE
	if (kmode_is_available) 
	{
		pspKernelSetKernelPC();
		pspDebugInstallErrorHandler(MyExceptionHandler);
		
		pspSdkInstallNoDeviceCheckPatch();
		pspSdkInstallNoPlainModuleCheckPatch();
		
		pspDebugScreenPrintf("- Loading networking modules...\n");
		sceDisplayWaitVblankStart();
		
		if(pspSdkLoadInetModules() < 0)
		{
			pspDebugScreenPrintf("** Error, could not load inet modules\n");
			
		}
	}
	else
	{
		pspDebugScreenPrintf("- F/W Version Found >= 2.0: Skipping Module Loading...\n");
		sceDisplayWaitVblankStart();
	}
#endif
	
	pspDebugScreenPrintf("- Connecting to Access Point...\n");
	sceDisplayWaitVblankStart();
	
	shdparam.sched_priority = 32;
	pthread_attr_setschedparam(&pthattr, &shdparam);
	pthread_create(&pthid, &pthattr, StartNetworkThread, NULL);
	
	//Wait while the network thread connects
	while (networkThreadInitialized != truE)
	{
		sceKernelDelayThread(200*1000);
	}
	
	shdparam.sched_priority = 35;
	pthread_attr_setschedparam(&pthattr, &shdparam);
	pthread_create(&pthid, &pthattr, app_plugin_main, NULL);
	
	sceKernelSleepThreadCB();
	return 0;
}

/* Exit callback */
int exit_callback(int arg1, int arg2, void *common)
{
	sceKernelExitGame();
	return 0;
}

/* Callback thread */
void StartNetworkThread(void *argp)
{
	int cbid;
	static int ResolverId;
	static char resolver_buffer[1024];

	pspSdkInetInit();
	
	sceNetResolverCreate(&ResolverId, resolver_buffer, 1024);
	
	wifiChooseConnect();

	networkThreadInitialized = truE;

	sceKernelSleepThreadCB();
}

void CallbackThread(void *argp)
{
	int cbid;
	
	cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);
	
	sceKernelSleepThreadCB();
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
#else
void TakeScreenShot()
{
	PSPRadioExport_TakeScreenShot();
}
#endif
