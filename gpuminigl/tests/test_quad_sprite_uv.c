#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../quad_sprite_uv.h"
static const MGLQuadSpriteVertex base[4]={{10,20,8,16},{42,20,40,16},{10,36,8,32},{42,36,40,32}};
static void rejected(MGLQuadSpriteVertex q[4],float sx,float sy,int linear,int reduction,int window)
{
    MGLQuadSpriteVertex copy[4];memcpy(copy,q,sizeof(copy));
    assert(!mgl_quad_sprite_inset(q,sx,sy,linear,reduction,window));
    assert(!memcmp(copy,q,sizeof(copy)));
}
static void replay(const char *path)
{
    FILE *f=fopen(path,"r");char line[768],source[64];
    MGLQuadSpriteVertex q[4];unsigned id=0,n=0,v,seen=0,count=0,total=0;
    int linear=0,reduction=0,window=0;float sx=1,sy=1;
    assert(f);
    while(fgets(line,sizeof(line),f)) {
        if(line[0]=='P' && line[1]==' ') {
            char *s;
            assert(sscanf(line,"P %u source=%63s n=%u",&id,source,&n)==3);seen=0;
            s=strstr(line,"linear=");assert(s);linear=atoi(s+7);
            s=strstr(line,"reduction=");assert(s);reduction=atoi(s+10);
            s=strstr(line,"window=");assert(s);window=strtoul(s+7,NULL,16)!=0;
            s=strstr(line,"sx256=");assert(s);sx=atoi(s+6)/256.0f;
            s=strstr(line,"sy256=");assert(s);sy=atoi(s+6)/256.0f;
        } else if(line[0]==' ' && line[1]=='V') {
            long x,y,u,t;unsigned long rgb;
            assert(sscanf(line," V%u xyuv256=%ld,%ld,%ld,%ld rgb=%lx",&v,&x,&y,&u,&t,&rgb)==6);
            assert(v<4);q[v].x=x/256.0f;q[v].y=y/256.0f;q[v].u=u/256.0f;q[v].v=t/256.0f;seen|=1<<v;
            if(seen==15 && n==4 && !strcmp(source,"drawPoly4FTi") && ((rgb>>24)&0xfc)==0x2c) {
                ++total;
                if(mgl_quad_sprite_inset(q,sx,sy,linear,reduction,window)) {printf("capture eligible P %u\n",id);++count;}
            }
        }
    }
    fclose(f);printf("Capture replay: %u/%u eligible quads (heuristic, not proof of 2D)\n",count,total);
}
int main(int argc,char **argv)
{
    MGLQuadSpriteVertex q[4];int flip,swap,i,inc;
    /* Exclusive/inclusive mixed axes, reflected and transposed.
     * Inclusive maxima must move +0.5, NOT shrink to max-0.5. */
    for(inc=0;inc<4;++inc)for(flip=0;flip<4;++flip)for(swap=0;swap<2;++swap) {
        MGLQuadSpriteVertex before[4];
        memcpy(q,base,sizeof(q));
        for(i=0;i<4;++i) {
            if((inc&1) && q[i].u==40)q[i].u-=1;
            if((inc&2) && q[i].v==32)q[i].v-=1;
            if(flip&1)q[i].u=48-((inc&1)?1:0)-q[i].u;
            if(flip&2)q[i].v=48-((inc&2)?1:0)-q[i].v;
            if(swap){float t=q[i].u;q[i].u=q[i].v;q[i].v=t;}
        }
        memcpy(before,q,sizeof(q));
        assert(mgl_quad_sprite_inset(q,2.5f,2.5f,1,0,0)==3);
        for(i=0;i<4;++i) {
            int iu=inc&(swap?2:1),iv=inc&(swap?1:2);
            assert(q[i].x==before[i].x && q[i].y==before[i].y);
            assert(q[i].u==before[i].u+((iu || before[i].u==(swap?16:8))?0.5f:-0.5f));
            assert(q[i].v==before[i].v+((iv || before[i].v==(swap?8:16))?0.5f:-0.5f));
        }
    }
    for(flip=0;flip<4;++flip)for(swap=0;swap<2;++swap) {
        memcpy(q,base,sizeof(q));
        for(i=0;i<4;++i) {
            if(flip&1)q[i].u=48-q[i].u;
            if(flip&2)q[i].v=48-q[i].v;
            if(swap){float t=q[i].u;q[i].u=q[i].v;q[i].v=t;}
        }
        assert(mgl_quad_sprite_inset(q,2,2,1,0,0)==3);
        for(i=0;i<4;++i){assert(q[i].x==base[i].x && q[i].y==base[i].y);}
    }
    memcpy(q,base,sizeof(q));rejected(q,2,2,0,0,0);
    rejected(q,1,1,1,0,0);rejected(q,2,2,1,1,0);rejected(q,2,2,1,0,1);
    q[3].x+=1;rejected(q,2,2,1,0,0); /* perspective/skew */
    memcpy(q,base,sizeof(q));q[1].x+=32;q[3].x+=32;rejected(q,2,2,1,0,0); /* stretched world quad */
    memcpy(q,base,sizeof(q));q[1].u-=2;q[3].u-=2;rejected(q,2,2,1,0,0); /* not 1:1, even with inclusive end */
    memcpy(q,base,sizeof(q));q[0]=q[1];rejected(q,2,2,1,0,0);
    memcpy(q,base,sizeof(q));q[0].u=-1;rejected(q,2,2,1,0,0);
    memcpy(q,base,sizeof(q));{MGLQuadSpriteVertex t=q[2];q[2]=q[3];q[3]=t;}rejected(q,2,2,1,0,0);
    memcpy(q,base,sizeof(q));assert(mgl_quad_sprite_inset(q,2,1,1,0,0)==1);
    assert(q[0].u==8.5f && q[1].u==39.5f && q[0].v==16);
    {
        /* Actual omitted P146 from the E capture: mirrored X and Y,
         * 64x14 geometry, inclusive U 0..63, exclusive V 192..206. */
        MGLQuadSpriteVertex captured[4]={{117,119,0,192},{53,119,63,192},
            {117,105,0,206},{53,105,63,206}};
        memcpy(q,captured,sizeof(q));
        assert(mgl_quad_sprite_inset(q,2.5f,2.5f,1,0,0)==3);
        assert(q[0].u==0.5f && q[1].u==63.5f);
        assert(q[0].v==192.5f && q[2].v==205.5f);
        memcpy(q,captured,sizeof(q));rejected(q,2.5f,2.5f,0,0,0);
        rejected(q,1,1,1,0,0);rejected(q,2.5f,2.5f,1,0,1);
    }
    puts("PASS: quad UV axes, mirror, transpose, scaling, rejection and unchanged geometry");
    if(argc>1)replay(argv[1]);
    return 0;
}
