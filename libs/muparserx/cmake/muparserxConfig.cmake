if(DEFINED INCLUDED_MUPARSERX_CONFIG_CMAKE)
  return()
endif()
set(INCLUDED_MUPARSERX_CONFIG_CMAKE TRUE)

########################################################################
# muparserxConfig - cmake project configuration
#
# The following will be set after find_package(muparserx CONFIG):
# muparserx_LIBRARIES    - development libraries
# muparserx_INCLUDE_DIRS - development includes
########################################################################

########################################################################
## installation root
########################################################################
if (UNIX)
  get_filename_component(MUPARSERX_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
elseif (WIN32)
  get_filename_component(MUPARSERX_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
endif ()

########################################################################
## locate the library
########################################################################
find_library(
  MUPARSERX_LIBRARY muparserx
  PATHS ${MUPARSERX_ROOT}/lib${LIB_SUFFIX}
  PATH_SUFFIXES ${CMAKE_LIBRARY_ARCHITECTURE}
  NO_DEFAULT_PATH
  )
if(NOT MUPARSERX_LIBRARY)
  message(FATAL_ERROR "cannot find muparserx library in ${MUPARSERX_ROOT}/lib${LIB_SUFFIX}")
endif()
set(muparserx_LIBRARIES ${MUPARSERX_LIBRARY})

########################################################################
## locate the includes
########################################################################
find_path(
  MUPARSERX_INCLUDE_DIR mpDefines.h
  PATHS ${MUPARSERX_ROOT}/include/muparserx
  NO_DEFAULT_PATH
)
if(NOT MUPARSERX_INCLUDE_DIR)
  message(FATAL_ERROR "cannot find muparserx includes in ${MUPARSERX_ROOT}/include/muparserx")
endif()
set(muparserx_INCLUDE_DIRS ${MUPARSERX_INCLUDE_DIR})

