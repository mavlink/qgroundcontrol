# ============================================================================
# QGC Android serial port — vendored Qt SerialPort fully replaced.
#
# On Android, QGCAndroidSerialPort (QIODevice subclass) talks to the Java USB
# serial layer via QAndroidSerialEngine. On host, the same QGCAndroidSerialPort
# class is built without the JNI engine so its production logic can be tested
# via MockQSerialPortEngine (test/Comms/Android). QGCSerialPortAdapter
# (src/Comms) is the single serial-port type used by SerialLink / Bootloader /
# GPSProvider / NMEA — see its header for the full contract.
# ============================================================================

if(ANDROID)
    qt_add_library(QGCAndroidSerialPort STATIC
        iqserialportengine_p.h
        qandroidserialenginereceiver_p.h
        qandroidserialengine.cpp
        qandroidserialengine_p.h
        QGCAndroidSerialPort.cc
        QGCAndroidSerialPort.h
    )

    target_link_libraries(QGCAndroidSerialPort
        PUBLIC
            Qt6::Core
        PRIVATE
            QGCLogging
    )

    target_include_directories(QGCAndroidSerialPort
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src/Android
    )

    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCAndroidSerialPort)
else()
    # Host build: only the JNI-free portion of the backend. Tests inject a
    # MockQSerialPortEngine via QGCAndroidSerialPortFactory::setEngineFactory.
    qt_add_library(QGCAndroidSerialPort STATIC
        iqserialportengine_p.h
        qandroidserialenginereceiver_p.h
        QGCAndroidSerialPort.cc
        QGCAndroidSerialPort.h
    )

    target_link_libraries(QGCAndroidSerialPort
        PUBLIC
            Qt6::Core
        PRIVATE
            QGCLogging
    )

    target_include_directories(QGCAndroidSerialPort
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src/Android
    )

    # QGCSerialPortAdapter (src/Comms) references QGCAndroidSerialPort symbols on host
    # as well — needed for the setForceAndroidBackendForTests path that lets host tests
    # exercise the Android backend via MockQSerialPortEngine.
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCAndroidSerialPort)
endif()
