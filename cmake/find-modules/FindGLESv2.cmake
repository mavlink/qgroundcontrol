# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

include(CheckCXXSourceCompiles)

# No library linkage is necessary to use GLESv2 with Emscripten. The headers are also
# system headers, so we don't need to search for them.
if(EMSCRIPTEN)
    set(HAVE_GLESv2 ON)
else()
    find_library(GLESv2_LIBRARY NAMES GLESv2 OpenGLES)
    find_path(GLESv2_INCLUDE_DIR NAMES "GLES2/gl2.h" "OpenGLES/ES2/gl.h" DOC "The OpenGLES 2 include path")
    find_package(EGL)
    set(_libraries "${CMAKE_REQUIRED_LIBRARIES}")
    if(GLESv2_LIBRARY)
        list(APPEND CMAKE_REQUIRED_LIBRARIES "${GLESv2_LIBRARY}")
    endif ()
    if(EGL_LIBRARY)
        list(APPEND CMAKE_REQUIRED_LIBRARIES "${EGL_LIBRARY}")
    endif()
    if(_qt_igy_gui_libs)
        list(APPEND CMAKE_REQUIRED_LIBRARIES "${_qt_igy_gui_libs}")
    endif()
    set(_includes "${CMAKE_REQUIRED_INCLUDES}")
    list(APPEND CMAKE_REQUIRED_INCLUDES "${GLESv2_INCLUDE_DIR}")

    check_cxx_source_compiles("
#ifdef __APPLE__
#  include <OpenGLES/ES2/gl.h>
#else
#  define GL_GLEXT_PROTOTYPES
#  include <GLES2/gl2.h>
#endif

int main(int, char **) {
    glUniform1f(1, GLfloat(1.0));
    glClear(GL_COLOR_BUFFER_BIT);
}" HAVE_GLESv2)

    set(CMAKE_REQUIRED_LIBRARY "${_libraries}")
    unset(_libraries)
    set(CMAKE_REQUIRED_INCLUDES "${_includes}")
    unset(_includes)
    set(package_args GLESv2_INCLUDE_DIR GLESv2_LIBRARY HAVE_GLESv2)
endif()

# Framework handling partially inspired by FindGLUT.cmake.
if(GLESv2_LIBRARY MATCHES "/([^/]+)\\.framework$")
    # TODO: Might need to handle non .tbd suffixes, but didn't find an
    # example like that.
    # TODO: Might need to handle INTERFACE_INCLUDE_DIRECTORIES differently.
    set(_library_imported_location "${GLESv2_LIBRARY}/${CMAKE_MATCH_1}.tbd")
    if(NOT EXISTS "${_library_imported_location}")
        set(_library_imported_location "")
    endif()
else()
    set(_library_imported_location "${GLESv2_LIBRARY}")
endif()
set(GLESv2_LIBRARY "${_library_imported_location}")

list(APPEND package_args HAVE_GLESv2)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLESv2 DEFAULT_MSG ${package_args})

mark_as_advanced(${package_args})

if(GLESv2_FOUND AND NOT TARGET GLESv2::GLESv2)
    if(EMSCRIPTEN OR IOS)
        add_library(GLESv2::GLESv2 INTERFACE IMPORTED)
        if(IOS)
            # For simulator_and_device builds we can't specify the full library path, because
            # it's specific to either the device or the simulator. Resort to passing a link
            # flag instead.
            target_link_libraries(GLESv2::GLESv2 INTERFACE "-framework OpenGLES")
        endif()
    else()
        add_library(GLESv2::GLESv2 UNKNOWN IMPORTED)
        set_target_properties(GLESv2::GLESv2 PROPERTIES
            IMPORTED_LOCATION "${GLESv2_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${GLESv2_INCLUDE_DIR}")

        if(EGL_LIBRARY)
            target_link_libraries(GLESv2::GLESv2 INTERFACE "${EGL_LIBRARY}")
        endif()
    endif()
endif()
