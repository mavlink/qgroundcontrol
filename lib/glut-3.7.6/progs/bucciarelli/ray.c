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

#ifdef _WIN32
#include <windows.h>
#endif

#include <GL/glut.h>

#if defined(GL_VERSION_1_1)
/* Routines called directly. */
#elif defined(GL_EXT_texture_object) && defined(GL_EXT_copy_texture) && defined(GL_EXT_subtexture)
#define glBindTexture(A,B)     glBindTextureEXT(A,B)
#define glGenTextures(A,B)     glGenTexturesEXT(A,B)
#define glTexSubImage2D(A,B,C,D,E,F,G,H,I) glTexSubImage2DEXT(A,B,C,D,E,F,G,H,I)
#else
#define glBindTexture(A,B)
#define glGenTextures(A,B)
#define glTexSubImage2D(A,B,C,D,E,F,G,H,I)
#endif

static int WIDTH=640;
static int HEIGHT=480;

#define FRAME 50

#define BASESIZE 7.5f
#define SPHERE_RADIUS 0.75f

#define TEX_CHECK_WIDTH 256
#define TEX_CHECK_HEIGHT 256
#define TEX_CHECK_SLOT_SIZE (TEX_CHECK_HEIGHT/16)
#define TEX_CHECK_NUMSLOT (TEX_CHECK_HEIGHT/TEX_CHECK_SLOT_SIZE)

#define TEX_REFLECT_WIDTH 256
#define TEX_REFLECT_HEIGHT 256
#define TEX_REFLECT_SLOT_SIZE (TEX_REFLECT_HEIGHT/16)
#define TEX_REFLECT_NUMSLOT (TEX_REFLECT_HEIGHT/TEX_REFLECT_SLOT_SIZE)

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define EPSILON 0.0001

#define clamp255(a)  ( (a)<(0.0f) ? (0.0f) : ((a)>(255.0f) ? (255.0f) : (a)) )

#define fabs(x) ((x)<0.0f?-(x):(x))

#define vequ(a,b) { (a)[0]=(b)[0]; (a)[1]=(b)[1]; (a)[2]=(b)[2]; }
#define vsub(a,b,c) { (a)[0]=(b)[0]-(c)[0]; (a)[1]=(b)[1]-(c)[1]; (a)[2]=(b)[2]-(c)[2]; }
#define	dprod(a,b) ((a)[0]*(b)[0]+(a)[1]*(b)[1]+(a)[2]*(b)[2])
#define vnormalize(a,b) { \
  register float m_norm; \
  m_norm=sqrt((double)dprod((a),(a))); \
  (a)[0] /=m_norm; \
  (a)[1] /=m_norm; \
  (a)[2] /=m_norm; }

static GLubyte checkmap[TEX_CHECK_HEIGHT][TEX_CHECK_WIDTH][3];
static GLuint checkid;
static int checkmap_currentslot=0;

static GLubyte reflectmap[TEX_REFLECT_HEIGHT][TEX_REFLECT_WIDTH][3];
static GLuint reflectid;
static int reflectmap_currentslot=0;

static GLuint lightdlist;
static GLuint objdlist;

static float lightpos[3]={2.1,2.1,2.8};
static float objpos[3]={0.0,0.0,1.0};

static float sphere_pos[TEX_CHECK_HEIGHT][TEX_REFLECT_WIDTH][3];

static float fogcolor[4]={0.05,0.05,0.05,1.0};

static float obs[3]={7.0,0.0,2.0};
static float dir[3];
static float v=0.0;
static float alpha=-90.0;
static float beta=90.0;

static int fog=1;
static int bfcull=1;
static int poutline=0;
static int help=1;
static int showcheckmap=1;
static int showreflectmap=1;
static int joyavailable=0;
static int joyactive=0;

static float gettime(void)
{
  static float told=0.0f;
  float tnew,ris;

  tnew=glutGet(GLUT_ELAPSED_TIME);

  ris=tnew-told;

  told=tnew;

  return ris/1000.0;
}

static void calcposobs(void)
{
  dir[0]=sin(alpha*M_PI/180.0);
  dir[1]=cos(alpha*M_PI/180.0)*sin(beta*M_PI/180.0);
  dir[2]=cos(beta*M_PI/180.0);

  obs[0]+=v*dir[0];
  obs[1]+=v*dir[1];
  obs[2]+=v*dir[2];
}

