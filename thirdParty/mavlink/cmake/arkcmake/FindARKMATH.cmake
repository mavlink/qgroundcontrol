# - Try to find  ARKMATH
# Once done, this will define
#
#  ARKMATH_FOUND - system has scicoslab 
#  ARKMATH_INCLUDE_DIRS - the scicoslab include directories
#  ARKMATH_LIBRARIES - libraries to link to

include(LibFindMacros)
include(MacroCommonPaths)

MacroCommonPaths(ARKMATH)

# Include dir
find_path(ARKMATH_INCLUDE_DIR
    NAMES arkmath/storage_adaptors.hpp
    PATHS ${COMMON_INCLUDE_PATHS_ARKMATH}
)

# data dir
find_path(ARKMATH_DATA_DIR_SEARCH
    NAMES arkmath/data/WMM.COF
    PATHS ${COMMON_DATA_PATHS_ARKMATH}
)
set(ARKMATH_DATA_DIR ${ARKMATH_DATA_DIR_SEARCH}/arkmath/data)

# the library itself
find_library(ARKMATH_LIBRARY
    NAMES arkmath
    PATHS ${COMMON_LIBRARY_PATHS_ARKMATH}
)

# the import file
find_path(ARKMATH_LIBRARY_DIR
    NAMES arkmath/arkmath-targets.cmake
    PATHS ${COMMON_LIBRARY_PATHS_ARKMATH}
)
set(ARKMATH_LIB_IMPORT ${ARKMATH_LIBRARY_DIR}/arkmath/arkmath-targets.cmake)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(ARKMATH_PROCESS_INCLUDES ARKMATH_INCLUDE_DIR)
set(ARKMATH_PROCESS_LIBS ARKMATH_LIBRARY ARKMATH_LIBRARIES)
libfind_process(ARKMATH)

macro(build_arkmath TAG EP_BASE_DIR EP_INSTALL_PREFIX EP_DATADIR)
    if(NOT ARKMATH_FOUND)
        ExternalProject_Add(arkmath
            GIT_REPOSITORY "git://github.com/arktools/arkmath.git"
            GIT_TAG ${TAG}
            UPDATE_COMMAND ""
            INSTALL_DIR ${EP_BASE_DIR}/${EP_INSTALL_PREFIX}
            CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EP_INSTALL_PREFIX}
            INSTALL_COMMAND make DESTDIR=${EP_BASE_DIR} install
           )
        set(ARKMATH_INCLUDE_DIRS ${EP_BASE_DIR}/${EP_INSTALL_PREFIX}/include)
        set(ARKMATH_DATA_DIR ${EP_DATADIR}/arkmath/data)
        # static lib prefix
        if(WIN32)
            set(STATIC_LIB_PREFIX "")
        elseif(APPLE)
            set(STATIC_LIB_PREFIX "lib")
        elseif(UNIX)
            set(STATIC_LIB_PREFIX "lib")
        else()
            message(FATAL_ERROR "unknown operating system")
        endif()
        set(ARKMATH_LIBRARIES ${EP_BASE_DIR}/${EP_INSTALL_PREFIX}/lib/${STATIC_LIB_PREFIX}arkmath.a)
        set(ARKMATH_FOUND TRUE)
    endif()
endmacro()
