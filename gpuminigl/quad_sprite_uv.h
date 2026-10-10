/* Conservative EXPERIMENT, not a reliable 2D/3D semantic classifier.
 * Flat axis-aligned quads with one screen pixel per source texel BEFORE
 * output scaling. Reject skew, rotation, tiling and texture windows.
 * Accept either exclusive bounds or inclusive texel indices per axis.
 * The latter map to texel centers (0..63 -> 0.5..63.5), also mirrored. */
#ifndef FPSE_MINIGL_QUAD_SPRITE_UV_H
#define FPSE_MINIGL_QUAD_SPRITE_UV_H
typedef struct { float x,y,u,v; } MGLQuadSpriteVertex;
static __inline__ float mgl_qabs(float x) {return x<0?-x:x;}
static __inline__ int mgl_qspan(float pixels,float texels)
{ return texels==pixels || texels+1==pixels; }
static __inline__ unsigned mgl_quad_sprite_inset(MGLQuadSpriteVertex q[4],
    float sx,float sy,int linear,int reduction,int window)
{
    int i,axis_u,axis_v,inclusive_u,inclusive_v;unsigned seen_xy=0,seen_uv=0,axes=0;
    float xmin=q[0].x,xmax=xmin,ymin=q[0].y,ymax=ymin;
    float umin=q[0].u,umax=umin,vmin=q[0].v,vmax=vmin;
    if(!linear || reduction || window || (sx<=1 && sy<=1)) return 0;
    for(i=1;i<4;++i) {
        if(q[i].x<xmin)xmin=q[i].x;
        if(q[i].x>xmax)xmax=q[i].x;
        if(q[i].y<ymin)ymin=q[i].y;
        if(q[i].y>ymax)ymax=q[i].y;
        if(q[i].u<umin)umin=q[i].u;
        if(q[i].u>umax)umax=q[i].u;
        if(q[i].v<vmin)vmin=q[i].v;
        if(q[i].v>vmax)vmax=q[i].v;
    }
    if(xmax<=xmin || ymax<=ymin || umin<0 || vmin<0 || umax>256 || vmax>256 ||
        umax-umin<1 || vmax-vmin<1) return 0;
    for(i=0;i<4;++i) {
        if((q[i].x!=xmin && q[i].x!=xmax) || (q[i].y!=ymin && q[i].y!=ymax) ||
           (q[i].u!=umin && q[i].u!=umax) || (q[i].v!=vmin && q[i].v!=vmax))return 0;
        seen_xy|=1U<<((q[i].x==xmax)+2*(q[i].y==ymax));
        seen_uv|=1U<<((q[i].u==umax)+2*(q[i].v==vmax));
    }
    if(seen_xy!=15 || seen_uv!=15)return 0;
    /* PSX quad triangles (0,1,2), (1,2,3): 1--2 must be the diagonal. */
    if(q[1].x==q[2].x || q[1].y==q[2].y || q[0].x==q[3].x || q[0].y==q[3].y)return 0;
    axis_u=axis_v=-1;
    for(i=1;i<4;++i) {
        float dx=mgl_qabs(q[i].x-q[0].x),dy=mgl_qabs(q[i].y-q[0].y);
        float du=mgl_qabs(q[i].u-q[0].u),dv=mgl_qabs(q[i].v-q[0].v);
        if(dy==0) {
            if(mgl_qspan(dx,du) && dv==0){axis_u=0;axis_v=1;}
            else if(mgl_qspan(dx,dv) && du==0){axis_u=1;axis_v=0;}
            else return 0;
        }
    }
    if(axis_u<0)return 0;
    for(i=1;i<4;++i) {
        float dx=mgl_qabs(q[i].x-q[0].x),dy=mgl_qabs(q[i].y-q[0].y);
        float du=mgl_qabs(q[i].u-q[0].u),dv=mgl_qabs(q[i].v-q[0].v);
        float pu=axis_u?dy:dx,pv=axis_v?dy:dx;
        if((pu==0 ? du!=0 : !mgl_qspan(pu,du)) ||
           (pv==0 ? dv!=0 : !mgl_qspan(pv,dv)))return 0;
    }
    inclusive_u=(umax-umin+1==(axis_u?ymax-ymin:xmax-xmin));
    inclusive_v=(vmax-vmin+1==(axis_v?ymax-ymin:xmax-xmin));
    if((axis_u?sy:sx)>1)axes|=1;
    if((axis_v?sy:sx)>1)axes|=2;
    for(i=0;i<4;++i) {
        if(axes&1)q[i].u+=(inclusive_u || q[i].u==umin)?0.5f:-0.5f;
        if(axes&2)q[i].v+=(inclusive_v || q[i].v==vmin)?0.5f:-0.5f;
    }
    return axes;
}
#endif