/* ARGSUSED1 */
static void special(int k, int x, int y)
{
  switch(k) {
  case GLUT_KEY_LEFT:
    alpha-=2.0;
    break;
  case GLUT_KEY_RIGHT:
    alpha+=2.0;
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
static void key(unsigned char k, int x, int y)
{
  switch(k) {
  case 27:
    exit(0);
    break;

  case 's':
    lightpos[1]-=0.1;
    break;
  case 'd':
    lightpos[1]+=0.1;
    break;
  case 'e':
    lightpos[0]-=0.1;
    break;
  case 'x':
    lightpos[0]+=0.1;
    break;
  case 'w':
    lightpos[2]-=0.1;
    break;
  case 'r':
    lightpos[2]+=0.1;
    break;

  case 'j':
    objpos[1]-=0.1;
    break;
  case 'k':
    objpos[1]+=0.1;
    break;
  case 'i':
    objpos[0]-=0.1;
    break;
  case 'm':
    objpos[0]+=0.1;
    break;
  case 'u':
    objpos[2]-=0.1;
    break;
  case 'o':
    objpos[2]+=0.1;
    break;

  case 'a':
    v+=0.005;
    break;
  case 'z':
    v-=0.005;
    break;

  case 'g':
    joyactive=(!joyactive);
    break;
  case 'h':
    help=(!help);
    break;
  case 'f':
    fog=(!fog);
    break;

  case '1':
    showcheckmap=(!showcheckmap);
    break;
  case '2':
    showreflectmap=(!showreflectmap);
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
  case 'p':
    if(poutline) {
      glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
      poutline=0;
    }	else {
      glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
      poutline=1;
    }
    break;
  }
}

static void reshape(int w, int h) 
{
  WIDTH=w;
  HEIGHT=h;
  glViewport(0,0,w,h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(45.0,w/(float)h,0.8,40.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
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
  glColor4f(0.5,0.5,0.5,0.5);
  glRecti(40,40,600,440);
  glDisable(GL_BLEND);

  glColor3f(0.0,0.0,1.0);
  glRasterPos2i(300,420);
  printstring(GLUT_BITMAP_HELVETICA_18,"Help");

  glRasterPos2i(60,390);
  printstring(GLUT_BITMAP_HELVETICA_12,"h - Togle Help");
  glRasterPos2i(60,370);
  printstring(GLUT_BITMAP_HELVETICA_12,"f - Togle Fog");
  glRasterPos2i(60,350);
  printstring(GLUT_BITMAP_HELVETICA_12,"b - Togle Back face culling");
  glRasterPos2i(60,330);
  printstring(GLUT_BITMAP_HELVETICA_12,"p - Togle Wire frame");
  glRasterPos2i(60,310);
  printstring(GLUT_BITMAP_HELVETICA_12,"Arrow Keys - Rotate");
  glRasterPos2i(60,290);
  printstring(GLUT_BITMAP_HELVETICA_12,"a - Increase velocity");
  glRasterPos2i(60,270);
  printstring(GLUT_BITMAP_HELVETICA_12,"z - Decrease velocity");

  glRasterPos2i(60,250);
  if(joyavailable)
    printstring(GLUT_BITMAP_HELVETICA_12,"j - Togle jostick control (Joystick control available)");
  else
    printstring(GLUT_BITMAP_HELVETICA_12,"(No Joystick control available)");

  glRasterPos2i(60,230);
  printstring(GLUT_BITMAP_HELVETICA_12,"To move the light source: s - left,  d - right,  e - far,  x - near,  w - down r - up");
  glRasterPos2i(60,210);
  printstring(GLUT_BITMAP_HELVETICA_12,"To move the mirror sphere: j - left,  k - right,  i - far,  m - near,  u - down o - up");

  glRasterPos2i(60,190);
  printstring(GLUT_BITMAP_HELVETICA_12,"1 - Togle the plane texture map window");

  glRasterPos2i(60,170);
  printstring(GLUT_BITMAP_HELVETICA_12,"2 - Togle the sphere texture map window");
}

static GLboolean seelight(float p[3],float dir[3])
{
  float c[3],b,a,d,t,dist[3];

  vsub(c,p,objpos);
  b=-dprod(c,dir);
  a=dprod(c,c)-SPHERE_RADIUS*SPHERE_RADIUS;

  if((d=b*b-a)<0.0 || (b<0.0 && a>0.0))
    return GL_FALSE;

  d=sqrt(d);

  t=b-d;

  if(t<EPSILON) {
    t=b+d;
    if(t<EPSILON)
      return GL_FALSE;
  }

  vsub(dist,lightpos,p);
  if(dprod(dist,dist)<t*t)
      return GL_FALSE;

  return GL_TRUE;
}

static int colorcheckmap(float ppos[3], float c[3])
{
  static float norm[3]={0.0f,0.0f,1.0f};
  float ldir[3],vdir[3],h[3],dfact,kfact,r,g,b;
  int x,y;

  x=(int)((ppos[0]+BASESIZE/2)*(10.0f/BASESIZE));
  if((x<0) || (x>10))
    return GL_FALSE;

  y=(int)((ppos[1]+BASESIZE/2)*(10.0f/BASESIZE));
  if((y<0) || (y>10))
    return GL_FALSE;

  r=255.0f;
  if(y & 1) {
    if(x & 1)
      g=255.0f;
    else
      g=0.0f;
  } else {
    if(x & 1)
      g=0.0f;
    else
      g=255.0f;
  }
  b=0.0f;

  vsub(ldir,lightpos,ppos);
  vnormalize(ldir,ldir);

  if(seelight(ppos,ldir)) {
    c[0]=r*0.05f;
    c[1]=g*0.05f;
    c[2]=b*0.05f;

    return GL_TRUE;
  }

  dfact=dprod(ldir,norm);
  if(dfact<0.0f)
    dfact=0.0f;

  vsub(vdir,obs,ppos);
  vnormalize(vdir,vdir);
  h[0]=0.5f*(vdir[0]+ldir[0]);
  h[1]=0.5f*(vdir[1]+ldir[1]);
  h[2]=0.5f*(vdir[2]+ldir[2]);
  kfact=dprod(h,norm);
  kfact=kfact*kfact*kfact*kfact*kfact*kfact*kfact*7.0f*255.0f;

  r=r*dfact+kfact;
  g=g*dfact+kfact;
  b=b*dfact+kfact;
  
  c[0]=clamp255(r);
  c[1]=clamp255(g);
  c[2]=clamp255(b);

  return GL_TRUE;
}

static void updatecheckmap(int slot)
{
  float c[3],ppos[3];
  int x,y;

  glBindTexture(GL_TEXTURE_2D,checkid);

  ppos[2]=0.0f;
  for(y=slot*TEX_CHECK_SLOT_SIZE;y<(slot+1)*TEX_CHECK_SLOT_SIZE;y++) {
    ppos[1]=(y/(float)TEX_CHECK_HEIGHT)*BASESIZE-BASESIZE/2;

    for(x=0;x<TEX_CHECK_WIDTH;x++) {
      ppos[0]=(x/(float)TEX_CHECK_WIDTH)*BASESIZE-BASESIZE/2;

      colorcheckmap(ppos,c);
      checkmap[y][x][0]=(GLubyte)c[0];
      checkmap[y][x][1]=(GLubyte)c[1];
      checkmap[y][x][2]=(GLubyte)c[2];
    }
  }

  glTexSubImage2D(GL_TEXTURE_2D,0,0,slot*TEX_CHECK_SLOT_SIZE,TEX_CHECK_WIDTH,
		  TEX_CHECK_SLOT_SIZE,GL_RGB,GL_UNSIGNED_BYTE,
		  &checkmap[slot*TEX_CHECK_SLOT_SIZE][0][0]);

}

static void updatereflectmap(int slot)
{
  float rf,r,g,b,t,dfact,kfact,rdir[3];
  float rcol[3],ppos[3],norm[3],ldir[3],h[3],vdir[3],planepos[3];
  int x,y;

  glBindTexture(GL_TEXTURE_2D,reflectid);

  for(y=slot*TEX_REFLECT_SLOT_SIZE;y<(slot+1)*TEX_REFLECT_SLOT_SIZE;y++)
    for(x=0;x<TEX_REFLECT_WIDTH;x++) {
      ppos[0]=sphere_pos[y][x][0]+objpos[0];
      ppos[1]=sphere_pos[y][x][1]+objpos[1];
      ppos[2]=sphere_pos[y][x][2]+objpos[2];

      vsub(norm,ppos,objpos);
      vnormalize(norm,norm);

      vsub(ldir,lightpos,ppos);
      vnormalize(ldir,ldir);
      vsub(vdir,obs,ppos);
      vnormalize(vdir,vdir);

      rf=2.0f*dprod(norm,vdir);
      if(rf>EPSILON) {
	rdir[0]=rf*norm[0]-vdir[0];
	rdir[1]=rf*norm[1]-vdir[1];
	rdir[2]=rf*norm[2]-vdir[2];

	t=-objpos[2]/rdir[2];

	if(t>EPSILON) {
	  planepos[0]=objpos[0]+t*rdir[0];
	  planepos[1]=objpos[1]+t*rdir[1];
	  planepos[2]=0.0f;
	  
	  if(!colorcheckmap(planepos,rcol))
	    rcol[0]=rcol[1]=rcol[2]=0.0f;
	} else
	  rcol[0]=rcol[1]=rcol[2]=0.0f;
      } else
	rcol[0]=rcol[1]=rcol[2]=0.0f;

      dfact=0.1f*dprod(ldir,norm);

      if(dfact<0.0f) {
	dfact=0.0f;
	kfact=0.0f;
      } else {
	h[0]=0.5f*(vdir[0]+ldir[0]);
	h[1]=0.5f*(vdir[1]+ldir[1]);
	h[2]=0.5f*(vdir[2]+ldir[2]);
	kfact=dprod(h,norm);
	kfact*=kfact;
	kfact*=kfact;
	kfact*=kfact;
	kfact*=kfact;
	kfact*=10.0f;
      }

      r=dfact+kfact;
      g=dfact+kfact;
      b=dfact+kfact;

      r*=255.0f;
      g*=255.0f;
      b*=255.0f;

      r+=rcol[0];
      g+=rcol[1];
      b+=rcol[2];

      r=clamp255(r);
      g=clamp255(g);
      b=clamp255(b);

      reflectmap[y][x][0]=(GLubyte)r;
      reflectmap[y][x][1]=(GLubyte)g;
      reflectmap[y][x][2]=(GLubyte)b;
    }

  glTexSubImage2D(GL_TEXTURE_2D,0,0,slot*TEX_REFLECT_SLOT_SIZE,TEX_REFLECT_WIDTH,
		  TEX_REFLECT_SLOT_SIZE,GL_RGB,GL_UNSIGNED_BYTE,
		  &reflectmap[slot*TEX_REFLECT_SLOT_SIZE][0][0]);
}

static void drawbase(void)
{
  glColor3f(0.0,0.0,0.0);
  glBindTexture(GL_TEXTURE_2D,checkid);
  glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);

  glBegin(GL_QUADS);
  glTexCoord2f(0.0f,0.0f);
  glVertex3f(-BASESIZE/2.0f,-BASESIZE/2.0f,0.0f);

  glTexCoord2f(1.0f,0.0f);
  glVertex3f(BASESIZE/2.0f,-BASESIZE/2.0f,0.0f);

  glTexCoord2f(1.0f,1.0f);
  glVertex3f(BASESIZE/2.0f,BASESIZE/2.0f,0.0f);

  glTexCoord2f(0.0f,1.0f);
  glVertex3f(-BASESIZE/2.0f,BASESIZE/2.0f,0.0f);

  glEnd();
}

static void drawobj(void)
{
  glColor3f(0.0,0.0,0.0);
  glBindTexture(GL_TEXTURE_2D,reflectid);
  glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);

  glPushMatrix();
  glTranslatef(objpos[0],objpos[1],objpos[2]);
  glCallList(objdlist);
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
	alpha-=2.5*(center[0]-(float)joy.wXpos)/(max[0]-min[0]);
      if(fabs(center[1]-(float)joy.wYpos)>0.1*(max[1]-min[1]))
	beta+=2.5*(center[1]-(float)joy.wYpos)/(max[1]-min[1]);

      if(joy.wButtons & JOY_BUTTON1)
	v+=0.005;
      if(joy.wButtons & JOY_BUTTON2)
	v-=0.005;
    }
  } else
    joyavailable=0;
#endif
}

