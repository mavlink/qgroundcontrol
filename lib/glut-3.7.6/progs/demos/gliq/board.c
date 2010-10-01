/*  
 *  CS 453 - Final project : An OpenGL version of the pegboard game IQ
 *  Due : June 5, 1997
 *  Author : Kiri Wagstaff
 *
 *  File : board.c
 *  Description : Contains the board readin and selection functions.
 *
 *  
 */

#include "gliq.h"

/* functions */
void selectboard(void);
void readboards(void);
void drawboard(void);
void drawpegs(void); /* Draw all the pegs */
void drawpeg(void);  /* Draw one peg */
void displaybuttons(void);

/* globals */
int numboards, curboard;
int*** boards;
int filled[BOARDSIZE][BOARDSIZE]; /* Current state of the pegs */

/* Define the board */
GLfloat vertices[8*3] = { 
  -5.0,0.0,5.0, 
  5.0,0.0,5.0,
  5.0,0.5,5.0,
  -5.0,0.5,5.0,
  -5.0,0.0,-5.0,
  5.0,0.0,-5.0,
  5.0,0.5,-5.0,
  -5.0,0.5,-5.0
};

GLuint faces[6*4] = { 
  0,1,2,3, /*front*/
  0,3,7,4, /*left*/ 
  0,4,5,1, /*bottom*/
  1,5,6,2, /*right*/
  3,2,6,7, /*top*/
  4,7,6,5  /*back*/
};

GLfloat normals[6*3] = {
   0.0,  0.0,  1.0,
  -1.0,  0.0,  0.0,
   0.0, -1.0,  0.0,
   1.0,  0.0,  0.0,
   0.0,  1.0,  0.0,
   0.0,  0.0, -1.0,
};


void selectboard(void)
{
  int height=glutGet(GLUT_WINDOW_HEIGHT);
  int width=glutGet(GLUT_WINDOW_WIDTH);
  static float spin=0.0;

  /* Eventually make it spin */
  /* Display the buttons */
  displaybuttons();
  /* Draw the quit button */
  drawquit(7.0, 9.0, 0.4, 1.0);
  /* Quit */
  glColor3f(1.0, 1.0, 1.0);  /* white */
  /*  text(0.78*width, 0.89*height, 0.1*height, "Quit"); */
  /* Select message */
  glColor3f(0.0, 1.0, 0.0);
  text(0.3*width, 0.9*height, 0.07*height, "Select a board");

  /* Draw the total # of pegs */
  glPushMatrix();
    glColor3f(1.0, 1.0, 0.0);     /* yellow */
    glTranslatef(-7.8, 8.8, 0.0);
    drawpeg();
    text(0.1*width, 0.9*height, 0.07*height, ": %02d", totalpegs);
  glPopMatrix();
  
  /* do the trackball rotation. */
  glPushMatrix();
  /*    tbMatrix(); */
    glRotatef(45.0, 1.0, 0.0, 0.0);
    glRotatef(spin, 0.0, 1.0, 0.0);
    drawboard();
    drawpegs();
  glPopMatrix();
  spin++;
}

void readboards(void)
{
  int i, j, hole;
  FILE* fp;

  /* Read in the boards */
  fp = fopen("boards.txt", "r");
  if (!fp)
    {
      printf("Could not open boards.txt, exiting.\n");
      exit(1);
    }
  fscanf(fp, "%d", &numboards);
  boards = (int***)malloc(numboards*sizeof(int**));
  for (i=0; i<numboards; i++)
    {
      boards[i] = (int**)malloc(BOARDSIZE*sizeof(int*));
      for (j=0; j<BOARDSIZE; j++)
	boards[i][j] = (int*)malloc(BOARDSIZE*sizeof(int));
    }
  for (i=0; i<numboards; i++)
    for (j=0; j<BOARDSIZE*BOARDSIZE; j++)
      {
	fscanf(fp, "%d", &hole);
	boards[i][j/BOARDSIZE][j%BOARDSIZE] = hole;
      }
  totalpegs = 0;
  /* Set up filled array */
  for (i=0; i<BOARDSIZE; i++)
    for (j=0; j<BOARDSIZE; j++) 
      {
	filled[i][j] = boards[curboard][i][j];
	if (filled[i][j] == FULL)
	  totalpegs++;
      }

  if (numboards == 1)
    {
      curboard = 0;
      pegs = totalpegs;
      curstate = PLAY;
    }
}

void drawboard(void)
{
  int i, j;
  GLUquadricObj* hole;

  /* Draw the board */
  glColor3f(0.3, 0.3, 1.0);  /* Blue */
  glShadeModel(GL_FLAT);
  glBegin(GL_QUADS);
  for (i=0; i<6; i++) 
    {
      glNormal3fv(&normals[3*i]);
      for (j=0; j<4; j++) 
	glVertex3fv(&vertices[3*faces[i*4 + j]]);
    }
  glEnd();

  /* Draw holes */
  glShadeModel(GL_SMOOTH);
  /*  glColor3f(0.0, 0.0, 0.0); */
  glPushMatrix();
  glTranslatef(-4.0, 0.51, -4.0);
  for (i=0; i<BOARDSIZE; i++)
    {
      glPushMatrix();
      for (j=0; j<BOARDSIZE; j++)
	{
	  if (filled[i][j] == UNUSED)
	    {
	      glTranslatef(1.0, 0.0, 0.0);
	      continue;
	    }
	  glColor3f(0.3, 0.3, 1.0);  /* Blue */
	  glPushMatrix();
	    glRotatef(-90.0, 1.0, 0.0, 0.0);
	    hole = gluNewQuadric();
	    gluQuadricDrawStyle(hole, GLU_FILL);
	    gluQuadricNormals(hole, GLU_SMOOTH);
	    gluCylinder(hole, 0.3, 0.3, 0.5, 8, 1); 
	    gluDeleteQuadric(hole);
	  glPopMatrix();
	  glTranslatef(1.0, 0.0, 0.0);
	}
      glPopMatrix();
      glTranslatef(0.0, 0.0, 1.0);
    }
  glPopMatrix();
}

