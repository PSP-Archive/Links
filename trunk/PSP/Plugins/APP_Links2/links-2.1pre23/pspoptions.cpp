/*
	PSPRadio / Music streaming client for the PSP. (Initial Release: Sept. 2005)
	Copyright (C) 2005  Rafael Cabezas a.k.a. Raf

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <malloc.h>
#include <iniparser.h>
#include <Logging.h>
#include <pspwlan.h>
#include <psppower.h>
#include <pspctrl.h>
#include "pspoptions.h"

#define ReportError(a,b...)
#undef Log
#define Log(a,b...)

///net glue
#include <pspnet.h>
#include <pspnet_resolver.h>
#include <pspnet_apctl.h>
#include <psputility_netparam.h>
#include <pspnet_inet.h>
#include <pspusb.h>
#include <pspusbstor.h>
#include <sys/select.h>
#include <sys/fcntl.h>

#include <netinet/in.h>

#include <psputility_netmodules.h>
#include <psputility_netparam.h>
#include <pspwlan.h>
#include <pspnet.h>
#include <pspnet_apctl.h>
#include "PSPNet.h"

#define SCE_NET_APCTL_INFO_IP_ADDRESS		8
#define apctl_state_IPObtained				4

bool m_NetworkEnabled = false;
char m_strMyIP[64];
char m_ResolverBuffer[1024];
int  m_ResolverId;

int NetApctlHandler() 
{
	int iRet = 0;
	int state1 = 0;
	int err = sceNetApctlGetState(&state1);
	if (err != 0)
	{
		ReportError("NetApctlHandler: getstate: err=%d state=%d\n", err, state1);
		iRet = -1;
	}
	
	int statechange=0;
	int ostate=0xffffffff;

	while (iRet == 0)
	{
		int state;
		
		err = sceNetApctlGetState(&state);
		if (err != 0)
		{
			ReportError("NetApctlHandler: sceNetApctlGetState returns %d\n", err);
			iRet = -1;
			break;
		}
		
		/** Timeout */
		if(statechange > 180) 
		{
			iRet = -1;
			break;
		} 
		else if(state == ostate) 
		{
			statechange++;
		} 
		else 
		{
			statechange=0;
		}
		ostate=state;
		
		sceKernelDelayThread(50000);  /** 50ms */
		
		if (state == apctl_state_IPObtained)
		{
			break;  /** IP Address Ready */
		}
	}

	if(iRet == 0)
	{
		/** get IP address */
		if (sceNetApctlGetInfo(SCE_NET_APCTL_INFO_IP_ADDRESS, m_strMyIP) != 0)
		{
			/** Error! */
			strcpy(m_strMyIP, "0.0.0.0");
			ReportError("NetApctlHandler: Error-could not get IP\n");
			iRet = -1;
		}
	}
	
	return iRet;
}

int WLANConnectionHandler(int profile) 
{
    u32 err;
    int iRet = 0;

	err = sceNetApctlConnect(profile);
    if (err != 0) 
	{
		ReportError("ERROR - WLANConnectionHandler : sceNetApctlConnect returned '0x%x'.\n", err);
        iRet =-1;
    }
    
	sceKernelDelayThread(500*1000);  /** 500ms */
	
	if (0 == iRet)  
	{
		/** Let's aquire the IP address */
		if (NetApctlHandler() == 0)
		{
			iRet = 0;
		}
		else
		{
			iRet = -1;
		}
	}
		
	return iRet;
}


int GetNumberOfNetworkProfiles()
{
	int iNumProfiles = 0, blank_found = 0;
	for( int i = 0; i < 100; i++)
	{
		/** Updated to not stop immediately when a non-zero value is found -- idea by danzel (forums.ps2dev.org/viewtopic.php?t=5349) */
		if (sceUtilityCheckNetParam(i) == 0)
		{
			iNumProfiles++;
		}
		else
		{
			/** If we found 5 invalid ones in a row, we assume we're done, and stop enumeration */
			blank_found++;
			if (blank_found > 5)
				break;
		}
	}

	return iNumProfiles;
}

