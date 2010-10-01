/* 

 Newave - Ported from the original IrisGL SGI demo 
          (see https://www.sgi.com/toolbox/src/)

 I've ported an old IrisGL demo, newave, to OpenGL and GLUT.  
 This port has a couple of new features compared to the 
 "ancient" GL demo:

     * environment mapping (very cool!)
     * texture mapping
     * line antialiasing (needs some work)
     * better wave propagation
 
 I haven't implemented the mesh editing found in the old demo. 

 By default the program loads "texmap.rgb" and "spheremap.rgb"
 if no filenames are given as command line arguments.  
 Specify the texture map as the first argument and the sphere
 map as the second argument.

 Left mouse rotates the scene, middle mouse or +/- keys zoom, 
 right mouse for menu.

 Erik Larsen
 cayman@sprintmail.com

*/

#include <math.h>
#include <stdlib.h>
#include <GL/glut.h>
#include "texture.h"

#if defined(GL_EXT_texture_object) && !defined(GL_VERSION_1_1)
#define glBindTexture(A,B)     glBindTextureEXT(A,B)
#define glGenTextures(A,B)     glGenTexturesEXT(A,B)
#endif
#if defined(GL_EXT_polygon_offset) && !defined(GL_VERSION_1_1)
#define glPolygonOffset(A,B)     glPolygonOffsetEXT(A,B)
/* OpenGL 1.1's polygon offset can be different for each
   polygon mode primitive type.  The EXT extension has
   only one offset. */
#define GL_POLYGON_OFFSET_FILL   GL_POLYGON_OFFSET_EXT
#endif

typedef int bool;
#define true 1
#define false 0

/* Grid */
#define MAXGRID 63
enum {WIREFRAME, HIDDENLINE, FLATSHADED, SMOOTHSHADED, TEXTURED};
enum {FULLSCREEN, FACENORMALS, ANTIALIAS, ENVMAP};
enum {WEAK, NORMAL, STRONG};
enum {SMALL, MEDIUM, LARGE, XLARGE};
enum {CURRENT, FLAT, SPIKE, DIAGONALWALL, SIDEWALL, HOLE, 
      MIDDLEBLOCK, DIAGONALBLOCK, CORNERBLOCK, HILL, HILLFOUR};
int displayMode = WIREFRAME;
int resetMode = DIAGONALBLOCK;
int grid = 17;
float dt = 0.004;
float force[MAXGRID][MAXGRID],
      veloc[MAXGRID][MAXGRID],
      posit[MAXGRID][MAXGRID],
      vertNorms[MAXGRID][MAXGRID][3],
      faceNorms[2][MAXGRID][MAXGRID][3],
      faceNormSegs[2][2][MAXGRID][MAXGRID][3];
bool waving = false, editing = false, 
     drawFaceNorms = false, antialias = false,
     envMap = false;
#define SQRTOFTWOINV 1.0 / 1.414213562

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int texWidth, texHeight;
GLubyte *texData;
char *texFilename1 = "texmap.rgb", *texFilename2 = "spheremap.rgb";
GLuint texId1, texId2;
float texCoords[MAXGRID][MAXGRID][2];

/* Viewing */
float sphi=90.0, stheta=45.0;
float sdepth = 5.0/4.0 * MAXGRID;
float zNear=15.0, zFar=100.0;
float aspect = 5.0/4.0;
long xsize, ysize;
int downX, downY;
bool leftButton = false, middleButton = false;
int i,j;
GLfloat lightPosition[] = { 0.0, 0.0, 1.0, 1.0}; 
int displayMenu, otherMenu, speedMenu, sizeMenu, 
    resetMenu, mainMenu;

