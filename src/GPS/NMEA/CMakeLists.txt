qgc_add_gps_component(QGCGPSNMEA COMPONENT NMEA LOGGING_TARGET QGCLogging)
qt_extract_metatypes(QGCGPSNMEA)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGPSNMEA)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE NMEASourceManager.cc NMEASourceManager.h)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(${CMAKE_PROJECT_NAME} PRIVATE NMEAConnectionAttempt.cc NMEAConnectionAttempt.h)
