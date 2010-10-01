/************************************************************************/
/*									*/
/* fullscreen_stereo.c  --  GLUT support for full screen stereo mode	*/
/*			    on SGI workstations.			*/
/*									*/
/* 24-Oct-95	Mike Blackwell  mkb@cs.cmu.edu				*/
/*	Written.							*/
/*									*/
/************************************************************************/

/* Standard screen diminsions */
#define XMAXSCREEN	1280
#define YMAXSCREEN	1024

#define YSTEREO		491		/* Subfield height in pixels */
#define YOFFSET_LEFT	532		/* YSTEREO + YBLANK */

void start_fullscreen_stereo(void);
void stop_fullscreen_stereo(void);
void stereo_left_buffer(void);
void stereo_right_buffer(void);
void window_no_border(void);
