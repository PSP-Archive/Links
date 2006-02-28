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
#ifndef _CLOGGINGH_
	#define _CLOGGINGH_
	
//	#include <pspkernel.h>
//	#include <pspkerneltypes.h>
//	#include <PSPSema.h>

#ifdef WIN32
	#include <time.h>
#endif
	
	
	enum loglevel_enum
	{
		LOG_VERYLOW  = 10,
		LOG_LOWLEVEL = 20,
		LOG_INFO	 = 50,
		LOG_ERROR	 = 80,
		LOG_ALWAYS	 = 100
	};
	
	class CLogging
	{
	public:
		CLogging();
		~CLogging();
	
		int Set(char *strLogFilename, loglevel_enum iLogLevel);
		void SetLevel(loglevel_enum iNewLevel);
		int  GetLevel(){return m_LogLevel;}
		
		int Log_(char *strModuleName, int iLineNo, loglevel_enum LogLevel, char *strFormat, ...);
		
	private:
		char *m_strFilename;
		loglevel_enum m_LogLevel;
		FILE *m_fp;
//		CLock *m_lock;
		char *m_msg;
		clock_t m_timeInitial;
		
		/** fflush() doesn't work, so reopen for now */
		void Open();
		void Close();
		
	};
	
	extern CLogging Logging;

#ifndef WIN32
	#define Log(level, format, args...) Logging.Log_(__FILE__, __LINE__, level, format, ## args)
#else
	#define Log(level, format) Logging.Log_(__FILE__, __LINE__, level, format)
#endif

#endif

