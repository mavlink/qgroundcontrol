# ============================================================================
# Android Serial Port Tests
# Compiles Android's QSerialPort (src/Android/Serial) for the host. Only the Java side and the JNI calls
# to it are simulated (AndroidSerialFake); the POSIX backend opens pseudo-terminals. The test binary also links Qt's
# own QSerialPort, so these sources and every test that uses them are compiled into this library under test-only
# names.
# ============================================================================

set(_android_serial "${CMAKE_SOURCE_DIR}/src/Android/Serial")
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
    ${_android_serial}/AndroidSerialAccess.cc
    ${_android_serial}/AndroidSerialPort.cc
    ${_android_serial}/AndroidSerialPort.h
    ${_android_serial}/AndroidSerialPortInfo.cc
    ${_android_serial}/AndroidSerialRegistry.cc
    ${_android_serial}/PosixSerialBackend.cc
    ${_android_serial}/PosixSerialPorts.cc
    ${_android_serial}/UsbSerialBackend.cc
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
        QSerialPortInfo=QGCAndroidTestSerialPortInfo
        AndroidSerial=QGCAndroidTestSerial
        AndroidSerialAccess=QGCAndroidTestSerialAccess
        AndroidSerialLog=QGCAndroidTestSerialLog
        AndroidSerialPortLog=QGCAndroidTestSerialPortLog
        AndroidSerialPortInfoLog=QGCAndroidTestSerialPortInfoLog
)
target_include_directories(
    QGCAndroidSerialTestBackend
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${_android_serial}
        ${_android_serial}/include
        ${CMAKE_SOURCE_DIR}/test/GPS/Core # "Transport/Support/PseudoTerminal.h"
        ${CMAKE_SOURCE_DIR}/test/UnitTestFramework
        ${CMAKE_SOURCE_DIR}/src/Utilities/Logging # "QGCLoggingCategoryManager.h", which switches the trace on
)
target_link_libraries(
    QGCAndroidSerialTestBackend
    PRIVATE
        Qt6::Core
        Qt6::Network
        Qt6::QmlIntegration
        Qt6::Test
        QGCLoggingCategory
)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCAndroidSerialTestBackend)
add_qgc_test(AndroidSerialPortInfoTest LABELS Unit Comms)
add_qgc_test(AndroidSerialPortPosixTest LABELS Unit Comms)
add_qgc_test(AndroidSerialPortUsbErrorTest LABELS Unit Comms)
add_qgc_test(AndroidSerialPortUsbTest LABELS Unit Comms)
