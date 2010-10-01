/*
 * This program is under the GNU GPL.
 * Use at your own risk.
 *
 * written by David Bucciarelli (tech.hmw@plus.it)
 *            Humanware s.r.l.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include <GL/glut.h>
#include "image.h"

#if defined(GL_VERSION_1_1)
/* Routines called directly. */
#elif defined(GL_EXT_texture_object) && defined(GL_EXT_copy_texture) && defined(GL_EXT_subtexture)
#define glBindTexture(A,B)     glBindTextureEXT(A,B)
#define glGenTextures(A,B)     glGenTexturesEXT(A,B)
#else
#define glBindTexture(A,B)
#define glGenTextures(A,B)
#endif

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define vinit(a,i,j,k) {\
  (a)[0]=i;\
  (a)[1]=j;\
  (a)[2]=k;\
}

#define vinit4(a,i,j,k,w) {\
  (a)[0]=i;\
  (a)[1]=j;\
  (a)[2]=k;\
  (a)[3]=w;\
}


#define vadds(a,dt,b) {\
  (a)[0]+=(dt)*(b)[0];\
  (a)[1]+=(dt)*(b)[1];\
  (a)[2]+=(dt)*(b)[2];\
}

#define vequ(a,b) {\
  (a)[0]=(b)[0];\
  (a)[1]=(b)[1];\
  (a)[2]=(b)[2];\
}

#define vinter(a,dt,b,c) {\
  (a)[0]=(dt)*(b)[0]+(1.0-dt)*(c)[0];\
  (a)[1]=(dt)*(b)[1]+(1.0-dt)*(c)[1];\
  (a)[2]=(dt)*(b)[2]+(1.0-dt)*(c)[2];\
}

#define clamp(a)        ((a) < 0.0 ? 0.0 : ((a) < 1.0 ? (a) : 1.0))

#define vclamp(v) {\
  (v)[0]=clamp((v)[0]);\
  (v)[1]=clamp((v)[1]);\
  (v)[2]=clamp((v)[2]);\
}

static int WIDTH=640;
static int HEIGHT=480;

#define FRAME 50
#define DIMP 20.0
#define DIMTP 16.0

#define RIDCOL 0.4

#define NUMTREE 50
#define TREEINR 2.5
#define TREEOUTR 8.0

#define AGRAV -9.8

typedef struct {
  int age;
  float p[3][3];
  float v[3];
  float c[3][4];
} part;

static float treepos[NUMTREE][3];

static float black[3]={0.0,0.0,0.0};
static float blu[3]={0.0,0.2,1.0};
static float blu2[3]={0.0,1.0,1.0};

static float fogcolor[4]={1.0,1.0,1.0,1.0};

static float q[4][3]={
  {-DIMP,0.0,-DIMP},
  {DIMP,0.0,-DIMP},
  {DIMP,0.0,DIMP},
  {-DIMP,0.0,DIMP}
};

static float qt[4][2]={
  {-DIMTP,-DIMTP},
  {DIMTP,-DIMTP},
  {DIMTP,DIMTP},
  {-DIMTP,DIMTP}
};

static int np;
static float eject_r,dt,maxage,eject_vy,eject_vl;
static short shadows;
static float ridtri;
static int fog=1;
static int help=1;
static int joyavailable=0;
static int joyactive=0;

static part *p;

static GLuint groundid;
static GLuint treeid;

static float obs[3]={2.0,1.0,0.0};
static float dir[3];
static float v=0.0;
static float alpha=-90.0;
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

float vrnd(void)
{
  return(((float)rand())/RAND_MAX);
}

static void setnewpart(part *p)
{
  float a,v[3],*c;

  p->age=0;

  a=vrnd()*3.14159265359*2.0;

  vinit(v,sin(a)*eject_r*vrnd(),0.15,cos(a)*eject_r*vrnd());
  vinit(p->p[0],v[0]+vrnd()*ridtri,v[1]+vrnd()*ridtri,v[2]+vrnd()*ridtri);
  vinit(p->p[1],v[0]+vrnd()*ridtri,v[1]+vrnd()*ridtri,v[2]+vrnd()*ridtri);
  vinit(p->p[2],v[0]+vrnd()*ridtri,v[1]+vrnd()*ridtri,v[2]+vrnd()*ridtri);

  vinit(p->v,v[0]*eject_vl/(eject_r/2),vrnd()*eject_vy+eject_vy/2,v[2]*eject_vl/(eject_r/2));

  c=blu;

  vinit4(p->c[0],c[0]*((1.0-RIDCOL)+vrnd()*RIDCOL),
	 c[1]*((1.0-RIDCOL)+vrnd()*RIDCOL),
	 c[2]*((1.0-RIDCOL)+vrnd()*RIDCOL),
	 1.0);
  vinit4(p->c[1],c[0]*((1.0-RIDCOL)+vrnd()*RIDCOL),
	 c[1]*((1.0-RIDCOL)+vrnd()*RIDCOL),
	 c[2]*((1.0-RIDCOL)+vrnd()*RIDCOL),
	 1.0);
  vinit4(p->c[2],c[0]*((1.0-RIDCOL)+vrnd()*RIDCOL),
	 c[1]*((1.0-RIDCOL)+vrnd()*RIDCOL),
	 c[2]*((1.0-RIDCOL)+vrnd()*RIDCOL),
	 1.0);
}

