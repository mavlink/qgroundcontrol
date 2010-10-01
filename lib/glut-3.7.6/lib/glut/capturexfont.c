
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

/* capturexfont.c connects to an X server and downloads a
   bitmap font from which a C source file is generated,
   encoding  the font for GLUT's use. Example usage:
   capturexfont.c 9x15 glutBitmap9By15 > glut_9x15.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <GL/gl.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define MAX_GLYPHS_PER_GRAB 512  /* This is big enough for 2^9
                                    glyph character sets */

static void
outputChar(int num, int width, int height,
  int xoff, int yoff, int advance, int data)
{
  if (width == 0 || height == 0) {
    printf("#ifdef _WIN32\n");
    printf("/* XXX Work around Microsoft OpenGL 1.1 bug where glBitmap with\n");
    printf("   a height or width of zero does not advance the raster position\n");
    printf("   as specified by OpenGL. (Cosmo OpenGL does not have this bug.) */\n");
    printf("static const GLubyte ch%ddata[] = { 0x0 };\n", num);
    printf("static const BitmapCharRec ch%d = {", num);
    printf("%d,", 0);
    printf("%d,", 0);
    printf("%d,", xoff);
    printf("%d,", yoff);
    printf("%d,", advance);
    printf("ch%ddata", num);
    printf("};\n");
    printf("#else\n");
  }
  printf("static const BitmapCharRec ch%d = {", num);
  printf("%d,", width);
  printf("%d,", height);
  printf("%d,", xoff);
  printf("%d,", yoff);
  printf("%d,", advance);
  if (data) {
    printf("ch%ddata", num);
  } else {
    printf("0");
  }
  printf("};\n");
  if (width == 0 || height == 0) {
    printf("#endif\n");
  }
  printf("\n");
}

/* Can't just use isprint because it only works for the range
   of ASCII characters (ie, TRUE for isascii) and capturexfont
   might be run on 16-bit fonts. */
#define PRINTABLE(ch)  (isascii(ch) ? isprint(ch) : 0)

