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

float m_data[][2] = {
	{0.590769, 9.449335},
	{2.116923, 9.842375},
	{1.362051, 9.383828},
	{2.527179, 9.825998},
	{1.591795, 9.072672},
	{2.789744, 9.514841},
	{1.690256, 8.663255},
	{2.658462, 8.335722},
	{1.575385, 7.222108},
	{2.067692, 6.255886},
	{0.918974, 4.028659},
	{1.050256, 3.013306},
	{0.705641, 3.013306},

	{2.018461, 6.386899},
	{1.788718, 5.617196},
	{2.921026, 7.991812},
	{3.167180, 8.008188},
	{3.544615, 8.827022},
	{3.872821, 8.843398},
	{4.414359, 9.547595},
	{4.447179, 9.056294},
	{5.120000, 9.891504},
	{4.841026, 8.843398},
	{5.825641, 9.809621},
	{5.005128, 8.040941},
	{5.989744, 8.761515},
	{4.906667, 6.714432},
	{5.595897, 7.123848},
	{3.987692, 2.996929},
	{4.348718, 2.996929},

	{5.218462, 5.977482},
	{5.251282, 6.354146},
	{6.449231, 7.893552},
	{6.400000, 8.221085},
	{7.302564, 8.843398},
	{7.351795, 9.334698},
	{7.827693, 9.154554},
	{8.008205, 9.842375},
	{8.139487, 9.121801},
	{8.795897, 9.973388},
	{8.402051, 8.728762},
	{9.337436, 9.531218},
	{8.402051, 8.040941},
	{9.288205, 8.433982},
	{7.745641, 5.813715},
	{8.320000, 5.928352},
	{7.286154, 4.012282},
	{7.991795, 4.126919},
	{7.499487, 3.357216},
	{8.533334, 3.766633},
	{8.123077, 3.062436},
	{8.927179, 3.832139},
	{8.910769, 3.340839},
	{9.550769, 4.126919},

};

void draw_m(void) {

    glBegin(GL_LINE_STRIP);
	glVertex2fv(m_data[0]);
	glVertex2fv(m_data[2]);
	glVertex2fv(m_data[4]);
	glVertex2fv(m_data[6]);
	glVertex2fv(m_data[8]);
	glVertex2fv(m_data[10]);
	glVertex2fv(m_data[12]);
	glVertex2fv(m_data[11]);
	glVertex2fv(m_data[9]);
	glVertex2fv(m_data[7]);
	glVertex2fv(m_data[5]);
	glVertex2fv(m_data[3]);
	glVertex2fv(m_data[1]);
    glEnd();

    glBegin(GL_LINE_STRIP);
	glVertex2fv(m_data[14]);
	glVertex2fv(m_data[16]);
	glVertex2fv(m_data[18]);
	glVertex2fv(m_data[20]);
	glVertex2fv(m_data[22]);
	glVertex2fv(m_data[24]);
	glVertex2fv(m_data[26]);
	glVertex2fv(m_data[28]);
	glVertex2fv(m_data[29]);
	glVertex2fv(m_data[27]);
	glVertex2fv(m_data[25]);
	glVertex2fv(m_data[23]);
	glVertex2fv(m_data[21]);
	glVertex2fv(m_data[19]);
	glVertex2fv(m_data[17]);
	glVertex2fv(m_data[15]);
	glVertex2fv(m_data[13]);
    glEnd();

    glBegin(GL_LINE_STRIP);
	glVertex2fv(m_data[30]);
	glVertex2fv(m_data[32]);
	glVertex2fv(m_data[34]);
	glVertex2fv(m_data[36]);
	glVertex2fv(m_data[38]);
	glVertex2fv(m_data[40]);
	glVertex2fv(m_data[42]);
	glVertex2fv(m_data[44]);
	glVertex2fv(m_data[46]);
	glVertex2fv(m_data[48]);
	glVertex2fv(m_data[50]);
	glVertex2fv(m_data[52]);
	glVertex2fv(m_data[53]);
	glVertex2fv(m_data[51]);
	glVertex2fv(m_data[49]);
	glVertex2fv(m_data[47]);
	glVertex2fv(m_data[45]);
	glVertex2fv(m_data[43]);
	glVertex2fv(m_data[41]);
	glVertex2fv(m_data[39]);
	glVertex2fv(m_data[37]);
	glVertex2fv(m_data[35]);
	glVertex2fv(m_data[33]);
	glVertex2fv(m_data[31]);
    glEnd();

    glBegin(GL_TRIANGLE_STRIP);
	glVertex2fv(m_data[0]);
	glVertex2fv(m_data[1]);
	glVertex2fv(m_data[2]);
	glVertex2fv(m_data[3]);
	glVertex2fv(m_data[4]);
	glVertex2fv(m_data[5]);
	glVertex2fv(m_data[6]);
	glVertex2fv(m_data[7]);
	glVertex2fv(m_data[8]);
	glVertex2fv(m_data[9]);
	glVertex2fv(m_data[10]);
	glVertex2fv(m_data[11]);
	glVertex2fv(m_data[12]);
    glEnd();

    glBegin(GL_TRIANGLE_STRIP);
	glVertex2fv(m_data[13]);
	glVertex2fv(m_data[14]);
	glVertex2fv(m_data[15]);
	glVertex2fv(m_data[16]);
	glVertex2fv(m_data[17]);
	glVertex2fv(m_data[18]);
	glVertex2fv(m_data[19]);
	glVertex2fv(m_data[20]);
	glVertex2fv(m_data[21]);
	glVertex2fv(m_data[22]);
	glVertex2fv(m_data[23]);
	glVertex2fv(m_data[24]);
	glVertex2fv(m_data[25]);
	glVertex2fv(m_data[26]);
	glVertex2fv(m_data[27]);
	glVertex2fv(m_data[28]);
	glVertex2fv(m_data[29]);
    glEnd();

    glBegin(GL_TRIANGLE_STRIP);
	glVertex2fv(m_data[30]);
	glVertex2fv(m_data[31]);
	glVertex2fv(m_data[32]);
	glVertex2fv(m_data[33]);
	glVertex2fv(m_data[34]);
	glVertex2fv(m_data[35]);
	glVertex2fv(m_data[36]);
	glVertex2fv(m_data[37]);
	glVertex2fv(m_data[38]);
	glVertex2fv(m_data[39]);
	glVertex2fv(m_data[40]);
	glVertex2fv(m_data[41]);
	glVertex2fv(m_data[42]);
	glVertex2fv(m_data[43]);
	glVertex2fv(m_data[44]);
	glVertex2fv(m_data[45]);
	glVertex2fv(m_data[46]);
	glVertex2fv(m_data[47]);
	glVertex2fv(m_data[48]);
	glVertex2fv(m_data[49]);
	glVertex2fv(m_data[50]);
	glVertex2fv(m_data[51]);
	glVertex2fv(m_data[52]);
	glVertex2fv(m_data[53]);
    glEnd();

}

