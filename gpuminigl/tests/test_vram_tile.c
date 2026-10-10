#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "../vram_tile.h"
static unsigned char vram[1024*1024], guarded[256*256*4+32];
static unsigned char *tile=guarded+16;
static int limit(int p,int size) { return p<0?0:p>=size?size-1:p; }
/* Independent reference, deliberately not using pixels.h or tile helpers. */
static void reference(int x,int y,int sx,int sy,int w,int h,int rgb24,unsigned char *p)
{
    unsigned row,a,c;
    sx=limit(sx,w);sy=limit(sy,h);row=(unsigned)(y+sy)&511;
    if(rgb24) {
        a=(row*2048+x*2+sx*3)&0xfffff;
        for(c=0;c<3;++c) p[c]=vram[(a+c)&0xfffff];
    } else {
        unsigned word;
        a=(row*1024+((unsigned)(x+sx)&1023))*2;
        word=vram[a]+256U*vram[a+1];
        /* Independently expand each five-bit component. */
        for(c=0;c<3;++c) { unsigned v=(word>>(5*c))&31;p[c]=v*8+v/4; }
    }
    p[3]=255;
}
static void sample_check(int x,int y,int w,int h,int tx,int ty,int rgb24,double gx,double gy)
{
    int ix=(int)floor(gx),iy=(int)floor(gy),a,b,c;
    double fx=gx-ix,fy=gy-iy,actual[4]={0,0,0,0},expected[4]={0,0,0,0};
    /* UV=((global centre coordinate - tile origin)+1.5)/256.
     * GL linear sampling uses UV*256-0.5, i.e. gx-tx+1. */
    for(b=0;b<2;++b) for(a=0;a<2;++a) {
        unsigned char ref[4];
        int px=ix+a-tx+1,py=iy+b-ty+1;
        double weight=(a?fx:1-fx)*(b?fy:1-fy);
        assert(px>=0 && px<256 && py>=0 && py<256);
        reference(x,y,ix+a,iy+b,w,h,rgb24,ref);
        for(c=0;c<4;++c) {
            actual[c]+=tile[(py*256+px)*4+c]*weight;
            expected[c]+=ref[c]*weight;
        }
    }
    for(c=0;c<4;++c) assert(fabs(actual[c]-expected[c])<0.00001);
}
int main(void)
{
    static const int cases[][4]={{1,1,0,0},{256,240,0,0},{512,480,1018,510},
        {640,255,-4,-2},{1024,512,2,511},{255,254,1023,511}};
    unsigned k,i,count=0;int rgb24,tx,ty,px,py,c;
    for(i=0;i<sizeof(vram);++i) vram[i]=(unsigned char)((i*73U)^(i>>7)^(i>>13));
    vram[0]=vram[1]=vram[2]=0;vram[3]=128; /* opaque upload of zero/STP black */
    memset(guarded,0xa5,sizeof(guarded));
    for(k=0;k<sizeof(cases)/sizeof(cases[0]);++k) for(rgb24=0;rgb24<2;++rgb24) {
        int w=cases[k][0],h=cases[k][1],x=cases[k][2],y=cases[k][3];
        for(ty=0;ty<h;ty+=254) for(tx=0;tx<w;tx+=254) {
            int tw=w-tx<254?w-tx:254,th=h-ty<254?h-ty:254;
            double xs[4]={tx-0.49,tx+0.25,tx+tw-0.75,tx+tw-0.51};
            double ys[4]={ty-0.49,ty+0.25,ty+th-0.75,ty+th-0.51};
            mgl_vram_tile_rgba(tile,vram,x,y,w,h,tx,ty,rgb24);
            for(py=0;py<256;++py) for(px=0;px<256;++px) {
                unsigned char ref[4];
                reference(x,y,tx+px-1,ty+py-1,w,h,rgb24,ref);
                for(c=0;c<4;++c) assert(tile[(py*256+px)*4+c]==ref[c]);
            }
            for(py=0;py<4;++py) for(px=0;px<4;++px)
                sample_check(x,y,w,h,tx,ty,rgb24,xs[px],ys[py]);
            for(i=0;i<16;++i) assert(guarded[i]==0xa5 && guarded[sizeof(guarded)-1-i]==0xa5);
            ++count;
        }
    }
    /* Sensitivity control: old GL_REPEAT at a full tile's right edge
     * mixes the last pixel with column zero instead of its neighbour. */
    {
        unsigned char last[4],next[4],wrong[4];int differs=0;
        reference(0,0,255,5,512,240,1,last);
        reference(0,0,256,5,512,240,1,next);
        reference(0,0,0,5,512,240,1,wrong);
        for(c=0;c<3;++c) if(last[c]+next[c]!=last[c]+wrong[c]) differs=1;
        assert(differs);
    }
    printf("PASS: %u padded tiles; RGB555/RGB24, partials, outer clamps, VRAM wrap, guards, bilinear seams\n",count);
    return 0;
}
