/*
 * This program is under the GNU GPL.
 * Use at your own risk.
 *
 * written by David Bucciarelli (tech.hmw@plus.it)
 *            Humanware s.r.l.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <GL/glut.h>

typedef struct {
  char *name;
  char *unit;
  void (*init)(void);
  int (*run)(int, int);
  int type;
  int numsize;
  int size[10];
} benchmark;

static int frontbuffer=1;

/***************************************************************************/

static void init_test01(void)
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(-0.5,639.5,-0.5,479.5);
  glMatrixMode(GL_MODELVIEW);

  glShadeModel(GL_FLAT);
  glDisable(GL_DEPTH_TEST);

  glClearColor(0.0,0.1,1.0,0.0);
  glClear(GL_COLOR_BUFFER_BIT);
  glColor3f(1.0,0.0,0.0);
}

/* ARGSUSED */
static int test01(int size, int num)
{
  int x,y;

  glBegin(GL_POINTS);
  for(y=0;y<num;y++)
    for(x=0;x<480;x++)
      glVertex2i(x,x);
  glEnd();

  return 480*num;
}

/***************************************************************************/

static void init_test02(void)
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(-0.5,639.5,-0.5,479.5);
  glMatrixMode(GL_MODELVIEW);

  glShadeModel(GL_SMOOTH);
  glDisable(GL_DEPTH_TEST);

  glClearColor(0.0,0.1,1.0,0.0);
  glClear(GL_COLOR_BUFFER_BIT);
}

static int test02(int size, int num)
{
  int x,y;

  glBegin(GL_LINES);
  for(y=0;y<num;y++)
    for(x=0;x<size;x++) {
      glColor3f(0.0,1.0,y/(float)num);
      glVertex2i(0,size-1);
      glColor3f(1.0,0.0,x/(float)size);
      glVertex2i(x,x);
    }
  glEnd();

  return num*size;
}

/***************************************************************************/

static void init_test03(void)
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-0.5,639.5,-0.5,479.5,1.0,-1000.0*480.0);
  glMatrixMode(GL_MODELVIEW);

  glShadeModel(GL_SMOOTH);
  glEnable(GL_DEPTH_TEST);

  glClearColor(0.0,0.1,1.0,0.0);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}

static int test03(int size, int num)
{
  int x,y,z;

  glBegin(GL_TRIANGLES);
  for(y=0;y<num;y++)
    for(x=0;x<size;x+=5) {
      z=num*size-(y*size+x);
      glColor3f(0.0,1.0,0.0);
      glVertex3i(0,x,z);

      glColor3f(1.0,0.0,x/(float)size);
      glVertex3i(size-1-x,0,z);

      glColor3f(1.0,x/(float)size,0.0);
      glVertex3i(x,size-1-x,z);
    }
  glEnd();

  return size*num/5;
}

/***************************************************************************/

static void init_test04(void)
{
  int x,y;
  GLubyte tex[128*128*3];
  GLenum gluerr;

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-0.5,639.5,-0.5,479.5,1.0,-1000.0*480.0);

  glMatrixMode(GL_MODELVIEW);

  glShadeModel(GL_SMOOTH);
  glEnable(GL_DEPTH_TEST);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

  for(y=0;y<128;y++)
    for(x=0;x<128;x++) {
      tex[(x+y*128)*3+0]=((x % (128/4)) < (128/8)) ? 255 : 0;
      tex[(x+y*128)*3+1]=((y % (128/4)) < (128/8)) ? 255 : 0;
      tex[(x+y*128)*3+2]=x;
    }

  glPixelStorei(GL_UNPACK_ALIGNMENT,1);
  if((gluerr=gluBuild2DMipmaps(GL_TEXTURE_2D,3,128,128,GL_RGB,
			       GL_UNSIGNED_BYTE,(GLvoid *)(&tex[0])))) {
    fprintf(stderr,"GLULib%s\n",gluErrorString(gluerr));
    exit(-1);
  }

  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);

  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST_MIPMAP_LINEAR);
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

  glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
  glEnable(GL_TEXTURE_2D);

  glClearColor(0.0,0.1,1.0,0.0);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}

static int test04(int size, int num)
{
  int x,y,z;

  glBegin(GL_TRIANGLES);
  for(y=0;y<num;y++)
    for(x=0;x<size;x+=5) {
      z=num*size-(y*size+x);
      glTexCoord2f(1.0,1.0);
      glColor3f(1.0,0.0,0.0);
      glVertex3i(0,x,z);

      glTexCoord2f(0.0,1.0);
      glColor3f(0.0,1.0,0.0);
      glVertex3i(size-1-x,0,z);

      glTexCoord2f(1.0,0.0);
      glColor3f(0.0,0.0,1.0);
      glVertex3i(x,size-1-x,z);
    }
  glEnd();

  return num*size/5;
}

/***************************************************************************/

