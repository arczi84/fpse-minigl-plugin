/*
*/

#include "dll/dll.h"
#include <stdio.h>

#include "plugin.h"

DLL_SYMBOLS_DECL_BEGIN()
  DLL_SYMBOL_DECL(GPU_Open)
  DLL_SYMBOL_DECL(GPU_Close)
  DLL_SYMBOL_DECL(GP0_Read)
  DLL_SYMBOL_DECL(GP1_Read)
  DLL_SYMBOL_DECL(GP0_Write)
  DLL_SYMBOL_DECL(GP1_Write)
  DLL_SYMBOL_DECL(GPU_Update)
  DLL_SYMBOL_DECL(GPU_DmaExec)
  DLL_SYMBOL_DECL(GPU_DmaChain)
  DLL_SYMBOL_DECL(GPU_DmaOut)
  DLL_SYMBOL_DECL(GPU_DmaIn)
  DLL_SYMBOL_DECL(GPU_ScreenShot)
  DLL_SYMBOL_DECL(GPU_Configure)
  DLL_SYMBOL_DECL(GPU_About)
  DLL_SYMBOL_DECL(GPU_LoadState)
  DLL_SYMBOL_DECL(GPU_SaveState)
  DLL_SYMBOL_DECL(GPU_InfoDisplay)
  DLL_SYMBOL_DECL(GPU_DrawBmp)
  DLL_SYMBOL_DECL(GPU_DmaWait)
  DLL_SYMBOL_DECL(GPU_DrawCursor)
DLL_SYMBOLS_DECL_END()

#define LIBAUTO_GRAPHICS      1
#define LIBAUTO_INTUITION     1
#define LIBAUTO_MUIMASTER     1
#define LIBAUTO_ASL           1
#include "dll/libauto.h"

int dll_setup(void)
{
	if (libauto_open() == LIBAUTO_OK)
		return DLL_RESULT_OK;
	else
		return DLL_RESULT_SETUP_FAILED;
}

void dll_cleanup(void)
{
	libauto_close();
}

