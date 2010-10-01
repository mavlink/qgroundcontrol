/*
 * This program is under the GNU GPL.
 * Use at your own risk.
 *
 * written by David Bucciarelli (tech.hmw@plus.it)
 *            Humanware s.r.l.
 *
 * based on a Mikael SkiZoWalker's (MoDEL) / France (Skizo@Hol.Fr) demo
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include <GL/glut.h>

#ifndef M_PI
#define M_PI 3.14159265
#endif

#define heightMnt	450
#define	lenghtXmnt	62
#define lenghtYmnt	62

#define stepXmnt     96.0
#define stepYmnt     96.0

#define WIDTH 640
#define HEIGHT 480

#define TSCALE 4

#define	FRAME 50

#define FOV 85

static GLfloat terrain[256*256];
static GLfloat terraincolor[256*256][3];

static int fog=1;
static int bfcull=1;
static int usetex=1;
static int poutline=0;
static int help=1;
static int joyavailable=0;
static int joyactive=0;
static long GlobalMnt=0;

static int scrwidth=WIDTH;
static int scrheight=HEIGHT;

#define OBSSTARTX 992.0
#define OBSSTARTY 103.0

static float obs[3]={OBSSTARTX,heightMnt*1.3,OBSSTARTY};
static float dir[3],v1[2],v2[2];
static float v=0.0;
static float alpha=75.0;
static float beta=90.0;

static float gettime(void)
{
  static clock_t told=0;
  clock_t tnew,ris;

  tnew=clock();

  ris=tnew-told;

  told=tnew;

  return(ris/(float)CLOCKS_PER_SEC);
}

static void calcposobs(void)
{
  float alpha1,alpha2;

  dir[0]=sin(alpha*M_PI/180.0);
  dir[2]=cos(alpha*M_PI/180.0)*sin(beta*M_PI/180.0);
  dir[1]=cos(beta*M_PI/180.0);

  alpha1=alpha+FOV/2.0;
  v1[0]=sin(alpha1*M_PI/180.0);
  v1[1]=cos(alpha1*M_PI/180.0);

  alpha2=alpha-FOV/2.0;
  v2[0]=sin(alpha2*M_PI/180.0);
  v2[1]=cos(alpha2*M_PI/180.0);
  
  obs[0]+=v*dir[0];
  obs[1]+=v*dir[1];
  obs[2]+=v*dir[2];

  if(obs[1]<0.0)
    obs[1]=0.0;
}

static void reshape( int width, int height )
{
  scrwidth=width;
  scrheight=height;
  glViewport(0, 0, (GLint)width, (GLint)height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(50.0, ((GLfloat) width/(GLfloat)height), lenghtXmnt*stepYmnt*0.01,
		 lenghtXmnt*stepYmnt*0.7);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

int clipstrip(float y, float *start, float *end)
{
  float x1,x2,t1,t2,tmp;

  if(v1[1]==0.0) {
    t1=0.0;
    x1=-HUGE_VAL;
  } else {
    t1=y/v1[1];
    x1=t1*v1[0];
  }

  if(v2[1]==0.0) {
    t2=0.0;
    x2=HUGE_VAL;
  } else {
    t2=y/v2[1];
    x2=t2*v2[0];
  }

  if(((x1<-(lenghtXmnt*stepXmnt)/2) && (t2<=0.0)) ||
     ((t1<=0.0) && (x2>(lenghtXmnt*stepXmnt)/2)) ||
     ((t1<0.0) && (t2<0.0)))
    return 0;

  if((t1==0.0) && (t2==0.0)) {
    if((v1[0]<0.0) && (v1[1]>0.0) && (v2[0]<0.0) && (v2[1]<0.0)) {
      *start=-(lenghtXmnt*stepXmnt)/2;
      *end=stepXmnt;
      return 1;
    } else {
      if((v1[0]>0.0) && (v1[1]<0.0) && (v2[0]>0.0) && (v2[1]>0.0)) {
	*start=-stepXmnt;
	*end=(lenghtXmnt*stepXmnt)/2;
	return 1;
      } else
	return 0;
    }
  } else {
    if(t2<0.0) {
      if(x1<0.0)
	x2=-(lenghtXmnt*stepXmnt)/2;
      else
	x2=(lenghtXmnt*stepXmnt)/2;
    }

    if(t1<0.0) {
      if(x2<0.0)
	x1=-(lenghtXmnt*stepXmnt)/2;
      else
	x1=(lenghtXmnt*stepXmnt)/2;
    }
  }

  if(x1>x2) {
    tmp=x1;
    x1=x2;
    x2=tmp;
  }

  x1-=stepXmnt;
  if(x1<-(lenghtXmnt*stepXmnt)/2)
    x1=-(lenghtXmnt*stepXmnt)/2;

  x2+=stepXmnt;
  if(x2>(lenghtXmnt*stepXmnt)/2)
    x2=(lenghtXmnt*stepXmnt)/2;	

  *start=((int)(x1/stepXmnt))*stepXmnt;
  *end=((int)(x2/stepXmnt))*stepXmnt;

  return 1;
}

static void printstring(void *font, char *string)
{
  int len,i;

  len=(int)strlen(string);
  for(i=0;i<len;i++)
    glutBitmapCharacter(font,string[i]);
}

static void printhelp(void)
{
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glColor4f(0.0,0.0,0.0,0.5);
  glRecti(40,40,600,440);
  glDisable(GL_BLEND);

  glColor3f(1.0,0.0,0.0);
  glRasterPos2i(300,420);
  printstring(GLUT_BITMAP_TIMES_ROMAN_24,"Help");

  glRasterPos2i(60,390);
  printstring(GLUT_BITMAP_TIMES_ROMAN_24,"h - Togle Help");
  glRasterPos2i(60,360);
  printstring(GLUT_BITMAP_TIMES_ROMAN_24,"t - Togle Textures");
  glRasterPos2i(60,330);
  printstring(GLUT_BITMAP_TIMES_ROMAN_24,"f - Togle Fog");
  glRasterPos2i(60,300);
  printstring(GLUT_BITMAP_TIMES_ROMAN_24,"p - Wire frame");
  glRasterPos2i(60,270);
  printstring(GLUT_BITMAP_TIMES_ROMAN_24,"b - Togle Back face culling");
  glRasterPos2i(60,240);
  printstring(GLUT_BITMAP_TIMES_ROMAN_24,"Arrow Keys - Rotate");
  glRasterPos2i(60,210);
  printstring(GLUT_BITMAP_TIMES_ROMAN_24,"a - Increase velocity");
  glRasterPos2i(60,180);
  printstring(GLUT_BITMAP_TIMES_ROMAN_24,"z - Decrease velocity");

  glRasterPos2i(60,150);
  if(joyavailable)
    printstring(GLUT_BITMAP_TIMES_ROMAN_24,"j - Togle jostick control (Joystick control available)");
  else
    printstring(GLUT_BITMAP_TIMES_ROMAN_24,"(No Joystick control available)");
}

void drawterrain(void)
{
  int h,i,idx,ox,oy;
  float j,k,start,end;

  ox=(int)(obs[0]/stepXmnt);
  oy=(int)(obs[2]/stepYmnt);
  GlobalMnt=((ox*TSCALE)&255)+((oy*TSCALE)&255)*256;

  glPushMatrix();
  glTranslatef((float)ox*stepXmnt,0,(float)oy*stepYmnt);

  for(h=0,k=-(lenghtYmnt*stepYmnt)/2;h<lenghtYmnt;k+=stepYmnt,h++) {
    if(!clipstrip(k,&start,&end))
      continue;

    glBegin(GL_TRIANGLE_STRIP); /* I hope that the optimizer will be able to improve this code */
    for(i=(int)(lenghtXmnt/2+start/stepXmnt),j=start;j<=end;j+=stepXmnt,i++) {
      idx=(i*TSCALE+h*256*TSCALE+GlobalMnt)&65535;
      glColor3fv(terraincolor[idx]);
      glTexCoord2f((ox+i)/8.0,(oy+h)/8.0);
      glVertex3f(j,terrain[idx],k);

      idx=(i*TSCALE+h*256*TSCALE+256*TSCALE+GlobalMnt)&65535; 
      glColor3fv(terraincolor[idx]);
      glTexCoord2f((ox+i)/8.0,(oy+h+1)/8.0);
      glVertex3f(j,terrain[idx],k+stepYmnt);
    }
    glEnd();
  }

  glDisable(GL_CULL_FACE);
  glDisable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glBegin(GL_QUADS);
  glColor4f(0.1,0.7,1.0,0.4);
  glVertex3f(-(lenghtXmnt*stepXmnt)/2.0,heightMnt*0.6,-(lenghtYmnt*stepYmnt)/2.0);
  glVertex3f(-(lenghtXmnt*stepXmnt)/2.0,heightMnt*0.6,(lenghtYmnt*stepYmnt)/2.0);
  glVertex3f((lenghtXmnt*stepXmnt)/2.0,heightMnt*0.6,(lenghtYmnt*stepYmnt)/2.0);
  glVertex3f((lenghtXmnt*stepXmnt)/2.0,heightMnt*0.6,-(lenghtYmnt*stepYmnt)/2.0);
  glEnd();
  glDisable(GL_BLEND);
  if(bfcull)
    glEnable(GL_CULL_FACE);
  glEnable(GL_TEXTURE_2D);

  glPopMatrix();

}

