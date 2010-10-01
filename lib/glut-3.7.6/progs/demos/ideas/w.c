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

float w_data[][2] = {
	{2.400000, 12.899384-3.0},
	{1.624615, 12.344969-3.0},
	{1.513846, 11.790554-3.0},
	{0.812308, 11.310061-3.0},
	{1.292308, 10.570842-3.0},
	{0.387692, 9.554415-3.0},
	{1.163077, 8.907598-3.0},
	{0.443077, 8.427105-3.0},
	{1.236923, 7.983573-3.0},
	{0.627692, 6.911705-3.0},
	{1.218462, 6.874743-3.0},
	{0.701538, 5.765914-3.0},
	{1.089231, 5.858316-3.0},
	{0.590769, 5.082136-3.0},

	{0.904615, 5.636550-3.0},
	{1.070769, 6.227926-3.0},
	{1.716923, 6.708419-3.0},
	{1.993846, 7.577002-3.0},
	{2.603077, 7.780287-3.0},
	{2.621538, 8.408625-3.0},
	{3.138462, 8.630390-3.0},
	{3.193846, 9.314168-3.0},
	{4.080000, 10.995893-3.0},
	{4.209231, 11.550308-3.0},

	{4.689231, 12.954825-3.0},
	{4.209231, 11.809035-3.0},
	{4.615385, 12.030801-3.0},
	{4.006154, 10.552361-3.0},
	{4.633846, 10.995893-3.0},
	{4.080000, 9.887064-3.0},
	{4.966154, 9.850102-3.0},
	{4.375385, 8.981520-3.0},
	{5.409231, 8.889117-3.0},
	{4.744616, 7.946612-3.0},
	{5.704616, 7.687885-3.0},
	{5.058462, 7.041068-3.0},
	{5.889231, 6.135524-3.0},
	{5.427692, 5.599589-3.0},
	{5.501538, 5.026694-3.0},

	{5.630769, 5.414784-3.0},
	{5.741539, 5.987679-3.0},
	{6.203077, 6.264887-3.0},
	{6.572308, 7.281314-3.0},
	{7.347692, 7.817248-3.0},
	{7.255384, 9.221766-3.0},
	{7.993846, 9.166325-3.0},
	{7.458462, 11.032854-3.0},
	{8.603077, 11.550308-3.0},
	{7.403077, 11.975359-3.0},
	{8.510769, 12.511293-3.0},
	{7.070769, 12.326488-3.0},
	{7.956923, 12.936345-3.0},
	{6.590769, 12.308008-3.0},
	{7.236923, 12.954825-3.0},
	{6.000000, 12.012321-3.0},
	{6.461538, 12.511293-3.0},

};

