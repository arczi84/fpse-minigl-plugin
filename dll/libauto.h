/*
 * libauto.h
 *
 * Define one or several of these to have the corresponding libraries opened:
 *
 *  . LIBAUTO_AMIGAINPUT    [0|1]
 *  . LIBAUTO_ASL           [0|1]
 *  . LIBAUTO_BSDSOCKET     [0|1]
 *  . LIBAUTO_CYBERGRAPHICS [0|1]
 *  . LIBAUTO_GRAPHICS      [0|1]
 *  . LIBAUTO_ICON          [0|1]
 *  . LIBAUTO_INTUITION     [0|1]
 *  . LIBAUTO_KEYMAP        [0|1]
 *  . LIBAUTO_LAYERS        [0|1]
 *  . LIBAUTO_MUIMASTER     [0|1]
 *  . LIBAUTO_PICASSO96     [0|1]
 *  . LIBAUTO_WARP3D        [0|1]
 *  . LIBAUTO_LOWLEVEL      [0|1]
 *
 * If defined to 1 and the library can't be opened, a requester will be shown
 * and libauto will fail. If defined to 0 and the library can't be opened, it
 * isn't considered an error.
 *
 * Define this to have the libraries automatically opened before main():
 *
 *  . LIBAUTO_AUTO
 *
 * Define this to open ALL the libraries:
 *
 *  . LIBAUTO_ALL
 *
 * Notes:
 *
 *  . LIBAUTO_INTUITION is required to make the requester work.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <proto/exec.h>

#define LIBAUTO_OK   ( 0)
#define LIBAUTO_FAIL (-1)

#ifdef LIBAUTO_ALL
#define LIBAUTO_AMIGAINPUT    1
#define LIBAUTO_ASL           1
#if defined(__amigaos4__)
#define LIBAUTO_BSDSOCKET     1
#endif
#define LIBAUTO_CYBERGRAPHICS 1
#define LIBAUTO_GRAPHICS      1
#define LIBAUTO_ICON          1
#define LIBAUTO_INTUITION     1
#define LIBAUTO_KEYMAP        1
#define LIBAUTO_LAYERS        1
#define LIBAUTO_MUIMASTER     1
#if !defined(__AROS__)
#define LIBAUTO_PICASSO96     1
#define LIBAUTO_WARP3D        1
#endif
#define LIBAUTO_LOWLEVEL      1
#endif

#ifdef __amigaos4__
typedef struct libauto_iface_s
{
  const char *name;
  int min_ver;
  int opt_req;
  struct Interface **iface;
} libauto_iface_t;
#endif

typedef struct libauto_s
{
  const char *name;
  int min_ver;
  int opt_req;
  struct Library **lib;
#ifdef __amigaos4__
  libauto_iface_t *iface;
#endif
} libauto_t;

#ifdef __amigaos4__
  #define LIBAUTO_IFACE_DECL(name, ifacebase) \
    static libauto_iface_t name[] = { \
      { "main", 1, 1, (struct Interface **)(void *)&ifacebase }, \
      { NULL, 0, 0, NULL }, \
    }
#endif

#ifdef __amigaos4__
  #if defined(LIBAUTO_AMIGAINPUT)
    #include <proto/amigainput.h>
    struct Library *AIN_Base = NULL;
    struct AIN_IFace *IAIN = NULL;
    LIBAUTO_IFACE_DECL(amigainput_iface, IAIN);
  #endif
#endif

#if defined(LIBAUTO_ASL)
  #include <proto/asl.h>
  struct Library *AslBase = NULL;
  #ifdef __amigaos4__
    struct AslIFace *IAsl = NULL;
    LIBAUTO_IFACE_DECL(asl_iface, IAsl);
  #endif
#endif

#if defined(LIBAUTO_BSDSOCKET)
  #include <proto/socket.h>
  struct Library *SocketBase = NULL;
  #ifdef __amigaos4__
    struct SocketIFace *ISocket = NULL;
    LIBAUTO_IFACE_DECL(socket_iface, ISocket);
  #endif
#endif

#if defined(LIBAUTO_CYBERGRAPHICS)
  #include <proto/cybergraphics.h>
  struct Library *CyberGfxBase = NULL;
  #ifdef __amigaos4__
    struct CyberGfxIFace *ICyberGfx = NULL;
    LIBAUTO_IFACE_DECL(cybergraphics_iface, ICyberGfx);
  #endif
#endif

#if defined(LIBAUTO_GRAPHICS)
  #include <proto/graphics.h>
  #ifdef __amigaos4__
    struct Library *GfxBase = NULL;
    struct GraphicsIFace *IGraphics = NULL;
    LIBAUTO_IFACE_DECL(graphics_iface, IGraphics);
  #else
    struct GfxBase *GfxBase = NULL;
  #endif
#endif

#if defined(LIBAUTO_ICON)
  #include <proto/icon.h>
  struct Library *IconBase = NULL;
  #ifdef __amigaos4__
    struct IconIFace *IIcon = NULL;
    LIBAUTO_IFACE_DECL(icon_iface, IIcon);
  #endif
#endif

#if defined(LIBAUTO_INTUITION)
  #include <proto/intuition.h>
  #ifdef __amigaos4__
    struct Library *IntuitionBase = NULL;
    struct IntuitionIFace *IIntuition = NULL;
    LIBAUTO_IFACE_DECL(intuition_iface, IIntuition);
  #else
    struct IntuitionBase *IntuitionBase = NULL;
  #endif
#endif

#if defined(LIBAUTO_KEYMAP)
  #include <proto/keymap.h>
  struct Library *KeymapBase = NULL;
  #ifdef __amigaos4__
    struct KeymapIFace *IKeymap = NULL;
    LIBAUTO_IFACE_DECL(keymap_iface, IKeymap);
  #endif
#endif

#if defined(LIBAUTO_LAYERS)
  #include <proto/layers.h>
  struct Library *LayersBase = NULL;
  #ifdef __amigaos4__
    struct LayersIFace *ILayers = NULL;
    LIBAUTO_IFACE_DECL(layers_iface, ILayers);
  #endif
#endif

#if defined(LIBAUTO_MUIMASTER)
  #include <proto/muimaster.h>
  struct Library *MUIMasterBase = NULL;
  #ifdef __amigaos4__
    struct MUIMasterIFace *IMUIMaster = NULL;
    LIBAUTO_IFACE_DECL(muimaster_iface, IMUIMaster);
  #endif
#endif

#if defined(LIBAUTO_PICASSO96)
  #ifdef __amigaos4__
    #include <proto/Picasso96API.h>
  #else
    #include <proto/Picasso96.h>
  #endif
  struct Library *P96Base = NULL;
  #ifdef __amigaos4__
    struct P96IFace *IP96 = NULL;
    LIBAUTO_IFACE_DECL(picasso96_iface, IP96);
  #endif
#endif

#if defined(LIBAUTO_WARP3D)
  #ifdef __amigaos4__
    #include <proto/warp3d.h>
  #else
    #include <proto/Warp3D.h>
  #endif
  struct Library *Warp3DBase = NULL;
  #ifdef __amigaos4__
    struct Warp3DIFace *IWarp3D = NULL;
    LIBAUTO_IFACE_DECL(warp3d_iface, IWarp3D);
  #endif
#endif

#if defined(LIBAUTO_LOWLEVEL)
  #include <proto/lowlevel.h>
  struct Library *LowLevelBase = NULL;
  #ifdef __amigaos4__
    struct LowLevelIFace *ILowLevel = NULL;
    LIBAUTO_IFACE_DECL(lowlevel_iface, ILowLevel);
  #endif
#endif

#ifdef __amigaos4__
  #define LIBAUTO_LIB_DECL(libname, veros4, veros3, required, libbase, iface) \
    { libname, veros4, required, (struct Library **)(void *)libbase, iface }
#else
  #define LIBAUTO_LIB_DECL(libname, veros4, veros3, required, libbase, iface) \
    { libname, veros3, required, (struct Library **)(void *)libbase }
#endif

static libauto_t libauto[] = {
  #if defined(LIBAUTO_INTUITION) /* keep this first */
    LIBAUTO_LIB_DECL("intuition.library", 52, 39, LIBAUTO_INTUITION, &IntuitionBase, intuition_iface),
  #endif

  #if defined(LIBAUTO_ASL)
    LIBAUTO_LIB_DECL("asl.library", 52, 39, LIBAUTO_ASL, &AslBase, asl_iface),
  #endif

  #if defined(LIBAUTO_BSDSOCKET)
    LIBAUTO_LIB_DECL("bsdsocket.library", 4, 4, LIBAUTO_BSDSOCKET, &SocketBase, socket_iface),
  #endif

  #if defined(LIBAUTO_CYBERGRAPHICS)
    LIBAUTO_LIB_DECL("cybergraphics.library", 43, 40, LIBAUTO_CYBERGRAPHICS, &CyberGfxBase, cybergraphics_iface),
  #endif

  #if defined(LIBAUTO_GRAPHICS)
    LIBAUTO_LIB_DECL("graphics.library", 52, 39, LIBAUTO_GRAPHICS, &GfxBase, graphics_iface),
  #endif

  #if defined(LIBAUTO_ICON)
    LIBAUTO_LIB_DECL("icon.library", 52, 39, LIBAUTO_ICON, &IconBase, icon_iface),
  #endif

  #if defined(LIBAUTO_KEYMAP)
    LIBAUTO_LIB_DECL("keymap.library", 52, 39, LIBAUTO_KEYMAP, &KeymapBase, keymap_iface),
  #endif

  #if defined(LIBAUTO_LAYERS)
    LIBAUTO_LIB_DECL("layers.library", 52, 39, LIBAUTO_LAYERS, &LayersBase, layers_iface),
  #endif

  #if defined(LIBAUTO_MUIMASTER)
    LIBAUTO_LIB_DECL("muimaster.library", 11, 11, LIBAUTO_MUIMASTER, &MUIMasterBase, muimaster_iface),
  #endif

  #if defined(LIBAUTO_PICASSO96)
    LIBAUTO_LIB_DECL("Picasso96API.library", 2, 2, LIBAUTO_PICASSO96, &P96Base, picasso96_iface),
  #endif

  #if defined(LIBAUTO_WARP3D)
    LIBAUTO_LIB_DECL("Warp3D.library", 52, 4, LIBAUTO_WARP3D, &Warp3DBase, warp3d_iface),
  #endif

  #if defined(LIBAUTO_LOWLEVEL)
    LIBAUTO_LIB_DECL("lowlevel.library", 52, 39, LIBAUTO_LOWLEVEL, &LowLevelBase, lowlevel_iface),
  #endif

  #ifdef __amigaos4__ /* keep OS version specifik libraries last */
    #if defined(LIBAUTO_AMIGAINPUT)
      LIBAUTO_LIB_DECL("AmigaInput.library", 51, 0, LIBAUTO_AMIGAINPUT, &AIN_Base, amigainput_iface),
    #endif
  #endif

  LIBAUTO_LIB_DECL(NULL, 0, 0, 0, NULL, NULL)
};

