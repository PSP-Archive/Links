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

PSP_MODULE_INFO("APP_Links2", 0, 1, 1);
PSP_HEAP_SIZE_KB(1024*10);

#define printf pspDebugScreenPrintf
void app_plugin_main();
void wait_for_triangle(char *str);
int main_loop(int argc, char** argv);
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
		fprintf(fp, "This port is a plugin for PSPRadio Version %s.<br>", PSPRadioExport_GetVersion());
		fprintf(fp, "Visit us at <a href=\"http://pspradio.sourceforge.net\">PSPRadio Forums</a> Or <a href=\"http://rafpsp.blogspot.com\">PSPRadio HomePage</a>.</p>\n");
		
		/** Load tips from tips file */
		{
			FILE *fTips = NULL;
			char strLine[512];
			fTips = fopen("APP_Links2/tips.html", "r");
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

	pspDebugScreenInit();
	
	///PSPRadioExport_GiveUpExclusiveAccess(); /** PluginExits gives up access too */
	PSPRadioExport_PluginExits(PLUGIN_APP); /** Notify PSPRadio, so it can unload the plugin */
}

int getfreebytes()
{
   	struct mallinfo mi;
   	//mi = mallinfo();
	//return ((20*1024*1024) - mi.arena + mi.fordblks;
}

void TakeScreenShot()
{
	PSPRadioExport_TakeScreenShot();
}
