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

static float a_data[][2] = {
	{5.618949, 10.261048},
	{5.322348, 9.438848},
	{5.124614, 10.030832},
	{4.860968, 9.488181},
	{4.811534, 9.932169},
	{3.938208, 9.438848},
	{3.658084, 9.685509},
	{2.784758, 8.994862},
	{2.801236, 9.175745},
	{1.960865, 8.172662},
	{1.186406, 7.761562},
	{1.252317, 6.561151},
	{0.576725, 6.610483},
	{0.939238, 5.525180},
	{0.164779, 4.883864},
	{0.840371, 4.818089},
	{0.230690, 3.963001},
	{0.939238, 4.242549},
	{0.609681, 3.255909},
	{1.268795, 3.963001},
	{1.021627, 3.075026},
	{1.861998, 4.045221},
	{1.829042, 3.535457},
	{2.817714, 4.818089},
	{3.163749, 4.998972},
	{3.971164, 6.643371},
	{4.267765, 6.725591},
	{4.663234, 7.630010},

	{5.404737, 9.734840},
	{4.646756, 9.669065},
	{5.108136, 8.731757},
	{4.679712, 8.600205},
	{4.926879, 7.564234},
	{4.366632, 6.692703},
	{4.663234, 5.344296},
	{3.888774, 4.850976},
	{4.630278, 4.094553},
	{3.954686, 3.963001},
	{4.828012, 3.798561},
	{4.168898, 3.321686},
	{5.157569, 3.864337},
	{4.514933, 3.091470},
	{5.553038, 4.045221},
	{5.305870, 3.634121},
	{5.932029, 4.176773},

};

void draw_a(void) {

    glBegin(GL_TRIANGLE_STRIP);
	glVertex2fv(a_data[0]);
	glVertex2fv(a_data[1]);
	glVertex2fv(a_data[2]);
	glVertex2fv(a_data[3]);
	glVertex2fv(a_data[4]);
	glVertex2fv(a_data[5]);
	glVertex2fv(a_data[6]);
	glVertex2fv(a_data[7]);
	glVertex2fv(a_data[8]);
	glVertex2fv(a_data[9]);
	glVertex2fv(a_data[10]);
	glVertex2fv(a_data[11]);
	glVertex2fv(a_data[12]);
	glVertex2fv(a_data[13]);
	glVertex2fv(a_data[14]);
	glVertex2fv(a_data[15]);
	glVertex2fv(a_data[16]);
	glVertex2fv(a_data[17]);
	glVertex2fv(a_data[18]);
	glVertex2fv(a_data[19]);
	glVertex2fv(a_data[20]);
	glVertex2fv(a_data[21]);
	glVertex2fv(a_data[22]);
	glVertex2fv(a_data[23]);
	glVertex2fv(a_data[24]);
	glVertex2fv(a_data[25]);
	glVertex2fv(a_data[26]);
	glVertex2fv(a_data[27]);
    glEnd();

    glBegin(GL_TRIANGLE_STRIP);
	glVertex2fv(a_data[28]);
	glVertex2fv(a_data[29]);
	glVertex2fv(a_data[30]);
	glVertex2fv(a_data[31]);
	glVertex2fv(a_data[32]);
	glVertex2fv(a_data[33]);
	glVertex2fv(a_data[34]);
	glVertex2fv(a_data[35]);
	glVertex2fv(a_data[36]);
	glVertex2fv(a_data[37]);
	glVertex2fv(a_data[38]);
	glVertex2fv(a_data[39]);
	glVertex2fv(a_data[40]);
	glVertex2fv(a_data[41]);
	glVertex2fv(a_data[42]);
	glVertex2fv(a_data[43]);
	glVertex2fv(a_data[44]);
    glEnd();

    glBegin(GL_LINE_STRIP);
	glVertex2fv(a_data[0]);
	glVertex2fv(a_data[2]);
	glVertex2fv(a_data[4]);
	glVertex2fv(a_data[6]);
	glVertex2fv(a_data[8]);
	glVertex2fv(a_data[10]);
	glVertex2fv(a_data[12]);
	glVertex2fv(a_data[14]);
	glVertex2fv(a_data[16]);
	glVertex2fv(a_data[18]);
	glVertex2fv(a_data[20]);
	glVertex2fv(a_data[22]);
	glVertex2fv(a_data[24]);
	glVertex2fv(a_data[26]);
	glVertex2fv(a_data[27]);
	glVertex2fv(a_data[25]);
	glVertex2fv(a_data[23]);
	glVertex2fv(a_data[21]);
	glVertex2fv(a_data[19]);
	glVertex2fv(a_data[17]);
	glVertex2fv(a_data[15]);
	glVertex2fv(a_data[13]);
	glVertex2fv(a_data[11]);
	glVertex2fv(a_data[9]);
	glVertex2fv(a_data[7]);
	glVertex2fv(a_data[5]);
	glVertex2fv(a_data[3]);
	glVertex2fv(a_data[1]);
    glEnd();

    glBegin(GL_LINE_STRIP);
	glVertex2fv(a_data[28]);
	glVertex2fv(a_data[30]);
	glVertex2fv(a_data[32]);
	glVertex2fv(a_data[34]);
	glVertex2fv(a_data[36]);
	glVertex2fv(a_data[38]);
	glVertex2fv(a_data[40]);
	glVertex2fv(a_data[42]);
	glVertex2fv(a_data[44]);
	glVertex2fv(a_data[43]);
	glVertex2fv(a_data[41]);
	glVertex2fv(a_data[39]);
	glVertex2fv(a_data[37]);
	glVertex2fv(a_data[35]);
	glVertex2fv(a_data[33]);
	glVertex2fv(a_data[31]);
	glVertex2fv(a_data[29]);
    glEnd();

}

