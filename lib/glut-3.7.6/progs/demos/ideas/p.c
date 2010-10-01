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

float p_data[][2] = {
	{1.987500, 11.437500-3.0},
	{2.606250, 11.662500-3.0},
	{2.887500, 12.150000-3.0},
	{3.037500, 11.943750-3.0},
	{3.618750, 12.956250-3.0},
	{2.793750, 10.256250-3.0},
	{3.037500, 8.775000-3.0},
	{2.418750, 8.006250-3.0},
	{2.381250, 3.656250-3.0},
	{1.518750, 2.756250-3.0},
	{1.856250, 1.687500-3.0},
	{0.975000, 1.087500-3.0},
	{1.087500, 0.393750-3.0},
	{0.600000, 0.431250-3.0},
	{0.018750, 0.037500-3.0},

	{3.093750, 9.787500-3.0},
	{3.037500, 9.412500-3.0},
	{4.050000, 11.231250-3.0},
	{4.331250, 11.175000-3.0},
	{5.100000, 12.187500-3.0},
	{5.137500, 11.906250-3.0},
	{5.831250, 12.712500-3.0},
	{5.643750, 12.000000-3.0},
	{6.656250, 12.731250-3.0},
	{5.962500, 11.831250-3.0},
	{6.956250, 12.393750-3.0},
	{6.112500, 11.512500-3.0},
	{7.012500, 11.512500-3.0},
	{6.093750, 10.575000-3.0},
	{6.787500, 9.993750-3.0},
	{5.868750, 9.412500-3.0},
	{6.018750, 7.950000-3.0},
	{5.193750, 7.256250-3.0},
	{5.043750, 6.318750-3.0},
	{4.068750, 5.775000-3.0},
	{3.881250, 5.418750-3.0},
	{3.337500, 5.606250-3.0},
	{3.093750, 5.193750-3.0},
	{2.868750, 5.793750-3.0},
	{2.493750, 5.512500-3.0},
	{2.643750, 6.675000-3.0},

};

void draw_p(void) {

    glBegin(GL_TRIANGLE_STRIP);
	glVertex2fv(p_data[0]);
	glVertex2fv(p_data[1]);
	glVertex2fv(p_data[2]);
	glVertex2fv(p_data[3]);
	glVertex2fv(p_data[4]);
	glVertex2fv(p_data[5]);
	glVertex2fv(p_data[6]);
	glVertex2fv(p_data[7]);
	glVertex2fv(p_data[8]);
	glVertex2fv(p_data[9]);
	glVertex2fv(p_data[10]);
	glVertex2fv(p_data[11]);
	glVertex2fv(p_data[12]);
	glVertex2fv(p_data[13]);
	glVertex2fv(p_data[14]);
    glEnd();

    glBegin(GL_TRIANGLE_STRIP);
	glVertex2fv(p_data[15]);
	glVertex2fv(p_data[16]);
	glVertex2fv(p_data[17]);
	glVertex2fv(p_data[18]);
	glVertex2fv(p_data[19]);
	glVertex2fv(p_data[20]);
	glVertex2fv(p_data[21]);
	glVertex2fv(p_data[22]);
	glVertex2fv(p_data[23]);
	glVertex2fv(p_data[24]);
	glVertex2fv(p_data[25]);
	glVertex2fv(p_data[26]);
	glVertex2fv(p_data[27]);
	glVertex2fv(p_data[28]);
	glVertex2fv(p_data[29]);
	glVertex2fv(p_data[30]);
	glVertex2fv(p_data[31]);
	glVertex2fv(p_data[32]);
	glVertex2fv(p_data[33]);
	glVertex2fv(p_data[34]);
	glVertex2fv(p_data[35]);
	glVertex2fv(p_data[36]);
	glVertex2fv(p_data[37]);
	glVertex2fv(p_data[38]);
	glVertex2fv(p_data[39]);
	glVertex2fv(p_data[40]);
    glEnd();

    glBegin(GL_LINE_STRIP);
	glVertex2fv(p_data[0]);
	glVertex2fv(p_data[2]);
	glVertex2fv(p_data[4]);
	glVertex2fv(p_data[6]);
	glVertex2fv(p_data[8]);
	glVertex2fv(p_data[10]);
	glVertex2fv(p_data[12]);
	glVertex2fv(p_data[14]);
	glVertex2fv(p_data[13]);
	glVertex2fv(p_data[11]);
	glVertex2fv(p_data[9]);
	glVertex2fv(p_data[7]);
	glVertex2fv(p_data[5]);
	glVertex2fv(p_data[3]);
	glVertex2fv(p_data[1]);
	glVertex2fv(p_data[0]);
    glEnd();

    glBegin(GL_LINE_STRIP);
	glVertex2fv(p_data[15]);
	glVertex2fv(p_data[17]);
	glVertex2fv(p_data[19]);
	glVertex2fv(p_data[21]);
	glVertex2fv(p_data[23]);
	glVertex2fv(p_data[25]);
	glVertex2fv(p_data[27]);
	glVertex2fv(p_data[29]);
	glVertex2fv(p_data[31]);
	glVertex2fv(p_data[33]);
	glVertex2fv(p_data[35]);
	glVertex2fv(p_data[37]);
	glVertex2fv(p_data[39]);
	glVertex2fv(p_data[40]);
	glVertex2fv(p_data[38]);
	glVertex2fv(p_data[36]);
	glVertex2fv(p_data[34]);
	glVertex2fv(p_data[32]);
	glVertex2fv(p_data[30]);
	glVertex2fv(p_data[28]);
	glVertex2fv(p_data[26]);
	glVertex2fv(p_data[24]);
	glVertex2fv(p_data[22]);
	glVertex2fv(p_data[20]);
	glVertex2fv(p_data[18]);
	glVertex2fv(p_data[16]);
	glVertex2fv(p_data[15]);
    glEnd();

}

