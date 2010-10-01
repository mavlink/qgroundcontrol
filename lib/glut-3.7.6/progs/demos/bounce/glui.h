/*
  glui.h
  Nate Robins, 1997.

  OpenGL based user interface.

 */

int
gluiHorizontalSlider(int parent, int x, int y, int width, int height,
		     float percent, void (*update)(float));

int
gluiVerticalSlider(int parent, int x, int y, int width, int height,
		   float percent, void (*update)(float));

void
gluiReshape(int width, int height);