void GetNetworkProfileName(int iProfile, char *buf, size_t size)
{
	netData data;
	memset(&data, 0, sizeof(netData));
	data.asUint = 0xBADF00D;
	memset(&data.asString[4], 0, 124);
	sceUtilityGetNetParam(iProfile, 0/** 0 = Profile Name*/, &data);
	
	strlcpy(buf, data.asString, size);
}

bool IsNetworkEnabled()
{
	return m_NetworkEnabled;
}

void DisableNetwork()
{
	u32 err = 0;
	
	if (true == IsNetworkEnabled())
	{
		err = sceNetApctlDisconnect();
		if (err != 0) 
		{
			ReportError("ERROR - DisableNetwork: sceNetApctlDisconnect returned '0x%x'.\n", err);
	    }
    }
    else
    {
	    Log(LOG_ERROR, "DisableNetwork() Called, but networking was not enabled. Ignoring.");
    }
	m_NetworkEnabled = false;
}

int EnableNetwork(int profile)
{
	int iRet = 0;
	if (sceNetApctlGetInfo(SCE_NET_APCTL_INFO_IP_ADDRESS, m_strMyIP) != 0)
	{
		if (WLANConnectionHandler(profile) == 0)
		{
			iRet = 0;
			
			if (0 == iRet)
			{
				m_NetworkEnabled = true;
			}
		}
		else
		{
			ReportError("Error starting network\n");
			iRet = -1;
		}
	}
	else
	{
		Log(LOG_ERROR, "IP already assigned: '%s'.", m_strMyIP);
		m_NetworkEnabled = true;
		iRet = 0;
	}
	return iRet;
}
char *GetMyIP() { return m_strMyIP; };
///glue


enum OptionIDs
{
	OPTION_ID_NETWORK_PROFILES,
	OPTION_ID_WIFI_AUTOSTART,
	OPTION_ID_USB_ENABLE,
	OPTION_ID_CPU_SPEED,
	OPTION_ID_LOG_LEVEL,
	OPTION_ID_SAVE_CONFIG,
	OPTION_ID_SWITCH,
	OPTION_ID_EXIT,
};

OptionsScreen::Options OptionsScreenData[] =
{
		/* ID						Option Name					Option State List			(active,selected,number-of)-states */
	{	OPTION_ID_NETWORK_PROFILES,	"WiFi",						{"Off","1","2","3","4"},		1,1,5		},
	{	OPTION_ID_WIFI_AUTOSTART,	"WiFi AutoStart",			{"No", "Yes"},					1,1,2		},
	
	{	OPTION_ID_USB_ENABLE,		"USB",						{"OFF","ON","AUTOSTART"},					1,1,3		},
	{	OPTION_ID_CPU_SPEED,		"CPU Speed",				{"111","222","266","333"},		2,2,4		},
	{	OPTION_ID_LOG_LEVEL,		"Log Level",				{"All","Verbose","Info","Errors","Off"},	1,1,5		},
	{	OPTION_ID_SAVE_CONFIG,		"Save Options",				{""},							0,0,0		},
	{	OPTION_ID_SWITCH,			"Switch to PSPRadio",		{""},							0,0,0		},
	{	OPTION_ID_EXIT,				"Exit Links2",				{""},							0,0,0		},

	{  -1,  						"",							{""},							0,0,0		}
};

OptionsScreen::OptionsScreen()
{
	m_iNetworkProfile = 1;
	m_WifiAutoStart = false;
	m_USBAutoStart = false;
	//m_USBStorage = new CPSPUSBStorage(pPSPApp);
	m_Dirty = true;
	LoadFromConfig();
	
	Activate();
}

void OptionsScreen::Activate()
{
	// Update network and UI options.  This is necesary the first time */
	UpdateOptionsData();
}

