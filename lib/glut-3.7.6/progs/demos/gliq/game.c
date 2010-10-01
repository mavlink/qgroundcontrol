/*  
 *  CS 453 - Final project : An OpenGL version of the pegboard game IQ
 *  Due : June 5, 1997
 *  Author : Kiri Wagstaff
 *
 *  File : game.c
 *  Description : All the routines to actually play the game.
 *  
 */

#include "gliq.h"

int playdone=0;

void playgame(void);
void drawquit(float x, float y, float r1, float r2);
int legalmove(void);
int canmove(int peg);
int movesexist(void);

void playgame(void)
{
  int height = glutGet(GLUT_WINDOW_HEIGHT);
  int width = glutGet(GLUT_WINDOW_WIDTH);

  /* Draw the quit button */
  drawquit(7.0, 9.0, 0.4, 1.0);
  /* Quit */
#if 0
  //  glColor3f(1.0, 1.0, 1.0);  /* white */
  //  text(0.78*width, 0.88*height, 0.1*height, "Quit");
#endif
  
  /* Draw the current scores */
  /* Draw the total # of pegs */
  glPushMatrix();
    glColor3f(1.0, 1.0, 0.0);     /* yellow */
    glTranslatef(-7.8, 8.8, 0.0);
    drawpeg();
    text(0.1*width, 0.9*height, 0.07*height, ": %02d", pegs);
  glPopMatrix();

  if (playdone)
    text(0.2*glutGet(GLUT_WINDOW_WIDTH),
	 0.75*glutGet(GLUT_WINDOW_HEIGHT),
	 0.08*glutGet(GLUT_WINDOW_HEIGHT),
	 "No moves left.");

  /* do the trackball rotation. */
  glPushMatrix();
  glRotatef(45.0, 1.0, 0.0, 0.0);
  tbMatrix();

  drawboard();
  drawpegs();
  glPopMatrix();
}

int canmove(int peg)
{
  int i, j;

  if (peg == 0)
    return 0;
  
  i = (peg-1)/BOARDSIZE;
  j = (peg-1)%BOARDSIZE;
  
  if ((i-2>0) && (filled[i-1][j]==FULL) && (filled[i-2][j]==EMPTY))
    return 1;
  else if ((i+2<BOARDSIZE) &&
	   (filled[i+1][j]==FULL) && (filled[i+2][j]==EMPTY))
    return 1;
  else if ((j-2>0) && (filled[i][j-1]==FULL) && (filled[i][j-2]==EMPTY))
    return 1;
  else if ((j+2<BOARDSIZE) &&
	   (filled[i][j+1]==FULL) && (filled[i][j+2]==EMPTY))
    return 1;
  else
    return 0;
  
}

/** returns 0 if not a legal move;
 ** returns the index of the middle (jumped) peg if it was legal
 ** (1 to BOARDSIZE*BOARDSIZE)
 **/

int legalmove(void)
{
  int lasti, lastj;
  int i, j;

  if (lastpicked == 0)
    return 0;
  
  lasti = (lastpicked-1)/BOARDSIZE;
  lastj = (lastpicked-1)%BOARDSIZE;
  i = (picked-1)/BOARDSIZE;
  j = (picked-1)%BOARDSIZE;
#if 0
  //  printf("Jumping from (%d,%d) to (%d,%d)\n", lasti, lastj, i, j);
#endif
  
  if (filled[lasti][lastj] == CANMOVE && filled[i][j] == EMPTY)
    if (lasti==i+2)
      return (i+1)*BOARDSIZE+(j)+1;  /* i+1, +1 to get the name right */
    else if (lasti==i-2)
      return (i-1)*BOARDSIZE+(j)+1;  /* i-1, +1 */
    else if (lastj==j+2)
      return (i)*BOARDSIZE+(j+1)+1;  /* j+1, +1 */
    else if (lastj==j-2)
      return (i)*BOARDSIZE+(j-1)+1;  /* j-1, +1 */
    else
      return 0;
  return 0;
}

/* Checks for any legal moves remaining */
int movesexist(void)
{
  int i, j, peg;
  
  for (peg=1; peg<=BOARDSIZE*BOARDSIZE; peg++)
    {
      i = (peg-1)/BOARDSIZE;
      j = (peg-1)%BOARDSIZE;
      if (filled[i][j] == FULL && canmove(peg))
	return 1;
    }
  return 0;
}

void drawquit(float x, float y, float r1, float r2)
{
  GLUquadricObj* stick;

  glLoadName(QUIT);
#if 0
  //glDisable(GL_LIGHTING);
#endif
  glColor3f(1.0, 0.0, 0.0);  /* red */
#if 0
  //glRectf(x, y, x+w, y+h);
  //glEnable(GL_LIGHTING);
#endif

  glPushMatrix();
    glTranslatef(x, y, 0.0);
    glPushMatrix();
      stick = gluNewQuadric();
      glRotatef(90, 0.0, 1.0, 0.0);
      glRotatef(45, 1.0, 0.0, 0.0);
      glTranslatef(0.0, 0.0, r2-1.5*r1);
      gluQuadricDrawStyle(stick, GLU_FILL);
      gluQuadricNormals(stick, GLU_SMOOTH);
      gluCylinder(stick, 0.85*r1, 0.85*r1, 3*r1, 8, 1); 
      gluDeleteQuadric(stick);
    /*    glutSolidCone(rad, len, 8, 8);*/
  glPopMatrix();

  glRotatef(22.5, 0.0, 0.0, 1.0);
  glutSolidTorus(r1, r2, 8, 8);
  glPopMatrix();
    
  glLoadName(0);

}




