#if 1

#ifdef W3D_VERBOSE
  W3D_VERBOSE_LINE++;
#endif

  /* Do we have a point? */
  if ((x1 == x2) && (y1 == y2)) {
    W3D_Float alpha;

    SETUP(W3D_PRIMITIVE_POINTS, PRIM_DEFAULT)

    alpha = set_blend_mode(NULL);

    INDEX_1(0)

    VERTEX(0, x1, y1, rgb1);

    DRAW(1)
  } else {
    W3D_Float alpha;

    SETUP(W3D_PRIMITIVE_LINES, PRIM_DEFAULT)

    alpha = set_blend_mode(NULL);

    INDEX_2(0, 1)

#ifdef GOURAUD
    VERTEX(0, x1, y1, rgb1);
    VERTEX(1, x2, y2, rgb2);
#else
    VERTEX(0, x1, y1, rgb1);
    VERTEX(1, x2, y2, rgb1);
#endif

    DRAW(2)
  }

#endif