void OptionsScreen::LoadFromConfig()
{
	CIniParser *pConfig = NULL;
	Log(LOG_INFO, "LoadFromConfig(): Loading Options from configuration file");
	Log(LOG_LOWLEVEL, "LoadFromConfig(): pConfig=%p", pConfig);

	if (pConfig)
	{
		/** CPU FREQ */
		int iRet = 0;
		switch(pConfig->GetInteger("SYSTEM:CPUFREQ"))
		{
			case 111:
				iRet = scePowerSetClockFrequency(111, 111, 55);
				break;
			case 222:
				iRet = scePowerSetClockFrequency(222, 222, 111);
				break;
			case 265:
			case 266:
				iRet = scePowerSetClockFrequency(266, 266, 133);
				break;
			case 333:
				iRet = scePowerSetClockFrequency(333, 333, 166);
				break;
			default:
				iRet = -1;
				break;
		}
		if (0 != iRet)
		{
			Log(LOG_ERROR, "LoadFromConfig(): Unable to change CPU/BUS Speed to selection %d",
					pConfig->GetInteger("SYSTEM:CPUFREQ"));
		}
		/** CPU FREQ */

		/** LOGLEVEL */
		//pLogging->SetLevel((loglevel_enum)pConfig->GetInteger("DEBUGGING:LOGLEVEL", 100));
		/** LOGLEVEL */

		/** WIFI PROFILE */
		m_iNetworkProfile = pConfig->GetInteger("WIFI:PROFILE", 1);
		/** WIFI PROFILE */

		/** OPTION_ID_WIFI_AUTOSTART */
		if (1 == pConfig->GetInteger("WIFI:AUTOSTART", 0))
		{
			m_WifiAutoStart = true;
			Log(LOG_INFO, "LoadFromConfig(): WIFI AUTOSTART SET: Enabling Network; using profile: %d",
				m_iNetworkProfile);
			Start_Network();
		}
		else
		{
			m_WifiAutoStart = false;
			Log(LOG_INFO, "LoadFromConfig(): WIFI AUTOSTART Not Set, Not starting network");
		}
		/** OPTION_ID_WIFI_AUTOSTART */

		/** OPTION_ID_USB_AUTOSTART */
		if (1 == pConfig->GetInteger("SYSTEM:USB_AUTOSTART", 0))
		{
			m_USBAutoStart = true;
			Log(LOG_INFO, "LoadFromConfig(): USB_AUTOSTART SET: Enabling USB");
			//m_USBStorage->EnableUSB();
		}
		else
		{
			m_USBAutoStart = false;
			Log(LOG_INFO, "LoadFromConfig(): USB_AUTOSTART Not Set");
		}
		/** OPTION_ID_USB_AUTOSTART */
	}
	else
	{
		Log(LOG_ERROR, "LoadFromConfig(): Error: no config object.");
	}
}

void OptionsScreen::SaveToConfigFile()
{
	CIniParser *pConfig = NULL;

	if (pConfig)
	{
		pConfig->SetInteger("DEBUGGING:LOGLEVEL", 0);//pLogging->GetLevel());
		pConfig->SetInteger("SYSTEM:CPUFREQ", scePowerGetCpuClockFrequency());
		pConfig->SetInteger("WIFI:PROFILE", m_iNetworkProfile);
		pConfig->SetInteger("WIFI:AUTOSTART", m_WifiAutoStart);
		pConfig->SetInteger("SYSTEM:USB_AUTOSTART", m_USBAutoStart);

		pConfig->Save();
	}
	else
	{
		Log(LOG_ERROR, "SaveToConfigFile(): Error: no config object.");
	}
}

