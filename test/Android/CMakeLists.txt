# ============================================================================
# Android Serial Port Tests
# Compiles Android's QSerialPort (src/Android/qtandroidserialport) for the host. Only the Java side and the JNI calls
# to it are simulated (AndroidSerialFake); the POSIX backend opens pseudo-terminals. The test binary also links Qt's
# own QSerialPort, so these sources and every test that uses them are compiled into this library under test-only
# names.
# ============================================================================

set(_android_serial "${CMAKE_SOURCE_DIR}/src/Android/qtandroidserialport")
find_package(Qt6 REQUIRED COMPONENTS CorePrivate)
add_library(
    QGCAndroidSerialTestBackend OBJECT
    AndroidHostPlatform.h
    AndroidSerialFake.cc
    AndroidSerialFake.h
    AndroidSerialPortInfoTest.cc
    AndroidSerialPortInfoTest.h
    AndroidSerialPortPosixTest.cc
    AndroidSerialPortPosixTest.h
    AndroidSerialPortUsbErrorTest.cc
    AndroidSerialPortUsbErrorTest.h
    AndroidSerialPortUsbTest.cc
    AndroidSerialPortUsbTest.h
    ${_android_serial}/AndroidSerialRegistry.cc
    ${_android_serial}/qserialport.h
    ${_android_serial}/qserialport.cpp
    ${_android_serial}/qserialport_android.cpp
    ${_android_serial}/qserialport_posix.cpp
    ${_android_serial}/qserialport_usb.cpp
    ${_android_serial}/qserialportinfo.cpp
    ${_android_serial}/qserialportinfo_android.cpp
)
set_target_properties(
    QGCAndroidSerialTestBackend
    PROPERTIES
        AUTOMOC ON
        AUTOUIC OFF
        AUTORCC OFF
        UNITY_BUILD OFF
)
target_compile_features(QGCAndroidSerialTestBackend PRIVATE cxx_std_20)
# The host-platform prelude includes whole Qt modules, so precompile it once for the library's sources.
set(_android_host_platform "${CMAKE_CURRENT_SOURCE_DIR}/AndroidHostPlatform.h")
target_precompile_headers(QGCAndroidSerialTestBackend PRIVATE ${_android_host_platform})
target_compile_options(
    QGCAndroidSerialTestBackend PRIVATE
    "$<$<BOOL:$<TARGET_PROPERTY:DISABLE_PRECOMPILE_HEADERS>>:SHELL:-include \"${_android_host_platform}\">"
)
# AUTOMOC receives the same aliases as the compiler, including the meta-object class name.
target_compile_definitions(
    QGCAndroidSerialTestBackend
    PRIVATE
        QSerialPort=QGCAndroidTestSerialPort
        QSerialPortPrivate=QGCAndroidTestSerialPortPrivate
        QSerialPortErrorInfo=QGCAndroidTestSerialPortErrorInfo
        QSerialPortInfo=QGCAndroidTestSerialPortInfo
        QSerialPortInfoPrivate=QGCAndroidTestSerialPortInfoPrivate
        AndroidSerial=QGCAndroidTestSerial
        AndroidSerialLog=QGCAndroidTestSerialLog
        AndroidSerialPorts=QGCAndroidTestSerialPorts
        AndroidSerialPortLog=QGCAndroidTestSerialPortLog
        QSerialPortInfo_AndroidLog=QGCAndroidTestSerialPortInfoLog
)
target_include_directories(
    QGCAndroidSerialTestBackend
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${_android_serial}
        ${CMAKE_SOURCE_DIR}/test/GPS/Core # "Transport/Support/PseudoTerminal.h"
        ${CMAKE_SOURCE_DIR}/test/UnitTestFramework
)
target_link_libraries(
    QGCAndroidSerialTestBackend
    PRIVATE
        Qt6::Core
        Qt6::CorePrivate
        Qt6::Network
        Qt6::Test
        QGCLoggingCategory
)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCAndroidSerialTestBackend)
add_qgc_test(AndroidSerialPortInfoTest LABELS Unit Comms)
add_qgc_test(AndroidSerialPortPosixTest LABELS Unit Comms)
add_qgc_test(AndroidSerialPortUsbErrorTest LABELS Unit Comms)
add_qgc_test(AndroidSerialPortUsbTest LABELS Unit Comms)
