if(TARGET QGCGPSNativeContracts)
    return()
endif()
if(NOT TARGET QGCClock)
    add_subdirectory(../../../../Utilities/Timing/Clock "${CMAKE_CURRENT_BINARY_DIR}/Clock")
endif()
add_library(QGCGPSNativeContracts INTERFACE)
add_library(QGC::GPSNativeContracts ALIAS QGCGPSNativeContracts)
target_compile_features(QGCGPSNativeContracts INTERFACE cxx_std_20)
target_link_libraries(QGCGPSNativeContracts INTERFACE QGCClock)
target_include_directories(QGCGPSNativeContracts INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

foreach(family UBX ASHTECH SBF FEMTO)
    # The existing PX4 driver builds every family; selective protocol builds belong to its replacement.
    set(QGC_GPS_ENABLE_${family} ON)
endforeach()
configure_file(GPSProtocolFeatures.h.in GPSProtocolFeatures.h)
target_include_directories(QGCGPSNativeContracts INTERFACE ${CMAKE_CURRENT_BINARY_DIR})

target_sources(
    QGCGPSNativeContracts
    INTERFACE FILE_SET
              HEADERS
              FILES
              GPSBaseStationConfig.h
              GPSReceiverConfig.h
              GPSCommandTransaction.h
              GPSConstellation.h
              GPSDeadline.h
              GPSDecodedBatch.h
              GPSDriverRevision.h
              GPSExecutionContext.h
              GPSIOStatus.h
              GPSIntegrityReport.h
              GPSPositionReport.h
              GPSProtocolIO.h
              GPSReceiverSettingId.h
              GPSRelativeReport.h
              GPSSatelliteData.h
              GPSSatelliteReport.h
              GPSSurveyReport.h
)
target_sources(
    QGCGPSNativeContracts
    INTERFACE FILE_SET
              generated_headers
              TYPE
              HEADERS
              BASE_DIRS
              ${CMAKE_CURRENT_BINARY_DIR}
              FILES
              ${CMAKE_CURRENT_BINARY_DIR}/GPSProtocolFeatures.h
)
