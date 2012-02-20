# - Try to find  OPENSCENEGRAPH
# Once done, this will define
#
#  OPENSCENEGRAPH_FOUND - system has scicoslab 
#  OPENSCENEGRAPH_INCLUDE_DIRS - the scicoslab include directories
#  OPENSCENEGRAPH_LIBRARIES - libraries to link to

include(LibFindMacros)
include(MacroCommonPaths)

MacroCommonPaths(OPENSCENEGRAPH)

macro(build_openscenegraph TAG EP_BASE_DIR EP_INSTALL_PREFIX EP_DATADIR)
    ExternalProject_Add(openscenegraph
        SVN_REPOSITORY "http://www.openscenegraph.org/svn/osg/OpenSceneGraph/tags/OpenSceneGraph-${TAG}"
        UPDATE_COMMAND ""
        INSTALL_DIR ${EP_BASE_DIR}/${EP_INSTALL_PREFIX}
        CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${EP_INSTALL_PREFIX}
        INSTALL_COMMAND make DESTDIR=${EP_BASE_DIR} install
       )
    set(OPENSCENEGRAPH_INCLUDE_DIRS ${EP_BASE_DIR}/${EP_INSTALL_PREFIX}/include)
    set(OPENSCENEGRAPH_DATA_DIR ${EP_DATADIR}/openscenegraph/data)
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
    set(OPENSCENEGRAPH_LIBRARIES 
        )
    set(OPENSCENEGRAPH_FOUND TRUE)
endmacro()
