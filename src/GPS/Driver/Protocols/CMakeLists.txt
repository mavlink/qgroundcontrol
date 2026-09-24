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
include("${CMAKE_CURRENT_SOURCE_DIR}/../../../Utilities/Geo/GeographicLib.cmake")

qt_add_library(
    QGCGPSNativeCommon
    STATIC
    GPSProtocol.cc
    GPSProtocol.h
    GPSProtocolTime.h
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
target_sources(QGCGPSNative INTERFACE FILE_SET HEADERS FILES GPSProtocol.h GPSProtocolTime.h)

# Adds one receiver family protocol library; sources are relative to this directory.
function(qgc_add_gps_family family)
    qt_add_library(QGCGPS${family} STATIC ${ARGN})
    set_target_properties(
        QGCGPS${family}
        PROPERTIES AUTOMOC OFF
                   AUTOUIC OFF
                   AUTORCC OFF
    )
    target_link_libraries(QGCGPS${family} PUBLIC QGCGPSNativeCommon)
    add_library(QGC::GPS${family} ALIAS QGCGPS${family})
endfunction()

set(QGC_GPS_FAMILY_UBX_SOURCES
    UBX/GPSDriverUBX.cc
    UBX/GPSDriverUBX.h
    UBX/UBXConfiguration.cc
    UBX/UBXConfigurationValues.h
    UBX/UBXDecoder.cc
    UBX/UBXFrameDecoder.h
    UBX/UBXMessageCodec.h
    UBX/UBXMessageSchema.h
    UBX/UBXMessages.h
    UBX/UBXNavigationEpoch.h
    UBX/UBXReceiverController.h
    UBX/UBXReceiverProfile.h
)
set(QGC_GPS_FAMILY_Ashtech_SOURCES Ashtech/AshtechConfiguration.cc Ashtech/AshtechDecoder.cc Ashtech/GPSDriverAshtech.h)
set(QGC_GPS_FAMILY_SBF_SOURCES SBF/GPSDriverSBF.h SBF/SBFConfiguration.cc SBF/SBFDecoder.cc SBF/SBFMessages.h)
set(QGC_GPS_FAMILY_Femto_SOURCES Femto/FemtoConfiguration.cc Femto/FemtoDecoder.cc Femto/FemtoMessages.h
                                 Femto/GPSDriverFemto.h
)
set(QGC_GPS_FAMILY_Unicore_SOURCES Unicore/GPSDriverUnicore.cc Unicore/GPSDriverUnicore.h)
set(QGC_GPS_FAMILY_Quectel_SOURCES Quectel/GPSDriverQuectel.cc Quectel/GPSDriverQuectel.h Quectel/QuectelDecoder.cc
                                   Quectel/QuectelCodec_p.h
)
set(QGC_GPS_FAMILY_Passive_SOURCES Passive/GPSDriverPassive.cc Passive/GPSDriverPassive.h)

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
    qgc_add_gps_family(${family} ${QGC_GPS_FAMILY_${family}_SOURCES})
    target_sources(QGCGPS${family} PUBLIC FILE_SET HEADERS BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}" FILES
                                          "${family}/GPSDriver${family}.h"
    )
    get_target_property(family_sources QGCGPS${family} SOURCES)
    foreach(source IN LISTS family_sources)
        if(source MATCHES "\\.h$"
           AND NOT source MATCHES "(Private|_p)\\.h$"
           AND NOT IS_ABSOLUTE "${source}"
        )
            target_sources(QGCGPS${family} PUBLIC FILE_SET HEADERS FILES "${source}")
        endif()
    endforeach()
    target_link_libraries(QGCGPSNative INTERFACE QGCGPS${family})
    target_sources(QGCGPSNative INTERFACE FILE_SET HEADERS FILES "${family}/GPSDriver${family}.h")
    list(APPEND _native_targets QGCGPS${family})
endforeach()
set_property(TARGET QGCGPSNative PROPERTY QGC_GPS_NATIVE_TARGETS "${_native_targets}")
