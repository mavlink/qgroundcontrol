/*
 * To compile: cc -o fractals fractals.c -lGL -lGLU -lX11 -lglut -lXmu -lm
 *
 * Usage: fractals
 *
 * Homework 6, Part 2: fractal mountains and fractal trees 
 * (Pretty Late)
 *
 * Draws fractal mountains and trees -- and an island of mountains in water 
 * (I tried having trees on the island but it didn't work too well.)
 *
 * Two viewer modes: polar and flying (both restrained to y>0 for up vector).
 * Keyboard 0->9 and +/- control speed when flying.
 *
 * Only keyboard commands are 0-9 and +/- for speed in flying mode.
 *
 * Fog would make the island look much better, but I couldn't get it to work
 * correctly.  Would line up on -z axis not from eye.
 *
 * Philip Winston - 3/4/95
 * pwinston@hmc.edu
 * http://www.cs.hmc.edu/people/pwinston
 *
 */

#include <GL/glut.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>           /* ULONG_MAX is defined here */
#include <float.h>            /* FLT_MAX is atleast defined here */

#include <time.h>  /* for random seed */

#include "fracviewer.h"

#ifdef _WIN32
#define drand48() (((float) rand())/((float) RAND_MAX))
#define srand48(x) (srand((x)))
#endif

typedef enum { NOTALLOWED, MOUNTAIN, TREE, ISLAND, BIGMTN, STEM, LEAF, 
               MOUNTAIN_MAT, WATER_MAT, LEAF_MAT, TREE_MAT, STEMANDLEAVES,
               AXES } DisplayLists;

#define MAXLEVEL 8

int Rebuild = 1,        /* Rebuild display list in next display? */
    Fract   = TREE,     /* What fractal are we building */
    Level   = 4;        /* levels of recursion for fractals */     

int DrawAxes = 0;       

/***************************************************************/
/************************* VECTOR JUNK *************************/
/***************************************************************/

  /* print vertex to stderr */
void printvert(float v[3])
{
  fprintf(stderr, "(%f, %f, %f)\n", v[0], v[1], v[2]);
}

  /* normalizes v */
void normalize(GLfloat v[3])
{
  GLfloat d = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);

  if (d == 0)
    fprintf(stderr, "Zero length vector in normalize\n");
  else
    v[0] /= d; v[1] /= d; v[2] /= d;
}

  /* calculates a normalized crossproduct to v1, v2 */
void ncrossprod(float v1[3], float v2[3], float cp[3])
{
  cp[0] = v1[1]*v2[2] - v1[2]*v2[1];
  cp[1] = v1[2]*v2[0] - v1[0]*v2[2];
  cp[2] = v1[0]*v2[1] - v1[1]*v2[0];
  normalize(cp);
}

  /* calculates normal to the triangle designated by v1, v2, v3 */
void triagnormal(float v1[3], float v2[3], float v3[3], float norm[3])
{
  float vec1[3], vec2[3];

  vec1[0] = v3[0] - v1[0];  vec2[0] = v2[0] - v1[0];
  vec1[1] = v3[1] - v1[1];  vec2[1] = v2[1] - v1[1];
  vec1[2] = v3[2] - v1[2];  vec2[2] = v2[2] - v1[2];

  ncrossprod(vec2, vec1, norm);
}

float xzlength(float v1[3], float v2[3])
{
  return sqrt((v1[0] - v2[0])*(v1[0] - v2[0]) +
              (v1[2] - v2[2])*(v1[2] - v2[2]));
}

float xzslope(float v1[3], float v2[3])
{
  return ((v1[0] != v2[0]) ? ((v1[2] - v2[2]) / (v1[0] - v2[0]))
	                   : FLT_MAX);
}


/***************************************************************/
/************************ MOUNTAIN STUFF ***********************/
/***************************************************************/

GLfloat DispFactor[MAXLEVEL];  /* Array of what to multiply random number
				  by for a given level to get midpoint
				  displacement  */
GLfloat DispBias[MAXLEVEL];  /* Array of what to add to random number
				before multiplying it by DispFactor */

#define NUMRANDS 191
float RandTable[NUMRANDS];  /* hash table of random numbers so we can
			       raise the same midpoints by the same amount */ 

         /* The following are for permitting an edge of a moutain to be   */
         /* pegged so it won't be displaced up or down.  This makes it    */
         /* easier to setup scenes and makes a single moutain look better */