void getforce(void)
{
    float d;

    for(i=0;i<grid;i++) 
        for(j=0;j<grid;j++) 
        {
            force[i][j]=0.0;
        }

    for(i=2;i<grid-2;i++)
        for(j=2;j<grid-2;j++) 
        {
            d=posit[i][j]-posit[i][j-1];
            force[i][j] -= d;
            force[i][j-1] += d;

            d=posit[i][j]-posit[i-1][j];
            force[i][j] -= d;
            force[i-1][j] += d;

            d= (posit[i][j]-posit[i][j+1]); 
            force[i][j] -= d ;
            force[i][j+1] += d;

            d= (posit[i][j]-posit[i+1][j]); 
            force[i][j] -= d ;
            force[i+1][j] += d;

            d= (posit[i][j]-posit[i+1][j+1])*SQRTOFTWOINV; 
            force[i][j] -= d ;
            force[i+1][j+1] += d;

            d= (posit[i][j]-posit[i-1][j-1])*SQRTOFTWOINV; 
            force[i][j] -= d ;
            force[i-1][j-1] += d;

            d= (posit[i][j]-posit[i+1][j-1])*SQRTOFTWOINV; 
            force[i][j] -= d ;
            force[i+1][j-1] += d;

            d= (posit[i][j]-posit[i-1][j+1])*SQRTOFTWOINV; 
            force[i][j] -= d ;
            force[i- 1][j+1] += d;
        }
}

void getvelocity(void)
{
    for(i=0;i<grid;i++)
        for(j=0;j<grid;j++)
            veloc[i][j]+=force[i][j] * dt;
}

void getposition(void)
{
    for(i=0;i<grid;i++)
        for(j=0;j<grid;j++)
            posit[i][j]+=veloc[i][j];
}


void copy(float vec0[3], float vec1[3])
{
    vec0[0] = vec1[0];
    vec0[1] = vec1[1];
    vec0[2] = vec1[2];
}

void sub(float vec0[3], float vec1[3], float vec2[3])
{
    vec0[0] = vec1[0] - vec2[0];
    vec0[1] = vec1[1] - vec2[1];
    vec0[2] = vec1[2] - vec2[2];
}

void add(float vec0[3], float vec1[3], float vec2[3])
{
    vec0[0] = vec1[0] + vec2[0];
    vec0[1] = vec1[1] + vec2[1];
    vec0[2] = vec1[2] + vec2[2];
}

void scalDiv(float vec[3], float c)
{
    vec[0] /= c; vec[1] /= c; vec[2] /= c;
}

void cross(float vec0[3], float vec1[3], float vec2[3])
{
    vec0[0] = vec1[1] * vec2[2] - vec1[2] * vec2[1];
    vec0[1] = vec1[2] * vec2[0] - vec1[0] * vec2[2];
    vec0[2] = vec1[0] * vec2[1] - vec1[1] * vec2[0];
}