static void updatemaps(void)
{
  updatecheckmap(checkmap_currentslot);
  checkmap_currentslot=(checkmap_currentslot+1) % TEX_CHECK_NUMSLOT;

  updatereflectmap(reflectmap_currentslot);
  reflectmap_currentslot=(reflectmap_currentslot+1) % TEX_REFLECT_NUMSLOT;
}

static void draw(void)
{
  static int count=0;
  static char frbuf[80];
  float fr;

  dojoy();

  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

  glEnable(GL_TEXTURE_2D);
  glEnable(GL_DEPTH_TEST);
  if(fog)
    glEnable(GL_FOG);
  else
    glDisable(GL_FOG);

  glPushMatrix();
  calcposobs();

  gluLookAt(obs[0],obs[1],obs[2],
	    obs[0]+dir[0],obs[1]+dir[1],obs[2]+dir[2],
	    0.0,0.0,1.0);

  drawbase();
  drawobj();

  glColor3f(1.0,1.0,1.0);
  glDisable(GL_TEXTURE_2D);

  glPushMatrix();
  glTranslatef(lightpos[0],lightpos[1],lightpos[2]);
  glCallList(lightdlist);
  glPopMatrix();

  glPopMatrix();
  
  if((count % FRAME)==0) {
    fr=gettime();
    sprintf(frbuf,"Frame rate: %f",FRAME/fr);
  }

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_FOG);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(-0.5,639.5,-0.5,479.5,-1.0,1.0);
  glMatrixMode(GL_MODELVIEW);

  glColor3f(0.0f,0.3f,1.0f);

  if(showcheckmap) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,checkid);
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);

    glBegin(GL_QUADS);
    glTexCoord2f(1.0f,0.0f);
    glVertex2i(10,30);
    glTexCoord2f(1.0f,1.0f);
    glVertex2i(10+90,30);
    glTexCoord2f(0.0f,1.0f);
    glVertex2i(10+90,30+90);
    glTexCoord2f(0.0f,0.0f);
    glVertex2i(10,30+90);
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glBegin(GL_LINE_LOOP);
    glVertex2i(10,30);
    glVertex2i(10+90,30);
    glVertex2i(10+90,30+90);
    glVertex2i(10,30+90);
    glEnd();
    glRasterPos2i(105,65);
    printstring(GLUT_BITMAP_HELVETICA_18,"Plane Texture Map");
  }

  if(showreflectmap) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,reflectid);
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);

    glBegin(GL_QUADS);
    glTexCoord2f(1.0f,0.0f);
    glVertex2i(540,30);
    glTexCoord2f(1.0f,1.0f);
    glVertex2i(540+90,30);
    glTexCoord2f(0.0f,1.0f);
    glVertex2i(540+90,30+90);
    glTexCoord2f(0.0f,0.0f);
    glVertex2i(540,30+90);
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glBegin(GL_LINE_LOOP);
    glVertex2i(540,30);
    glVertex2i(540+90,30);
    glVertex2i(540+90,30+90);
    glVertex2i(540,30+90);
    glEnd();
    glRasterPos2i(360,65);
    printstring(GLUT_BITMAP_HELVETICA_18,"Sphere Texture Map");
  }

  glDisable(GL_TEXTURE_2D);

  glRasterPos2i(10,10);
  printstring(GLUT_BITMAP_HELVETICA_18,frbuf);
  glRasterPos2i(360,470);
  printstring(GLUT_BITMAP_HELVETICA_10,"Ray V1.0 Written by David Bucciarelli (tech.hmw@plus.it)");

  if(help)
    printhelp();

  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);

  updatemaps();

  glutSwapBuffers();

  count++;
}

