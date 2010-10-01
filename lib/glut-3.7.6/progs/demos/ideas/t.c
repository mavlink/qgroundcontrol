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

float t_data[][2] = {
	{2.986667, 14.034801},
	{2.445128, 10.088024},
	{1.788718, 9.236438},
	{2.264615, 7.664279},
	{1.165128, 5.666326},
	{2.034872, 4.945752},
	{1.132308, 3.766633},
	{2.182564, 3.570113},
	{1.411282, 2.309109},
	{2.510769, 2.341863},
	{2.149744, 1.048106},
	{3.364103, 1.375640},
	{3.167180, 0.327533},
	{4.381538, 0.736950},
	{5.005128, 0.032753},
	{5.612308, 0.638690},
	{6.235898, 0.540430},
	{7.187692, 1.162743},

	{1.985641, 9.039918},
	{2.133333, 10.186285},
	{1.509744, 9.023541},
	{1.608205, 9.662231},
	{1.050256, 9.023541},
	{1.050256, 9.334698},
	{0.196923, 9.007165},

	{2.363077, 9.711361},
	{2.264615, 9.023541},
	{3.282051, 9.563972},
	{3.446154, 9.023541},
	{4.069744, 9.531218},
	{4.299487, 9.236438},
	{4.644103, 9.613101},
	{5.251282, 9.875128},

};

void draw_t(void) {

    glBegin(GL_TRIANGLE_STRIP);
	glVertex2fv(t_data[0]);
	glVertex2fv(t_data[1]);
	glVertex2fv(t_data[2]);
	glVertex2fv(t_data[3]);
	glVertex2fv(t_data[4]);
	glVertex2fv(t_data[5]);
	glVertex2fv(t_data[6]);
	glVertex2fv(t_data[7]);
	glVertex2fv(t_data[8]);
	glVertex2fv(t_data[9]);
	glVertex2fv(t_data[10]);
	glVertex2fv(t_data[11]);
	glVertex2fv(t_data[12]);
	glVertex2fv(t_data[13]);
	glVertex2fv(t_data[14]);
	glVertex2fv(t_data[15]);
	glVertex2fv(t_data[16]);
	glVertex2fv(t_data[17]);
    glEnd();

    glBegin(GL_TRIANGLE_STRIP);
	glVertex2fv(t_data[18]);
	glVertex2fv(t_data[19]);
	glVertex2fv(t_data[20]);
	glVertex2fv(t_data[21]);
	glVertex2fv(t_data[22]);
	glVertex2fv(t_data[23]);
	glVertex2fv(t_data[24]);
    glEnd();

    glBegin(GL_TRIANGLE_STRIP);
	glVertex2fv(t_data[25]);
	glVertex2fv(t_data[26]);
	glVertex2fv(t_data[27]);
	glVertex2fv(t_data[28]);
	glVertex2fv(t_data[29]);
	glVertex2fv(t_data[30]);
	glVertex2fv(t_data[31]);
	glVertex2fv(t_data[32]);
    glEnd();

    glBegin(GL_LINE_STRIP);
	glVertex2fv(t_data[0]);
	glVertex2fv(t_data[2]);
	glVertex2fv(t_data[4]);
	glVertex2fv(t_data[6]);
	glVertex2fv(t_data[8]);
	glVertex2fv(t_data[10]);
	glVertex2fv(t_data[12]);
	glVertex2fv(t_data[14]);
	glVertex2fv(t_data[16]);
	glVertex2fv(t_data[17]);
	glVertex2fv(t_data[15]);
	glVertex2fv(t_data[13]);
	glVertex2fv(t_data[11]);
	glVertex2fv(t_data[9]);
	glVertex2fv(t_data[7]);
	glVertex2fv(t_data[5]);
	glVertex2fv(t_data[3]);
	glVertex2fv(t_data[1]);
    glEnd();

    glBegin(GL_LINE_STRIP);
	glVertex2fv(t_data[18]);
	glVertex2fv(t_data[20]);
	glVertex2fv(t_data[22]);
	glVertex2fv(t_data[24]);
	glVertex2fv(t_data[23]);
	glVertex2fv(t_data[21]);
	glVertex2fv(t_data[19]);
    glEnd();

    glBegin(GL_LINE_STRIP);
	glVertex2fv(t_data[26]);
	glVertex2fv(t_data[28]);
	glVertex2fv(t_data[30]);
	glVertex2fv(t_data[32]);
	glVertex2fv(t_data[31]);
	glVertex2fv(t_data[29]);
	glVertex2fv(t_data[27]);
	glVertex2fv(t_data[25]);
    glEnd();

}

