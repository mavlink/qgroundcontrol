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

float d_data[][2] = {
	{4.714579, 9.987679},
	{2.841889, 9.429158},
	{2.825462, 9.166325},
	{1.856263, 8.722793},
	{2.004107, 8.000000},
	{0.969199, 7.605750},
	{1.494866, 6.636550},
	{0.607803, 6.028748},
	{1.527721, 4.960986},
	{0.772074, 4.254620},
	{1.774127, 4.139630},
	{1.445585, 3.186858},
	{2.266940, 3.843942},
	{2.250513, 3.022587},
	{2.776181, 3.843942},
	{3.137577, 3.383984},
	{3.351129, 4.008214},
	{3.909651, 4.451746},
	{4.090349, 4.960986},
	{4.862423, 5.946612},
	{4.763860, 6.652977},
	{5.388090, 7.572895},
	{4.862423, 8.492813},
	{5.618070, 9.921971},
	{4.698152, 10.940452},
	{5.338809, 12.303902},
	{4.238193, 12.960985},
	{4.451746, 14.554415},
	{3.581109, 14.291581},
	{3.613963, 15.342916},
	{2.677618, 15.145790},
	{2.480493, 15.540041},
	{2.036961, 15.211499},
	{1.281314, 15.112936},

};

void draw_d(void) {

    glBegin(GL_TRIANGLE_STRIP);
	glVertex2fv(d_data[0]);
	glVertex2fv(d_data[1]);
	glVertex2fv(d_data[2]);
	glVertex2fv(d_data[3]);
	glVertex2fv(d_data[4]);
	glVertex2fv(d_data[5]);
	glVertex2fv(d_data[6]);
	glVertex2fv(d_data[7]);
	glVertex2fv(d_data[8]);
	glVertex2fv(d_data[9]);
	glVertex2fv(d_data[10]);
	glVertex2fv(d_data[11]);
	glVertex2fv(d_data[12]);
	glVertex2fv(d_data[13]);
	glVertex2fv(d_data[14]);
	glVertex2fv(d_data[15]);
	glVertex2fv(d_data[16]);
	glVertex2fv(d_data[17]);
	glVertex2fv(d_data[18]);
	glVertex2fv(d_data[19]);
	glVertex2fv(d_data[20]);
	glVertex2fv(d_data[21]);
	glVertex2fv(d_data[22]);
	glVertex2fv(d_data[23]);
	glVertex2fv(d_data[24]);
	glVertex2fv(d_data[25]);
	glVertex2fv(d_data[26]);
	glVertex2fv(d_data[27]);
	glVertex2fv(d_data[28]);
	glVertex2fv(d_data[29]);
	glVertex2fv(d_data[30]);
	glVertex2fv(d_data[31]);
	glVertex2fv(d_data[32]);
	glVertex2fv(d_data[33]);
    glEnd();

    glBegin(GL_LINE_STRIP);
	glVertex2fv(d_data[0]);
	glVertex2fv(d_data[2]);
	glVertex2fv(d_data[4]);
	glVertex2fv(d_data[6]);
	glVertex2fv(d_data[8]);
	glVertex2fv(d_data[10]);
	glVertex2fv(d_data[12]);
	glVertex2fv(d_data[14]);
	glVertex2fv(d_data[16]);
	glVertex2fv(d_data[18]);
	glVertex2fv(d_data[20]);
	glVertex2fv(d_data[22]);
	glVertex2fv(d_data[24]);
	glVertex2fv(d_data[26]);
	glVertex2fv(d_data[28]);
	glVertex2fv(d_data[30]);
	glVertex2fv(d_data[32]);
	glVertex2fv(d_data[33]);
	glVertex2fv(d_data[31]);
	glVertex2fv(d_data[29]);
	glVertex2fv(d_data[27]);
	glVertex2fv(d_data[25]);
	glVertex2fv(d_data[23]);
	glVertex2fv(d_data[21]);
	glVertex2fv(d_data[19]);
	glVertex2fv(d_data[17]);
	glVertex2fv(d_data[15]);
	glVertex2fv(d_data[13]);
	glVertex2fv(d_data[11]);
	glVertex2fv(d_data[9]);
	glVertex2fv(d_data[7]);
	glVertex2fv(d_data[5]);
	glVertex2fv(d_data[3]);
	glVertex2fv(d_data[1]);
    glEnd();

}

