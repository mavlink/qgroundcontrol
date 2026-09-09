target_sources(${CMAKE_PROJECT_NAME}
               PRIVATE GPSProviderTest.cc GPSProviderTest.h GPSReceiverPositionSourceTest.cc
                       GPSReceiverPositionSourceTest.h GPSReceiverSessionTest.cc GPSReceiverSessionTest.h
)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_qgc_test(GPSProviderTest LABELS Unit)
add_qgc_test(GPSReceiverPositionSourceTest LABELS Unit)
add_qgc_test(GPSReceiverSessionTest LABELS Unit)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSReceiverAutoConnectTest.cc GPSReceiverAutoConnectTest.h)
add_qgc_test(GPSReceiverAutoConnectTest LABELS Unit RESOURCE_LOCK Settings)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSReceiverTest.cc GPSReceiverTest.h)
add_qgc_test(GPSReceiverTest LABELS Unit RESOURCE_LOCK Settings)

qt_add_executable(QGCGPSReceiverContractTest GPSReceiverContractTest.cc)
target_include_directories(QGCGPSReceiverContractTest PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(QGCGPSReceiverContractTest PRIVATE QGCGPSReceiver QGCGPSCorrections Qt6::Test)
add_test(NAME GPSReceiverContractTest COMMAND QGCGPSReceiverContractTest)
set_tests_properties(GPSReceiverContractTest PROPERTIES LABELS "Unit;GPS" TIMEOUT 60)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSReceiverIntegrationTest.cc GPSReceiverIntegrationTest.h)
add_qgc_test(GPSReceiverIntegrationTest LABELS Unit)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSReceiverTestProfile.h)
