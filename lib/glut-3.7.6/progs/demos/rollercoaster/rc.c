#include <GL/glut.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
double floor(double o);
#define ACC 48

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define FOG
#define FOG_D 0.1
#define DIST 30

#define MESA_BUGGED

int rr=0;
int frame = 0;

GLfloat difmat1[4] = { 1.0, 0.4, 0.4, 1.0 };
GLfloat difamb1[4] = { 1.0, 0.4, 0.4, 1.0 };
GLfloat difmat2[4] = { 0.6, 0.6, 0.6, 1.0 };
GLfloat difamb2[4] = { 0.6, 0.6, 0.6, 1.0 };
GLfloat difmat3[4] = { 1.0, 1.0, 1.0, 1.0 };
GLfloat difamb3[4] = { 1.0, 1.0, 1.0, 1.0 };
GLfloat difmat4[4] = { 0.5, 0.5, 1.0, 1.0 };
GLfloat difamb4[4] = { 0.5, 0.5, 1.0, 1.0 };
GLfloat difmat5[4] = { 1.0, 1.0, 0.5, 1.0 };
GLfloat difamb5[4] = { 1.0, 1.0, 0.5, 1.0 };
GLfloat matspec1[4] = { 1.0, 1.0, 1.0, 0.0 };
GLfloat matspec2[4] = { 0.774, 0.774, 0.774, 1.0 };
GLfloat matspec4[4] = { 0.5, 0.5, 1.0, 1.0 };
GLfloat dif_zwart[4] = { 0.3, 0.3, 0.3, 1.0 };
GLfloat amb_zwart[4] = { 0.4, 0.4, 0.4, 1.0 };
GLfloat spc_zwart[4] = { 0.4, 0.4, 0.4, 1.0 };
GLfloat dif_copper[4] = { 0.5, 0.3, 0.1, 1.0 };
GLfloat amb_copper[4] = { 0.2, 0.1, 0.0, 1.0 };
GLfloat spc_copper[4] = { 0.3, 0.1, 0.1, 1.0 };
GLfloat fogcol[4] = { 1.0, 1.0, 1.0, 1.0 };
GLfloat hishin[1] = { 100.0 };
GLfloat loshin[1] = { 5.0 };
GLfloat lightpos[4] = { 1.0, 1.0, 1.0, 0.0 };
GLfloat lightamb[4] = { 0.2, 0.2, 0.2, 1.0 };
GLfloat lightdif[4] = { 0.8, 0.8, 0.8, 1.0 };

GLubyte texture[32][32][3];
GLubyte sky[32][32][3];

#if defined(_WIN32)
#define drand48() (((float) rand())/((float) RAND_MAX))
#endif

int rnd(int i)
{
   return (int) ((double) drand48()*i);
}

void make_texture(void)
{
    int i,j;
    GLubyte r, g, b;
    for (i=0;i<32;i++)
    {
	for (j=0;j<32;j++)
	{
	    r = 100 + rnd(156);
	    g = 100 + rnd(156);
	    b = (b+g)/2 - rnd(100);
	    texture[i][j][0] = r/2;
	    texture[i][j][1] = g/2;
	    texture[i][j][2] = b/2;
	    r = rnd(100);
	    b = rnd(100)+156;
	    sky[i][j][1] = sky[i][j][0] = r;
	    sky[i][j][2] = b;
	}
    }
}

#define MAX 10000
extern int tot;
float plaatje = 0.0;
float speed = 0;
int angle = 0;
float angle2 = 0;
float angle3 = 0;

GLfloat x[MAX], y[MAX], z[MAX];
GLfloat dx[MAX], dy[MAX], dz[MAX];
GLfloat al[MAX], rl[MAX], hd[MAX], pt[MAX];
GLfloat strips[27][MAX][3], normal[27][MAX][3], bnormal[2][MAX][3];
GLdouble cum_al = 0.0, view_al = 0.0;
int opt[MAX];
GLfloat r1[MAX], r2[MAX], r3[MAX];

void copper_texture(void)
{
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, dif_copper);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, amb_copper);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spc_copper);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 13);
}

