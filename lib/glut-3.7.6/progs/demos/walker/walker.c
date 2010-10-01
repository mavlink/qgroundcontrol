#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#ifdef _WIN32
#include "win32_dirent.h"
/* Have to #undef LoadMenu or else Microsoft VC++ won't allow us to
   redefine it. */
#undef LoadMenu
#else
#include <dirent.h>
#endif
#include <assert.h>

#include "walker.h"



#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define MIN(x,y) ((x) < (y) ? (x) : (y))

#define CYCLE_SIZE 100
#define CYCLE_STEP 1.0/CYCLE_SIZE
#define OVERSAMPLE 10

#define MAX_CPOINTS 34   /* 2 end point ones and 10 in the middle */

#define MAX_CSETNAMELEN 25

#define NUM_JOINTS 5

#define CSET_EXT ".cset"
#define CSET_EXTLEN 5

#include "walkviewer.h"


typedef enum { CMENU_QUIT, CMENU_CURVE, CMENU_HORIZ, CMENU_RESET,
               CMENU_WALK, CMENU_DONEEDIT, CMENU_SAVE, CMENU_LOAD,
	       CMENU_MIRROR
              } CurveMenuChoices;

typedef enum { WIRECUBE, SOLIDCUBE, CYLINDER1, CYLINDER2 } ModelTypes;

int GuyModel = SOLIDCUBE;

/***************************************************************/
/*************************** GLOBALS ***************************/
/***************************************************************/

GLuint HorizontalList, AxesList,     /* Display lists */
       CurveLists, ControlPtsLists;  /* Firsts of groups of display lists */

int CurveAsPoints   =  0,    /* Display curve as points? */
    DrawHorizontals =  0,    /* Draw horizontal lines?   */
    EditingCurve    = -1;    /* Editing what curve, -1 means none */

int CurveWindow,   /* Glut window id's to two top level windows */
    GuyWindow;

int MirrorLegs = 0;

int CurveMenu     = -1,
    CurveEditMenu = -1,
    StepSizeMenu  = -1,
    LoadMenu      = -1,
    SaveMenu      = -1;

char *CSetNames[MAX_CSETNAMELEN];

int CurrentCurve = -1;  /* Curve loaded, index to CSetNames */

GLfloat Walk_cycle[2][NUM_JOINTS][CYCLE_SIZE];  /* array of computed angles */

int Step = CYCLE_SIZE/2;  /* Position in cycle, start in middle */

float fStep = CYCLE_SIZE/2;   /* floating point for non-integer steping */
float IncStep = 1.0;

typedef struct ControlPts {
  int numpoints;
  float xcoords[MAX_CPOINTS];
  float angles[MAX_CPOINTS];
} tControlPts;

tControlPts RotCurve[NUM_JOINTS];   /* series of cntrl points for ea joint */

int Walking         = 0,     /* Guy is walking? */
    ViewPerspective = 1,     /* Perspective or orthographic views */
    DrawAxes        = 0;     /* Draw axes for alignment */

int CurveWWidth,             /* Dimensions of curve window */
    CurveWHeight;  

int CurveDownBtn = -1,               /* mouse stuff, for editing curves */
    WasWalking,
    CurvePickedPoint = -1,
    CurveLastX,
    CurveLastY;

int CurveWindowVisible = 1;


  /* prototypes */
void RedisplayBoth(void);
void IncrementStep(void);
void CurveCPointDrag(void);
void CurveHandleMenu(int value);
void StopWalking(void);
void CurveHandleEditMenu(int curve);
void ComputeCSetAndMakeLists(void);
int MakeLoadAndSaveMenus(void);
void CurveMenuInit(void);
void SetWindowTitles(char *csetname);

/***************************************************************/
/**************************** BEZIER ***************************/
/***************************************************************/

  /* Matrix times a vector  dest = m*v */
void MultMV(float m[3][4], float v[4], float dest[3])
{
  int i, j;

  for (i = 0; i < 3; i++) {
    dest[i] = 0;
    for (j = 0; j < 4; j++)
      dest[i] += m[i][j] * v[j];
  }
}

  /* Matrix multiplication, dest = m1*m2 */
void MultM(float m1[3][4], float m2[4][4], float dest[3][4])
{
  int i, j, k;

  for (i = 0; i < 3; i++)
    for (j = 0; j < 4; j++) {
      dest[i][j] = 0;
      for (k = 0; k < 4; k++)
	  dest[i][j] += (m1[i][k] * m2[k][j]);
    }
}

