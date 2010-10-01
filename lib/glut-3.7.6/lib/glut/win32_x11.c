
/* Copyright (c) Nate Robins, 1997. */
/* portions Copyright (c) Mark Kilgard, 1998. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include "glutint.h"

/* global variable that must be set for some functions to operate
   correctly. */
HDC XHDC;

XVisualInfo*
XGetVisualInfo(Display* display, long mask, XVisualInfo* template, int* nitems)
{
  /* KLUDGE: this function needs XHDC to be set to the HDC currently
     being operated on before it is invoked! */

  PIXELFORMATDESCRIPTOR* pfds = NULL;
  int i, n;

  n = DescribePixelFormat(XHDC, 1, 0, NULL);

  if (n > 0) {
    pfds = (PIXELFORMATDESCRIPTOR*)malloc(sizeof(PIXELFORMATDESCRIPTOR) * n);
    memset(pfds, 0, sizeof(PIXELFORMATDESCRIPTOR) * n);
  }
 
  for (i = 0; i < n; i++) {
    DescribePixelFormat(XHDC, i + 1, sizeof(PIXELFORMATDESCRIPTOR), &pfds[i]);
  }

  *nitems = n;
  return pfds;
}

Colormap
XCreateColormap(Display* display, Window root, Visual* visual, int alloc)
{
  /* KLUDGE: this function needs XHDC to be set to the HDC currently
     being operated on before it is invoked! */

  PIXELFORMATDESCRIPTOR pfd;
  LOGPALETTE *logical;
  HPALETTE    palette;
  int n;

  /* grab the pixel format */
  memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
  DescribePixelFormat(XHDC, GetPixelFormat(XHDC), 
		      sizeof(PIXELFORMATDESCRIPTOR), &pfd);

  if (!(pfd.dwFlags & PFD_NEED_PALETTE ||
      pfd.iPixelType == PFD_TYPE_COLORINDEX))
  {
    return 0;
  }

  n = 1 << pfd.cColorBits;

  /* allocate a bunch of memory for the logical palette (assume 256
     colors in a Win32 palette */
  logical = (LOGPALETTE*)malloc(sizeof(LOGPALETTE) +
				sizeof(PALETTEENTRY) * n);
  memset(logical, 0, sizeof(LOGPALETTE) + sizeof(PALETTEENTRY) * n);

  /* set the entries in the logical palette */
  logical->palVersion = 0x300;
  logical->palNumEntries = n;

  /* start with a copy of the current system palette */
  GetSystemPaletteEntries(XHDC, 0, 256, &logical->palPalEntry[0]);
    
  if (pfd.iPixelType == PFD_TYPE_RGBA) {
    int redMask = (1 << pfd.cRedBits) - 1;
    int greenMask = (1 << pfd.cGreenBits) - 1;
    int blueMask = (1 << pfd.cBlueBits) - 1;
    int i;

    /* fill in an RGBA color palette */
    for (i = 0; i < n; ++i) {
      logical->palPalEntry[i].peRed = 
	(((i >> pfd.cRedShift)   & redMask)   * 255) / redMask;
      logical->palPalEntry[i].peGreen = 
	(((i >> pfd.cGreenShift) & greenMask) * 255) / greenMask;
	logical->palPalEntry[i].peBlue = 
	(((i >> pfd.cBlueShift)  & blueMask)  * 255) / blueMask;
      logical->palPalEntry[i].peFlags = 0;
    }
  }

  palette = CreatePalette(logical);
  free(logical);

  SelectPalette(XHDC, palette, FALSE);
  RealizePalette(XHDC);

  return palette;
}

void
XAllocColorCells(Display* display, Colormap colormap, Bool contig, 
		 unsigned long plane_masks_return[], unsigned int nplanes,
		 unsigned long pixels_return[], unsigned int npixels)
{
  /* NOP -- we did all the allocating in XCreateColormap! */
}

