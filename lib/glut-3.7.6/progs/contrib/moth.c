/* 
	"moth" by Robert Doyle, Naval Research Laboratory, Washington, DC.
	Scene objects are built into display lists in the 'myInit' function  
	(look for three rows of I's). Objects are assembled and motion
	described in the 'display' function (look for three rows of $'s).
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GL/glut.h>

GLfloat wall_color[] = {1.0, 1.0, 1.0, 1.0};
GLfloat wall_ambient[] = {0.5, 0.5, 0.5, 1.0};
GLfloat floor_color[] = {0.5, 1.0, 0.5, 0.5};
GLfloat column_color[] = {1.0, 0.0, 0.0, 1.0};
GLfloat column_ambient[] = {0.25, 0.0, 0.0, 1.0};

GLfloat panel_color[] = {0.1, 0.1, 1.0, 1.0};
GLfloat panel_ambient[] = {0.01, 0.01, 0.3, 1.0};

GLfloat lamp_ambient[] = {1.0, 1.0, 1.0, 1.0};
GLfloat lamp_diffuse[] = {1.0, 1.0, 1.0, 1.0};
GLfloat lamp_specular[] = {1.0, 1.0, 1.0, 1.0};
GLfloat lamp_post_diffuse[] = {0.8, 0.0, 0.0, 1.0};
GLfloat lamp_post_specular[] = {0.8, 0.0, 0.0, 1.0};
GLfloat lamp_post_ambient[] = {0.25, 0.0, 0.0, 1.0};

GLfloat satellite_diffuse[] = {1.0, 0.69, 0.0, 1.0};
GLfloat satellite_shiny[] = {128.0};
GLfloat satellite_specular[] = {1.0, 1.0, 1.0, 1.0};
GLfloat satellite_ambient[] = {0.37, 0.25, 0.0, 1.0};

GLfloat cube_color[] = {1.0, 1.0, 0.0, 1.0};
GLfloat cube_shiny[] = {99.0};
GLfloat cube_specular[] = {0.9, 0.9, 0.9, 1.0};
GLfloat cube_ambient[] = {0.1, 0.1, 0.1, 1.0};

GLfloat shadow_ambient[] = {0.0, 0.0, 0.0, 1.0};
GLfloat shadow_diffuse[] = {0.0, 0.0, 0.0, 0.3};
GLfloat shadow_shiny[] = {0.0};
GLfloat shadow_specular[] = {0.0, 0.0, 0.0, 1.0};

GLuint column = 3;
GLuint ground_shadow_matrix = 16;
GLuint left_wall_shadow_matrix = 17;
GLuint ground = 30;
GLuint left_wall = 40;
GLuint right_wall = 50;
GLuint four_columns = 7;
GLuint two_columns = 32;
GLuint satellite1 = 301;
GLuint satellite2 = 302;
GLuint panel1 = 303;
GLuint panel2 = 304;

static GLfloat Tx = -0.01;
static GLfloat Ty = -0.01;
static GLfloat Tz = -0.02;
#if 0
static GLfloat mvt_zi = -0.7;
#endif
#if 0
static GLfloat mvt_x = 0.0;
static GLfloat mvt_y = 0.0;
static GLfloat mvt_z = -0.7;
#endif

static GLfloat mvt_x = -15.0;
static GLfloat mvt_y = -15.0;
static GLfloat mvt_z = -30.7;

/*static GLfloat Rx = 0.1;
static GLfloat mvr_d = 0.0;
static GLfloat mvr_x = 1.0;
static GLfloat mvr_y = -1.0;
static GLfloat mvr_z = -1.0;*/

static GLfloat Rx = 0.1;
static GLfloat mvr_d = 150.0;
static GLfloat mvr_x = 1.0;
static GLfloat mvr_y = -1.0;
static GLfloat mvr_z = -1.0;

GLfloat cubeXform[4][4];
GLfloat column1Xform[4][4];  
GLfloat column2Xform[4][4];
GLfloat column3Xform[4][4];
GLfloat four_columnsXform[4][4];

/*static GLint nest[1];*/
static float shadowMat_ground[4][4];
static float shadowMat_left[4][4];
static float shadowMat_back[4][4];
static float shadowMat_column[4][4];
static float shadowMat_right[4][4];

static float shadowMat1_ground[4][4];
static float shadowMat1_left[4][4];
static float shadowMat1_back[4][4];
static float shadowMat1_right[4][4];

static int useDB = 1;

static int tick = -1;
static int moving = 1;

#if 0
static float lmodel_ambient[4] = {0.2, 0.2, 0.2, 1.0};
#endif
static float lightPos[4] = {1.0, 2.5, 3.0, 1.0};
#if 0
static float lightDir[4] = {-2.0, -4.0, -2.0, 1.0};
static float lightAmb[4] = {0.2, 0.2, 0.2, 1.0};
static float lightDiff[4] = {0.3, 0.3, 0.3, 1.0};
static float lightSpec[4] = {0.4, 0.4, 0.4, 1.0};
#endif

static float light1Pos[4] = {0.0, 1.6, -5.0, 1.0};
static float light1Amb[4] = {1.0, 1.0, 1.0, 1.0};
static float light1Diff[4] = {1.0, 1.0, 1.0, 1.0};
static float light1Spec[4] = {1.0, 1.0, 1.0, 1.0};

static float leftPlane[4] = {1.0, 0.0, 0.0, 4.88}; /* X = -4.88 */
static float rightPlane[4] = {-1.0, 0.0, 0.0, 4.88}; /* X = 4.98 */
static float groundPlane[4] = {0.0, 1.0, 0.0, 1.450}; /* Y = -1.480 */
static float columnPlane[4] = {0.0, 0.0, 1.0, 0.899}; /* Z = -0.899 */
static float backPlane[4] = {0.0, 0.0, 1.0, 8.98}; /* Z = -8.98 */

