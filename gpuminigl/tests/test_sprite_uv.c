#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../sprite_uv.h"
static void unchanged(float sx,float sy,int f,int r,int tw,int wrap)
{
    MGLSpriteUV a={8,16,40,48},b=a;
    assert(!mgl_sprite_uv_inset(&b,sx,sy,f,r,tw,wrap));
    assert(!memcmp(&a,&b,sizeof(a)));
}
int main(void)
{
    int flip, width, height;
    unsigned cases=0;
    unchanged(1,1,1,0,0,0); unchanged(0.5f,1,1,0,0,0);
    unchanged(2,2,0,0,0,0); unchanged(2,2,1,1,0,0);
    unchanged(2,2,1,2,0,0); unchanged(2,2,1,0,1,0);
    unchanged(2,2,1,0,0,1);
    for(flip=0;flip<4;++flip) for(width=1;width<=256;++width)
    for(height=1;height<=256;++height) {
        MGLSpriteUV uv;
        uv.u0=(flip&1)?width:0; uv.u1=(flip&1)?0:width;
        uv.v0=(flip&2)?height:0; uv.v1=(flip&2)?0:height;
        assert(mgl_sprite_uv_inset(&uv,2,2,1,0,0,0)==3);
        assert(uv.u0==((flip&1)?width-0.5f:0.5f));
        assert(uv.u1==((flip&1)?0.5f:width-0.5f));
        assert(uv.v0==((flip&2)?height-0.5f:0.5f));
        assert(uv.v1==((flip&2)?0.5f:height-0.5f));
        ++cases;
    }
    {
        MGLSpriteUV uv={8,16,40,48};
        assert(mgl_sprite_uv_inset(&uv,2,1,1,0,0,0)==1);
        assert(uv.v0==16 && uv.v1==48);
        uv.u0=8;uv.u1=40;
        assert(mgl_sprite_uv_inset(&uv,1,2,1,0,0,0)==2);
        assert(uv.u0==8 && uv.u1==40);
    }
    {
        MGLSpriteUV invalid[]={{0,0,0,1},{0,0,1,0},{-1,0,32,32},{240,0,272,32}};
        unsigned i;
        for(i=0;i<sizeof(invalid)/sizeof(invalid[0]);++i) {
            MGLSpriteUV old=invalid[i];
            assert(!mgl_sprite_uv_inset(&invalid[i],2,2,1,0,0,0));
            assert(!memcmp(&old,&invalid[i],sizeof(old)));
        }
    }
    printf("PASS: %u sprite sizes/flips; axis scaling, nearest, 1:1, reduction, windows, wraps, degenerate UV\n",cases);
    return 0;
}
