# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause
#.rst:
# FindFFmpeg
# ----------
#
# Try to find the required ffmpeg components (default: AVFORMAT, AVUTIL, AVCODEC)
#
# Next variables can be used to hint FFmpeg libs search:
#
# ::
#
#   PC_<component>_LIBRARY_DIRS
#   PC_FFMPEG_LIBRARY_DIRS
#   PC_<component>_INCLUDE_DIRS
#   PC_FFMPEG_INCLUDE_DIRS
#
# Once done this will define
#
# ::
#
#   FFMPEG_FOUND         - System has the all required components.
#   FFMPEG_SHARED_LIBRARIES - Found FFmpeg shared libraries.
#
# For each of the components it will additionally set.
#
# ::
#
#   AVCODEC
#   AVDEVICE
#   AVFORMAT
#   AVFILTER
#   AVUTIL
#   POSTPROC
#   SWSCALE
#
# the following variables will be defined
#
# ::
#
#   <component>_FOUND        - System has <component>
#   FFMPEG_<component>_FOUND - System has <component> (as checked by FHSPA)
#   <component>_INCLUDE_DIRS - Include directory necessary for using the <component> headers
#   <component>_LIBRARIES    - Link these to use <component>
#   <component>_LIBRARY_DIRS - Link directories
#   <component>_DEFINITIONS  - Compiler switches required for using <component>
#   <component>_VERSION      - The components version
#
# the following import targets is created
#
# ::
#
#   FFmpeg::FFmpeg - for all components
#   FFmpeg::<component> - where <component> in lower case (FFmpeg::avcodec) for each components
#
# Copyright (c) 2006, Matthias Kretz, <kretz@kde.org>
# Copyright (c) 2008, Alexander Neundorf, <neundorf@kde.org>
# Copyright (c) 2011, Michael Jansen, <kde@michael-jansen.biz>
# Copyright (c) 2017, Alexander Drozdov, <adrozdoff@gmail.com>
#

include(FindPackageHandleStandardArgs)

# The default components were taken from a survey over other FindFFMPEG.cmake files
if (NOT FFmpeg_FIND_COMPONENTS)
  set(FFmpeg_FIND_COMPONENTS AVCODEC AVFORMAT AVUTIL)
endif ()

if (QT_DEPLOY_FFMPEG AND BUILD_SHARED_LIBS)
    set(shared_libs_desired TRUE)
endif()

# finds FFmpeg libs, including symlinks, for the specified component.
macro(find_shared_libs_for_component _component)
  # the searching pattern is pretty rough but it seems to be sufficient to gather dynamic libs
  get_filename_component(name_we ${${_component}_LIBRARY} NAME_WE)

  if (WIN32)
    get_filename_component(dir_name ${${_component}_LIBRARY_DIR} NAME)
    if (${dir_name} STREQUAL "lib" AND EXISTS "${${_component}_LIBRARY_DIR}/../bin")
      # llvm-mingv builds aux ffmpeg static libs like lib/libavutil.dll.a and cmake finds
      # only them even though the folder bin/ contains proper *.dll and *.lib.

      string(REGEX REPLACE "^lib" "" name_we "${name_we}")
      set(shared_lib_pattern "../bin/${name_we}*${CMAKE_SHARED_LIBRARY_SUFFIX}")
    else()
      set(shared_lib_pattern "${name_we}*${CMAKE_SHARED_LIBRARY_SUFFIX}")
    endif()

  else()
    set(shared_lib_pattern "${name_we}*${CMAKE_SHARED_LIBRARY_SUFFIX}*")
  endif()

  file(GLOB ${_component}_SHARED_LIBRARIES "${${_component}_LIBRARY_DIR}/${shared_lib_pattern}")
endmacro()

#
### Macro: set_component_found
#
# Marks the given component as found if both *_LIBRARY_NAME AND *_INCLUDE_DIRS is present.
#
macro(set_component_found _component)
  if (${_component}_LIBRARY_NAME AND ${_component}_INCLUDE_DIR)
      # message(STATUS "  - ${_component} found.")
    set(${_component}_FOUND TRUE)
    set(${CMAKE_FIND_PACKAGE_NAME}_${_component}_FOUND TRUE)
  else ()
    # message(STATUS "  - ${_component} not found.")
  endif ()
endmacro()

find_package(PkgConfig QUIET)
if (NOT PKG_CONFIG_FOUND AND NOT FFMPEG_DIR)
    set(FFMPEG_DIR "/usr/local")