#define S 0.7071
#define NS 0.382683
#define NC 0.923880

/* satellite body. */
static float oct_vertices[8][3][4] =
{
	{
		{0.0, 0.0, 0.0, 1.0},
		{0.0, 1.0, 0.0, 1.0},
		{-S, S, 0.0, 1.0}},
		
	{
		{0.0, 0.0, 0.0, 1.0},
		{-S, S, 0.0, 1.0},
		{-1.0, 0.0, 0.0, 1.0}},
	
	{
		{0.0, 0.0, 0.0, 1.0},
		{-1.0, 0.0, 0.0, 1.0},
		{-S, -S, 0.0, 1.0}},
		
	{
		{0.0, 0.0, 0.0, 1.0},
		{-S, -S, 0.0, 1.0},
		{0.0, -1.0, 0.0, 1.0}},
		
	{
		{0.0, 0.0, 0.0, 1.0},
		{0.0, -1.0, 0.0, 1.0},
		{S, -S, 0.0, 1.0}},
		
	{
		
		{0.0, 0.0, 0.0, 1.0},
		{S, -S, 0.0, 1.0},
		{1.0, 0.0, 0.0, 1.0}},
		
	{
		{0.0, 0.0, 0.0, 1.0},
		{1.0, 0.0, 0.0, 1.0},
		{S, S, 0.0, 1.0}},
		
	{
		{0.0, 0.0, 0.0, 1.0},
		{S, S, 0.0, 1.0},
		{0.0, 1.0, 0.0, 1.0}}
	
};

static float oct_side_vertices[8][4][4] =
{
	{
		{-S, S, 0.0, 1.0},
		{0.0, 1.0, 0.0, 1.0},
		{0.0, 1.0, -1.0, 1.0},
		{-S, S, -1.0, 1.0}},
		
	{
		{-1.0, 0.0, 0.0, 1.0},
		{-S, S, 0.0, 1.0},
		{-S, S, -1.0, 1.0},
		{-1.0, 0.0, -1.0, 1.0}},
		
	{
		{-S, -S, 0.0, 1.0},
		{-1.0, 0.0, 0.0, 1.0},
		{-1.0, 0.0, -1.0, 1.0},
		{-S, -S, -1.0, 1.0}},
		
	{
		{0.0, -1.0, 0.0, 1.0},
		{-S, -S, 0.0, 1.0},	
		{-S, -S, -1.0, 1.0},
		{0.0, -1.0, -1.0, 1.0}},
		
	{
		{S, -S, 0.0, 1.0},
		{0.0, -1.0, 0.0, 1.0},
		{0.0, -1.0, -1.0, 1.0},
		{S, -S, -1.0, 1.0}},
		
	{
		{1.0, 0.0, 0.0, 1.0},
		{S, -S, 0.0, 1.0},
		{S, -S, -1.0, 1.0},
		{1.0, 0.0, -1.0, 1.0}},
		
	{
		{S, S, 0.0, 1.0},
		{1.0, 0.0, 0.0, 1.0},
		{1.0, 0.0, -1.0, 1.0},
		{S, S, -1.0, 1.0}},
		
	{
		{0.0, 1.0, 0.0, 1.0},
		{S, S, 0.0, 1.0},
		{S, S, -1.0, 1.0},
		{0.0, 1.0, -1.0, 1.0}}
		
};			
			
static float oct_side_normals[8][3] =
{
		{-NS, NC, 0.0},
		{-NC, NS, 0.0},
		{-NC, -NS, 0.0},
		{-NS, -NC, 0.0},		
		{NS, -NC, 0.0},
		{NC, -NS, 0.0},
		{NC, NS, 0.0},
		{NS, NC, 0.0}
		
};

static float cube_vertexes[6][4][4] =
{
  {
    {-1.0, -1.0, -1.0, 1.0},
    {-1.0, -1.0, 1.0, 1.0},
    {-1.0, 1.0, 1.0, 1.0},
    {-1.0, 1.0, -1.0, 1.0}},

  {
    {1.0, 1.0, 1.0, 1.0},
    {1.0, -1.0, 1.0, 1.0},
    {1.0, -1.0, -1.0, 1.0},
    {1.0, 1.0, -1.0, 1.0}},

  {
    {-1.0, -1.0, -1.0, 1.0},
    {1.0, -1.0, -1.0, 1.0},
    {1.0, -1.0, 1.0, 1.0},
    {-1.0, -1.0, 1.0, 1.0}},

  {
    {1.0, 1.0, 1.0, 1.0},
    {1.0, 1.0, -1.0, 1.0},
    {-1.0, 1.0, -1.0, 1.0},
    {-1.0, 1.0, 1.0, 1.0}},

  {
    {-1.0, -1.0, -1.0, 1.0},
    {-1.0, 1.0, -1.0, 1.0},
    {1.0, 1.0, -1.0, 1.0},
    {1.0, -1.0, -1.0, 1.0}},

  {
    {1.0, 1.0, 1.0, 1.0},
    {-1.0, 1.0, 1.0, 1.0},
    {-1.0, -1.0, 1.0, 1.0},
    {1.0, -1.0, 1.0, 1.0}}
};

static float cube_normals[6][4] =
{
  {-1.0, 0.0, 0.0, 0.0},
  {1.0, 0.0, 0.0, 0.0},
  {0.0, -1.0, 0.0, 0.0},
  {0.0, 1.0, 0.0, 0.0},
  {0.0, 0.0, -1.0, 0.0},
  {0.0, 0.0, 1.0, 0.0}
};

