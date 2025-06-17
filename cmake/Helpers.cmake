function(qgc_set_qt_resource_alias)
    foreach(resource_file IN LISTS ARGN)
        get_filename_component(alias "${resource_file}" NAME)
        set_source_files_properties(
            "${resource_file}"
            PROPERTIES QT_RESOURCE_ALIAS "${alias}"
        )
    endforeach()
endfunction()
