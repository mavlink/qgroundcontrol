#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "walker.h"


  /* for cube models */
#define UPPER_LEG_SIZE_C 0.45
#define LOWER_LEG_SIZE_C 0.45
#define UPPER_ARM_SIZE_C 0.4
#define LOWER_ARM_SIZE_C 0.4
#define FOOT_SIZE_C 0.2
#define HEAD_SIZE_C 0.25
#define TORSO_HEIGHT_C 0.8
#define TORSO_WIDTH_C 0.5
#define LEG_GIRTH_C 0.1
#define ARM_GIRTH_C 0.05

  /* for cylinder models */
#define UPPER_LEG_SIZE 0.5
#define LOWER_LEG_SIZE 0.5
#define LEG_GIRTH 0.08
#define UPPER_LEG_GIRTH 0.1
#define UPPER_LEG_TAPER 0.8
#define LOWER_LEG_TAPER 0.8
#define LOWER_LEG_GIRTH 0.07

#define UPPER_ARM_SIZE 0.45
#define LOWER_ARM_SIZE 0.45
#define ARM_GIRTH 0.05
#define UPPER_ARM_GIRTH 0.05
#define LOWER_ARM_GIRTH 0.04
#define UPPER_ARM_TAPER 0.8
#define LOWER_ARM_TAPER 0.8

#define HIP_JOINT_SIZE 0.1
#define KNEE_JOINT_SIZE 0.09
#define SHOULDER_JOINT_SIZE 0.05
#define ELBOW_JOINT_SIZE 0.045

#define HEAD_SIZE 0.2
#define FOOT_SIZE 0.15

#define TORSO_HEIGHT 0.8
#define TORSO_WIDTH 0.35
#define TORSO_TAPER 0.7

#define STACKS 10
#define SLICES 10

#define NUM_BODY_PARTS 7


#define LEFT 0
#define RIGHT 1
#define SOLID 1
#define WIRE 0



void DrawTheGuy_WC(void);
void DrawTheGuy_SC(void);
void draw_head_C(int solid);
void draw_torso_C(int solid);
void draw_leg_C(int which, int solid);
void draw_arm_C(int which, int solid);

void StoreTheGuy_SL(void);
void DrawTheGuy_SL(void);
void draw_head_SL(void);
void draw_torso_SL(void);
void draw_leg_SL(int which);
void draw_arm_SL(int which);
void store_head_SL(void);
void store_torso_SL(void);
void store_uleg_SL(void);
void store_lleg_SL(void);
void store_foot_SL(void);
void store_uarm_SL(void);
void store_larm_SL(void);

void StoreTheGuy_SL2(void);
void DrawTheGuy_SL2(void);
void draw_head_SL2(void);
void draw_torso_SL2(void);
void draw_leg_SL2(int which);
void draw_arm_SL2(int which);
void store_head_SL2(void);
void store_torso_SL2(void);
void store_uleg_SL2(void);
void store_lleg_SL2(void);
void store_foot_SL2(void);
void store_uarm_SL2(void);
void store_larm_SL2(void);

/**************************************************************/
void DrawTheGuy_WC(void)
{
  draw_head_C(WIRE);
  draw_torso_C(WIRE);
  draw_leg_C(LEFT,  WIRE);
  draw_leg_C(RIGHT, WIRE);
  draw_arm_C(LEFT,  WIRE);
  draw_arm_C(RIGHT, WIRE);
}

/**************************************************************/
void DrawTheGuy_SC(void)
{
  GLfloat head_diffuse[] = { 0.7, 0.7, 0.0, 1.0 };
  GLfloat torso_diffuse[] = { 0.0, 0.7, 0.7, 1.0 };
  GLfloat leg_diffuse[] = { 0.7, 0.0, 0.7, 1.0 };
  GLfloat arm_diffuse[] = { 0.7, 0.4, 0.4, 1.0 };

  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, head_diffuse);
  draw_head_C(SOLID);

  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, torso_diffuse);
  draw_torso_C(SOLID);

  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, leg_diffuse);
  draw_leg_C(LEFT,  SOLID);
  draw_leg_C(RIGHT, SOLID);

  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, arm_diffuse);
  draw_arm_C(LEFT,  SOLID);
  draw_arm_C(RIGHT, SOLID);
}



