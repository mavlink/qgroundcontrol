#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#define sleep(x) Sleep(1000*x)
#else
#include <unistd.h>
#endif

#include <GL/glut.h>

const GLdouble FRUSTDIM = 100.f;
int win_width = 256;
int win_height = 256;
int show_results = GL_TRUE;
int object_offset, font_offset;

enum {SPHERE, CONE};

/*{0x00, 0x60, 0x60, 0x30, 0x30, 0x18, 0x18, 0x0c, 0x0c, 0x06, 0x06, 0x03, 0x03},*/
GLubyte rasters[][13] = {
{0x00, 0x00, 0x3c, 0x66, 0xc3, 0xe3, 0xf3, 0xdb, 0xcf, 0xc7, 0xc3, 0x66, 0x3c},
{0x00, 0x00, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x78, 0x38, 0x18},
{0x00, 0x00, 0xff, 0xc0, 0xc0, 0x60, 0x30, 0x18, 0x0c, 0x06, 0x03, 0xe7, 0x7e},
{0x00, 0x00, 0x7e, 0xe7, 0x03, 0x03, 0x07, 0x7e, 0x07, 0x03, 0x03, 0xe7, 0x7e},
{0x00, 0x00, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0xff, 0xcc, 0x6c, 0x3c, 0x1c, 0x0c},
{0x00, 0x00, 0x7e, 0xe7, 0x03, 0x03, 0x07, 0xfe, 0xc0, 0xc0, 0xc0, 0xc0, 0xff},
{0x00, 0x00, 0x7e, 0xe7, 0xc3, 0xc3, 0xc7, 0xfe, 0xc0, 0xc0, 0xc0, 0xe7, 0x7e},
{0x00, 0x00, 0x30, 0x30, 0x30, 0x30, 0x18, 0x0c, 0x06, 0x03, 0x03, 0x03, 0xff},
{0x00, 0x00, 0x7e, 0xe7, 0xc3, 0xc3, 0xe7, 0x7e, 0xe7, 0xc3, 0xc3, 0xe7, 0x7e},
{0x00, 0x00, 0x7e, 0xe7, 0x03, 0x03, 0x03, 0x7f, 0xe7, 0xc3, 0xc3, 0xe7, 0x7e},
};

GLfloat mults[] = {1., 1./2., 2./3., 3./4., 4./5., 5./6., 6./7., 7./8., 8./9., 9./10.};


/*
** Create a single component texture map
*/

void make_font(void)
{
   GLuint i;
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

   font_offset = glGenLists (10);
   for (i = 0; i < 10; i++) {
      glNewList(i+font_offset, GL_COMPILE);
      glBitmap(8, 13, 0.0, 2.0, 10.0, 0.0, rasters[i]);
      glEndList();
   }
}

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

void print_frame_number(GLuint frame)
{
    GLuint f1, f2;

    glPushAttrib( GL_ENABLE_BIT | GL_CURRENT_BIT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho (0.0, win_width, 0.0, win_height, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    glColor3f(1.0, 0.0, 0.0);

    f1 = frame/10;
    if (f1 > 0) {
        glRasterPos2i(50, win_height - 50);
        glCallList(font_offset + f1);
    }
    else
        glRasterPos2i(60, win_height - 50);
    f2 = frame - f1*10;
    glCallList(font_offset + f2);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
}

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
    glTranslatef(-80.f, -60.f, -420.f);
    glCallList(object_offset + SPHERE);
    glPopMatrix();


    glPushMatrix();
    glTranslatef(-20.f, -80.f, -500.f);
    glCallList(object_offset + CONE);
    glPopMatrix();

    if(glGetError()) /* to catch programming errors; should never happen */
       printf("Oops! I screwed up my OpenGL calls somewhere\n");

    glFlush(); /* high end machines may need this */
}

/* compute scale factor for window->object space transform */
/* could use gluUnProject(), but probably too much trouble */
void
computescale(GLfloat *sx, GLfloat *sy)
{
  enum {XORG, YORG, WID, HT};
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);

  *sx = 2 * FRUSTDIM/viewport[WID];
  *sy = 2 * FRUSTDIM/viewport[WID];
}

enum {NONE, AA, AA_NEW};

int rendermode = NONE;

void
menu(int selection)
{
  rendermode = selection;
  glutPostRedisplay();
}



