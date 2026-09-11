if(TARGET QGCGPSNativeContracts)
    return()
endif()
add_library(QGCGPSNativeContracts INTERFACE)
add_library(QGC::GPSNativeContracts ALIAS QGCGPSNativeContracts)
target_compile_features(QGCGPSNativeContracts INTERFACE cxx_std_20)
target_include_directories(QGCGPSNativeContracts INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

foreach(family UBX ASHTECH SBF FEMTO)
    option(QGC_GPS_ENABLE_${family} "Build the ${family} GPS receiver protocol" ON)
endforeach()
if(NOT QGC_GPS_ENABLE_UBX
   AND NOT QGC_GPS_ENABLE_ASHTECH
   AND NOT QGC_GPS_ENABLE_SBF
   AND NOT QGC_GPS_ENABLE_FEMTO
)
    message(FATAL_ERROR "At least one GPS receiver family must be enabled")
endif()
configure_file(GPSProtocolFeatures.h.in GPSProtocolFeatures.h)
target_include_directories(QGCGPSNativeContracts INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