void ComputeCurve(int joint)
{
  float prod[3][4], tm[4], pos[3];
  float t = 0, tinc = (float)CYCLE_STEP/OVERSAMPLE;
  int ctlpoint, i;
  float BBasis[4][4] = {{-1, 3, -3, 1}, {3, -6, 3, 0},
                        {-3, 3,  0, 0}, {1,  0, 0, 0}};
  int lastindex, newindex;

  float pointset[3][4];
  for (i = 0; i < 4; i++)   /* z's are always zero, only 2-d */
    pointset[2][i] = 0; 

  lastindex = -1;
  for(ctlpoint = 0; ctlpoint < RotCurve[joint].numpoints; ctlpoint += 3) {
    t = 0;
    for (i = 0; i < 4; i++)
      pointset[0][i] = RotCurve[joint].xcoords[ctlpoint + i];
    for (i = 0; i < 4; i++)
      pointset[1][i] = RotCurve[joint].angles[ctlpoint + i];

    MultM(pointset, BBasis, prod);

    while (t <= 1) {
      tm[0] = t*t*t;
      tm[1] = t*t;
      tm[2] = t;
      tm[3] = 1;
      MultMV(prod, tm, pos);
      newindex = (int)(pos[0]*(CYCLE_SIZE-1));
      if ((int)(newindex > lastindex))  {  /* go at least one */
	Walk_cycle[0][joint][newindex] = pos[1];
	lastindex++;
      } 
      t += tinc;
    }
  }

  for (i = 0; i < CYCLE_SIZE; i++) {      /* copy to other leg, out-o-phase */
    if (MirrorLegs)
      Walk_cycle[1][joint][i] =
        Walk_cycle[0][joint][i];
    else
      Walk_cycle[1][joint][i] =
        Walk_cycle[0][joint][(i+(CYCLE_SIZE/2))%CYCLE_SIZE];
  }
}

/***************************************************************/
/************************* CURVE I/O ***************************/
/***************************************************************/

void FlatCSet(void)
{
  int joint;

  for (joint = 0; joint < NUM_JOINTS; joint++) {
    RotCurve[joint].numpoints = 4;
    RotCurve[joint].xcoords[0] = 0.0;
    RotCurve[joint].angles[0]  = 0.0;
    RotCurve[joint].xcoords[1] = 0.2;
    RotCurve[joint].angles[1]  = 0.0;
    RotCurve[joint].xcoords[2] = 0.8;
    RotCurve[joint].angles[2]  = 0.0;
    RotCurve[joint].xcoords[3] = 1.0;
    RotCurve[joint].angles[3]  = 0.0;
  }
}

int ReadCSetFromFile(char *filename)
{
  FILE *infile = fopen(filename, "r");
  int numjoints, numpoints, joint, point, mirrorlegs;
  float value;

  if (infile == NULL)
    goto abort;
  
  if (!fscanf(infile, " %d", &numjoints) || numjoints != NUM_JOINTS)
    goto abort;

  if (!fscanf(infile, " %d", &mirrorlegs) || (mirrorlegs != 0 &&
                                              mirrorlegs != 1))
    goto abort;

  MirrorLegs = mirrorlegs;

  for (joint = 0; joint < NUM_JOINTS; joint++) {
    if (!fscanf(infile, " %d", &numpoints) || numpoints < 4 || 
                                              numpoints > MAX_CPOINTS)
      goto abort;
    RotCurve[joint].numpoints = numpoints;
    for (point = 0; point < numpoints; point++) {
      if (!fscanf(infile, " %f", &value))
	goto abort;
      RotCurve[joint].xcoords[point] = value;
    }
    for (point = 0; point < numpoints; point++) {
      if (!fscanf(infile, " %f", &value))
	goto abort;
      RotCurve[joint].angles[point] = value;
    }
  }

  fclose(infile);
  return 0;

  abort:
    fclose(infile);
    fprintf(stderr, "Something went wrong while reading file %s\n", filename);
    FlatCSet();
    return -1;
}

void WriteCSetToFile(char *filename)
{
  FILE *outfile = fopen(filename, "w+");
  int joint, point;

  if (outfile == NULL) {
    fprintf(stderr, "Error: could not create file %s\n", filename);
    return;
  }

  fprintf(outfile, "%d\n", NUM_JOINTS);

  fprintf(outfile, "%d\n", MirrorLegs);

  for (joint = 0; joint < NUM_JOINTS; joint++) {

    fprintf(outfile, "%d\n", RotCurve[joint].numpoints);

    for (point = 0; point < RotCurve[joint].numpoints; point++) {
      fprintf(outfile, "%f ", RotCurve[joint].xcoords[point]);
    }
    fprintf(outfile, "\n");

    for (point = 0; point < RotCurve[joint].numpoints; point++) {
      fprintf(outfile, "%f ", RotCurve[joint].angles[point]);
    }
    fprintf(outfile, "\n");
  }

  fclose(outfile);
}

