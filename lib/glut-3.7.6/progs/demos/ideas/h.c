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

float h_data[][2] = {
	{1.462963, 17.499485-3.0},
	{3.259259, 17.851700-3.0},
	{2.074074, 17.351185-3.0},
	{3.037037, 17.073120-3.0},
	{2.277778, 17.054583-3.0},
	{2.814815, 15.775489-3.0},
	{2.018518, 14.737384-3.0},
	{2.203704, 12.142121-3.0},
	{1.296296, 10.158600-3.0},
	{1.259259, 7.674562-3.0},
	{0.666667, 6.933059-3.0},
	{0.685185, 5.728116-3.0},
	{0.314815, 5.653965-3.0},
	{0.444444, 5.227601-3.0},
	{0.074074, 5.097837-3.0},

	{1.611111, 9.824923-3.0},
	{1.333333, 8.527291-3.0},
	{2.240741, 10.937179-3.0},
	{2.500000, 10.992791-3.0},
	{2.888889, 11.771370-3.0},
	{3.314815, 11.845520-3.0},
	{3.462963, 12.253347-3.0},
	{3.740741, 12.067971-3.0},
	{4.500000, 12.846550-3.0},
	{4.148148, 12.030896-3.0},
	{5.185185, 12.883625-3.0},
	{4.296296, 11.771370-3.0},
	{5.351852, 12.420185-3.0},
	{4.333333, 11.196705-3.0},
	{5.129630, 10.955716-3.0},
	{4.129630, 9.583934-3.0},
	{4.203704, 7.192585-3.0},
	{3.518518, 6.414006-3.0},
	{4.129630, 6.469619-3.0},
	{3.537037, 5.765191-3.0},
	{4.296296, 6.061792-3.0},
	{3.851852, 5.171988-3.0},
	{4.722222, 5.802266-3.0},
	{4.277778, 5.060762-3.0},
	{5.314815, 5.894954-3.0},
	{5.148148, 5.431514-3.0},
	{5.777778, 6.098867-3.0},

};

void draw_h(void) {

    glBegin(GL_TRIANGLE_STRIP);
	glVertex2fv(h_data[0]);
	glVertex2fv(h_data[1]);
	glVertex2fv(h_data[2]);
	glVertex2fv(h_data[3]);
	glVertex2fv(h_data[4]);
	glVertex2fv(h_data[5]);
	glVertex2fv(h_data[6]);
	glVertex2fv(h_data[7]);
	glVertex2fv(h_data[8]);
	glVertex2fv(h_data[9]);
	glVertex2fv(h_data[10]);
	glVertex2fv(h_data[11]);
	glVertex2fv(h_data[12]);
	glVertex2fv(h_data[13]);
	glVertex2fv(h_data[14]);
    glEnd();

    glBegin(GL_TRIANGLE_STRIP);
	glVertex2fv(h_data[15]);
	glVertex2fv(h_data[16]);
	glVertex2fv(h_data[17]);
	glVertex2fv(h_data[18]);
	glVertex2fv(h_data[19]);
	glVertex2fv(h_data[20]);
	glVertex2fv(h_data[21]);
	glVertex2fv(h_data[22]);
	glVertex2fv(h_data[23]);
	glVertex2fv(h_data[24]);
	glVertex2fv(h_data[25]);
	glVertex2fv(h_data[26]);
	glVertex2fv(h_data[27]);
	glVertex2fv(h_data[28]);
	glVertex2fv(h_data[29]);
	glVertex2fv(h_data[30]);
	glVertex2fv(h_data[31]);
	glVertex2fv(h_data[32]);
	glVertex2fv(h_data[33]);
	glVertex2fv(h_data[34]);
	glVertex2fv(h_data[35]);
	glVertex2fv(h_data[36]);
	glVertex2fv(h_data[37]);
	glVertex2fv(h_data[38]);
	glVertex2fv(h_data[39]);
	glVertex2fv(h_data[40]);
	glVertex2fv(h_data[41]);
    glEnd();

    glBegin(GL_LINE_STRIP);
	glVertex2fv(h_data[0]);
	glVertex2fv(h_data[2]);
	glVertex2fv(h_data[4]);
	glVertex2fv(h_data[6]);
	glVertex2fv(h_data[8]);
	glVertex2fv(h_data[10]);
	glVertex2fv(h_data[12]);
	glVertex2fv(h_data[14]);
	glVertex2fv(h_data[13]);
	glVertex2fv(h_data[11]);
	glVertex2fv(h_data[9]);
	glVertex2fv(h_data[7]);
	glVertex2fv(h_data[5]);
	glVertex2fv(h_data[3]);
	glVertex2fv(h_data[1]);
	glVertex2fv(h_data[0]);
    glEnd();

    glBegin(GL_LINE_STRIP);
	glVertex2fv(h_data[15]);
	glVertex2fv(h_data[17]);
	glVertex2fv(h_data[19]);
	glVertex2fv(h_data[21]);
	glVertex2fv(h_data[23]);
	glVertex2fv(h_data[25]);
	glVertex2fv(h_data[27]);
	glVertex2fv(h_data[29]);
	glVertex2fv(h_data[31]);
	glVertex2fv(h_data[33]);
	glVertex2fv(h_data[35]);
	glVertex2fv(h_data[37]);
	glVertex2fv(h_data[39]);
	glVertex2fv(h_data[41]);
	glVertex2fv(h_data[40]);
	glVertex2fv(h_data[38]);
	glVertex2fv(h_data[36]);
	glVertex2fv(h_data[34]);
	glVertex2fv(h_data[32]);
	glVertex2fv(h_data[30]);
	glVertex2fv(h_data[28]);
	glVertex2fv(h_data[26]);
	glVertex2fv(h_data[24]);
	glVertex2fv(h_data[22]);
	glVertex2fv(h_data[20]);
	glVertex2fv(h_data[18]);
	glVertex2fv(h_data[16]);
	glVertex2fv(h_data[15]);
    glEnd();

}

