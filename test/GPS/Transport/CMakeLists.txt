if(COMMAND add_qgc_test)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSStreamTransportTest.cc)
    add_qgc_test(GPSStreamTransportTest LABELS Unit GPS)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE TCPGPSTransportTest.cc TCPGPSTransportTest.h)
    add_qgc_test(TCPGPSTransportTest LABELS Unit)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE UDPGPSTransportTest.cc UDPGPSTransportTest.h)
    add_qgc_test(UDPGPSTransportTest LABELS Unit)
    if(NOT QGC_NO_SERIAL_LINK)
        target_sources(${CMAKE_PROJECT_NAME} PRIVATE SerialGPSTransportTest.cc SerialGPSTransportTest.h)
        add_qgc_test(SerialGPSTransportTest LABELS Unit)
    endif()
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
        add_test(
            NAME CMake.GPSReceiverTransportsMinimal
            COMMAND
                "${CMAKE_COMMAND}" --fresh -G "${CMAKE_GENERATOR}" -S "${CMAKE_CURRENT_SOURCE_DIR}/../Standalone" -B
                "${CMAKE_CURRENT_BINARY_DIR}/GPSReceiverTransportsMinimal" "-DQt6_DIR=${Qt6_DIR}"
                -DQGC_GPS_COMPONENTS=ReceiverTransports -DQGC_NO_SERIAL_LINK=ON
                -DCMAKE_DISABLE_FIND_PACKAGE_Qt6CorePrivate=ON -DCMAKE_DISABLE_FIND_PACKAGE_Qt6Positioning=ON
                -DCMAKE_DISABLE_FIND_PACKAGE_Qt6SerialPort=ON
        )
        set_tests_properties(CMake.GPSReceiverTransportsMinimal PROPERTIES LABELS "CMake;Unit;GPS")
    endif()
else()
    qt_add_executable(GPSStreamTransportTest GPSStreamTransportTest.cc
                      "${CMAKE_CURRENT_SOURCE_DIR}/../../Portable/PortableTest.h"
    )
    set_target_properties(GPSStreamTransportTest PROPERTIES AUTOMOC ON)
    target_compile_definitions(GPSStreamTransportTest PRIVATE QGC_PORTABLE_TEST)
    target_include_directories(GPSStreamTransportTest PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../Portable")
    target_link_libraries(GPSStreamTransportTest PRIVATE QGC::GPSReceiverTransports Qt6::Test)
    add_test(NAME GPSStreamTransportTest COMMAND GPSStreamTransportTest)
    set_tests_properties(GPSStreamTransportTest PROPERTIES LABELS "Unit;GPS" TIMEOUT 30 ENVIRONMENT
                                                           "QT_FATAL_WARNINGS=1"
    )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND (COMMAND add_qgc_test OR QGC_BUILD_ANDROID_SERIAL_TESTS))
    add_subdirectory(Android)
endif()
