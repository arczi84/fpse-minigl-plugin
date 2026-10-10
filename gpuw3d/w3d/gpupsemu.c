
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "plugin.h"

#include <proto/exec.h>
#include <proto/intuition.h>
#include <libraries/mui.h>
#include <clib/alib_protos.h>
#include <proto/muimaster.h>
#include <proto/asl.h>
#include <proto/graphics.h>
#include <proto/dos.h>

#if defined(__AROS__) || defined (__morphos__)
#include <proto/alib.h>
#endif

#ifndef MAKE_ID
#define MAKE_ID(a,b,c,d) ((ULONG) (a)<<24 | (ULONG) (b)<<16 | (ULONG) (c)<<8 | (ULONG) (d))
#endif

#include "version.h"

static FPSEAmigaAbout info = {
    FPSE_IFACE_MAGIC,
    FPSE_IFACE_VERSION,
    FPSE_GPU,
    "gpuw3d",
    (char *)VERSION,
    "AmiDog",
    "Warp3D GPU plugin.",
};

//
extern int amiga_open(void);
extern void input_update(void);
extern void amiga_close(void);
//

extern long GPUinit(void);
extern long GPUshutdown(void);
extern long GPUopen(void);
extern long GPUclose(void);
extern unsigned long GPUreadStatus(void);
extern void GPUwriteStatus(unsigned long);
extern unsigned long GPUreadData(void);
extern void GPUwriteData(unsigned long);
extern void GPUupdateLace(void);
extern long GPUdmaChain(unsigned long *, unsigned long);
extern void GPUwriteDataMem(unsigned long *, int);
extern void GPUreadDataMem(unsigned long *, int);
extern long GPUconfigure(void);
extern long GPUtest(void);
extern long GPUabout(void);
extern long PSEgetLibName(void);
extern long PSEgetLibType(void);
extern long PSEgetLibVersion(void);
extern void GPUdisplayText(char *);
extern void GPUcursor(int iPlayer,int x,int y);
extern void GPUgetScreenPic(unsigned char * pMem);

static void (*FlushRec)();
static int core_initialized, display_open;
extern void w3d_gpu_discard(void);
extern void TCacheInvalidate(void);
extern void w3d_gpu_vram_changed(int, int, int, int);

void GPU_Update(void)
{
    input_update();
    GPUupdateLace();
}

UINT32 GP1_Read(void)
{
    return GPUreadStatus();
}

UINT32 GP0_Read(void)
{
    return GPUreadData();
}

void GP0_Write(UINT32 data)
{
    GPUwriteData(data);
}

void GP1_Write(UINT32 code)
{
    GPUwriteStatus(code);
}

UINT8 *ram;

extern int amiga_use_screen, frame_skip, scale_gfx, speed_limit, hide_pointer, resolution, screenmode, texred, texcache, texfilter, alpha_threshold;
extern int plugin_is_master, plugin_is_slave;

static void set_defaults(UINT32 *par)
{
    FPSEAmiga *spu = (FPSEAmiga *)par;
    plugin_is_master = plugin_is_slave = 0;

    if (spu->Flags & CORE_IS_MASTER)
    {
      plugin_is_master = 1;
    }

    if (spu->Flags & CORE_IS_SLAVE)
    {
      plugin_is_slave = 1;
    }

    amiga_use_screen = 0;
    frame_skip = 1;
    scale_gfx = 2; // SCALING_EXP
    speed_limit = 1;
    hide_pointer = 0;
    screenmode = 0;
    texred = 0;
    texcache = 256;
    texfilter = 0;
    alpha_threshold = 50;

    if (plugin_is_master)
    {
      resolution = 1; /* 320x240 */
    }
    else if (plugin_is_slave)
    {
      resolution = 1; /* 320x240 */
    }
    else
    {
      resolution = 3; /* 640x480 */
    }
}