GLfloat Verts[3][3],    /* Vertices of outside edges of mountain */
        Slopes[3];      /* Slopes between these outside edges */
int     Pegged[3];      /* Is this edge pegged or not */           

 /*
  * Comes up with a new table of random numbers [0,1)
  */
void InitRandTable(unsigned int seed)
{
  int i;

  srand48((long) seed);
  for (i = 0; i < NUMRANDS; i++)
    RandTable[i] = drand48() - 0.5;
}

  /* calculate midpoint and displace it if required */
void Midpoint(GLfloat mid[3], GLfloat v1[3], GLfloat v2[3],
	      int edge, int level)
{
  unsigned hash;

  mid[0] = (v1[0] + v2[0]) / 2;
  mid[1] = (v1[1] + v2[1]) / 2;
  mid[2] = (v1[2] + v2[2]) / 2;
  if (!Pegged[edge] || (fabs(xzslope(Verts[edge], mid) 
                        - Slopes[edge]) > 0.00001)) {
    srand48((int)((v1[0]+v2[0])*23344));
    hash = drand48() * 7334334;
    srand48((int)((v2[2]+v1[2])*43433));
    hash = (unsigned)(drand48() * 634344 + hash) % NUMRANDS;
    mid[1] += ((RandTable[hash] + DispBias[level]) * DispFactor[level]);
  }
}

  /*
   * Recursive moutain drawing routine -- from lecture with addition of 
   * allowing an edge to be pegged.  This function requires the above
   * globals to be set, as well as the Level global for fractal level 
   */
void FMR(GLfloat v1[3], GLfloat v2[3], GLfloat v3[3], int level)
{
  if (level == Level) {
    GLfloat norm[3];

    triagnormal(v1, v2, v3, norm);
    glNormal3fv(norm);
    glVertex3fv(v1);
    glVertex3fv(v2);
    glVertex3fv(v3);

  } else {
    GLfloat m1[3], m2[3], m3[3];

    Midpoint(m1, v1, v2, 0, level);
    Midpoint(m2, v2, v3, 1, level);
    Midpoint(m3, v3, v1, 2, level);

    FMR(v1, m1, m3, level + 1);
    FMR(m1, v2, m2, level + 1);
    FMR(m3, m2, v3, level + 1);
    FMR(m1, m2, m3, level + 1);
  }
}

 /*
  * sets up lookup tables and calls recursive mountain function
  */
void FractalMountain(GLfloat v1[3], GLfloat v2[3], GLfloat v3[3],
                     int pegged[3])
{
  GLfloat lengths[MAXLEVEL];
  GLfloat fraction[8] = { 0.3, 0.3, 0.4, 0.2, 0.3, 0.2, 0.4, 0.4  };
  GLfloat bias[8]     = { 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1  };
  int i;
  float avglen = (xzlength(v1, v2) + 
                  xzlength(v2, v3) +
		  xzlength(v3, v1) / 3);

  for (i = 0; i < 3; i++) {
    Verts[0][i] = v1[i];      /* set mountain vertex globals */
    Verts[1][i] = v2[i];
    Verts[2][i] = v3[i];
    Pegged[i] = pegged[i];
  }

  Slopes[0] = xzslope(Verts[0], Verts[1]);   /* set edge slope globals */
  Slopes[1] = xzslope(Verts[1], Verts[2]);
  Slopes[2] = xzslope(Verts[2], Verts[0]);

  lengths[0] = avglen;          
  for (i = 1; i < Level; i++) {   
    lengths[i] = lengths[i-1]/2;     /* compute edge length for each level */
  }

  for (i = 0; i < Level; i++) {     /* DispFactor and DispBias arrays */      
    DispFactor[i] = (lengths[i] * ((i <= 7) ? fraction[i] : fraction[7]));
    DispBias[i]   = ((i <= 7) ? bias[i] : bias[7]);
  } 

  glBegin(GL_TRIANGLES);
    FMR(v1, v2, v3, 0);    /* issues no GL but vertex calls */
  glEnd();
}

 /*
  * draw a mountain and build the display list
  */
