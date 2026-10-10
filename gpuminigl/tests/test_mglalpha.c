#include <assert.h>
#include <stdio.h>
#include "mglalpha_pattern.h"
static unsigned char black[MA_BYTES], gold[MA_BYTES], atlas_test[MA_BYTES];
int main(void)
{
    unsigned i, ink=0, transparent=0;
    ma_pattern(black,0); ma_pattern(gold,1); ma_pattern(atlas_test,2);
    for(i=0;i<MA_SIZE*MA_SIZE;++i) {
        const unsigned char *a=black+i*4,*b=gold+i*4,*c=atlas_test+i*4;
        unsigned x=i%MA_SIZE,y=i/MA_SIZE;
        assert(a[3]==b[3]); assert(c[3]==255);
        assert(b[0]==255 && b[1]==192 && b[2]==48);
        if(a[3]) { ++ink; assert(a[0]==255 && a[1]==192 && a[2]==48); }
        else { ++transparent; assert(!a[0] && !a[1] && !a[2]); }
        if(x<8 || x>=120 || y<8 || y>=120) assert(!c[0] && !c[1] && !c[2]);
    }
    assert(ink>1000 && transparent>1000);
    /* Centre of atlas's top boundary: linear mixes outside black with blue.
     * Half-texel inset selects the inside texel instead. */
    i=(8*MA_SIZE+64)*4;
    assert(atlas_test[i]==48 && atlas_test[i+1]==96 && atlas_test[i+2]==176);
    assert(atlas_test[i-MA_SIZE*4]==0);
    assert(ma_word_pixel("TOMB",200,20,18,19,4)==0);
    assert(ma_word_pixel("TOMB",18,18,18,19,4)==0);
    assert(ma_glyph(' ',0)==0 && ma_glyph('T',7)==0);
    /* A 25% opaque / 75% transparent sample passes alpha > 0.
     * With black transparent RGB and blending disabled its colour is
     * darkened to 25% gold. A higher cutoff rejects it, losing coverage.
     * Gold transparent RGB preserves colour at the SAME interpolated alpha.
     * These are mathematical controls, not a simulation of either driver. */
    {
        unsigned alpha=255/4, black_red=255/4, gold_red=255;
        assert(alpha>0 && alpha<128);
        assert(black_red<gold_red);
        /* Straight-alpha blend on the blue panel double-attenuates the
         * gold contribution if interpolation already mixed it with black. */
        assert((black_red+48*3)/4 < (gold_red+48*3)/4);
    }
    puts("PASS: alpha masks, transparent RGB controls, opaque atlas neighbours and logo bounds");
    return 0;
}