/**********************************************************/
void draw_head_C(int solid)
{
  glPushMatrix();
  glColor3f(1.0, 1.0, 0.0);
  glTranslatef(0.0, TORSO_HEIGHT_C+(HEAD_SIZE_C/1.5), 0.0);
  glScalef(HEAD_SIZE_C, HEAD_SIZE_C, LEG_GIRTH_C);
  if (solid)
    glutSolidCube(1.0);
  else
    glutWireCube(1.0);
  glPopMatrix();
}

/**********************************************************/
void draw_torso_C(int solid)
{
  glPushMatrix();
  glColor3f(0.0, 1.0, 1.0);
  glTranslatef(0.0, TORSO_HEIGHT_C/2.0, 0.0);
  glScalef(TORSO_WIDTH_C, TORSO_HEIGHT_C, LEG_GIRTH_C);
  if (solid)
    glutSolidCube(1.0);
  else
    glutWireCube(1.0);
  glPopMatrix();
}

/**********************************************************/
void draw_leg_C(int which, int solid)
{
  glPushMatrix();
  if (which == 0)
    glTranslatef(TORSO_WIDTH_C/4.0, 0.0, 0.0);
  else glTranslatef(-TORSO_WIDTH_C/4.0, 0.0, 0.0);
  /* Upper leg: rotates about the x axis only */
  glColor3f(1.0, 0.0, 0.0);
  glRotatef(Walk_cycle[which][0][Step],1.0, 0.0, 0.0);
  glPushMatrix();
  glTranslatef(0.0, -UPPER_LEG_SIZE_C/2.0, 0.0);
  glScalef(LEG_GIRTH_C, UPPER_LEG_SIZE_C, LEG_GIRTH_C);
  if (solid)
    glutSolidCube(1.0);
  else
    glutWireCube(1.0);
  glPopMatrix();

  /* Lower leg: rotates about the x axis only */
  glColor3f(0.0, 1.0, 0.0);
  glTranslatef(0.0, -(UPPER_LEG_SIZE_C+LOWER_LEG_SIZE_C)/2.0, 0.0);
  glRotatef(Walk_cycle[which][1][Step], 1.0, 0.0, 0.0);
  glPushMatrix();
  glTranslatef(0.0, -LOWER_LEG_SIZE_C/2.0, 0.0);
  glScalef(LEG_GIRTH_C, LOWER_LEG_SIZE_C, LEG_GIRTH_C);
  if (solid)
    glutSolidCube(1.0);
  else
    glutWireCube(1.0);
  glPopMatrix();

  /* Foot: rotates about the x axis only */
  glColor3f(0.0, 0.0, 1.0);

  glTranslatef(0.0, -(UPPER_LEG_SIZE_C+LOWER_LEG_SIZE_C+LEG_GIRTH_C)/2.0, 0.0);
  glRotatef(Walk_cycle[which][2][Step], 1.0, 0.0, 0.0);
  glPushMatrix();
  glTranslatef(0.0, -LEG_GIRTH_C/2.0, -FOOT_SIZE_C/4.0);
  glScalef(LEG_GIRTH_C, LEG_GIRTH_C, FOOT_SIZE_C);
  if (solid)
    glutSolidCube(1.0);
  else
    glutWireCube(1.0);
  glPopMatrix();

  glPopMatrix();
}
 
