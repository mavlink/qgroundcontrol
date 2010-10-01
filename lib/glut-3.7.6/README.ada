
GLUT 3.4 was the first release of GLUT to support an Ada language
binding for SGI's Ada run-time and development environment.  (With a
bit of work, GLUT could probably be easily be adapted to other Ada
development environments, assuming the environment already has an
OpenGL binding.)

To use the SGI Ada binding, please make sure that the following GNAT
(SGI's Ada compiler) subsystems are installed on your system:

  Ada Execution-only Environment (eoe)
  -------------------------------------
    gnat_eoe.sw.lib

  Ada Development Option (dev)
  -----------------------------
    gnat_dev.bindings.GL
    gnat_dev.bindings.std
    gnat_dev.lib.src
    gnat_dev.lib.obj
    gnat_dev.sw.gnat

The GLUT Ada binding was developed and tested with the IRIX 5.3 and 6.2
gnat_dev and gnat_eoe images (v3.07, built 960827).

Some fairly simple GLUT examples written in Ada can be found in the
progs/ada subdirectory.  GLUT 3.6 expanded the number of Ada example
programs included in the GLUT source code distribution.  GLUT's actual
Ada binding is found in the adainclude/GL subdirectory.

To build the Ada binding and example programs, first build GLUT
normally, then:

  cd adainclude/GL
  make glut.o
  cd ../../progs/ada
  make

Good luck!

- Mark Kilgard
  November 12, 1997