/** This populates and updates the option data */
void OptionsScreen::UpdateOptionsData()
{
	Options Option;

	list<Options>::iterator		OptionIterator;

	while(m_OptionsList.size())
	{
		// Release memory allocated for network profile names
		OptionIterator = m_OptionsList.begin();
		if ((*OptionIterator).Id == OPTION_ID_NETWORK_PROFILES)
		{
			for (int i = 0; i < (*OptionIterator).iNumberOfStates; i++)
			{
				free((*OptionIterator).strStates[i]);
			}
		}
		m_OptionsList.pop_front();
	}

	for (int iOptNo=0;; iOptNo++)
	{
		if (-1 == OptionsScreenData[iOptNo].Id)
			break;

		Option.Id = OptionsScreenData[iOptNo].Id;
		sprintf(Option.strName, 	OptionsScreenData[iOptNo].strName);
		memcpy(Option.strStates, OptionsScreenData[iOptNo].strStates,
			sizeof(char*)*OptionsScreenData[iOptNo].iNumberOfStates);
		Option.iActiveState		= OptionsScreenData[iOptNo].iActiveState;
		Option.iSelectedState	= OptionsScreenData[iOptNo].iSelectedState;
		Option.iNumberOfStates	= OptionsScreenData[iOptNo].iNumberOfStates;

		/** Modify from data table */
		switch(iOptNo)
		{
			case OPTION_ID_NETWORK_PROFILES:
			{
				Option.iNumberOfStates = GetNumberOfNetworkProfiles();
				char *NetworkName = NULL;
				Option.strStates[0] = strdup("Off");
				for (int i = 1; i <= Option.iNumberOfStates; i++)
				{
					NetworkName = (char*)malloc(128);
					GetNetworkProfileName(i, NetworkName, 128);
					Option.strStates[i] = NetworkName;
				}
				Option.iActiveState = (IsNetworkEnabled()==true)?(m_iNetworkProfile+1):1;
				Option.iSelectedState = Option.iActiveState;
				break;
			}
			
			case OPTION_ID_USB_ENABLE:
				if (m_USBAutoStart==true)
				{
					Option.iActiveState = 3;
				}
				else
				{
					Option.iActiveState = 1;//(m_USBStorage->IsUSBEnabled()==true)?2:1;
				}
				Option.iSelectedState = Option.iActiveState;
				break;
			
			case OPTION_ID_CPU_SPEED:
				switch(scePowerGetCpuClockFrequency())
				{
					case 111:
						Option.iActiveState = 1;
						break;
					case 222:
						Option.iActiveState = 2;
						break;
					case 265:
					case 266:
						Option.iActiveState = 3;
						break;
					case 333:
						Option.iActiveState = 4;
						break;
					default:
						Log(LOG_ERROR, "CPU speed unrecognized: %dMHz",
							scePowerGetCpuClockFrequency());
						break;
				}
				Option.iSelectedState = Option.iActiveState;
				break;

			case OPTION_ID_LOG_LEVEL:
				#if 0
				switch(pLogging->GetLevel())
				{
					case 0:
					case LOG_VERYLOW:
						Option.iActiveState = 1;
						break;
					case LOG_LOWLEVEL:
						Option.iActiveState = 2;
						break;
					case LOG_INFO:
						Option.iActiveState = 3;
						break;
					case LOG_ERROR:
						Option.iActiveState = 4;
						break;
					case LOG_ALWAYS:
					default:
						Option.iActiveState = 5;
						break;
				}
				#endif
				Option.iSelectedState = Option.iActiveState;
				break;

			case OPTION_ID_WIFI_AUTOSTART:
				Option.iActiveState = (m_WifiAutoStart==true)?2:1;
				Option.iSelectedState = Option.iActiveState;
				break;
		}

		m_OptionsList.push_back(Option);
	}

	m_CurrentOptionIterator = m_OptionsList.begin();

}

