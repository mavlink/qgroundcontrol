
/* convolve.c - by Tom McReynolds, SGI */

/* Using the accumulation buffer for fast convolutions. */

#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* convolution choices */
enum {CONV_NONE, 
      CONV_BOX_3X3,
      CONV_BOX_5X5,
      CONV_SOBEL_X,
      CONV_LAPLACE
};

/* Filter contents and size */
typedef struct {
  GLfloat scale; /* 1/scale applied to image to prevent overflow */
  GLfloat bias; /* for biasing images */
  int rows;
  int cols;
  GLfloat *array;
} Filter;

Filter *curmat; /* current filter to use for redrawing */

/* identity filter */
void
identity(Filter *mat)
{
  int n, size;
  size = mat->rows * mat->cols;

  mat->array[0] = 1.f;
  for(n = 1; n < size; n++)
    mat->array[n] = 0.f;

  mat->scale = 1.f;
  mat->bias = 0.f;
}


/* create a new filter with identity filter in it */
Filter *
newfilter(int rows, int cols)
{
  Filter *mat;

  mat = (Filter *)malloc(sizeof(Filter));
  mat->rows = rows;
  mat->cols = cols;
  mat->array = (GLfloat *)malloc(rows * cols * sizeof(GLfloat));
  identity(mat);
  
  return mat;
}


/* doesn't re-initialize matrix */
void
resize(Filter *mat, int rows, int cols)
{
  if(mat->rows != rows ||
     mat->cols != cols) {
    free(mat->array);
    mat->array = NULL;
    mat->array = (GLfloat *)realloc(mat->array, rows * cols * sizeof(GLfloat));
  }
  mat->rows = rows;
  mat->cols = cols;
}


/* box filter blur */
void
box(Filter *mat)
{
  int n, count;
  GLfloat blur;

  count = mat->cols * mat->rows;
  blur = 1.f/count;
  for(n = 0; n < count; n++)
     mat->array[n] = blur;

  mat->scale = 1.f;
  mat->bias = 0.f;
}

/* sobel filter */

void
sobel(Filter *mat)
{
  static GLfloat sobel[] = {-.5f, 0.f, .5f,
                            -1.f, 0.f, 1.f,
                            -.5f, 0.f, .5f};

  /* sobel is fixed size */
  resize(mat, 3, 3); /* will do nothing if size is right already */
  
  memcpy(mat->array, sobel, sizeof(sobel));

  mat->scale = 2.f;
  mat->bias = 0.f;
}

/* laplacian filter */
void
laplace(Filter *mat)
{
  static GLfloat laplace[] = {  0.f, -.25f,   0.f,
                              -.25f,   1.f, -.25f,
                                0.f, -.25f,   0.f};

  /* sobel is fixed size */
  resize(mat, 3, 3); /* will do nothing if size is right already */
  
  memcpy(mat->array, laplace, sizeof(laplace));

  mat->scale = 4.f;
  mat->bias = .125f;
}

/* add menu callback */

void menu(int filter)
{
  switch(filter) {
    case CONV_NONE:
      resize(curmat, 1,1);
      identity(curmat);
      break;
    case CONV_BOX_3X3:
      resize(curmat, 3, 3);
      box(curmat);
      break;
    case CONV_BOX_5X5:
      resize(curmat, 5, 5);
      box(curmat);
      break;
    case CONV_SOBEL_X:
      sobel(curmat);
      break;
    case CONV_LAPLACE:
      laplace(curmat);
      break;
  }
  glutPostRedisplay();
}

int winWidth = 0;
int winHeight = 0;

/* used to get current width and height of viewport */
void
reshape(int wid, int ht)
{
  glViewport(0, 0, wid, ht);
  winWidth = wid;
  winHeight = ht;
}

/* ARGSUSED1 */
void key(unsigned char key, int x, int y)
{
    if(key == '\033')
        exit(0);
}


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
    glCallList(SPHERE);
    glPopMatrix();


    glPushMatrix();
    glTranslatef(-20.f, -80.f, -500.f);
    glCallList(CONE);
    glPopMatrix();

}



void
convolve(void (*draw)(void), Filter *mat)
{
  int i, j;
  int imax, jmax;

  imax = mat->cols;
  jmax = mat->rows;
  for(j = 0; j < jmax; j++) {
      for(i = 0; i < imax; i++) {
        glViewport(-i, -j, winWidth - i, winHeight - j);
        draw();
        glAccum(GL_ACCUM, mat->array[i + j * imax]);
      }
  }
}


/* Called when window needs to be redrawn */
void redraw(void)
{
    glClearAccum(curmat->bias,
                 curmat->bias,
                 curmat->bias,
                 1.0);

    glClear(GL_ACCUM_BUFFER_BIT);

    convolve(render, curmat);

    glViewport(0, 0, winWidth, winHeight);

    glAccum(GL_RETURN, curmat->scale);

    glutSwapBuffers();

    if(glGetError()) /* to catch programming errors; should never happen */
       printf("Oops! I screwed up my OpenGL calls somewhere\n");
}


const int TEXDIM = 256;

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
    (void)glutCreateWindow("accumulation buffer convolve");
    glutDisplayFunc(redraw);
    glutKeyboardFunc(key);
    glutReshapeFunc(reshape);

    /* draw a perspective scene */
    glMatrixMode(GL_PROJECTION);
    glFrustum(-100., 100., -100., 100., 320., 640.); 
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

    /* make display lists for sphere and cone; for efficiency */

    glNewList(SPHERE, GL_COMPILE);
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

    glutCreateMenu(menu);
    glutAddMenuEntry("none", CONV_NONE);
    glutAddMenuEntry("box filter (3x3 blur)", CONV_BOX_3X3);
    glutAddMenuEntry("box filter (5x5 blur)", CONV_BOX_5X5);
    glutAddMenuEntry("Sobel(x direction)", CONV_SOBEL_X);
    glutAddMenuEntry("Laplace", CONV_LAPLACE);
    glutAttachMenu(GLUT_RIGHT_BUTTON);


    /* load pattern for current 2d texture */
    tex = make_texture(TEXDIM, TEXDIM);
    glTexImage2D(GL_TEXTURE_2D, 0, 1, TEXDIM, TEXDIM, 0, GL_RED, GL_FLOAT, tex);
    free(tex);

    curmat = newfilter(1, 1);
    identity(curmat);

    glutMainLoop();
    return 0;             /* ANSI C requires main to return int. */
}
