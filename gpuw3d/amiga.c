#include "stdafx.h"
#include "externals.h"
#include "gpu.h"

/* fps.c */
int            UseFrameSkip=0;

/* from zn.c */
unsigned long dwGPUVersion=0;
int           iGPUHeight=512;
int           iGPUHeightMask=511;
int           GlobalTextIL=0;
int           iTileCheat=0;

/* from draw.c */
extern void amiga_refresh(unsigned short *, int, int, int, int, int, double);
void DoBufferSwap(void)
{
  amiga_refresh(psxVuw,
                PSXDisplay.DisplayPosition.x,
                PSXDisplay.DisplayPosition.y,
                PreviousPSXDisplay.Range.x1,
                PreviousPSXDisplay.DisplayMode.y,
                PSXDisplay.RGB24,
                PSXDisplay.PAL ? (50.0f) : (60.0f/1.001f));
}

extern void amiga_clear(void);

void DoClearScreenBuffer(void){ amiga_clear(); } // fill output display with black pixels
void DoClearFrontBuffer(void){ amiga_clear(); } // same, but doesn't clear any scale buffer

/* from cfg.c */
#define MAKELONG(low,high)     ((unsigned long)(((unsigned short)(low)) | (((unsigned long)((unsigned short)(high))) << 16)))

extern int frame_skip;

void ReadConfig(void)
{
 // defaults
 iResX=640;iResY=480;
 iWinSize=MAKELONG(iResX,iResY);
 iColDepth=16;
 iWindowMode=1;
 iUseScanLines=0;
 UseFrameSkip=0;
 dwCfgFixes=0;
 iUseFixes=0;
 iUseNoStretchBlt=1;
 iUseDither=0;
 iShowFPS=0;

 if (frame_skip > 1) {
   UseFrameSkip = 1;
 } else {
   UseFrameSkip = 0;
 }

 SetFixes();
}

void WriteConfig(void)
{
}