static void usage(void)
{
  printf("\n");
  printf("usage: moth\n");
  printf("\n");
  printf("    Open_gl demo.\n");
  printf("\n");
  printf("  Options:\n");
  printf("    Press the right mouse button for very limited options.\n");
  printf("\n");
#ifndef EXIT_FAILURE /* should be defined by ANSI C <stdlib.h> */
#define EXIT_FAILURE 1
#endif
  exit(EXIT_FAILURE);
}

/*!!!!!!!!!!!!!!!!!!!!!! ERRORS? !!!!!!!!!!!!!!!!!!!!*/

static void checkErrors(void)
{
  GLenum error;
  while ((error = glGetError()) != GL_NO_ERROR) {
    fprintf(stderr, "Error: %s\n", (char *) gluErrorString(error));
  }
}

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/

/*%%%%%%%%%%%%%%%%%%%% DRAW CUBE %%%%%%%%%%%%%%%%%%*/

static void
drawCube(GLfloat color[4], GLfloat ambient[4])
{
  int i;

	glMaterialfv(GL_FRONT, GL_DIFFUSE, color);
	glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);

  for (i = 0; i < 6; ++i) {
    glNormal3fv(&cube_normals[i][0]);
    glBegin(GL_POLYGON);
    glVertex4fv(&cube_vertexes[i][0][0]);
    glVertex4fv(&cube_vertexes[i][1][0]);
    glVertex4fv(&cube_vertexes[i][2][0]);
    glVertex4fv(&cube_vertexes[i][3][0]);
    glEnd();
  }
}

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*OOOOOOOOOOO DRAW OCTOGON TOP OOOOOOOOOOOOO*/

static void drawOct(void)
{
	int i;

	for (i = 0; i < 8; ++i) {
    glNormal3f(0.0, 0.0, 1.0);
    glBegin(GL_TRIANGLE_FAN);
    glVertex4fv(&oct_vertices[i][0][0]);
    glVertex4fv(&oct_vertices[i][1][0]);
    glVertex4fv(&oct_vertices[i][2][0]);    
	 glEnd();
	}
}

/*OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO*/

/*oooooooooooDRAW OCTOGON SIDES ooooooooo*/

static void drawOctSides(void)
{
	int i;

	for (i = 0; i < 8; ++i) {
    glNormal3fv(&oct_side_normals[i][0]);
    glBegin(GL_POLYGON);
    glVertex4fv(&oct_side_vertices[i][0][0]);
    glVertex4fv(&oct_side_vertices[i][1][0]);
    glVertex4fv(&oct_side_vertices[i][2][0]);    
    glVertex4fv(&oct_side_vertices[i][3][0]);
	 glEnd();
	}
}

/*ooooooooooooooooooooooooooooooooooooooo*/

/*SSSSSSSSSSSSSSSS DRAW SATELLITE BODY SSSSSSSSSSSSSSSSSSS*/

static void drawSatellite(GLfloat diffuse[4], GLfloat ambient[4], GLfloat specular[4], GLfloat shiny[1])
{

	glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
	glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
	glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, shiny);	
	

		glPushMatrix();
			glScalef(0.3, 0.3, 0.9);
		glPushMatrix();
			drawOctSides();
		glPopMatrix();

		glPushMatrix();
			glTranslatef(0.0, 0.0, 0.0);
			drawOct();		
		glPopMatrix();
		glPushMatrix();
			glRotatef(180, 1.0, 0.0, 0.0);
			glTranslatef(0.0, 0.0, 1.0);
			drawOct();
		glPopMatrix(); 
		glPopMatrix(); 
}

/*SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS*/

/*PPPPPPPPPPPPPPPP DRAW SOLAR PANELS PPPPPPPPPPPP*/

static void drawPanels(GLfloat color[4], GLfloat ambient[4])
{

	glMaterialfv(GL_FRONT, GL_DIFFUSE, color);
	glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);	

		glPushMatrix();
			glTranslatef(0.95, 0.0, -0.45);
			glRotatef(45.0, 1.0, 0.0, 0.0);
			glScalef(0.65, 0.20, 0.02);
			drawCube(color, ambient);
		glPopMatrix();

/*		glPushMatrix();
			glTranslatef(0.95, 0.0, -0.45);		
			glTranslatef((1.3/3.0), 0.1, 0.01);
			glRotatef(45.0, 1.0, 0.0, 0.0);
			glScalef(0.65/3.2, 0.20/2.1, 0.08);					
			drawCube(color, ambient);
		glPopMatrix();
*/		
		glPushMatrix();
			glTranslatef(-0.95, 0.0, -0.45);
			glRotatef(45.0, 1.0, 0.0, 0.0);
			glScalef(0.65, 0.20, 0.02);
			drawCube(color, ambient);
		glPopMatrix();

}
/*PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP*/

/*################ DRAW FLOOR ################*/

void drawFloor(GLfloat f_color[4], GLfloat ambient[4])
{
        
    glMaterialfv(GL_FRONT, GL_DIFFUSE, f_color);
    glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);

/*    glNormal3f (0.0, 0.0, 1.0); */

    glBegin (GL_QUADS);    
    glVertex3f (-1.0, -1.0, 0.0);    
    glVertex3f (1.0, -1.0, 0.0); 
    glVertex3f (1.0, 1.0, 0.0);
    glVertex3f (-1.0, 1.0, 0.0);  
        glEnd();

}

/*################################################*/

/*||||||||||||||| DRAW GROUND |||||||||||||||||*/

/* Ground coordinates are in drawGround() below. Subdivision */
/* of triangles id done by subDivide(). */


