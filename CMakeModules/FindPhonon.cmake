# Find libphonon
# Once done this will define
#
#  PHONON_FOUND    - system has Phonon Library
#  PHONON_INCLUDES - the Phonon include directory
#  PHONON_LIBS     - link these to use Phonon
#  PHONON_VERSION  - the version of the Phonon Library

# Copyright (c) 2008, Matthias Kretz <kretz@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

macro(_phonon_find_version)
   set(_phonon_namespace_header_file "${PHONON_INCLUDE_DIR}/phonon/phononnamespace.h")
   if (APPLE AND EXISTS "${PHONON_INCLUDE_DIR}/Headers/phononnamespace.h")
      set(_phonon_namespace_header_file "${PHONON_INCLUDE_DIR}/Headers/phononnamespace.h")
   endif (APPLE AND EXISTS "${PHONON_INCLUDE_DIR}/Headers/phononnamespace.h")
   file(READ ${_phonon_namespace_header_file} _phonon_header LIMIT 5000 OFFSET 1000)
   string(REGEX MATCH "define PHONON_VERSION_STR \"(4\\.[0-9]+\\.[0-9a-z]+)\"" _phonon_version_match "${_phonon_header}")
   set(PHONON_VERSION "${CMAKE_MATCH_1}")
endmacro(_phonon_find_version)

# the dirs listed with HINTS are searched before the default sets of dirs
find_library(PHONON_LIBRARY NAMES phonon HINTS ${KDE4_LIB_INSTALL_DIR} ${QT_LIBRARY_DIR})
find_path(PHONON_INCLUDE_DIR NAMES phonon/phonon_export.h HINTS ${KDE4_INCLUDE_INSTALL_DIR} ${QT_INCLUDE_DIR} ${INCLUDE_INSTALL_DIR} ${QT_LIBRARY_DIR})

if(PHONON_INCLUDE_DIR AND PHONON_LIBRARY)
   set(PHONON_LIBS ${phonon_LIB_DEPENDS} ${PHONON_LIBRARY})
   set(PHONON_INCLUDES ${PHONON_INCLUDE_DIR}/phonon ${PHONON_INCLUDE_DIR}/KDE ${PHONON_INCLUDE_DIR})
   _phonon_find_version()
endif(PHONON_INCLUDE_DIR AND PHONON_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Phonon  DEFAULT_MSG  PHONON_INCLUDE_DIR PHONON_LIBRARY)

mark_as_advanced(PHONON_INCLUDE_DIR PHONON_LIBRARY)
