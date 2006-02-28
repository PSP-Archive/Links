/* 
	Logging Library for the PSP. (Initial Release: Sept. 2005)
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
#include <new>
#include <stdio.h>
#ifndef WIN32
	#include <sys/fcntl.h>
	#include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdarg.h>
#include <time.h>
#include "Logging.h"

CLogging Logging;

CLogging::CLogging()
{
	m_strFilename = NULL;
	m_LogLevel = LOG_INFO;
	m_fp = NULL;
//	m_lock = new CLock("LogLock");
	m_msg = (char *) malloc(2500); /** A message this big would fill up the whole screen of the psp if sent to the screen. */
	m_timeInitial = clock();
}

CLogging::~CLogging()
{
	if (m_strFilename)
	{
		free(m_strFilename), m_strFilename = NULL;
	}
	if (m_fp)
	{
		fclose(m_fp), m_fp = NULL;
	}
//	if(m_lock)
//	{
//		delete m_lock;
//	}
	if (m_msg)
	{
		free (m_msg), m_msg = NULL;
	}
}

int CLogging::Set(char *strLogFilename, loglevel_enum iLogLevel)
{
	int iRes = 0;
	if (strLogFilename && (NULL == m_strFilename) ) //m_fp == NULL)
	{
		m_strFilename = strdup(strLogFilename);
		m_LogLevel = iLogLevel;
		
		Open();
		if (m_fp)
		{
			//fprintf(m_fp, "File opened successfully!\n");
			Close();
			
			/** Remove log file - so we start with a fresh one every time */
//			sceIoRemove(m_strFilename); 
			iRes = 0;
		}
		else
		{
			iRes = -1;
		}
	}
	else
	{
		iRes = -1;
	}
	
	return iRes;
	
}

void CLogging::Open()
{
	if (m_fp)
	{
		Close();
	}
	m_fp = fopen(m_strFilename, "a"); 
}

void CLogging::Close()
{
	if (m_fp)
	{
//		fflush(m_fp);
		fclose(m_fp);
		m_fp = NULL;
	}
}	

void CLogging::SetLevel(loglevel_enum iNewLevel)
{
	m_LogLevel = iNewLevel;
}

int CLogging::Log_(char *strModuleName, int iLineNo, loglevel_enum LogLevel, char *strFormat, ...)
//int CLogging::Log_(char *strModuleName,loglevel_enum LogLevel, char *strFormat, ...)
{
	va_list args;
	//char msg[4096];
	
//	m_lock->Lock();
	if (m_strFilename && LogLevel >= m_LogLevel) /** Log only if Set() was called and loglevel is correct */
	{
		va_start (args, strFormat);         /* Initialize the argument list. */
		int timeDelta = (int)(clock() - m_timeInitial)/1000; /** Clock is in microseconds! */
		
		Open();
		if (m_fp)
		{
			fprintf(m_fp, "%02d:%02d.%03d:",
				((timeDelta / 1000) / 60) % 60,
				(timeDelta / 1000) % 60,
				timeDelta % 1000);

			fprintf(m_fp, "%s@%d<%d>: ", strModuleName, iLineNo, LogLevel);

			vsprintf(m_msg, strFormat, args);
			if (m_msg[strlen(m_msg)-1] == 0x0A)
				m_msg[strlen(m_msg)-1] = 0; /** Remove LF 0D*/
			if (m_msg[strlen(m_msg)-1] == 0x0D) 
				m_msg[strlen(m_msg)-1] = 0; /** Remove CR 0A*/
			fprintf(m_fp, "%s\r\n", m_msg);
		}
		Close();
		
		va_end (args);                  /* Clean up. */
	}
//	m_lock->Unlock();
	
	return 0;
}


