# - Try to find  QSERIALPORT
# Once done, this will define
#
#  QSERIALPORT_FOUND - system has scicoslab 
#  QSERIALPORT_INCLUDE_DIRS - the scicoslab include directories
#  QSERIALPORT_LIBRARIES - libraries to link to

include(LibFindMacros)

# Include dir
find_path(QSERIALPORT_INCLUDE_DIR
	NAMES QSerialPort
	PATHS 
		/usr/include/QtSerialPort
		/usr/local/include/QtSerialPort
		/usr/local/qserialport/include/QtSerialPort
)

# Finally the library itself
find_library(QSERIALPORT_LIBRARY
	NAMES
		QtSerialPort
	PATHS 
		/usr/lib
		/usr/local/lib
		/usr/local/qserialport/lib
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(QSERIALPORT_PROCESS_INCLUDES QSERIALPORT_INCLUDE_DIR)
set(QSERIALPORT_PROCESS_LIBS QSERIALPORT_LIBRARY QSERIALPORT_LIBRARIES)
libfind_process(QSERIALPORT)