void HandleLoadMenu(int cset)
{
  char filename[MAX_CSETNAMELEN + CSET_EXTLEN + 1];

  if (cset == -1) {
    MakeLoadAndSaveMenus();
    CurveMenuInit();
  } else {
    (void)strcpy(filename, CSetNames[cset]);
    (void)strcat(filename, CSET_EXT);
    if (ReadCSetFromFile(filename) == 0) {
      glutSetMenu(SaveMenu);
      glutChangeToMenuEntry(1, CSetNames[cset], cset);
      ComputeCSetAndMakeLists();
      SetWindowTitles(CSetNames[cset]);
      RedisplayBoth();
    }
  }
}

void HandleSaveMenu(int cset)
{
  char filename[MAX_CSETNAMELEN + CSET_EXTLEN + 1];

  (void)strcpy(filename, CSetNames[cset]);
  (void)strcat(filename, CSET_EXT);
  WriteCSetToFile(filename);
  ComputeCSetAndMakeLists();
  RedisplayBoth();
}

int MakeLoadAndSaveMenus(void)
{
  DIR *dirp = opendir(".");
  struct dirent *direntp;
  int csetnum = 0;
  char *newcsetname;

  if (LoadMenu != -1)
    glutDestroyMenu(LoadMenu);
  if (SaveMenu != -1)
    glutDestroyMenu(SaveMenu);

  SaveMenu = glutCreateMenu(HandleSaveMenu);
  LoadMenu = glutCreateMenu(HandleLoadMenu);

  if (dirp == NULL) {
    fprintf(stderr, "Error opening current dir in MakeLoadAndSaveMenus\n");
    return(0);
  }

  while ((direntp = readdir(dirp)) != NULL) {
    char *ext = direntp->d_name + (strlen(direntp->d_name) - CSET_EXTLEN);
    if (!strcmp(ext, CSET_EXT)) {
      newcsetname = malloc(strlen(direntp->d_name) - CSET_EXTLEN + 1);
      strncpy(newcsetname, direntp->d_name,
	      strlen(direntp->d_name) - CSET_EXTLEN);
      newcsetname[strlen(direntp->d_name) - CSET_EXTLEN] = 0;
      CSetNames[csetnum] = newcsetname;
      glutAddMenuEntry(newcsetname, csetnum++);
    }
  }
  closedir(dirp);
  glutSetMenu(LoadMenu);
  glutAddMenuEntry("-> Rescan Directory <-", -1);
  glutSetMenu(SaveMenu);
  CSetNames[csetnum] = "NewCurve0";
  glutAddMenuEntry("NewCurve0", csetnum++);
  CSetNames[csetnum] = "NewCurve1";
  glutAddMenuEntry("NewCurve1", csetnum++);
  CSetNames[csetnum] = "NewCurve2";
  glutAddMenuEntry("NewCurve2", csetnum++);

  return (csetnum - 2);  /* just indicate curves in Load menu */
}


/***************************************************************/
/********************* DISPLAY LISTS ***************************/
/***************************************************************/


void MakeCurveList(int joint)
{
  int i;

  glNewList(CurveLists+joint, GL_COMPILE);
  glColor3f(1, 1, 1);
  for (i = 0; i < CYCLE_SIZE; i++) {
    glVertex3f((GLfloat)i/CYCLE_SIZE, Walk_cycle[0][joint][i]/180, 0);
  }
  glEndList();
}

void MakeCPointList(int joint)
{
  int point;
 
  glNewList(ControlPtsLists+joint, GL_COMPILE);

    glColor3f(0, 0.4, 0);
    glBegin(GL_LINE_STRIP);
    for (point = 0; point < RotCurve[joint].numpoints; point++) {
      if (!((point-2) % 3)) {
        glEnd();
        glBegin(GL_LINE_STRIP);
      }
      glVertex3f(RotCurve[joint].xcoords[point],
	         (RotCurve[joint].angles[point])/180.0, 0.0);
    }
    glEnd();

    glBegin(GL_POINTS);
    for (point = 0; point < RotCurve[joint].numpoints; point++) {
      if (point % 3)
        glColor3f(0, 0.7, 0);
      else
        glColor3f(0.7, 0.0, 0);
      glVertex3f(RotCurve[joint].xcoords[point],
	         (RotCurve[joint].angles[point])/180, 0);
    }
    glEnd();

  glEndList();
}

