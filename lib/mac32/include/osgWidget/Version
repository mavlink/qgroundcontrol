// Code by: Jeremy Moles (cubicool) 2007-2008

#ifndef OSGWIDGET_VERSION
#define OSGWIDGET_VERSION

#include <osgWidget/Export>

extern "C" {

/**
 * osgWidgetGetVersion() returns the library version number.
 * Numbering convention : OpenSceneGraph-1.0 will return 1.0 from osgWidgetGetVersion.
 *
 * This C function can be also used to check for the existence of the OpenSceneGraph
 * library using autoconf and its m4 macro AC_CHECK_LIB.
 *
 * Here is the code to add to your configure.in:
 \verbatim
 #
 # Check for the OpenSceneGraph (OSG) Util library
 #
 AC_CHECK_LIB(osg, osgWidgetGetVersion, ,
    [AC_MSG_ERROR(OpenSceneGraph Util library not found. See http://www.openscenegraph.org)],)
 \endverbatim
*/
extern OSGWIDGET_EXPORT const char* osgWidgetGetVersion();

/**
 * osgWidgetGetLibraryName() returns the library name in human friendly form.
 */
extern OSGWIDGET_EXPORT const char* osgWidgetGetLibraryName();

}

#endif