void groen_texture(void)
{
    glMaterialfv(GL_FRONT, GL_DIFFUSE, difmat4);
    glMaterialfv(GL_FRONT, GL_AMBIENT, difamb4);
    glMaterialfv(GL_FRONT, GL_SPECULAR, matspec4);
    glMaterialf(GL_FRONT, GL_SHININESS, 5.0);
}

void rood_texture(void)
{
    glMaterialfv(GL_FRONT, GL_DIFFUSE, difmat1);
    glMaterialfv(GL_FRONT, GL_AMBIENT, difamb1);
    glMaterialfv(GL_FRONT, GL_SPECULAR, matspec1);
    glMaterialf(GL_FRONT, GL_SHININESS, 0.1*128);
}

void metaal_texture(void)
{
    glMaterialfv(GL_FRONT, GL_DIFFUSE, difmat2);
    glMaterialfv(GL_FRONT, GL_AMBIENT, difamb2);
    glMaterialfv(GL_FRONT, GL_SPECULAR, matspec2);
    glMaterialf(GL_FRONT, GL_SHININESS, 0.6*128.0);
}

void wit_texture(void)
{
    glMaterialfv(GL_FRONT, GL_DIFFUSE, difmat3);
    glMaterialfv(GL_FRONT, GL_AMBIENT, difamb3);
    glMaterialfv(GL_FRONT, GL_SPECULAR, matspec1);
    glMaterialf(GL_FRONT, GL_SHININESS, 0.8*128.0);
}

void geel_texture(void)
{
    glMaterialfv(GL_FRONT, GL_DIFFUSE, difmat5);
    glMaterialfv(GL_FRONT, GL_AMBIENT, difamb5);
    glMaterialfv(GL_FRONT, GL_SPECULAR, matspec1);
    glMaterialf(GL_FRONT, GL_SHININESS, 0.8*128.0);
}

void zwart_texture(void)
{
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, dif_zwart);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, amb_zwart);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spc_zwart);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 90);
}

#define VERTEX(I,J) glNormal3fv(normal[I][J]); glVertex3fv(strips[I][J]);

void do_display (void)
{
    int i,j,s,t, tmp;
    cum_al = 0.0;

    metaal_texture();

    for (s=0;s<24;s += 2)
    {
	t = s+2;
	if (!(t&7))
	    t=t-8;

	if (s == 16)
	    rood_texture();
	glBegin(GL_QUADS);
	for (i=0;i<tot;i=j)
	{
	    tmp = 0;
	    for (j=i+1;j<tot;j++)
		if ((tmp=tmp+tmp+opt[j]) > 4000 || (!(j%(3*DIST))))
		    break;

	    if (j>=tot)
		j = 0;

	    rr++;
	    VERTEX(s, j);
	    VERTEX(s, i);
	    VERTEX(t, i);
	    VERTEX(t, j);
	    if (!j)
		break;
	}
	glEnd();
    }
    printf("Split up to %d parts.\n", rr);
    rood_texture();
    for (i=0;i<tot-2;i+=DIST)
    {
	if (!(i%(DIST*5)))
	    continue;

	glBegin(GL_QUADS);
	glNormal3fv(bnormal[0][i]);
	glVertex3fv(strips[24][i]);
	glVertex3fv(strips[24][i+5]);
	glVertex3fv(strips[26][i+5]);
	glVertex3fv(strips[26][i]);

	glNormal3fv(bnormal[1][i]);
	glVertex3fv(strips[25][i]);
	glVertex3fv(strips[25][i+5]);
	glVertex3fv(strips[26][i+5]);
	glVertex3fv(strips[26][i]);
	glEnd();
    }

    wit_texture();
    for (i=0;i<tot-2;i+=DIST)
    {
	if (i%(DIST*5))
	    continue;

	glBegin(GL_QUADS);
	glNormal3fv(bnormal[0][i]);
	glVertex3fv(strips[24][i]);
	glVertex3fv(strips[24][i+5]);
	glVertex3fv(strips[26][i+5]);
	glVertex3fv(strips[26][i]);

	glNormal3fv(bnormal[1][i]);
	glVertex3fv(strips[25][i]);
	glVertex3fv(strips[25][i+5]);
	glVertex3fv(strips[26][i+5]);
	glVertex3fv(strips[26][i]);
	glEnd();
    }

    groen_texture();
    glBegin(GL_QUADS);
    for (i=0;i<tot;i+=90)
    {
	if (dy[i]<0.2)
	    continue;
	glNormal3f(-1.0, 0.0, 0.0);
	glVertex3f(strips[22][i][0]-0.7,-4,strips[22][i][2]-0.7);
	glVertex3f(strips[22][i][0]-0.2,strips[16][i][1],strips[22][i][2]-0.2);
	glVertex3f(strips[22][i][0]-0.2,strips[16][i][1],strips[22][i][2]+0.2);
	glVertex3f(strips[22][i][0]-0.7,-4,strips[22][i][2]+0.7);

	glNormal3f(0.0, 0.0, 1.0);
	glVertex3f(strips[22][i][0]+0.7,-4,strips[22][i][2]+0.7);
	glVertex3f(strips[22][i][0]+0.2,strips[16][i][1],strips[22][i][2]+0.2);
	glVertex3f(strips[22][i][0]-0.2,strips[16][i][1],strips[22][i][2]+0.2);
	glVertex3f(strips[22][i][0]-0.7,-4,strips[22][i][2]+0.7);

	glNormal3f(0.0, 0.0, -1.0);
	glVertex3f(strips[22][i][0]+0.7,-4,strips[22][i][2]-0.7);
	glVertex3f(strips[22][i][0]+0.2,strips[16][i][1],strips[22][i][2]-0.2);
	glVertex3f(strips[22][i][0]-0.2,strips[16][i][1],strips[22][i][2]-0.2);
	glVertex3f(strips[22][i][0]-0.7,-4,strips[22][i][2]-0.7);

	glNormal3f(1.0, 0.0, 0.0);
	glVertex3f(strips[22][i][0]+0.7,-4,strips[22][i][2]-0.7);
	glVertex3f(strips[22][i][0]+0.2,strips[16][i][1],strips[22][i][2]-0.2);
	glVertex3f(strips[22][i][0]+0.2,strips[16][i][1],strips[22][i][2]+0.2);
	glVertex3f(strips[22][i][0]+0.7,-4,strips[22][i][2]+0.7);
    }
    glEnd();
}

