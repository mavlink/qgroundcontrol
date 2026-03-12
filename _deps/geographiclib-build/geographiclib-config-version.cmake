# Version checking for GeographicLib

set (PACKAGE_VERSION "2.7")
set (PACKAGE_VERSION_MAJOR "2")
set (PACKAGE_VERSION_MINOR "7")
set (PACKAGE_VERSION_PATCH "0")

# These variable definitions parallel those in GeographicLib's
# cmake/CMakeLists.txt.
if (MSVC)
  # For checking the compatibility of MSVC_TOOLSET_VERSION; see
  # https://docs.microsoft.com/en-us/cpp/porting/overview-of-potential-upgrade-issues-visual-cpp
  # Assume major version number is obtained by dropping the last decimal
  # digit.
  math (EXPR MSVC_TOOLSET_MAJOR "${MSVC_TOOLSET_VERSION}/10")
endif ()

if (NOT PACKAGE_FIND_NAME STREQUAL "GeographicLib")
  # Check package name (in particular, because of the way cmake finds
  # package config files, the capitalization could easily be "wrong").
  # This is necessary to ensure that the automatically generated
  # variables, e.g., <package>_FOUND, are consistently spelled.
  set (REASON "package = GeographicLib, NOT ${PACKAGE_FIND_NAME}")
  set (PACKAGE_VERSION_UNSUITABLE TRUE)
elseif (NOT (APPLE OR (NOT DEFINED CMAKE_SIZEOF_VOID_P) OR
      CMAKE_SIZEOF_VOID_P EQUAL 8))
  # Reject if there's a 32-bit/64-bit mismatch (not necessary with Apple
  # since a multi-architecture library is built for that platform).
  set (REASON "sizeof(*void) =  8")
  set (PACKAGE_VERSION_UNSUITABLE TRUE)
elseif (MSVC AND NOT (
    # toolset version must be at least as great as GeographicLib's.
    MSVC_TOOLSET_VERSION GREATER_EQUAL 0
    # and major versions must match
    AND MSVC_TOOLSET_MAJOR EQUAL 0 ))
  # Reject if there's a mismatch in MSVC compiler versions.
  set (REASON "MSVC_TOOLSET_VERSION = 0")
  set (PACKAGE_VERSION_UNSUITABLE TRUE)
elseif (GEOGRAPHICLIB_PRECISION MATCHES "^[1-9][0-9]*\$" AND NOT (
      GEOGRAPHICLIB_PRECISION EQUAL 2 ))
  # Reject if the user asks for an incompatible precsision.
  set (REASON "GEOGRAPHICLIB_PRECISION = 2")
  set (PACKAGE_VERSION_UNSUITABLE TRUE)
elseif (CMAKE_CROSSCOMPILING AND
    NOT (CMAKE_SYSTEM_NAME STREQUAL "Windows" AND
      CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64"))
  # Reject if cross-compiling and there's a mismatch in the target system.
  set (REASON "target = Windows-AMD64")
  set (PACKAGE_VERSION_UNSUITABLE TRUE)
elseif (PACKAGE_FIND_VERSION)
  if (PACKAGE_FIND_VERSION VERSION_EQUAL PACKAGE_VERSION)
    set (PACKAGE_VERSION_EXACT TRUE)
  elseif (PACKAGE_FIND_VERSION VERSION_LESS PACKAGE_VERSION
    AND PACKAGE_FIND_VERSION_MAJOR EQUAL PACKAGE_VERSION_MAJOR)
    set (PACKAGE_VERSION_COMPATIBLE TRUE)
  endif ()
endif ()

set (GeographicLib_SHARED_FOUND OFF)
set (GeographicLib_STATIC_FOUND ON)

# Check for the components requested.  The convention is that
# GeographicLib_${comp}_FOUND should be true for all the required
# components.
if (GeographicLib_FIND_COMPONENTS)
  foreach (comp ${GeographicLib_FIND_COMPONENTS})
    if (GeographicLib_FIND_REQUIRED_${comp} AND
        NOT GeographicLib_${comp}_FOUND)
      set (REASON "without ${comp}")
      set (PACKAGE_VERSION_UNSUITABLE TRUE)
    endif ()
  endforeach ()
endif ()

# If unsuitable, append the reason to the package version so that it's
# visible to the user.
if (PACKAGE_VERSION_UNSUITABLE)
  set (PACKAGE_VERSION "${PACKAGE_VERSION} (${REASON})")
endif ()
