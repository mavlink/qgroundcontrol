# ============================================================================
# Android Serial Port
# QGC's QSerialPort and QSerialPortInfo for Android: USB serial adapters, which Java drives, and serial device nodes.
# Other code includes them as <QtSerialPort/QSerialPort> and <QtSerialPort/QSerialPortInfo>, which include/ provides.
# Qt has headers of those names too, whose classes differ in size from these, so the root CMakeLists.txt puts
# include/ ahead of them for every target of an Android build, whether or not it links this library.
# include/ holds all that the app may use. The other headers are private to the library and its tests
# (test/Android/Serial).
# ============================================================================

if(NOT ANDROID)
    return()
endif()

qt_add_library(
    QGCAndroidSerialPort STATIC
    AndroidSerialAccess.cc
    AndroidSerialBackend.h
    AndroidSerialJni.cc
    AndroidSerialJni.h
    AndroidSerialPort.cc
    AndroidSerialPort.h
    AndroidSerialPortInfo.cc
    AndroidSerialPortInfo.h
    AndroidSerialRegistry.cc
    AndroidSerialSelfTest.cc
    JavaBinding.h
    PosixSerialBackend.cc
    PosixSerialPorts.cc
    UsbSerialBackend.cc
    include/AndroidSerialAccess.h
    include/AndroidSerialApp.h
)

target_link_libraries(
    QGCAndroidSerialPort
    PUBLIC Qt6::Core
    PRIVATE QGCLoggingCategory
)

target_include_directories(
    QGCAndroidSerialPort
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_SOURCE_DIR}/src/Android # AndroidInterface.h
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCAndroidSerialPort)
