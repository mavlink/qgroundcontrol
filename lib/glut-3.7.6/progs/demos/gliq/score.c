/*  
 *  CS 453 - Final project : An OpenGL version of the pegboard game IQ
 *  Due : June 5, 1997
 *  Author : Kiri Wagstaff
 *
 *  File : score.c
 *  Description : Maintains and adds to highscores.
 *
 */

#include "gliq.h"

int scores[10];
int totpegs[10];
char inits[10][4];
int minscore=0;
int minpegs=100;
char newinits[3];
int numentered=0;
int written=0;
float color1=1.0, color2=1.0, color3=0.0;
int lasthigh=-1;

void highscore(void);
void readscores(void);
void showhighscores(void);
void keyscores(unsigned char key, int x, int y);

void highscore(void)
{
  int i, j;
  int width = glutGet(GLUT_WINDOW_WIDTH);
  int height = glutGet(GLUT_WINDOW_HEIGHT);
  FILE* fp;
  
  /* Prompt for initials */
  glColor3f(1.0, 1.0, 0.0); /* yellow */
  text(0.08*width, 0.85*height, 0.1*height, "CONGRATULATIONS!");
  glColor3f(1.0, 0.0, 0.0); /* red */
  text(0.05*width, 0.7*height, 0.07*height, "You made it into the top 10!");
  glColor3f(1.0, 1.0, 0.0); /* yellow */
  text(0.2*width, 0.55*height, 0.07*height, "%02d remaining of %02d",
       pegs, totalpegs);
  glColor3f(0.0, 0.0, 1.0); /* blue */
  text(0.13*width, 0.4*height, 0.07*height, "Please enter your initials:");

  /* Display what's been entered */
  glColor3f(color1, color2, color3);
  for (i=0; i<numentered; i++)
    text((0.4+i/10.0)*width, 0.2*height, 0.2*height, "%c", newinits[i]);

  if (!written && numentered == 3)
    {
#if 0
//      printf("Saving to file scores.txt...\n");
#endif
      for (i=0; i<10; i++)
	if (scores[i]==-1 || 
	    (pegs<scores[i] || (pegs==scores[i] && totalpegs>totpegs[i])))
	  break;
      for (j=9; j>i; j--)
	{
	  if (scores[j-1]==-1 || scores[j-1]==0)
	    continue;
#if 0
//	  printf("compare : ");
//	  printf(" %s      %02d     %02d\n", inits[j], scores[j], totpegs[j]);
#endif
	  scores[j] = scores[j-1];
	  totpegs[j] = totpegs[j-1];
	  inits[j][0] = inits[j-1][0];
	  inits[j][1] = inits[j-1][1];
	  inits[j][2] = inits[j-1][2];
	  inits[j][3] = inits[j-1][3];
#if 0
//	  printf("with : ");
//	  printf(" %s      %02d     %02d\n", inits[j], scores[j], totpegs[j]);
#endif
	}
#if 0
//      printf("Storing in index %d\n", i);
#endif
      lasthigh=i;
      scores[i] = pegs;
      totpegs[i] = totalpegs;
      inits[i][0] = newinits[0];
      inits[i][1] = newinits[1];
      inits[i][2] = newinits[2];
      inits[i][3] = 0;

      /* get the new min */
      for (j=9; j>0; j--)
	if (scores[j]==-1 || scores[j]==0)
	  continue;
	else
	  {
	    minscore = scores[j];
	    minpegs = totpegs[j];
	    break;
	  }
#if 0
//      printf("New minscore %d, minpegs %d\n", minscore, minpegs);
#endif
      fp = fopen("scores.txt", "w");
      if (!fp)
	{
	  printf("Could not open scores.txt, exiting.\n");
	  exit(1);
	}
      for (i=0; i<10; i++)
	if (scores[i]!=-1 && scores[i]!=0)
	  fprintf(fp, "%02d  %02d  %s\n", scores[i], totpegs[i], inits[i]);
	else
	  break;
      written=1;
    }
      
}