static void libauto_close(void)
{
  libauto_t *lib;
  for (lib = libauto; lib->name != NULL; lib++) {
#ifdef __amigaos4__
    libauto_iface_t *iface;
    for (iface = lib->iface; iface->name != NULL; iface++) {
      if (*iface->iface != NULL) {
        DropInterface(*iface->iface);
        *iface->iface = NULL;
      }
    }
#endif
    if (*lib->lib != NULL) {
      CloseLibrary(*lib->lib);
      *lib->lib = NULL;
    }
  }
}

#if defined(LIBAUTO_INTUITION)
#ifdef __amigaos4__
  #define libauto_req(format, args...) ({ \
    if ((IntuitionBase != NULL) && (IIntuition != NULL)) { \
      struct EasyStruct es = { sizeof(struct EasyStruct), 0, (char *)"Error", (char *)format, (char *)"Ok", NULL, NULL }; \
      EasyRequest(NULL, &es, NULL, args); \
    } })
#else
  #define libauto_req(format, args...) ({ \
    if (IntuitionBase != NULL) { \
      struct EasyStruct es = { sizeof(struct EasyStruct), 0, (char *)"Error", (char *)format, (char *)"Ok" }; \
      EasyRequest(NULL, &es, NULL, args); \
    } })
#endif
#else
  #define libauto_req(format, args...)
