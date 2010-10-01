
/* field.c - by Tom McReynolds, SGI */

/* Using the accumulation buffer for depth of field (camera focus blur). */

#include <GL/glut.h>
#include <stdlib.h>
#include <stdio.h>

const GLdouble FRUSTDIM = 100.f;
const GLdouble FRUSTNEAR = 320.f;
const GLdouble FRUSTFAR = 660.f;

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

enum {SPHERE = 1, CONE};

void
render(void)
{
    /* material properties for objects in scene */
    static GLfloat wall_mat[] = {1.f, 1.f, 1.f, 1.f};

    glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

    /*
    ** Note: wall verticies are ordered so they are all front facing
    ** this lets me do back face culling to speed things up.
    */
 
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, wall_mat);

    /* floor */
    /* make the floor textured */
    glEnable(GL_TEXTURE_2D);

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
    glVertex3f( 100.f, -100.f, -640.f);
    glTexCoord2i(0, 1);
    glVertex3f(-100.f, -100.f, -640.f);
    glEnd();

    glDisable(GL_TEXTURE_2D);

    /* walls */

    glBegin(GL_QUADS);
    /* left wall */
    glNormal3f(1.f, 0.f, 0.f);
    glVertex3f(-100.f, -100.f, -320.f);
    glVertex3f(-100.f, -100.f, -640.f);
    glVertex3f(-100.f,  100.f, -640.f);
    glVertex3f(-100.f,  100.f, -320.f);

    /* right wall */
    glNormal3f(-1.f, 0.f, 0.f);
    glVertex3f( 100.f, -100.f, -320.f);
    glVertex3f( 100.f,  100.f, -320.f);
    glVertex3f( 100.f,  100.f, -640.f);
    glVertex3f( 100.f, -100.f, -640.f);

    /* ceiling */
    glNormal3f(0.f, -1.f, 0.f);
    glVertex3f(-100.f,  100.f, -320.f);
    glVertex3f(-100.f,  100.f, -640.f);
    glVertex3f( 100.f,  100.f, -640.f);
    glVertex3f( 100.f,  100.f, -320.f);

    /* back wall */
    glNormal3f(0.f, 0.f, 1.f);
    glVertex3f(-100.f, -100.f, -640.f);
    glVertex3f( 100.f, -100.f, -640.f);
    glVertex3f( 100.f,  100.f, -640.f);
    glVertex3f(-100.f,  100.f, -640.f);
    glEnd();


    glPushMatrix();
    glTranslatef(-80.f, -60.f, -420.f);
    glCallList(SPHERE);
    glPopMatrix();


    glPushMatrix();
    glTranslatef(-20.f, -80.f, -600.f);
    glCallList(CONE);
    glPopMatrix();

    if(glGetError()) /* to catch programming errors; should never happen */
       printf("Oops! I screwed up my OpenGL calls somewhere\n");

    glFlush(); /* high end machines may need this */
}

enum {NONE, FIELD};

int rendermode = NONE;

void
menu(int selection)
{
  rendermode = selection;
  glutPostRedisplay();
}

GLdouble focus = 420.;

/* Called when window needs to be redrawn */
void redraw(void)
{
    int i, j;
    int min, max;
    int count;
    GLfloat scale, dx, dy;

    switch(rendermode) {
    case NONE:
      glLoadIdentity();
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glFrustum(-FRUSTDIM, FRUSTDIM, -FRUSTDIM, FRUSTDIM, FRUSTNEAR, FRUSTFAR); 
      glMatrixMode(GL_MODELVIEW);
      render();
      break;
    case FIELD:
      min = -2;
      max = -min + 1;
      count = -2 * min + 1;
      count *= count;

      scale = 2.f;

      glClear(GL_ACCUM_BUFFER_BIT);

      for(j = min; j < max; j++) {
        for(i = min; i < max; i++) {
          dx = scale * i * FRUSTNEAR/focus;
          dy = scale * j * FRUSTNEAR/focus;
          glMatrixMode(GL_PROJECTION);
          glLoadIdentity();
          glFrustum(-FRUSTDIM + dx, 
                    FRUSTDIM + dx, 
                    -FRUSTDIM + dy, 
                    FRUSTDIM + dy, 
                    FRUSTNEAR,
                    FRUSTFAR); 
          glMatrixMode(GL_MODELVIEW);
          glLoadIdentity();
          glTranslatef(scale * i, scale * j, 0.f);
          render();
          glAccum(GL_ACCUM, 1.f/count);
        }
      } 
      glAccum(GL_RETURN, 1.f);
    break;
    }

    glutSwapBuffers();
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
    GLfloat *tex;
    static GLfloat lightpos[] = {50.f, 50.f, -320.f, 1.f};
    static GLfloat sphere_mat[] = {1.f, .5f, 0.f, 1.f};
    static GLfloat cone_mat[] = {0.f, .5f, 1.f, 1.f};
    GLUquadricObj *sphere, *cone, *base;

    glutInit(&argc, argv);
    glutInitWindowSize(512, 512);
    glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH|GLUT_ACCUM|GLUT_DOUBLE);
    (void)glutCreateWindow("depth of field");
    glutDisplayFunc(redraw);
    glutKeyboardFunc(key);

    glutCreateMenu(menu);
    glutAddMenuEntry("Normal", NONE);
    glutAddMenuEntry("Depth of Field", FIELD);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    /* turn on features */
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    /* place light 0 in the right place */
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

    /* remove back faces to speed things up */
    glCullFace(GL_BACK);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glNewList(SPHERE, GL_COMPILE);
    /* make display lists for sphere and cone; for efficiency */
    sphere = gluNewQuadric();
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, sphere_mat);
    gluSphere(sphere, 20.f, 20, 20);
    gluDeleteQuadric(sphere);
    glEndList();

    glNewList(CONE, GL_COMPILE);
    cone = gluNewQuadric();
    base = gluNewQuadric();
    glRotatef(-90.f, 1.f, 0.f, 0.f);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, cone_mat);
    gluDisk(base, 0., 20., 20, 1);
    gluCylinder(cone, 20., 0., 60., 20, 20);
    gluDeleteQuadric(cone);
    gluDeleteQuadric(base);
    glEndList();

    /* load pattern for current 2d texture */
    tex = make_texture(TEXDIM, TEXDIM);
    glTexImage2D(GL_TEXTURE_2D, 0, 1, TEXDIM, TEXDIM, 0, GL_RED, GL_FLOAT, tex);
    free(tex);

    glutMainLoop();
    return 0;             /* ANSI C requires main to return int. */
}