void do_one_wheel(void)
{
    float a,p,q;
    int i;
    copper_texture();
    glBegin(GL_QUAD_STRIP);
    for (i=0;i<=ACC;i++)
    {
	a = i*M_PI*2/ACC;
	p = cos(a);
	q = sin(a);
	glNormal3f(p, q, 0.4);
	glVertex3f(0.7*p, 0.7*q, 0.8);
	glNormal3f(0.0, 0.0, 1.0);
	glVertex3f(0.8*p, 0.8*q, 0.7);
    }
    glEnd();

    glBegin(GL_QUAD_STRIP);
    for (i=0;i<=ACC;i++)
    {
	a = i*M_PI*2/ACC;
	p = cos(a);
	q = sin(a);
	glNormal3f(0.0, 0.0, 1.0);
	glVertex3f(0.7*p, 0.7*q, 0.8);
	glVertex3f(0.6*p, 0.6*q, 0.8);
    }
    glEnd();

    glBegin(GL_QUAD_STRIP);
    for (i=0;i<=ACC;i++)
    {
	a = i*M_PI*2/ACC;
	p = cos(a);
	q = sin(a);
	glNormal3f(-p, -q, 0.0);
	glVertex3f(0.6*p, 0.6*q, 0.8);
	glVertex3f(0.6*p, 0.6*q, 0.7);
    }
    glEnd();

    glBegin(GL_QUADS);

    for (i=0;i<=12;i++)
    {
	a = i*M_PI/6;
	p = cos(a);
	q = sin(a);
	glNormal3f(p, q, 0.0);
	glVertex3f(0.65*p + 0.08*q, 0.65*q + 0.08*p, 0.75);
	glVertex3f(0.65*p - 0.08*q, 0.65*q - 0.08*p, 0.75);
	glVertex3f(-0.08*q, -0.08*p, 0.95);
	glVertex3f(0.08*q, 0.08*p, 0.95);
	if (!i)
	    rood_texture();
    }

    zwart_texture();
    glNormal3f(0.0, 1.0, 0.0);
    glVertex3f(0.1, 0.0, 0.8);
    glVertex3f(-0.1, 0.0, 0.8);
    glVertex3f(-0.1, 0.0, -0.8);
    glVertex3f(0.1, 0.0, -0.8);

    glNormal3f(1.0, 0.0, 0.0);
    glVertex3f(0.0, 0.1, 0.8);
    glVertex3f(0.0, -0.1, 0.8);
    glVertex3f(0.0, -0.1, -0.8);
    glVertex3f(0.0, 0.1, -0.8);

    glEnd();
}

