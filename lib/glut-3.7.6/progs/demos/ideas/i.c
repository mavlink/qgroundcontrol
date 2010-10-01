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

float i_data[][2] = {
	{0.548767, 9.414791},
	{2.795284, 9.757771},
	{1.457663, 9.311897},
	{2.503751, 9.157557},
	{1.714898, 8.986067},
	{2.109325, 7.785638},
	{1.286174, 7.013934},
	{1.800643, 6.070740},
	{0.994641, 5.161843},
	{1.783494, 4.767417},
	{0.943194, 4.167202},
	{1.852090, 4.304394},
	{1.063237, 3.549839},
	{2.023580, 3.978564},
	{1.406217, 3.172562},
	{2.315113, 3.875670},
	{2.006431, 3.018221},
	{2.812433, 3.944266},
	{2.726688, 3.429796},
	{3.258307, 4.132905},

	{1.989282, 10.923902},
	{2.778135, 12.295820},
	{2.966774, 11.678456},
	{3.687031, 12.947481},

};


void draw_i(void) {

    glBegin(GL_TRIANGLE_STRIP);
	glVertex2fv(i_data[0]);
	glVertex2fv(i_data[1]);
	glVertex2fv(i_data[2]);
	glVertex2fv(i_data[3]);
	glVertex2fv(i_data[4]);
	glVertex2fv(i_data[5]);
	glVertex2fv(i_data[6]);
	glVertex2fv(i_data[7]);
	glVertex2fv(i_data[8]);
	glVertex2fv(i_data[9]);
	glVertex2fv(i_data[10]);
	glVertex2fv(i_data[11]);
	glVertex2fv(i_data[12]);
	glVertex2fv(i_data[13]);
	glVertex2fv(i_data[14]);
	glVertex2fv(i_data[15]);
	glVertex2fv(i_data[16]);
	glVertex2fv(i_data[17]);
	glVertex2fv(i_data[18]);
	glVertex2fv(i_data[19]);
    glEnd();

    glBegin(GL_TRIANGLE_STRIP);
	glVertex2fv(i_data[20]);
	glVertex2fv(i_data[21]);
	glVertex2fv(i_data[22]);
	glVertex2fv(i_data[23]);
    glEnd();

    glBegin(GL_LINE_STRIP);
	glVertex2fv(i_data[0]);
	glVertex2fv(i_data[2]);
	glVertex2fv(i_data[4]);
	glVertex2fv(i_data[6]);
	glVertex2fv(i_data[8]);
	glVertex2fv(i_data[10]);
	glVertex2fv(i_data[12]);
	glVertex2fv(i_data[14]);
	glVertex2fv(i_data[16]);
	glVertex2fv(i_data[18]);
	glVertex2fv(i_data[19]);
	glVertex2fv(i_data[17]);
	glVertex2fv(i_data[15]);
	glVertex2fv(i_data[13]);
	glVertex2fv(i_data[11]);
	glVertex2fv(i_data[9]);
	glVertex2fv(i_data[7]);
	glVertex2fv(i_data[5]);
	glVertex2fv(i_data[3]);
	glVertex2fv(i_data[1]);
    glEnd();

    glBegin(GL_LINE_STRIP);
	glVertex2fv(i_data[20]);
	glVertex2fv(i_data[22]);
	glVertex2fv(i_data[23]);
	glVertex2fv(i_data[21]);
    glEnd();

}

