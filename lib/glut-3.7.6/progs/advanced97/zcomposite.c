#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>

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

GLboolean stencil = GL_TRUE;

/* ARGSUSED1 */
void key(unsigned char key, int x, int y)
{
    switch(key) {
    case 't': /* toggle using stencil */
      if(stencil == GL_TRUE)
	stencil = GL_FALSE;
      else
	stencil = GL_TRUE;
      glutPostRedisplay();
      break;
    case '\033':
      exit(0);
      break;
    }
}

enum {SPHERE = 1, CONE};
enum {X, Y, Z};

int startx, starty;
int wid, ht;
int oldwid = 0, oldht = 0;

const int WINDIM = 512;
const GLfloat FRUSTDIM = 110.f;
const GLfloat FRUSTNEAR = 320.f;
const GLfloat FRUSTFAR = 540.f;
const GLfloat FRUSTDIFF = 540.f - 320.f;

GLboolean drawmode = GL_FALSE;
GLboolean depthmode = GL_FALSE;
GLboolean rubberbandmode = GL_FALSE;
GLfloat *color;
GLfloat *depth;
GLfloat depthbias = 0.f;
GLfloat raspos[] = {0.f, 0.f, -430.f};


int winWidth = 512;
int winHeight = 512;

GLfloat sx = 0;
GLfloat sy = 0;


/* Overlay Stuff */
int transparent;
int red;

void
setRasterPosXY(int x, int y)
{
      raspos[X] = (x - winWidth/2) * sx;
      raspos[Y] = (y - winHeight/2) * sy;

      glRasterPos3fv(raspos);

      glutPostRedisplay();
}

void
setRasterPosZ(int y)
{
      raspos[Z] = -(FRUSTNEAR + y * FRUSTDIFF/winHeight);

      depthbias = (y - winHeight/2.f)/winHeight;

      glRasterPos3fv(raspos);

      glutPostRedisplay();
}



void
motion(int x, int y)
{
  y = winHeight - y;
  if(drawmode)
    setRasterPosXY(x, y);

  if(rubberbandmode) {
    wid = x - startx;
    ht = y - starty;
    glutPostOverlayRedisplay();
  }

  if(depthmode)
     setRasterPosZ(y);
}

/* redraw function for overlay: used to show selected region */
void
overlay(void)
{
  if(glutLayerGet(GLUT_OVERLAY_DAMAGED)) {
    glClear(GL_COLOR_BUFFER_BIT);
  } else {
    glIndexi(transparent);
    glBegin(GL_LINE_LOOP);
    glVertex2i(startx, starty);
    glVertex2i(startx + oldwid, starty);
    glVertex2i(startx + oldwid, starty + oldht);
    glVertex2i(startx, starty + oldht);
    glEnd();
  }

  glIndexi(red);
  glBegin(GL_LINE_LOOP);
  glVertex2i(startx, starty);
  glVertex2i(startx + wid, starty);
  glVertex2i(startx + wid, starty + ht);
  glVertex2i(startx, starty + ht);
  glEnd();

  oldwid = wid;
  oldht = ht;

  glFlush();
}


/* used to get current width and height of viewport */
void
reshape(int wid, int ht)
{
  glutUseLayer(GLUT_OVERLAY);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, wid, 0, ht); /* 1 to 1 with window */
  glMatrixMode(GL_MODELVIEW);
  glViewport(0, 0, wid, ht);

  glutUseLayer(GLUT_NORMAL);
  glViewport((GLint) (-wid * .1), (GLint) (-ht * .1),
    (GLsizei) (wid * 1.2), (GLsizei) (ht * 1.2));

  winWidth = wid;
  winHeight = ht;

  sx = 2 * FRUSTDIM/(winWidth * 1.2);
  sy = 2 * FRUSTDIM/(winHeight * 1.2);
}


void
mouse(int button, int state, int x, int y)
{
  y = winHeight - y; /* flip y orientation */
  if(state == GLUT_DOWN)
    switch(button) {
    case GLUT_LEFT_BUTTON: /* select an image */
      startx = x;
      starty = y;
      wid = 0; ht = 0;
      rubberbandmode = GL_TRUE;
      glutShowOverlay();
      break;
    case GLUT_MIDDLE_BUTTON:
      glutUseLayer(GLUT_NORMAL);
      if(color && depth) {
	  drawmode = GL_TRUE;
	  setRasterPosXY(x, y);
      }
      break;
    case GLUT_RIGHT_BUTTON: /* change depth */
      glutUseLayer(GLUT_NORMAL);
      if(color && depth) {
	depthmode = GL_TRUE;
	setRasterPosZ(y);
      }
      break;
    }
  else /* GLUT_UP */
    switch(button) {
    case GLUT_LEFT_BUTTON:
      rubberbandmode = GL_FALSE;
      glutHideOverlay();
      wid = x - startx;
      ht = y - starty;
      if(wid < 0) {
	wid = -wid;
	startx = x;
      }
      if(ht < 0) {
	ht = -ht;
	starty = y;
      }
      color = (GLfloat *)realloc(color, wid * ht * 3 * sizeof(GLfloat));
      depth = (GLfloat *)realloc(depth, wid * ht * sizeof(GLfloat));

      glutUseLayer(GLUT_NORMAL);
      glReadPixels(startx, starty, wid, ht, GL_RGB, GL_FLOAT, color);
      glReadPixels(startx, starty, wid, ht, GL_DEPTH_COMPONENT, GL_FLOAT,
		   depth);
      break;
    case GLUT_MIDDLE_BUTTON:
      drawmode = GL_FALSE;
      break;
    case GLUT_RIGHT_BUTTON: /* change depth */
      depthmode = GL_FALSE;
      break;
    }
}