void drawpegs(void)
{
  int i, j;
  int name = 0;
  
  /* Draw pegs */
  glShadeModel(GL_SMOOTH);
  glColor3f(1.0, 1.0, 0.0);  /* Yellow */

  glPushMatrix();
    glTranslatef(-4.0, 0.51, -4.0);
    for (i=0; i<BOARDSIZE; i++)
      {
	glPushMatrix();
	for (j=0; j<BOARDSIZE; j++)
	  {
	    name++;
	    switch (filled[i][j])
	      {
	      case EMPTY:
		glLoadName(name);
		glDepthMask(GL_FALSE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(1.0, 1.0, 0.0, 0.0);  /* Invisible */
		drawpeg();
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
		break;
	      case UNUSED:
		glTranslatef(1.0, 0.0, 0.0);
		continue;
	      case FULL:
		glLoadName(name);
		glColor3f(1.0, 1.0, 0.0);  /* Yellow */
		if (picked == name)
		  glColor3f(1.0, 0.5, 0.0); /* Orange */
		drawpeg();
		break;
	      case CANMOVE:
		glLoadName(name);
		glColor3f(0.0, 1.0, 0.0); /* Green */
		drawpeg();
		break;
	      case CANTMOVE:
		glLoadName(name);
		glColor3f(1.0, 0.0, 0.0); /* Red */
		drawpeg();
		break;
	      default:
		printf("Unknown peg value %d, exiting.", filled[i][j]);
		exit(1);
	      }
	    glTranslatef(1.0, 0.0, 0.0);
	  }
	glPopMatrix();
	glTranslatef(0.0, 0.0, 1.0);
      }
  glPopMatrix();
}


void drawpeg(void)
{
  float ang=-90.0;
  float radcyl=0.25;
  float radball=0.4;
  float len=0.8;
  static GLuint peg=0;
  GLUquadricObj* stick;

  /* Generate the displaylist on the first call */ 
  if (peg) 
    {
      glCallList(peg);
      return;
    }

  peg = glGenLists(1);
  glNewList(peg, GL_COMPILE_AND_EXECUTE);

  /* Draw the ball */
  glPushMatrix();
    glTranslatef(0.0, len+(radball/2), 0.0);
    glutSolidSphere(radball, 8, 8);
  glPopMatrix();

  /* Draw the cone (stick) */
  /*  glColor3f(1.0, 1.0, 0.0);  Yellow */
  stick = gluNewQuadric();
  glPushMatrix();
    glRotatef(ang, 1.0, 0.0, 0.0);
    gluQuadricDrawStyle(stick, GLU_FILL);
    gluQuadricNormals(stick, GLU_SMOOTH);
    gluCylinder(stick, radcyl, radcyl, len, 8, 1); 
    gluDeleteQuadric(stick);
    /*    glutSolidCone(rad, len, 8, 8);*/
  glPopMatrix();
  glEndList();
}


void displaybuttons(void)
{
  GLUquadricObj* stick;

  /* Previous*/
  glPushMatrix();
    glLoadName(LEFTARR);
    if (picked == LEFTARR)
      glColor3f(1.0, 1.0, 1.0);  /* white */
    else
      glColor3f(0.0, 1.0, 0.0);  /* green */

    glTranslatef(-5.0, 6.5, -2.0);
    glRotatef(-90.0, 0.0, 1.0, 0.0);
    glutSolidCone(1.5, 3.0, 8, 8);
    glTranslatef(0.0, 0.0, -2.0);
    stick = gluNewQuadric();
    gluQuadricDrawStyle(stick, GLU_FILL);
    gluQuadricNormals(stick, GLU_SMOOTH);
    gluCylinder(stick, 1.0, 1.0, 2.0, 8, 1); 
    gluDeleteQuadric(stick);
  glPopMatrix();
  /* Select */
  glPushMatrix();
    glLoadName(SELECT);
    if (picked == SELECT)
      glColor3f(1.0, 1.0, 1.0);  /* white */
    else
      glColor3f(0.0, 1.0, 0.0);  /* green */

    glTranslatef(0.0, 6.5, -2.0);
    glutSolidCube(2.5);
  glPopMatrix();
  /* Next */
  glPushMatrix();
    glLoadName(RIGHTARR);
    if (picked == RIGHTARR)
      glColor3f(1.0, 1.0, 1.0);  /* white */
    else
      glColor3f(0.0, 1.0, 0.0);  /* green */

    glTranslatef(5.0, 6.5, -2.0);
    glRotatef(90.0, 0.0, 1.0, 0.0);
    glutSolidCone(1.5, 3.0, 8, 8);
    glTranslatef(0.0, 0.0, -2.0);
    stick = gluNewQuadric();
    gluQuadricDrawStyle(stick, GLU_FILL);
    gluQuadricNormals(stick, GLU_SMOOTH);
    gluCylinder(stick, 1.0, 1.0, 2.0, 8, 1); 
    gluDeleteQuadric(stick);
  glPopMatrix();

  glLoadName(0); /* stop name loading */
}  