static void read_cfg(UINT32 *par)
{
    FPSEAmiga *spu = (FPSEAmiga *)par;
    char tmp[256];

    set_defaults(par);

    if (spu->ReadCfg("GPUW3D", "FullScreen", tmp) == FPSE_OK) {
      amiga_use_screen = atoi(tmp);
    }
    if (spu->ReadCfg("GPUW3D", "Resolution", tmp) == FPSE_OK) {
      resolution = atoi(tmp);
    }
    if (spu->ReadCfg("GPUW3D", "Screenmode", tmp) == FPSE_OK) {
      screenmode = atoi(tmp);
    }
    if (spu->ReadCfg("GPUW3D", "Scale", tmp) == FPSE_OK) {
      scale_gfx = atoi(tmp);
    }
    if (spu->ReadCfg("GPUW3D", "FrameSkip", tmp) == FPSE_OK) {
      frame_skip = atoi(tmp);
    }
    if (spu->ReadCfg("GPUW3D", "SpeedLimit", tmp) == FPSE_OK) {
      speed_limit = atoi(tmp);
    }
    if (spu->ReadCfg("GPUW3D", "HidePointer", tmp) == FPSE_OK) {
      hide_pointer = atoi(tmp);
    }
    if (spu->ReadCfg("GPUW3D", "TextureReduction", tmp) == FPSE_OK) {
      texred = atoi(tmp);
    }
    if (spu->ReadCfg("GPUW3D", "TextureCache", tmp) == FPSE_OK) {
      texcache = atoi(tmp);
    }
    if (spu->ReadCfg("GPUW3D", "TextureFilter", tmp) == FPSE_OK) {
      texfilter = atoi(tmp);
    }
    if (spu->ReadCfg("GPUW3D", "AlphaThreshold", tmp) == FPSE_OK) {
      alpha_threshold = atoi(tmp);
    }
}

static void write_cfg(UINT32 *par)
{
    FPSEAmiga *spu = (FPSEAmiga *)par;
    char tmp[256];

    sprintf(tmp, "%d", amiga_use_screen);
    spu->WriteCfg("GPUW3D", "FullScreen", tmp);
    sprintf(tmp, "%d", resolution);
    spu->WriteCfg("GPUW3D", "Resolution", tmp);
    sprintf(tmp, "%d", screenmode);
    spu->WriteCfg("GPUW3D", "ScreenMode", tmp);
    sprintf(tmp, "%d", scale_gfx);
    spu->WriteCfg("GPUW3D", "Scale", tmp);
    sprintf(tmp, "%d", frame_skip);
    spu->WriteCfg("GPUW3D", "FrameSkip", tmp);
    sprintf(tmp, "%d", speed_limit);
    spu->WriteCfg("GPUW3D", "SpeedLimit", tmp);
    sprintf(tmp, "%d", hide_pointer);
    spu->WriteCfg("GPUW3D", "HidePointer", tmp);
    sprintf(tmp, "%d", texred);
    spu->WriteCfg("GPUW3D", "TextureReduction", tmp);
    sprintf(tmp, "%d", texcache);
    spu->WriteCfg("GPUW3D", "TextureCache", tmp);
    sprintf(tmp, "%d", texfilter);
    spu->WriteCfg("GPUW3D", "TextureFilter", tmp);
    sprintf(tmp, "%d", alpha_threshold);
    spu->WriteCfg("GPUW3D", "AlphaThreshold", tmp);
}

int GPU_Open(UINT32 *par)
{
    FPSEAmiga *spu = (FPSEAmiga *)par;
    if (!spu) return FPSE_ERR;
    if (core_initialized) GPU_Close();

    ram = spu->SystemRam;
    FlushRec = spu->FlushRec;

    read_cfg(par);
    if (GPUinit()) return FPSE_ERR;
    core_initialized = 1;
    if (amiga_open() != FPSE_OK) {
        GPU_Close();
        return FPSE_ERR;
    }
    display_open = 1;

    if (GPUopen()) {
        GPU_Close();
        return FPSE_ERR;
    }

    return FPSE_OK;
}