void subDivide(float u1[3], float u2[3], float u3[3], int depth)
{

GLfloat u12[3];
GLfloat u23[3];
GLfloat u31[3];	

GLint i;

	if(depth == 0) {

    glBegin (GL_POLYGON);    
    glNormal3f (0.0, 0.0, 1.0); glVertex3fv(u1);
    glNormal3f (0.0, 0.0, 1.0); glVertex3fv(u2);
    glNormal3f (0.0, 0.0, 1.0); glVertex3fv(u3);
	glEnd();
	return;
}

	for(i = 0; i < 3; i++){  

		u12[i] = (u1[i] + u2[i]) / 2.0;
		u23[i] = (u2[i] + u3[i]) / 2.0;
		u31[i] = (u3[i] + u1[i]) / 2.0;	

	} 

	subDivide(u1, u12, u31, depth - 1); 	
	subDivide(u2, u23, u12, depth - 1);
	subDivide(u3, u31, u23, depth - 1);
	subDivide(u12, u23, u31, depth - 1);	

}

void drawGround(void)
{

/* Use two subdivided triangles for the unscaled 1X1 square. */
/* Subdivide to this depth: */

GLint maxdepth = 2;

/* Coordinates of first triangle: */

GLfloat u1[] = {-1.0, -1.0, 0.0};
GLfloat u2[] = {1.0, -1.0, 0.0};
GLfloat u3[] = {1.0, 1.0, 0.0};		

/* Coordinates of second triangle: */
    
GLfloat v1[] = {-1.0, -1.0, 0.0};    
GLfloat v2[] = {1.0, 1.0, 0.0};
GLfloat v3[] = {-1.0, 1.0, 0.0};

	subDivide(u1, u2, u3, maxdepth);
	subDivide(v1, v2, v3, maxdepth);
	
}
	

/*|||||||||||||||||||||||||||||||||||||||||||*/

/* Matrix for shadow. From Mark Kilgard's "scube". */

static void
myShadowMatrix(float ground[4], float light[4], float shadowMat[4][4])
{
  float dot;
/*  float shadowMat[4][4]; */

  dot = ground[0] * light[0] +
    ground[1] * light[1] +
    ground[2] * light[2] +
    ground[3] * light[3];

  shadowMat[0][0] = dot - light[0] * ground[0];
  shadowMat[1][0] = 0.0 - light[0] * ground[1];
  shadowMat[2][0] = 0.0 - light[0] * ground[2];
  shadowMat[3][0] = 0.0 - light[0] * ground[3];

  shadowMat[0][1] = 0.0 - light[1] * ground[0];
  shadowMat[1][1] = dot - light[1] * ground[1];
  shadowMat[2][1] = 0.0 - light[1] * ground[2];
  shadowMat[3][1] = 0.0 - light[1] * ground[3];

  shadowMat[0][2] = 0.0 - light[2] * ground[0];
  shadowMat[1][2] = 0.0 - light[2] * ground[1];
  shadowMat[2][2] = dot - light[2] * ground[2];
  shadowMat[3][2] = 0.0 - light[2] * ground[3];

  shadowMat[0][3] = 0.0 - light[3] * ground[0];
  shadowMat[1][3] = 0.0 - light[3] * ground[1];
  shadowMat[2][3] = 0.0 - light[3] * ground[2];
  shadowMat[3][3] = dot - light[3] * ground[3];

/*  glMultMatrixf((const GLfloat *) shadowMat); */
}

void
idle(void)
{
  tick++;
  if (tick >= 60) {
    tick = 0;
  }
  glutPostRedisplay();
}

/* ARGSUSED1 */
void
keyboard(unsigned char ch, int x, int y)
{
  switch (ch) {
  case 27:             /* escape */
    exit(0);
    break;
  case ' ':
    if (!moving) {
      idle();
      glutPostRedisplay();
    }
  }
}

/*$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$*/
/*$$$$$$$$$$$$$$$$$$$$$$ DISPLAY $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$*/
/*$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$*/
                      
void display(void)
{


  glPushMatrix(); /* Make sure the matrix stack is cleared at the end of this function. */

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();

/*@@@@@@ Rotation and Translation of Entire Scene @@@@@*/
		
	if(mvt_x < 0 && mvt_y < 0){
		glTranslatef(mvt_x ,mvt_y ,mvt_z );
		mvt_x = mvt_x - Tx;
		mvt_y = mvt_y - Ty;
		mvt_z = mvt_z - Tz;

		glRotatef(mvr_d, mvr_x, mvr_y, mvr_z);
		mvr_d = mvr_d - Rx;
	}
	
	else{
		glTranslatef(0.0, 0.0 ,mvt_z);
	}

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

glPushMatrix();
	   glLightfv(GL_LIGHT1, GL_POSITION, light1Pos);
glPopMatrix();
/*______________________ Draw Floor _______________________*/
  
glPushMatrix();
  glCallList(ground);
glPopMatrix();

/*_________________________________________________________*/

/*@@@@@@@@@ Draw Lamp Post amd Lamp @@@@@@@@@@*/

glPushMatrix();
  glCallList(21);
glPopMatrix(); 

glPushMatrix();
  glCallList(22);
glPopMatrix();

glPushMatrix();
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glCallList(501);
	glDisable(GL_BLEND);
glPopMatrix(); 

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

/*||||||||||||||||||| Draw Left Wall ||||||||||||||||||*/  

  glCallList(left_wall); 
    
/*|||||||||||||||||||||||||||||||||||||||||||||||||||||*/

/*\\\\\\\\\\\\\\\\ Draw Right Wall \\\\\\\\\\\\\\*/
  
  glCallList(right_wall);

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

/*[[[[[[[[[[[[[[[[[[[ Draw Columns ]]]]]]]]]]]]]]]]]]]*/

/***** Place columns at front of scene. *****/

       glCallList(four_columns);	
       
/***** Place columns at back of scene. *****/

       glPushMatrix();
		 glTranslatef(0.0, 0.0, -9.0);
       glCallList(four_columns);	
       glPopMatrix();

/***** Place columns at centers of left and right walls. *****/		

       glCallList(two_columns);	

/*[[[[[[[[[[[[[[[[[[[[[[[[[]]]]]]]]]]]]]]]]]]]]]]]]]]]]*/


/*....................... Draw Column Shadows ....................*/

/*glDepthMask(GL_FALSE);  
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);*/
  
/* shadows on floor */

/*	glPushMatrix();	  	  
	  glCallList(100);	  
	glPopMatrix();*/

/* shdows on left wall */

/*	glPushMatrix();
	  glCallList(101);
	glPopMatrix();*/

/* shdows on back wall */

/*	glPushMatrix();
	  glCallList(102); 	  
	glPopMatrix();*/

/* shdows on right wall */

/*	glPushMatrix();
	  glCallList(103); 	  
	glPopMatrix();*/

/*glDepthMask(GL_TRUE); 
glDisable(GL_BLEND);*/

/*................................................................*/

/************************* CUBE ***********************/

	glMaterialf(GL_FRONT, GL_SHININESS, 99.0);
	glMaterialfv(GL_FRONT, GL_SPECULAR, cube_specular);

  glPushMatrix();
  	glTranslatef(0.0, 0.0, -5.0);
  	glRotatef((360.0 / (30 * 2)) * tick, 0, 1, 0);  	
  	glPushMatrix();
  		glTranslatef(0.0, 0.2, 2.0);
/*  		glTranslatef(0.0, 0.2, 0.0); */
/*  		glScalef(0.3, 0.3, 0.3); */
  		glRotatef((360.0 / (30 * 1)) * tick, 1, 0, 0);
  		glRotatef((360.0 / (30 * 2)) * tick, 0, 1, 0);
  		glRotatef((360.0 / (30 * 4)) * tick, 0, 0, 1);

  		glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *) cubeXform);

