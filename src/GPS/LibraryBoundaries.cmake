# Enforce the application boundary for every production GPS target, including private dependencies.
function(qgc_check_gps_library_boundaries directory)
    string(CONCAT forbidden "/Integration/|/Presentation/|QGroundControl|QGCSettings|QGCFactSystem|QGCVehicle|"
                  "QGCMultiVehicleManager|QGCComms|Qt6::Quick|Qt6::Qml($|[;>])|Qt6::Widgets"
    )
    get_property(
        targets
        DIRECTORY "${directory}"
        PROPERTY BUILDSYSTEM_TARGETS
    )
    foreach(library_target IN LISTS targets)
        if(NOT library_target MATCHES "^QGCGPS")
            continue()
        endif()
        foreach(property LINK_LIBRARIES INTERFACE_LINK_LIBRARIES INCLUDE_DIRECTORIES INTERFACE_INCLUDE_DIRECTORIES
                         SOURCES
        )
            get_target_property(values ${library_target} ${property})
            foreach(value IN LISTS values)
                if(value MATCHES "${forbidden}")
                    message(
                        FATAL_ERROR "${library_target} crosses the GPS library boundary: ${property} contains ${value}"
                    )
                endif()
            endforeach()
        endforeach()
    endforeach()
    get_property(
        children
        DIRECTORY "${directory}"
        PROPERTY SUBDIRECTORIES
    )
    foreach(child IN LISTS children)
        qgc_check_gps_library_boundaries("${child}")
    endforeach()
endfunction()
