# Base station tests.

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSBaseStationStateTest.cc GPSBaseStationStateTest.h)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(GPSBaseStationStateTest LABELS Unit RESOURCE_LOCK Settings)