/*********************************************************************/
void draw_arm_C(int which, int solid)
{
  int arm_which;

  if (which == 1)
    arm_which = 1;
  else arm_which = 0;

  glPushMatrix();
  glTranslatef(0.0, TORSO_HEIGHT_C, 0.0);
  if (which == 0)
    glTranslatef(TORSO_WIDTH_C/1.5, 0.0, 0.0);
  else glTranslatef(-TORSO_WIDTH_C/1.5, 0.0, 0.0);
  /* Upper leg: rotates about the x axis only */
  glColor3f(1.0, 0.0, 0.0);
  glRotatef(Walk_cycle[arm_which][3][Step],1.0, 0.0, 0.0);
  glPushMatrix();
  glTranslatef(0.0, -UPPER_ARM_SIZE_C/2.0, 0.0);
  glScalef(ARM_GIRTH_C, UPPER_ARM_SIZE_C, ARM_GIRTH_C);
  if (solid)
    glutSolidCube(1.0);
  else
    glutWireCube(1.0);
  glPopMatrix();

  /* Lower leg: rotates about the x axis only */
  glColor3f(0.0, 1.0, 0.0);
  glTranslatef(0.0, -(UPPER_ARM_SIZE_C+LOWER_ARM_SIZE_C)/2.0, 0.0);
  glRotatef(Walk_cycle[arm_which][4][Step], 1.0, 0.0, 0.0);
  glPushMatrix();
  glTranslatef(0.0, -LOWER_ARM_SIZE_C/2.0, 0.0);
  glScalef(ARM_GIRTH_C, LOWER_ARM_SIZE_C, ARM_GIRTH_C);
  if (solid)
    glutSolidCube(1.0);
  else
    glutWireCube(1.0);
  glPopMatrix();

  glPopMatrix();
}




GLUquadricObj *quadObj;
GLuint body_lists;

/**************************************************************/
void StoreTheGuy_SL(void)
{
  quadObj = gluNewQuadric();

  body_lists = glGenLists(NUM_BODY_PARTS);

  glNewList(body_lists, GL_COMPILE);
  store_head_SL();
  glEndList();

  glNewList(body_lists+1, GL_COMPILE);
  store_torso_SL();
  glEndList();

  glNewList(body_lists+2, GL_COMPILE);
  store_uleg_SL();
  glEndList();

  glNewList(body_lists+3, GL_COMPILE);
  store_lleg_SL();
  glEndList();

  glNewList(body_lists+4, GL_COMPILE);
  store_foot_SL();
  glEndList();

  glNewList(body_lists+5, GL_COMPILE);
  store_uarm_SL();
  glEndList();

  glNewList(body_lists+6, GL_COMPILE);
  store_larm_SL();
  glEndList();

}

/**********************************************************/
void store_head_SL(void)
{ 
  glPushMatrix();
  glTranslatef(0.0, TORSO_HEIGHT+HEAD_SIZE, 0.0);
  glScalef(HEAD_SIZE, HEAD_SIZE, LEG_GIRTH);
  glutSolidSphere(1.0, SLICES, STACKS);
  glPopMatrix();
}

/**********************************************************/
void store_torso_SL(void)
{
  glPushMatrix();
  glScalef(TORSO_WIDTH, TORSO_HEIGHT, LEG_GIRTH);
  glRotatef(-90.0, 1.0, 0.0, 0.0);
  gluCylinder(quadObj, TORSO_TAPER, 1.0, 1.0, SLICES, STACKS);
  glPopMatrix();
}

/**************************************************************/
void store_uleg_SL(void)
{
  glPushMatrix();
  glScalef(LEG_GIRTH, UPPER_LEG_SIZE, LEG_GIRTH);
  glRotatef(90.0, 1.0, 0.0, 0.0);
  gluCylinder(quadObj, 1.0, 1.0, 1.0, SLICES, STACKS);
  glPopMatrix();
}
  
/**************************************************************/
void store_lleg_SL(void)
{
  glPushMatrix();
  glScalef(LEG_GIRTH, LOWER_LEG_SIZE, LEG_GIRTH);
  glRotatef(90.0, 1.0, 0.0, 0.0);
  gluCylinder(quadObj, 1.0, 1.0, 1.0, SLICES, STACKS);
  glPopMatrix();
}
  
/**************************************************************/
void store_foot_SL(void)
{
  glPushMatrix();
  glTranslatef(0.0, 0.0, -FOOT_SIZE/2.0);
  glScalef(LEG_GIRTH, LEG_GIRTH, FOOT_SIZE);
  glRotatef(90.0, 1.0, 0.0, 0.0);
  gluCylinder(quadObj, 1.0, 1.0, 1.0, SLICES, STACKS);
  glPopMatrix();
}
  
/**************************************************************/
void store_uarm_SL(void)
{
  glPushMatrix();
  glScalef(ARM_GIRTH, UPPER_ARM_SIZE, ARM_GIRTH);
  glRotatef(90.0, 1.0, 0.0, 0.0);
  gluCylinder(quadObj, 1.0, 1.0, 1.0, SLICES, STACKS);
  glPopMatrix();
}
  