void OptionsScreen::OnOptionActivation()
{
	bool fOptionActivated = false;
	static time_t	timeLastTime = 0;
	time_t timeNow = clock() / (1000*1000); /** clock is in microseconds */
	int iSelectionBase0 = (*m_CurrentOptionIterator).iSelectedState - 1;
	int iSelectionBase1 = (*m_CurrentOptionIterator).iSelectedState;
	char *strSelection  = (*m_CurrentOptionIterator).strStates[iSelectionBase0];

	switch ((*m_CurrentOptionIterator).Id)
	{
		case OPTION_ID_NETWORK_PROFILES:
			m_iNetworkProfile = iSelectionBase0;
			if (m_iNetworkProfile > 0) /** Enable */
			{
				Start_Network();
				if (true == IsNetworkEnabled())
				{
					fOptionActivated = true;
				}
			}
			else /** Disable */
			{
				Stop_Network();
				fOptionActivated = true;
			}
			break;

		case OPTION_ID_WIFI_AUTOSTART:
			m_WifiAutoStart = (iSelectionBase1 == 2)?true:false;
			fOptionActivated = true;
			break;

		case OPTION_ID_USB_ENABLE:
			switch (iSelectionBase1)
			{
				case 1: /* OFF */
				{
					int iRet = -1;//m_USBStorage->DisableUSB();
					if (-1 == iRet)
					{
						ReportError("USB Busy, try again later.");
					}
					if (false)// == m_USBStorage->IsUSBEnabled())
					{
						fOptionActivated = true;
					}
					m_USBAutoStart = false;
					break;
				}
				case 2: /* ON  */
					//m_USBStorage->EnableUSB();
					if (0)//true == m_USBStorage->IsUSBEnabled())
					{
						fOptionActivated = true;
					}
					m_USBAutoStart = false;
					break;
				case 3: /* AUTOSTART */
					//m_USBStorage->EnableUSB();
					if (0)//true == m_USBStorage->IsUSBEnabled())
					{
						fOptionActivated = true;
					}
					m_USBAutoStart = true;
					break;
			}
			break;
			
		case OPTION_ID_CPU_SPEED:
		{
			int iRet = -1;
			switch (iSelectionBase1)
			{
				case 1: /* 111 */
					iRet = scePowerSetClockFrequency(111, 111, 55);
					break;
				case 2: /* 222 */
					iRet = scePowerSetClockFrequency(222, 222, 111);
					break;
				case 3: /* 266 */
					iRet = scePowerSetClockFrequency(266, 266, 133);
					break;
				case 4: /* 333 */
					iRet = scePowerSetClockFrequency(333, 333, 166);
					break;
			}
		    if (0 == iRet)
			{
				fOptionActivated = true;
			}
			else
			{
				Log(LOG_ERROR, "Unable to change CPU/BUS Speed to selection %d",
						(*m_CurrentOptionIterator).iSelectedState);
			}
			break;
		}

		case OPTION_ID_LOG_LEVEL:
			#if 0
			switch(iSelectionBase1)
			{
				case 1:
					pLogging->SetLevel(LOG_VERYLOW);
					break;
				case 2:
					pLogging->SetLevel(LOG_LOWLEVEL);
					break;
				case 3:
					pLogging->SetLevel(LOG_INFO);
					break;
				case 4:
					pLogging->SetLevel(LOG_ERROR);
					break;
				case 5:
					pLogging->SetLevel(LOG_ALWAYS);
					break;
			}
			Log(LOG_ALWAYS, "Log Level Changed to (%d)", pLogging->GetLevel());
			#endif
			fOptionActivated = true;
			break;

		case OPTION_ID_SAVE_CONFIG:
			//gPSPRadio->m_UI->DisplayMessage("Saving Configuration Options");
			Log(LOG_INFO, "User selected to save config file.");
			SaveToConfigFile();
			//gPSPRadio->m_UI->DisplayMessage("Done");
			fOptionActivated = true;
			break;

		case OPTION_ID_EXIT:
			Log(LOG_ALWAYS, "User selected to Exit.");
			//pPSPApp->SendEvent(EID_EXIT_SELECTED, NULL, SID_SCREENHANDLER);
			break;

	}

	if (true == fOptionActivated)
	{
		(*m_CurrentOptionIterator).iActiveState = (*m_CurrentOptionIterator).iSelectedState;
		//gPSPRadio->m_UI->UpdateOptionsScreen(m_OptionsList, m_CurrentOptionIterator);
		m_Dirty = true;

	}
}

