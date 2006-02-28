/*
 * PSP Software Development Kit - http://www.pspdev.org
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in PSPSDK root for details.
 *
 * main.c - Simple PRX example.
 *
 * Copyright (c) 2005 James Forshaw <tyranid@gmail.com>
 *
 * $Id: main.c 1531 2005-12-07 18:27:12Z tyranid $
 */
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <PSPApp.h>
#include <PSPSound.h>
#include <TextUI.h>

PSP_MODULE_INFO("TEXTUI", 0, 1, 1);
PSP_NO_CREATE_MAIN_THREAD();
PSP_HEAP_SIZE_KB(3072);

extern "C" 
{
	int module_stop(int args, void *argp);
}

int main(int argc, char **argv)
{
	return 0;
}

IPSPRadio_UI *ModuleStartUI()
{
	ModuleLog(LOG_LOWLEVEL, "TextUI- ModuleStartUI()");

	ModuleLog(LOG_LOWLEVEL, "TextUI- _global_impure_ptr=%p, _impure_ptr=%p", _global_impure_ptr, _impure_ptr);

	return new CTextUI();
}

void* getModuleInfo(void)
{
	return (void *) &module_info;
}

int module_stop(int args, void *argp)
{
	return 0;
}