static void dojoy(void)
{
#ifdef _WIN32
  static UINT max[2]={0,0};
  static UINT min[2]={0xffffffff,0xffffffff},center[2];
  MMRESULT res;
  JOYINFO joy;

  res=joyGetPos(JOYSTICKID1,&joy);

  if(res==JOYERR_NOERROR) {
    joyavailable=1;

    if(max[0]<joy.wXpos)
      max[0]=joy.wXpos;
    if(min[0]>joy.wXpos)
      min[0]=joy.wXpos;
    center[0]=(max[0]+min[0])/2;

    if(max[1]<joy.wYpos)
      max[1]=joy.wYpos;
    if(min[1]>joy.wYpos)
      min[1]=joy.wYpos;
    center[1]=(max[1]+min[1])/2;

    if(joyactive) {
      if(fabs(center[0]-(float)joy.wXpos)>0.1*(max[0]-min[0]))
	alpha+=2.5*(center[0]-(float)joy.wXpos)/(max[0]-min[0]);
      if(fabs(center[1]-(float)joy.wYpos)>0.1*(max[1]-min[1]))
	beta+=2.5*(center[1]-(float)joy.wYpos)/(max[1]-min[1]);

      if(joy.wButtons & JOY_BUTTON1)
	v+=0.5;
      if(joy.wButtons & JOY_BUTTON2)
	v-=0.5;
    }
  } else
    joyavailable=0;
#endif
}

