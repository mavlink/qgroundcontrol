if(COMMAND add_qgc_test)
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSSourceHealthTest.cc GPSSourceHealthTest.h)
    add_qgc_test(GPSSourceHealthTest LABELS Unit)

    target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSSatelliteStoreTest.cc GPSSatelliteStoreTest.h)
    add_qgc_test(GPSSatelliteStoreTest LABELS Unit)
endif()

qt_add_executable(QGCGPSAcceptedStateTest GPSAcceptedStateTest.cc)
set_target_properties(QGCGPSAcceptedStateTest PROPERTIES AUTOMOC ON)
target_link_libraries(QGCGPSAcceptedStateTest PRIVATE QGC::GPSCore QGCTestTiming Qt6::Test)
add_test(NAME GPSAcceptedStateTest COMMAND QGCGPSAcceptedStateTest)
set_tests_properties(GPSAcceptedStateTest PROPERTIES LABELS "Unit;GPS" TIMEOUT 60 ENVIRONMENT "QT_FATAL_WARNINGS=1")
