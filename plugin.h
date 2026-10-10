#ifndef _FPSE_PLUGIN_H_
#define _FPSE_PLUGIN_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "type.h"
#include "endian.h"
#include "sdk.h"
#include "extra.h"

// Plugin interface magic
#define FPSE_IFACE_MAGIC ((('F') << 24) | (('P') << 16) | (('S') << 8) | ('E'))

// Plugin interface version
#define FPSE_IFACE_VERSION (4)

// Plugin type
#define FPSE_GPU (1)
#define FPSE_SPU (2)
#define FPSE_JOY (3)
#define FPSE_CD  (4)
#define FPSE_PAR (5)

// For internal use
#define FPSE_PLUGIN_TYPE_MIN (1)
#define FPSE_PLUGIN_TYPE_MAX (5)

// New MDEC from GPU plugin
typedef struct {
    UINT16 *(*Decode16)(UINT16 *mdec_rl, UINT16 *image, int size);
    UINT16 *(*Decode24)(UINT16 *mdec_rl, UINT16 *image, int size);
    void    (*IqTab_Init)(UINT8 *iq);
    int     (*MDetect)(void);
    char     *NameStamp;
} MDEC_Export;

// Bit masks for flags field
#define CORE_IS_MASTER   (1 << 0) // For link-mode
#define CORE_IS_SLAVE    (1 << 1) // For link-mode
#define GPU_USE_NEW_MDEC (1 << 2) // Plugin may set this

// Pad number
// ,-------,        ,-------,
// | Port0 |        | Port1 |
// '-------'        '-------'
//    |                |
//    0                1
//    | (opt)          | (opt)
// ,----------,     ,----------,
// | Multitap |     | Multitap |
// '----------'     '----------'
//  |  |  |  |       |  |  |  |
//  00 01 02 03      10 11 12 13

// Main struct for initialization
typedef struct {
    UINT8        *SystemRam;   // Pointer to PSX system RAM
    UINT32        Flags;       // Flags to/from plugin
    UINT32       *IrqPulsePtr; // Pointer to interrupt pending register
    MDEC_Export  *MDecAltern;  // Alternative MDEC engine
    UINT8         PadNr[4];    // Pad number, see above
    void        (*DrawCursor)(int Player, int X, int Y);              // Draw a GUN cursor
    int         (*ReadCfg)(char *Section, char *Entry, char *Value);  // Read an item from INI
    int         (*WriteCfg)(char *Section, char *Entry, char *Value); // Write an item to INI
    void        (*FlushRec)(UINT32 StartPC, UINT32 EndPC);            // Notify RAM changes
} FPSEAmiga;

// Information about a plugin
typedef struct {
    UINT32 PluginIFaceMagic;   // Interface magic
    UINT16 PluginIFaceVersion; // Interface version
    UINT16 PluginType;         // Type
    char  *Name;               // Name
    char  *Version;            // Version
    char  *Author;             // Author
    char  *Description;        // Description
} FPSEAmigaAbout;

#endif /* _FPSE_PLUGIN_H_ */
