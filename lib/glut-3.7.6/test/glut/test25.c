
/* Copyright (c) Mark J. Kilgard, 1997. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

/* This tests glutBitmapLength and glutStrokeLength. */

#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>

unsigned char *abc = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
char *null = "";
char *space = "      ";

void *bitmap_fonts[] =
{
  GLUT_BITMAP_TIMES_ROMAN_24,
  GLUT_BITMAP_TIMES_ROMAN_10,
  GLUT_BITMAP_9_BY_15,
  GLUT_BITMAP_8_BY_13,
  GLUT_BITMAP_HELVETICA_10,
  GLUT_BITMAP_HELVETICA_12,
  GLUT_BITMAP_HELVETICA_18
};
#define NUM_BITMAP_FONTS (sizeof(bitmap_fonts)/sizeof(void*))
char *bitmap_names[NUM_BITMAP_FONTS] =
{
  "Times Roman 24",
  "Times Roman 10",
  "9 by 15",
  "8 by 13",
  "Helvetica 10",
  "Helvetica 12",
  "Helvetica 18",
};
int bitmap_lens[NUM_BITMAP_FONTS] =
{
  2399,
  1023,
  2016,
  1792,
  1080,
  1291,
  1895
};
int bitmap_abc_lens[NUM_BITMAP_FONTS] =
{
  713,
  305,
  468,
  416,
  318,
  379,
  572,
};
void *stroke_fonts[] =
{
  GLUT_STROKE_ROMAN,
  GLUT_STROKE_MONO_ROMAN
};
#define NUM_STROKE_FONTS (sizeof(stroke_fonts)/sizeof(void*))
char *stroke_names[NUM_STROKE_FONTS] =
{
  "Roman",
  "Monospaced Roman"
};
int stroke_lens[NUM_STROKE_FONTS] =
{
  6635,
  9984
};
int stroke_abc_lens[NUM_STROKE_FONTS] =
{
  3683,
  5408
};

int
main(int argc, char **argv)
{
  void *font;
  int total;
  int i, j;

  glutInit(&argc, argv);

  /* Touch test the width determination of all bitmap
     characters. */
  for (i = 0; i < NUM_BITMAP_FONTS; i++) {
    font = bitmap_fonts[i];
    total = 0;
    for (j = -2; j < 259; j++) {
      total += glutBitmapWidth(font, j);
    }
    printf("  %s: bitmap total = %d (expected %d)\n", bitmap_names[i], total, bitmap_lens[i]);
    if (total != bitmap_lens[i]) {
      printf("FAIL: test25\n");
      exit(1);
    }
  }

  /* Touch test the width determination of all stroke
     characters. */
  for (i = 0; i < NUM_STROKE_FONTS; i++) {
    font = stroke_fonts[i];
    total = 0;
    for (j = -2; j < 259; j++) {
      total += glutStrokeWidth(font, j);
    }
    printf("  %s: stroke total = %d (expected %d)\n", stroke_names[i], total, stroke_lens[i]);
    if (total != stroke_lens[i]) {
      printf("FAIL: test25\n");
      exit(1);
    }
  }

  for (i = 0; i < NUM_BITMAP_FONTS; i++) {
    font = bitmap_fonts[i];
    total = glutBitmapLength(font, abc);
    printf("  %s: bitmap abc len = %d (expected %d)\n", bitmap_names[i], total, bitmap_abc_lens[i]);
    if (total != bitmap_abc_lens[i]) {
      printf("FAIL: test25\n");
      exit(1);
    }
  }

  for (i = 0; i < NUM_BITMAP_FONTS; i++) {
    font = bitmap_fonts[i];
    total = glutBitmapLength(font, "");
    printf("  %s: bitmap abc len = %d (expected %d)\n", bitmap_names[i], total, 0);
    if (total != 0) {
      printf("FAIL: test25\n");
      exit(1);
    }
  }

  for (i = 0; i < NUM_STROKE_FONTS; i++) {
    font = stroke_fonts[i];
    total = glutStrokeLength(font, abc);
    printf("  %s: stroke abc len = %d (expected %d)\n", stroke_names[i], total, stroke_abc_lens[i]);
    if (total != stroke_abc_lens[i]) {
      printf("FAIL: test25\n");
      exit(1);
    }
  }

  for (i = 0; i < NUM_STROKE_FONTS; i++) {
    font = stroke_fonts[i];
    total = glutStrokeLength(font, "");
    printf("  %s: stroke null len = %d (expected %d)\n", stroke_names[i], total, 0);
    if (total != 0) {
      printf("FAIL: test25\n");
      exit(1);
    }
  }

  printf("PASS: test25\n");
  return 0;             /* ANSI C requires main to return int. */
}
