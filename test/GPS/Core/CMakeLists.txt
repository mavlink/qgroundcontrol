qt_add_executable(QGCGPSCoreTest GPSCoreTest.cc)
target_link_libraries(QGCGPSCoreTest PRIVATE QGCGPSCore Qt6::Test)
add_test(NAME GPSCoreTest COMMAND QGCGPSCoreTest)
set_tests_properties(GPSCoreTest PROPERTIES LABELS "Unit;GPS" TIMEOUT 60)
