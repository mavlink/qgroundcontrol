
/* Copyright (c) Mark J. Kilgard, 1994. */

/**
 * (c) Copyright 1993, 1994, Silicon Graphics, Inc.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GL/glut.h>
#include "atlantis.h"

fishRec sharks[NUM_SHARKS];
fishRec momWhale;
fishRec babyWhale;
fishRec dolph;

GLboolean moving;

void
InitFishs(void)
{
    int i;

    for (i = 0; i < NUM_SHARKS; i++) {
        sharks[i].x = 70000.0 + rand() % 6000;
        sharks[i].y = rand() % 6000;
        sharks[i].z = rand() % 6000;
        sharks[i].psi = rand() % 360 - 180.0;
        sharks[i].v = 1.0;
    }

    dolph.x = 30000.0;
    dolph.y = 0.0;
    dolph.z = 6000.0;
    dolph.psi = 90.0;
    dolph.theta = 0.0;
    dolph.v = 3.0;

    momWhale.x = 70000.0;
    momWhale.y = 0.0;
    momWhale.z = 0.0;
    momWhale.psi = 90.0;
    momWhale.theta = 0.0;
    momWhale.v = 3.0;

    babyWhale.x = 60000.0;
    babyWhale.y = -2000.0;
    babyWhale.z = -2000.0;
    babyWhale.psi = 90.0;
    babyWhale.theta = 0.0;
    babyWhale.v = 3.0;
}

void
Init(void)
{
    static float ambient[] =
    {0.1, 0.1, 0.1, 1.0};
    static float diffuse[] =
    {1.0, 1.0, 1.0, 1.0};
    static float position[] =
    {0.0, 1.0, 0.0, 0.0};
    static float mat_shininess[] =
    {90.0};
    static float mat_specular[] =
    {0.8, 0.8, 0.8, 1.0};
    static float mat_diffuse[] =
    {0.46, 0.66, 0.795, 1.0};
    static float mat_ambient[] =
    {0.0, 0.1, 0.2, 1.0};
    static float lmodel_ambient[] =
    {0.4, 0.4, 0.4, 1.0};
    static float lmodel_localviewer[] =
    {0.0};

    glFrontFace(GL_CW);

    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, position);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
    glLightModelfv(GL_LIGHT_MODEL_LOCAL_VIEWER, lmodel_localviewer);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient);

    InitFishs();

    glClearColor(0.0, 0.5, 0.9, 0.0);
}

void
Reshape(int width, int height)
{
    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(400.0, 2.0, 10000.0, 400000.0);
    glMatrixMode(GL_MODELVIEW);
}

void
Animate(void)
{
    int i;

    for (i = 0; i < NUM_SHARKS; i++) {
        SharkPilot(&sharks[i]);
        SharkMiss(i);
    }
    WhalePilot(&dolph);
    dolph.phi++;
    glutPostRedisplay();
    WhalePilot(&momWhale);
    momWhale.phi++;
    WhalePilot(&babyWhale);
    babyWhale.phi++;
}

/* ARGSUSED1 */
void
Key(unsigned char key, int x, int y)
{
    switch (key) {
    case 27:           /* Esc will quit */
        exit(1);
        break;
    case ' ':          /* space will advance frame */
        if (!moving) {
            Animate();
        }
    }
}

void
Display(void)
{
    int i;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (i = 0; i < NUM_SHARKS; i++) {
        glPushMatrix();
        FishTransform(&sharks[i]);
        DrawShark(&sharks[i]);
        glPopMatrix();
    }

    glPushMatrix();
    FishTransform(&dolph);
    DrawDolphin(&dolph);
    glPopMatrix();

    glPushMatrix();
    FishTransform(&momWhale);
    DrawWhale(&momWhale);
    glPopMatrix();

    glPushMatrix();
    FishTransform(&babyWhale);
    glScalef(0.45, 0.45, 0.3);
    DrawWhale(&babyWhale);
    glPopMatrix();

    glutSwapBuffers();
}

void
Visible(int state)
{
    if (state == GLUT_VISIBLE) {
        if (moving)
            glutIdleFunc(Animate);
    } else {
        if (moving)
            glutIdleFunc(NULL);
    }
}

void
menuSelect(int value)
{
    switch (value) {
    case 1:
        moving = GL_TRUE;
        glutIdleFunc(Animate);
        break;
    case 2:
        moving = GL_FALSE;;
        glutIdleFunc(NULL);
        break;
    case 3:
        exit(0);
        break;
    }
}

int
main(int argc, char **argv)
{
    glutInitWindowSize(500, 250);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutCreateWindow("GLUT Atlantis Demo");
    Init();
    glutDisplayFunc(Display);
    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Key);
    moving = GL_TRUE;
    glutIdleFunc(Animate);
    glutVisibilityFunc(Visible);
    glutCreateMenu(menuSelect);
    glutAddMenuEntry("Start motion", 1);
    glutAddMenuEntry("Stop motion", 2);
    glutAddMenuEntry("Quit", 3);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    glutMainLoop();
    return 0;             /* ANSI C requires main to return int. */
}
