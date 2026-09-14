target_sources(${CMAKE_PROJECT_NAME} PRIVATE QGCSerialPortInfoTest.cc QGCSerialPortInfoTest.h SerialAutoConnectTest.cc
                                             SerialAutoConnectTest.h SerialPortManagerTest.cc SerialPortManagerTest.h
)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(QGCSerialPortInfoTest LABELS Unit Comms)
add_qgc_test(SerialAutoConnectTest LABELS Unit Comms)
add_qgc_test(SerialPortManagerTest LABELS Unit Comms)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE SerialWorkerTest.cc SerialWorkerTest.h)
    add_qgc_test(SerialWorkerTest LABELS Unit Comms)
endif()

target_sources(${CMAKE_PROJECT_NAME} PRIVATE SerialSettingsTest.cc SerialSettingsTest.h)
add_qgc_test(SerialSettingsTest LABELS Unit Comms)
