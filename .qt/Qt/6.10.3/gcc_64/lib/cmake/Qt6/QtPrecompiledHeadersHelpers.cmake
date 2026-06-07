# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

function(qt_update_precompiled_header target precompiled_header)
    if (precompiled_header AND BUILD_WITH_PCH)
        set_property(TARGET "${target}" APPEND PROPERTY "PRECOMPILE_HEADERS" "$<$<COMPILE_LANGUAGE:CXX,OBJCXX>:${precompiled_header}>")
    endif()
endfunction()

function(qt_update_precompiled_header_with_library target library)
    if(NOT TARGET "${library}")
        return()
    endif()

    get_target_property(target_type "${library}" TYPE)
    if(target_type STREQUAL "INTERFACE_LIBRARY")
        return()
    endif()

    get_target_property(header_file_path "${library}" MODULE_HEADER)
    if(NOT header_file_path)
        # The Qt module is imported from the Qt installation prefix.
        # Calculate the module header location.
        get_target_property(include_name "${library}" _qt_module_include_name)
        if(NOT include_name)
            message(DEBUG "Property _qt_module_include_name is not set on ${library}.")
            return()
        endif()

        get_target_property(relative_include_dir "${library}" _qt_module_relative_include_dir)
        if(NOT relative_include_dir)
            message(DEBUG "Property _qt_module_relative_include_dir is not set on ${library}.")
            return()
        endif()

        get_target_property(package_name ${library} _qt_package_name)
        if(NOT package_name)
            message(DEBUG "No package name found for ${library}.")
            return()
        endif()
        set(package_location "${${package_name}_DIR}")
        get_filename_component(absolute_include_dir
            "${package_location}/${relative_include_dir}"
            ABSOLUTE
        )

        set(header_file_path "${absolute_include_dir}/${include_name}")
        if(NOT EXISTS "${header_file_path}")
            message(DEBUG "Module header '${header_file_path}' does not exist.")
            return()
        endif()
    endif()

    qt_update_precompiled_header("${target}" "${header_file_path}")
endfunction()

function(qt_update_ignore_pch_source target sources)
    if (sources)
        set_source_files_properties(${sources} PROPERTIES
                                               SKIP_PRECOMPILE_HEADERS ON
                                               SKIP_UNITY_BUILD_INCLUSION ON)
    endif()
endfunction()

function(qt_ignore_pch_obj_c_sources target sources)
    # No obj-cxx PCH support for versions lower than 3.16.
    if(CMAKE_VERSION VERSION_LESS 3.16.0)
        list(FILTER sources INCLUDE REGEX "\\.mm$")
        qt_update_ignore_pch_source("${target}" "${sources}")
    endif()
endfunction()
