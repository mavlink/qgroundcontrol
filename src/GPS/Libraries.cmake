include_guard(GLOBAL)
include("${CMAKE_CURRENT_LIST_DIR}/LibraryComponents.cmake")
qgc_resolve_gps_components(QGC_GPS_COMPONENTS_ENABLED)

set(_gps_qt_components)
foreach(component IN LISTS QGC_GPS_COMPONENTS_ENABLED)
    if(NOT component MATCHES "^(Native|NativeContracts|NMEAProtocol)$")
        list(APPEND _gps_qt_components Core)
    endif()
    if(component MATCHES "^(Core|NMEA|Positioning|NTRIPNetwork)$")
        list(APPEND _gps_qt_components Positioning)
    endif()
    if(component MATCHES "^(NTRIPSession|NTRIPNetwork|ReceiverTransports|Connections)$")
        list(APPEND _gps_qt_components Network)
    endif()
    if(component MATCHES "^(Positioning|NTRIPNetwork|Connections|RecordingController)$")
        list(APPEND _gps_qt_components QmlIntegration)
    endif()
    if(component STREQUAL "RecordingController")
        list(APPEND _gps_qt_components Concurrent)
    endif()
    if(component MATCHES "^(ReceiverTransports|Connections)$"
       AND NOT QGC_NO_SERIAL_LINK
       AND NOT ANDROID
    )
        list(APPEND _gps_qt_components SerialPort)
    endif()
endforeach()
if(_gps_qt_components)
    list(REMOVE_DUPLICATES _gps_qt_components)
    find_package(Qt6 6.8 REQUIRED COMPONENTS ${_gps_qt_components})
endif()

# Share utility targets with the embedding application when they already exist.
function(qgc_gps_add_utility target path)
    if(NOT TARGET ${target})
        add_subdirectory("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../Utilities/${path}"
                         "${CMAKE_CURRENT_BINARY_DIR}/GPSUtilities/${target}"
        )
    endif()
endfunction()
if(_gps_qt_components)
    qgc_gps_add_utility(QGCLoggingCategory Logging/Category)
endif()
if("Core" IN_LIST QGC_GPS_COMPONENTS_ENABLED OR "NTRIPNetwork" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
    qgc_gps_add_utility(QGCIO IO)
endif()
if("Core" IN_LIST QGC_GPS_COMPONENTS_ENABLED OR "NTRIPSession" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
    qgc_gps_add_utility(QGCTiming Timing)
endif()
if("Recording" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
    qgc_gps_add_utility(QGCJsonValidation Parsing/Json)
endif()
if("ReceiverTransports" IN_LIST QGC_GPS_COMPONENTS_ENABLED OR "Connections" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
    qgc_gps_add_utility(QGCNetworkIO Network/IO)
endif()
if("NativeContracts" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Driver/Protocols/Contracts" "GPS/NativeContracts")
endif()
if("NMEAProtocol" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
    include("${CMAKE_CURRENT_LIST_DIR}/Driver/Protocols/NMEA/NMEAProtocol.cmake")
endif()
foreach(
    component
    Contracts
    Core
    Corrections
    Models
    NMEA
    NTRIPSession
    Positioning
    Native
    Transport
    Driver
    Recording
    Receiver
)
    set(_component_directory ${component})
    if(component STREQUAL "NTRIPSession")
        set(_component_directory NTRIP)
    elseif(component STREQUAL "Positioning")
        set(_component_directory PositionManager)
    elseif(component STREQUAL "Native")
        set(_component_directory Driver/Protocols)
    elseif(component STREQUAL "Transport")
        set(_component_directory Driver/Transport)
    endif()
    if(component IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/${_component_directory}" "GPS/${component}")
    endif()
endforeach()
include("${CMAKE_CURRENT_LIST_DIR}/PublicHeaders.cmake")
qgc_gps_declare_public_headers()
include("${CMAKE_CURRENT_LIST_DIR}/LibraryBoundaries.cmake")
qgc_check_gps_library_boundaries("${CMAKE_CURRENT_SOURCE_DIR}")

# Resolve dependencies declared later by the application or an embedding project as well.
cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL qgc_check_gps_library_boundaries "${CMAKE_SOURCE_DIR}")
