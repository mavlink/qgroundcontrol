# - Try to find  ARKHANGAR
# Once done, this will define
#
#  ARKHANGAR_FOUND - system has scicoslab 
#  ARKHANGAR_INCLUDE_DIRS - the scicoslab include directories

include(LibFindMacros)
include(MacroCommonPaths)

MacroCommonPaths(ARKHANGAR)

# Include dir
find_path(ARKHANGAR_INCLUDE_DIR
    NAMES arkhangar/easystar/easystar-windtunnel.xml
    PATHS ${COMMON_DATA_PATHS_ARKHANGAR}
)

# data dir
find_path(ARKHANGAR_DATA_DIR_SEARCH
    NAMES arkhangar/easystar/easystar-windtunnel.xml
    PATHS ${COMMON_DATA_PATHS_ARKHANGAR}
)
set(ARKHANGAR_DATA_DIR ${ARKHANGAR_DATA_DIR_SEARCH}/arkhangar)

# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(ARKHANGAR_PROCESS_INCLUDES ARKHANGAR_INCLUDE_DIR)
libfind_process(ARKHANGAR)

macro(build_arkhangar TAG EP_BASE_DIR EP_INSTALL_PREFIX EP_DATADIR)
    find_package(ARKCOMM ${TAG})
    if( NOT ARKHANGAR_FOUND)
        ExternalProject_Add(arkhangar
            GIT_REPOSITORY "git://github.com/arktools/arkhangar.git"
            GIT_TAG ${TAG}
            UPDATE_COMMAND ""
            INSTALL_DIR ${EP_BASE_DIR}/${EP_INSTALL_PREFIX}
            CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EP_INSTALL_PREFIX}
            INSTALL_COMMAND make DESTDIR=${EP_BASE_DIR} install
        )
        set(ARKHANGAR_INCLUDE_DIR "")
        set(ARKHANGAR_INCLUDES_DIR "")
        set(ARKHANGAR_DATA_DIR "${EP_BASE_DIR}/${EP_INSTALL_PREFIX}/share/arkhangar")
        set(ARKHANGAR_FOUND TRUE)
    endif()
endmacro()
