
/* Copyright (c) Mark J. Kilgard, 1997. */

/* This program is freely distributable without licensing fees  and is
   provided without guarantee or warrantee expressed or  implied. This
   program is -not- in the public domain. */

/* X compile line: cc -o gentexfont gentexfont.c -lX11 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <math.h>

#include "TexFont.h"

typedef struct {
  short width;
  short height;
  short xoffset;
  short yoffset;
  short advance;
  unsigned char *bitmap;
} PerGlyphInfo, *PerGlyphInfoPtr;

typedef struct {
  int min_char;
  int max_char;
  int max_ascent;
  int max_descent;
  PerGlyphInfo glyph[1];
} FontInfo, *FontInfoPtr;

Display *dpy;
FontInfoPtr fontinfo;
int format = TXF_FORMAT_BITMAP;
int gap = 1;

/* #define REPORT_GLYPHS */
#ifdef REPORT_GLYPHS
#define DEBUG_GLYPH4(msg,a,b,c,d) printf(msg,a,b,c,d)
#define DEBUG_GLYPH(msg) printf(msg)
#else
#define DEBUG_GLYPH4(msg,a,b,c,d) { /* nothing */ }
#define DEBUG_GLYPH(msg) { /* nothing */ }
#endif

#define MAX_GLYPHS_PER_GRAB 512  /* this is big enough for 2^9 glyph
                                    character sets */