void drawscene(void)
{
  static int count=0;
  static char frbuf[80];
  float fr;

  dojoy();

  glShadeModel(GL_SMOOTH);
  glEnable(GL_DEPTH_TEST);

  if(usetex)
    glEnable(GL_TEXTURE_2D);
  else
    glDisable(GL_TEXTURE_2D);

  if(fog)
    glEnable(GL_FOG);
  else
    glDisable(GL_FOG);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glPushMatrix();

  calcposobs();
  gluLookAt(obs[0],obs[1],obs[2],
	    obs[0]+dir[0],obs[1]+dir[1],obs[2]+dir[2],
	    0.0,1.0,0.0);

  drawterrain();
  glPopMatrix();

  if((count % FRAME)==0) {
    fr=gettime();
    sprintf(frbuf,"Frame rate: %.3f",FRAME/fr);
  }

  glDisable(GL_TEXTURE_2D);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_FOG);
  glShadeModel(GL_FLAT);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-0.5,639.5,-0.5,479.5,-1.0,1.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glColor3f(1.0,0.0,0.0);
  glRasterPos2i(10,10);
  printstring(GLUT_BITMAP_HELVETICA_18,frbuf);
  glRasterPos2i(350,470);
  printstring(GLUT_BITMAP_HELVETICA_10,"Terrain V1.2 Written by David Bucciarelli (tech.hmw@plus.it)");
  glRasterPos2i(434,457);
  printstring(GLUT_BITMAP_HELVETICA_10,"Based on a Mickael's demo (Skizo@Hol.Fr)");

  if(help)
    printhelp();

  reshape(scrwidth,scrheight);

  glutSwapBuffers();

  count++;
}

/* ARGSUSED1 */
static void key(unsigned char k, int x, int y)
{
  switch (k) {
  case 27:
    exit(0);
    break;
  case 'a':
    v+=0.5;
    break;
  case 'z':
    v-=0.5;
    break;
  case 'p':
    if(poutline) {
      glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
      poutline=0;
    }	else {
      glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
      poutline=1;
    }
    break;
  case 'j':
    joyactive=(!joyactive);
    break;
  case 'h':
    help=(!help);
    break;
  case 'f':
    fog=(!fog);
    break;
  case 't':
    usetex=(!usetex);
    break;
  case 'b':
    if(bfcull) {
      glDisable(GL_CULL_FACE);
      bfcull=0;
    } else {
      glEnable(GL_CULL_FACE);
      bfcull=1;
    }
    break;
  }
}

/* ARGSUSED1 */
static void special(int k, int x, int y)
{
  switch(k) {
  case GLUT_KEY_LEFT:
    alpha+=2.0;
    break;
  case GLUT_KEY_RIGHT:
    alpha-=2.0;
    break;
  case GLUT_KEY_DOWN:
    beta-=2.0;
    break;
  case GLUT_KEY_UP:
    beta+=2.0;
    break;
  }
}

