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

float o_data[][2] = {
	{2.975610, 9.603255},
	{2.878049, 9.342828},
	{2.292683, 9.131231},
	{2.048780, 8.691760},
	{1.707317, 8.528993},
	{1.658537, 7.731434},
	{0.878049, 7.047813},
	{1.349594, 5.550356},
	{0.569106, 5.029501},
	{1.528455, 4.443540},
	{0.991870, 3.434385},
	{1.967480, 3.955239},
	{1.772358, 2.994914},
	{2.422764, 3.825025},
	{2.829268, 3.092574},
	{3.154472, 3.971516},
	{3.512195, 3.727365},
	{3.772358, 4.264496},
	{4.130081, 4.524924},
	{4.162601, 4.996948},
	{4.699187, 5.403866},
	{4.471545, 6.461852},
	{5.219512, 7.243133},
	{4.439024, 8.105799},
	{5.235772, 8.756866},
	{4.065041, 8.870804},
	{4.991870, 9.391658},
	{3.853658, 9.228891},
	{4.390244, 9.912513},
	{3.463415, 9.407935},
	{3.674797, 9.912513},
	{2.829268, 9.342828},
	{2.959350, 9.603255},

};

void draw_o(void) {

    glBegin(GL_TRIANGLE_STRIP);
	glVertex2fv(o_data[0]);
	glVertex2fv(o_data[1]);
	glVertex2fv(o_data[2]);
	glVertex2fv(o_data[3]);
	glVertex2fv(o_data[4]);
	glVertex2fv(o_data[5]);
	glVertex2fv(o_data[6]);
	glVertex2fv(o_data[7]);
	glVertex2fv(o_data[8]);
	glVertex2fv(o_data[9]);
	glVertex2fv(o_data[10]);
	glVertex2fv(o_data[11]);
	glVertex2fv(o_data[12]);
	glVertex2fv(o_data[13]);
	glVertex2fv(o_data[14]);
	glVertex2fv(o_data[15]);
	glVertex2fv(o_data[16]);
	glVertex2fv(o_data[17]);
	glVertex2fv(o_data[18]);
	glVertex2fv(o_data[19]);
	glVertex2fv(o_data[20]);
	glVertex2fv(o_data[21]);
	glVertex2fv(o_data[22]);
	glVertex2fv(o_data[23]);
	glVertex2fv(o_data[24]);
	glVertex2fv(o_data[25]);
	glVertex2fv(o_data[26]);
	glVertex2fv(o_data[27]);
	glVertex2fv(o_data[28]);
	glVertex2fv(o_data[29]);
	glVertex2fv(o_data[30]);
	glVertex2fv(o_data[31]);
	glVertex2fv(o_data[32]);
    glEnd();

    glBegin(GL_LINE_STRIP);
	glVertex2fv(o_data[0]);
	glVertex2fv(o_data[2]);
	glVertex2fv(o_data[4]);
	glVertex2fv(o_data[6]);
	glVertex2fv(o_data[8]);
	glVertex2fv(o_data[10]);
	glVertex2fv(o_data[12]);
	glVertex2fv(o_data[14]);
	glVertex2fv(o_data[16]);
	glVertex2fv(o_data[18]);
	glVertex2fv(o_data[20]);
	glVertex2fv(o_data[22]);
	glVertex2fv(o_data[24]);
	glVertex2fv(o_data[26]);
	glVertex2fv(o_data[28]);
	glVertex2fv(o_data[30]);
	glVertex2fv(o_data[32]);
	glVertex2fv(o_data[31]);
	glVertex2fv(o_data[29]);
	glVertex2fv(o_data[27]);
	glVertex2fv(o_data[25]);
	glVertex2fv(o_data[23]);
	glVertex2fv(o_data[21]);
	glVertex2fv(o_data[19]);
	glVertex2fv(o_data[17]);
	glVertex2fv(o_data[15]);
	glVertex2fv(o_data[13]);
	glVertex2fv(o_data[11]);
	glVertex2fv(o_data[9]);
	glVertex2fv(o_data[7]);
	glVertex2fv(o_data[5]);
	glVertex2fv(o_data[3]);
	glVertex2fv(o_data[1]);
    glEnd();

}

