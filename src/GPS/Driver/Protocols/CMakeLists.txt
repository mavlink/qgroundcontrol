if(NOT TARGET Qt6::Core)
    find_package(Qt6 6.8 REQUIRED COMPONENTS Core)
endif()
if(NOT TARGET QGCLoggingCategory)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../Utilities/Logging/Category"
                     "${CMAKE_CURRENT_BINARY_DIR}/LoggingCategory"
    )
endif()
if(NOT TARGET QGCWire)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../Utilities/Parsing/Wire" "${CMAKE_CURRENT_BINARY_DIR}/Wire")
endif()
if(NOT TARGET QGCClock)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../Utilities/Timing/Clock" "${CMAKE_CURRENT_BINARY_DIR}/Clock")
endif()
block(SCOPE_FOR VARIABLES)
set(QGC_GPS_COMPONENTS_ENABLED NMEAProtocol RTCM Receiver Transport)
if(NOT TARGET QGCGPSTransport)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../Transport" "${CMAKE_CURRENT_BINARY_DIR}/Transport")
endif()
if(NOT TARGET QGCGPSNMEAProtocol)
    add_subdirectory(NMEA)
endif()
if(NOT TARGET QGCGPSRTCM)
    add_subdirectory(RTCM)
endif()
if(NOT TARGET QGCGPSReceiver)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../Receiver" "${CMAKE_CURRENT_BINARY_DIR}/Receiver")
endif()
endblock()
add_subdirectory(Contracts)
if(NOT QGC_GPS_ENABLE_UBX
   AND NOT QGC_GPS_ENABLE_ASHTECH
   AND NOT QGC_GPS_ENABLE_SBF
   AND NOT QGC_GPS_ENABLE_FEMTO
   AND NOT QGC_GPS_ENABLE_UNICORE
   AND NOT QGC_GPS_ENABLE_QUECTEL
   AND NOT QGC_GPS_ENABLE_PASSIVE
)
    message(FATAL_ERROR "At least one GPS receiver family must be enabled")
endif()
include("${CMAKE_CURRENT_SOURCE_DIR}/../../../Utilities/Geo/GeographicLib.cmake")

qt_add_library(
    QGCGPSNativeCommon
    STATIC
    GPSProtocol.cc
    GPSProtocol.h
    GPSProtocolTime.h
    GPSBaseProtocol.h
    GPSAsciiProtocol.cc
    GPSAsciiProtocol.h
)
target_sources(
    QGCGPSNativeCommon
    PUBLIC FILE_SET
           HEADERS
           FILES
           GPSProtocol.h
           GPSProtocolTime.h
           GPSBaseProtocol.h
           GPSAsciiProtocol.h
           NMEA/GPSNMEAReport.h
)
set_target_properties(
    QGCGPSNativeCommon
    PROPERTIES AUTOMOC OFF
               AUTOUIC OFF
               AUTORCC OFF
)
target_include_directories(QGCGPSNativeCommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
                                                     "${CMAKE_CURRENT_SOURCE_DIR}/../../../Utilities/Math"
)
target_link_libraries(
    QGCGPSNativeCommon
    PUBLIC QGCWire QGCClock QGCGPSNativeContracts QGCGPSNMEAProtocol QGCGPSRTCM
    PRIVATE GeographicLib::GeographicLib
)
add_library(QGCGPSNative INTERFACE)
add_library(QGC::GPSNative ALIAS QGCGPSNative)
target_link_libraries(QGCGPSNative INTERFACE QGCGPSNativeCommon)
target_sources(QGCGPSNative INTERFACE FILE_SET HEADERS FILES GPSProtocol.h GPSProtocolTime.h GPSBaseProtocol.h)
set(_native_targets QGCGPSNativeContracts QGCGPSNativeCommon)
foreach(
    family
    UBX
    Ashtech
    SBF
    Femto
    Unicore
    Quectel
    Passive
)
    string(TOUPPER "${family}" option_name)
    if(QGC_GPS_ENABLE_${option_name})
        add_subdirectory(${family})
        target_sources(QGCGPS${family} PUBLIC FILE_SET HEADERS BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}" FILES
                                              "${family}/GPSDriver${family}.h"
        )
        get_target_property(family_sources QGCGPS${family} SOURCES)
        foreach(source IN LISTS family_sources)
            if(source MATCHES "\\.h$"
               AND NOT source MATCHES "(Private|_p)\\.h$"
               AND NOT IS_ABSOLUTE "${source}"
            )
                target_sources(QGCGPS${family} PUBLIC FILE_SET HEADERS FILES "${family}/${source}")
            endif()
        endforeach()
        target_link_libraries(QGCGPSNative INTERFACE QGCGPS${family})
        target_sources(QGCGPSNative INTERFACE FILE_SET HEADERS FILES "${family}/GPSDriver${family}.h")
        list(APPEND _native_targets QGCGPS${family})
    endif()
endforeach()
set_property(TARGET QGCGPSNative PROPERTY QGC_GPS_NATIVE_TARGETS "${_native_targets}")