void
captureXFont(Display * dpy, Font font, char *xfont, char *name)
{
  int first, last, count;
  int cnt, len;
  Pixmap offscreen;
  Window drawable;
  XFontStruct *fontinfo;
  XImage *image;
  GC xgc;
  XGCValues values;
  int width, height;
  int i, j, k;
  XCharStruct *charinfo;
  XChar2b character;
  GLubyte *bitmapData;
  int x, y;
  int spanLength;
  int charWidth, charHeight, maxSpanLength, pixwidth;
  int grabList[MAX_GLYPHS_PER_GRAB];
  int glyphsPerGrab = MAX_GLYPHS_PER_GRAB;
  int numToGrab;
  int rows, pages, byte1, byte2, index;
  int nullBitmap;

  drawable = RootWindow(dpy, DefaultScreen(dpy));

  fontinfo = XQueryFont(dpy, font);
  pages = fontinfo->max_char_or_byte2 - fontinfo->min_char_or_byte2 + 1;
  first = (fontinfo->min_byte1 << 8) + fontinfo->min_char_or_byte2;
  last = (fontinfo->max_byte1 << 8) + fontinfo->max_char_or_byte2;
  count = last - first + 1;

  width = fontinfo->max_bounds.rbearing -
    fontinfo->min_bounds.lbearing;
  height = fontinfo->max_bounds.ascent +
    fontinfo->max_bounds.descent;
  /* 16-bit fonts have more than one row; indexing into
     per_char is trickier. */
  rows = fontinfo->max_byte1 - fontinfo->min_byte1 + 1;

  maxSpanLength = (width + 7) / 8;
  /* For portability reasons we don't use alloca for
     bitmapData, but we could. */
  bitmapData = malloc(height * maxSpanLength);
  /* Be careful determining the width of the pixmap; the X
     protocol allows pixmaps of width 2^16-1 (unsigned short
     size) but drawing coordinates max out at 2^15-1 (signed
     short size).  If the width is too large, we need to limit
     the glyphs per grab. */
  if ((glyphsPerGrab * 8 * maxSpanLength) >= (1 << 15)) {
    glyphsPerGrab = (1 << 15) / (8 * maxSpanLength);
  }
  pixwidth = glyphsPerGrab * 8 * maxSpanLength;
  offscreen = XCreatePixmap(dpy, drawable, pixwidth, height, 1);

  values.font = font;
  values.background = 0;
  values.foreground = 0;
  xgc = XCreateGC(dpy, offscreen,
    GCFont | GCBackground | GCForeground, &values);
  XFillRectangle(dpy, offscreen, xgc, 0, 0,
    8 * maxSpanLength * glyphsPerGrab, height);
  XSetForeground(dpy, xgc, 1);

  numToGrab = 0;
  if (fontinfo->per_char == NULL) {
    charinfo = &(fontinfo->min_bounds);
    charWidth = charinfo->rbearing - charinfo->lbearing;
    charHeight = charinfo->ascent + charinfo->descent;
    spanLength = (charWidth + 7) / 8;
  }
  printf("\n/* GENERATED FILE -- DO NOT MODIFY */\n\n");
  printf("#include \"glutbitmap.h\"\n\n");
  for (i = first; count; i++, count--) {
    int undefined;
    if (rows == 1) {
      undefined = (fontinfo->min_char_or_byte2 > i ||
        fontinfo->max_char_or_byte2 < i);
    } else {
      byte2 = i & 0xff;
      byte1 = i >> 8;
      undefined = (fontinfo->min_char_or_byte2 > byte2 ||
        fontinfo->max_char_or_byte2 < byte2 ||
        fontinfo->min_byte1 > byte1 ||
        fontinfo->max_byte1 < byte1);

    }
    if (undefined) {
      goto PossiblyDoGrab;
    }
    if (fontinfo->per_char != NULL) {
      if (rows == 1) {
        index = i - fontinfo->min_char_or_byte2;
      } else {
        byte2 = i & 0xff;
        byte1 = i >> 8;
        index =
          (byte1 - fontinfo->min_byte1) * pages +
          (byte2 - fontinfo->min_char_or_byte2);
      }
      charinfo = &(fontinfo->per_char[index]);
      charWidth = charinfo->rbearing - charinfo->lbearing;
      charHeight = charinfo->ascent + charinfo->descent;
      if (charWidth == 0 || charHeight == 0) {
        if (charinfo->width != 0) {
          /* Still must move raster pos even if empty character 

           */
          outputChar(i, 0, 0, 0, 0, charinfo->width, 0);
        }
        goto PossiblyDoGrab;
      }
    }
    grabList[numToGrab] = i;
    character.byte2 = i & 255;
    character.byte1 = i >> 8;

    /* XXX We could use XDrawImageString16 which would also
       paint the backing rectangle but X server bugs in some
       scalable font rasterizers makes it more effective to do
       XFillRectangles to clear the pixmap and then
       XDrawImage16 for the text.  */
    XDrawString16(dpy, offscreen, xgc,
      -charinfo->lbearing + 8 * maxSpanLength * numToGrab,
      charinfo->ascent, &character, 1);

    numToGrab++;

  PossiblyDoGrab:

    if (numToGrab >= glyphsPerGrab || count == 1) {
      image = XGetImage(dpy, offscreen,
        0, 0, pixwidth, height, 1, XYPixmap);
      for (j = numToGrab - 1; j >= 0; j--) {
        if (fontinfo->per_char != NULL) {
          byte2 = grabList[j] & 0xff;
          byte1 = grabList[j] >> 8;
          index =
            (byte1 - fontinfo->min_byte1) * pages +
            (byte2 - fontinfo->min_char_or_byte2);
          charinfo = &(fontinfo->per_char[index]);
          charWidth = charinfo->rbearing - charinfo->lbearing;
          charHeight = charinfo->ascent + charinfo->descent;
          spanLength = (charWidth + 7) / 8;
        }
        memset(bitmapData, 0, height * spanLength);
        for (y = 0; y < charHeight; y++) {
          for (x = 0; x < charWidth; x++) {
            if (XGetPixel(image, j * maxSpanLength * 8 + x,
                charHeight - 1 - y)) {
              /* Little endian machines (such as DEC Alpha)
                 could  benefit from reversing the bit order
                 here and changing the GL_UNPACK_LSB_FIRST
                 parameter in glutBitmapCharacter to GL_TRUE. */
              bitmapData[y * spanLength + x / 8] |=
                (1 << (7 - (x & 7)));
            }
          }
        }
        if (PRINTABLE(grabList[j])) {
          printf("/* char: 0x%x '%c' */\n\n",
            grabList[j], grabList[j]);
        } else {
          printf("/* char: 0x%x */\n\n", grabList[j]);
        }

        /* Determine if the bitmap is null. */
        nullBitmap = 1;
        len = (charinfo->ascent + charinfo->descent) *
          ((charinfo->rbearing - charinfo->lbearing + 7) / 8);
        cnt = 0;
        while (cnt < len) {
          for (k = 0; k < 16 && cnt < len; k++, cnt++) {
            if (bitmapData[cnt] != 0) {
              nullBitmap = 0;
            }
          }
        }

        if (!nullBitmap) {
          printf("static const GLubyte ch%ddata[] = {\n", grabList[j]);
          len = (charinfo->ascent + charinfo->descent) *
            ((charinfo->rbearing - charinfo->lbearing + 7) / 8);
          cnt = 0;
          while (cnt < len) {
            for (k = 0; k < 16 && cnt < len; k++, cnt++) {
              printf("0x%x,", bitmapData[cnt]);
            }
            printf("\n");
          }
          printf("};\n\n");
        } else {
          charWidth = 0;
          charHeight = 0;
        }

        outputChar(grabList[j], charWidth, charHeight,
          -charinfo->lbearing, charinfo->descent,
          charinfo->width, !nullBitmap);
      }
      XDestroyImage(image);
      numToGrab = 0;
      if (count > 0) {
        XSetForeground(dpy, xgc, 0);
        XFillRectangle(dpy, offscreen, xgc, 0, 0,
          8 * maxSpanLength * glyphsPerGrab, height);
        XSetForeground(dpy, xgc, 1);
      }
    }
  }
  XFreeGC(dpy, xgc);
  XFreePixmap(dpy, offscreen);
  /* For portability reasons we don't use alloca for
     bitmapData, but we could. */
  free(bitmapData);

  printf("static const BitmapCharRec * const chars[] = {\n");
  for (i = first; i <= last; i++) {
    int undefined;
    byte2 = i & 0xff;
    byte1 = i >> 8;
    undefined = (fontinfo->min_char_or_byte2 > byte2 ||
      fontinfo->max_char_or_byte2 < byte2 ||
      fontinfo->min_byte1 > byte1 ||
      fontinfo->max_byte1 < byte1);
    if (undefined) {
      printf("0,\n");
    } else {
      if (fontinfo->per_char != NULL) {
        if (rows == 1) {
          index = i - fontinfo->min_char_or_byte2;
        } else {
          byte2 = i & 0xff;
          byte1 = i >> 8;
          index =
            (byte1 - fontinfo->min_byte1) * pages +
            (byte2 - fontinfo->min_char_or_byte2);
        }
        charinfo = &(fontinfo->per_char[index]);
        charWidth = charinfo->rbearing - charinfo->lbearing;
        charHeight = charinfo->ascent + charinfo->descent;
        if (charWidth == 0 || charHeight == 0) {
          if (charinfo->width == 0) {
            printf("0,\n");
            continue;
          }
        }
      }
      printf("&ch%d,\n", i);
    }
  }
  printf("};\n\n");
  printf("const BitmapFontRec %s = {\n", name);
  printf("\"%s\",\n", xfont);
  printf("%d,\n", last - first + 1);
  printf("%d,\n", first);
  printf("chars\n");
  printf("};\n\n");
  XFreeFont(dpy, fontinfo);
}

int
main(int argc, char **argv)
{
  Display *dpy;
  Font font;

  if (argc != 3) {
    fprintf(stderr, "usage: capturexfont XFONT NAME\n");
    exit(1);
  }
  dpy = XOpenDisplay(NULL);
  if (dpy == NULL) {
    fprintf(stderr, "capturexfont: could not open X display\n");
    exit(1);
  }
  font = XLoadFont(dpy, argv[1]);
  if (font == None) {
    fprintf(stderr, "capturexfont: bad font\n");
    exit(1);
  }
  captureXFont(dpy, font, argv[1], argv[2]);
  XCloseDisplay(dpy);
  return 0;
}
