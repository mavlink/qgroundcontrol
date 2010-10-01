/*
** An Example of dissolve, using stencil
*/
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <GL/glut.h>

#ifdef _WIN32
#define drand48() ((double)rand()/RAND_MAX)
#endif

int winWidth = 512;
int winHeight = 512;

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




GLfloat angle = 0.f; /* angle of rotating object in  layer 0 */

enum {X, Y};
GLboolean eraser = GL_FALSE;
GLint layer = 1;
GLint eraserpos[2] = {512/8, 512/12};

/* draw eraser and erase what's underneath */

GLubyte *eraserpix = 0;
int erasersize = 0;
int eraserWidth;
int eraserHeight;
void
makeEraser(void)
{
  int i;
  int x, y;
  int dx, dy;
  float d;

  eraserWidth = winWidth / 6;
  eraserHeight = winHeight / 6;
  erasersize = 4 * eraserWidth * eraserHeight;
  eraserpix = (GLubyte *)realloc(eraserpix, erasersize * sizeof(GLubyte));
				 

  /* make it not erase */
  (void)memset(eraserpix, 0, erasersize * sizeof(GLubyte));
  
  i = 0;
  for(y = 0; y < eraserHeight; y++)
      for(x = 0; x < eraserWidth; x++)
      {
	  dx = x - eraserWidth / 2;
	  dy = y - eraserHeight / 2;
	  d = sqrt(dx * dx + dy * dy);
          if(pow(drand48(), .75) * eraserWidth / 2 > d)
	  {
	      eraserpix[i + 0] = 255;
	      eraserpix[i + 1] = 255;
	      eraserpix[i + 2] = 255;
	      eraserpix[i + 3] = 255;
	  }
	  i += 4;
      }
}


/* left button, first layer, middle button, second layer */
/* ARGSUSED */
void
mouse(int button, int state, int x, int y)
{
  if(state == GLUT_DOWN) {
      eraser = GL_TRUE;
      if(button == GLUT_LEFT_BUTTON)
	layer = 1;
      else /* GLUT_MIDDLE: GLUT_RIGHT is for menu */
	layer = 0;
  } else { /* GLUT_UP */
    eraser = GL_FALSE;
  }
  glutPostRedisplay();
}


enum {CLEAR}; /* menu choices */
GLboolean clearstencil = GL_TRUE;

void
menu(int choice)
{
  switch(choice) {
  case CLEAR:
    clearstencil = GL_TRUE;
    break;
  }
  glutPostRedisplay();
}

/* used to get current width and height of viewport */
void
reshape(int wid, int ht)
{
  glViewport(0, 0, wid, ht);
  winWidth = wid;
  winHeight = ht;
  clearstencil = GL_TRUE;
  makeEraser();
  glutPostRedisplay();
}



void
draweraser(void)
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, winWidth, 0, winHeight);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  /* replace with this layer */
  glStencilFunc(GL_ALWAYS, layer, 0);
  glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);
  glDisable(GL_LIGHTING);
  glDisable(GL_DEPTH_TEST);

  glEnable(GL_ALPHA_TEST);
  glAlphaFunc(GL_NOTEQUAL, 0);
  glRasterPos2i(eraserpos[X], eraserpos[Y]);
  glBitmap(0, 0, 0.f, 0.f, -winWidth/8.f, -winHeight/12.f, 0);
  glDrawPixels(eraserWidth, eraserHeight, GL_RGBA, GL_UNSIGNED_BYTE, eraserpix);
  glDisable(GL_ALPHA_TEST);
}


void
drawlayer2(void)
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, winWidth, 0, winHeight);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glDisable(GL_LIGHTING);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_TEXTURE_2D);

  glBegin(GL_QUADS);
  glTexCoord2i(0, 0);
  glVertex2i(0, 0);
  glTexCoord2i(1, 0);
  glVertex2i(winWidth, 0);
  glTexCoord2i(1, 1);
  glVertex2i(winWidth, winHeight);
  glTexCoord2i(0, 1);
  glVertex2i(0, winHeight);
  glEnd();

  glDisable(GL_TEXTURE_2D);

  if(glGetError()) /* to catch programming errors; should never happen */
       printf("Oops! I screwed up my OpenGL calls somewhere\n");

}

