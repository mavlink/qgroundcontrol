
/* Copyright (c) Mark J. Kilgard, 1997. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

/* This tests unexpected interactions between
   glutInitDisplayMode and glutInitDisplayString. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GL/glut.h>

int modes[] =
{
  GLUT_RGB | GLUT_SINGLE,
  GLUT_RGB | GLUT_DOUBLE,
  GLUT_INDEX | GLUT_SINGLE,
  GLUT_INDEX | GLUT_DOUBLE
};
#define NUM_MODES (sizeof(modes)/sizeof(modes[0]))

char *strings[] =
{
  "rgb double",
  "rgba double",
  "rgba single",
  "index",
  "index double",
  "rgb samples=4",
  "stencil depth red green blue alpha conformant auxbufs buffer acc acca double rgb rgba",
  "stereo index samples slow",
  NULL
};

char *ostrings[] =
{
  "index double",
  "index single",
  "index buffer=4",
  "index buffer=8",
  "index buffer~4",
  "index buffer=4 depth",
  NULL
};

int verbose;

int
main(int argc, char **argv)
{
  int k, i, j, win;
  int num, exists;
  char mode[200];

  glutInit(&argc, argv);
  if (argc > 1) {
    if (!strcmp(argv[1], "-v")) {
      verbose = 1;
    }
  }
  glutInitWindowPosition(10, 10);
  glutInitWindowSize(200, 200);
  for (k = 0; k < NUM_MODES; k++) {
    glutInitDisplayMode(modes[k]);
    printf("Display Mode = %d (%s,%s)\n", modes[k],
      modes[k] & GLUT_INDEX ? "index" : "rgba",
      modes[k] & GLUT_DOUBLE ? "double" : "single");
    for (i = 0; strings[i]; i++) {
      glutInitDisplayString(strings[i]);
      if (glutGet(GLUT_DISPLAY_MODE_POSSIBLE)) {
        if (verbose)
          printf("  Possible: %s\n", strings[i]);
        win = glutCreateWindow("test23");
        if (verbose)
          printf("    Created: %s\n", strings[i]);
        for (j = 0; ostrings[j]; j++) {
          glutInitDisplayString(ostrings[j]);
          if (glutLayerGet(GLUT_OVERLAY_POSSIBLE)) {
            if (verbose)
              printf("    Overlay possible: %s\n", ostrings[j]);
            glutEstablishOverlay();
            if (verbose)
              printf("      Overlay establish: %s\n", ostrings[j]);
            glutRemoveOverlay();
            if (verbose)
              printf("        Overlay remove: %s\n", ostrings[j]);
          }
        }
        glutDestroyWindow(win);
        if (verbose)
          printf("      Destroyed: %s\n", strings[i]);
      } else {
        if (verbose)
          printf("Not possible: %s\n", strings[i]);
      }
    }
  }

  glutInitDisplayString(NULL);

  num = 1;
  do {
    sprintf(mode, "rgb num=%d", num);
    glutInitDisplayString(mode);
    exists = glutGet(GLUT_DISPLAY_MODE_POSSIBLE);
    if (exists) {
      if (verbose)
        printf("  Possible: %s\n", mode);
      win = glutCreateWindow("test23");
      if (verbose)
        printf("    Created: %s\n", mode);
      glutDestroyWindow(win);
      if (verbose)
        printf("      Destroyed: %s\n", mode);
      sprintf(mode, "rgb num=0x%x", num);
      glutInitDisplayString(mode);
      exists = glutGet(GLUT_DISPLAY_MODE_POSSIBLE);
      if (!exists) {
        printf("FAIL: test23 (hex num= don't work)\n");
        exit(1);
      }
      win = glutCreateWindow("test23");
      glutDestroyWindow(win);
      num++;
    } else {
      if (verbose)
        printf("Not possible: %s\n", mode);
    }
  } while (exists);

  glutInitDisplayString(NULL);

  printf("PASS: test23\n");
  return 0;             /* ANSI C requires main to return int. */
}