/* Called when window needs to be redrawn */
void 
redraw(void)
{
    /* material properties for objects in scene */
    static GLfloat wall_mat[] = {1.f, 1.f, 1.f, 1.f};

    glutUseLayer(GLUT_NORMAL);	
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

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
    glTranslatef(-40.f, -60.f, -400.f);
    glScalef(2, 2, 2);
    glCallList(SPHERE);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(50.f, -120.f, -400.f);
    glScalef(2, 2, 2);
    glCallList(CONE);
    glPopMatrix();

    if(stencil) {
      glEnable(GL_STENCIL_TEST);
      glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
      glStencilFunc(GL_ALWAYS, 1, 1);
      glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
      glPixelTransferf(GL_DEPTH_BIAS, depthbias);

      glDrawPixels(wid, ht, GL_DEPTH_COMPONENT, GL_FLOAT, depth);

      glPixelTransferf(GL_DEPTH_BIAS, 0.f);
      glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      glStencilFunc(GL_EQUAL, 1, 1);
      glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
      glDisable(GL_DEPTH_TEST);

      glDrawPixels(wid, ht, GL_RGB, GL_FLOAT, color);

      glEnable(GL_DEPTH_TEST);
      glDisable(GL_STENCIL_TEST);
    } else
      glDrawPixels(wid, ht, GL_RGB, GL_FLOAT, color);

    glutSwapBuffers();
}


const int TEXDIM = 256;
/* Parse arguments, and set up interface between OpenGL and window system */
main(int argc, char *argv[])
{
    GLfloat *tex;
    static GLfloat lightpos[] = {50.f, 50.f, -320.f, 1.f};
    static GLfloat sphere_mat[] = {1.f, .5f, 0.f, 1.f};
    static GLfloat cone_mat[] = {0.f, .5f, 1.f, 1.f};
    GLUquadricObj *sphere, *cone, *base;

    glutInit(&argc, argv);
    glutInitWindowSize(WINDIM, WINDIM);

    glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH|GLUT_STENCIL|GLUT_DOUBLE);
    (void)glutCreateWindow("compositing images with depth");
    glutDisplayFunc(redraw);
    glutKeyboardFunc(key);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutReshapeFunc(reshape);

    /* draw a perspective scene */
    glMatrixMode(GL_PROJECTION);
    glFrustum(-FRUSTDIM, FRUSTDIM, -FRUSTDIM, FRUSTDIM, FRUSTNEAR, FRUSTFAR); 
    glMatrixMode(GL_MODELVIEW);

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
    gluQuadricOrientation(base, GLU_INSIDE);
    gluDisk(base, 0., 20., 20, 1);
    gluCylinder(cone, 20., 0., 60., 20, 20);
    gluDeleteQuadric(cone);
    gluDeleteQuadric(base);
    glEndList();

    /* load pattern for current 2d texture */
    tex = make_texture(TEXDIM, TEXDIM);
    glTexImage2D(GL_TEXTURE_2D, 0, 1, TEXDIM, TEXDIM, 0, GL_RED, GL_FLOAT, tex);
    free(tex);

    /* storage for saved image */
    color = 0;
    depth = 0;
    
    glReadBuffer(GL_FRONT);/* so glReadPixel() always get the right image */

    glutInitDisplayMode(GLUT_SINGLE|GLUT_INDEX);
    if(glutLayerGet(GLUT_OVERLAY_POSSIBLE)) {
      glutEstablishOverlay();
      glutHideOverlay();
      transparent = glutLayerGet(GLUT_TRANSPARENT_INDEX);
      glClearIndex(transparent);
      red = (transparent + 1) % glutGet(GLUT_WINDOW_COLORMAP_SIZE);
      glutSetColor(red, 1.0, 0.0, 0.0);  /* Red. */
      glutOverlayDisplayFunc(overlay);
    }
	else
	{
		printf( "Overlay support unavailable - aborting.\n" );
		return 1;
	}

    glutMainLoop();

    return 0;
}
