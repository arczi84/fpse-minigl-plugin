#ifndef _FPSE_EXTRA_H_
#define	_FPSE_EXTRA_H_

/* Single version of JOY interface */
int  JOY_Open(UINT32 *joy);
void JOY_Close(void);
void JOY_SetOutputBuffer(UINT8 *buf);
int  JOY_StartPoll(void);
int  JOY_Poll(int outbyte);
int  JOY_SetAnalog(int mode);
void JOY_LoadState(JOY_State *state);
void JOY_SaveState(JOY_State *state);
int  JOY_Configure(UINT32 *par);
void JOY_About(UINT32 *par);

/* Extra GPU functions */
#define GPU_GUN_PLAYER_MIN 0
#define GPU_GUN_PLAYER_MAX 7
#define GPU_GUN_X_MIN 0
#define GPU_GUN_X_MAX 511
#define GPU_GUN_Y_MIN 0
#define GPU_GUN_Y_MAX 255
typedef void (*pGPU_DrawCursor)(int Player, int X, int Y);
void GPU_DrawCursor(int Player, int X, int Y);

#endif /* _FPSE_EXTRA_H_ */
