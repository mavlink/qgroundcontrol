# ============================================================================
# Qt Android Serial Port — vendored from Qt SerialPort
# ============================================================================

if(NOT ANDROID)
    return()
endif()

qt_add_library(
    QGCAndroidSerialPort STATIC
    AndroidSerial.cc
    AndroidSerial.h
    AndroidSerialPorts.h
    AndroidSerialRegistry.cc
    qserialport.cpp
    qserialport.h
    qserialport_android.cpp
    qserialport_p.h
    qserialport_posix.cpp
    qserialport_usb.cpp
    qserialportbackend_p.h
    qserialportglobal.h
    qserialportinfo.cpp
    qserialportinfo.h
    qserialportinfo_p.h
    qserialportinfo_android.cpp
    qtserialportexports.h
    qtserialportversion.h
)

target_link_libraries(
    QGCAndroidSerialPort
    PUBLIC Qt6::Core
    PRIVATE
        Qt6::CorePrivate # <QtCore/private/qiodevice_p.h>, qproperty_p.h
        QGCLoggingCategory
)

target_include_directories(
    QGCAndroidSerialPort
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${CMAKE_SOURCE_DIR}/src/Android # AndroidInterface.h
)

# TODO: Enable I/O device debugging
# target_compile_definitions(QGCAndroidSerialPort PRIVATE QIODEVICE_DEBUG)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCAndroidSerialPort)
