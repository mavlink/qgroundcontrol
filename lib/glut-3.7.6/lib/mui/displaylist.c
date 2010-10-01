/*
 * Copyright (c) 1993-1997, Silicon Graphics, Inc.
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

#include <mui/displaylist.h>
#include <GL/glut.h>
#include <string.h>

#define MOVE2I	1
#define DRAW2I	2
#define RECTFI	3
#define CMOV2I	4
#define CHARSTR	5
#define PMV2I	6
#define PDR2I	7
#define PCLOS	8
#define RECTI	9
#define CLEAR	10
#define ENDLINE 11
#define PUSHVP	12
#define POPVP	13
#define VP	14

extern short ditherflag;

void charstr(char *s, int font)
{
    int len = (int) strlen(s);
    int i;
    void **f;
    switch(font) {
	case UI_FONT_BOLD:
	case UI_FONT_NORMAL:
	    f = GLUT_BITMAP_HELVETICA_12;
	    break;
	case UI_FONT_FIXED_PITCH:
	    f = GLUT_BITMAP_9_BY_15;
	    break;
    }
    for (i = 0; i < len; i++)
	glutBitmapCharacter(f, s[i]);
}

void uipushviewport(void)
{
}

void uipopviewport(void)
{
    glDisable(GL_SCISSOR_TEST);
}

void uiviewport(int x, int y, int width, int height)
{
    glScissor(x, y, width, height);
    glEnable(GL_SCISSOR_TEST);
}

void uicharstr(char *s, int font)
{
    charstr(s, font);
}

void uirecti(int x, int y, int z, int w)
{
    glBegin(GL_LINE_LOOP);
    glVertex2i(x, y);
    glVertex2i(x, w);
    glVertex2i(z, w);
    glVertex2i(z, y);
    glEnd();
}

void uirectfi(int x, int y, int x1, int y1)
{
    glRecti(x, y, x1, y1);
}

void uipclos(void)
{
    glEnd();
}

void uiclear(void)
{
    glClearColor(214.0/255.0, 214.0/255.0, 214.0/255.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
}

void uipdr2i(int x, int y)
{
    glVertex2i(x, y);
}

void uipmv2i(int x, int y)
{
    glBegin(GL_POLYGON);
    glVertex2i(x, y);
}

void uicmov2i(int x, int y)
{
    glRasterPos2i(x, y);
}

void uidraw2i(int x, int y)
{
    glVertex2i(x, y);
}

void uiendline(void)
{
    glEnd();
}

void uimove2i(int x, int y)
{
    glBegin(GL_LINE_STRIP);
    glVertex2i(x, y);
}

