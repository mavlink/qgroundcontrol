if(QGC_NO_SERIAL_LINK)
    return()
endif()

set(QGC_SOURCES
    ${QGC_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/definitions.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSManager.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSProvider.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSProvider.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSRtk.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSRtk.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSRTKFactGroup.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSRTKFactGroup.h
    ${CMAKE_CURRENT_SOURCE_DIR}/RTCMMavlink.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/RTCMMavlink.h
    ${CMAKE_CURRENT_SOURCE_DIR}/satellite_info.h
    ${CMAKE_CURRENT_SOURCE_DIR}/sensor_gnss_relative.h
    ${CMAKE_CURRENT_SOURCE_DIR}/sensor_gps.h
    CACHE INTERNAL "" FORCE
)

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL "" FORCE
)

# qt_add_resources(${CMAKE_PROJECT_NAME} "GPSRTKFact.json"
#     PREFIX "/Vehicle"
#     FILES GPSRTKFact.json
# )

#===========================================================================#

CPMAddPackage(
    NAME px4-gpsdrivers
    GITHUB_REPOSITORY PX4/PX4-GPSDrivers
    GIT_TAG main
    SOURCE_SUBDIR src
)

file(GLOB GPS_DRIVERS_SOURCES "${px4-gpsdrivers_SOURCE_DIR}/src/*")
qt_add_library(px4-gpsdrivers STATIC ${GPS_DRIVERS_SOURCES})

target_link_libraries(px4-gpsdrivers PUBLIC Qt6::Core)

target_include_directories(px4-gpsdrivers PUBLIC ${px4-gpsdrivers_SOURCE_DIR}/src)

target_compile_definitions(px4-gpsdrivers PUBLIC GPS_DEFINITIONS_HEADER=<${CMAKE_CURRENT_SOURCE_DIR}/definitions.h>)

set(QGC_LIBRARIES
    ${QGC_LIBRARIES}
    px4-gpsdrivers
    CACHE INTERNAL "" FORCE
)