void MakeJointLists(int joint)
{
  MakeCurveList(joint);
  MakeCPointList(joint);
}


void ComputeCSetAndMakeLists(void)
{
  int joint;

  for(joint = 0; joint < NUM_JOINTS; joint++) {
    ComputeCurve(joint);
    MakeJointLists(joint);
  }
}

void MakeLists(void)
{
  HorizontalList = glGenLists(1);
  glNewList(HorizontalList, GL_COMPILE);
  {
    float line1 = 25.0/180,
          line2 = 35.0/180,
          line3 = 45.0/180;
    glColor3f(0, 0, 0.7);
    glPushAttrib(GL_ENABLE_BIT);
    glEnable(GL_LINE_STIPPLE);
    glBegin(GL_LINES);
      glVertex3f(0, 0, 0.5);
      glVertex3f(1, 0, 0.5);
      glVertex3f(0, line1, 0.5);  glVertex3f(1, line1, 0.5);
      glVertex3f(0, -line1, 0.5); glVertex3f(1, -line1, 0.5);
      glVertex3f(0, line2, 0.5);  glVertex3f(1, line2, 0.5);
      glVertex3f(0, -line2, 0.5); glVertex3f(1, -line2, 0.5);
      glVertex3f(0, line3, 0.5);  glVertex3f(1, line3, 0.5);
      glVertex3f(0, -line3, 0.5); glVertex3f(1, -line3, 0.5);
    glEnd();
    glPopAttrib();
  }
  glEndList();


  CurveLists = glGenLists(NUM_JOINTS);
  assert(CurveLists != 0);

  ControlPtsLists = glGenLists(NUM_JOINTS);
  assert(ControlPtsLists != 0);

  ComputeCSetAndMakeLists();
}

/***************************************************************/
/********************* curve WINDOW ****************************/
/***************************************************************/

void CurveReshape(int w, int h)
{
  glViewport(0,0,w,h);
  CurveWWidth = w; 
  CurveWHeight = h;
  glFlush();
}

void CurveDisplay(void)
{
  int joint, otherlegstep;

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glPushMatrix();
  glTranslatef(0, 0.5, 0);

  for (joint = NUM_JOINTS-1; joint >= 0; joint--) {

    if (DrawHorizontals) 
      glCallList(HorizontalList);

    (CurveAsPoints) ?
    glBegin(GL_POINTS) :
    glBegin(GL_LINE_STRIP);
      glCallList(CurveLists+joint);
    glEnd();

    if (joint == EditingCurve) { 
      glPointSize(5.0);
      glCallList(ControlPtsLists+EditingCurve);
      glPointSize(1.0);
    }
  glTranslatef(0, 1, 0);
  }

  glPopMatrix();

  otherlegstep = (Step+50) % CYCLE_SIZE;
    /* draw vertical line */
  glColor3f(1, 1, 1);
  glBegin(GL_LINES);
    glVertex3f((GLfloat)Step/CYCLE_SIZE, 0, 0);
    glVertex3f((GLfloat)Step/CYCLE_SIZE, NUM_JOINTS, 0);
    if (!MirrorLegs) {
      glVertex3f((GLfloat)otherlegstep/CYCLE_SIZE, 0, 0);
      glVertex3f((GLfloat)otherlegstep/CYCLE_SIZE, NUM_JOINTS, 0);
    }
  glEnd();

  glFlush();
  glutSwapBuffers();
}


void CurveGLInit(void)
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0,1,0,NUM_JOINTS,1,-1);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glLineStipple(1, 0x00FF);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);


  glFlush();

} 


/* ARGSUSED2 */
void CurveHandleButton(int button, int state, int x, int y)
{
  if (button == GLUT_RIGHT_BUTTON )
    return;

  if (state == GLUT_DOWN && CurveDownBtn == -1) {
    if (button == GLUT_MIDDLE_BUTTON)
      CurveHandleMenu(CMENU_WALK);
    else
      fStep = Step = (int)((float)x/glutGet(GLUT_WINDOW_WIDTH) * CYCLE_SIZE);
      
    CurveDownBtn = button;

    WasWalking = Walking;
    StopWalking();
    RedisplayBoth();
  } else if (button == CurveDownBtn) {
    CurveDownBtn = -1;
    if (WasWalking) {
      Walking = 1;
      agvSetAllowIdle(0);
      glutIdleFunc(IncrementStep);	
    }
  }
}

