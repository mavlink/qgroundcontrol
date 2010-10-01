
/* Copyright (c) Mark J. Kilgard, 1995. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include "glutint.h"
#include "glutstroke.h"

/* CENTRY */
int APIENTRY 
glutStrokeWidth(GLUTstrokeFont font, int c)
{
  StrokeFontPtr fontinfo;
  const StrokeCharRec *ch;

#if defined(_WIN32)
  fontinfo = (StrokeFontPtr) __glutFont(font);
#else
  fontinfo = (StrokeFontPtr) font;
#endif

  if (c < 0 || c >= fontinfo->num_chars)
    return 0;
  ch = &(fontinfo->ch[c]);
  if (ch)
    return ch->right;
  else
    return 0;
}

int APIENTRY 
glutStrokeLength(GLUTstrokeFont font, const unsigned char *string)
{
  int c, length;
  StrokeFontPtr fontinfo;
  const StrokeCharRec *ch;

#if defined(_WIN32)
  fontinfo = (StrokeFontPtr) __glutFont(font);
#else
  fontinfo = (StrokeFontPtr) font;
#endif

  length = 0;
  for (; *string != '\0'; string++) {
    c = *string;
    if (c >= 0 && c < fontinfo->num_chars) {
      ch = &(fontinfo->ch[c]);
      if (ch)
        length += ch->right;
    }
  }
  return length;
}

/* ENDCENTRY */