FontInfoPtr
SuckGlyphsFromServer(Display * dpy, Font font)
{
  Pixmap offscreen;
  XFontStruct *fontinfo;
  XImage *image;
  GC xgc;
  XGCValues values;
  int numchars;
  int width, height, pixwidth;
  int i, j;
  XCharStruct *charinfo;
  XChar2b character;
  unsigned char *bitmapData;
  int x, y;
  int spanLength;
  int charWidth, charHeight, maxSpanLength;
  int grabList[MAX_GLYPHS_PER_GRAB];
  int glyphsPerGrab = MAX_GLYPHS_PER_GRAB;
  int numToGrab, thisglyph;
  FontInfoPtr myfontinfo;

  fontinfo = XQueryFont(dpy, font);
  if (!fontinfo)
    return NULL;

  numchars = fontinfo->max_char_or_byte2 - fontinfo->min_char_or_byte2 + 1;
  if (numchars < 1)
    return NULL;

  myfontinfo = (FontInfoPtr) malloc(sizeof(FontInfo) + (numchars - 1) * sizeof(PerGlyphInfo));
  if (!myfontinfo)
    return NULL;

  myfontinfo->min_char = fontinfo->min_char_or_byte2;
  myfontinfo->max_char = fontinfo->max_char_or_byte2;
  myfontinfo->max_ascent = fontinfo->max_bounds.ascent;
  myfontinfo->max_descent = fontinfo->max_bounds.descent;

  width = fontinfo->max_bounds.rbearing - fontinfo->min_bounds.lbearing;
  height = fontinfo->max_bounds.ascent + fontinfo->max_bounds.descent;

  maxSpanLength = (width + 7) / 8;
  /* Be careful determining the width of the pixmap; the X protocol allows
     pixmaps of width 2^16-1 (unsigned short size) but drawing coordinates
     max out at 2^15-1 (signed short size).  If the width is too large, we
     need to limit the glyphs per grab.  */
  if ((glyphsPerGrab * 8 * maxSpanLength) >= (1 << 15)) {
    glyphsPerGrab = (1 << 15) / (8 * maxSpanLength);
  }
  pixwidth = glyphsPerGrab * 8 * maxSpanLength;
  offscreen = XCreatePixmap(dpy, RootWindow(dpy, DefaultScreen(dpy)),
    pixwidth, height, 1);

  values.font = font;
  values.background = 0;
  values.foreground = 0;
  xgc = XCreateGC(dpy, offscreen, GCFont | GCBackground | GCForeground, &values);

  XFillRectangle(dpy, offscreen, xgc, 0, 0, 8 * maxSpanLength * glyphsPerGrab, height);
  XSetForeground(dpy, xgc, 1);

  numToGrab = 0;
  if (fontinfo->per_char == NULL) {
    charinfo = &(fontinfo->min_bounds);
    charWidth = charinfo->rbearing - charinfo->lbearing;
    charHeight = charinfo->ascent + charinfo->descent;
    spanLength = (charWidth + 7) / 8;
  }
  for (i = 0; i < numchars; i++) {
    if (fontinfo->per_char != NULL) {
      charinfo = &(fontinfo->per_char[i]);
      charWidth = charinfo->rbearing - charinfo->lbearing;
      charHeight = charinfo->ascent + charinfo->descent;
      if (charWidth == 0 || charHeight == 0) {
        /* Still must move raster pos even if empty character */
        myfontinfo->glyph[i].width = 0;
        myfontinfo->glyph[i].height = 0;
        myfontinfo->glyph[i].xoffset = 0;
        myfontinfo->glyph[i].yoffset = 0;
        myfontinfo->glyph[i].advance = charinfo->width;
        myfontinfo->glyph[i].bitmap = NULL;
        goto PossiblyDoGrab;
      }
    }
    grabList[numToGrab] = i;

    /* XXX is this right for large fonts? */
    character.byte2 = (i + fontinfo->min_char_or_byte2) & 255;
    character.byte1 = (i + fontinfo->min_char_or_byte2) >> 8;

    /* XXX we could use XDrawImageString16 which would also paint the backing 

       rectangle but X server bugs in some scalable font rasterizers makes it 

       more effective to do XFillRectangles to clear the pixmap and
       XDrawImage16 for the text.  */
    XDrawString16(dpy, offscreen, xgc,
      -charinfo->lbearing + 8 * maxSpanLength * numToGrab,
      charinfo->ascent, &character, 1);

    numToGrab++;

  PossiblyDoGrab:

    if (numToGrab >= glyphsPerGrab || i == numchars - 1) {
      image = XGetImage(dpy, offscreen,
        0, 0, pixwidth, height, 1, XYPixmap);
      for (j = 0; j < numToGrab; j++) {
        thisglyph = grabList[j];
        if (fontinfo->per_char != NULL) {
          charinfo = &(fontinfo->per_char[thisglyph]);
          charWidth = charinfo->rbearing - charinfo->lbearing;
          charHeight = charinfo->ascent + charinfo->descent;
          spanLength = (charWidth + 7) / 8;
        }
        bitmapData = calloc(height * spanLength, sizeof(char));
        if (!bitmapData)
          goto FreeFontAndReturn;
        DEBUG_GLYPH4("index %d, glyph %d (%d by %d)\n",
          j, thisglyph + fontinfo->min_char_or_byte2, charWidth, charHeight);
        for (y = 0; y < charHeight; y++) {
          for (x = 0; x < charWidth; x++) {
            /* XXX The algorithm used to suck across the font ensures that
               each glyph begins on a byte boundary.  In theory this would
               make it convienent to copy the glyph into a byte oriented
               bitmap.  We actually use the XGetPixel function to extract
               each pixel from the image which is not that efficient.  We
               could either do tighter packing in the pixmap or more
               efficient extraction from the image.  Oh well.  */
            if (XGetPixel(image, j * maxSpanLength * 8 + x, charHeight - 1 - y)) {
              DEBUG_GLYPH("x");
              bitmapData[y * spanLength + x / 8] |= (1 << (x & 7));
            } else {
              DEBUG_GLYPH(" ");
            }
          }
          DEBUG_GLYPH("\n");
        }
        myfontinfo->glyph[thisglyph].width = charWidth;
        myfontinfo->glyph[thisglyph].height = charHeight;
        myfontinfo->glyph[thisglyph].xoffset = charinfo->lbearing;
        myfontinfo->glyph[thisglyph].yoffset = -charinfo->descent;
        myfontinfo->glyph[thisglyph].advance = charinfo->width;
        myfontinfo->glyph[thisglyph].bitmap = bitmapData;
      }
      XDestroyImage(image);
      numToGrab = 0;
      /* do we need to clear the offscreen pixmap to get more? */
      if (i < numchars - 1) {
        XSetForeground(dpy, xgc, 0);
        XFillRectangle(dpy, offscreen, xgc, 0, 0, 8 * maxSpanLength * glyphsPerGrab, height);
        XSetForeground(dpy, xgc, 1);
      }
    }
  }
  XFreeGC(dpy, xgc);
  XFreePixmap(dpy, offscreen);
  return myfontinfo;

FreeFontAndReturn:
  XDestroyImage(image);
  XFreeGC(dpy, xgc);
  XFreePixmap(dpy, offscreen);
  for (j = i - 1; j >= 0; j--) {
    if (myfontinfo->glyph[j].bitmap)
      free(myfontinfo->glyph[j].bitmap);
  }
  free(myfontinfo);
  return NULL;
}

void
printGlyph(FontInfoPtr font, int c)
{
  PerGlyphInfoPtr glyph;
  unsigned char *bitmapData;
  int width, height, spanLength;
  int x, y;

  if (c < font->min_char || c > font->max_char) {
    printf("out of range glyph\n");
    return;
  }
  glyph = &font->glyph[c - font->min_char];
  bitmapData = glyph->bitmap;
  if (bitmapData) {
    width = glyph->width;
    spanLength = (width + 7) / 8;
    height = glyph->height;

    for (y = 0; y < height; y++) {
      for (x = 0; x < width; x++) {
        if (bitmapData[y * spanLength + x / 8] & (1 << (x & 7))) {
          putchar('X');
        } else {
          putchar('.');
        }
      }
      putchar('\n');
    }
  }
}

