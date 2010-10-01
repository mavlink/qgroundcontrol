
with GL; use GL;
with Glut; use Glut;

package ada_sphere_procs is
  procedure display;
  procedure reshape (w : Integer; h : Integer);
  procedure menu (value : Integer);
  procedure init;
end ada_sphere_procs;