float CurveEditConstrain(float fx)
{
  if (CurvePickedPoint == 0)
    fx = 0;
  else if (CurvePickedPoint == RotCurve[EditingCurve].numpoints-1)
    fx = 1;
  else if (!(CurvePickedPoint % 3)) {  /* is a pivot */
    fx = MAX(fx, RotCurve[EditingCurve].xcoords[CurvePickedPoint-1]);
    fx = MIN(fx, RotCurve[EditingCurve].xcoords[CurvePickedPoint+1]);   
    fx = MAX(fx, RotCurve[EditingCurve].xcoords[CurvePickedPoint-3]);
    fx = MIN(fx, RotCurve[EditingCurve].xcoords[CurvePickedPoint+3]);       
  } else if (!((CurvePickedPoint - 1) % 3)) {   /* is right slope */
    fx = MAX(fx, RotCurve[EditingCurve].xcoords[CurvePickedPoint-1]);
  } else {
    fx = MIN(fx, RotCurve[EditingCurve].xcoords[CurvePickedPoint+1]);
  } 
  return fx;
}

void RemovePoint(int pt)
{
  int i;

  for (i = pt - 1; i < RotCurve[EditingCurve].numpoints; i++) {
    RotCurve[EditingCurve].xcoords[i] = RotCurve[EditingCurve].xcoords[i+3];
    RotCurve[EditingCurve].angles[i]  = RotCurve[EditingCurve].angles[i+3];
  }
  RotCurve[EditingCurve].numpoints -= 3;
}

void AddPoint(float fx)
{
  int i, j;

  if (fx < 0.05 || fx > 0.95 || RotCurve[EditingCurve].numpoints + 3 >=
                                MAX_CPOINTS)
    return;

  for (i = 3; i < RotCurve[EditingCurve].numpoints; i += 3) {
    if (fx < RotCurve[EditingCurve].xcoords[i]) {
      for (j = RotCurve[EditingCurve].numpoints + 2; j > i + 1; j--) { 
        RotCurve[EditingCurve].xcoords[j] =
                RotCurve[EditingCurve].xcoords[j-3];
        RotCurve[EditingCurve].angles[j] =
                RotCurve[EditingCurve].angles[j-3];
      }
    RotCurve[EditingCurve].xcoords[i]   = fx;
    RotCurve[EditingCurve].angles[i]    =
      Walk_cycle[0][EditingCurve][(int)(fx*CYCLE_SIZE)];
    RotCurve[EditingCurve].xcoords[i-1] = fx - 0.05;
    RotCurve[EditingCurve].angles[i-1]  =
      Walk_cycle[0][EditingCurve][(int)((fx-0.05)*CYCLE_SIZE)];
    RotCurve[EditingCurve].xcoords[i+1] = fx + 0.05;
    RotCurve[EditingCurve].angles[i+1]  = 
      Walk_cycle[0][EditingCurve][(int)((fx+0.05)*CYCLE_SIZE)];
    RotCurve[EditingCurve].numpoints += 3;
    break;
    }
  }  
}


void CurveEditHandleButton(int button, int state, int x, int y)
{
  float fx, fy;
  int point;

  fy = -(((float)y - ((float)CurveWHeight/NUM_JOINTS * EditingCurve)) /
       ((float)CurveWHeight/NUM_JOINTS) - 0.5) * 180.0,
  fx = (float)x/CurveWWidth;
  
  if (state == GLUT_DOWN && button == GLUT_LEFT_BUTTON &&
                            CurveDownBtn == -1) {
    CurvePickedPoint = -1;
    
    for (point = 0; point < RotCurve[EditingCurve].numpoints; point++) {
      if (fabs(RotCurve[EditingCurve].xcoords[point] - fx) < 0.01 &&
	  fabs(RotCurve[EditingCurve].angles[point] - fy) < 4) {
	CurvePickedPoint = point;
	CurveLastX = x;
	CurveLastY = y;
	glutIdleFunc(CurveCPointDrag);
	break;
      }
    }
   if (CurvePickedPoint == -1)
     CurveHandleButton(button, state, x, y);
    CurveDownBtn = button;


  } else if (state == GLUT_DOWN && button == GLUT_MIDDLE_BUTTON &&
                                   CurveDownBtn == -1) {

    for (point = 3; point < RotCurve[EditingCurve].numpoints - 1; point += 3) {
      if (fabs(RotCurve[EditingCurve].xcoords[point] - fx) < 0.01 &&
	  fabs(RotCurve[EditingCurve].angles[point] - fy) < 4) {
	break;
      }
    }
    if (point >= 3 && point < RotCurve[EditingCurve].numpoints - 1)
      RemovePoint(point);
    else if (fabs(Walk_cycle[0][EditingCurve][(int)(fx*CYCLE_SIZE)] - fy) < 4)
      AddPoint(fx);
    ComputeCurve(EditingCurve);
    MakeJointLists(EditingCurve);    
    RedisplayBoth();

  } else if (button == GLUT_LEFT_BUTTON && button == CurveDownBtn) {

    y = MAX(y, 0); y = MIN(y, CurveWHeight);
    x = MAX(x, 0); x = MIN(x, CurveWWidth);
    fy = -(((float)y - ((float)CurveWHeight/NUM_JOINTS * EditingCurve)) /
         ((float)CurveWHeight/NUM_JOINTS) - 0.5) * 180.0,
    fx = (float)x/CurveWWidth;
    CurveDownBtn = -1;
    if (CurvePickedPoint != -1) {
      fx = CurveEditConstrain(fx);
      RotCurve[EditingCurve].xcoords[CurvePickedPoint] = fx;
      RotCurve[EditingCurve].angles[CurvePickedPoint] = fy;        
      ComputeCurve(EditingCurve);
      MakeJointLists(EditingCurve);
      glutIdleFunc(NULL);
      RedisplayBoth();
    }
  }
}  


