/*
 * (c) Copyright 1993, Silicon Graphics, Inc.
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
 * OpenGL(TM) is a trademark of Silicon Graphics, Inc.
 */
#include <GL/glut.h>

float r_data[][2] = {
	{0.018462, 12.344969-3.0},
	{0.775385, 12.677618-3.0},
	{0.812308, 12.344969-3.0},
	{1.421538, 12.899384-3.0},
	{1.126154, 12.123203-3.0},
	{1.864615, 12.825462-3.0},
	{1.181538, 11.716633-3.0},
	{2.030769, 12.603696-3.0},
	{1.089231, 10.700206-3.0},
	{1.495385, 9.351130-3.0},
	{0.516923, 7.355236-3.0},
	{0.756923, 6.375770-3.0},
	{0.129231, 5.119096-3.0},
	{0.461538, 5.322382-3.0},

	{1.680000, 10.663244-3.0},
	{1.550769, 9.942505-3.0},
	{2.160000, 11.383984-3.0},
	{2.400000, 11.531828-3.0},
	{2.640000, 12.086243-3.0},
	{2.916923, 12.086243-3.0},
	{3.341538, 12.806981-3.0},
	{3.526154, 12.160164-3.0},
	{4.043077, 12.954825-3.0},
	{4.209231, 12.308008-3.0},
	{4.504615, 12.880903-3.0},
	{4.541538, 12.622176-3.0},

};

void draw_r(void) {

    glBegin(GL_TRIANGLE_STRIP);
	glVertex2fv(r_data[0]);
	glVertex2fv(r_data[1]);
	glVertex2fv(r_data[2]);
	glVertex2fv(r_data[3]);
	glVertex2fv(r_data[4]);
	glVertex2fv(r_data[5]);
	glVertex2fv(r_data[6]);
	glVertex2fv(r_data[7]);
	glVertex2fv(r_data[8]);
	glVertex2fv(r_data[9]);
	glVertex2fv(r_data[10]);
	glVertex2fv(r_data[11]);
	glVertex2fv(r_data[12]);
	glVertex2fv(r_data[13]);
    glEnd();

    glBegin(GL_TRIANGLE_STRIP);
	glVertex2fv(r_data[14]);
	glVertex2fv(r_data[15]);
	glVertex2fv(r_data[16]);
	glVertex2fv(r_data[17]);
	glVertex2fv(r_data[18]);
	glVertex2fv(r_data[19]);
	glVertex2fv(r_data[20]);
	glVertex2fv(r_data[21]);
	glVertex2fv(r_data[22]);
	glVertex2fv(r_data[23]);
	glVertex2fv(r_data[24]);
	glVertex2fv(r_data[25]);
    glEnd();

    glBegin(GL_LINE_STRIP);
	glVertex2fv(r_data[0]);
	glVertex2fv(r_data[2]);
	glVertex2fv(r_data[4]);
	glVertex2fv(r_data[6]);
	glVertex2fv(r_data[8]);
	glVertex2fv(r_data[10]);
	glVertex2fv(r_data[12]);
	glVertex2fv(r_data[13]);
	glVertex2fv(r_data[11]);
	glVertex2fv(r_data[9]);
	glVertex2fv(r_data[7]);
	glVertex2fv(r_data[5]);
	glVertex2fv(r_data[3]);
	glVertex2fv(r_data[1]);
	glVertex2fv(r_data[0]);
    glEnd();

    glBegin(GL_LINE_STRIP);
	glVertex2fv(r_data[14]);
	glVertex2fv(r_data[16]);
	glVertex2fv(r_data[18]);
	glVertex2fv(r_data[20]);
	glVertex2fv(r_data[22]);
	glVertex2fv(r_data[24]);
	glVertex2fv(r_data[25]);
	glVertex2fv(r_data[23]);
	glVertex2fv(r_data[21]);
	glVertex2fv(r_data[19]);
	glVertex2fv(r_data[17]);
	glVertex2fv(r_data[15]);
	glVertex2fv(r_data[14]);
    glEnd();

}