void CreateMountain(void)
{
  GLfloat v1[3] = { 0, 0, -1 }, v2[3] = { -1, 0, 1 }, v3[3] = { 1, 0, 1 };
  int pegged[3] = { 1, 1, 1 };

  glNewList(MOUNTAIN, GL_COMPILE);
  glPushAttrib(GL_LIGHTING_BIT);
    glCallList(MOUNTAIN_MAT);
    FractalMountain(v1, v2, v3, pegged);
  glPopAttrib();
  glEndList();
}

  /*
   * new random numbers to make a different moutain
   */
void NewMountain(void)
{
  InitRandTable(time(NULL));
}

/***************************************************************/
/***************************** TREE ****************************/
/***************************************************************/

long TreeSeed;   /* for srand48 - remember so we can build "same tree"
                     at a different level */

 /*
  * recursive tree drawing thing, fleshed out from class notes pseudocode 
  */
void FractalTree(int level)
{
  long savedseed;  /* need to save seeds while building tree too */

  if (level == Level) {
      glPushMatrix();
        glRotatef(drand48()*180, 0, 1, 0);
        glCallList(STEMANDLEAVES);
      glPopMatrix();
  } else {
    glCallList(STEM);
    glPushMatrix();
    glRotatef(drand48()*180, 0, 1, 0);
    glTranslatef(0, 1, 0);
    glScalef(0.7, 0.7, 0.7);

      savedseed = (long) drand48()*ULONG_MAX;    /* recurse on a 3-way branching */
      glPushMatrix();    
        glRotatef(110 + drand48()*40, 0, 1, 0);
        glRotatef(30 + drand48()*20, 0, 0, 1);
        FractalTree(level + 1);
      glPopMatrix();

      srand48(savedseed);
      savedseed = (long) drand48()*ULONG_MAX;
      glPushMatrix();
        glRotatef(-130 + drand48()*40, 0, 1, 0);
        glRotatef(30 + drand48()*20, 0, 0, 1);
        FractalTree(level + 1);
      glPopMatrix();

      srand48(savedseed);
      glPushMatrix();
        glRotatef(-20 + drand48()*40, 0, 1, 0);
        glRotatef(30 + drand48()*20, 0, 0, 1);
        FractalTree(level + 1);
      glPopMatrix();

    glPopMatrix();
  }
}

  /*
   * Create display lists for a leaf, a set of leaves, and a stem
   */
void CreateTreeLists(void)
{
  GLUquadricObj *cylquad = gluNewQuadric();
  int i;

  glNewList(STEM, GL_COMPILE);
  glPushMatrix();
    glRotatef(-90, 1, 0, 0);
    gluCylinder(cylquad, 0.1, 0.08, 1, 10, 2 );
  glPopMatrix();
  glEndList();

  glNewList(LEAF, GL_COMPILE);  /* I think this was jeff allen's leaf idea */
    glBegin(GL_TRIANGLES);
      glNormal3f(-0.1, 0, 0.25);  /* not normalized */
      glVertex3f(0, 0, 0);
      glVertex3f(0.25, 0.25, 0.1);
      glVertex3f(0, 0.5, 0);

      glNormal3f(0.1, 0, 0.25);
      glVertex3f(0, 0, 0);
      glVertex3f(0, 0.5, 0);
      glVertex3f(-0.25, 0.25, 0.1);
    glEnd();
  glEndList();

  glNewList(STEMANDLEAVES, GL_COMPILE);
  glPushMatrix();
  glPushAttrib(GL_LIGHTING_BIT);
    glCallList(STEM);
    glCallList(LEAF_MAT);
    for(i = 0; i < 3; i++) {
      glTranslatef(0, 0.333, 0);
      glRotatef(90, 0, 1, 0);
      glPushMatrix();
        glRotatef(0, 0, 1, 0);
        glRotatef(50, 1, 0, 0);
        glCallList(LEAF);
      glPopMatrix();
      glPushMatrix();
        glRotatef(180, 0, 1, 0);
        glRotatef(60, 1, 0, 0);
        glCallList(LEAF);
      glPopMatrix();
    }
  glPopAttrib();
  glPopMatrix();
  glEndList();
}

 /*
  * draw and build display list for tree
  */