void CurveHandleMotion(int x, int y)
{
  if (CurvePickedPoint == -1) { 
    
    if (CurveDownBtn == GLUT_LEFT_BUTTON || CurveDownBtn ==
                                            GLUT_MIDDLE_BUTTON) {
      Step = (int)((float)x/glutGet(GLUT_WINDOW_WIDTH) * CYCLE_SIZE)
	% CYCLE_SIZE;
      if (Step < 0)
	Step = CYCLE_SIZE + Step;
      fStep = Step;

    RedisplayBoth();
    }
  } else {
    y = MAX(y, 0); y = MIN(y, CurveWHeight);
    x = MAX(x, 0); x = MIN(x, CurveWWidth);
    CurveLastX = x;
    CurveLastY = y;
  }
}

void CurveCPointDrag(void)
{
  float fx, fy;

  if (CurveDownBtn == GLUT_LEFT_BUTTON && CurvePickedPoint != -1) {
    fy = -(((float)CurveLastY -
           ((float)CurveWHeight/NUM_JOINTS * EditingCurve)) /
          ((float)CurveWHeight/NUM_JOINTS) - 0.5) * 180.0,
    fx = (float)CurveLastX/CurveWWidth;

    fx = CurveEditConstrain(fx);
    RotCurve[EditingCurve].xcoords[CurvePickedPoint] = fx;
    RotCurve[EditingCurve].angles[CurvePickedPoint] = fy;        
    ComputeCurve(EditingCurve);
    MakeJointLists(EditingCurve);
    RedisplayBoth();
  }
}

/* ARGSUSED1 */
void CurveHandleKeys(unsigned char key, int x, int y)
{
  if (key > '0' && key < '9')
    CurveHandleEditMenu((key-'0')-1);
  else if (key == 'd')
    CurveHandleMenu(CMENU_DONEEDIT);
  else {
    switch(key) {
      case 'f':
      case ' ': Step++;
                StopWalking(); break;
      case 'F': Step += 5;
                StopWalking(); break;
      case 'b': Step--;
                StopWalking(); break;
      case 'B': Step -= 5; 
                StopWalking(); break;
    }
    Step %= CYCLE_SIZE;
    if (Step < 0)
      Step = CYCLE_SIZE + Step;
    fStep = Step;
    RedisplayBoth();
  }
}

void CurveHandleEditMenu(int curve)
{
  if (curve >= NUM_JOINTS)
    return;
  if (EditingCurve == -1) {
    WasWalking = Walking;
    Walking = 0;
    agvSetAllowIdle(0);   /* don't allow spinning, just slows us down */
    glutIdleFunc(NULL);
    glutMouseFunc(CurveEditHandleButton);
  }
  EditingCurve = curve;
  glutPostRedisplay();
}

void CurveHandleSZMenu(int size)
{
  IncStep = (float)size/100;
}