void init_wheel(void)
{
    glNewList(2, GL_COMPILE);
    do_one_wheel();
    glRotatef(180.0, 0.0, 1.0, 0.0);
    do_one_wheel();
    glRotatef(180.0, 0.0, 1.0, 0.0);
    glEndList();
}

void display_wheel(float w)
{
    int ww = w;
    glPopMatrix();
    glPushMatrix();
    glTranslatef(x[ww], y[ww], z[ww]);
    glRotatef(r3[ww]*180/M_PI, 0.0, 0.0, 1.0);
    glRotatef(-r2[ww]*180/M_PI, 0.0, 1.0, 0.0);
    glRotatef(r1[ww]*180/M_PI, 1.0, 0.0, 0.0);
    glTranslatef(-0.15*(w-ww), 0.8, 0.0);
    glRotatef(-w, 0.0, 0.0, 1.0);
    glCallList(2);
    glRotatef(w, 0.0, 0.0, 1.0);
    glTranslatef(0.0, -0.8, 0.0);
    zwart_texture();
    glBegin(GL_QUADS);
    glNormal3f(0.0, 1.0, 0.0);
    glVertex3f(-0.3, 1.1, 0.3);
    glVertex3f(1.0, 1.1, 0.3);
    glVertex3f(1.0, 1.1, -0.3);
    glVertex3f(0.3, 1.1, -0.3);
    glEnd();
}

void display_cart(float w)
{
    display_wheel(w);
    geel_texture();
    glEnable(GL_AUTO_NORMAL);
    glEnable(GL_NORMALIZE);
    glBegin(GL_QUADS);
    glNormal3f(0.0, 1.0, 0.0);
    glVertex3f(0.5, 0.8, -0.70);
    glVertex3f(0.5, 0.8, 0.70);
    glVertex3f(-2.0, 0.8, 0.70);
    glVertex3f(-2.0, 0.8, -0.70);

    glNormal3f(1.0, 0.0, 0.0);
    glVertex3f(-2.0, 0.8, -0.70);
    glVertex3f(-2.0, 0.8, 0.70);
    glVertex3f(-2.0, 2.3, 0.70);
    glVertex3f(-2.0, 2.3, -0.70);

    glNormal3f(0.71, 0.71, 0.0);
    glVertex3f(-2.0, 2.3, -0.70);
    glVertex3f(-2.0, 2.3, 0.70);
    glVertex3f(-1.7, 2.0, 0.70);
    glVertex3f(-1.7, 2.0, -0.70);

    glNormal3f(0.12, 0.03, 0.0);
    glVertex3f(-1.7, 2.0, -0.70);
    glVertex3f(-1.7, 2.0, 0.70);
    glVertex3f(-1.4, 0.8, 0.70);
    glVertex3f(-1.4, 0.8, -0.70);

    glNormal3f(0.0, 1.0, 0.0);
    glVertex3f(-1.4, 0.8, -0.70);
    glVertex3f(-1.4, 0.8, 0.70);
    glVertex3f(0.0, 0.8, 0.70);
    glVertex3f(0.0, 0.8, -0.70);

    glNormal3f(1.0, 0.0, 0.0);
    glVertex3f(0.0, 0.8, -0.70);
    glVertex3f(0.0, 0.8, 0.70);
    glVertex3f(0.0, 1.5, 0.70);
    glVertex3f(0.0, 1.5, -0.70);

    glNormal3f(0.5, 0.3, 0.0);
    glVertex3f(0.0, 1.5, -0.70);
    glVertex3f(0.0, 1.5, 0.70);
    glVertex3f(-0.5, 1.8, 0.70);
    glVertex3f(-0.5, 1.8, -0.70);

    glNormal3f(1.0, 0.0, 0.0);
    glVertex3f(-0.5, 1.8, -0.70);
    glVertex3f(-0.5, 1.8, 0.70);
    glVertex3f(-0.5, 0.8, 0.70);
    glVertex3f(-0.5, 0.8, -0.70);

    zwart_texture();
    glNormal3f(0.0, 0.0, 1.0);
    glVertex3f(-1.8, 0.8, 0.70);
    glVertex3f(-1.8, 1.3, 0.70);
    glVertex3f(0.0, 1.3, 0.70);
    glVertex3f(0.0, 0.8, 0.70);

    glVertex3f(-1.8, 0.8, -0.70);
    glVertex3f(-1.8, 1.6, -0.70);
    glVertex3f(0.0, 1.4, -0.70);
    glVertex3f(0.0, 0.8, -0.70);

    glVertex3f(-2.0, 0.8, 0.70);
    glVertex3f(-2.0, 2.3, 0.70);
    glVertex3f(-1.7, 2.0, 0.70);
    glVertex3f(-1.4, 0.8, 0.70);

    glVertex3f(-2.0, 0.8, -0.70);
    glVertex3f(-2.0, 2.3, -0.70);
    glVertex3f(-1.7, 2.0, -0.70);
    glVertex3f(-1.4, 0.8, -0.70);

    glVertex3f(0.0, 0.8, -0.70);
    glVertex3f(0.0, 1.5, -0.70);
    glVertex3f(-0.5, 1.8, -0.70);
    glVertex3f(-0.5, 0.8, -0.70);

    glVertex3f(0.0, 0.8, 0.70);
    glVertex3f(0.0, 1.5, 0.70);
    glVertex3f(-0.5, 1.8, 0.70);
    glVertex3f(-0.5, 0.8, 0.70);
    glEnd();
}