/*			drawSatellite(satellite_diffuse, satellite_ambient, satellite_specular, satellite_shiny); */
			glCallList(satellite1);
			glCallList(panel1);
/*			drawPanels(panel_color, panel_ambient); */

  	glPopMatrix();
  glPopMatrix();

	glMaterialf(GL_FRONT, GL_SHININESS, 0.0);
	glMaterialfv(GL_FRONT, GL_SPECULAR, shadow_specular);

/****************************************************/

/*................... CUBE SHADOWS .............................*/  

/*glDepthMask(GL_FALSE);*/
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  
	glPushMatrix();
		glMultMatrixf((const GLfloat *) shadowMat1_ground); 	
		glRotatef(-mvr_d, mvr_x, mvr_y, mvr_z);
	 	glTranslatef(-mvt_x, -mvt_y, -mvt_z);	/* correct for modelview matrix */		
		glMultMatrixf((const GLfloat *) cubeXform);


/*		drawSatellite(shadow_diffuse, shadow_ambient, shadow_specular, shadow_shiny); */     /* draw ground shadow */
		glCallList(satellite2);
		glTranslatef(0.0, -.040, 0.0);
		glCallList(panel2);
/*		drawPanels(shadow_diffuse, shadow_ambient); */
	glPopMatrix(); 

/* Shadow left wall only if cube is in front of left wall. */
	if((tick*6) >= 220 && (tick*6) <= 320) { 

	glPushMatrix();
		glMultMatrixf((const GLfloat *) shadowMat1_left);
		glRotatef(-mvr_d, mvr_x, mvr_y, mvr_z);
		glTranslatef(-mvt_x, -mvt_y, -mvt_z);	/* correct for modelview matrix */
		glMultMatrixf((const GLfloat *) cubeXform);
		drawSatellite(shadow_diffuse, shadow_ambient, shadow_specular, shadow_shiny);      /* draw left shadow */
		drawPanels(shadow_diffuse, shadow_ambient);
	glPopMatrix();

	}
	
/* Shadow back wall only if cube is in front of back wall. */
	if((tick*6) >= 125 && (tick*6) <= 330) {

	glPushMatrix();
		glMultMatrixf((const GLfloat *) shadowMat1_back); 	
		glRotatef(-mvr_d, mvr_x, mvr_y, mvr_z);
		glTranslatef(-mvt_x, -mvt_y, -mvt_z);	/* correct for modelview matrix */
		glMultMatrixf((const GLfloat *) cubeXform);
		drawSatellite(shadow_diffuse, shadow_ambient, shadow_specular, shadow_shiny);      /* draw back wall shadow */
		drawPanels(shadow_diffuse, shadow_ambient);
	glPopMatrix();

	}

/* Shadow right wall only if cube is in front of right wall.  */
	if((tick*6) >= 40 && (tick*6) <= 145) {
	
	glPushMatrix();
		glMultMatrixf((const GLfloat *) shadowMat1_right); 	
		glRotatef(-mvr_d, mvr_x, mvr_y, mvr_z);
		glTranslatef(-mvt_x, -mvt_y, -mvt_z);	/* correct for modelview matrix */
		glMultMatrixf((const GLfloat *) cubeXform);
		drawSatellite(shadow_diffuse, shadow_ambient, shadow_specular, shadow_shiny);      /* draw right wall shadow */
		drawPanels(shadow_diffuse, shadow_ambient);
	glPopMatrix();

	}

/*glDepthMask(GL_TRUE);*/
glDisable(GL_BLEND);

/*.........................................................*/

	glutSwapBuffers();

	checkErrors();

	glPopMatrix(); /* Clear the matrix stack */

}

/*$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$*/

void
menu_select(int mode)
{
  switch (mode) {
  case 1:
    moving = 1;
    glutIdleFunc(idle);
    break;
  case 2:
    moving = 0;
    glutIdleFunc(NULL);
    break;
  case 5:
    exit(0);
    break;
  }
}

void
visible(int state)
{
  if (state == GLUT_VISIBLE) {
    if (moving)
      glutIdleFunc(idle);
  } else {
    if (moving)
      glutIdleFunc(NULL);
  }
}

/* IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII*/
/* IIIIIIIIIIIIIIIIII INITIALIZE IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII*/
/* IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII*/

void myInit(void)
{

/*	glGetIntegerv(GL_MAX_CLIP_PLANES, nest);
	 printf("GL_MAX_CLIP_PLANES are %d \n", nest[0]); */

/*%%%%%%%% Initialize Positional Light and Ambient Light %%%%%%%%*/

#if 0
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
#endif

#if 0
    	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	   glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
	   glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmb);
	   glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiff);
	   glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpec);
	   glEnable(GL_LIGHT0);
#endif

/*** Initial light position is declared in the display function ***/

	   glLightfv(GL_LIGHT1, GL_AMBIENT, light1Amb);
	   glLightfv(GL_LIGHT1, GL_DIFFUSE, light1Diff);
	   glLightfv(GL_LIGHT1, GL_SPECULAR, light1Spec);
	   glEnable(GL_LIGHT1);
	   
	   glEnable(GL_LIGHTING);	   

/*  glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, 0.7);*/ 
/*  glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 0.04);*/ /* use 0.04 w/ 24 bit color */
  glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.07); /* try 0.07 w/ 24 bit color */


/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*########### Initialize Fog ##################*/

/*
{	
	GLfloat fog_color[] = {0.5, 0.5, 0.5, 1.0};
	GLfloat fog_start[] = {0.0, 0.0, 1.0, 20.0};
	
	glEnable(GL_FOG);
	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogfv(GL_FOG_COLOR, fog_color);
	glFogf(GL_FOG_DENSITY, 0.35);
	glFogfv(GL_FOG_START, fog_start);
	glHint(GL_FOG_HINT, GL_FASTEST);
}	

*/

/*##########################################*/

/*....Shadow Matrices For Floor, Left Wall, Back Wall, and Right Wall......*/


/* For light0 */

	   myShadowMatrix(groundPlane, lightPos, shadowMat_ground);
	   myShadowMatrix(leftPlane, lightPos, shadowMat_left);
	   myShadowMatrix(columnPlane, lightPos, shadowMat_column);
	   myShadowMatrix(backPlane, lightPos, shadowMat_back);   
	   myShadowMatrix(rightPlane, lightPos, shadowMat_right);

/* For light1 */

	   myShadowMatrix(groundPlane, light1Pos, shadowMat1_ground);
	   myShadowMatrix(leftPlane, light1Pos, shadowMat1_left);
	   myShadowMatrix(backPlane, light1Pos, shadowMat1_back);
	   myShadowMatrix(rightPlane, light1Pos, shadowMat1_right);   

/*.......................................................................*/

/*sssssssssssssssss Make Satellite Body and Shadow ssssssssssssssssssssssss*/

  glNewList(satellite1, GL_COMPILE);
    glPushMatrix();
	drawSatellite(satellite_diffuse, satellite_ambient, satellite_specular, satellite_shiny);
    glPopMatrix();
  glEndList();
  glNewList(satellite2, GL_COMPILE);
    glPushMatrix();
	drawSatellite(shadow_diffuse, shadow_ambient, shadow_specular, shadow_shiny);
    glPopMatrix();
  glEndList();

/*sssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss*/

/*ppppppppppppppppppppppppppp Make Solar Panels and Shadows pppppppppppppppppp*/

  glNewList(panel1, GL_COMPILE);
    glPushMatrix();
		drawPanels(panel_color, panel_ambient);
    glPopMatrix();
  glEndList();
  
  glNewList(panel2, GL_COMPILE);
    glPushMatrix();
		drawPanels(shadow_diffuse, shadow_ambient);
    glPopMatrix();
  glEndList();  


/*pppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppp*/

/*========= Make Floor ==============*/ 

  glNewList(ground, GL_COMPILE);
    glPushMatrix();
      	glPushAttrib(GL_LIGHTING_BIT);
      		glMaterialfv(GL_FRONT, GL_DIFFUSE, floor_color);
      		glMaterialfv(GL_FRONT, GL_AMBIENT, shadow_ambient);
      		glTranslatef(0.0, -1.5, -5.0);
      		glRotatef(-90.0, 1, 0, 0);
      		glScalef(5.0, 5.0, 1.0);           
      		drawGround();  /* draw ground */
   		glPopAttrib();
    glPopMatrix();
  glEndList();

/*==================================*/

/*@@@@@@@@@@ Make Lamp Post and Lamp @@@@@@@@@@@@*/

	 glNewList(21, GL_COMPILE);
      glPushMatrix(); 
      	glPushAttrib(GL_LIGHTING_BIT); 
			glMaterialfv(GL_FRONT, GL_AMBIENT, lamp_post_specular);
			glTranslatef(0.0, -0.1, -5.0);
	      	glScalef(0.07, 1.45, 0.07);    
	      	drawCube(lamp_post_diffuse, lamp_post_ambient);  /* draw lamp post */ 
   		glPopAttrib();
      glPopMatrix();
      glPushMatrix();
		  glTranslatef(0.0, -1.45, -5.0);      
	      glScalef(0.3, 0.05, 0.3);
	      drawCube(wall_color, cube_ambient);  /* draw lamp post base */
      glPopMatrix();
	 glEndList();

	 glNewList(22, GL_COMPILE);
      glPushMatrix();
      	glPushAttrib(GL_LIGHTING_BIT); 
			glMaterialfv(GL_FRONT, GL_AMBIENT, lamp_ambient);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, lamp_diffuse);				
			glMaterialfv(GL_FRONT, GL_SPECULAR, lamp_specular);				
			glTranslatef(0.0, 1.6, -5.0);
   			glutSolidSphere(0.3, 20.0, 20.0);   /* draw lamp */
   		glPopAttrib();
      glPopMatrix();
	 glEndList();

