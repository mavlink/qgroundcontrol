/* Text3d by Robert J. Doyle, Jr., Naval Research Laboratory, Washington, DC. */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <GL/glut.h>

typedef enum {RESERVED, M_SIDE, M_EDGE, M_WHOLE, O_SIDE, O_EDGE, O_WHOLE, 
T_SIDE, T_EDGE, T_WHOLE, H_SIDE, H_EDGE, H_WHOLE,REPEAT_SIDE, REPEAT_EDGE, REPEAT1, 
REPEAT2_SIDE, REPEAT2_EDGE,REPEAT2, REPEAT3_SIDE, REPEAT3_EDGE,REPEAT3, 
REPEAT4_SIDE, REPEAT4_EDGE,REPEAT4} displayLists;

GLfloat sideColor[] = {0.0, 0.0, 0.5, 1.0};
GLfloat edgeColor[] = {0.7, 0.7, 0.0, 1.0};
GLfloat shininess[] = {128.0};
GLfloat mat_specular[] = {0.7, 0.7, 0.7, 1.0};

GLfloat width = 0.0;
GLfloat width2 = 2.0;

GLfloat letterM[][3] = 
{ 
    {-3.125000, 0.000000, 0.000000},
    {-3.125000, 6.208000, 0.000000},
    {-1.233000, 6.208000, 0.000000},
    {0.003000, 1.484000, 0.000000},
    {1.223000, 6.208000, 0.000000},
    {3.123000, 6.208000, 0.000000},
    {3.123000, 0.000000, 0.000000},
    {1.923000, 0.000000, 0.000000},
    {1.923000, 5.010000, 0.000000},
    {0.659000, 0.000000, 0.000000},
    {-0.649000, 0.000000, 0.000000},
    {-1.925000, 5.010000, 0.000000},
    {-1.925000, 0.000000, 0.000000}

};