/* Called when window needs to be redrawn */
void redraw(void)
{
    GLfloat invx, invy;
    GLfloat scale, dx, dy;
    int i, j;
    int min, max;
    int count, nframes;

    switch(rendermode) {
    case NONE:
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glFrustum(-FRUSTDIM, FRUSTDIM, -FRUSTDIM,	FRUSTDIM, 320., 640.); 
      glMatrixMode(GL_MODELVIEW);
      render();
      glutSwapBuffers();
      break;
    case AA:
      min = -1;
      max = -min + 1;
      count = -2 * min + 1;
      count *= count;
      /* uniform scaling, less than one pixel wide */
      scale = -.9f/min;

      computescale(&invx, &invy);

      glClear(GL_ACCUM_BUFFER_BIT);

      for(j = min, nframes = 1; j < max; j++) {
	for(i = min; i < max; i++, nframes++) {
	  dx = invx * scale * i;
	  dy = invy * scale * j;
	  glMatrixMode(GL_PROJECTION);
	  glLoadIdentity();
	  glFrustum(-FRUSTDIM + dx, 
		    FRUSTDIM + dy, 
		    -FRUSTDIM + dx, 
		    FRUSTDIM + dy, 
		    320., 640.); 
	  glMatrixMode(GL_MODELVIEW);
	  render();
	  glAccum(GL_ACCUM, 1.f/count);

          if (show_results) {
	    if (nframes == 1) {
		glutSwapBuffers();
	        glDrawBuffer(GL_FRONT);
	    } else if (nframes == 2) {
	        glDrawBuffer(GL_FRONT);
                glAccum(GL_RETURN, 3.99);
            } else {
	        glDrawBuffer(GL_FRONT);
                glAccum(GL_RETURN, (GLfloat)count/(GLfloat)nframes);
	    }
	    print_frame_number(nframes);
            glFlush();
	    printf("frame number %d\n",nframes);
	    glDrawBuffer(GL_BACK);
	    sleep(3);
	  }
	}
      }
      if (!show_results) {
	 glAccum(GL_RETURN, 1.f);
         glutSwapBuffers();
      }
      break;
    case AA_NEW:
      min = -2;
      max = -min + 1;
      count = -2 * min + 1;
      count *= count;
      /* uniform scaling, less than one pixel wide */
      scale = -.9f/min;

      computescale(&invx, &invy);

      glClear(GL_ACCUM_BUFFER_BIT);

      for(j = min, nframes = 1; j < max; j++) {
	for(i = min; i < max; i++, nframes++) {
	  dx = invx * scale * i;
	  dy = invy * scale * j;
	  glMatrixMode(GL_PROJECTION);
	  glLoadIdentity();
	  glFrustum(-FRUSTDIM + dx, 
		    FRUSTDIM + dy, 
		    -FRUSTDIM + dx, 
		    FRUSTDIM + dy, 
		    320., 640.); 
	  glMatrixMode(GL_MODELVIEW);
	  render();
 	  if (nframes == 1)
	      glAccum(GL_ACCUM, 1.);
 	  else
	      glAccum(GL_ACCUM, .5);

          if (show_results) {
	      glDrawBuffer(GL_FRONT);
              glAccum(GL_RETURN, 1.);
	      print_frame_number(nframes);
              glFlush();
	      printf("frame number %d\n",nframes);
	      glDrawBuffer(GL_BACK);
	      sleep(3);
	  }

	  if (nframes < count) {
	      glAccum(GL_RETURN, 1.);
	      glAccum(GL_LOAD, .5);
          }

	}
      }
      if (!show_results) {
	 glAccum(GL_RETURN, 1.f);
         glutSwapBuffers();
      }
      break;
    }

}

void reshape(int w, int h)
{
    if (w < h) {
       win_height = win_width = w;
    } else {
       win_height = win_width = h;
    }
    glViewport(0, 0, win_width, win_height);
}

/* ARGSUSED1 */
void key(unsigned char key, int x, int y)
{
    if(key == '\033')
	exit(0);
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
    glutInitWindowSize(win_width, win_height);
    glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH|GLUT_ACCUM|GLUT_DOUBLE);
    (void)glutCreateWindow("Accum antialias");
    glutDisplayFunc(redraw);
    glutKeyboardFunc(key);
    glutReshapeFunc(reshape);

    glutCreateMenu(menu);
    glutAddMenuEntry("Aliased View", NONE);
    glutAddMenuEntry("AntiAliased View", AA);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    make_font();

    /* draw a perspective scene */
    glMatrixMode(GL_PROJECTION);
    glFrustum(-FRUSTDIM, FRUSTDIM, -FRUSTDIM, FRUSTDIM, 320., 640.); 
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

    object_offset = glGenLists(2);
    glNewList(object_offset + SPHERE, GL_COMPILE);
    /* make display lists for sphere and cone; for efficiency */
    sphere = gluNewQuadric();
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, sphere_mat);
    gluSphere(sphere, 20.f, 20, 20);
    gluDeleteQuadric(sphere);
    glEndList();

    glNewList(object_offset + CONE, GL_COMPILE);
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

    glReadBuffer(GL_BACK); /* input to accum buffer */

    glutMainLoop();

    return 0;
}