static void inittextures(void)
{
  int y;

  glGenTextures(1,&checkid);
  glBindTexture(GL_TEXTURE_2D,checkid);

  for(y=0;y<TEX_CHECK_NUMSLOT;y++)
    updatecheckmap(y);

  glPixelStorei(GL_UNPACK_ALIGNMENT,1);
  glTexImage2D(GL_TEXTURE_2D,0,3,TEX_CHECK_WIDTH,TEX_CHECK_HEIGHT,
	       0,GL_RGB,GL_UNSIGNED_BYTE,checkmap);

  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
  
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

  glGenTextures(1,&reflectid);
  glBindTexture(GL_TEXTURE_2D,reflectid);

  for(y=0;y<TEX_REFLECT_NUMSLOT;y++)
    updatereflectmap(y);

  glPixelStorei(GL_UNPACK_ALIGNMENT,1);
  glTexImage2D(GL_TEXTURE_2D,0,3,TEX_REFLECT_WIDTH,TEX_REFLECT_HEIGHT,
	       0,GL_RGB,GL_UNSIGNED_BYTE,reflectmap);

  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
  
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
}

static void initspherepos(void)
{
  float alpha,beta,sa,ca,sb,cb;
  int x,y;

  for(y=0;y<TEX_REFLECT_HEIGHT;y++) {
    beta=M_PI-y*(M_PI/TEX_REFLECT_HEIGHT);

    for(x=0;x<TEX_REFLECT_WIDTH;x++) {
      alpha=-x*(2.0f*M_PI/TEX_REFLECT_WIDTH);

      sa=sin(alpha);
      ca=cos(alpha);

      sb=sin(beta);
      cb=cos(beta);

      sphere_pos[y][x][0]=SPHERE_RADIUS*sa*sb;
      sphere_pos[y][x][1]=SPHERE_RADIUS*ca*sb;
      sphere_pos[y][x][2]=SPHERE_RADIUS*cb;
    }
  }
}