/**************************************************************/
void store_larm_SL(void)
{
  glPushMatrix();
  glScalef(ARM_GIRTH, LOWER_ARM_SIZE, ARM_GIRTH);
  glRotatef(90.0, 1.0, 0.0, 0.0);
  gluCylinder(quadObj, 1.0, 1.0, 1.0, SLICES, STACKS);
  glPopMatrix();
}
  
/**************************************************************/
void DrawTheGuy_SL(void)
{
  GLfloat head_diffuse[] =  { 0.7, 0.7, 0.0, 1.0 };
  GLfloat torso_diffuse[] = { 0.0, 0.7, 0.7, 1.0 };
  GLfloat leg_diffuse[] =   { 0.7, 0.0, 0.7, 1.0 };
  GLfloat arm_diffuse[] =   { 0.7, 0.4, 0.4, 1.0 };

  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, head_diffuse);
  draw_head_SL();

  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, torso_diffuse);
  draw_torso_SL();

  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, leg_diffuse);
  draw_leg_SL(0);
  draw_leg_SL(1);

  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, arm_diffuse);
  draw_arm_SL(0);
  draw_arm_SL(1);

}

/**********************************************************/
void draw_head_SL(void)
{
  glPushMatrix();
  glCallList(body_lists);
  glPopMatrix();
}

/**********************************************************/
void draw_torso_SL(void)
{
  glPushMatrix();
  glCallList(body_lists+1);
  glPopMatrix();
}

/**********************************************************/
void draw_leg_SL(int which)
{
  glPushMatrix();
  if (which == 0)
    glTranslatef(TORSO_TAPER*TORSO_WIDTH/2.0, 0.0, 0.0);
  else glTranslatef(-TORSO_TAPER*TORSO_WIDTH/2.0, 0.0, 0.0);
  /* Upper leg: rotates about the x axis only */
  glRotatef(Walk_cycle[which][0][Step],1.0, 0.0, 0.0);
  glPushMatrix();
  glCallList(body_lists+2);
  glPopMatrix();

  /* Lower leg: rotates about the x axis only */
  glTranslatef(0.0, -(UPPER_LEG_SIZE+LOWER_LEG_SIZE)/2.0, 0.0);
  glRotatef(Walk_cycle[which][1][Step], 1.0, 0.0, 0.0);
  glPushMatrix();
  glCallList(body_lists+3);
  glPopMatrix();

  /* Foot: rotates about the x axis only */
  glTranslatef(0.0, -(UPPER_LEG_SIZE+LOWER_LEG_SIZE+LEG_GIRTH)/2.0, 0.0);
  glRotatef(Walk_cycle[which][2][Step], 1.0, 0.0, 0.0);
  glPushMatrix();
  glCallList(body_lists+4);
  glPopMatrix();

  glPopMatrix();
}
 
/*********************************************************************/
void draw_arm_SL(int which)
{
  int arm_which;

  if (which == 1)
    arm_which = 1;
  else arm_which = 0;

  glPushMatrix();
  glTranslatef(0.0, TORSO_HEIGHT, 0.0);
  if (which == 0)
    glTranslatef(TORSO_WIDTH, 0.0, 0.0);
  else glTranslatef(-TORSO_WIDTH, 0.0, 0.0);
  /* Upper leg: rotates about the x axis only */
  glRotatef(Walk_cycle[arm_which][3][Step],1.0, 0.0, 0.0);
  glPushMatrix();
  glCallList(body_lists+5);
  glPopMatrix();

  /* Lower leg: rotates about the x axis only */
  glTranslatef(0.0, -(UPPER_ARM_SIZE+LOWER_ARM_SIZE)/2.0, 0.0);
  glRotatef(Walk_cycle[arm_which][4][Step], 1.0, 0.0, 0.0);
  glPushMatrix();
  glCallList(body_lists+6);
  glPopMatrix();

  glPopMatrix();
}
 


GLUquadricObj *quadObj2;
GLuint body_lists2;

