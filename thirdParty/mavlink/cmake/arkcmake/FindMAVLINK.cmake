# - Try to find  MAVLINK
# Once done, this will define
#
#  MAVLINK_FOUND - system has scicoslab 
#  MAVLINK_INCLUDE_DIRS - the scicoslab include directories

include(LibFindMacros)
include(MacroCommonPaths)

MacroCommonPaths(MAVLINK)

# Include dir
find_path(MAVLINK_INCLUDE_DIR
    NAMES mavlink/mavlink_types.h
    PATHS ${COMMON_INCLUDE_PATHS_MAVLINK}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(MAVLINK_PROCESS_INCLUDES MAVLINK_INCLUDE_DIR)
libfind_process(MAVLINK)

macro(build_mavlink TAG EP_BASE_DIR EP_INSTALL_PREFIX EP_DATADIR)
    if( NOT MAVLINK_FOUND)
        ExternalProject_Add(mavlink
            GIT_REPOSITORY "git://github.com/pixhawk/mavlink.git"
            GIT_TAG ${TAG}
            UPDATE_COMMAND ""
            INSTALL_DIR ${EP_BASE_DIR}/${EP_INSTALL_PREFIX}
            CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EP_INSTALL_PREFIX}
            INSTALL_COMMAND make DESTDIR=${EP_BASE_DIR} install
        )
        set(MAVLINK_INCLUDE_DIRS ${EP_INCLUDEDIR})
        set(MAVLINK_DATA_DIR "")
        set(MAVLINK_LIBRARIES "")
        set(MAVLINK_FOUND TRUE)
    endif()
endmacro()
