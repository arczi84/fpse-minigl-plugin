#if 1

  W3D_Float alpha;
  int opaque_pass;

#ifdef FPSE_MINIGL_UI_CAPTURE
  if (mgl_ui_collecting) {
    MGLUIRecord record = {0};
    record.source = __func__; record.n = N;
    record.vertex[0][0]=x1;record.vertex[0][1]=y1;record.vertex[0][2]=tx1;record.vertex[0][3]=ty1;
    record.vertex[1][0]=x2;record.vertex[1][1]=y2;record.vertex[1][2]=tx2;record.vertex[1][3]=ty2;
    record.vertex[2][0]=x3;record.vertex[2][1]=y3;record.vertex[2][2]=tx3;record.vertex[2][3]=ty3;
    record.rgb[0]=record.rgb[1]=record.rgb[2]=record.rgb[3]=rgb1;
#if N == 4
    record.vertex[3][0]=x4;record.vertex[3][1]=y4;record.vertex[3][2]=tx4;record.vertex[3][3]=ty4;
#endif
#ifdef GOURAUD
    record.rgb[1]=rgb2;record.rgb[2]=rgb3;
#if N == 4
    record.rgb[3]=rgb4;
#endif
#endif
    record.mode=GlobalTextTP;record.page_x=GlobalTextAddrX;record.page_y=GlobalTextAddrY;
    record.clut_x=clX;record.clut_y=clY;record.window=lGPUInfoVals[INFO_TW];
    record.twin=bUsingTWin;record.semitrans=DrawSemiTrans;
    record.linear=texfilter;record.reduction=texred;record.sx=x_scale;record.sy=y_scale;
    mgl_ui_record(&record);
  }
#endif

  if (SHADETEXBIT(rgb1)) {
    rgb1 = 
#ifdef GOURAUD
    rgb2 = rgb3 =
#if N == 4
    rgb4 =
#endif
#endif
    0x00808080;
  }

  if (!(W3D_Flags & FLAG_MULTITEXT)) {
    /* "light" rendering hack, not accurate, multitexture required */
    rgb1 = ARGBx2(rgb1);
#ifdef GOURAUD
    rgb2 = ARGBx2(rgb2);
    rgb3 = ARGBx2(rgb3);
#if N == 4
    rgb4 = ARGBx2(rgb4);
#endif
#endif
  }

  SETUP(W3D_PRIMITIVE_TRIANGLES, PRIM_TEXTURED)

#ifdef W3D_VERBOSE
  W3D_VERBOSE_POLY_TEX += ((N == 4) ? 2 : 1);
#endif

  opaque_pass = set_texture(clX, clY, 0);

  alpha = set_blend_mode(&opaque_pass);

#if N == 4
  INDEX_6(0, 1, 2, 1, 2, 3)
#else
  INDEX_3(0, 1, 2)
#endif

#ifdef GOURAUD
  VERTEX_TEX(0, x1, y1, tx1, ty1, rgb1);
  VERTEX_TEX(1, x2, y2, tx2, ty2, rgb2);
  VERTEX_TEX(2, x3, y3, tx3, ty3, rgb3);
#if N == 4
  VERTEX_TEX(3, x4, y4, tx4, ty4, rgb4);
#endif
#else
  VERTEX_TEX(0, x1, y1, tx1, ty1, rgb1);
  VERTEX_TEX(1, x2, y2, tx2, ty2, rgb1);
  VERTEX_TEX(2, x3, y3, tx3, ty3, rgb1);
#if N == 4
  VERTEX_TEX(3, x4, y4, tx4, ty4, rgb1);
#endif
#endif

  DRAW(N)

#if 1
  if (opaque_pass) {
  SETUP(W3D_PRIMITIVE_TRIANGLES, PRIM_TEXTURED)

#ifdef W3D_VERBOSE
  W3D_VERBOSE_POLY_TEX += ((N == 4) ? 2 : 1);
#endif

  set_texture(clX, clY, 1);

  alpha = set_blend_mode(&opaque_pass);

#if N == 4
  INDEX_6(0, 1, 2, 1, 2, 3)
#else
  INDEX_3(0, 1, 2)
#endif

#ifdef GOURAUD
  VERTEX_TEX(0, x1, y1, tx1, ty1, rgb1);
  VERTEX_TEX(1, x2, y2, tx2, ty2, rgb2);
  VERTEX_TEX(2, x3, y3, tx3, ty3, rgb3);
#if N == 4
  VERTEX_TEX(3, x4, y4, tx4, ty4, rgb4);
#endif
#else
  VERTEX_TEX(0, x1, y1, tx1, ty1, rgb1);
  VERTEX_TEX(1, x2, y2, tx2, ty2, rgb1);
  VERTEX_TEX(2, x3, y3, tx3, ty3, rgb1);
#if N == 4
  VERTEX_TEX(3, x4, y4, tx4, ty4, rgb1);
#endif
#endif

  DRAW(N)
  }
#endif

#endif
