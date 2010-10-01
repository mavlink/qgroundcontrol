#ifndef TEXTMAPDEF
#define TEXTMAPDEF

#define LETTER_INDEX 1

typedef struct texchardesc {
  float movex;          /* advance */
  int haveimage;
  float llx, lly;       /* geometry box */
  float urx, ury;
  float tllx, tlly;     /* texture box */
  float turx, tury;
  float data[3 * 8];
} texchardesc;

typedef struct texfnt {
  short charmin, charmax;
  short nchars;
  float pixhigh;
  texchardesc *chars;
  short rasxsize, rasysize;
  unsigned short *rasdata;
} texfnt;

texfnt *readtexfont(char *name);
float texstrwidth(char *str);
int texfntinit(char *file);
void texfntstroke(char *s, float xoffset, float yoffset);

#endif