void norm(float vec[3])
{
    float c = sqrt(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
    scalDiv(vec, c); 
}

void set(float vec[3], float x, float y, float z)
{
    vec[0] = x;
    vec[1] = y;
    vec[2] = z;
}


/* face normals - for flat shading */
void getFaceNorms(void)
{
    float vec0[3], vec1[3], vec2[3], norm0[3], norm1[3];
    float geom0[3], geom1[3], geom2[3], geom3[3];
    for (i = 0; i < grid-1; ++i)
    {
        for (j = 0; j < grid-1; ++j)
        {
            /* get vectors from geometry points */
            geom0[0] = i; geom0[1] = j; geom0[2] = posit[i][j];
            geom1[0] = i; geom1[1] = j+1; geom1[2] = posit[i][j+1];
            geom2[0] = i+1; geom2[1] = j; geom2[2] = posit[i+1][j];
            geom3[0] = i+1; geom3[1] = j+1; geom3[2] = posit[i+1][j+1];

            sub( vec0, geom1, geom0 );
            sub( vec1, geom1, geom2 );
            sub( vec2, geom1, geom3 );

            /* get triangle face normals from vectors & normalize them */
            cross( norm0, vec0, vec1 );
            norm( norm0 );

            cross( norm1, vec1, vec2 ); 
            norm( norm1 );

            copy( faceNorms[0][i][j], norm0 );
            copy( faceNorms[1][i][j], norm1 );
        }
    }
}

/* vertex normals - average of face normals for smooth shading */
void getVertNorms(void)
{
    float avg[3];
    for (i = 0; i < grid; ++i)
    {
        for (j = 0; j < grid; ++j)
        {
            /* For each vertex, average normals from all faces sharing */
            /* vertex.  Check each quadrant in turn */
            set(avg, 0.0, 0.0, 0.0);

            /* Right & above */
            if (j < grid-1 && i < grid-1)
            {
                add( avg, avg, faceNorms[0][i][j] );
            }
            /* Right & below */
            if (j < grid-1 && i > 0)
            {
                add( avg, avg, faceNorms[0][i-1][j] );
                add( avg, avg, faceNorms[1][i-1][j] );
            }
            /* Left & above */
            if (j > 0 && i < grid-1)
            {
                add( avg, avg, faceNorms[0][i][j-1] );
                add( avg, avg, faceNorms[1][i][j-1] );
            }
            /* Left & below */
            if (j > 0 && i > 0)
            {
                add( avg, avg, faceNorms[1][i-1][j-1] );
            }

            /* Normalize */
            norm( avg );
            copy( vertNorms[i][j], avg );
        }
    }
}


void getFaceNormSegs(void)
{
    float center0[3], center1[3], normSeg0[3], normSeg1[3];
    float geom0[3], geom1[3], geom2[3], geom3[3];
    for (i = 0; i < grid - 1; ++i)
    {
        for (j = 0; j < grid - 1; ++j)
        {
            geom0[0] = i; geom0[1] = j; geom0[2] = posit[i][j];
            geom1[0] = i; geom1[1] = j+1; geom1[2] = posit[i][j+1];
            geom2[0] = i+1; geom2[1] = j; geom2[2] = posit[i+1][j];
            geom3[0] = i+1; geom3[1] = j+1; geom3[2] = posit[i+1][j+1];

            /* find center of triangle face by averaging three vertices */
            add( center0, geom2, geom0 );
            add( center0, center0, geom1 );
            scalDiv( center0, 3.0 );

            add( center1, geom2, geom1 );
            add( center1, center1, geom3 );
            scalDiv( center1, 3.0 );

            /* translate normal to center of triangle face to get normal segment */
            add( normSeg0, center0, faceNorms[0][i][j] );
            add( normSeg1, center1, faceNorms[1][i][j] );

            copy( faceNormSegs[0][0][i][j], center0 );
            copy( faceNormSegs[1][0][i][j], center1 );

            copy( faceNormSegs[0][1][i][j], normSeg0 );
            copy( faceNormSegs[1][1][i][j], normSeg1 );
        }
    }
}

void getTexCoords(void)
{
    for (i = 0; i < grid; ++i)
    {
        for (j = 0; j < grid; ++j)
        {
            texCoords[i][j][0] = (float)j/(float)(grid-1);
            texCoords[i][j][1] = (float)i/(float)(grid-1);
        }
    }
}


void wave(void)
{
    if (waving)
    {
        getforce();
        getvelocity();
        getposition();
        glutPostRedisplay();
    }
}

void go(void)
{
    waving = true;
    editing = false;
    glutIdleFunc(wave);
}

void stop(void)
{
    waving = false;
    glutIdleFunc(NULL);
}

void edit(void)
{
    stop();
    editing = true;
}

void reverse(void)
{
    for(i=1;i<(grid-1);i++)
        for(j=1;j<(grid-1);j++)
            veloc[i][j]= -veloc[i][j];

    if (!waving)
        go();
}

void reset(int value)
{
    if (waving)
        stop();

    if (value != CURRENT)
        resetMode = value;
    for(i=0;i<grid;i++)
        for(j=0;j<grid;j++)
        {
            force[i][j]=0.0;
            veloc[i][j]=0.0;

            switch(resetMode)
            {
            case FLAT:
                posit[i][j] = 0.0;
                break;
            case SPIKE:
                 posit[i][j]= (i==j && i == grid/2) ? grid*1.5 : 0.0;
                break;
            case HOLE:
                posit[i][j]= (!((i > grid/3 && j > grid/3)&&(i < grid*2/3 && j < grid*2/3))) ? grid/4 : 0.0;
                break;
            case DIAGONALWALL:
                posit[i][j]= (((grid-i)-j<3) && ((grid-i)-j>0)) ? grid/6 : 0.0;
                break;
            case SIDEWALL:
                posit[i][j]= (i==1) ? grid/4 : 0.0;
                break;
            case DIAGONALBLOCK:
                posit[i][j]= ((grid-i)-j<3) ? grid/6 : 0.0;
                break;
            case MIDDLEBLOCK:
                posit[i][j]= ((i > grid/3 && j > grid/3)&&(i < grid*2/3 && j < grid*2/3)) ? grid/4 : 0.0;
                break;
            case CORNERBLOCK:
                posit[i][j]= ((i > grid*3/4 && j > grid*3/4)) ? grid/4 : 0.0;
                break;
            case HILL:
                posit[i][j]= 
                    (sin(M_PI * ((float)i/(float)grid)) +
                     sin(M_PI * ((float)j/(float)grid)))* grid/6.0;
            break;        
            case HILLFOUR:
                posit[i][j]= 
                    (sin(M_PI*2 * ((float)i/(float)grid)) +
                     sin(M_PI*2 * ((float)j/(float)grid)))* grid/6.0;
            break;        
            }
            if (i==0||j==0||i==grid-1||j==grid-1) posit[i][j]=0.0;
        }
    glutPostRedisplay();
}

void setSize(int value)
{
    int prevGrid = grid;
    switch(value) 
    {
        case SMALL : grid = MAXGRID/4; break;
        case MEDIUM: grid = MAXGRID/2; break;
        case LARGE : grid = MAXGRID/1.5; break;
        case XLARGE : grid = MAXGRID; break;
    }
    if (prevGrid > grid)
    {
        reset(resetMode);
    }
    zNear= grid/10.0;
    zFar= grid*3.0;
    sdepth = 5.0/4.0 * grid;
    getTexCoords();
    glutPostRedisplay();
}

void setSpeed(int value)
{
    switch(value) 
    {
        case WEAK  : dt = 0.001; break;
        case NORMAL: dt = 0.004; break;
        case STRONG: dt = 0.008; break;
    }
}

void setDisplay(int value)
{
    displayMode = value;
    switch(value) 
    {
        case WIREFRAME   : 
            glShadeModel(GL_FLAT); 
            glDisable(GL_LIGHTING);
            break;
        case HIDDENLINE: 
            glShadeModel(GL_FLAT); 
            glDisable(GL_LIGHTING);
            break;
        case FLATSHADED  : 
            glShadeModel(GL_FLAT); 
            glEnable(GL_LIGHTING);
            break;
        case SMOOTHSHADED: 
            glShadeModel(GL_SMOOTH); 
            glEnable(GL_LIGHTING);
            break;
        case TEXTURED: 
            glShadeModel(GL_SMOOTH); 
            glEnable(GL_LIGHTING);
            break;
    }
    glutPostRedisplay();
}

void setOther(int value)
{
    switch (value)
    {
        case FULLSCREEN: 
            glutFullScreen();
            break;
        case FACENORMALS: 
            drawFaceNorms = !drawFaceNorms;
            break;
        case ANTIALIAS: 
            antialias = !antialias;
            if (antialias)
            {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glEnable(GL_LINE_SMOOTH);
                glLineWidth(1.5);
            }
            else
            {
                glDisable(GL_BLEND);
                glDisable(GL_LINE_SMOOTH);
                glLineWidth(1.0);
            }
            break;
        case ENVMAP: 
            envMap = !envMap;
            if (envMap)
            {
                glBindTexture(GL_TEXTURE_2D, texId2);
                glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
                glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
                glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
                glEnable(GL_TEXTURE_GEN_S);
                glEnable(GL_TEXTURE_GEN_T);
            }
            else
            {
                glBindTexture(GL_TEXTURE_2D, texId1);
                glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                glDisable(GL_TEXTURE_GEN_S);
                glDisable(GL_TEXTURE_GEN_T);
            }
            break;
    }
    glutPostRedisplay();
}

void setMain(int value)
{
    switch(value) 
    {
        case 1: edit();    break;
        case 2:    go();      break; /* set idle func to something */
        case 3: stop();    break; /* set idle func to null */
        case 4:    reverse(); break;
        case 5:    exit(0);   break;
    }
}


void drawFaceNormals(void)
{
    glColor3f(1.0,1.0,1.0);
    for (i = 0; i < grid - 1; ++i)
    {
        for (j = 0; j < grid - 1; ++j)
        {
            glBegin(GL_LINES);
            glVertex3fv(faceNormSegs[0][0][i][j]);
            glVertex3fv(faceNormSegs[0][1][i][j]);
            glEnd();

            glBegin(GL_LINES);
            glVertex3fv(faceNormSegs[1][0][i][j]);
            glVertex3fv(faceNormSegs[1][1][i][j]);
            glEnd();
        }
    }
}

void drawSmoothShaded(void)
{
    glColor3f(0.8f, 0.2f, 0.8f);
    for (i = 0; i < grid - 1; ++i)
    {
        glBegin(GL_TRIANGLE_STRIP);
        for (j = 0; j < grid; ++j)
        {
            glNormal3fv( vertNorms[i][j] );
            glVertex3f( i, j, posit[i][j] );
            glNormal3fv( vertNorms[i+1][j] );
            glVertex3f( i+1, j, posit[i+1][j] );
        }
        glEnd();
    }
}

void drawWireframe(void)
{
    glColor3f(1.0, 1.0, 1.0);

    for(i=0;i<grid;i++)
    {
        glBegin(GL_LINE_STRIP);
        for(j=0;j<grid;j++)
            glVertex3f( (float) i, (float) j, (float) posit[i][j]);
        glEnd();
    }
    
    for(i=0;i<grid;i++)
    {
        glBegin(GL_LINE_STRIP);
        for(j=0;j<grid;j++)
            glVertex3f( (float) j, (float) i, (float) posit[j][i]);
        glEnd();
    }
}

void drawFlatShaded(void)
{
    glEnable(GL_POLYGON_OFFSET_FILL);
    glColor3f(0.8f, 0.2f, 0.8f);
    for (i = 0; i < grid - 1; ++i)
    {
        glBegin(GL_TRIANGLE_STRIP);
        glVertex3f( (float) i, (float) 0, (float) posit[i][0]);
        glVertex3f( (float) i+1, (float) 0, (float) posit[i+1][0]);
        for (j = 1; j < grid; ++j)
        {
            glNormal3fv( faceNorms[0][i][j-1] );
            glVertex3f( (float) i, (float) j, (float) posit[i][j]);
              glNormal3fv( faceNorms[1][i][j-1] );
            glVertex3f( (float) i+1, (float) j, (float) posit[i+1][j]);
        }
        glEnd();
    }
    glDisable(GL_POLYGON_OFFSET_FILL);
}

void drawHiddenLine(void)
{
    glEnable(GL_POLYGON_OFFSET_FILL);
    glColor3f(0.8f, 0.2f, 0.8f);
    for (i = 0; i < grid - 1; ++i)
    {
        glBegin(GL_TRIANGLE_STRIP);
        glVertex3f( (float) i, (float) 0, (float) posit[i][0]);
        glVertex3f( (float) i+1, (float) 0, (float) posit[i+1][0]);
        for (j = 1; j < grid; ++j)
        {
            glVertex3f( (float) i, (float) j, (float) posit[i][j]);
            glVertex3f( (float) i+1, (float) j, (float) posit[i+1][j]);
        }
        glEnd();
    }    
    glDisable(GL_POLYGON_OFFSET_FILL);
    
    glColor3f(1.0,1.0,1.0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    for (i = 0; i < grid - 1; ++i)
    {
        glBegin(GL_TRIANGLE_STRIP);
        glVertex3f( (float) i, (float) 0, (float) posit[i][0]);
        glVertex3f( (float) i+1, (float) 0, (float) posit[i+1][0]);
        for (j = 1; j < grid; ++j)
        {
            glVertex3f( (float) i, (float) j, (float) posit[i][j]);
            glVertex3f( (float) i+1, (float) j, (float) posit[i+1][j]);
        }
        glEnd();
    }    
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void loadImageTexture(void)
{

    glGenTextures(1,&texId1);
    glBindTexture(GL_TEXTURE_2D, texId1);
    imgLoad(texFilename1, 0, 0, &texWidth, &texHeight, &texData);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, 4, 
        texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
        texData );

    glGenTextures(1,&texId2);
    glBindTexture(GL_TEXTURE_2D, texId2);
    imgLoad(texFilename2, 0, 0, &texWidth, &texHeight, &texData);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, 4, 
        texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
        texData );
}

void drawTextured(void)
{
    glColor3f(1.0f, 1.0f, 1.0f);
    glEnable(GL_TEXTURE_2D);
    for (i = 0; i < grid - 1; ++i)
    {
        glBegin(GL_TRIANGLE_STRIP);
        for (j = 0; j < grid; ++j)
        {
            glNormal3fv( vertNorms[i][j] );
            glTexCoord2fv( texCoords[i][j] );
            glVertex3f( i, j, posit[i][j] );
            glNormal3fv( vertNorms[i+1][j] );
            glTexCoord2fv( texCoords[i+1][j] );
            glVertex3f( i+1, j, posit[i+1][j] );
        }
        glEnd();
    }
    glDisable(GL_TEXTURE_2D);
}


void reshape(int width, int height)
{
    xsize = width; 
    ysize = height;
    aspect = (float)xsize/(float)ysize;
    glViewport(0, 0, xsize, ysize);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glutPostRedisplay();
}

void display(void) 
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(64.0, aspect, zNear, zFar);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity(); 

    glTranslatef(0.0,0.0,-sdepth);
    glRotatef(-stheta, 1.0, 0.0, 0.0);
    glRotatef(sphi, 0.0, 0.0, 1.0);
    glTranslatef(-(float)((grid+1)/2-1), -(float)((grid+1)/2-1), 0.0);

      getFaceNorms();
    getVertNorms();

    switch (displayMode) 
    {
        case WIREFRAME: drawWireframe(); break;
        case HIDDENLINE: drawHiddenLine(); break;
        case FLATSHADED: drawFlatShaded(); break;
        case SMOOTHSHADED: drawSmoothShaded(); break;
        case TEXTURED: drawTextured(); break;
    }

    if (drawFaceNorms)    
    {
        getFaceNormSegs();
        drawFaceNormals();
    }

    glutSwapBuffers();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void visibility(int state)
{
    if ((state == GLUT_VISIBLE) && waving)
        go();
    else 
        stop();
}

void motion(int x, int y)
{
    if (leftButton)
    {
        sphi += (float)(x - downX) / 4.0;
        stheta += (float)(downY - y) / 4.0;
    }
    if (middleButton)
    {
        sdepth += (float)(downY - y) / 10.0;
    }
    downX = x;
    downY = y;
    glutPostRedisplay();
}


void mouse(int button, int state, int x, int y)
{
    downX = x;
    downY = y;
    leftButton = ((button == GLUT_LEFT_BUTTON) && 
                  (state == GLUT_DOWN));
    middleButton = ((button == GLUT_MIDDLE_BUTTON) && 
                    (state == GLUT_DOWN));
}

void keyboard(unsigned char ch, int x, int y)
{
    switch (ch) 
    {
        case '+': sdepth += 2.0; break;
        case '-': sdepth -= 2.0; break;
        case 27: exit(0); break;
    }
    glutPostRedisplay();
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(500, 500);
    glutCreateWindow("Newave");
    if (argc > 1 && argv[1] != 0)
        texFilename1 = argv[1];
    if (argc > 2 && argv[2] != 0)
        texFilename2 = argv[2];
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glPolygonOffset(1.0, 1.0);
    glEnable(GL_CULL_FACE);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_DIFFUSE);
    glLightfv (GL_LIGHT0, GL_POSITION, lightPosition);
    glEnable(GL_LIGHT0);
    loadImageTexture();

    setSize(MEDIUM);
    setSpeed(NORMAL);
    setDisplay(TEXTURED);
    setOther(ENVMAP);
    reset(HILLFOUR);

    glutReshapeFunc(reshape);
    glutDisplayFunc(display);
    glutVisibilityFunc(visibility);

    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);

    displayMenu = glutCreateMenu(setDisplay);
    glutAddMenuEntry("Wireframe", WIREFRAME);
    glutAddMenuEntry("Hidden Line", HIDDENLINE);
    glutAddMenuEntry("Flat Shaded", FLATSHADED);
    glutAddMenuEntry("Smooth Shaded", SMOOTHSHADED);
    glutAddMenuEntry("Textured", TEXTURED);

    otherMenu = glutCreateMenu(setOther);
    glutAddMenuEntry("Full Screen", FULLSCREEN);
    glutAddMenuEntry("Face Normals", FACENORMALS);
    glutAddMenuEntry("Antialias", ANTIALIAS);
    glutAddMenuEntry("Environment Map", ENVMAP);

    speedMenu = glutCreateMenu(setSpeed);
    glutAddMenuEntry("Weak", WEAK);
    glutAddMenuEntry("Normal", NORMAL);
    glutAddMenuEntry("Strong", STRONG);

    sizeMenu = glutCreateMenu(setSize);
    glutAddMenuEntry("Small", SMALL);
    glutAddMenuEntry("Medium", MEDIUM);
    glutAddMenuEntry("Large", LARGE);
    glutAddMenuEntry("Extra Large", XLARGE);

    resetMenu = glutCreateMenu(reset);
    glutAddMenuEntry("Current", CURRENT);
    glutAddMenuEntry("Spike", SPIKE);
    glutAddMenuEntry("Hole", HOLE);
    glutAddMenuEntry("Diagonal Wall", DIAGONALWALL);
    glutAddMenuEntry("Side Wall", SIDEWALL);
    glutAddMenuEntry("Middle Block", MIDDLEBLOCK);
    glutAddMenuEntry("Diagonal Block", DIAGONALBLOCK);
    glutAddMenuEntry("Corner Block", CORNERBLOCK);
    glutAddMenuEntry("Hill", HILL);
    glutAddMenuEntry("Hill Four", HILLFOUR);

    mainMenu = glutCreateMenu(setMain);
    glutAddMenuEntry("Go", 2);
    glutAddMenuEntry("Stop", 3);
    glutAddMenuEntry("Reverse", 4);
    glutAddSubMenu("Display", displayMenu);
    glutAddSubMenu("Reset", resetMenu);
    glutAddSubMenu("Size", sizeMenu);
    glutAddSubMenu("Speed", speedMenu);
    glutAddSubMenu("Other", otherMenu);
    glutAddMenuEntry("Exit", 5);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    glutMainLoop();
    return 0;             /* ANSI C requires main to return int. */
}

    
