# - Try to find  JSBSIM
# Once done, this will define
#
#  JSBSIM_FOUND - system has scicoslab 
#  JSBSIM_INCLUDE_DIRS - the scicoslab include directories
#  JSBSIM_LIBRARIES - libraries to link to

include(LibFindMacros)
include(MacroCommonPaths)

MacroCommonPaths(JSBSIM)

# Include dir
find_path(JSBSIM_INCLUDE_DIR
    NAMES JSBSim/initialization/FGTrimmer.h
    PATHS ${COMMON_INCLUDE_PATHS_JSBSIM}
)

# data dir
find_path(JSBSIM_DATA_DIR_SEARCH
    NAMES jsbsim/aircraft/aircraft_template.xml
    PATHS ${COMMON_DATA_PATHS_JSBSIM}
)
set(JSBSIM_DATA_DIR ${JSBSIM_DATA_DIR_SEARCH}/jsbsim)

# Finally the library itself
find_library(JSBSIM_LIBRARY
    NAMES JSBSim
    PATHS ${COMMON_LIBRARY_PATHS_JSBSIM}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(JSBSIM_PROCESS_INCLUDES JSBSIM_INCLUDE_DIR)
set(JSBSIM_PROCESS_LIBS JSBSIM_LIBRARY JSBSIM_LIBRARIES)
set(JSBSIM_INCLUDE_DIR ${JSBSIM_INCLUDE_DIR} ${JSBSIM_INCLUDE_DIR}/JSBSim)
set(JSBSIM_INCLUDES ${JSBSIM_INCLUDES} ${JSBSIM_INCLUDE_DIR}/JSBSim)

libfind_process(JSBSIM)

macro(build_jsbsim TAG EP_BASE_DIR EP_INSTALL_PREFIX EP_DATADIR)
    if(NOT JSBSIM_FOUND)
        ExternalProject_Add(jsbsim
            GIT_REPOSITORY "git://github.com/jgoppert/jsbsim.git"
            GIT_TAG ${TAG}
            UPDATE_COMMAND ""
            INSTALL_DIR ${EP_BASE_DIR}/${EP_INSTALL_PREFIX}
            CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EP_INSTALL_PREFIX}
            INSTALL_COMMAND make DESTDIR=${EP_BASE_DIR} install
            )
        set(JSBSIM_INCLUDE_DIRS  ${EP_BASE_DIR}/${EP_INSTALL_PREFIX}/include ${EP_BASE_DIR}/${EP_INSTALL_PREFIX}/include/jsbsim)
        set(JSBSIM_DATA_DIR ${EP_DATADIR}/jsbsim)
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

        set(JSBSIM_LIBRARIES ${EP_BASE_DIR}/${EP_INSTALL_PREFIX}/lib/${STATIC_LIB_PREFIX}jsbsim.a)
        set(JSBSIM_FOUND TRUE)
    endif()
endmacro()