void draw_w(void) {

    glBegin(GL_TRIANGLE_STRIP);
	glVertex2fv(w_data[0]);
	glVertex2fv(w_data[1]);
	glVertex2fv(w_data[2]);
	glVertex2fv(w_data[3]);
	glVertex2fv(w_data[4]);
	glVertex2fv(w_data[5]);
	glVertex2fv(w_data[6]);
	glVertex2fv(w_data[7]);
	glVertex2fv(w_data[8]);
	glVertex2fv(w_data[9]);
	glVertex2fv(w_data[10]);
	glVertex2fv(w_data[11]);
	glVertex2fv(w_data[12]);
	glVertex2fv(w_data[13]);
    glEnd();

    glBegin(GL_TRIANGLE_STRIP);
	glVertex2fv(w_data[14]);
	glVertex2fv(w_data[15]);
	glVertex2fv(w_data[16]);
	glVertex2fv(w_data[17]);
	glVertex2fv(w_data[18]);
	glVertex2fv(w_data[19]);
	glVertex2fv(w_data[20]);
	glVertex2fv(w_data[21]);
	glVertex2fv(w_data[22]);
	glVertex2fv(w_data[23]);
    glEnd();

    glBegin(GL_TRIANGLE_STRIP);
	glVertex2fv(w_data[24]);
	glVertex2fv(w_data[25]);
	glVertex2fv(w_data[26]);
	glVertex2fv(w_data[27]);
	glVertex2fv(w_data[28]);
	glVertex2fv(w_data[29]);
	glVertex2fv(w_data[30]);
	glVertex2fv(w_data[31]);
	glVertex2fv(w_data[32]);
	glVertex2fv(w_data[33]);
	glVertex2fv(w_data[34]);
	glVertex2fv(w_data[35]);
	glVertex2fv(w_data[36]);
	glVertex2fv(w_data[37]);
	glVertex2fv(w_data[38]);
    glEnd();

    glBegin(GL_TRIANGLE_STRIP);
	glVertex2fv(w_data[39]);
	glVertex2fv(w_data[40]);
	glVertex2fv(w_data[41]);
	glVertex2fv(w_data[42]);
	glVertex2fv(w_data[43]);
	glVertex2fv(w_data[44]);
	glVertex2fv(w_data[45]);
	glVertex2fv(w_data[46]);
	glVertex2fv(w_data[47]);
	glVertex2fv(w_data[48]);
	glVertex2fv(w_data[49]);
	glVertex2fv(w_data[50]);
	glVertex2fv(w_data[51]);
	glVertex2fv(w_data[52]);
	glVertex2fv(w_data[53]);
	glVertex2fv(w_data[54]);
	glVertex2fv(w_data[55]);
    glEnd();

    glBegin(GL_LINE_STRIP);
	glVertex2fv(w_data[0]);
	glVertex2fv(w_data[2]);
	glVertex2fv(w_data[4]);
	glVertex2fv(w_data[6]);
	glVertex2fv(w_data[8]);
	glVertex2fv(w_data[10]);
	glVertex2fv(w_data[12]);
	glVertex2fv(w_data[13]);
	glVertex2fv(w_data[11]);
	glVertex2fv(w_data[9]);
	glVertex2fv(w_data[7]);
	glVertex2fv(w_data[5]);
	glVertex2fv(w_data[3]);
	glVertex2fv(w_data[1]);
	glVertex2fv(w_data[0]);
    glEnd();

    glBegin(GL_LINE_STRIP);
	glVertex2fv(w_data[14]);
	glVertex2fv(w_data[16]);
	glVertex2fv(w_data[18]);
	glVertex2fv(w_data[20]);
	glVertex2fv(w_data[22]);
	glVertex2fv(w_data[23]);
	glVertex2fv(w_data[21]);
	glVertex2fv(w_data[19]);
	glVertex2fv(w_data[17]);
	glVertex2fv(w_data[15]);
	glVertex2fv(w_data[14]);
    glEnd();

    glBegin(GL_LINE_STRIP);
	glVertex2fv(w_data[24]);
	glVertex2fv(w_data[26]);
	glVertex2fv(w_data[28]);
	glVertex2fv(w_data[30]);
	glVertex2fv(w_data[32]);
	glVertex2fv(w_data[34]);
	glVertex2fv(w_data[36]);
	glVertex2fv(w_data[38]);
	glVertex2fv(w_data[37]);
	glVertex2fv(w_data[35]);
	glVertex2fv(w_data[33]);
	glVertex2fv(w_data[31]);
	glVertex2fv(w_data[29]);
	glVertex2fv(w_data[27]);
	glVertex2fv(w_data[25]);
	glVertex2fv(w_data[24]);
    glEnd();

    glBegin(GL_LINE_STRIP);
	glVertex2fv(w_data[39]);
	glVertex2fv(w_data[41]);
	glVertex2fv(w_data[43]);
	glVertex2fv(w_data[45]);
	glVertex2fv(w_data[47]);
	glVertex2fv(w_data[49]);
	glVertex2fv(w_data[51]);
	glVertex2fv(w_data[53]);
	glVertex2fv(w_data[55]);
	glVertex2fv(w_data[54]);
	glVertex2fv(w_data[52]);
	glVertex2fv(w_data[50]);
	glVertex2fv(w_data[48]);
	glVertex2fv(w_data[46]);
	glVertex2fv(w_data[44]);
	glVertex2fv(w_data[42]);
	glVertex2fv(w_data[40]);
	glVertex2fv(w_data[39]);
    glEnd();

}