endif()
#
### Macro: find_component
#
# Checks for the given component by invoking pkgconfig and then looking up the libraries and
# include directories.
#
macro(find_component _component _pkgconfig _library _header)

  # use pkg-config to get the directories and then use these values
  # in the FIND_PATH() and FIND_LIBRARY() calls
  if (PKG_CONFIG_FOUND AND NOT FFMPEG_DIR)
    pkg_check_modules(PC_${_component} ${_pkgconfig})
  endif ()

  if (FFMPEG_DIR OR FFMPEG_ROOT)
      set(__find_ffmpeg_backup_root_dir "${CMAKE_FIND_ROOT_PATH}")
  endif()

  if(FFMPEG_DIR)
       list(APPEND CMAKE_FIND_ROOT_PATH "${FFMPEG_DIR}")
  endif()

  if(FFMPEG_ROOT)
      list(APPEND CMAKE_FIND_ROOT_PATH "${FFMPEG_ROOT}")
  endif()

  if (${_component}_INCLUDE_DIR AND NOT EXISTS ${${_component}_INCLUDE_DIR})
    message(STATUS "Cached include dir ${${_component}_INCLUDE_DIR} doesn't exist")
    unset(${_component}_INCLUDE_DIR CACHE)
  endif()

  find_path(${_component}_INCLUDE_DIR ${_header}
    HINTS
      ${PC_${_component}_INCLUDEDIR}
      ${PC_${_component}_INCLUDE_DIRS}
      ${PC_FFMPEG_INCLUDE_DIRS}
    PATHS
      ${FFMPEG_DIR}
    PATH_SUFFIXES
      ffmpeg include
  )

  if (shared_libs_desired AND NOT WIN32)
    set(CMAKE_FIND_LIBRARY_SUFFIXES "${CMAKE_SHARED_LIBRARY_SUFFIX};${CMAKE_STATIC_LIBRARY_SUFFIX}")
  endif()

  if (${_component}_LIBRARY AND NOT EXISTS ${${_component}_LIBRARY})
    message(STATUS "Cached library ${${_component}_LIBRARY} doesn't exist")
    unset(${_component}_LIBRARY CACHE)
  endif()

  find_library(${_component}_LIBRARY
    NAMES ${PC_${_component}_LIBRARIES} ${_library}
    HINTS
      ${PC_${_component}_LIBDIR}
      ${PC_${_component}_LIBRARY_DIRS}
      ${PC_FFMPEG_LIBRARY_DIRS}
    PATHS
      ${FFMPEG_DIR}
    PATH_SUFFIXES
      lib bin
  )

  if(FFMPEG_DIR OR FFMPEG_ROOT)
    set(CMAKE_FIND_ROOT_PATH "${__find_ffmpeg_backup_root_dir}")
  endif()

  if (${_component}_LIBRARY)
    get_filename_component(${_component}_LIBRARY_DIR ${${_component}_LIBRARY} DIRECTORY)
    get_filename_component(${_component}_LIBRARY_NAME ${${_component}_LIBRARY} NAME)

    # On Windows, shared linking goes through 'integration' static libs, so we should look for shared ones anyway
    # On Unix, we gather symlinks as well so that we could install them.
    if (WIN32 OR ${${_component}_LIBRARY_NAME} MATCHES "\\${CMAKE_SHARED_LIBRARY_SUFFIX}$")
      find_shared_libs_for_component(${_component})
    endif()

  endif()

  set(${_component}_DEFINITIONS  ${PC_${_component}_CFLAGS_OTHER})
  set_component_found(${_component})

  mark_as_advanced(${_component}_LIBRARY)
endmacro()

# Clear the previously cached variables, because they are recomputed every time
# the Find script is included.
unset(FFMPEG_SHARED_LIBRARIES CACHE)
unset(FFMPEG_STUBS CACHE)