#endif

static int libauto_open(void)
{
  libauto_t *lib;
  for (lib = libauto; lib->name != NULL; lib++) {
    *lib->lib = OpenLibrary(lib->name, lib->min_ver);
    if ((*lib->lib == NULL) && (lib->opt_req)) {
      libauto_req("Failed to open '%s' version %ld or later.", (ULONG)lib->name, lib->min_ver);
      libauto_close();
      return LIBAUTO_FAIL;
    }
#ifdef __amigaos4__
    if (*lib->lib != NULL) {
      libauto_iface_t *iface;
      for (iface = lib->iface; iface->name != NULL; iface++) {
        *iface->iface = GetInterface(*lib->lib, iface->name, iface->min_ver, NULL);
        if ((*iface->iface == NULL) && (iface->opt_req)) {
          libauto_req("Failed to open '%s' interface '%s' version %ld or later.", (ULONG)lib->name, (ULONG)iface->name, iface->min_ver);
          libauto_close();
          return LIBAUTO_FAIL;
        }
      }
    }
#endif
  }
  return LIBAUTO_OK;
}

#ifdef LIBAUTO_AUTO

void libauto_init(void) __attribute__ ((constructor));
void libauto_exit(void) __attribute__ ((destructor));

void libauto_init(void)
{
  if (libauto_open() != LIBAUTO_OK) {
    exit(0);
  }
}

void libauto_exit(void)
{
  libauto_close();
}

#endif