/**************************************************************/
void StoreTheGuy_SL2(void)
{
  quadObj2 = gluNewQuadric();

  body_lists2 = glGenLists(NUM_BODY_PARTS);

  glNewList(body_lists2, GL_COMPILE);
  store_head_SL2();
  glEndList();

  glNewList(body_lists2+1, GL_COMPILE);
  store_torso_SL2();
  glEndList();

  glNewList(body_lists2+2, GL_COMPILE);
  store_uleg_SL2();
  glEndList();

  glNewList(body_lists2+3, GL_COMPILE);
  store_lleg_SL2();
  glEndList();

  glNewList(body_lists2+4, GL_COMPILE);
  store_foot_SL2();
  glEndList();

  glNewList(body_lists2+5, GL_COMPILE);
  store_uarm_SL2();
  glEndList();

  glNewList(body_lists2+6, GL_COMPILE);
  store_larm_SL2();
  glEndList();

}

/**********************************************************/
void store_head_SL2(void)
{ 
  glPushMatrix();
  glTranslatef(0.0, TORSO_HEIGHT+HEAD_SIZE, 0.0);
  glScalef(HEAD_SIZE, HEAD_SIZE, UPPER_LEG_GIRTH);
  glutSolidSphere(1.0, SLICES, STACKS);
  glPopMatrix();
}

/**********************************************************/
void store_torso_SL2(void)
{
  glPushMatrix();
  glScalef(TORSO_WIDTH, TORSO_HEIGHT, UPPER_LEG_GIRTH);
  glRotatef(-90.0, 1.0, 0.0, 0.0);
  gluCylinder(quadObj2, TORSO_TAPER, 1.0, 1.0, SLICES, STACKS);
  glPopMatrix();
}

/**************************************************************/
void store_uleg_SL2(void)
{
  glPushMatrix();
  glTranslatef(0.0, -(HIP_JOINT_SIZE+UPPER_LEG_SIZE), 0.0);
  glutSolidSphere(KNEE_JOINT_SIZE, SLICES, STACKS);
  glPopMatrix();
  glTranslatef(0.0, -HIP_JOINT_SIZE, 0.0);
  glutSolidSphere(HIP_JOINT_SIZE, SLICES, STACKS);
  glPushMatrix();
  glScalef(UPPER_LEG_GIRTH, UPPER_LEG_SIZE, UPPER_LEG_GIRTH);
  glRotatef(90.0, 1.0, 0.0, 0.0);
  gluCylinder(quadObj2, 1.0, UPPER_LEG_TAPER, 1.0, SLICES, STACKS);
  glPopMatrix();
}
  
/**************************************************************/
void store_lleg_SL2(void)
{
  glPushMatrix();
  glScalef(LOWER_LEG_GIRTH, LOWER_LEG_SIZE, LOWER_LEG_GIRTH);
  glRotatef(90.0, 1.0, 0.0, 0.0);
  gluCylinder(quadObj2, 1.0, LOWER_LEG_TAPER, 1.0, SLICES, STACKS);
  glPopMatrix();
}
  
/**************************************************************/
void store_foot_SL2(void)
{
  glPushMatrix();
  glTranslatef(0.0, 0.0, -FOOT_SIZE/2.0);
  glScalef(LOWER_LEG_GIRTH, LOWER_LEG_GIRTH, FOOT_SIZE);
  glRotatef(90.0, 1.0, 0.0, 0.0);
  gluCylinder(quadObj2, 1.0, 1.0, 1.0, SLICES, STACKS);
  glPopMatrix();
}
  
/**************************************************************/
void store_uarm_SL2(void)
{
  glPushMatrix();
  glTranslatef(0.0, -(SHOULDER_JOINT_SIZE+UPPER_ARM_SIZE), 0.0);
  glutSolidSphere(ELBOW_JOINT_SIZE, SLICES, STACKS);
  glPopMatrix();
  glTranslatef(0.0, -SHOULDER_JOINT_SIZE, 0.0);
  glutSolidSphere(SHOULDER_JOINT_SIZE, SLICES, STACKS);
  glPushMatrix();
  glScalef(UPPER_ARM_GIRTH, UPPER_ARM_SIZE, UPPER_ARM_GIRTH);
  glRotatef(90.0, 1.0, 0.0, 0.0);
  gluCylinder(quadObj2, 1.0, UPPER_ARM_TAPER, 1.0, SLICES, STACKS);
  glPopMatrix();
}
  
