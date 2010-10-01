#ifndef IZOOMDEF
#define IZOOMDEF

/**
 **     header for izoom- 
 **             Magnify or minify a picture with or without filtering.  The 
 **     filtered method is one pass, uses 2-d convolution, and is optimized 
 **     by integer arithmetic and precomputation of filter coeffs.
 **
 **                             Paul Haeberli - 1988
 **/

#define IMPULSE		1
#define BOX		2
#define TRIANGLE	3
#define QUADRATIC	4
#define MITCHELL	5
#define GAUSSIAN	6

typedef struct FILTER {
  int n, totw, halftotw;
  short *dat;
  short *w;
} FILTER;

typedef void (*getfunc_t) (short *, int);

typedef struct zoom {
  getfunc_t getfunc;
  short *abuf;
  short *bbuf;
  int anx, any;
  int bnx, bny;
  short **xmap;
  int type;
  int curay;
  int y;
  FILTER *xfilt, *yfilt;  /* stuff for fitered zoom */
  short *tbuf;
  int nrows, clamp, ay;
  short **filtrows;
  int *accrow;
} zoom;

zoom *newzoom(getfunc_t getfunc, int anx, int any, int bnx, int bny, int filttype, float blur);
float filterinteg(float bmin, float bmax, float blurf);
void filterzoom(getfunc_t getfunc, getfunc_t putfunc, int anx, int any, int bnx, int bny, int filttype, float blur);

#endif