/*** Lamp post base shadow ***/

	 glNewList(501, GL_COMPILE);
      glPushMatrix();
      	glPushAttrib(GL_LIGHTING_BIT); 
			glMaterialfv(GL_FRONT, GL_AMBIENT, shadow_ambient);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, shadow_diffuse);
			glMaterialfv(GL_FRONT, GL_SPECULAR, shadow_specular);
			glMaterialfv(GL_FRONT, GL_SHININESS, shadow_shiny);
			glTranslatef(0.0, -1.49, -5.0);
			glRotatef(-90.0, 1.0, 0.0, 0.0);
			glScalef(0.7, 0.7, 1.0);
			drawOct();
   		glPopAttrib();
      glPopMatrix();
	 glEndList();





/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

/*||||||||||| Make Left Wall |||||||||||||*/

  glNewList(left_wall, GL_COMPILE);
    glPushMatrix();      
    glPushAttrib(GL_LIGHTING_BIT);
      		glMaterialfv(GL_FRONT, GL_DIFFUSE, wall_color);
      		glMaterialfv(GL_FRONT, GL_AMBIENT, wall_ambient);
      		glTranslatef(0.0, -1.5, 0.0);
     		glTranslatef(0.0, 1.2, 0.0);      
      		glTranslatef(0.0, 0.0, -5.0);
      		glTranslatef(-5.0, 0.0, 0.0);
      		glRotatef(90.0, 0, 1, 0);
      		glScalef(4.5, 1.2, 1.0);       
      		glNormal3f (0.0, 0.0, 1.0);
      		drawGround();  /* draw left wall */
	glPopAttrib();
    glPopMatrix();
  glEndList();

/*||||||||||||||||||||||||||||||||||||||||*/

/*\\\\\\\\\\\\\ Make Right Wall \\\\\\\\\\\\\\\\\\\*/

  glNewList(right_wall, GL_COMPILE);
    glPushMatrix();
    glPushAttrib(GL_LIGHTING_BIT);
      glMaterialfv(GL_FRONT, GL_DIFFUSE, wall_color);
      glMaterialfv(GL_FRONT, GL_AMBIENT, wall_ambient);
      glTranslatef(0.0, -1.5, 0.0);
      glTranslatef(0.0, 1.2, 0.0);

      glTranslatef(0.0, 0.0, -5.0);
      glTranslatef(5.0, 0.0, 0.0);
      glRotatef(270.0, 0, 1, 0);      
                
      glScalef(4.5, 1.2, 1.0);
      glNormal3f (0.0, 0.0, 1.0);      
      drawGround();  /* draw right wall */
    glPopAttrib();
    glPopMatrix();
  glEndList();                        

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

/*[[[[[[[[[[[ Build Columns ]]]]]]]]]]]*/
	
glPushMatrix();    
	 glNewList(1, GL_COMPILE);
      glPushMatrix(); 
	      glScalef(0.4, 1.4, 0.4);    
	      drawCube(column_color, column_ambient);  /* draw column1 */ 
      glPopMatrix();
	 glEndList();

	glNewList(2, GL_COMPILE);
        glPushMatrix();
            glTranslatef(0.0, -1.45, 0.0);
            glScalef(0.5, 0.1, 0.5);
            drawCube(wall_color, cube_ambient); /* draw base */
        glPopMatrix();
        glPushMatrix();	    
		    glTranslatef(0.0, 1.45, 0.0);
 	        glScalef(0.5, 0.1, 0.5);
	        drawCube(wall_color, cube_ambient); /* draw top */
        glPopMatrix();
    glEndList();    	  
glPopMatrix();
    
   glNewList(column, GL_COMPILE);
 		glPushMatrix();
     		glCallList(1);
			glCallList(2);
 		glPopMatrix();
	glEndList(); 

/***** Place columns at front of scene. *****/

glNewList(4, GL_COMPILE);
       glPushMatrix();
         glTranslatef(-5.0, 0.0, -0.5);
         glCallList(column);	
       glPopMatrix();
glEndList();
                    	
glNewList(5, GL_COMPILE);
       glPushMatrix();
         glTranslatef(-1.75, 0.0, -0.5);
         glCallList(column);	
       glPopMatrix();
glEndList();       

glNewList(6, GL_COMPILE);
       glPushMatrix();
         glTranslatef(1.75, 0.0, -0.5);
         glCallList(column);	
       glPopMatrix();
glEndList();       
       
glNewList(17, GL_COMPILE);
       glPushMatrix();
         glTranslatef(5.0, 0.0, -0.5);
         glCallList(column);	
       glPopMatrix();
glEndList();     


/*** Get the modelview matrix once ***/
       glPushMatrix();
			glRotatef(-mvr_d, mvr_x, mvr_y, mvr_z);
         		glTranslatef(-mvt_x, -mvt_y, -mvt_z);
         glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *) four_columnsXform);
       glPopMatrix();

glNewList(four_columns, GL_COMPILE);
       glPushMatrix();
			glCallList(4);
			glCallList(5);
			glCallList(6);
			glCallList(17);
       glPopMatrix();	
glEndList();

/***** Make two columns for sides of scene *****/

glNewList(two_columns, GL_COMPILE);
     glPushMatrix();
		glRotatef(90.0, 0.0, 1.0, 0.0);
        glTranslatef(5.0, 0.0, -5.0);
       		glPushMatrix();
           		glTranslatef(0.0, 0.0, -0.3);
           		glCallList(column);	
       		glPopMatrix();
       		glPushMatrix();
           		glTranslatef(0.0, 0.0, 10.3);
           		glCallList(column);	
       		glPopMatrix();
     glPopMatrix();
