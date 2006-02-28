/* 
	VRAM manager for the Sony PSP.
	Copyright (C) 2005 Jesper Sandberg

	
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
#ifndef _JSAVRAMMANAGER_
#define _JSAVRAMMANAGER_

#include <list>
using namespace std;

class jsaVRAMManager
{
public:
	jsaVRAMManager() {};
	~jsaVRAMManager() {};

	static void jsaVRAMManagerInit(unsigned long buffersize);
	static void *jsaVRAMManagerMalloc(unsigned long size);

private:
	static bool m_initialized;
	static unsigned long m_vram_start;
	static unsigned long m_vram_size;
	static unsigned long m_vram_free;
	static unsigned long m_vram_offset;
	static unsigned long m_systemoffset;
};

#endif
