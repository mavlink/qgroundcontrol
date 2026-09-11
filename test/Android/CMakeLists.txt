if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.25)
    project(AndroidSerialWriteTests LANGUAGES CXX)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_AUTOMOC ON)
    find_package(Qt6 REQUIRED COMPONENTS Core Test)
    enable_testing()
endif()

qt_add_executable(QGCAndroidSerialWriteTest AndroidSerialWriteTest.cc
                  ../../src/Android/qtandroidserialport/AndroidSerialWrite.cc
)
target_include_directories(QGCAndroidSerialWriteTest PRIVATE ../../src/Android/qtandroidserialport)
target_link_libraries(QGCAndroidSerialWriteTest PRIVATE Qt6::Core Qt6::Test)
add_test(NAME AndroidSerialWriteTest COMMAND QGCAndroidSerialWriteTest)
set_tests_properties(AndroidSerialWriteTest PROPERTIES LABELS "Unit;Comms" TIMEOUT 30)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(Qt6 REQUIRED COMPONENTS CorePrivate)
    set(_qgc_android_serial_test_root "${CMAKE_CURRENT_SOURCE_DIR}/../../src/Android")
    set(_qgc_android_logging_test_root "${CMAKE_CURRENT_SOURCE_DIR}/../../src/Utilities/Logging/Category")
    qt_add_executable(
        QGCAndroidSerialPortWriteTest
        AndroidSerialPortWriteTest.cc
        ${_qgc_android_serial_test_root}/qtandroidserialport/AndroidSerialWrite.cc
        ${_qgc_android_serial_test_root}/qtandroidserialport/qserialport.cpp
        ${_qgc_android_serial_test_root}/qtandroidserialport/qserialport_android.cpp
        ${_qgc_android_serial_test_root}/qtandroidserialport/qserialport_posix.cpp
        ${_qgc_android_serial_test_root}/qtandroidserialport/qserialportinfo.cpp
        ${_qgc_android_serial_test_root}/qtandroidserialport/qserialportinfo_android.cpp
        ${_qgc_android_logging_test_root}/QGCLoggingCategory.cc
    )
    target_include_directories(QGCAndroidSerialPortWriteTest
                               PRIVATE ${_qgc_android_serial_test_root}/qtandroidserialport
                                       ${_qgc_android_serial_test_root} ${_qgc_android_logging_test_root}
    )
    target_link_libraries(QGCAndroidSerialPortWriteTest PRIVATE Qt6::Core Qt6::CorePrivate Qt6::Test)
    add_test(NAME AndroidSerialPortWriteTest COMMAND QGCAndroidSerialPortWriteTest)
    set_tests_properties(AndroidSerialPortWriteTest PROPERTIES LABELS "Unit;Comms" TIMEOUT 30)
endif()