void CurveHandleMenu(int value)
{
  switch (value) {
    case CMENU_QUIT:
      exit(0);
      break;
    case CMENU_CURVE:
      CurveAsPoints = !CurveAsPoints;
      glutPostRedisplay();
      break;
    case CMENU_HORIZ:
      DrawHorizontals = !DrawHorizontals;
      glutPostRedisplay();
      break;
    case CMENU_WALK:
      if (EditingCurve != -1)
	break;
      Walking = !Walking;
      if (Walking) {
        agvSetAllowIdle(0);
	glutIdleFunc(IncrementStep);	
      } else {
        agvSetAllowIdle(1);
      }
      break;
    case CMENU_DONEEDIT:
      glutMouseFunc(CurveHandleButton);
      EditingCurve = -1;
      CurvePickedPoint = -1;
      Walking = WasWalking;
      if (Walking)
	glutIdleFunc(IncrementStep);
      else
        agvSetAllowIdle(1);
      glutPostRedisplay();
      break;
    case CMENU_RESET:
      FlatCSet();
      ComputeCSetAndMakeLists();
      glutPostRedisplay();
      break;
    case CMENU_MIRROR:
      MirrorLegs = !MirrorLegs;
      ComputeCSetAndMakeLists();
      glutPostRedisplay();      
    }
}

void CurveMenuInit(void)
{
  int i;
  char label[3];

  if (CurveEditMenu != -1) {
    glutDestroyMenu(CurveEditMenu);
    glutDestroyMenu(CurveMenu);
    glutDestroyMenu(StepSizeMenu);
  }

  CurveEditMenu = glutCreateMenu(CurveHandleEditMenu);
  for (i = 0; i < NUM_JOINTS; i++) {
    sprintf(label, " %d ", i+1);
    glutAddMenuEntry(label, i);
  }
  StepSizeMenu = glutCreateMenu(CurveHandleSZMenu);
  glutAddMenuEntry("0.25", 25);
  glutAddMenuEntry("0.5",  50);
  glutAddMenuEntry("1.0", 100);
  glutAddMenuEntry("2.0", 200);
  glutAddMenuEntry("3.0", 300);
  glutAddMenuEntry("5.0", 500);
  CurveMenu = glutCreateMenu(CurveHandleMenu);
  glutAddSubMenu("Load Curve Set", LoadMenu);
  glutAddSubMenu("Save As Curve Set", SaveMenu);
  glutAddSubMenu("Edit Curve", CurveEditMenu);
  glutAddMenuEntry("Done Editing", CMENU_DONEEDIT);
  glutAddMenuEntry("Flatten Curve Set", CMENU_RESET);
  glutAddMenuEntry("Toggle mirrored", CMENU_MIRROR);
  glutAddSubMenu("Step size", StepSizeMenu);
  glutAddMenuEntry("Toggle dotted", CMENU_CURVE);
  glutAddMenuEntry("Toggle horizontals", CMENU_HORIZ);
  glutAddMenuEntry("Toggle walking", CMENU_WALK);
  glutAddMenuEntry("Quit", CMENU_QUIT);
  glutSetWindow(CurveWindow);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
}

void CurveVisible(int v)
{
  if (v == GLUT_VISIBLE)
    CurveWindowVisible = 1;
  else 
    CurveWindowVisible = 0;
}

/***************************************************************/
/*********************** GUY WINDOW ****************************/
/***************************************************************/

void GuyDisplay(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glFlush();

  glLoadIdentity();

  agvViewTransform();

  if (DrawAxes)
    glCallList(AxesList);

  switch(GuyModel) {
    case WIRECUBE:   DrawTheGuy_WC();  break;
    case SOLIDCUBE:  DrawTheGuy_SC();  break;
    case CYLINDER1:  DrawTheGuy_SL();  break;
    case CYLINDER2:  DrawTheGuy_SL2(); break;
  }

  glutSwapBuffers();
  glFlush();
}

void GuyReshape(int w, int h)
{
  glViewport(0,0,w,h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  if (ViewPerspective)
    gluPerspective(60.0, (GLdouble)w/h, 0.01, 100);
  else
    glOrtho(-1.2, 1.2, -1.2, 1.2, 0.1, 100);
  glMatrixMode(GL_MODELVIEW);
  glFlush();
}


void GuyGLInit(void)
{
  GLfloat mat_ambient[] = { 1.0, 1.0, 1.0, 1.0 };
  GLfloat light_position[] = { 0.3, 0.5, 0.8, 0.0 };
  GLfloat lm_ambient[] = { 0.2, 0.2, 0.2, 1.0 };
  
  glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
  glLightfv(GL_LIGHT0, GL_POSITION, light_position);
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lm_ambient);
  
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glDepthFunc(GL_LESS);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_NORMALIZE);
  glShadeModel(GL_SMOOTH);
  /* Cylinder stuff */
  StoreTheGuy_SL();
  StoreTheGuy_SL2();
}

