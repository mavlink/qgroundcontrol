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

float n_data[][2] = {
	{1.009307, 9.444788},
	{2.548087, 9.742002},
	{1.737332, 9.213622},
	{2.994829, 9.659443},
	{1.985522, 8.751290},
	{3.127198, 9.180598},
	{1.935884, 7.975232},
	{2.481903, 6.571723},
	{1.472596, 5.019608},
	{1.439504, 2.988648},
	{1.025853, 2.988648},

	{2.283350, 6.059855},
	{2.035160, 5.366357},
	{3.292658, 7.711042},
	{3.540848, 7.744066},
	{4.384695, 9.031992},
	{4.699069, 8.916409},
	{5.609100, 9.808049},
	{5.145812, 8.982456},
	{6.155119, 9.791537},
	{5.410548, 8.635707},
	{6.337125, 9.312694},
	{5.360910, 7.991744},
	{6.088935, 8.090816},
	{4.947259, 5.977296},
	{5.261634, 4.804954},
	{4.616339, 4.028896},
	{5.211996, 3.962848},
	{4.732162, 3.318886},
	{5.559462, 3.814241},
	{5.228542, 3.038184},
	{5.940021, 3.814241},
	{5.906929, 3.335397},
	{6.684591, 4.094943},

};

void draw_n(void) {

    glBegin(GL_TRIANGLE_STRIP);
	glVertex2fv(n_data[0]);
	glVertex2fv(n_data[1]);
	glVertex2fv(n_data[2]);
	glVertex2fv(n_data[3]);
	glVertex2fv(n_data[4]);
	glVertex2fv(n_data[5]);
	glVertex2fv(n_data[6]);
	glVertex2fv(n_data[7]);
	glVertex2fv(n_data[8]);
	glVertex2fv(n_data[9]);
	glVertex2fv(n_data[10]);
    glEnd();

    glBegin(GL_TRIANGLE_STRIP);
	glVertex2fv(n_data[11]);
	glVertex2fv(n_data[12]);
	glVertex2fv(n_data[13]);
	glVertex2fv(n_data[14]);
	glVertex2fv(n_data[15]);
	glVertex2fv(n_data[16]);
	glVertex2fv(n_data[17]);
	glVertex2fv(n_data[18]);
	glVertex2fv(n_data[19]);
	glVertex2fv(n_data[20]);
	glVertex2fv(n_data[21]);
	glVertex2fv(n_data[22]);
	glVertex2fv(n_data[23]);
	glVertex2fv(n_data[24]);
	glVertex2fv(n_data[25]);
	glVertex2fv(n_data[26]);
	glVertex2fv(n_data[27]);
	glVertex2fv(n_data[28]);
	glVertex2fv(n_data[29]);
	glVertex2fv(n_data[30]);
	glVertex2fv(n_data[31]);
	glVertex2fv(n_data[32]);
	glVertex2fv(n_data[33]);
    glEnd();

    glBegin(GL_LINE_STRIP);
	glVertex2fv(n_data[0]);
	glVertex2fv(n_data[2]);
	glVertex2fv(n_data[4]);
	glVertex2fv(n_data[6]);
	glVertex2fv(n_data[8]);
	glVertex2fv(n_data[10]);
	glVertex2fv(n_data[9]);
	glVertex2fv(n_data[7]);
	glVertex2fv(n_data[5]);
	glVertex2fv(n_data[3]);
	glVertex2fv(n_data[1]);
    glEnd();

    glBegin(GL_LINE_STRIP);
	glVertex2fv(n_data[12]);
	glVertex2fv(n_data[14]);
	glVertex2fv(n_data[16]);
	glVertex2fv(n_data[18]);
	glVertex2fv(n_data[20]);
	glVertex2fv(n_data[22]);
	glVertex2fv(n_data[24]);
	glVertex2fv(n_data[26]);
	glVertex2fv(n_data[28]);
	glVertex2fv(n_data[30]);
	glVertex2fv(n_data[32]);
	glVertex2fv(n_data[33]);
	glVertex2fv(n_data[31]);
	glVertex2fv(n_data[29]);
	glVertex2fv(n_data[27]);
	glVertex2fv(n_data[25]);
	glVertex2fv(n_data[23]);
	glVertex2fv(n_data[21]);
	glVertex2fv(n_data[19]);
	glVertex2fv(n_data[17]);
	glVertex2fv(n_data[15]);
	glVertex2fv(n_data[13]);
	glVertex2fv(n_data[11]);
    glEnd();

}

