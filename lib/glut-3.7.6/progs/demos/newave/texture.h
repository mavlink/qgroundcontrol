
/* texture.h - by David Blythe, SGI */

/* Simple SGI .rgb image file loader routine. */
unsigned *
read_texture(char *name, int *width, int *height, int *components);

extern void imgLoad(char *filenameIn, 
  int borderIn, GLfloat borderColorIn[4],
  int *wOut, int *hOut, GLubyte ** imgOut);

