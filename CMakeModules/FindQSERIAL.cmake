# - Try to find  QSERIAL
# Once done, this will define
#
#  QSERIAL_FOUND - system has scicoslab 
#  QSERIAL_INCLUDE_DIRS - the scicoslab include directories
#  QSERIAL_LIBRARIES - libraries to link to

include(LibFindMacros)

# Include dir
find_path(QSERIAL_INCLUDE_DIR
	NAMES QSerialPort
	PATHS 
		/usr/include/QtSerialPort
		/usr/local/include/QtSerialPort
		/usr/local/qserialport/include/QtSerialPort
)

# Finally the library itself
find_library(QSERIAL_LIBRARY
	NAMES
		QtSerialPort
	PATHS 
		/usr/lib
		/usr/local/lib
		/usr/local/qserialport/lib
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(QSERIAL_PROCESS_INCLUDES QSERIAL_INCLUDE_DIR)
set(QSERIAL_PROCESS_LIBS QSERIAL_LIBRARY QSERIAL_LIBRARIES)
libfind_process(QSERIAL)