GLfloat letterO[][3] = 
{ 
    {-3.038000, 3.102000, 0.000000},
    {-2.974000, 3.874000, 0.000000},
    {-2.827000, 4.440000, 0.000000},
    {-2.802000, 4.508000, 0.000000},
    {-2.544000, 5.042000, 0.000000},
    {-2.502000, 5.110000, 0.000000},
    {-2.223000, 5.479000, 0.000000},
    {-2.132000, 5.576000, 0.000000},
    {-1.784000, 5.869000, 0.000000},
    {-1.678000, 5.940000, 0.000000},
    {-1.260000, 6.155000, 0.000000},
    {-1.148000, 6.198000, 0.000000},
    {-0.677000, 6.321000, 0.000000},
    {-0.638000, 6.328000, 0.000000},
    {-0.002000, 6.378000, 0.000000},
    {0.634000, 6.328000, 0.000000},
    {1.107000, 6.210000, 0.000000},
    {1.144000, 6.198000, 0.000000},
    {1.570000, 6.002000, 0.000000},
    {1.674000, 5.940000, 0.000000},
    {2.038000, 5.661000, 0.000000},
    {2.128000, 5.576000, 0.000000},
    {2.428000, 5.217000, 0.000000},
    {2.504000, 5.104000, 0.000000},
    {2.762000, 4.598000, 0.000000},
    {2.798000, 4.508000, 0.000000},
    {2.960000, 3.913000, 0.000000},
    {2.970000, 3.862000, 0.000000},
    {3.034000, 3.102000, 0.000000},
    {2.970000, 2.342000, 0.000000},
    {2.815000, 1.745000, 0.000000},
    {2.798000, 1.696000, 0.000000},
    {2.554000, 1.182000, 0.000000},
    {2.504000, 1.100000, 0.000000},
    {2.221000, 0.726000, 0.000000},
    {2.128000, 0.628000, 0.000000},
    {1.776000, 0.332000, 0.000000},
    {1.674000, 0.264000, 0.000000},
    {1.256000, 0.049000, 0.000000},
    {1.144000, 0.006000, 0.000000},
    {0.672000, -0.117000, 0.000000},
    {0.634000, -0.124000, 0.000000},
    {-0.002000, -0.174000, 0.000000},
    {-0.638000, -0.124000, 0.000000},
    {-1.112000, -0.006000, 0.000000},
    {-1.148000, 0.006000, 0.000000},
    {-1.576000, 0.202000, 0.000000},
    {-1.678000, 0.264000, 0.000000},
    {-2.041000, 0.540000, 0.000000},
    {-2.132000, 0.628000, 0.000000},
    {-2.430000, 0.983000, 0.000000},
    {-2.502000, 1.094000, 0.000000},
    {-2.773000, 1.622000, 0.000000},
    {-2.802000, 1.696000, 0.000000},
    {-2.962000, 2.258000, 0.000000},
    {-2.974000, 2.330000, 0.000000},
    {-1.736000, 3.102000, 10000.0},
    {-1.710000, 3.578000, 0.000000},
    {-1.644000, 3.934000, 0.000000},
    {-1.503000, 4.328000, 0.000000},
    {-1.494000, 4.346000, 0.000000},
    {-1.352000, 4.593000, 0.000000},
    {-1.306000, 4.656000, 0.000000},
    {-1.120000, 4.857000, 0.000000},
    {-1.040000, 4.926000, 0.000000},
    {-0.825000, 5.067000, 0.000000},
    {-0.726000, 5.116000, 0.000000},
    {-0.480000, 5.200000, 0.000000},
    {-0.402000, 5.218000, 0.000000},
    {-0.041000, 5.257000, 0.000000},
    {-0.002000, 5.258000, 0.000000},
    {0.361000, 5.227000, 0.000000},
    {0.400000, 5.220000, 0.000000},
    {0.650000, 5.147000, 0.000000},
    {0.726000, 5.116000, 0.000000},
    {0.950000, 4.990000, 0.000000},
    {1.038000, 4.926000, 0.000000},
    {1.239000, 4.736000, 0.000000},
    {1.306000, 4.656000, 0.000000},
    {1.462000, 4.413000, 0.000000},
    {1.498000, 4.342000, 0.000000},
    {1.635000, 3.964000, 0.000000},
    {1.644000, 3.934000, 0.000000},
    {1.710000, 3.568000, 0.000000},
    {1.736000, 3.102000, 0.000000},
    {1.710000, 2.636000, 0.000000},
    {1.642000, 2.268000, 0.000000},
    {1.508000, 1.886000, 0.000000},
    {1.496000, 1.860000, 0.000000},
    {1.351000, 1.610000, 0.000000},
    {1.304000, 1.546000, 0.000000},
    {1.115000, 1.343000, 0.000000},
    {1.036000, 1.276000, 0.000000},
    {0.823000, 1.135000, 0.000000},
    {0.724000, 1.086000, 0.000000},
    {0.480000, 1.001000, 0.000000},
    {0.400000, 0.984000, 0.000000},
    {0.035000, 0.946000, 0.000000},
    {-0.002000, 0.946000, 0.000000},
    {-0.368000, 0.979000, 0.000000},
    {-0.402000, 0.986000, 0.000000},
    {-0.653000, 1.057000, 0.000000},
    {-0.726000, 1.088000, 0.000000},
    {-0.952000, 1.213000, 0.000000},
    {-1.040000, 1.278000, 0.000000},
    {-1.240000, 1.467000, 0.000000},
    {-1.306000, 1.548000, 0.000000},
    {-1.460000, 1.788000, 0.000000},
    {-1.494000, 1.858000, 0.000000},
    {-1.639000, 2.251000, 0.000000},
    {-1.644000, 2.270000, 0.000000},
    {-1.710000, 2.626000, 0.000000}
};

GLfloat letterT[][3] = 
{
    {-0.640000, 0.000000, 0.000000},
    {-0.640000, 5.104000, 0.000000},
    {-2.476000, 5.104000, 0.000000},
    {-2.476000, 6.208000, 0.000000},
    {2.476000, 6.208000, 0.000000},
    {2.476000, 5.104000, 0.000000},
    {0.640000, 5.104000, 0.000000},
    {0.640000, 0.000000, 0.000000}
};

GLfloat letterH[][3] = 
{
    {-2.570000, 0.000000, 0.000000},
    {-2.570000, 6.208000, 0.000000},
    {-1.282000, 6.208000, 0.000000},
    {-1.282000, 3.900000, 0.000000},
    {1.280000, 3.900000, 0.000000},
    {1.280000, 6.208000, 0.000000},
    {2.568000, 6.208000, 0.000000},
    {2.568000, 0.000000, 0.000000},
    {1.280000, 0.000000, 0.000000},
    {1.280000, 2.760000, 0.000000},
    {-1.282000, 2.760000, 0.000000},
    {-1.282000, 0.000000, 0.000000}
};

/*  Initialize light source and lighting.
 */


static void checkErrors(void)
{
  GLenum error;
  while ((error = glGetError()) != GL_NO_ERROR) {
    fprintf(stderr, "Error: %s\n", (char *) gluErrorString(error));
  }
}

void myinit(void)
{
	int count1 =  sizeof(letterM) / (3 * sizeof(GLfloat)); 
	int count2 =  sizeof(letterO) / (3 * sizeof(GLfloat));
	int count3 =  sizeof(letterT) / (3 * sizeof(GLfloat)); 
	int count4 =  sizeof(letterH) / (3 * sizeof(GLfloat));

	int i;
	
    GLfloat light_ambient[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };
/*	light_position is NOT default value	*/
    GLfloat light_position[] = { -1.0, -1.0, 1.0, 0.0 };

    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);

	glDrawBuffer(GL_FRONT_AND_BACK);
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_ACCUM_BUFFER_BIT);
	glDrawBuffer(GL_BACK);	

