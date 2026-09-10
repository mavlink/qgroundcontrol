add_executable(QGCNativeGPSNMEATest NMEAProtocolTest.cc)
target_link_libraries(QGCNativeGPSNMEATest PRIVATE QGCGPSNMEAProtocol)
set_target_properties(
    QGCNativeGPSNMEATest
    PROPERTIES AUTOMOC OFF
               AUTOUIC OFF
               AUTORCC OFF
)
add_test(NAME GPSProtocolNMEA COMMAND QGCNativeGPSNMEATest)
set_tests_properties(GPSProtocolNMEA PROPERTIES LABELS "Unit;GPS;NativeDriver" TIMEOUT 60)
