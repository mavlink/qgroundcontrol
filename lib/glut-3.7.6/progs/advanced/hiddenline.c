
/* hiddenline.c - by Tom McReynolds, SGI */

/* Line Rendering: Hidden line techniques */

#include <GL/glut.h>
#include <stdlib.h>
#include <stdio.h>

/*
** Create a single component texture map
*/
GLfloat *make_texture(int maxs, int maxt)
{
    int s, t;
    static GLfloat *texture;

    texture = (GLfloat *)malloc(maxs * maxt * sizeof(GLfloat));
    for(t = 0; t < maxt; t++) {
        for(s = 0; s < maxs; s++) {
            texture[s + maxs * t] = ((s >> 4) & 0x1) ^ ((t >> 4) & 0x1);
        }
    }
    return texture;
}

enum {FILL, WIRE, BACKFACE, FRONTLINES, DEPTH, STENCIL, FAT_STENCIL};

int rendermode = FILL;

void
menu(int selection)
{
  rendermode = selection;
  glutPostRedisplay();
}


/* geometry display list names */
enum {SPHERE = 1, CONE, FLOOR, WALLS};

void
drawscene(void)
{
    glEnable(GL_TEXTURE_2D);
    glCallList(FLOOR);
    glDisable(GL_TEXTURE_2D);

    glCallList(WALLS);

    glPushMatrix();
    glTranslatef(-40.f, -50.f, -400.f);
    glCallList(SPHERE);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(20.f, -100.f, -420.f);
    glCallList(CONE);
    glPopMatrix();
}

/* do hidden line removal on an object in the scene */
/* use display lists to represent objects */
void
hiddenlineobj(GLint dlist)
{
    GLboolean tex2d;

    glGetBooleanv(GL_TEXTURE_2D, &tex2d);
    if(tex2d)
      glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glColor3f(.7f, .7f, .7f);

    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 0, 0); /* clear stencil for this object */

    glCallList(dlist); /* draw filled object in depth buffer */

    glEnable(GL_LIGHTING);
    if(tex2d)
      glEnable(GL_TEXTURE_2D);

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); /* turn off color */
    glDisable(GL_DEPTH_TEST); /* turn off depth */
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glStencilFunc(GL_ALWAYS, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    glCallList(dlist); /* draw lines into stencil buffer */

    glStencilFunc(GL_EQUAL, 1 , 1);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glCallList(dlist); /* use lines in stencil to stencil out solid pgons */

    /* clean up state */
    glDisable(GL_STENCIL_TEST);
    glDepthFunc(GL_LESS);
}

void
drawscenestencil(void)
{
    glEnable(GL_TEXTURE_2D);
    hiddenlineobj(FLOOR);
    glDisable(GL_TEXTURE_2D);

    hiddenlineobj(WALLS);

    glPushMatrix();
    glTranslatef(-40.f, -50.f, -400.f);
    hiddenlineobj(SPHERE);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(20.f, -100.f, -420.f);
    hiddenlineobj(CONE);
    glPopMatrix();
}

/* Called when window needs to be redrawn */
void redraw(void)
{
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

  switch(rendermode) {
  case FILL:
    drawscene();
    break;
  case WIRE: /* basic wireframe mode */
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    drawscene();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    break;
  case BACKFACE: /* use backface culling to clean things up */
    glEnable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    drawscene();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_CULL_FACE);
    break;
  case FRONTLINES: /* use polygon mode line on front, fill on back */
    glPolygonMode(GL_FRONT, GL_LINE);
    drawscene();
    glPolygonMode(GL_FRONT, GL_FILL);
    break;
  case DEPTH: /* use depth buffer to remove hidden lines */
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    drawscene();
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDepthFunc(GL_LEQUAL);
    drawscene();
    glDepthFunc(GL_LESS);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    break;
  case STENCIL: /* use stencil to remove hidden lines */
    glEnable(GL_CULL_FACE);
    drawscenestencil();
    glDisable(GL_CULL_FACE);
    break;
  case FAT_STENCIL: /* use stencil with fat lines to fix edges */
    glLineWidth(2.f);
    glEnable(GL_CULL_FACE);
    drawscenestencil();
    glDisable(GL_CULL_FACE);
    glLineWidth(1.f);
    break;
  }

    if(glGetError()) /* to catch programming errors; should never happen */
       printf("Oops! I screwed up my OpenGL calls somewhere\n");

    glFlush(); /* high end machines may need this */
}


/* ARGSUSED1 */
void key(unsigned char key, int x, int y)
{
    if(key == '\033')
        exit(0);
}

const int TEXDIM = 256;

