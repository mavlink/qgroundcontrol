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
#include <mui/uicolor.h>

#define MKRGB(r, g, b)  ((((r)&0xff)<<0) | (((g)&0xff)<<8) | (((b)&0xff)<<16))
extern short ditherflag;

void	uiBlack(void)
{
	glColor3f(0.0, 0.0, 0.0);
}

void	uiWhite(void)
{
	glColor3f(1.0, 1.0, 1.0);
}

void	uiLtGray(void)
{
	glColor3f(170.0/255.0, 170.0/255.0, 170.0/255.0);
}

void	uiDkGray(void)
{
	glColor3f(85.0/255.0, 85.0/255.0, 85.0/255.0);
}

void	uiSlateBlue(void)
{
	glColor3f(113.0/255.0, 113.0/255.0, 198.0/255.0);
}

void uiBlue(void)
{
	glColor3f(0.0, 0.0, 1.0);
}

void	uiBackground(void)
{
    uiVyLtGray();
}

void	uiVyDkGray(void)
{
	glColor3f(40.0/255.0, 40.0/255.0, 40.0/255.0);
}

void	uiMmGray(void)
{
	glColor3f(132.0/255.0, 132.0/255.0, 132.0/255.0);
}

void	uiVyLtGray(void)
{
	glColor3f(214.0/255.0, 214.0/255.0, 214.0/255.0);
}

void	uiTerraCotta(void)
{
	glColor3f(198.0/255.0, 113.0/255.0, 113.0/255.0);
}

void	uiYellow(void)
{
	glColor3f(1.0, 1.0, 0.0);
}

void	uiDkYellow(void)
{
	glColor3f(0x47/255.0, 0x72/255.0, 0x72/255.0);
}

void	uiMmYellow(void)
{
	glColor3f(0x38/255.0, 0xc7/255.0, 0xc7/255.0);
}

void	uiLtYellow(void)
{
	glColor3f(0x80/255.0, 0xff/255.0, 0xff/255.0);
}

/*
void	uiPupBlack(void)
{
    color(PUP_BLACK);
}

void	uiPupWhite(void)
{
    color(PUP_WHITE);
}

void	uiPupGray(void)
{
    color(PUP_COLOR);
}

void	uiPupClear(void)
{
    color(PUP_CLEAR);
}
*/