static void setpart(part *p)
{
  float fact;

  if(p->p[0][1]<0.1) {
    setnewpart(p);
    return;
  }

  p->v[1]+=AGRAV*dt;

  vadds(p->p[0],dt,p->v);
  vadds(p->p[1],dt,p->v);
  vadds(p->p[2],dt,p->v);

  p->age++;

  if((p->age)>maxage) {
    vequ(p->c[0],blu2);
    vequ(p->c[1],blu2);
    vequ(p->c[2],blu2);
  } else {
    fact=1.0/maxage;
    vadds(p->c[0],fact,blu2);
    vclamp(p->c[0]);
    p->c[0][3]=fact*(maxage-p->age);

    vadds(p->c[1],fact,blu2);
    vclamp(p->c[1]);
    p->c[1][3]=fact*(maxage-p->age);

    vadds(p->c[2],fact,blu2);
    vclamp(p->c[2]);
    p->c[2][3]=fact*(maxage-p->age);
  }
}

static void drawtree(float x, float y, float z)
{
  glBegin(GL_QUADS);
  glTexCoord2f(0.0,0.0);
  glVertex3f(x-1.5,y+0.0,z);

  glTexCoord2f(1.0,0.0);
  glVertex3f(x+1.5,y+0.0,z);

  glTexCoord2f(1.0,1.0);
  glVertex3f(x+1.5,y+3.0,z);

  glTexCoord2f(0.0,1.0);
  glVertex3f(x-1.5,y+3.0,z);


  glTexCoord2f(0.0,0.0);
  glVertex3f(x,y+0.0,z-1.5);

  glTexCoord2f(1.0,0.0);
  glVertex3f(x,y+0.0,z+1.5);

  glTexCoord2f(1.0,1.0);
  glVertex3f(x,y+3.0,z+1.5);

  glTexCoord2f(0.0,1.0);
  glVertex3f(x,y+3.0,z-1.5);

  glEnd();

}

static void calcposobs(void)
{
  dir[0]=sin(alpha*M_PI/180.0);
  dir[2]=cos(alpha*M_PI/180.0)*sin(beta*M_PI/180.0);
  dir[1]=cos(beta*M_PI/180.0);

  obs[0]+=v*dir[0];
  obs[1]+=v*dir[1];
  obs[2]+=v*dir[2];
}

static void printstring(void *font, char *string)
{
  int len,i;

  len=(int)strlen(string);
  for(i=0;i<len;i++)
    glutBitmapCharacter(font,string[i]);
}

static void reshape(int width, int height)
{
  WIDTH=width;
  HEIGHT=height;
  glViewport(0,0,(GLint)width,(GLint)height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(70.0,width/(float)height,0.1,30.0);

  glMatrixMode(GL_MODELVIEW);
}

static void printhelp(void)
{
  glColor4f(0.0,0.0,0.0,0.5);
  glRecti(40,40,600,440);

  glColor3f(1.0,0.0,0.0);
  glRasterPos2i(300,420);
  printstring(GLUT_BITMAP_TIMES_ROMAN_24,"Help");

  glRasterPos2i(60,390);
  printstring(GLUT_BITMAP_TIMES_ROMAN_24,"h - Togle Help");

  glRasterPos2i(60,360);
  printstring(GLUT_BITMAP_TIMES_ROMAN_24,"t - Increase particle size");
  glRasterPos2i(60,330);
  printstring(GLUT_BITMAP_TIMES_ROMAN_24,"T - Decrease particle size");

  glRasterPos2i(60,300);
  printstring(GLUT_BITMAP_TIMES_ROMAN_24,"r - Increase emission radius");
  glRasterPos2i(60,270);
  printstring(GLUT_BITMAP_TIMES_ROMAN_24,"R - Decrease emission radius");

  glRasterPos2i(60,240);
  printstring(GLUT_BITMAP_TIMES_ROMAN_24,"f - Togle Fog");
  glRasterPos2i(60,210);
  printstring(GLUT_BITMAP_TIMES_ROMAN_24,"s - Togle shadows");
  glRasterPos2i(60,180);
  printstring(GLUT_BITMAP_TIMES_ROMAN_24,"Arrow Keys - Rotate");
  glRasterPos2i(60,150);
  printstring(GLUT_BITMAP_TIMES_ROMAN_24,"a - Increase velocity");
  glRasterPos2i(60,120);
  printstring(GLUT_BITMAP_TIMES_ROMAN_24,"z - Decrease velocity");

  glRasterPos2i(60,90);
  if(joyavailable)
    printstring(GLUT_BITMAP_TIMES_ROMAN_24,"j - Togle jostick control (Joystick control available)");
  else
    printstring(GLUT_BITMAP_TIMES_ROMAN_24,"(No Joystick control available)");
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
	v+=0.01;
      if(joy.wButtons & JOY_BUTTON2)
	v-=0.01;
    }
  } else
    joyavailable=0;