void CreateTree(void)
{
  srand48(TreeSeed);

  glNewList(TREE, GL_COMPILE);
    glPushMatrix();
    glPushAttrib(GL_LIGHTING_BIT);
    glCallList(TREE_MAT);
    glTranslatef(0, -1, 0);
    FractalTree(0);
    glPopAttrib();
    glPopMatrix();
  glEndList();  
}

 /*
  * new seed for a new tree (groan)
  */
void NewTree(void)
{
  TreeSeed = time(NULL);
}

/***************************************************************/
/*********************** FRACTAL PLANET ************************/
/***************************************************************/

void CreateIsland(void)
{
  CreateMountain();
  glNewList(ISLAND, GL_COMPILE);
  glPushAttrib(GL_LIGHTING_BIT);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
    glCallList(WATER_MAT);

    glBegin(GL_QUADS);
      glNormal3f(0, 1, 0);
      glVertex3f(100, 0.01, 100);
      glVertex3f(100, 0.01, -100);
      glVertex3f(-100, 0.01, -100);
      glVertex3f(-100, 0.01, 100);
    glEnd();

    glPushMatrix();
    glTranslatef(0, -0.1, 0);
    glCallList(MOUNTAIN);
    glPopMatrix();

    glPushMatrix();
    glRotatef(135, 0, 1, 0);
    glTranslatef(0.2, -0.15, -0.4);
    glCallList(MOUNTAIN);
    glPopMatrix();

    glPushMatrix();
    glRotatef(-60, 0, 1, 0);
    glTranslatef(0.7, -0.07, 0.5);
    glCallList(MOUNTAIN);
    glPopMatrix();

    glPushMatrix();
    glRotatef(-175, 0, 1, 0);
    glTranslatef(-0.7, -0.05, -0.5);
    glCallList(MOUNTAIN);
    glPopMatrix();

    glPushMatrix();
    glRotatef(165, 0, 1, 0);
    glTranslatef(-0.9, -0.12, 0.0);
    glCallList(MOUNTAIN);
    glPopMatrix();

  glPopMatrix();
  glPopAttrib();
  glEndList();  
}


void NewFractals(void)
{
  NewMountain();
  NewTree();
}

void Create(int fract)
{
  switch(fract) {
    case MOUNTAIN:
      CreateMountain();
      break;
    case TREE:
      CreateTree();
      break;
    case ISLAND:
      CreateIsland();
      break;
  }
}



/***************************************************************/
/**************************** OPENGL ***************************/
/***************************************************************/


void SetupMaterials(void)
{
  GLfloat mtn_ambuse[] =   { 0.426, 0.256, 0.108, 1.0 };
  GLfloat mtn_specular[] = { 0.394, 0.272, 0.167, 1.0 };
  GLfloat mtn_shininess[] = { 10 };

  GLfloat water_ambuse[] =   { 0.0, 0.1, 0.5, 1.0 };
  GLfloat water_specular[] = { 0.0, 0.1, 0.5, 1.0 };
  GLfloat water_shininess[] = { 10 };

  GLfloat tree_ambuse[] =   { 0.4, 0.25, 0.1, 1.0 };
  GLfloat tree_specular[] = { 0.0, 0.0, 0.0, 1.0 };
  GLfloat tree_shininess[] = { 0 };

  GLfloat leaf_ambuse[] =   { 0.0, 0.8, 0.0, 1.0 };
  GLfloat leaf_specular[] = { 0.0, 0.8, 0.0, 1.0 };
  GLfloat leaf_shininess[] = { 10 };

  glNewList(MOUNTAIN_MAT, GL_COMPILE);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mtn_ambuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mtn_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, mtn_shininess);
  glEndList();

  glNewList(WATER_MAT, GL_COMPILE);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, water_ambuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, water_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, water_shininess);
  glEndList();

  glNewList(TREE_MAT, GL_COMPILE);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, tree_ambuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, tree_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, tree_shininess);
  glEndList();

  glNewList(LEAF_MAT, GL_COMPILE);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, leaf_ambuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, leaf_specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, leaf_shininess);
  glEndList();
}

void myGLInit(void)
{
  GLfloat light_ambient[] = { 0.0, 0.0, 0.0, 1.0 };
  GLfloat light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
  GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };
  GLfloat light_position[] = { 0.0, 0.3, 0.3, 0.0 };

  GLfloat lmodel_ambient[] = { 0.4, 0.4, 0.4, 1.0 };

  glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
  glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
  glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);

  glDepthFunc(GL_LEQUAL);
  glEnable(GL_DEPTH_TEST);

  glEnable(GL_NORMALIZE);
