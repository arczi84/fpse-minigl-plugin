
#include "stdafx.h"

#define _IN_SOFT

#include "w3d.h"

//#include "externals.h"
//#include "soft.h"

//#define VC_INLINE
#include "gpu.h"
#include "prim.h"
#include "menu.h"

////////////////////////////////////////////////////////////////////////////////////
// soft globals
////////////////////////////////////////////////////////////////////////////////////

short g_m1=255,g_m2=255,g_m3=255;
short DrawSemiTrans=FALSE;
short Ymin;
short Ymax;

short          ly0,lx0,ly1,lx1,ly2,lx2,ly3,lx3;        // global psx vertex coords
long           GlobalTextAddrX,GlobalTextAddrY,GlobalTextTP;
long           GlobalTextREST,GlobalTextABR,GlobalTextPAGE;

////////////////////////////////////////////////////////////////////////
// POLYGON OFFSET FUNCS
////////////////////////////////////////////////////////////////////////

void offsetPSX2(void)
{
 lx0 += PSXDisplay.DrawOffset.x;
 ly0 += PSXDisplay.DrawOffset.y;
 lx1 += PSXDisplay.DrawOffset.x;
 ly1 += PSXDisplay.DrawOffset.y;

 if (scale_gfx == SCALING_EXP) {
   OffsetSet();
   lx0 += W3D_Offset.x;
   lx1 += W3D_Offset.x;
   ly0 += W3D_Offset.y;
   ly1 += W3D_Offset.y;
 }
}

void offsetPSX3(void)
{
 lx0 += PSXDisplay.DrawOffset.x;
 ly0 += PSXDisplay.DrawOffset.y;
 lx1 += PSXDisplay.DrawOffset.x;
 ly1 += PSXDisplay.DrawOffset.y;
 lx2 += PSXDisplay.DrawOffset.x;
 ly2 += PSXDisplay.DrawOffset.y;

 if (scale_gfx == SCALING_EXP) {
   OffsetSet();
   lx0 += W3D_Offset.x;
   lx1 += W3D_Offset.x;
   lx2 += W3D_Offset.x;
   ly0 += W3D_Offset.y;
   ly1 += W3D_Offset.y;
   ly2 += W3D_Offset.y;
 }
}

void offsetPSX4(void)
{
 lx0 += PSXDisplay.DrawOffset.x;
 ly0 += PSXDisplay.DrawOffset.y;
 lx1 += PSXDisplay.DrawOffset.x;
 ly1 += PSXDisplay.DrawOffset.y;
 lx2 += PSXDisplay.DrawOffset.x;
 ly2 += PSXDisplay.DrawOffset.y;
 lx3 += PSXDisplay.DrawOffset.x;
 ly3 += PSXDisplay.DrawOffset.y;

 if (scale_gfx == SCALING_EXP) {
   OffsetSet();
   lx0 += W3D_Offset.x;
   lx1 += W3D_Offset.x;
   lx2 += W3D_Offset.x;
   lx3 += W3D_Offset.x;
   ly0 += W3D_Offset.y;
   ly1 += W3D_Offset.y;
   ly2 += W3D_Offset.y;
   ly3 += W3D_Offset.y;
 }
}