/* Parse arguments, and set up interface between OpenGL and window system */
int
main(int argc, char *argv[])
{
    /* material properties for objects in scene */
    static GLfloat wall_mat[] = {1.f, 1.f, 1.f, 1.f};

    GLfloat *tex;
    static GLfloat lightpos[] = {50.f, 50.f, -320.f, 1.f};
    static GLfloat sphere_mat[] = {1.f, .5f, 0.f, 1.f};
    static GLfloat cone_mat[] = {0.f, .5f, 1.f, 1.f};
    GLUquadricObj *sphere, *cone, *base;

    glutInit(&argc, argv);
    glutInitWindowSize(512, 512);
    glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH|GLUT_STENCIL);
    (void)glutCreateWindow("hidden line removal survey");
    glutDisplayFunc(redraw);
    glutKeyboardFunc(key);

    glutCreateMenu(menu);
    glutAddMenuEntry("Solid Fill", FILL);
    glutAddMenuEntry("Wireframe", WIRE);
    glutAddMenuEntry("Backface Culling", BACKFACE);
    glutAddMenuEntry("Frontface Lines", FRONTLINES);
    glutAddMenuEntry("Depth Test", DEPTH);
    glutAddMenuEntry("Stencil", STENCIL);
    glutAddMenuEntry("Stencil: Fat Lines", FAT_STENCIL);
    glutAttachMenu(GLUT_RIGHT_BUTTON);


    /* draw a perspective scene */
    glMatrixMode(GL_PROJECTION);
    glFrustum(-100., 100., -100., 100., 320., 640.); 
    glMatrixMode(GL_MODELVIEW);

    /* turn on features */
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glClearColor(.7f, .7f, .7f, .7f);

    glCullFace(GL_BACK);

    /* place light 0 in the right place */
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

    /* remove back faces to speed things up */
    glCullFace(GL_BACK);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glNewList(SPHERE, GL_COMPILE);
    /* make display lists for sphere and cone; for efficiency */
    sphere = gluNewQuadric();
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, sphere_mat);
    gluSphere(sphere, 50.f, 20, 20);
    gluDeleteQuadric(sphere);
    glEndList();

    glNewList(CONE, GL_COMPILE);
    cone = gluNewQuadric();
    base = gluNewQuadric();
    glPushMatrix();
    glRotatef(-90.f, 1.f, 0.f, 0.f);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, cone_mat);
    gluQuadricOrientation(base, GLU_INSIDE);
    gluDisk(base, 0., 40., 20, 1);
    gluCylinder(cone, 40., 0., 120., 20, 20);
    glPopMatrix();
    gluDeleteQuadric(cone);
    gluDeleteQuadric(base);
    glEndList();

    glNewList(FLOOR, GL_COMPILE);
    /*
    ** Note: wall verticies are ordered so they are all front facing
    ** this lets me do back face culling to speed things up.
    */
 
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, wall_mat);

    /* floor */

    /*
    ** Since we want to turn texturing on for floor only, we have to
    ** make floor a separate glBegin()/glEnd() sequence. You can't
    ** turn texturing on and off between begin and end calls
    */
    glBegin(GL_QUADS);
    glNormal3f(0.f, 1.f, 0.f);
    glTexCoord2i(0, 0);
    glVertex3f(-100.f, -100.f, -320.f);
    glTexCoord2i(1, 0);
    glVertex3f( 100.f, -100.f, -320.f);
    glTexCoord2i(1, 1);
    glVertex3f( 100.f, -100.f, -520.f);
    glTexCoord2i(0, 1);
    glVertex3f(-100.f, -100.f, -520.f);
    glEnd();
    glEndList();

    /* walls */

    glNewList(WALLS, GL_COMPILE);
    glBegin(GL_QUADS);
    /* left wall */
    glNormal3f(1.f, 0.f, 0.f);
    glVertex3f(-100.f, -100.f, -320.f);
    glVertex3f(-100.f, -100.f, -520.f);
    glVertex3f(-100.f,  100.f, -520.f);
    glVertex3f(-100.f,  100.f, -320.f);

    /* right wall */
    glNormal3f(-1.f, 0.f, 0.f);
    glVertex3f( 100.f, -100.f, -320.f);
    glVertex3f( 100.f,  100.f, -320.f);
    glVertex3f( 100.f,  100.f, -520.f);
    glVertex3f( 100.f, -100.f, -520.f);

    /* ceiling */
    glNormal3f(0.f, -1.f, 0.f);
    glVertex3f(-100.f,  100.f, -320.f);
    glVertex3f(-100.f,  100.f, -520.f);
    glVertex3f( 100.f,  100.f, -520.f);
    glVertex3f( 100.f,  100.f, -320.f);

    /* back wall */
    glNormal3f(0.f, 0.f, 1.f);
    glVertex3f(-100.f, -100.f, -520.f);
    glVertex3f( 100.f, -100.f, -520.f);
    glVertex3f( 100.f,  100.f, -520.f);
    glVertex3f(-100.f,  100.f, -520.f);
    glEnd();

    glEndList();
    /* load pattern for current 2d texture */
    tex = make_texture(TEXDIM, TEXDIM);
    glTexImage2D(GL_TEXTURE_2D, 0, 1, TEXDIM, TEXDIM, 0, GL_RED, GL_FLOAT, tex);
    free(tex);

    glutMainLoop();
    return 0;             /* ANSI C requires main to return int. */
}