void
XStoreColor(Display* display, Colormap colormap, XColor* color)
{
  /* KLUDGE: set XHDC to 0 if the palette should NOT be realized after
     setting the color.  set XHDC to the correct HDC if it should. */

  PALETTEENTRY pe;

  /* X11 stores color from 0-65535, Win32 expects them to be 0-256, so
     twiddle the bits ( / 256). */
  pe.peRed = color->red / 256;
  pe.peGreen = color->green / 256;
  pe.peBlue = color->blue / 256;

  /* make sure we use this flag, otherwise the colors might get mapped
     to another place in the colormap, and when we glIndex() that
     color, it may have moved (argh!!) */
  pe.peFlags = PC_NOCOLLAPSE;

  /* the pixel field of the XColor structure is the index into the
     colormap */
  SetPaletteEntries(colormap, color->pixel, 1, &pe);

  if (XHDC) {
    UnrealizeObject(colormap);
    SelectPalette(XHDC, colormap, FALSE);
    RealizePalette(XHDC);
  }
}

void
XSetWindowColormap(Display* display, Window window, Colormap colormap)
{
  HDC hdc = GetDC(window);

  /* if the third parameter is FALSE, the logical colormap is copied
     into the device palette when the application is in the
     foreground, if it is TRUE, the colors are mapped into the current
     palette in the best possible way. */
  SelectPalette(hdc, colormap, FALSE);
  RealizePalette(hdc);

  /* note that we don't have to release the DC, since our window class
     uses the WC_OWNDC flag! */
}

Bool
XTranslateCoordinates(Display *display, Window src, Window dst, 
		      int src_x, int src_y, 
		      int* dest_x_return, int* dest_y_return,
		      Window* child_return)
{
  /* KLUDGE: this isn't really a translate coordinates into some other
  windows coordinate system...it only translates coordinates into the
  root window (screen) coordinate system. */

  POINT point;

  point.x = src_x;
  point.y = src_y;

  ClientToScreen(src, &point);

  *dest_x_return = point.x;
  *dest_y_return = point.y;

  /* just to make compilers happy...we don't use the return value. */
  return True;
}

Status
XGetGeometry(Display* display, Window window, Window* root_return, 
	     int* x_return, int* y_return, 
	     unsigned int* width_return, unsigned int* height_return,
	     unsigned int *border_width_return, unsigned int* depth_return)
{
  /* KLUDGE: doesn't return the border_width or depth or root, x & y
     are in screen coordinates. */

  RECT rect;
  POINT point;

  GetClientRect(window, &rect);

  point.x = 0;
  point.y = 0;
  ClientToScreen(window, &point);

  *x_return = point.x;
  *y_return = point.y;
  *width_return = rect.right;
  *height_return = rect.bottom;

  /* just to make compilers happy...we don't use the return value. */
  return 1;  
}

int
DisplayWidthMM(Display* display, int screen)
{
  int width;
  HWND hwnd = GetDesktopWindow();
  HDC hdc = GetDC(hwnd);
  
  width = GetDeviceCaps(hdc, HORZSIZE);

  /* make sure to release this DC (it's the desktops, not ours) */
  ReleaseDC(hwnd, hdc);

  return width;
}

int
DisplayHeightMM(Display* display, int screen)
{
  int height;
  HWND hwnd = GetDesktopWindow();
  HDC hdc = GetDC(hwnd);
  
  height = GetDeviceCaps(hdc, VERTSIZE);

  /* make sure to release this DC (it's the desktops, not ours) */
  ReleaseDC(hwnd, hdc);

  return height;
}

void
XWarpPointer(Display* display, Window src, Window dst, 
	     int src_x, int src_y, int src_width, int src_height,
	     int dst_x, int dst_y)
{
  /* KLUDGE: this isn't really a warp pointer into some other windows
  coordinate system...it only warps the pointer into the root window
  (screen) coordinate system. */

  POINT point;

  point.x = dst_x;
  point.y = dst_y;
  ClientToScreen(dst, &point);

  SetCursorPos(point.x, point.y);
}

