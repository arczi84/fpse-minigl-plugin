/*
 * amiga_font.h
 */

#ifndef _AMIGA_FONT_H
#define _AMIGA_FONT_H

typedef struct text_line_s {
  char text[256];
  double autohide; /* in seconds */
  amidog_timeval_t st;
} text_line_t;

#define NUM_TEXT_LINES (4)

extern text_line_t text_lines[NUM_TEXT_LINES];

#endif /* _AMIGA_FONT_H_ */