/* Zero position of text */

    		for(i = 0; i < count1; i++) {
	    		letterM[i][1] = letterM[i][1] - 3.175;
	    	}
    		for(i = 0; i < count2; i++) {
	    		letterO[i][1] = letterO[i][1] - 3.175;
			}
    		for(i = 0; i < count3; i++) {
	    		letterT[i][1] = letterT[i][1] - 3.175;
			} 
    		for(i = 0; i < count4; i++) {
	    		letterH[i][1] = letterH[i][1] - 3.175;
			} 
}


/* Mark Kilgard's tessellation code from the "dino" demos. */
void extrudeSolidFromPolygon(GLfloat data[][3], unsigned int dataSize,
		GLdouble thickness, GLuint side, GLuint edge, GLuint whole)
{
	GLdouble vertex[3], dx, dy, len;
	int i, k;
	int flag = 0;
	int count = dataSize / (3 * sizeof(GLfloat));
    static GLUtriangulatorObj *tobj = NULL;

    if (tobj == NULL) {
    	tobj = gluNewTess();
    	
    	gluTessCallback(tobj, GLU_BEGIN, glBegin);
    	gluTessCallback(tobj, GLU_VERTEX, glVertex3fv);
    	gluTessCallback(tobj, GLU_END, glEnd);
    }
    glNewList(side, GL_COMPILE);
    	glShadeModel(GL_SMOOTH);
    	gluBeginPolygon(tobj);
    		for(i = 0; i < count; i++) {
    			/* This detects a new contour from a large number placed in
    			the unused z coordinate of the vertex where the new contour 
    			starts. See the coordinates for letterO, above. The coordinate 
    			must be reset below for additional calls. */

    			if (data[i][2] > 1000.0) {
    				data[i][2] = 0.0;
    				flag = 1; k = i;
    				gluNextContour(tobj, GLU_INTERIOR); 
    			}
    			
    			vertex[0] = data[i][0];
    			vertex[1] = data[i][1];
    			vertex[2] = 0.0;
    			gluTessVertex(tobj, vertex, data[i]);
    		}
    	gluEndPolygon(tobj);
    glEndList();
	
				/* Reset coordinate for new calls. */
				if (flag == 1) {
				data[k][2] = 10000.0;
				flag = 0;
				}
	glNewList(edge, GL_COMPILE);
		glBegin(GL_QUAD_STRIP);
		for(i = 0; i <= count; i++) {
			glVertex3f(data[i % count][0], data[i % count][1], 0.0);
			glVertex3f(data[i % count][0], data[i % count][1], thickness);
			/* Normals */
			dx = data[(i+ 1) % count][1] - data[i % count][1];
			dy = data[i % count][0] - data[(i + 1) % count][0];
			len = sqrt(dx * dx + dy * dy);
			glNormal3f(dx / len, dy / len, 0.0);
		}
		glEnd();
	glEndList();
	
	glNewList(whole, GL_COMPILE);
		glFrontFace(GL_CW);

		glMaterialfv(GL_FRONT, GL_DIFFUSE, edgeColor);
		glMaterialfv(GL_FRONT, GL_SHININESS, shininess);
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);				

		glCallList(edge);
		glNormal3f(0.0, 0.0, -1.0); 
		glCallList(side);
		glPushMatrix();
			glTranslatef(0.0, 0.0, thickness);
			glFrontFace(GL_CCW);
			glNormal3f(0.0, 0.0, 1.0);

		glMaterialfv(GL_FRONT, GL_DIFFUSE, sideColor);
		glMaterialfv(GL_FRONT, GL_SHININESS, shininess);
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);		
		
			glCallList(side);
		glPopMatrix();
	glEndList();
}

void repeat(int j)
{
	if(j == 2){			
		glPushMatrix();
			glTranslatef((31 * -0.34) , 9.3, -9.6);
			glCallList(REPEAT1);
		glPopMatrix();				
	}
	if(j == 3){			
		glPushMatrix();
			glTranslatef(31 * -0.34, 9.3, -9.6);
			glCallList(REPEAT1);
		glPopMatrix();
		glPushMatrix();
			glTranslatef(31 * -.09, 9.3, -9.6);
			glCallList(REPEAT2);
		glPopMatrix();	
	}
	if(j == 4){			
		glPushMatrix();
			glTranslatef(31 * -0.34, 9.3, -9.6);
			glCallList(REPEAT1);
		glPopMatrix();
		glPushMatrix();
			glTranslatef(31 * -.09, 9.3, -9.6);
			glCallList(REPEAT2);
		glPopMatrix();	
		glPushMatrix();
			glTranslatef(31 * 0.12, 9.3, -9.6);
			glCallList(REPEAT3);
		glPopMatrix();
	}	
}