void display(void)
{
    int plaatje2, l;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();
    glCallList(1);

    glPopMatrix();
    glPushMatrix();

    glEnable(GL_TEXTURE_2D);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, 32, 32, 0, GL_RGB, GL_UNSIGNED_BYTE,
	&texture[0][0][0]);

    glBegin(GL_QUADS);
    glNormal3f(0.0, 1.0, 0.0);
    glTexCoord2f(0.0, 0.0); glVertex3f(-120, -4.1, -120);
    glTexCoord2f(0.0, 1.0); glVertex3f(-120, -4.1, 120);
    glTexCoord2f(1.0, 1.0); glVertex3f(120, -4.1, 120);
    glTexCoord2f(1.0, 0.0); glVertex3f(120, -4.1, -120);
    glEnd();
    glDisable(GL_TEXTURE_2D);

/* The sky moves with us to give a perception of being infinitely far away */

    groen_texture();
    l = plaatje;
    glBegin(GL_QUADS);
    glNormal3f(0.0, 1.0, 0.0);
    glTexCoord2f(0.0, 0.0);
    glVertex3f(-400+x[l], 21+y[l], -400+z[l]);
    glTexCoord2f(0.0, 1.0);
    glVertex3f(-400+x[l], 21+y[l], 400+z[l]);
    glTexCoord2f(1.0, 1.0);
    glVertex3f(400+x[l], 21+y[l], 400+z[l]);
    glTexCoord2f(1.0, 0.0);
    glVertex3f(400+x[l], 21+y[l], -400+z[l]);
    glEnd();

    display_cart(plaatje);

    plaatje2 = plaatje + 40;
    if (plaatje2 >= tot)
	plaatje2 -= tot;
    display_cart(plaatje2);

    plaatje2 = plaatje + 20;
    if (plaatje2 >= tot)
	plaatje2 -= tot;
    display_cart(plaatje2);

    plaatje2 = plaatje - 20;
    if (plaatje2 < 0)
	plaatje2 += tot;
    display_wheel(plaatje2);

    glFlush();
    glutSwapBuffers();
    glPopMatrix();
}

void myinit (void) {
    glShadeModel (GL_SMOOTH);
    glFrontFace(GL_CCW);
    glEnable(GL_DEPTH_TEST);

    glClearColor(fogcol[0], fogcol[1], fogcol[2], fogcol[3]);
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightamb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightdif);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glColor3f(1.0, 1.0, 1.0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

#ifdef FOG
/* fog */
    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogfv(GL_FOG_COLOR, fogcol);
    glFogf(GL_FOG_DENSITY, 0.01);
    glFogf(GL_FOG_START, 0.01);
    glFogf(GL_FOG_END, 55.0);
    glHint(GL_FOG_HINT, GL_NICEST);
#endif

    make_texture();
    init_wheel();
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
/*
    glTexImage2D(GL_TEXTURE_2D, 0, 3, 32, 32, 0, GL_RGB, GL_UNSIGNED_BYTE,
	&texture[0][0][0]);
*/
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
}