void
getMetric(FontInfoPtr font, int c, TexGlyphInfo * tgi)
{
  PerGlyphInfoPtr glyph;
  unsigned char *bitmapData;

  tgi->c = c;
  if (c < font->min_char || c > font->max_char) {
    tgi->width = 0;
    tgi->height = 0;
    tgi->xoffset = 0;
    tgi->yoffset = 0;
    tgi->dummy = 0;
    tgi->advance = 0;
    return;
  }
  glyph = &font->glyph[c - font->min_char];
  bitmapData = glyph->bitmap;
  if (bitmapData) {
    tgi->width = glyph->width;
    tgi->height = glyph->height;
    tgi->xoffset = glyph->xoffset;
    tgi->yoffset = glyph->yoffset;
  } else {
    tgi->width = 0;
    tgi->height = 0;
    tgi->xoffset = 0;
    tgi->yoffset = 0;
  }
  tgi->dummy = 0;
  tgi->advance = glyph->advance;
}

int
glyphCompare(const void *a, const void *b)
{
  unsigned char *c1 = (unsigned char *) a;
  unsigned char *c2 = (unsigned char *) b;
  TexGlyphInfo tgi1;
  TexGlyphInfo tgi2;

  getMetric(fontinfo, *c1, &tgi1);
  getMetric(fontinfo, *c2, &tgi2);
  return tgi2.height - tgi1.height;
}

int
getFontel(unsigned char *bitmapData, int spanLength, int i, int j)
{
  return bitmapData[i * spanLength + j / 8] & (1 << (j & 7)) ? 255 : 0;
}

void
placeGlyph(FontInfoPtr font, int c, unsigned char *texarea, int stride, int x, int y)
{
  PerGlyphInfoPtr glyph;
  unsigned char *bitmapData;
  int width, height, spanLength;
  int i, j;

  if (c < font->min_char || c > font->max_char) {
    printf("out of range glyph\n");
    return;
  }
  glyph = &font->glyph[c - font->min_char];
  bitmapData = glyph->bitmap;
  if (bitmapData) {
    width = glyph->width;
    spanLength = (width + 7) / 8;
    height = glyph->height;

    for (i = 0; i < height; i++) {
      for (j = 0; j < width; j++) {
        texarea[stride * (y + i) + x + j] =
          getFontel(bitmapData, spanLength, i, j);
      }
    }
  }
}

char *
nodupstring(char *s)
{
  int len, i, p;
  char *string;

  len = (int) strlen(s);
  string = (char *) calloc(len + 1, 1);
  p = 0;
  for (i = 0; i < len; i++) {
    if (!strchr(string, s[i])) {
      string[p] = s[i];
      p++;
    }
  }
  string = realloc(string, p + 1);
  return string;
}

