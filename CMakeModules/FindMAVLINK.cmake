# - Try to find  MAVLINK
# Once done, this will define
#
#  MAVLINK_FOUND - system has scicoslab 
#  MAVLINK_INCLUDE_DIRS - the scicoslab include directories

include(LibFindMacros)

# Include dir
find_path(MAVLINK_INCLUDE_DIR
	NAMES mavlink_types.h
	PATHS 
  		/usr/include/mavlink
  		/usr/local/include/mavlink
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(MAVLINK_PROCESS_INCLUDES MAVLINK_INCLUDE_DIR)
libfind_process(MAVLINK)
