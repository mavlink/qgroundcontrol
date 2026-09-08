target_sources(${CMAKE_PROJECT_NAME} PRIVATE NmeaSourceManagerTest.cc NmeaSourceManagerTest.h)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(NmeaSourceManagerTest LABELS Unit RESOURCE_LOCK Settings)
