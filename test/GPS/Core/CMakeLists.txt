target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSConnectionStateTest.cc GPSConnectionStateTest.h)
add_qgc_test(GPSConnectionStateTest LABELS Unit)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSSourceHealthTest.cc GPSSourceHealthTest.h)
add_qgc_test(GPSSourceHealthTest LABELS Unit)