void
drawlayer1(void)
{
    /* material properties for objects in scene */
    static GLfloat wall_mat[] = {1.f, 1.f, 1.f, 1.f};
    static GLfloat lightpos[] = {50.f, 50.f, -320.f, 1.f};

    /* draw a perspective scene */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-100., 100., -100., 100., 320., 640.); 
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    /* turn on features */
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    /* place light 0 in the right place */
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

    /* remove back faces to speed things up */
    glCullFace(GL_BACK);

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
    glVertex3f( 100.f, -100.f, -520.f);
    glTexCoord2i(0, 1);
    glVertex3f(-100.f, -100.f, -520.f);
    glEnd();

    glDisable(GL_TEXTURE_2D);

    /* walls */

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

    glPushMatrix();
    glTranslatef(-80.f, -80.f, -420.f);
    glCallList(SPHERE);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-20.f, -100.f, -500.f);
    glCallList(CONE);
    glPopMatrix();

    if(glGetError()) /* to catch programming errors; should never happen */
       printf("Oops! I screwed up my OpenGL calls somewhere\n");
}


void
drawlayer0(void)
{
  static GLfloat lightpos[] = {50.f, 50.f, 0.f, 1.f};
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-50.f, 50.f, -50.f, 50.f, 0.f, 100.f);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(0.f, 0.f, -50.f);
  glRotatef(angle, 0.f, 1.f, 0.f);
  glRotatef(90.f, 0.f, 0.f, 1.f);
  glTranslatef(0.f, -25.f, 0.f);

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
  glCullFace(GL_BACK);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  glCallList(CONE);

}

void
redraw(void)
{
  if(glutLayerGet(GLUT_NORMAL_DAMAGED) || 
		  clearstencil == GL_TRUE) {
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
    clearstencil = GL_FALSE;
  } else
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

  glStencilFunc(GL_EQUAL, 2, (unsigned)~0);
  drawlayer2();

  glStencilFunc(GL_EQUAL, 1, (unsigned)~0);
  drawlayer1();

  glStencilFunc(GL_EQUAL, 0, (unsigned)~0);
  drawlayer0();

  if(eraser)
    draweraser();

  glutSwapBuffers();
}

void
idle(void)
{
  angle += 1.f;
  glutPostRedisplay();
}


void
passive(int x, int y)
{

  eraserpos[X] = x;
  eraserpos[Y] = winHeight - y;
}


void
motion(int x, int y)
{

  eraserpos[X] = x;
  eraserpos[Y] = winHeight - y;

  glutPostRedisplay();
}

/* ARGSUSED1 */
void key(unsigned char key, int x, int y)
{
  switch(key) {
  case '\033':
    exit(0);
  }
}

const int TEXDIM = 256;
GLfloat *tex = 0;

main(int argc, char *argv[])
{
    static GLfloat sphere_mat[] = {1.f, .5f, 0.f, 1.f};
    static GLfloat cone_mat[] = {0.f, .5f, 1.f, 1.f};
    GLUquadricObj *sphere, *cone, *base;

    glutInit(&argc, argv);
    glutInitWindowSize(winWidth, winHeight);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_STENCIL|GLUT_DEPTH);
    (void)glutCreateWindow("dissolve");
    glutDisplayFunc(redraw);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutPassiveMotionFunc(passive);
    glutKeyboardFunc(key);
    glutIdleFunc(idle);
    glutReshapeFunc(reshape);

    glutCreateMenu(menu);
    glutAddMenuEntry("Clear Stencil", CLEAR);
    glutAttachMenu(GLUT_RIGHT_BUTTON);


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
    gluQuadricOrientation(base, GLU_INSIDE);
    gluDisk(base, 0., 20., 20, 1);
    gluCylinder(cone, 20., 0., 60., 20, 20);
    gluDeleteQuadric(cone);
    gluDeleteQuadric(base);
    glEndList();

    makeEraser();

    /* load pattern for current 2d texture */
    tex = make_texture(TEXDIM, TEXDIM);
    glTexImage2D(GL_TEXTURE_2D, 0, 1, TEXDIM, TEXDIM, 0, GL_RED, GL_FLOAT, tex);
    free(tex);

    glClearStencil(2);
    glEnable(GL_STENCIL_TEST); /* used all the time */

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glutMainLoop();

    return 0;
}
