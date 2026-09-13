target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSPositionFactGroupTest.cc GPSPositionFactGroupTest.h)
add_qgc_test(GPSPositionFactGroupTest LABELS Unit)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSSatelliteModelTest.cc GPSSatelliteModelTest.h
                                             GPSRelativePositionModelTest.cc GPSRelativePositionModelTest.h
)
add_qgc_test(GPSSatelliteModelTest LABELS Unit)
add_qgc_test(GPSRelativePositionModelTest LABELS Unit)