void
main(int argc, char *argv[])
{
  int texw, texh;
  unsigned char *texarea, *texbitmap;
  FILE *file;
  int len, stride;
  unsigned char *glist;
  int width, height;
  int px, py, maxheight;
  TexGlyphInfo tgi;
  int usageError = 0;
  char *fontname, *filename;
  XFontStruct *xfont;
  int endianness;
  int i, j;

  texw = texh = 256;
  glist = " ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijmklmnopqrstuvwxyz?.;,!*:\"/+@#$%^&()";
  fontname = "-adobe-courier-bold-r-normal--46-*-100-100-m-*-iso8859-1";
  filename = "default.txf";

  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-w")) {
      i++;
      texw = atoi(argv[i]);
    } else if (!strcmp(argv[i], "-h")) {
      i++;
      texh = atoi(argv[i]);
    } else if (!strcmp(argv[i], "-gap")) {
      i++;
      gap = atoi(argv[i]);
    } else if (!strcmp(argv[i], "-byte")) {
      format = TXF_FORMAT_BYTE;
      break;
    } else if (!strcmp(argv[i], "-bitmap")) {
      format = TXF_FORMAT_BITMAP;
    } else if (!strcmp(argv[i], "-glist")) {
      i++;
      glist = (unsigned char *) argv[i];
    } else if (!strcmp(argv[i], "-fn")) {
      i++;
      fontname = argv[i];
    } else if (!strcmp(argv[i], "-file")) {
      i++;
      filename = argv[i];
    } else {
      usageError = 1;
    }
  }

  if (usageError) {
    putchar('\n');
    printf("usage: texfontgen [options] txf-file\n");
    printf(" -w #          textureWidth (def=%d)\n", texw);
    printf(" -h #          textureHeight (def=%d)\n", texh);
    printf(" -gap #        gap between glyphs (def=%d)\n", gap);
    printf(" -bitmap       use a bitmap encoding (default)\n");
    printf(" -byte         use a byte encoding (less compact)\n");
    printf(" -glist ABC    glyph list (def=%s)\n", glist);
    printf(" -fn name      X font name (def=%s)\n", fontname);
    printf(" -file name    output file for textured font (def=%s)\n", fontname);
    putchar('\n');
    exit(1);
  }
  texarea = calloc(texw * texh, sizeof(unsigned char));
  glist = (unsigned char *) nodupstring((char *) glist);

  dpy = XOpenDisplay(NULL);
  if (!dpy) {
    printf("could not open display\n");
    exit(1);
  }
  /* find an OpenGL-capable RGB visual with depth buffer */
  xfont = XLoadQueryFont(dpy, fontname);
  if (!xfont) {
    printf("could not get load X font: %s\n", fontname);
    exit(1);
  }
  fontinfo = SuckGlyphsFromServer(dpy, xfont->fid);
  if (!fontinfo) {
    printf("could not get font glyphs\n");
    exit(1);
  }
  len = (int) strlen((char *) glist);
  qsort(glist, len, sizeof(unsigned char), glyphCompare);

  file = fopen(filename, "wb");
  if (!file) {
    printf("could not open %s for writing\n", filename);
    exit(1);
  }
  fwrite("\377txf", 1, 4, file);
  endianness = 0x12345678;
  /*CONSTANTCONDITION*/
  assert(sizeof(int) == 4);  /* Ensure external file format size. */
  fwrite(&endianness, sizeof(int), 1, file);
  fwrite(&format, sizeof(int), 1, file);
  fwrite(&texw, sizeof(int), 1, file);
  fwrite(&texh, sizeof(int), 1, file);
  fwrite(&fontinfo->max_ascent, sizeof(int), 1, file);
  fwrite(&fontinfo->max_descent, sizeof(int), 1, file);
  fwrite(&len, sizeof(int), 1, file);

  px = gap;
  py = gap;
  maxheight = 0;
  for (i = 0; i < len; i++) {
    if (glist[i] != 0) {  /* If not already processed... */

      /* Try to find a character from the glist that will fit on the
         remaining space on the current row. */

      int foundWidthFit = 0;
      int c;

      getMetric(fontinfo, glist[i], &tgi);
      width = tgi.width;
      height = tgi.height;
      if (height > 0 && width > 0) {
        for (j = i; j < len;) {
          if (height > 0 && width > 0) {
            if (px + width + gap < texw) {
              foundWidthFit = 1;
	      if (j != i) {
		i--;  /* Step back so i loop increment leaves us at same character. */
	      }
              break;
            }
	  }
          j++;
          getMetric(fontinfo, glist[j], &tgi);
          width = tgi.width;
          height = tgi.height;
        }

        /* If a fit was found, use that character; otherwise, advance a line
           in  the texture. */
        if (foundWidthFit) {
          if (height > maxheight) {
            maxheight = height;
          }
          c = j;
        } else {
          getMetric(fontinfo, glist[i], &tgi);
          width = tgi.width;
          height = tgi.height;

          py += maxheight + gap;
          px = gap;
          maxheight = height;
          if (py + height + gap >= texh) {
            printf("Overflowed texture space.\n");
            exit(1);
          }
          c = i;
        }

        /* Place the glyph in the texture image. */
        placeGlyph(fontinfo, glist[c], texarea, texw, px, py);

        /* Assign glyph's texture coordinate. */
        tgi.x = px;
        tgi.y = py;

	/* Advance by glyph width, remaining in the current line. */
        px += width + gap;
      } else {
	/* No texture image; assign invalid bogus texture coordinates. */
        tgi.x = -1;
        tgi.y = -1;
      }
      glist[c] = 0;     /* Mark processed; don't process again. */
      /*CONSTANTCONDITION*/
      assert(sizeof(tgi) == 12);  /* Ensure external file format size. */
      fwrite(&tgi, sizeof(tgi), 1, file);
    }
  }

  switch (format) {
  case TXF_FORMAT_BYTE:
    fwrite(texarea, texw * texh, 1, file);
    break;
  case TXF_FORMAT_BITMAP:
    stride = (texw + 7) >> 3;
    texbitmap = (unsigned char *) calloc(stride * texh, 1);
    for (i = 0; i < texh; i++) {
      for (j = 0; j < texw; j++) {
        if (texarea[i * texw + j] >= 128) {
          texbitmap[i * stride + (j >> 3)] |= 1 << (j & 7);
        }
      }
    }
    fwrite(texbitmap, stride * texh, 1, file);
    free(texbitmap);
    break;
  default:
    printf("Unknown texture font format.\n");
    exit(1);
  }
  free(texarea);
  fclose(file);
}
