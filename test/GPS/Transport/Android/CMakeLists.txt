# Compile the real Android backend with test-only symbols; only JNI is simulated.
set(_android_serial "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/Android")
find_package(Qt6 REQUIRED COMPONENTS CorePrivate)
add_library(
    QGCAndroidSerialTestBackend OBJECT
    AndroidGPSCompatibilityTest.cc
    AndroidHostPlatform.h
    ${_android_serial}/qtandroidserialport/qserialport.h
    ${_android_serial}/qtandroidserialport/qserialport.cpp
    ${_android_serial}/qtandroidserialport/qserialport_android.cpp
    ${_android_serial}/qtandroidserialport/qserialport_posix.cpp
    ${_android_serial}/qtandroidserialport/qserialportinfo.cpp
    ${_android_serial}/qtandroidserialport/qserialportinfo_android.cpp
    ${_android_serial}/../GPS/Transport/SerialGPSTransport.cc
)
set_target_properties(
    QGCAndroidSerialTestBackend
    PROPERTIES AUTOMOC ON
               AUTOUIC OFF
               AUTORCC OFF
               DISABLE_PRECOMPILE_HEADERS ON
               UNITY_BUILD OFF
)
target_compile_features(QGCAndroidSerialTestBackend PRIVATE cxx_std_20)
target_compile_options(QGCAndroidSerialTestBackend PRIVATE -include ${CMAKE_CURRENT_SOURCE_DIR}/AndroidHostPlatform.h)
# AUTOMOC receives the same aliases as the compiler, including the meta-object class name.
target_compile_definitions(
    QGCAndroidSerialTestBackend
    PRIVATE QSerialPort=QGCAndroidTestSerialPort
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
    PRIVATE ${_android_serial} ${_android_serial}/qtandroidserialport ${_android_serial}/../GPS/Transport
            ${CMAKE_CURRENT_SOURCE_DIR}/../../../UnitTestFramework
)
target_link_libraries(QGCAndroidSerialTestBackend PRIVATE Qt6::Core Qt6::CorePrivate Qt6::Network Qt6::Test
                                                          QGCGPSTransport QGCLoggingCategory
)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCAndroidSerialTestBackend)
add_qgc_test(AndroidGPSCompatibilityTest LABELS Unit GPS)
