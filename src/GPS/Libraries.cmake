include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/Driver/Protocols/NMEA/NMEAProtocol.cmake")
if(NOT TARGET QGCLoggingCategory)
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../Utilities/Logging/Category"
                     "${CMAKE_CURRENT_BINARY_DIR}/LoggingCategory"
    )
endif()
if(NOT TARGET QGCJsonValidation)
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../Utilities/Parsing/Json" "${CMAKE_CURRENT_BINARY_DIR}/JsonValidation")
endif()
if(NOT TARGET QGCIO)
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../Utilities/IO" "${CMAKE_CURRENT_BINARY_DIR}/IO")
endif()
if(NOT TARGET QGCTiming)
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../Utilities/Timing" "${CMAKE_CURRENT_BINARY_DIR}/Timing")
endif()
if(NOT TARGET QGCNetworkIO)
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../Utilities/Network/IO" "${CMAKE_CURRENT_BINARY_DIR}/NetworkIO")
endif()
foreach(
    component
    Contracts
    Core
    Corrections
    Models
    NMEA
    NTRIP
    PositionManager
    Driver
    Recording
    Receiver
)
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/${component}" "${CMAKE_CURRENT_BINARY_DIR}/GPS/${component}")
endforeach()
include("${CMAKE_CURRENT_LIST_DIR}/LibraryBoundaries.cmake")
qgc_check_gps_library_boundaries("${CMAKE_CURRENT_SOURCE_DIR}")