void GPU_Close(void)
{
    if (display_open) GPUclose();
    amiga_close();
    display_open = 0;
    if (core_initialized) GPUshutdown();
    core_initialized = 0;
    ram = NULL;
}

void GPU_ScreenShot(char *path)
{
}        

int GPU_DmaChain(UINT32 adr)
{
    GPUdmaChain((unsigned long *)ram,adr&0x1fffff);

    return 0;
}

void GPU_DmaIn(UINT32 adr,int size)
{
    UINT32 *ram32 = (UINT32 *)(ram+(adr & 0x1FFFFF));

    FlushRec(adr,adr+size*4);
    GPUreadDataMem(ram32,size);
}

void GPU_DmaOut(UINT32 adr,int size)
{
    UINT32 *ram32 = (UINT32 *)(ram+(adr & 0x1FFFFF));

    GPUwriteDataMem(ram32,size);
}

void GPU_DmaExec(UINT32 adr,UINT32 bcr,UINT32 chcr)
{
    int size;
    UINT32 *ram32 = (UINT32 *)(ram+(adr & 0x1FFFFF));

    size = (bcr>>16)*(bcr&0xffff);

    switch(chcr){
    case 0x01000200:
        FlushRec(adr,adr+size*4);
        GPUreadDataMem(ram32,size);
        break;
    case 0x01000201:
        GPUwriteDataMem(ram32,size);
        break;
    case 0x01000401:
        GPUdmaChain((UINT32*)ram,adr&0x1fffff);
        break;
    case 0x00000200:
    case 0x00000201:
    case 0x00000401: // disable dma
        break;
    default: printf("gpudma unknown %08x\n",(int)chcr);
    }
}

typedef struct GPUFREEZETAG
{
 unsigned long ulFreezeVersion;      // should be always 1 for now (set by main emu)
 unsigned long ulStatus;             // current gpu status
 unsigned long ulControl[256];       // latest control register values
 unsigned char psxVRam[1024*1024*2]; // current VRam image (full 2 MB for ZN)
} GPUFreeze_t;

extern long GPUfreeze(unsigned long ulGetFreezeData,GPUFreeze_t * pF);

void GPU_LoadState(GPU_State *gpustat)
{
    GPUFreeze_t *pF;

    if (gpustat == NULL)
        return;

    pF = (GPUFreeze_t *)malloc(sizeof(GPUFreeze_t));
    if (pF == NULL) return;

    pF->ulFreezeVersion = 1;
    pF->ulStatus = gpustat->status;
    memcpy(pF->ulControl,gpustat->control,256*sizeof(unsigned long));
    memcpy(pF->psxVRam,gpustat->vram,1024*1024);

    w3d_gpu_discard();
    GPUfreeze(0, pF);
    TCacheInvalidate();
    w3d_gpu_vram_changed(0, 0, 1024, 512);
    free(pF);
}

void *GPU_SaveState(fpse_save_helper_t *helper)
{
    GPUFreeze_t *pF;
    GPU_State *gpustat;
    fpse_save_malloc_t host_malloc;

    if (helper == NULL)
        return NULL;

    pF = (GPUFreeze_t *)malloc(sizeof(GPUFreeze_t));
    if (pF == NULL) return NULL;

    pF->ulFreezeVersion = 1;
    if (GPUfreeze(1, pF) == 0) {
        free(pF);
        return NULL;
    }
    host_malloc = helper->save_malloc;
    gpustat = (GPU_State *)host_malloc(sizeof(GPU_State)+1024*1024);
    if (gpustat == NULL) {
        free(pF);
        return NULL;
    }
    gpustat->vram = ((UINT8 *)gpustat) + sizeof(GPU_State);
    gpustat->version = 0;
    gpustat->status = pF->ulStatus;
    memcpy(gpustat->control,pF->ulControl,256*sizeof(unsigned long));
    memcpy(gpustat->vram,pF->psxVRam,1024*1024);
    free(pF);

    gpustat->screenpic = (UINT8 *)host_malloc(128*96*3);
    if (gpustat->screenpic != NULL) {
        GPUgetScreenPic(gpustat->screenpic);
    }

    return gpustat;
}

