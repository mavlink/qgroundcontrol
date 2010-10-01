
/* Copyright (c) Mark J. Kilgard, 1996, 1997. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

/* Test glutExtensionSupported. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GL/glut.h>

void
wrangleExtensionName(char *extension)
{
  char buffer[512];
  int rc, len;

  sprintf(buffer, " %s", extension);
  rc = glutExtensionSupported(buffer);
  if (rc) {
    printf("FAIL: test20, space prefix\n");
    exit(1);
  }

  sprintf(buffer, "%s ", extension);
  rc = glutExtensionSupported(buffer);
  if (rc) {
    printf("FAIL: test20, space suffix\n");
    exit(1);
  }

  sprintf(buffer, "GL_%s", extension);
  rc = glutExtensionSupported(buffer);
  if (rc) {
    printf("FAIL: test20, GL_ prefix\n");
    exit(1);
  }

  sprintf(buffer, "%s", extension + 1);
  rc = glutExtensionSupported(buffer);
  if (rc) {
    printf("FAIL: test20, missing first character\n");
    exit(1);
  }

  sprintf(buffer, "%s", extension);
  len = (int) strlen(buffer);
  if(len > 0) {
    buffer[len-1] = '\0';
  }
  rc = glutExtensionSupported(buffer);
  if (rc) {
    printf("FAIL: test20, mising last character\n");
    exit(1);
  }

  sprintf(buffer, "%s", extension);
  len = (int) strlen(buffer);
  if(len > 0) {
    buffer[len-1] = 'X';
  }
  rc = glutExtensionSupported(buffer);
  if (rc) {
    printf("FAIL: test20, changed last character\n");
    exit(1);
  }
}

int
main(int argc, char **argv)
{
  char *extension;
  int rc;

  glutInit(&argc, argv);
  glutCreateWindow("test20");

  extension = "GL_EXT_blend_color";
  rc = glutExtensionSupported(extension);
  printf("Extension %s is %s by your OpenGL.\n", extension, rc ? "SUPPORTED" : "NOT supported");
  if (rc) wrangleExtensionName(extension);

  extension = "GL_EXT_abgr";
  rc = glutExtensionSupported(extension);
  printf("Extension %s is %s by your OpenGL.\n", extension, rc ? "SUPPORTED" : "NOT supported");
  if (rc) wrangleExtensionName(extension);

  extension = "GL_EXT_blend_minmax";
  rc = glutExtensionSupported(extension);
  printf("Extension %s is %s by your OpenGL.\n", extension, rc ? "SUPPORTED" : "NOT supported");
  if (rc) wrangleExtensionName(extension);

  extension = "GL_EXT_blend_logic_op";
  rc = glutExtensionSupported(extension);
  printf("Extension %s is %s by your OpenGL.\n", extension, rc ? "SUPPORTED" : "NOT supported");
  if (rc) wrangleExtensionName(extension);

  extension = "GL_EXT_blend_subtract";
  rc = glutExtensionSupported(extension);
  printf("Extension %s is %s by your OpenGL.\n", extension, rc ? "SUPPORTED" : "NOT supported");
  if (rc) wrangleExtensionName(extension);

  extension = "GL_EXT_polygon_offset";
  rc = glutExtensionSupported(extension);
  printf("Extension %s is %s by your OpenGL.\n", extension, rc ? "SUPPORTED" : "NOT supported");
  if (rc) wrangleExtensionName(extension);

  extension = "GL_EXT_subtexture";
  rc = glutExtensionSupported(extension);
  printf("Extension %s is %s by your OpenGL.\n", extension, rc ? "SUPPORTED" : "NOT supported");
  if (rc) wrangleExtensionName(extension);

  extension = "GL_EXT_texture";
  rc = glutExtensionSupported(extension);
  printf("Extension %s is %s by your OpenGL.\n", extension, rc ? "SUPPORTED" : "NOT supported");
  if (rc) wrangleExtensionName(extension);

  extension = "GL_EXT_texture_object";
  rc = glutExtensionSupported(extension);
  printf("Extension %s is %s by your OpenGL.\n", extension, rc ? "SUPPORTED" : "NOT supported");
  if (rc) wrangleExtensionName(extension);

  extension = "GL_SGIX_framezoom";
  rc = glutExtensionSupported(extension);
  printf("Extension %s is %s by your OpenGL.\n", extension, rc ? "SUPPORTED" : "NOT supported");
  if (rc) wrangleExtensionName(extension);

  rc = glutExtensionSupported("");
  if (rc) {
    printf("FAIL: test20, null string\n");
    exit(1);
  }

  printf("PASS: test20\n");
  return 0;             /* ANSI C requires main to return int. */
}
