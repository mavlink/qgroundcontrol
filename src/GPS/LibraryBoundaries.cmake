include_guard(GLOBAL)

# Find production GPS roots before following their complete dependency graph.
function(qgc_collect_gps_targets directory output)
    get_property(
        targets
        DIRECTORY "${directory}"
        PROPERTY BUILDSYSTEM_TARGETS
    )
    list(FILTER targets INCLUDE REGEX "^QGCGPS")
    get_property(
        children
        DIRECTORY "${directory}"
        PROPERTY SUBDIRECTORIES
    )
    foreach(child IN LISTS children)
        qgc_collect_gps_targets("${child}" child_targets)
        list(APPEND targets ${child_targets})
    endforeach()
    set(${output}
        ${targets}
        PARENT_SCOPE
    )
endfunction()

# Walk actual targets, including utility dependencies and aliases, instead of trusting target names.
function(qgc_check_gps_library_boundaries directory)
    set(forbidden_path "/Integration($|[/;>])|/Presentation($|[/;>])")
    string(CONCAT forbidden_target "^(QGroundControl$|QGCSettings|QGCFactSystem|QGCVehicle|"
                  "QGCMultiVehicleManager|QGCComms|Qt6::Quick|Qt6::Qml$|Qt6::Widgets)"
    )
    qgc_collect_gps_targets("${directory}" pending)
    set(visited)
    while(pending)
        list(POP_FRONT pending library_target)
        if(library_target IN_LIST visited OR NOT TARGET ${library_target})
            continue()
        endif()
        list(APPEND visited ${library_target})
        get_target_property(aliased ${library_target} ALIASED_TARGET)
        if(aliased)
            list(APPEND pending ${aliased})
        endif()
        foreach(property LINK_LIBRARIES INTERFACE_LINK_LIBRARIES INCLUDE_DIRECTORIES INTERFACE_INCLUDE_DIRECTORIES
                         SOURCES
        )
            get_target_property(values ${library_target} ${property})
            foreach(value IN LISTS values)
                if(value MATCHES "${forbidden_path}")
                    message(
                        FATAL_ERROR "${library_target} crosses the GPS library boundary: ${property} contains ${value}"
                    )
                endif()
                if(property MATCHES "LINK_LIBRARIES$")
                    # Link generator expressions may wrap a target; inspect each referenced target conservatively.
                    string(REGEX MATCHALL "[A-Za-z_][A-Za-z0-9_.+-]*(::[A-Za-z_][A-Za-z0-9_.+-]*)*" references
                                 "${value}"
                    )
                    foreach(reference IN LISTS references)
                        if(reference MATCHES "${forbidden_target}")
                            message(
                                FATAL_ERROR "${library_target} crosses the GPS library boundary through ${reference}"
                            )
                        endif()
                        if(TARGET ${reference})
                            list(APPEND pending ${reference})
                        endif()
                    endforeach()
                endif()
            endforeach()
        endforeach()
    endwhile()
endfunction()
