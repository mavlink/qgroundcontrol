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

float f_data[][2] = {
	{0.157570-3.0, 0.105155-2.0},
	{1.820803-3.0, 0.736082-2.0},
	{2.030896-3.0, 0.525773-2.0},
	{2.731205-3.0, 1.507216-2.0},
	{2.906282-3.0, 1.139175-2.0},
	{3.378991-3.0, 2.611340-2.0},
	{4.009269-3.0, 3.014433-2.0},
	{4.429454-3.0, 5.730928-2.0},
	{5.042224-3.0, 5.783505-2.0},
	{5.269825-3.0, 10.252578-2.0},
	{6.092688-3.0, 10.708247-2.0},
	{5.917611-3.0, 12.758763-2.0},
	{6.915551-3.0, 13.635052-2.0},
	{6.565396-3.0, 14.388659-2.0},
	{7.370752-3.0, 14.686598-2.0},
	{7.003089-3.0, 15.159794-2.0},
	{7.720906-3.0, 15.300000-2.0},
	{7.633368-3.0, 15.668041-2.0},
	{8.403708-3.0, 15.930928-2.0},
	{9.401648-3.0, 16.596907-2.0},
	{9.261586-3.0, 16.211340-2.0},
	{9.874356-3.0, 16.719587-2.0},
	{10.136972-3.0, 16.228867-2.0},
	{10.469619-3.0, 16.789690-2.0},
	{10.854789-3.0, 16.228867-2.0},
	{11.064881-3.0, 16.667011-2.0},
	{11.169928-3.0, 16.369072-2.0},

	{3.956746-3.0, 10.988660-2.0},
	{5.147271-3.0, 11.479382-2.0},
	{5.654995-3.0, 11.006186-2.0},
	{5.812564-3.0, 11.970103-2.0},
	{6.127703-3.0, 11.023711-2.0},
	{6.495366-3.0, 11.461856-2.0},
	{7.230690-3.0, 11.006186-2.0},
	{7.318229-3.0, 11.321650-2.0},
	{7.983522-3.0, 11.198969-2.0},
	{8.106076-3.0, 11.426805-2.0},
	{8.613800-3.0, 11.584537-2.0},

};

void draw_f(void) {

    glBegin(GL_TRIANGLE_STRIP);
	glVertex2fv(f_data[0]);
	glVertex2fv(f_data[1]);
	glVertex2fv(f_data[2]);
	glVertex2fv(f_data[3]);
	glVertex2fv(f_data[4]);
	glVertex2fv(f_data[5]);
	glVertex2fv(f_data[6]);
	glVertex2fv(f_data[7]);
	glVertex2fv(f_data[8]);
	glVertex2fv(f_data[9]);
	glVertex2fv(f_data[10]);
	glVertex2fv(f_data[11]);
	glVertex2fv(f_data[12]);
	glVertex2fv(f_data[13]);
	glVertex2fv(f_data[14]);
	glVertex2fv(f_data[15]);
	glVertex2fv(f_data[16]);
	glVertex2fv(f_data[17]);
	glVertex2fv(f_data[18]);
	glVertex2fv(f_data[19]);
	glVertex2fv(f_data[20]);
	glVertex2fv(f_data[21]);
	glVertex2fv(f_data[22]);
	glVertex2fv(f_data[23]);
	glVertex2fv(f_data[24]);
	glVertex2fv(f_data[25]);
	glVertex2fv(f_data[26]);
    glEnd();

    glBegin(GL_TRIANGLE_STRIP);
	glVertex2fv(f_data[27]);
	glVertex2fv(f_data[28]);
	glVertex2fv(f_data[29]);
	glVertex2fv(f_data[30]);
	glVertex2fv(f_data[31]);
	glVertex2fv(f_data[32]);
	glVertex2fv(f_data[33]);
	glVertex2fv(f_data[34]);
	glVertex2fv(f_data[35]);
	glVertex2fv(f_data[36]);
	glVertex2fv(f_data[37]);
    glEnd();

    glBegin(GL_LINE_STRIP);
	glVertex2fv(f_data[0]);
	glVertex2fv(f_data[2]);
	glVertex2fv(f_data[4]);
	glVertex2fv(f_data[6]);
	glVertex2fv(f_data[8]);
	glVertex2fv(f_data[10]);
	glVertex2fv(f_data[12]);
	glVertex2fv(f_data[14]);
	glVertex2fv(f_data[16]);
	glVertex2fv(f_data[18]);
	glVertex2fv(f_data[20]);
	glVertex2fv(f_data[22]);
	glVertex2fv(f_data[24]);
	glVertex2fv(f_data[26]);
	glVertex2fv(f_data[25]);
	glVertex2fv(f_data[23]);
	glVertex2fv(f_data[21]);
	glVertex2fv(f_data[19]);
	glVertex2fv(f_data[17]);
	glVertex2fv(f_data[15]);
	glVertex2fv(f_data[13]);
	glVertex2fv(f_data[11]);
	glVertex2fv(f_data[9]);
	glVertex2fv(f_data[7]);
	glVertex2fv(f_data[5]);
	glVertex2fv(f_data[3]);
	glVertex2fv(f_data[1]);
	glVertex2fv(f_data[0]);
    glEnd();

    glBegin(GL_LINE_STRIP);
	glVertex2fv(f_data[27]);
	glVertex2fv(f_data[29]);
	glVertex2fv(f_data[31]);
	glVertex2fv(f_data[33]);
	glVertex2fv(f_data[35]);
	glVertex2fv(f_data[37]);
	glVertex2fv(f_data[36]);
	glVertex2fv(f_data[34]);
	glVertex2fv(f_data[32]);
	glVertex2fv(f_data[30]);
	glVertex2fv(f_data[28]);
	glVertex2fv(f_data[27]);
    glEnd();

}

