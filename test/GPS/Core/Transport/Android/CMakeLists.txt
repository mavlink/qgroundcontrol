# ============================================================================
# Android Serial Compatibility Test
# Compiles the real Android serial backend and SerialGPSTransport with test-only symbols; only JNI is simulated.
# ============================================================================

set(_android_serial "${CMAKE_SOURCE_DIR}/src/Android")
find_package(Qt6 REQUIRED COMPONENTS CorePrivate)
add_library(
    QGCAndroidSerialTestBackend OBJECT
    AndroidGPSCompatibilityTest.cc
    AndroidGPSCompatibilityTest.h
    AndroidHostPlatform.h
    ${_android_serial}/qtandroidserialport/qserialport.h
    ${_android_serial}/qtandroidserialport/qserialport.cpp
    ${_android_serial}/qtandroidserialport/qserialport_android.cpp
    ${_android_serial}/qtandroidserialport/qserialport_posix.cpp
    ${_android_serial}/qtandroidserialport/qserialportinfo.cpp
    ${_android_serial}/qtandroidserialport/qserialportinfo_android.cpp
    ${CMAKE_SOURCE_DIR}/src/GPS/Core/Transport/SerialGPSTransport.cc
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
# The host-platform prelude includes whole Qt modules, so precompile it once for the backend's sources.
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
        SerialGPSTransport=QGCAndroidTestGPSTransport
        AndroidSerial=QGCAndroidTestSerial
        AndroidSerialPortLog=QGCAndroidTestSerialPortLog
        QSerialPortInfo_AndroidLog=QGCAndroidTestSerialPortInfoLog
        SerialGPSTransportLog=QGCAndroidTestGPSTransportLog
        availablePortsByFiltersOfDevices=qgcAndroidTestAvailablePortsByFiltersOfDevices
        availablePortsBySysfs=qgcAndroidTestAvailablePortsBySysfs
        availablePortsByUdev=qgcAndroidTestAvailablePortsByUdev
)
target_include_directories(
    QGCAndroidSerialTestBackend
    PRIVATE
        ${_android_serial}
        ${_android_serial}/qtandroidserialport
        ${CMAKE_SOURCE_DIR}/src/GPS/Core/Transport
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
        ${CMAKE_SOURCE_DIR}/test/UnitTestFramework
)
target_link_libraries(
    QGCAndroidSerialTestBackend
    PRIVATE
        Qt6::Core
        Qt6::CorePrivate
        Qt6::Network
        Qt6::Test
        QGCGPSCore
        QGCLoggingCategory
)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCAndroidSerialTestBackend)
add_qgc_test(AndroidGPSCompatibilityTest LABELS Unit GPS)
