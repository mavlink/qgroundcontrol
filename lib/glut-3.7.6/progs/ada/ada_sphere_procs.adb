
with GL; use GL;
with GLU; use GLU;
with Glut; use Glut;
with GNAT.OS_Lib;

package body ada_sphere_procs is

  procedure display is

  begin
    glClear(GL_COLOR_BUFFER_BIT or GL_DEPTH_BUFFER_BIT);
    glutSolidSphere(1.0, 10, 10);
    glutSwapBuffers;
  end display;

  procedure reshape(w : Integer; h : Integer) is

  begin
    glViewport(0, 0, GLsizei(w), GLsizei(h));
  end reshape;

  procedure menu (value : Integer) is

  begin
    if (value = 666) then
      GNAT.OS_Lib.OS_Exit (0);
    end if;
  end menu;

  procedure init is

    light_diffuse : array(0 .. 3) of aliased GLfloat :=
      (1.0, 0.0, 0.0, 1.0);
    light_position : array(0 .. 3) of aliased GLfloat :=
      (1.0, 1.0, 1.0, 0.0);

  begin
    glClearColor(0.1, 0.1, 0.1, 0.0);

    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse(0)'access);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position(0)'access);

    glFrontFace(GL_CW);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glMatrixMode(GL_PROJECTION);
    gluPerspective(40.0, 1.0, 1.0, 10.0);

    glMatrixMode(GL_MODELVIEW);
    gluLookAt(
      0.0, 0.0, -5.0,
      0.0, 0.0, 0.0,
      0.0, 1.0, 0.0);
  end init;

end ada_sphere_procs;
