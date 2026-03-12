#define GEOGRAPHICLIB_VERSION_STRING "2.7"
#define GEOGRAPHICLIB_VERSION_MAJOR 2
#define GEOGRAPHICLIB_VERSION_MINOR 7
#define GEOGRAPHICLIB_VERSION_PATCH 0
#define GEOGRAPHICLIB_DATA "C:/ProgramData/GeographicLib"

// These are macros which affect the building of the library
#define GEOGRAPHICLIB_HAVE_LONG_DOUBLE 1
#define GEOGRAPHICLIB_WORDS_BIGENDIAN 0
#define GEOGRAPHICLIB_PRECISION 2

// Specify whether GeographicLib is a shared or static library.  When compiling
// under Visual Studio it is necessary to specify whether GeographicLib is a
// shared library.  This is done with the macro GEOGRAPHICLIB_SHARED_LIB, which
// cmake will correctly define as 0 or 1 when only one type of library is in
// the package.  If both shared and static libraries are available,
// GEOGRAPHICLIB_SHARED_LIB is set to 2 which triggers a preprocessor error in
// Constants.hpp.  In this case, the appropriate value (0 or 1) for
// GEOGRAPHICLIB_SHARED_LIB must be specified when compiling any program that
// includes GeographicLib headers.  This is done automatically if GeographicLib
// and the user's code were built with cmake version 2.8.11 (which introduced
// the command target_compile_definitions) or later.
#if !defined(GEOGRAPHICLIB_SHARED_LIB)
#define GEOGRAPHICLIB_SHARED_LIB 0
#endif