int
XPending(Display* display)
{
  /* similar functionality...I don't think that it is exact, but this
     will have to do. */
  MSG msg;

  return PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
}

/* the following function was stolen from the X sources as indicated. */

/* Copyright 	Massachusetts Institute of Technology  1985, 1986, 1987 */
/* $XConsortium: XParseGeom.c,v 11.18 91/02/21 17:23:05 rws Exp $ */

/*
Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation, and that the name of M.I.T. not be used in advertising or
publicity pertaining to distribution of the software without specific,
written prior permission.  M.I.T. makes no representations about the
suitability of this software for any purpose.  It is provided "as is"
without express or implied warranty.
*/

/*
 *    XParseGeometry parses strings of the form
 *   "=<width>x<height>{+-}<xoffset>{+-}<yoffset>", where
 *   width, height, xoffset, and yoffset are unsigned integers.
 *   Example:  "=80x24+300-49"
 *   The equal sign is optional.
 *   It returns a bitmask that indicates which of the four values
 *   were actually found in the string.  For each value found,
 *   the corresponding argument is updated;  for each value
 *   not found, the corresponding argument is left unchanged. 
 */

static int
ReadInteger(char *string, char **NextString)
{
    register int Result = 0;
    int Sign = 1;
    
    if (*string == '+')
	string++;
    else if (*string == '-')
    {
	string++;
	Sign = -1;
    }
    for (; (*string >= '0') && (*string <= '9'); string++)
    {
	Result = (Result * 10) + (*string - '0');
    }
    *NextString = string;
    if (Sign >= 0)
	return (Result);
    else
	return (-Result);
}

int XParseGeometry(char *string, int *x, int *y, unsigned int *width, unsigned int *height)
{
	int mask = NoValue;
	register char *strind;
	unsigned int tempWidth, tempHeight;
	int tempX, tempY;
	char *nextCharacter;

	if ( (string == NULL) || (*string == '\0')) return(mask);
	if (*string == '=')
		string++;  /* ignore possible '=' at beg of geometry spec */

	strind = (char *)string;
	if (*strind != '+' && *strind != '-' && *strind != 'x') {
		tempWidth = ReadInteger(strind, &nextCharacter);
		if (strind == nextCharacter) 
		    return (0);
		strind = nextCharacter;
		mask |= WidthValue;
	}

	if (*strind == 'x' || *strind == 'X') {	
		strind++;
		tempHeight = ReadInteger(strind, &nextCharacter);
		if (strind == nextCharacter)
		    return (0);
		strind = nextCharacter;
		mask |= HeightValue;
	}

	if ((*strind == '+') || (*strind == '-')) {
		if (*strind == '-') {
  			strind++;
			tempX = -ReadInteger(strind, &nextCharacter);
			if (strind == nextCharacter)
			    return (0);
			strind = nextCharacter;
			mask |= XNegative;

		}
		else
		{	strind++;
			tempX = ReadInteger(strind, &nextCharacter);
			if (strind == nextCharacter)
			    return(0);
			strind = nextCharacter;
		}
		mask |= XValue;
		if ((*strind == '+') || (*strind == '-')) {
			if (*strind == '-') {
				strind++;
				tempY = -ReadInteger(strind, &nextCharacter);
				if (strind == nextCharacter)
			    	    return(0);
				strind = nextCharacter;
				mask |= YNegative;

			}
			else
			{
				strind++;
				tempY = ReadInteger(strind, &nextCharacter);
				if (strind == nextCharacter)
			    	    return(0);
				strind = nextCharacter;
			}
			mask |= YValue;
		}
	}
	
	/* If strind isn't at the end of the string the it's an invalid
		geometry specification. */

	if (*strind != '\0') return (0);

	if (mask & XValue)
	    *x = tempX;
 	if (mask & YValue)
	    *y = tempY;
	if (mask & WidthValue)
            *width = tempWidth;
	if (mask & HeightValue)
            *height = tempHeight;
	return (mask);
}