# Check for components.
foreach (_component ${FFmpeg_FIND_COMPONENTS})
  string(TOLOWER ${_component} library)
  find_component(${_component} "lib${library}" ${library} "lib${library}/${library}.h")

  if (${_component}_FOUND)
    list(APPEND FFMPEG_LIBRARIES    ${${_component}_LIBRARY_NAME})
    list(APPEND FFMPEG_DEFINITIONS  ${${_component}_DEFINITIONS})
    list(APPEND FFMPEG_INCLUDE_DIRS ${${_component}_INCLUDE_DIR})
    list(APPEND FFMPEG_LIBRARY_DIRS ${${_component}_LIBRARY_DIR})

    if (${_component}_SHARED_LIBRARIES)
      list(APPEND FFMPEG_SHARED_LIBRARIES ${${_component}_SHARED_LIBRARIES})
      list(APPEND FFMPEG_SHARED_COMPONENTS ${_component})
    else()
      list(APPEND FFMPEG_STATIC_COMPONENTS ${_component})
    endif()

    mark_as_advanced(${_component}_LIBRARY_NAME ${_component}_DEFINITIONS ${_component}_INCLUDE_DIR
        ${_component}_LIBRARY_DIR ${_component}_SHARED_LIBRARIES)
  endif()
endforeach()


function(qt_internal_multimedia_try_add_dynamic_resolve_dependency _component dep)
  set(dynamic_resolve_added FALSE PARENT_SCOPE)

  if (NOT ANDROID AND NOT LINUX)
    return()
  endif()

  set(supported_stubs "ssl|crypto|va|va-drm|va-x11")
  if(${_component}_SHARED_LIBRARIES)
    set(stub_prefix "Qt${PROJECT_VERSION_MAJOR}FFmpegStub-")
    if (${dep} MATCHES "^${stub_prefix}(${supported_stubs})$")
      string(REPLACE "${stub_prefix}" "" dep "${dep}")
      set(FFMPEG_STUBS ${FFMPEG_STUBS} ${dep} CACHE INTERNAL "")

      set(dynamic_resolve_added TRUE PARENT_SCOPE)
    endif()
  elseif (${dep} MATCHES "^(${supported_stubs})$")
    set(FFMPEG_STUBS ${FFMPEG_STUBS} ${dep} CACHE INTERNAL "")
    set(dynamic_resolve_added TRUE PARENT_SCOPE)
  endif()
endfunction()

# Function parses package config file to find the static library dependencies
# and adds them to the target library.
function(__ffmpeg_internal_set_dependencies _component)
  string(TOLOWER ${_component} lib)
  set(PC_FILE ${${_component}_LIBRARY_DIR}/pkgconfig/lib${lib}.pc)
  if(EXISTS ${PC_FILE})
    file(READ ${PC_FILE} pcfile)

    set(prefix_l "(^| )\\-l")
    set(suffix_lib "\\.lib($| )")

    string(REGEX REPLACE ".*Libs:([^\n\r]+).*" "\\1" out "${pcfile}")
    string(REGEX MATCHALL "${prefix_l}[^ ]+" libs_dependency ${out})
    string(REGEX MATCHALL "[^ ]+${suffix_lib}" libs_dependency_lib ${out})

    string(REGEX REPLACE ".*Libs.private:([^\n\r]+).*" "\\1" out "${pcfile}")
    string(REGEX MATCHALL "${prefix_l}[^ ]+" libs_private_dependency ${out})
    string(REGEX MATCHALL "[^ ]+${suffix_lib}" libs_private_dependency_lib ${out})

    list(APPEND deps_no_suffix ${libs_dependency} ${libs_private_dependency})
    foreach(dependency ${deps_no_suffix})
      string(REGEX REPLACE ${prefix_l} "" dependency ${dependency})
      if(NOT ${lib} STREQUAL ${dependency})
        qt_internal_multimedia_try_add_dynamic_resolve_dependency(${_component} ${dependency})
        if(NOT dynamic_resolve_added AND NOT ${_component}_SHARED_LIBRARIES)
          target_link_libraries(FFmpeg::${lib} INTERFACE ${dependency})
        endif()
      endif()
    endforeach()

    if(NOT ${_component}_SHARED_LIBRARIES)
      list(APPEND deps_lib_suffix ${libs_dependency_lib} ${libs_private_dependency_lib})
      foreach(dependency ${deps_lib_suffix})
        string(REGEX REPLACE ${suffix_lib} "" dependency ${dependency})
        target_link_libraries(FFmpeg::${lib} INTERFACE ${dependency})
      endforeach()
    endif()
  else()
    message(WARNING "FFmpeg pc file ${PC_FILE} is not found")
  endif()
endfunction()