static void init_test05(void)
{
  int x,y;
  GLubyte tex[128*128*3];
  GLenum gluerr;

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-0.5,639.5,-0.5,479.5,-1.0,1.0);

  glMatrixMode(GL_MODELVIEW);

  glShadeModel(GL_SMOOTH);
  glEnable(GL_DEPTH_TEST);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

  for(y=0;y<128;y++)
    for(x=0;x<128;x++) {
      tex[(x+y*128)*3+0]=((x % (128/4)) < (128/8)) ? 255 : 0;
      tex[(x+y*128)*3+1]=((y % (128/4)) < (128/8)) ? 255 : 0;
      tex[(x+y*128)*3+2]=x;
    }

  glPixelStorei(GL_UNPACK_ALIGNMENT,1);
  if((gluerr=gluBuild2DMipmaps(GL_TEXTURE_2D,3,128,128,GL_RGB,
			       GL_UNSIGNED_BYTE,(GLvoid *)(&tex[0])))) {
    fprintf(stderr,"GLULib%s\n",gluErrorString(gluerr));
    exit(-1);
  }

  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);

  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST_MIPMAP_LINEAR);
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

  glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
  glEnable(GL_TEXTURE_2D);

  glDepthFunc(GL_ALWAYS);

  glClearColor(0.0,0.1,1.0,0.0);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}

static int test05(int size, int num)
{
  int y;
  float v0[3];
  float v1[3];
  float v2[3];
  float v3[3];
  float cv0[3]={1.0,0.0,0.0};
  float cv1[3]={1.0,1.0,0.0};
  float cv2[3]={1.0,0.0,1.0};
  float cv3[3]={1.0,1.0,1.0};
  float tv0[3]={0.0,0.0};
  float tv1[3]={1.0,0.0};
  float tv2[3]={0.0,1.0};
  float tv3[3]={1.0,1.0};

  v0[0] = 320-size/2;
  v0[1] = 240-size/2;
  v0[2] = 0.0;
  v1[0] = 320+size/2;
  v1[1] = 240-size/2;
  v1[2] = 0.0;
  v2[0] = 320-size/2;
  v2[1] = 240+size/2;
  v2[2] = 0.0;
  v3[0] = 320+size/2;
  v3[1] = 240+size/2;
  v3[2] = 0.0;

  glBegin(GL_TRIANGLE_STRIP);
  for(y=0;y<num;y++) {
    glColor3fv(cv0);
    glTexCoord2fv(tv0);
    glVertex3fv(v0);

    glColor3fv(cv1);
    glTexCoord2fv(tv1);
    glVertex3fv(v1);

    glColor3fv(cv2);
    glTexCoord2fv(tv2);
    glVertex3fv(v2);

    glColor3fv(cv3);
    glTexCoord2fv(tv3);
    glVertex3fv(v3);
  }
  glEnd();

  return 4*num-2;
}

/***************************************************************************/

static void init_test06(void)
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(-0.5,639.5,-0.5,479.5);
  glMatrixMode(GL_MODELVIEW);

  glShadeModel(GL_SMOOTH);
  glEnable(GL_DEPTH_TEST);

  glClearColor(0.0,0.1,1.0,0.0);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}

/* ARGSUSED */
static int test06(int size, int num)
{
  int y;

  for(y=0;y<num;y++) {
    glClearColor(y/(float)num,0.1,1.0,0.0);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  }

  return num;
}

/***************************************************************************/

#define BMARKS_TIME 5.0

#define NUM_BMARKS 6

/* 554 ~= sqrt(640*480) */

static benchmark bmarks[NUM_BMARKS]={
  {"Simple Points","Pnts",init_test01,test01,0,0,{0,0,0,0,0,0,0,0,0,0}},
  {"Smooth Lines","Lins",init_test02,test02,1,5,{480,250,100,50,25,0,0,0,0,0}},
  {"ZSmooth Triangles","Tris",init_test03,test03,1,5,{480,250,100,50,25,0,0,0,0,0}},
  {"ZSmooth Tex Blend Triangles","Tris",init_test04,test04,1,5,{480,250,100,50,25,0,0,0,0,0}},
  {"ZSmooth Tex Blend TMesh Triangles","Tris",init_test05,test05,2,8,{400,250,100,50,25,10,5,2,0,0}},
  {"Color/Depth Buffer Clears","Clrs",init_test06,test06,3,0,{554,0,0,0,0,0,0,0,0,0}}
};

/***************************************************************************/

