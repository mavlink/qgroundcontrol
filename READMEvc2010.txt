
For use of qt 4x and visual studio2010 and add in.

The Visual studio adds automatically certain defines

In the projects properties -> C/C++ ->preprocessor change:

in DEBUG:
	delete QT_NO_DEBUG

Both:
	delete QT_NO_DYNAMIC_CAST
