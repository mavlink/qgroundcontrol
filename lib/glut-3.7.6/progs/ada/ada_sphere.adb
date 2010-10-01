
with GL; use GL;
with Interfaces.C.Strings;
with Glut; use Glut;
with ada_sphere_procs; use ada_sphere_procs;

procedure ada_sphere is
  
  package ICS renames Interfaces.C.Strings;

  type chars_ptr_ptr is access ICS.chars_ptr;

  argc : aliased integer;
  pragma Import (C, argc, "gnat_argc");

  argv : chars_ptr_ptr;
  pragma Import (C, argv, "gnat_argv");

  win : Integer;
  m : Integer;

begin

  glutInit (argc'access, argv);

  glutInitDisplayMode(GLUT_RGB or GLUT_DEPTH or GLUT_DOUBLE);
  win := glutCreateWindow("ada_sphere");

  glutDisplayFunc(display'access);
  glutReshapeFunc(reshape'access);

  init;

  m := glutCreateMenu(menu'access);
  glutAddMenuEntry("Quit", 666);
  glutAttachMenu(GLUT_RIGHT_BUTTON);

  glutMainLoop;

end ada_sphere;
