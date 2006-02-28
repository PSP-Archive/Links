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

#include <stdio.h>
#include <PSPApp.h>
#include <TextUI3D.h>

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspge.h>

#include "jsaVRAMManager.h"


// Initialize static members
bool 		jsaVRAMManager::m_initialized	= false;
unsigned long	jsaVRAMManager::m_vram_start	= 0;
unsigned long	jsaVRAMManager::m_vram_size	= 0;
unsigned long	jsaVRAMManager::m_vram_free	= 0;
unsigned long	jsaVRAMManager::m_vram_offset	= 0;
unsigned long	jsaVRAMManager::m_systemoffset	= 0;


void jsaVRAMManager::jsaVRAMManagerInit(unsigned long buffersize)
{
	m_vram_start	= (unsigned long)sceGeEdramGetAddr();
	m_vram_size	= (unsigned long)sceGeEdramGetSize();
	m_vram_free	= 0;
	m_vram_offset	= 0;
	m_systemoffset	= 0;

	m_systemoffset	= buffersize;
	m_vram_free	= m_vram_size  - m_systemoffset;
	m_vram_offset	= m_vram_start + m_systemoffset;
	m_initialized	= true;
	ModuleLog(LOG_VERYLOW, "VM:VRAM manager initialized.");
}

void *jsaVRAMManager::jsaVRAMManagerMalloc(unsigned long size)
{
	unsigned long	pointer = 0;

	ModuleLog(LOG_VERYLOW, "VM:Allocating : %d", size);

	if (m_initialized && (size <= m_vram_free))
	{
		pointer = m_vram_offset;

		/* Update internal VRAM pointers */
		m_vram_offset	+= size;
		m_vram_free 	-= size;
		ModuleLog(LOG_VERYLOW, "VM:VRAM Left : %d", m_vram_free);
	}
	else
	{
		ModuleLog(LOG_ERROR, "VM:Out of VRAM..");
	}
	return (void *)pointer;
}
