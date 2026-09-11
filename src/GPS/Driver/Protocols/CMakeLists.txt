if(NOT TARGET QGCWire)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../Utilities/Parsing/Wire" "${CMAKE_CURRENT_BINARY_DIR}/Wire")
endif()
include(NMEA/NMEAProtocol.cmake)
include("${CMAKE_CURRENT_SOURCE_DIR}/../../../Utilities/Geo/GeographicLib.cmake")
add_subdirectory(Contracts)
add_library(QGCGPSNativeCommon STATIC GPSProtocol.cc GPSProtocol.h GPSProtocolTime.h GPSBaseProtocol.h)
set_target_properties(
    QGCGPSNativeCommon
    PROPERTIES AUTOMOC OFF
               AUTOUIC OFF
               AUTORCC OFF
)
target_include_directories(QGCGPSNativeCommon
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/../../Corrections"
                                  "${CMAKE_CURRENT_SOURCE_DIR}/../../../Utilities/Math"
)
target_link_libraries(
    QGCGPSNativeCommon
    PUBLIC QGCWire QGCGPSNativeContracts QGCGPSNMEAProtocol
    PRIVATE GeographicLib::GeographicLib
)
add_library(QGCGPSNative INTERFACE)
add_library(QGC::GPSNative ALIAS QGCGPSNative)
target_link_libraries(QGCGPSNative INTERFACE QGCGPSNativeCommon)
set(_native_targets QGCGPSNativeCommon)
foreach(family UBX Ashtech SBF Femto)
    string(TOUPPER "${family}" option_name)
    if(QGC_GPS_ENABLE_${option_name})
        add_subdirectory(${family})
        target_link_libraries(QGCGPSNative INTERFACE QGCGPS${family})
        list(APPEND _native_targets QGCGPS${family})
    endif()
endforeach()
set_property(TARGET QGCGPSNative PROPERTY QGC_GPS_NATIVE_TARGETS "${_native_targets}")