glEndList();     




/*[[[[[[[[[[[[[[[[]]]]]]]]]]]]]]]]*/


/* .......................Make shadows .........................*/

    glPushMatrix();    	   
	 glNewList(8, GL_COMPILE);
        glPushMatrix(); 
	    		glScalef(0.4, 1.4, 0.4);    
	    		drawCube(shadow_diffuse, shadow_ambient);  /* draw column1 */ 
        glPopMatrix();
	 glEndList();


	glNewList(9, GL_COMPILE);
          glPushMatrix();
            glTranslatef(0.0, -1.45, 0.0);
            glScalef(0.5, 0.1, 0.5);
            drawCube(shadow_diffuse, shadow_ambient); /* draw base. */
          glPopMatrix();
          glPushMatrix();	    
			 glTranslatef(0.0, 1.45, 0.0);
 	         glScalef(0.5, 0.1, 0.5);
	         drawCube(shadow_diffuse, shadow_ambient); /* draw top. */
          glPopMatrix();
    glEndList();
   glPopMatrix();
    
    glNewList(10, GL_COMPILE);
    	glPushMatrix(); 
     		glCallList(8);
			glCallList(9);
    	glPopMatrix();
	 glEndList();

glNewList(11, GL_COMPILE);
       glPushMatrix();
       glTranslatef(-5.0, 0.0, -0.5);
       glCallList(10);	
       glPopMatrix();
glEndList();
                    	
glNewList(12, GL_COMPILE);
       glPushMatrix();
       glTranslatef(-1.75, 0.0, -0.5);
       glCallList(10);	
       glPopMatrix();
glEndList();       

glNewList(13, GL_COMPILE);
       glPushMatrix();
       glTranslatef(1.75, 0.0, -0.5 );
       glCallList(10);	
       glPopMatrix();
glEndList();       
       
glNewList(14, GL_COMPILE);
       glPushMatrix();
       glTranslatef(5.0, 0.0, -0.5 );
       glCallList(10);	
       glPopMatrix();
glEndList();       

glNewList(15, GL_COMPILE);
        glPushMatrix();
	glCallList(11);
	glCallList(12);
	glCallList(13);
	glCallList(14);	
	glPopMatrix();
glEndList();
       	
glNewList(100, GL_COMPILE);
       glPushMatrix();
	  glMultMatrixf((const GLfloat *) shadowMat_ground);	
	  glTranslatef(-mvt_x, -mvt_y, -mvt_z);	/* correct for modelview matrix */
	  glRotatef(-mvr_d, mvr_x, mvr_y, mvr_z);
	  glMultMatrixf((const GLfloat *) four_columnsXform);
       	  glCallList(15);
        glPopMatrix();
glEndList();

glNewList(101, GL_COMPILE);
       glPushMatrix();
	  glMultMatrixf((const GLfloat *) shadowMat_left);	
	  glTranslatef(-mvt_x, -mvt_y, -mvt_z);	/* correct for modelview matrix */
	  glRotatef(-mvr_d, mvr_x, mvr_y, mvr_z);
	  glMultMatrixf((const GLfloat *) four_columnsXform);
       	  glCallList(15);
        glPopMatrix();
glEndList();

glNewList(102, GL_COMPILE);
       glPushMatrix();
	  glMultMatrixf((const GLfloat *) shadowMat_back);	
	  glRotatef(-mvr_d, mvr_x, mvr_y, mvr_z);
	  glTranslatef(-mvt_x, -mvt_y, -mvt_z);	/* correct for modelview matrix */
	  glMultMatrixf((const GLfloat *) four_columnsXform);
       	  glCallList(15);
        glPopMatrix();
glEndList();

glNewList(103, GL_COMPILE);
       glPushMatrix();
	  glMultMatrixf((const GLfloat *) shadowMat_right);	
	  glRotatef(-mvr_d, mvr_x, mvr_y, mvr_z);
	  glTranslatef(-mvt_x, -mvt_y, -mvt_z);	/* correct for modelview matrix */
	  glMultMatrixf((const GLfloat *) four_columnsXform);
       	  glCallList(15);
        glPopMatrix();
glEndList();


/* ......................................................*/

}

/* IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII*/


/* ////////////////////////////////////////////////////////////// */
/* //////////////////////\ MAIN ///////////////////////////////// */
/* ////////////////////////////////////////////////////////////// */

int main(int argc, char **argv)
{
  int width = 320, height = 240;
  int i; 

  char *name;
  glutInitWindowSize(width, height);
  glutInit(&argc, argv);

  /* process commmand line args */
  for (i = 1; i < argc; ++i) {
    if (!strcmp("-db", argv[i])) {
    useDB = !useDB;
    } else {
      usage();
    }
  }

/* choose visual */

      glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
      name = "MOTH - by Bob Doyle";    
      glutCreateWindow(name);

  glutKeyboardFunc(keyboard);

  myInit();  /* initialize objects in scene */
  glutDisplayFunc(display); 
  glutVisibilityFunc(visible);


  glutCreateMenu(menu_select);
  glutAddMenuEntry("Start motion", 1);
  glutAddMenuEntry("Stop motion", 2);
  glutAddMenuEntry("Quit", 5);
  glutAddMenuEntry("Drink Ed's beer", 5);
  glutAttachMenu(GLUT_RIGHT_BUTTON);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  glFrustum(-.9, .9, -.9, .9, 1.0, 35.0);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
#if 0
  glTranslatef(0.0, 0.0, mvt_zi);
#endif

  glEnable(GL_NORMALIZE);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);      /* double your fun */
  glShadeModel(GL_SMOOTH);
  glDepthFunc(GL_LESS);
  glDepthMask(GL_TRUE);
  glEnable(GL_DEPTH_TEST);
	myInit();  /* initialize objects in scene */

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
