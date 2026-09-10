target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSConnectionStateTest.cc GPSConnectionStateTest.h)
add_qgc_test(GPSConnectionStateTest LABELS Unit)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSSourceHealthTest.cc GPSSourceHealthTest.h)
add_qgc_test(GPSSourceHealthTest LABELS Unit)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSSatelliteStoreTest.cc GPSSatelliteStoreTest.h)
add_qgc_test(GPSSatelliteStoreTest LABELS Unit)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSScheduledTaskTest.cc GPSScheduledTaskTest.h)
add_qgc_test(GPSScheduledTaskTest LABELS Unit)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSAcceptedStateTest.cc GPSAcceptedStateTest.h)
add_qgc_test(GPSAcceptedStateTest LABELS Unit)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSConnectionControlTest.cc GPSConnectionControlTest.h)
add_qgc_test(GPSConnectionControlTest LABELS Unit)
