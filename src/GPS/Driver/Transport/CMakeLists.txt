qgc_add_gps_component(QGCGPSTransport COMPONENT Transport LOGGING_TARGET QGCLogging)
set(_gps_serial_args)
if(NOT QGC_NO_SERIAL_LINK)
    if(ANDROID)
        set(_gps_serial_args SERIAL_TARGET QGCAndroidSerialPort)
    else()
        set(_gps_serial_args SERIAL_TARGET Qt6::SerialPort)
    endif()
endif()
qgc_add_gps_component(QGCGPSReceiverTransports COMPONENT ReceiverTransports LOGGING_TARGET QGCLogging
                      ${_gps_serial_args}
)