static void initdlists(void)
{
  GLUquadricObj *obj;

  obj=gluNewQuadric();

  lightdlist=glGenLists(1);
  glNewList(lightdlist,GL_COMPILE);
  gluQuadricDrawStyle(obj,GLU_FILL);
  gluQuadricNormals(obj,GLU_NONE);
  gluQuadricTexture(obj,GL_TRUE);
  gluSphere(obj,0.25f,6,6);
  glEndList();

  objdlist=glGenLists(1);
  glNewList(objdlist,GL_COMPILE);
  gluQuadricDrawStyle(obj,GLU_FILL);
  gluQuadricNormals(obj,GLU_NONE);
  gluQuadricTexture(obj,GL_TRUE);
  gluSphere(obj,SPHERE_RADIUS,16,16);
  glEndList();
}

int main(int ac, char **av)
{
  fprintf(stderr,"Ray V1.0\nWritten by David Bucciarelli (tech.hmw@plus.it)\n");

  /*
    if(!SetPriorityClass(GetCurrentProcess(),REALTIME_PRIORITY_CLASS)) {
    fprintf(stderr,"Error setting the process class.\n");
    return 0;
    }

    if(!SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL)) {
    fprintf(stderr,"Error setting the process priority.\n");
    return 0;
    }
    */

  glutInitWindowPosition(0,0);
  glutInitWindowSize(WIDTH,HEIGHT);
  glutInit(&ac,av);

  glutInitDisplayMode(GLUT_RGB|GLUT_DEPTH|GLUT_DOUBLE);

  glutCreateWindow("Ray");

  reshape(WIDTH,HEIGHT);

  glShadeModel(GL_FLAT);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_CULL_FACE);
  glEnable(GL_TEXTURE_2D);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

  glEnable(GL_FOG);
  glFogi(GL_FOG_MODE,GL_EXP2);
  glFogfv(GL_FOG_COLOR,fogcolor);

  glFogf(GL_FOG_DENSITY,0.01);
#ifdef FX
  glHint(GL_FOG_HINT,GL_NICEST);
#endif

  calcposobs();

  initspherepos();

  inittextures();
  initdlists();

  glClearColor(fogcolor[0],fogcolor[1],fogcolor[2],fogcolor[3]);

  glutReshapeFunc(reshape);
  glutDisplayFunc(draw);
  glutKeyboardFunc(key);
  glutSpecialFunc(special);
  glutIdleFunc(draw);

  glutMainLoop();

  return 0;             /* ANSI C requires main to return int. */
}