void readscores(void)
{
  int i;
  FILE* fp;
  
  newinits[0] = 0;
  newinits[1] = 0;
  newinits[2] = 0;
  
  /* Read in the current high scores */
  fp = fopen("scores.txt", "r");
  if (!fp)
    {
      printf("Could not open scores.txt, exiting.\n");
      exit(1);
    }
  for (i=0; i<10; i++)
    {
       /* Pegs remaining */
      if ((fscanf(fp, "%d", &(scores[i])))!=1)
	{
	  scores[i] = -1;
	  break;
	}
      /* Total pegs */
      if ((fscanf(fp, "%d", &(totpegs[i])))!=1)
	{
	  totpegs[i] = -1;
	  break;
	}
      fscanf(fp, "%s", inits[i]);
#if 0
//      printf("read %s\n", inits[i]);
#endif
    }
  if (i>0)
    {
      minscore = scores[i-1];
      minpegs = totpegs[i-1];
    }
  
  if (i<10)
    {
      minscore=100;
      minpegs=0;
    }
#if 0
//  printf("Minscore is %d, minpegs is %d\n", minscore, minpegs);
#endif
}

void showhighscores(void)
{
  int i;
  int width = glutGet(GLUT_WINDOW_WIDTH);
  int height = glutGet(GLUT_WINDOW_HEIGHT);
  
  /* Display the current highs */
  glColor3f(1.0, 1.0, 0.0);  /* yellow */
  text(0.15*width, 0.9*height, 0.07*height, "Initials  Score  Out of");
  for (i=0; i<10; i++)
    {
      if (i>=1)
	glColor3f(1.0, 0.0, 0.0); /* red */
      else if (i>=5)
	glColor3f(0.0, 0.0, 1.0); /* blue */
      if (scores[i]>0)
	{
	  if (i==lasthigh)
	    glColor3f(color1, color2, color3);
	  text(0.15*width, (8.0-0.65*i)/10.0*height, 0.05*height,
	       " %s", inits[i]);
	  text(0.48*width, (8.0-0.65*i)/10.0*height, 0.05*height,
	       "%02d", scores[i]);
	  text(0.75*width, (8.0-0.65*i)/10.0*height, 0.05*height,
	       "%02d", totpegs[i]);
	}
   }
  glColor3f(color1, color2, color3);
  text(0.15*width, 0.1*height, 0.07*height, "Click to continue...");

}

/* ARGSUSED1 */
void keyscores(unsigned char key, int x, int y)
{
#if 0
  if (key == '\r') /*return*/ {

  } else if (key == '\b') /*backspace*/ {
  }
#endif
  if (numentered>=3)
    return;
  newinits[numentered] = key;
  numentered++;
#if 0
//  printf("Read a %c\n", key);
#endif
  glutPostRedisplay();
}

void idlescore(void)
{
  static int hscolor=0;

  switch(hscolor)
    {
    case 0:
      color1=1.0;
      color2=0.0;
      color3=0.0;
      hscolor++;
      break;
    case 1:
      color1=0.5;
      color2=0.5;
      color3=0.0;
      hscolor++;
      break;
    case 2:
      color1=1.0;
      color2=1.0;
      color3=0.0;
      hscolor++;
      break;
    case 3:
      color1=0.0;
      color2=1.0;
      color3=0.0;
      hscolor++;
      break;
    case 4:
      color1=0.0;
      color2=0.0;
      color3=1.0;
      hscolor++;
      break;
    case 5:
      color1=1.0;
      color2=0.0;
      color3=1.0;
      hscolor=0;
      break;
    }

  if (curstate==HIGHSC)
    highscore();
  else if (curstate==VIEWSCORES)
    showhighscores();
  else
    {
      printf("Unknown state %d, exiting\n", curstate);
      exit(1);
    }

  glutPostRedisplay();
}