/**************************************************************/
void store_larm_SL2(void)
{
  glPushMatrix();
  glScalef(LOWER_ARM_GIRTH, LOWER_ARM_SIZE, LOWER_ARM_GIRTH);
  glRotatef(90.0, 1.0, 0.0, 0.0);
  gluCylinder(quadObj2, 1.0, LOWER_ARM_TAPER, 1.0, SLICES, STACKS);
  glPopMatrix();
}

/**************************************************************/
void DrawTheGuy_SL2(void)
{
  GLfloat head_diffuse[] = { 0.7, 0.7, 0.0, 1.0 };
  GLfloat torso_diffuse[] = { 0.0, 0.7, 0.7, 1.0 };
  GLfloat leg_diffuse[] = { 0.7, 0.0, 0.7, 1.0 };
  GLfloat arm_diffuse[] = { 0.7, 0.4, 0.4, 1.0 };

  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, head_diffuse);
  draw_head_SL2();

  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, torso_diffuse);
  draw_torso_SL2();

  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, leg_diffuse);
  draw_leg_SL2(0);
  draw_leg_SL2(1);

  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, arm_diffuse);
  draw_arm_SL2(0);
  draw_arm_SL2(1);
 
}

/**********************************************************/
void draw_head_SL2(void)
{
  glPushMatrix();
  glCallList(body_lists2);
  glPopMatrix();
}

/**********************************************************/
void draw_torso_SL2(void)
{
  glPushMatrix();
  glCallList(body_lists2+1);
  glPopMatrix();
}

/**********************************************************/
void draw_leg_SL2(int which)
{
  glPushMatrix();
  if (which == 0)
    glTranslatef(TORSO_TAPER*TORSO_WIDTH/2.0, 0.0, 0.0);
  else glTranslatef(-TORSO_TAPER*TORSO_WIDTH/2.0, 0.0, 0.0);
  /* UPPER leg: rotates about the x axis only */
  glRotatef(Walk_cycle[which][0][Step],1.0, 0.0, 0.0);
  glPushMatrix();
  glCallList(body_lists2+2);
  glPopMatrix();

  /* LOWER leg: rotates about the x axis only */
  glTranslatef(0.0, -(UPPER_LEG_SIZE+KNEE_JOINT_SIZE), 0.0);
  glRotatef(Walk_cycle[which][1][Step], 1.0, 0.0, 0.0);
  glPushMatrix();
  glCallList(body_lists2+3);
  glPopMatrix();

  /* Foot: rotates about the x axis only */
  glTranslatef(0.0, -(UPPER_LEG_SIZE+LOWER_LEG_SIZE+LOWER_LEG_GIRTH)/2.0, 0.0);
  glRotatef(Walk_cycle[which][2][Step], 1.0, 0.0, 0.0);
  glPushMatrix();
  glCallList(body_lists2+4);
  glPopMatrix();

  glPopMatrix();
}
 
/*********************************************************************/
void draw_arm_SL2(int which)
{
  int arm_which;

  if (which == 1)
    arm_which = 1;
  else arm_which = 0;

  glPushMatrix();
  glTranslatef(0.0, TORSO_HEIGHT, 0.0);
  if (which == 0)
    glTranslatef(TORSO_WIDTH, 0.0, 0.0);
  else glTranslatef(-TORSO_WIDTH, 0.0, 0.0);
  /* UPPER leg: rotates about the x axis only */
  glRotatef(Walk_cycle[arm_which][3][Step],1.0, 0.0, 0.0);
  glPushMatrix();
  glCallList(body_lists2+5);
  glPopMatrix();

  /* LOWER leg: rotates about the x axis only */
  glTranslatef(0.0, -(UPPER_ARM_SIZE+ELBOW_JOINT_SIZE), 0.0);
  glRotatef(Walk_cycle[arm_which][4][Step], 1.0, 0.0, 0.0);
  glPushMatrix();
  glCallList(body_lists2+6);
  glPopMatrix();

  glPopMatrix();
}
 