/* ARGSUSED1 */
void parsekey(unsigned char key, int x, int y)
{
    switch (key)
    {
	case 27: exit(0);
	case 13: speed = 0; break;
    }
}

/* ARGSUSED1 */
void parsekey_special(int key, int x, int y)
{
    switch (key)
    {
	case GLUT_KEY_UP:	speed ++; break;
	case GLUT_KEY_DOWN:	speed --; break;
	case GLUT_KEY_RIGHT:	angle ++; break;
	case GLUT_KEY_LEFT:	angle --; break;
    }
}

void SetCamera(void)
{
    float plaatje2;
    int l,l2;
    l = plaatje;
    plaatje2 = plaatje + 10;

    if (plaatje2 >= tot)
	plaatje2 -= tot;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum (-0.1, 0.1, -0.1, 0.1, 0.1, 550.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

#if 1
    glTranslated(0.0, +0.4, 0.0);
    glRotated(angle*5.0, 0.0, 1.0, 0.0);
    glTranslatef(0.0, 0.0, -6.5-2*sin(angle2)-sin(angle3));
    glRotatef(45 - 35*cos(angle3), 1.0, 0.0, 0.0);
    glRotatef(-100*sin(angle3), 0.0, 1.0, 0.0);
#else
    glRotatef(100*sin(angle3), 0.0, 1.0, 0.0);
    glRotatef(-45 + 35*cos(angle3), 1.0, 0.0, 0.0);
    glTranslatef(0.0, 0.0, +6+2*sin(angle2)+sin(angle3));
    glRotated(-angle*5.0, 0.0, 1.0, 0.0);
    glTranslated(0.0, -0.4, 0.0);
#endif

    l2 = plaatje2;
    glTranslatef(-0.15*(plaatje-l), 0.0, 0.0);

    gluLookAt(x[l], y[l], z[l],
	x[l2], y[l2], z[l2],
	dx[l], dy[l], dz[l]);
}

void Animate(void)
{
    int l1;

    l1 = plaatje;
    speed += (y[l1] - y[l1+4])*2-0.005;

    speed -= (fabs(rl[l1]-al[l1]) + fabs(pt[l1]) + fabs(hd[l1])) * speed/200;

    if (frame > 450)
	speed -= 0.2208;

    if (speed < 0)
	speed = 0;

    if (frame < 10)
	speed = 0;

    if (frame == 10)
	speed = 4.5;

    if (frame > 155 && frame < 195)
	speed = 7.72;

    plaatje += speed;

    if (plaatje >= tot) {
	plaatje = tot - 1;
	glutIdleFunc(NULL);
	return;
    }

    if (plaatje < 0)
	plaatje += tot;

    SetCamera();

    angle2 = frame*4*M_PI/503;
    angle3 = frame*6*M_PI/503;

    glutPostRedisplay();
    frame++;
}

void myReshape(int w, int h)
{
    SetCamera();
    glViewport (0, 0, w, h);
}

int main(int argc, char *argv[])
{
    int sz;
    extern void calculate_rc(void);

    calculate_rc();
    printf("tot = %d\n", tot);
    if (argc > 1)
	sz = atoi(argv[1]);
    else
	sz = 200;
    glutInitDisplayMode(GLUT_DEPTH | GLUT_RGBA | GLUT_DOUBLE | GLUT_MULTISAMPLE);
    glutInitWindowPosition(200, 0);
    glutInitWindowSize(sz, sz);
    glutCreateWindow("Roller coaster");
    glNewList(1, GL_COMPILE);
    do_display();
    glEndList();
    glutDisplayFunc(display);
    glutKeyboardFunc(parsekey);
    glutSpecialFunc(parsekey_special);
    glutReshapeFunc(myReshape);
    glutIdleFunc(Animate);
    myinit();
    glutSwapBuffers();
    glutMainLoop();
    return 0;             /* ANSI C requires main to return int. */
}
