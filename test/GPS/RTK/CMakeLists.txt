# RTK tests.

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSRtkStateTest.cc GPSRtkStateTest.h)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(GPSRtkStateTest LABELS Unit RESOURCE_LOCK Settings)