void GPU_DrawBmp(GPU_BmpType *bm, int x, int y)
{
}

int GPU_DmaWait(void)
{
    return 0;
}

void GPU_DrawCursor(int Player, int X, int Y)
{
  GPUcursor(Player,X,Y);
}

static char *id2name(void)
{
  static struct NameInfo ni;
  ni.Name[0] = '\0';
  GetDisplayInfoData(NULL, (UBYTE *)&ni, sizeof(ni), DTAG_NAME, screenmode);
  return (char *)ni.Name;
}

static ULONG Start(REG(a0, struct Hook *hook), REG(a2, Object *obj), REG(a1, APTR arg))
{
  return TRUE;
}

static ULONG Stop(REG(a0, struct Hook *hook), REG(a2, Object *obj), REG(a1, struct ScreenModeRequester *req))
{
  screenmode = req->sm_DisplayID;
  set(obj, MUIA_String_Contents, id2name());
  return TRUE;
}

int GPU_Configure(UINT32 *par)
{
  BOOL running = TRUE;
  ULONG signals;

  #define BTN_OK (32)

  APTR app, window, ok, cancel, Ofullscreen, Oscale, Oframeskip, Ospeed_limit, Ohide_pointer, Oresolution, Omode, Otexred, Otexcache, Otexfilter, Oalphathreshold;

#ifdef __morphos__
  static const struct Hook StartHook = { { NULL,NULL },(VOID *)HookEntry,(VOID *)Start,NULL };
  static const struct Hook StopHook = { { NULL,NULL },(VOID *)HookEntry,(VOID *)Stop,NULL };
#else
  static const struct Hook StartHook = { { NULL,NULL },(VOID *)Start,NULL,NULL };
  static const struct Hook StopHook = { { NULL,NULL },(VOID *)Stop,NULL,NULL };
#endif

     char *CY_ResolutionContent[9];
     char *CY_CD_MODEContent[3];
     char *CY_ScaleContent[4];
     char *CY_TEXREDContent[4];
     char *CY_TEXFILTERContent[3];

  read_cfg(par);

     CY_TEXREDContent[0] = "1/1 (256x256)";
     CY_TEXREDContent[1] = "1/4 (128x128)";
     CY_TEXREDContent[2] = "1/16 (64x64)";
     CY_TEXREDContent[3] = NULL;

     CY_TEXFILTERContent[0] = "Nearest";
     CY_TEXFILTERContent[1] = "Linear";
     CY_TEXFILTERContent[2] = NULL;

     CY_CD_MODEContent[0] = "Off";
     CY_CD_MODEContent[1] = "On";
     CY_CD_MODEContent[2] = NULL;

     CY_ScaleContent[0] = "Off";
     CY_ScaleContent[1] = "Scale MDECs only";
     CY_ScaleContent[2] = "Experimental scaling";
     CY_ScaleContent[3] = NULL;

     CY_ResolutionContent[0] = "Actual / ScreenMode";
     CY_ResolutionContent[1] = "320x240";
     CY_ResolutionContent[2] = "512x384";
     CY_ResolutionContent[3] = "640x480";
     CY_ResolutionContent[4] = "800x600";
     CY_ResolutionContent[5] = "1024x768";
     CY_ResolutionContent[6] = "1280x1024";
     CY_ResolutionContent[7] = "1600x1200";
     CY_ResolutionContent[8] = NULL;

	app = ApplicationObject,
	MUIA_Application_Author, "Mathias Roslund",
	MUIA_Application_Base, "GPUW3D",
	MUIA_Application_Title, "GPUW3D",
	MUIA_Application_Version, VERSTAG,
	MUIA_Application_Copyright, "Mathias Roslund",
	MUIA_Application_Description, "GPUW3D",
	SubWindow, window = WindowObject,
	MUIA_Window_Title, "GPUW3D",
	MUIA_Window_ID, MAKE_ID('0', 'W', 'I', 'N'),
	WindowContents, VGroup,
	Child, ColGroup(2),

     Child, TextObject,
          MUIA_Text_PreParse, "\033r",
          MUIA_Text_Contents, "FullScreen",
          MUIA_InnerLeft, 0,
          MUIA_InnerRight, 0,
     End,

     Child, Ofullscreen = CycleObject,
          MUIA_Cycle_Entries, CY_CD_MODEContent,
          MUIA_Cycle_Active, amiga_use_screen,
     End,

     Child, TextObject,
          MUIA_Text_PreParse, "\033r",
          MUIA_Text_Contents, "Resolution",
          MUIA_InnerLeft, 0,
          MUIA_InnerRight, 0,
     End,

     Child, Oresolution = CycleObject,
          MUIA_Cycle_Entries, CY_ResolutionContent,
          MUIA_Cycle_Active, resolution,
     End,

     Child, TextObject,
          MUIA_Text_PreParse, "\033r",
          MUIA_Text_Contents, "ScreenMode",
          MUIA_InnerLeft, 0,
          MUIA_InnerRight, 0,
     End,

     Child, PopaslObject,
          MUIA_Popasl_Type, ASL_ScreenModeRequest,
          MUIA_Popasl_StartHook, &StartHook,
          MUIA_Popasl_StopHook, &StopHook,
          MUIA_Popstring_String, Omode = String("", 1024),
          MUIA_Popstring_Button, PopButton(MUII_PopUp),
          ASLSM_MinDepth, 15,
     End,

     Child, TextObject,
          MUIA_Text_PreParse, "\033r",
          MUIA_Text_Contents, "Scale",
          MUIA_InnerLeft, 0,
          MUIA_InnerRight, 0,
     End,

     Child, Oscale = CycleObject,
          MUIA_Cycle_Entries, CY_ScaleContent,
          MUIA_Cycle_Active, scale_gfx,
     End,

     Child, TextObject,
          MUIA_Text_PreParse, "\033r",
          MUIA_Text_Contents, "FrameSkip",
          MUIA_InnerLeft, 0,
          MUIA_InnerRight, 0,
     End,

     Child, Oframeskip = SliderObject,
          MUIA_Frame, MUIV_Frame_Prop,
          MUIA_Slider_Min, 1,
          MUIA_Slider_Max, 10,
          MUIA_Slider_Level, 0,
          MUIA_Numeric_Value, frame_skip,
     End,

     Child, TextObject,
          MUIA_Text_PreParse, "\033r",
          MUIA_Text_Contents, "Speed Limit",
          MUIA_InnerLeft, 0,
          MUIA_InnerRight, 0,
     End,

     Child, Ospeed_limit = CycleObject,
          MUIA_Cycle_Entries, CY_CD_MODEContent,
          MUIA_Cycle_Active, speed_limit,
     End,

     Child, TextObject,
          MUIA_Text_PreParse, "\033r",
          MUIA_Text_Contents, "Hide Pointer",
          MUIA_InnerLeft, 0,
          MUIA_InnerRight, 0,
     End,

     Child, Ohide_pointer = CycleObject,
          MUIA_Cycle_Entries, CY_CD_MODEContent,
          MUIA_Cycle_Active, hide_pointer,
     End,

     Child, TextObject,
          MUIA_Text_PreParse, "\033r",
          MUIA_Text_Contents, "Texture Reduction",
          MUIA_InnerLeft, 0,
          MUIA_InnerRight, 0,
     End,

     Child, Otexred = CycleObject,
          MUIA_Cycle_Entries, CY_TEXREDContent,
          MUIA_Cycle_Active, texred,
     End,

     Child, TextObject,
          MUIA_Text_PreParse, "\033r",
          MUIA_Text_Contents, "Texture Cache",
          MUIA_InnerLeft, 0,
          MUIA_InnerRight, 0,
     End,

     Child, Otexcache = SliderObject,
          MUIA_Frame, MUIV_Frame_Prop,
          MUIA_Slider_Min, 64,
          MUIA_Slider_Max, 256,
          MUIA_Slider_Level, 0,
          MUIA_Numeric_Value, texcache,
     End,

     Child, TextObject,
          MUIA_Text_PreParse, "\033r",
          MUIA_Text_Contents, "Texture Filter",
          MUIA_InnerLeft, 0,
          MUIA_InnerRight, 0,
     End,

     Child, Otexfilter = CycleObject,
          MUIA_Cycle_Entries, CY_TEXFILTERContent,
          MUIA_Cycle_Active, texfilter,
     End,

     Child, TextObject,
          MUIA_Text_PreParse, "\033r",
          MUIA_Text_Contents, "Alpha Threshold",
          MUIA_InnerLeft, 0,
          MUIA_InnerRight, 0,
     End,

     Child, Oalphathreshold = SliderObject,
          MUIA_Frame, MUIV_Frame_Prop,
          MUIA_Slider_Min, 0,
          MUIA_Slider_Max, 100,
          MUIA_Slider_Level, 0,
          MUIA_Numeric_Value, alpha_threshold,
     End,

	End,

	Child, HGroup,
	Child, ok = TextObject,
	ButtonFrame,
	MUIA_Background, MUII_ButtonBack,
	MUIA_Text_Contents, "Ok",
	MUIA_Text_PreParse, "\033c",
	MUIA_InputMode, MUIV_InputMode_RelVerify,
	End,
	Child, cancel = TextObject,
	ButtonFrame,
	MUIA_Background, MUII_ButtonBack,
	MUIA_Text_Contents, "Cancel",
	MUIA_Text_PreParse, "\033c",
	MUIA_InputMode, MUIV_InputMode_RelVerify,
	End,

	End,
	End,
	End,
	End;

      if (app) {
	DoMethod(window,
	MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
	app,
	2,
	MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit
	);

	DoMethod(cancel,
	MUIM_Notify, MUIA_Pressed, FALSE,
	app,
	2,
	MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit
	);

	DoMethod(ok,
	MUIM_Notify, MUIA_Pressed, FALSE,
	app,
	2,
	MUIM_Application_ReturnID, BTN_OK
	);
        set(Omode, MUIA_String_Contents, id2name());
        set(window, MUIA_Window_Open, TRUE);
        while (running) {
          unsigned long retval = DoMethod(app, MUIM_Application_Input, &signals);
          switch (retval) {
            case MUIV_Application_ReturnID_Quit:
              running = FALSE;
              break;

            case BTN_OK: {
              get(Ofullscreen, MUIA_Cycle_Active, &amiga_use_screen);
              get(Oresolution, MUIA_Cycle_Active, &resolution);
              // screenmode already has its value
              get(Oscale, MUIA_Cycle_Active, &scale_gfx);
              get(Oframeskip, MUIA_Numeric_Value, &frame_skip);
              get(Ospeed_limit, MUIA_Cycle_Active, &speed_limit);
              get(Ohide_pointer, MUIA_Cycle_Active, &hide_pointer);
              get(Otexred, MUIA_Cycle_Active, &texred);
              get(Otexcache, MUIA_Numeric_Value, &texcache);
              get(Otexfilter, MUIA_Cycle_Active, &texfilter);
              get(Oalphathreshold, MUIA_Numeric_Value, &alpha_threshold);
              write_cfg(par);
              running = FALSE;
              } break;

            default:
              break;
          }
          if (running && signals) {
            Wait(signals);
          }
        }
        MUI_DisposeObject(app);
      }  

  #undef BTN_OK

    return FPSE_OK;
}

void GPU_About(UINT32 *par)
{
    memcpy(par, &info, sizeof(FPSEAmigaAbout));
}