int OptionsScreen::Start_Network(int iProfile)
{
	Log(LOG_LOWLEVEL, "Start_Network(%d)", iProfile);
	
	if (-1 != iProfile)
	{
		m_iNetworkProfile = abs(iProfile);
	}

	if (0 == iProfile)
	{
		m_iNetworkProfile = 1;
		Log(LOG_ERROR, "Network Profile in is invalid. Network profiles start from 1.");
	}
	if (sceWlanGetSwitchState() != 0)
	{
		if (IsNetworkEnabled())
		{
			//if (gPSPRadio->m_UI)
			//	gPSPRadio->m_UI->DisplayMessage_DisablingNetwork();

			Log(LOG_INFO, "Restarting networking...");
			DisableNetwork();
			sceKernelDelayThread(500000);
		}

		//if (gPSPRadio->m_UI)
		//	gPSPRadio->m_UI->DisplayMessage_EnablingNetwork();

		if (EnableNetwork(abs(m_iNetworkProfile)) == 0)
		{
			//if (gPSPRadio->m_UI)
			//	gPSPRadio->m_UI->DisplayMessage_NetworkReady(pPSPApp->GetMyIP());
			
		}
		else
		{
			//if (gPSPRadio->m_UI)
			//	gPSPRadio->m_UI->DisplayMessage_DisablingNetwork();
		}

		//if (gPSPRadio->m_UI)
		//	gPSPRadio->m_UI->DisplayMessage_NetworkReady(pPSPApp->GetMyIP());
		Log(LOG_INFO, "Networking Enabled, IP='%s'...", GetMyIP());

		Log(LOG_INFO, "Enabling Network: Done. IP='%s'", GetMyIP());
		
	}
	else
	{
		ReportError("The Network Switch is OFF, Cannot Start Network.");
	}

	return 0;
}

int OptionsScreen::Stop_Network()
{
	CIniParser *pConfig = NULL;//m_ScreenHandler->GetConfig();

	if (IsNetworkEnabled())
	{
		//gPSPRadio->m_UI->DisplayMessage_DisablingNetwork();

		Log(LOG_INFO, "Disabling network...");
		DisableNetwork();
		sceKernelDelayThread(500000);
	}
	return 0;
}

#define IS_BUTTON_PRESSED(i,b) (((i) & 0xFFFF) == (b)) /* i = read button mask b = expected match */

void OptionsScreen::InputHandler(int iButtonMask)
{
	if (IS_BUTTON_PRESSED(iButtonMask, PSP_CTRL_UP))
	{
		if(m_CurrentOptionIterator == m_OptionsList.begin())
			m_CurrentOptionIterator = m_OptionsList.end();
		m_CurrentOptionIterator--;
		//gPSPRadio->m_UI->UpdateOptionsScreen(m_OptionsList, m_CurrentOptionIterator);
		m_Dirty = true;
	}
	else if (IS_BUTTON_PRESSED(iButtonMask, PSP_CTRL_DOWN))
	{
		m_CurrentOptionIterator++;
		if(m_CurrentOptionIterator == m_OptionsList.end())
			m_CurrentOptionIterator = m_OptionsList.begin();

		//gPSPRadio->m_UI->UpdateOptionsScreen(m_OptionsList, m_CurrentOptionIterator);
		m_Dirty = true;
	}
	else if (IS_BUTTON_PRESSED(iButtonMask, PSP_CTRL_LEFT))
	{
		if ((*m_CurrentOptionIterator).iSelectedState > 1)
		{
			(*m_CurrentOptionIterator).iSelectedState--;

			//OnOptionChange();
			//gPSPRadio->m_UI->UpdateOptionsScreen(m_OptionsList, m_CurrentOptionIterator);
		}
		m_Dirty = true;
	}
	else if (IS_BUTTON_PRESSED(iButtonMask, PSP_CTRL_RIGHT))
	{
		if ((*m_CurrentOptionIterator).iSelectedState < (*m_CurrentOptionIterator).iNumberOfStates)
		{
			(*m_CurrentOptionIterator).iSelectedState++;

			//OnOptionChange();
			///gPSPRadio->m_UI->UpdateOptionsScreen(m_OptionsList, m_CurrentOptionIterator);
		}
		m_Dirty = true;
	}
	else if (IS_BUTTON_PRESSED(iButtonMask, PSP_CTRL_CROSS))
	{
		OnOptionActivation();
	}
}