void display(void)
{
	int i, j;
	GLfloat xPos = -0.34;
	glLoadIdentity();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
	glTranslatef(0.0, 0.0, -10.0);

	extrudeSolidFromPolygon(letterM, sizeof(letterM), width2, REPEAT_SIDE, 
		REPEAT_EDGE, REPEAT1);
	extrudeSolidFromPolygon(letterO, sizeof(letterO), width2, REPEAT2_SIDE, 
		REPEAT2_EDGE, REPEAT2);
	extrudeSolidFromPolygon(letterT, sizeof(letterT), width2, REPEAT3_SIDE, 
		REPEAT3_EDGE, REPEAT3);
	extrudeSolidFromPolygon(letterH, sizeof(letterH), width2, REPEAT4_SIDE, 
		REPEAT4_EDGE, REPEAT4);			

	for(j = 1; j < 5; j++){ 
		width = 0.0;
checkErrors();
		for(i = 0; i < 10; i++){ 

			glPushMatrix();
				repeat(j);
			glPopMatrix();
			
			glPushMatrix();
				glRotatef(90.0, 0.0, 1.0, 0.0); 
				if(j == 1){
					extrudeSolidFromPolygon(letterM, sizeof(letterM), width, M_SIDE, 
						M_EDGE, M_WHOLE);
					glCallList(M_WHOLE);					
				}
				if(j == 2){
					extrudeSolidFromPolygon(letterO, sizeof(letterO), width, O_SIDE, 
						O_EDGE, O_WHOLE);
					glCallList(O_WHOLE);
				}				
				if(j == 3){
					extrudeSolidFromPolygon(letterT, sizeof(letterT), width, T_SIDE, 
						T_EDGE, T_WHOLE);
					glCallList(T_WHOLE);
				}
				if(j == 4){
					extrudeSolidFromPolygon(letterH, sizeof(letterH), width, H_SIDE, 
						H_EDGE, H_WHOLE);
					glCallList(H_WHOLE);
				}				
				glutSwapBuffers();
		    	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				width = width + 0.2;
			glPopMatrix();
		}
		for(i = 0; i < 45 ; i++){
				
			glPushMatrix();
				repeat(j);
			glPopMatrix();					
			
			glPushMatrix();					
				glRotatef(90.0 - (2.0 * i), 0.0, 1.0, 0.0);
				if(j == 1){
				glCallList(M_WHOLE);
				}
				if(j == 2){
				glCallList(O_WHOLE);
				}
				if(j == 3){
				glCallList(T_WHOLE);
				}
				if(j == 4){
				glCallList(H_WHOLE);
				}			
				glutSwapBuffers();
		    	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		
			glPopMatrix();
		} 
		for(i = 1; i < 32 ; i++){
			
			glPushMatrix();
				repeat(j);
			glPopMatrix();
			
			glPushMatrix();
				glTranslatef(i * xPos, i * 0.3, i * -0.3);
				if(j == 1){
				glCallList(M_WHOLE);
				}
				if(j == 2){
				glCallList(O_WHOLE);
				}
				if(j == 3){
				glCallList(T_WHOLE);
				}
				if(j == 4){
				glCallList(H_WHOLE);
				}		
				glutSwapBuffers();
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);				
			glPopMatrix();
		}				

		if(j == 1){
			xPos = xPos + 0.25;
		}
		else{
			xPos = xPos + 0.21;
		}
	}
	glFlush();
}

void myReshape(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
  glFrustum(-7.0, 7.0, -7.0, 7.0, 6.0, 20.0);
/*    if (w <= h) 
	glOrtho (-7.0, 7.0, -7.0*(GLfloat)h/(GLfloat)w, 
	    7.0*(GLfloat)h/(GLfloat)w, -10.0, 10.0);
    else 
	glOrtho (-7.0*(GLfloat)w/(GLfloat)h, 
	    7.0*(GLfloat)w/(GLfloat)h, -7.0, 7.0, -10.0, 10.0); */
    glMatrixMode(GL_MODELVIEW);
}

/*  Main Loop
 *  Open window with initial window size, title bar, 
 *  RGBA display mode, and handle input events.
 */
int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutCreateWindow ("text3d");

/*glCullFace(GL_FRONT);*/
/*glEnable(GL_CULL_FACE);*/

    myinit();
    glutReshapeFunc (myReshape);
    glutDisplayFunc (display);
    glutMainLoop();
    return 0;             /* ANSI C requires main to return int. */
}