#if 0
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
#endif

  glShadeModel(GL_SMOOTH);
#if 0
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif

  SetupMaterials();
  CreateTreeLists();

  glFlush();
} 

/***************************************************************/
/************************ GLUT STUFF ***************************/
/***************************************************************/

void reshape(GLsizei w, GLsizei h)
{
  glViewport(0,0,w,h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60.0, (GLdouble)w/h, 0.01, 100);
  glPushMatrix();
  glMatrixMode(GL_MODELVIEW);
  glFlush();
}

void display(void)
{ 
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glFlush();

  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glPushMatrix();  /* clear of last viewing xform, leaving perspective */

  agvViewTransform();

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  if (Rebuild) {
    Create(Fract);
    Rebuild = 0;
  }

  glCallList(Fract);

  if (DrawAxes)
    glCallList(AXES);

  glutSwapBuffers();
  glFlush();
}


void visible(int v)
{
  if (v == GLUT_VISIBLE)
    agvSetAllowIdle(1);
  else {
    glutIdleFunc(NULL);
    agvSetAllowIdle(0);
  }
}

void menuuse(int v)
{
  if (v == GLUT_MENU_NOT_IN_USE)
    agvSetAllowIdle(1);
  else {
    glutIdleFunc(NULL);
    agvSetAllowIdle(0);
  }
}

/***************************************************************/
/******************* MENU SETUP & HANDLING *********************/
/***************************************************************/

typedef enum { MENU_QUIT, MENU_RAND, MENU_MOVE, MENU_AXES } MenuChoices;

void setlevel(int value)
{
  Level = value;
  Rebuild = 1;
  glutPostRedisplay();
}

void choosefract(int value)
{
  Fract = value;
  Rebuild = 1;
  glutPostRedisplay();
}

void handlemenu(int value)
{
  switch (value) {
    case MENU_QUIT:
      exit(0);
      break;
    case MENU_RAND:
      NewFractals();
      Rebuild = 1;
      glutPostRedisplay();
      break;
    case MENU_AXES:
      DrawAxes = !DrawAxes;
      glutPostRedisplay();
      break;
    }
}

void MenuInit(void)
{
  int submenu3, submenu2, submenu1;

  submenu1 = glutCreateMenu(setlevel);
  glutAddMenuEntry("0", 0);  glutAddMenuEntry("1", 1);
  glutAddMenuEntry("2", 2);  glutAddMenuEntry("3", 3);
  glutAddMenuEntry("4", 4);  glutAddMenuEntry("5", 5);
  glutAddMenuEntry("6", 6);  glutAddMenuEntry("7", 7);
  glutAddMenuEntry("8", 8);

  submenu2 = glutCreateMenu(choosefract);
  glutAddMenuEntry("Moutain", MOUNTAIN);
  glutAddMenuEntry("Tree", TREE);
  glutAddMenuEntry("Island", ISLAND);

  submenu3 = glutCreateMenu(agvSwitchMoveMode);
  glutAddMenuEntry("Flying", FLYING);
  glutAddMenuEntry("Polar", POLAR);

  glutCreateMenu(handlemenu);
  glutAddSubMenu("Level", submenu1);
  glutAddSubMenu("Fractal", submenu2);
  glutAddSubMenu("Movement", submenu3);
  glutAddMenuEntry("New Fractal",      MENU_RAND);
  glutAddMenuEntry("Toggle Axes", MENU_AXES);
  glutAddMenuEntry("Quit",             MENU_QUIT);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
}


/***************************************************************/
/**************************** MAIN *****************************/
/***************************************************************/

int main(int argc, char** argv)
{
  glutInit(&argc, argv);
  glutInitWindowSize(512, 512);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE);
  glutCreateWindow("Fractal Planet?");

  agvInit(1); /* 1 cause we don't have our own idle */

  glutReshapeFunc(reshape);
  glutDisplayFunc(display);
  glutVisibilityFunc(visible);
  glutMenuStateFunc(menuuse);

  NewFractals();
  agvMakeAxesList(AXES);
  myGLInit(); 
  MenuInit();

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
