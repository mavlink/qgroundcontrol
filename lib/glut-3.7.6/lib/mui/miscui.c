/*
 * Copyright (c) 1990,1997, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED 
 * Permission to use, copy, modify, and distribute this software for 
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that 
 * the name of Silicon Graphics, Inc. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission. 
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
 * AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
 * KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION,
 * LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF
 * THIRD PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
 * POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * US Government Users Restricted Rights 
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor
 * clauses in the FAR or the DOD or NASA FAR Supplement.
 * Unpublished-- rights reserved under the copyright laws of the
 * United States.  Contractor/manufacturer is Silicon Graphics,
 * Inc., 2011 N.  Shoreline Blvd., Mountain View, CA 94039-7311.
 *
 * OpenGL(R) is a registered trademark of Silicon Graphics, Inc.
 */

#include <GL/glut.h>
#include <mui/displaylist.h>
#include <mui/uicolor.h>
#include <mui/gizmo.h>

extern int mui_xsize, mui_ysize;

short	sharefont1 = 0;
short	sharefont2 = 0;

void	drawedges(int xmin,int xmax,int ymin,int ymax,
		  void (*topleft)(void), void (*bottomright)(void))
{
    (*topleft)();
        uimove2i(xmin,ymin);
        uidraw2i(xmin,ymax);
        uidraw2i(xmax,ymax);
	uiendline();

    (*bottomright)();
        uimove2i(xmax,ymax-1);
        uidraw2i(xmax,ymin);
        uidraw2i(xmin+1,ymin);
	uiendline();
}

static short called = 0;

void	drawsetup(void)
{
    if (called) return;
    else called = 1;
}

void	drawrestore(void)
{
    called = 0;
}

void	backgrounddraw(int xmin, int ymin, int xmax, int ymax)
{
    drawsetup();

    uiBackground();
        uirectfi(xmin, ymin, xmax, ymax);

    drawrestore();
}

void	muiBackgroundClear(void)
{
    drawsetup();

    uiBackground();
        uiclear();

    drawrestore();
}

#if 0
void	windowborderdraw(void)
{
    int sxsize, sysize;

    sxsize = mui_xsize-1; 	sysize = mui_ysize-1;

    drawsetup();

    uiBlack();
    	uirecti(0,0,sxsize,sysize);

    uiWhite();
    	uirecti(0+1,0+1,sxsize-1,sysize-1);
    	uirecti(0+2,0+2,sxsize-2,sysize-2);

    uiLtGray();
    	uirecti(0+3,0+3,sxsize-3,sysize-3);
    	uirecti(0+4,0+4,sxsize-4,sysize-4);
    	uirecti(0+5,0+5,sxsize-5,sysize-5);
    	uirecti(0+6,0+6,sxsize-6,sysize-6);

    uiBlack();
    	uirecti(0+7,0+7,sxsize-7,sysize-7);

    uiDkGray();
    	uimove2i(0+1,0+1); 	uidraw2i(sxsize-1,0+1);	uiendline();
    	uimove2i(0+2,0+2); 	uidraw2i(sxsize-2,0+2);	uiendline();
    	uimove2i(0+3,0+3); 	uidraw2i(sxsize-3,0+3);	uiendline();

    drawrestore();
}
#endif