void GuyHandleKeys(unsigned char key, int x, int y)
{
  switch(key) {
    case 'f':
    case ' ': Step++;
              StopWalking(); break;
    case 'F': Step += 5;
              StopWalking(); break;
    case 'b': Step--;
              StopWalking(); break;
    case 'B': Step -= 5; 
              StopWalking(); break;
  }
  Step %= CYCLE_SIZE;
  if (Step < 0)
    Step = CYCLE_SIZE + Step;
  agvHandleKeys(key, x, y);
  RedisplayBoth();
}

typedef enum { GMENU_QUIT, GMENU_CURVE, GMENU_HORIZ,
               GMENU_AXES, GMENU_PERSP } GuyMenuChoices;


void GuyModelHandleMenu(int model)
{
  GuyModel = model;
  if (model == WIRECUBE)
    glDisable(GL_LIGHTING);
  else
    glEnable(GL_LIGHTING);
  glutPostRedisplay();
}

void GuyHandleMenu(int value)
{
  switch (value) {
    case GMENU_QUIT:
      exit(0);
      break;
    case GMENU_AXES:
      DrawAxes = !DrawAxes;
      glutPostRedisplay();
      break;
    case GMENU_PERSP:
      ViewPerspective = !ViewPerspective;
      GuyReshape(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
      glutPostRedisplay();
      break;
  }
}

void GuyMenuInit(void)
{
  int sub2, sub1 = glutCreateMenu(agvSwitchMoveMode);
  glutAddMenuEntry("Flying move",  FLYING);
  glutAddMenuEntry("Polar move",   POLAR);
  sub2 = glutCreateMenu(GuyModelHandleMenu);
  glutAddMenuEntry("Wire cubes",  WIRECUBE);
  glutAddMenuEntry("Solid cubes", SOLIDCUBE);
  glutAddMenuEntry("Cylinder 1",  CYLINDER1);
  glutAddMenuEntry("Cylinder 2",  CYLINDER2);
  glutCreateMenu(GuyHandleMenu);
  glutAddSubMenu("Viewing", sub1);
  glutAddSubMenu("Model", sub2);
  glutAddMenuEntry("Toggle Axes",    GMENU_AXES);
  glutAddMenuEntry("Toggle Perspective View", GMENU_PERSP);
  glutAddMenuEntry("Quit",           GMENU_QUIT);
  glutSetWindow(GuyWindow);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
}

/***************************************************************/
/********************* BOTH WINDOWS ****************************/
/***************************************************************/

void RedisplayBoth(void)
{
  glutPostWindowRedisplay(GuyWindow);
  if (CurveWindowVisible) {
    glutPostWindowRedisplay(CurveWindow);
  }
}

void IncrementStep(void)
{
  fStep = fmod(fStep + IncStep, CYCLE_SIZE);
  Step = (int)fStep;
  if (agvMoving)
    agvMove();
  RedisplayBoth();
}

void StopWalking(void)
{
  if (Walking) {
    Walking = 0;
    agvSetAllowIdle(1);
  }
}

void SetWindowTitles(char *csetname)
{
  char windowtitle[MAX_CSETNAMELEN + 20];

  strcpy(windowtitle, "Rotation Curves: ");
  strcat(windowtitle, csetname);
  glutSetWindow(CurveWindow);
  glutSetWindowTitle(windowtitle);

  strcpy(windowtitle, "The Guy: ");
  strcat(windowtitle, csetname);
  glutSetWindow(GuyWindow);
  glutSetWindowTitle(windowtitle);
}

/***************************************************************/
/***************************** MAIN ****************************/
/***************************************************************/

int main(int argc, char** argv)
{
  glutInit(&argc, argv);

  glutInitWindowSize(512, 512);
  glutInitWindowPosition(700, 250);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  GuyWindow = glutCreateWindow("The Guy:");
  agvInit(!Walking);
  AxesList = glGenLists(1);
  agvMakeAxesList(AxesList);
  GuyGLInit();
  GuyMenuInit();
  glutDisplayFunc(GuyDisplay);
  glutReshapeFunc(GuyReshape);
  glutKeyboardFunc(GuyHandleKeys);

  glutInitWindowSize(512, 1024);
  glutInitWindowPosition(100, 0);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  CurveWindow = glutCreateWindow("Rotation Curves:");
  CurveGLInit();
  glutDisplayFunc(CurveDisplay);
  glutReshapeFunc(CurveReshape);
  glutMouseFunc(CurveHandleButton);
  glutMotionFunc(CurveHandleMotion);
  glutKeyboardFunc(CurveHandleKeys);
  glutVisibilityFunc(CurveVisible);

  FlatCSet();
  MakeLists();

  if (MakeLoadAndSaveMenus() > 0)  /* read first curve if there was one */
    HandleLoadMenu(0);

  CurveMenuInit();

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}