# Check for cached results. If there are skip the costly part.
#if (NOT FFMPEG_LIBRARIES)

  # Check if the required components were found and add their stuff to the FFMPEG_* vars.


  foreach (_component ${FFmpeg_FIND_COMPONENTS})
    if (${_component}_FOUND)
      string(TOLOWER ${_component} _lowerComponent)
      if (NOT TARGET FFmpeg::${_lowerComponent})
        add_library(FFmpeg::${_lowerComponent} INTERFACE IMPORTED)
        set_target_properties(FFmpeg::${_lowerComponent} PROPERTIES
            INTERFACE_COMPILE_OPTIONS "${${_component}_DEFINITIONS}"
            INTERFACE_INCLUDE_DIRECTORIES ${${_component}_INCLUDE_DIR}
            INTERFACE_LINK_LIBRARIES "${${_component}_LIBRARY_NAME}"
            INTERFACE_LINK_DIRECTORIES "${${_component}_LIBRARY_DIR}"
        )

        __ffmpeg_internal_set_dependencies(${_component})
        target_link_libraries(FFmpeg::${_lowerComponent} INTERFACE "${${_component}_LIBRARY_NAME}")
        if (UNIX AND NOT APPLE)
          target_link_options(FFmpeg::${_lowerComponent} INTERFACE  "-Wl,--exclude-libs=lib${_lowerComponent}")
        endif ()
      endif()
    endif()
  endforeach ()

  # Build the include path with duplicates removed.
  list(REMOVE_DUPLICATES FFMPEG_INCLUDE_DIRS)
  list(REMOVE_DUPLICATES FFMPEG_LIBRARY_DIRS)
  list(REMOVE_DUPLICATES FFMPEG_SHARED_LIBRARIES)
  list(REMOVE_DUPLICATES FFMPEG_STUBS)

  message(STATUS "FFmpeg shared libs: ${FFMPEG_SHARED_LIBRARIES}")
  message(STATUS "FFmpeg stubs: ${FFMPEG_STUBS}")

  # cache the vars.
  set(FFMPEG_SHARED_LIBRARIES ${FFMPEG_SHARED_LIBRARIES} CACHE STRING "The FFmpeg dynamic libraries." FORCE)
  set(FFMPEG_STUBS ${FFMPEG_STUBS} CACHE STRING "The FFmpeg stubs." FORCE)

  mark_as_advanced(FFMPEG_SHARED_LIBRARIES)
  mark_as_advanced(FFMPEG_STUBS)
# endif ()

list(LENGTH FFMPEG_LIBRARY_DIRS DIRS_COUNT)
if (${DIRS_COUNT} GREATER 1)
  message(WARNING "One ffmpeg library dir is expected, found dirs: ${FFMPEG_LIBRARY_DIRS}")
endif()

if(FFMPEG_SHARED_COMPONENTS AND FFMPEG_STATIC_COMPONENTS)
  message(WARNING
    "Only static or shared components are expected\n"
    "  static components: ${FFMPEG_STATIC_COMPONENTS}\n"
    "  shared components: ${FFMPEG_SHARED_COMPONENTS}")
endif()

if (shared_libs_desired AND NOT FFMPEG_SHARED_COMPONENTS)
  message(WARNING 
         "Shared FFmpeg libs are desired as QT_DEPLOY_FFMPEG=TRUE, but haven't been found!\n"
         "Remove QT_DEPLOY_FFMPEG or set the proper path to shared FFmpeg via FFMPEG_DIR.")
endif()

if (NOT TARGET FFmpeg::FFmpeg)
  add_library(FFmpeg INTERFACE)
  set_target_properties(FFmpeg PROPERTIES
      INTERFACE_COMPILE_OPTIONS "${FFMPEG_DEFINITIONS}"
      INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIRS}"
      INTERFACE_LINK_LIBRARIES "${FFMPEG_LIBRARIES}"
      INTERFACE_LINK_DIRECTORIES "${FFMPEG_LIBRARY_DIRS}"
  )
  add_library(FFmpeg::FFmpeg ALIAS FFmpeg)
endif()

# Compile the list of required vars
set(_FFmpeg_REQUIRED_VARS FFMPEG_LIBRARIES FFMPEG_INCLUDE_DIRS)
foreach (_component ${FFmpeg_FIND_COMPONENTS})
  list(APPEND _FFmpeg_REQUIRED_VARS ${_component}_LIBRARY ${_component}_INCLUDE_DIR)
endforeach ()

# Give a nice error message if some of the required vars are missing.
find_package_handle_standard_args(FFmpeg
    REQUIRED_VARS ${_FFmpeg_REQUIRED_VARS}
    HANDLE_COMPONENTS
)