#endif
}

static void drawfire(void)
{
  static int count=0;
  static char frbuf[80];
  int	j;
  float fr;

  dojoy();

  glEnable(GL_DEPTH_TEST);

  if(fog)
    glEnable(GL_FOG);
  else
    glDisable(GL_FOG);

  glDepthMask(GL_TRUE);
  glClearColor(1.0,1.0,1.0,1.0);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

  glPushMatrix();
  calcposobs();
  gluLookAt(obs[0],obs[1],obs[2],
	    obs[0]+dir[0],obs[1]+dir[1],obs[2]+dir[2],
	    0.0,1.0,0.0);

  glColor4f(1.0,1.0,1.0,1.0);

  glEnable(GL_TEXTURE_2D);

  glBindTexture(GL_TEXTURE_2D,groundid);
  glBegin(GL_QUADS);
  glTexCoord2fv(qt[0]);
  glVertex3fv(q[0]);
  glTexCoord2fv(qt[1]);
  glVertex3fv(q[1]);
  glTexCoord2fv(qt[2]);
  glVertex3fv(q[2]);
  glTexCoord2fv(qt[3]);
  glVertex3fv(q[3]);
  glEnd();

  glEnable(GL_ALPHA_TEST);
  glAlphaFunc(GL_GEQUAL,0.9);

  glBindTexture(GL_TEXTURE_2D,treeid);
  for(j=0;j<NUMTREE;j++)
    drawtree(treepos[j][0],treepos[j][1],treepos[j][2]);
  
  glDisable(GL_TEXTURE_2D);
  glDepthMask(GL_FALSE);
  glDisable(GL_ALPHA_TEST);

  if(shadows) {
    glBegin(GL_TRIANGLES);
    for(j=0;j<np;j++) {
      glColor4f(black[0],black[1],black[2],p[j].c[0][3]);
      glVertex3f(p[j].p[0][0],0.1,p[j].p[0][2]);

      glColor4f(black[0],black[1],black[2],p[j].c[1][3]);
      glVertex3f(p[j].p[1][0],0.1,p[j].p[1][2]);

      glColor4f(black[0],black[1],black[2],p[j].c[2][3]);
      glVertex3f(p[j].p[2][0],0.1,p[j].p[2][2]);
    }
    glEnd();
  }

  glBegin(GL_TRIANGLES);
  for(j=0;j<np;j++) {
    glColor4fv(p[j].c[0]);
    glVertex3fv(p[j].p[0]);

    glColor4fv(p[j].c[1]);
    glVertex3fv(p[j].p[1]);

    glColor4fv(p[j].c[2]);
    glVertex3fv(p[j].p[2]);

    setpart(&p[j]);
  }
  glEnd();

  if((count % FRAME)==0) {
    fr=gettime();
    sprintf(frbuf,"Frame rate: %f",FRAME/fr);
  }

  glDisable(GL_TEXTURE_2D);
  glDisable(GL_ALPHA_TEST);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_FOG);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-0.5,639.5,-0.5,479.5,-1.0,1.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glColor3f(1.0,0.0,0.0);
  glRasterPos2i(10,10);
  printstring(GLUT_BITMAP_HELVETICA_18,frbuf);
  glRasterPos2i(370,470);
  printstring(GLUT_BITMAP_HELVETICA_10,"Fire V1.5 Written by David Bucciarelli (tech.hmw@plus.it)");

  if(help)
    printhelp();

  reshape(WIDTH,HEIGHT);
  glPopMatrix();

  glutSwapBuffers();

  count++;
}

