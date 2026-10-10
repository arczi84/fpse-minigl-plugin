#if 1

  W3D_Float alpha;

  SETUP(W3D_PRIMITIVE_TRIANGLES, PRIM_DEFAULT)

#ifdef W3D_VERBOSE
  W3D_VERBOSE_POLY_SMP += ((N == 4) ? 2 : 1);
#endif

  alpha = set_blend_mode(NULL);

#if N == 4
  INDEX_6(0, 1, 2, 1, 2, 3)
#else
  INDEX_3(0, 1, 2)
#endif

#ifdef GOURAUD
  VERTEX(0, x1, y1, rgb1);
  VERTEX(1, x2, y2, rgb2);
  VERTEX(2, x3, y3, rgb3);
#if N == 4
  VERTEX(3, x4, y4, rgb4);
#endif
#else
  VERTEX(0, x1, y1, rgb1);
  VERTEX(1, x2, y2, rgb1);
  VERTEX(2, x3, y3, rgb1);
#if N == 4
  VERTEX(3, x4, y4, rgb1);
#endif
#endif

  DRAW(N)

#endif
