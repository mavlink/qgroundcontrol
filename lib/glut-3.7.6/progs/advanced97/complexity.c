/* This program demonstrates how to use the stencil buffer to visualize
** the depth complexity of a scene.
*/

#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>

/* show contents of stencil buffer */
int winwid = 512;
int winht = 512;
GLubyte *stencil = 0; /* so realloc works the first time */

void resize(int wid, int ht)
{
    winwid = wid;
    winht = ht;
    stencil = (GLubyte *)realloc((void*)stencil,
                                 winwid * winht * sizeof(GLubyte));
    glViewport(0, 0, wid, ht);
}

/* ARGSUSED1 */
void key(unsigned char key, int x, int y)
{
    if(key == '\033')
        exit(0);
}

int rotate = 0;
GLfloat udangle = 0.f;
GLfloat lrangle = 0.f;

void motion(int x, int y)
{
    if(rotate)
    {
        udangle = (y - winht/2) * 360./winht;
        lrangle = (x - winwid/2) * 360./winwid;
    }
    glutPostRedisplay();
}

void mouse(int button, int state, int x, int y)
{
    if(state == GLUT_DOWN)
    switch(button)
    {
    case GLUT_LEFT_BUTTON: /* rotate the scene up and down */
    case GLUT_MIDDLE_BUTTON: /* rotate the scene left and right */
        rotate = 1;
        motion(x, y);
        break;
    }
    else
        rotate = 0; /* overkill; cover right button too */
}


/* read back stencil buffer, store in memory, draw back colorized */
void showstencil(void)
{
    glReadPixels(0, 0, winwid, winht, GL_STENCIL_INDEX, 
                 GL_UNSIGNED_BYTE, stencil);

    glRasterPos2i(-1, -1);
    glDrawPixels(winwid, winht, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, stencil);
}


int showdepth = 0;
int depthtest = 1;
/* Called when window needs to be redrawn */
void redraw(void)
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

    glEnable(GL_STENCIL_TEST);
    if(depthtest)
        glEnable(GL_DEPTH_TEST);

    glPushMatrix();
    glRotatef(lrangle, 0.f, 1.f, 0.f);
    glRotatef(udangle, 1.f, 0.f, 0.f);
    glCallList(1); /* draw scene */
    glPopMatrix();

    glDisable(GL_STENCIL_TEST);
    glFlush(); /* high end machines may need this */

    if(depthtest)
        glDisable(GL_DEPTH_TEST);

    if(showdepth)
        showstencil();

    if(glGetError()) /* to catch programming errors; should never happen */
       printf("Oops! I screwed up my OpenGL calls somewhere\n");
    glutSwapBuffers();
}

/* menu entries mapped to actions */
enum {RENDER, SHOW_STENCIL, DEPTH_TEST};
void menu(int choice)
{
    switch(choice)
    {
    case RENDER:
        showdepth = 0;
        break;
    case SHOW_STENCIL:
        showdepth = 1;
        break;
    case DEPTH_TEST:
        depthtest = !depthtest;
        if(depthtest)
            /* show how many pixels were discarded by depth test */
            glStencilOp(GL_KEEP, GL_INCR, GL_KEEP);
        else
            /* show how many pixels were written to frame buffer */
            glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
        break;
    }
    glutPostRedisplay();
}

typedef struct {
    GLfloat r;
    GLfloat g;
    GLfloat b;
} Color;
 
/* color map to indicate different depth complexities */
Color map[] = {
    {0.f, 0.f, 0.f,},
    {0.f, .25f, 0.f,},
    {0.f, .5f, 0.f,},
    {0.f, .75f, 0.f,},
    {0.f, 1.f, 0.f,},
    {.25f, 1.f, 0.f,},
    {.5f, 1.f, 0.f,},
    {.75f, 1.f, 0.f,},
    {1.f, 1.f, 0.f,},
    {1.f, .75f, 0.f,},
    {1.f, .5f, 0.f,},
    {1.f, .25f, 0.f,},
    {1.f, .0f, 0.f,},
    {1.f, .0f, 0.f,},
    {1.f, .0f, 0.f,},
    {1.f, .0f, 0.f,}
};

/* mapsize should be a power of two */
#define mapsize 16
GLfloat lightpos[4] = {.5f, .5f, -1.f, 1.f};
main(int argc, char *argv[])
{
    GLfloat rmap[mapsize], gmap[mapsize], bmap[mapsize];
    int i;

    glutInit(&argc, argv);
    glutInitWindowSize(winwid, winht);
    glutInitDisplayMode(GLUT_RGBA|GLUT_STENCIL|GLUT_DOUBLE|GLUT_DEPTH);
    (void)glutCreateWindow("visualizing depth complexity");
    glutDisplayFunc(redraw);
    glutKeyboardFunc(key);
    glutReshapeFunc(resize);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);

    glutCreateMenu(menu);
    glutAddMenuEntry("Draw Scene", RENDER);
    glutAddMenuEntry("Show Stencil", SHOW_STENCIL);
    glutAddMenuEntry("Toggle Depth Test", DEPTH_TEST);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    glStencilFunc(GL_ALWAYS, ~0, ~0);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);

    /* draw an interesting scene */
    glNewList(1, GL_COMPILE);
    /* center */
    glPushMatrix();
    glScalef(.2f, .2f, .2f);
    glutSolidTetrahedron();
    glPopMatrix();
    /* right */
    glTranslatef(.4f, 0.f, 0.f);
    glutSolidSphere(.25, 8, 8);
    /* left */
    glTranslatef(-.8f, 0.f, 0.f);
    glutSolidSphere(.25, 8, 8);
    /* bottom */
    glTranslatef(.4f, -.4f, 0.f);
    glutSolidSphere(.25, 8, 8);
    /* top */
    glTranslatef(0.f, .8f, 0.f);
    glutSolidSphere(.25, 8, 8);

    /* lefttop */
    glTranslatef(-.5f, .1f, 0.f);
    glutSolidCube(.3);
    /* righttop */
    glTranslatef(1.f, 0.f, 0.f);
    glutSolidCube(.3);
    /* rightbot */
    glTranslatef(0.f, -1.f, 0.f);
    glutSolidCube(.3);
    /* rightbot */
    glTranslatef(-1.f, 0.f, 0.f);
    glutSolidCube(.3);
    glEndList();

    /* color ramp to show increasing complexity */
    /* black shading to green to yellow to red */
    for(i = 0; i < mapsize; i++)
    {
        rmap[i] = map[i].r;
        gmap[i] = map[i].g;
        bmap[i] = map[i].b;
    }

    glPixelMapfv(GL_PIXEL_MAP_I_TO_R, mapsize, rmap);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_G, mapsize, gmap);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_B, mapsize, bmap);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

    glutMainLoop();
    return 0;
}