static void calccolor(GLfloat height, GLfloat c[3])
{
  GLfloat color[4][3]={
    {1.0,1.0,1.0},
    {0.0,0.8,0.0},
    {1.0,1.0,0.3},
    {0.0,0.0,0.8}
  };
  GLfloat fact;

  height=height*(1.0/255.0);

  if(height>=0.9) {
    c[0]=color[0][0]; c[1]=color[0][1]; c[2]=color[0][2];
    return;
  }

  if((height<0.9) && (height>=0.7)) {
    fact=(height-0.7)*5.0;
    c[0]=fact*color[0][0]+(1.0-fact)*color[1][0];
    c[1]=fact*color[0][1]+(1.0-fact)*color[1][1];
    c[2]=fact*color[0][2]+(1.0-fact)*color[1][2];
    return;
  }

  if((height<0.7) && (height>=0.6)) {
    fact=(height-0.6)*10.0;
    c[0]=fact*color[1][0]+(1.0-fact)*color[2][0];
    c[1]=fact*color[1][1]+(1.0-fact)*color[2][1];
    c[2]=fact*color[1][2]+(1.0-fact)*color[2][2];
    return;
  }

  if((height<0.6) && (height>=0.5)) {
    fact=(height-0.5)*10.0;
    c[0]=fact*color[2][0]+(1.0-fact)*color[3][0];
    c[1]=fact*color[2][1]+(1.0-fact)*color[3][1];
    c[2]=fact*color[2][2]+(1.0-fact)*color[3][2];
    return;
  }

  c[0]=color[3][0]; c[1]=color[3][1]; c[2]=color[3][2];
}

static void loadpic (void)
{
  GLubyte bufferter[256*256],terrainpic[256*256];
  FILE *FilePic;
  int i,tmp;
  GLenum gluerr;

  if((FilePic=fopen("mnt.bin","r"))==NULL) {
    fprintf(stderr,"Error loading Mnt.bin\n");
    exit(-1);
  }
  fread(bufferter , 256*256 , 1 , FilePic);
  fclose(FilePic);

  for (i=0;i<(256*256);i++) {
    terrain[i]=(bufferter[i]*(heightMnt/255.0f));
    calccolor((GLfloat)bufferter[i],terraincolor[i]);
    tmp=(((int)bufferter[i])+96);
    terrainpic[i]=(tmp>255) ? 255 : tmp;
  }

  glPixelStorei(GL_UNPACK_ALIGNMENT,1);
  if((gluerr=gluBuild2DMipmaps(GL_TEXTURE_2D, 1, 256, 256, GL_LUMINANCE,
			       GL_UNSIGNED_BYTE, (GLvoid *)(&terrainpic[0])))) {
    fprintf(stderr,"GLULib%s\n",gluErrorString(gluerr));
    exit(-1);
  }

  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);

  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST_MIPMAP_LINEAR);
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

  glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
  glEnable(GL_TEXTURE_2D);
}

static void init( void )
{
  float fogcolor[4]={0.6,0.7,0.7,1.0};

  glClearColor(fogcolor[0],fogcolor[1],fogcolor[2],fogcolor[3]);
  glClearDepth(1.0);
  glDepthFunc(GL_LEQUAL);
  glShadeModel(GL_SMOOTH);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);

  glDisable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

  glEnable(GL_FOG);
  glFogi(GL_FOG_MODE,GL_EXP2);
  glFogfv(GL_FOG_COLOR,fogcolor);
  glFogf(GL_FOG_DENSITY,0.0007);
#ifdef FX
  glHint(GL_FOG_HINT,GL_NICEST);
#endif

  reshape(scrwidth,scrheight);
}


int main(int ac, char **av)
{
  glutInitWindowPosition(0,0);
  glutInitWindowSize(WIDTH,HEIGHT);
  glutInit(&ac,av);

  glutInitDisplayMode(GLUT_RGB|GLUT_DEPTH|GLUT_DOUBLE);

  glutCreateWindow("Terrain");

  loadpic();

  init();

#ifndef FX
  glDisable(GL_TEXTURE_2D);
  usetex=0;
#endif

  glutReshapeFunc(reshape);
  glutDisplayFunc(drawscene);
  glutKeyboardFunc(key);
  glutSpecialFunc(special);
  glutIdleFunc(drawscene);

  glutMainLoop();

  return 0;             /* ANSI C requires main to return int. */
}
