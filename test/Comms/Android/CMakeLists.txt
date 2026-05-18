# ============================================================================
# Android Serial Engine Mock + Tests
# Host-buildable tests for the IQSerialPortEngine contract via MockQSerialPortEngine.
# The production QAndroidSerialEngine and QSerialPortPrivate are Android-only and not
# exercised here — they share the interface so the contract is validated for both.
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        MockQSerialPortEngine.cc
        MockQSerialPortEngine.h
        MockQSerialPortEngineTest.cc
        MockQSerialPortEngineTest.h
        QGCAndroidSerialPortTest.cc
        QGCAndroidSerialPortTest.h
)

# Pulls in iqserialportengine_p.h, qandroidserialenginereceiver_p.h, AndroidSerial.h.
# These headers are JNI-free and host-buildable; the matching .cpp files stay Android-only.
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/Android
    ${CMAKE_SOURCE_DIR}/src/Android/qtandroidserialport
)

# QGCAndroidSerialPort is linked from src/Android/qtandroidserialport/CMakeLists.txt
# on both host and Android — adapter.cc references its symbols.