static void dotest0param(benchmark *bmark)
{
  float stime,etime,dtime,tottime,maxtime,mintime;
  int num,numelem,calibnum,j;

  glPushAttrib(GL_ALL_ATTRIB_BITS);
  bmark->init();

  stime=glutGet(GLUT_ELAPSED_TIME);

  dtime=0.0;
  calibnum=0;
  while(dtime<2.0) {
    bmark->run(0,1);
    glFinish();
    etime=glutGet(GLUT_ELAPSED_TIME);
    dtime=(etime-stime)/1000.0;
    calibnum++;
  }
  glPopAttrib();

  fprintf(stderr,"Elapsed time for the calibration test (%d): %f\n",calibnum,dtime);

  num=(int)((BMARKS_TIME/dtime)*calibnum);

  if(num<1)
    num=1;

  fprintf(stderr,"Selected number of benchmark iterations: %d\n",num);

  mintime=HUGE_VAL;
  maxtime=-HUGE_VAL;

  for(tottime=0.0,j=0;j<5;j++) {
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    bmark->init();

    stime=glutGet(GLUT_ELAPSED_TIME);
    numelem=bmark->run(0,num);
    glFinish();
    etime=glutGet(GLUT_ELAPSED_TIME);

    glPopAttrib();

    dtime=(etime-stime)/1000.0;
    tottime+=dtime;

    fprintf(stderr,"Elapsed time for run %d: %f\n",j,dtime);

    if(dtime<mintime)
      mintime=dtime;
    if(dtime>maxtime)
      maxtime=dtime;
  }

  tottime-=mintime+maxtime;

  fprintf(stdout,"%s\n%f %s/sec",bmark->name,numelem/(tottime/3.0),bmark->unit);

  if(bmark->type==3)
    fprintf(stdout,", MPixel Fill/sec: %f\n\n",
	    (numelem*bmark->size[0]*(float)bmark->size[0])/(1000000.0*tottime/3.0));
  else
    fprintf(stdout,"\n\n");
}

/***************************************************************************/

static void dotest1param(benchmark *bmark)
{
  float stime,etime,dtime,tottime,maxtime,mintime;
  int num,numelem,calibnum,j,k;

  fprintf(stdout,"%s\n",bmark->name);

  for(j=0;j<bmark->numsize;j++) {
    fprintf(stderr,"Current size: %d\n",bmark->size[j]);

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    bmark->init();

    stime=glutGet(GLUT_ELAPSED_TIME);

    dtime=0.0;
    calibnum=0;
    while(dtime<2.0) {
      bmark->run(bmark->size[j],1);
      glFinish();
      etime=glutGet(GLUT_ELAPSED_TIME);
      dtime=(etime-stime)/1000.0;
      calibnum++;
    }
    glPopAttrib();

    fprintf(stderr,"Elapsed time for the calibration test (%d): %f\n",calibnum,dtime);

    num=(int)((BMARKS_TIME/dtime)*calibnum);

    if(num<1)
      num=1;

    fprintf(stderr,"Selected number of benchmark iterations: %d\n",num);

    mintime=HUGE_VAL;
    maxtime=-HUGE_VAL;

    for(numelem=1,tottime=0.0,k=0;k<5;k++) {
      glPushAttrib(GL_ALL_ATTRIB_BITS);
      bmark->init();

      stime=glutGet(GLUT_ELAPSED_TIME);
      numelem=bmark->run(bmark->size[j],num);
      glFinish();
      etime=glutGet(GLUT_ELAPSED_TIME);

      glPopAttrib();

      dtime=(etime-stime)/1000.0;
      tottime+=dtime;

      fprintf(stderr,"Elapsed time for run %d: %f\n",k,dtime);

      if(dtime<mintime)
	mintime=dtime;
      if(dtime>maxtime)
	maxtime=dtime;
    }

    tottime-=mintime+maxtime;

    fprintf(stdout,"SIZE=%03d => %f %s/sec",bmark->size[j],numelem/(tottime/3.0),bmark->unit);
    if(bmark->type==2)
      fprintf(stdout,", MPixel Fill/sec: %f\n",
	      (numelem*bmark->size[j]*bmark->size[j]/2)/(1000000.0*tottime/3.0));
    else
      fprintf(stdout,"\n");
  }

  fprintf(stdout,"\n\n");
}

/***************************************************************************/

static void display(void)
{
  int i;

  if(frontbuffer)
    glDrawBuffer(GL_FRONT);
  else
    glDrawBuffer(GL_BACK);

  for(i=0;i<NUM_BMARKS;i++) {
    fprintf(stderr,"Benchmark: %d\n",i);

    switch(bmarks[i].type) {
    case 0:
    case 3:
      dotest0param(&bmarks[i]);
      break;
    case 1:
    case 2:
      dotest1param(&bmarks[i]);
      break;
    }
  }

  exit(0);
}

int main(int ac, char **av)
{
  fprintf(stderr,"GLTest v1.0\nWritten by David Bucciarelli\n");

  if(ac==2)
    frontbuffer=0;

  glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB|GLUT_DEPTH);
  glutInitWindowPosition(0,0);
  glutInitWindowSize(640,480);
  glutCreateWindow("OpenGL/Mesa Performances");
  glutDisplayFunc(display);
  glutMainLoop();

  return 0;             /* ANSI C requires main to return int. */
}
