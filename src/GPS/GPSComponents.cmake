# GPS build components.
#
# The application builds every GPS component. A standalone entry point (the receiver hardware runner, the protocol
# fuzzer, the NTRIP HTTP fuzzer) sets QGC_GPS_COMPONENTS_ENABLED to the components it needs before adding GPS
# directories, and a directory whose component is absent from that list builds nothing:
#
#   NTRIP               NTRIP client and service (the HTTP library is always built)
#   Receiver            receiver sessions and connection policy
#   ReceiverTransports  serial, TCP and UDP receiver transports (the transport interface is always built)
include_guard(GLOBAL)

# Sets <out> to whether <component> is built by the current entry point.
function(qgc_gps_component_enabled component out)
    if(NOT DEFINED QGC_GPS_COMPONENTS_ENABLED OR component IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        set(${out}
            TRUE
            PARENT_SCOPE
        )
    else()
        set(${out}
            FALSE
            PARENT_SCOPE
        )
    endif()
endfunction()
