include_guard(GLOBAL)

set(QGC_GPS_COMPONENT_NAMES
    NativeContracts
    NMEAProtocol
    Contracts
    Core
    Corrections
    Models
    NMEA
    NTRIPSession
    NTRIPNetwork
    Positioning
    Native
    Transport
    ReceiverTransports
    Driver
    Recording
    RecordingController
    Receiver
    Connections
)
set(QGC_GPS_COMPONENTS
    "All"
    CACHE STRING "GPS library components to configure (semicolon-separated, or All)"
)

# Expand requested components into the production target dependency closure.
function(qgc_resolve_gps_components output)
    set(dependencies_NativeContracts)
    set(dependencies_NMEAProtocol NativeContracts)
    set(dependencies_Contracts NativeContracts)
    set(dependencies_Core Contracts)
    set(dependencies_Corrections)
    set(dependencies_Models Core)
    set(dependencies_NMEA Core NMEAProtocol)
    set(dependencies_NTRIPSession)
    set(dependencies_NTRIPNetwork NTRIPSession NMEA Corrections)
    set(dependencies_Positioning Core)
    set(dependencies_Native NativeContracts NMEAProtocol)
    set(dependencies_Transport Contracts)
    set(dependencies_ReceiverTransports Transport)
    set(dependencies_Driver Native Transport Core)
    set(dependencies_Recording Transport Core)
    set(dependencies_RecordingController Recording)
    set(dependencies_Receiver Driver Core Corrections Recording ReceiverTransports)
    set(dependencies_Connections Receiver NMEA)
    set(pending ${QGC_GPS_COMPONENTS})
    if(NOT pending)
        message(FATAL_ERROR "QGC_GPS_COMPONENTS must name at least one component")
    endif()
    foreach(component IN LISTS pending)
        if(NOT component STREQUAL "All" AND NOT component IN_LIST QGC_GPS_COMPONENT_NAMES)
            message(FATAL_ERROR "Unknown GPS component '${component}'; choose ${QGC_GPS_COMPONENT_NAMES}")
        endif()
    endforeach()
    if("All" IN_LIST pending)
        set(pending ${QGC_GPS_COMPONENT_NAMES})
    endif()
    set(resolved)
    while(pending)
        list(POP_FRONT pending component)
        if(NOT component IN_LIST QGC_GPS_COMPONENT_NAMES)
            message(FATAL_ERROR "Unknown GPS component '${component}'; choose ${QGC_GPS_COMPONENT_NAMES}")
        endif()
        if(NOT component IN_LIST resolved)
            list(APPEND resolved ${component})
            list(APPEND pending ${dependencies_${component}})
        endif()
    endwhile()
    set(${output}
        ${resolved}
        PARENT_SCOPE
    )
endfunction()