/* ARGSUSED1 */
static void special(int key, int x, int y)
{
  switch (key) {
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

/* ARGSUSED1 */
static void key(unsigned char key, int x, int y)
{
  switch (key) {
  case 27:
    exit(0);
    break;

  case 'a':
    v+=0.01;
    break;
  case 'z':
    v-=0.01;
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
  case 's':
    shadows= !shadows;
    break;
  case 'R':
    eject_r-=0.03;
    break;
  case 'r':
    eject_r+=0.03;
    break;
  case 't':
    ridtri+=0.005;
    break;
  case 'T':
    ridtri-=0.005;
    break;
  }
}

static void inittextures(void)
{
  IMAGE *img;
  GLenum gluerr;
  GLubyte tex[128][128][4];
  int x,y;

  glGenTextures(1,&groundid);
  glBindTexture(GL_TEXTURE_2D,groundid);

  if(!(img=ImageLoad("s128.rgb"))) {
    fprintf(stderr,"Error reading a texture.\n");
    exit(-1);
  }

  glPixelStorei(GL_UNPACK_ALIGNMENT,4);
  if((gluerr=gluBuild2DMipmaps(GL_TEXTURE_2D, 3, img->sizeX, img->sizeY, GL_RGB,
			       GL_UNSIGNED_BYTE, (GLvoid *)(img->data)))) {
    fprintf(stderr,"GLULib%s\n",gluErrorString(gluerr));
    exit(-1);
  }

  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);

  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST_MIPMAP_LINEAR);
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

  glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);

  glGenTextures(1,&treeid);
  glBindTexture(GL_TEXTURE_2D,treeid);

  if(!(img=ImageLoad("tree2.rgb"))) {
    fprintf(stderr,"Error reading a texture.\n");
    exit(-1);
  }

  for(y=0;y<128;y++)
    for(x=0;x<128;x++) {
      tex[x][y][0]=img->data[(y+x*128)*3];
      tex[x][y][1]=img->data[(y+x*128)*3+1];
      tex[x][y][2]=img->data[(y+x*128)*3+2];
      if((tex[x][y][0]==tex[x][y][1]) && (tex[x][y][1]==tex[x][y][2]) && (tex[x][y][2]==255))
	tex[x][y][3]=0;
      else
	tex[x][y][3]=255;
    }
  
  if((gluerr=gluBuild2DMipmaps(GL_TEXTURE_2D, 4, 128, 128, GL_RGBA,
			       GL_UNSIGNED_BYTE, (GLvoid *)(tex)))) {
    fprintf(stderr,"GLULib%s\n",gluErrorString(gluerr));
    exit(-1);
  }

  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);

  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST_MIPMAP_LINEAR);
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

  glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
}

static void inittree(void)
{
  int i;
  float dist;

  for(i=0;i<NUMTREE;i++)
    do {
      treepos[i][0]=vrnd()*TREEOUTR*2.0-TREEOUTR;
      treepos[i][1]=0.0;
      treepos[i][2]=vrnd()*TREEOUTR*2.0-TREEOUTR;
      dist=sqrt(treepos[i][0]*treepos[i][0]+treepos[i][2]*treepos[i][2]);
    } while((dist<TREEINR) || (dist>TREEOUTR));
}

int main(int ac,char **av)
{
  int i;

  fprintf(stderr,"Fire V1.5\nWritten by David Bucciarelli (tech.hmw@plus.it)\n");

  /* Default settings */

  WIDTH=640;
  HEIGHT=480;
  np=800;
  eject_r=0.1;
  dt=0.015;
  eject_vy=4;
  eject_vl=1;
  shadows=1;
  ridtri=0.1;

  maxage=1.0/dt;

  if(ac==2)
    np=atoi(av[1]);

  if(ac==4) {
    WIDTH=atoi(av[2]);
    HEIGHT=atoi(av[3]);
  }

  glutInitWindowPosition(0,0);
  glutInitWindowSize(WIDTH,HEIGHT);
  glutInit(&ac,av);

  glutInitDisplayMode(GLUT_RGB|GLUT_DEPTH|GLUT_DOUBLE);

  glutCreateWindow("Fire");
  
  reshape(WIDTH,HEIGHT);

  inittextures();

  glShadeModel(GL_FLAT);
  glEnable(GL_DEPTH_TEST);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

  glEnable(GL_FOG);
  glFogi(GL_FOG_MODE,GL_EXP);
  glFogfv(GL_FOG_COLOR,fogcolor);
  glFogf(GL_FOG_DENSITY,0.1);
#ifdef FX
  glHint(GL_FOG_HINT,GL_NICEST);
#endif

  p=malloc(sizeof(part)*np);

  for(i=0;i<np;i++)
    setnewpart(&p[i]);

  inittree();

  glutKeyboardFunc(key);
  glutSpecialFunc(special);
  glutDisplayFunc(drawfire);
  glutIdleFunc(drawfire);
  glutReshapeFunc(reshape);
  glutMainLoop();

  return 0;             /* ANSI C requires main to return int. */
}
