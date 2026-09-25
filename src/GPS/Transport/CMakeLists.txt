qt_add_library(QGCGPSTransport STATIC GPSTransport.cc)
add_library(QGC::GPSTransport ALIAS QGCGPSTransport)
target_compile_features(QGCGPSTransport PUBLIC cxx_std_20)
target_link_libraries(
    QGCGPSTransport
    PUBLIC Qt6::Core
    PRIVATE QGCLoggingCategory
)
target_sources(QGCGPSTransport PUBLIC FILE_SET HEADERS FILES GPSTransport.h GPSTransportResult.h)

# libc++ before LLVM 20 (including Android NDK r27 and r28) keeps <stop_token> behind -fexperimental-library.
# Its stop_token is header-only there, so the flag is needed only to compile and libc++experimental is not linked.
include(CMakePushCheckState)
include(CheckCXXSourceCompiles)
set(_qgc_stop_token_probe
    [=[
#include <stop_token>
int main()
{
    std::stop_source source;
    std::stop_callback callback(source.get_token(), [] {});
    return source.request_stop() ? 0 : 1;
}
]=]
)
# Probe in C++20 even when a standalone project including this directory keeps an older default standard.
if(MSVC)
    set(_qgc_stop_token_std /std:c++20)
else()
    set(_qgc_stop_token_std -std=c++20)
endif()
cmake_push_check_state(RESET)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_REQUIRED_FLAGS ${_qgc_stop_token_std})
check_cxx_source_compiles("${_qgc_stop_token_probe}" QGC_HAS_STD_STOP_TOKEN)
if(NOT QGC_HAS_STD_STOP_TOKEN)
    set(CMAKE_REQUIRED_FLAGS "${_qgc_stop_token_std} -fexperimental-library")
    check_cxx_source_compiles("${_qgc_stop_token_probe}" QGC_HAS_EXPERIMENTAL_STOP_TOKEN)
    if(NOT QGC_HAS_EXPERIMENTAL_STOP_TOKEN)
        message(FATAL_ERROR "GPS receiver cancellation requires C++20 <stop_token>")
    endif()
    target_compile_options(QGCGPSTransport PUBLIC -fexperimental-library)
endif()
unset(CMAKE_TRY_COMPILE_TARGET_TYPE)
cmake_pop_check_state()

if(NOT DEFINED QGC_GPS_COMPONENTS_ENABLED OR "ReceiverTransports" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
    find_package(Qt6 6.8 REQUIRED COMPONENTS Network)
    qt_add_library(
        QGCGPSReceiverTransports
        STATIC
        GPSSocketWait.cc
        GPSSocketWait_p.h
        GPSStreamRead_p.h
        GPSStreamWrite_p.h
        TCPGPSTransport.cc
        UDPGPSTransport.cc
    )
    add_library(QGC::GPSReceiverTransports ALIAS QGCGPSReceiverTransports)
    target_compile_features(QGCGPSReceiverTransports PUBLIC cxx_std_20)
    target_link_libraries(
        QGCGPSReceiverTransports
        PUBLIC QGCGPSTransport Qt6::Network
        PRIVATE QGCLoggingCategory
    )
    target_sources(QGCGPSReceiverTransports PUBLIC FILE_SET HEADERS FILES TCPGPSTransport.h UDPGPSTransport.h)
    if(NOT QGC_NO_SERIAL_LINK)
        target_sources(QGCGPSReceiverTransports PRIVATE SerialGPSTransport.cc)
        target_sources(QGCGPSReceiverTransports PUBLIC FILE_SET HEADERS FILES SerialGPSTransport.h)
        if(ANDROID)
            target_link_libraries(QGCGPSReceiverTransports PRIVATE QGCAndroidSerialPort)
        else()
            target_link_libraries(QGCGPSReceiverTransports PRIVATE Qt6::SerialPort)
        endif()
    else()
        target_compile_definitions(QGCGPSReceiverTransports PUBLIC QGC_NO_SERIAL_LINK)
    endif()
endif()
