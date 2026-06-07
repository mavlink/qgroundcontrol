# Copyright 2005-2011 Kitware, Inc.
# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

######################################
#
#       Macros for building Qt files
#
######################################


# qt6_wrap_ui(outfiles inputfile ... )

function(qt6_wrap_ui outfiles )
    set(options)
    set(oneValueArgs)
    set(multiValueArgs OPTIONS)

    cmake_parse_arguments(_WRAP_UI "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(ui_files ${_WRAP_UI_UNPARSED_ARGUMENTS})
    set(ui_options ${_WRAP_UI_OPTIONS})

    foreach(it ${ui_files})
        get_filename_component(outfile ${it} NAME_WE)
        get_filename_component(infile ${it} ABSOLUTE)
        set(outfile ${CMAKE_CURRENT_BINARY_DIR}/ui_${outfile}.h)
        add_custom_command(OUTPUT ${outfile}
          DEPENDS ${QT_CMAKE_EXPORT_NAMESPACE}::uic
          COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::uic
          ARGS ${ui_options} -o ${outfile} ${infile}
          MAIN_DEPENDENCY ${infile} VERBATIM)
        set_source_files_properties(${infile} PROPERTIES SKIP_AUTOUIC ON)
        _qt_internal_set_source_file_generated(
            SOURCES ${outfile}
            SKIP_AUTOGEN
        )
        list(APPEND ${outfiles} ${outfile})
    endforeach()
    set(${outfiles} ${${outfiles}} PARENT_SCOPE)
endfunction()

# This will override the CMake upstream command, because that one is for Qt 3.
if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_wrap_ui outfiles)
        if(QT_DEFAULT_MAJOR_VERSION EQUAL 5)
            qt5_wrap_ui("${outfiles}" ${ARGN})
        elseif(QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_wrap_ui("${outfiles}" ${ARGN})
        endif()
        set("${outfiles}" "${${outfiles}}" PARENT_SCOPE)
    endfunction()
endif()

function(qt6_add_ui target)
    if(NOT TARGET ${target})
        message(FATAL_ERROR "Target \"${target}\" does not exist.")
    endif()

    set(options)
    set(oneValueArgs INCLUDE_PREFIX)
    set(multiValueArgs SOURCES OPTIONS)

    cmake_parse_arguments(arg "${options}" "${oneValueArgs}" "${multiValueArgs}"
                         ${ARGN})

    set(sources ${arg_SOURCES})
    set(ui_options ${arg_OPTIONS})
    set(raw_include_prefix "${arg_INCLUDE_PREFIX}")

    if("${sources}" STREQUAL "")
        message(FATAL_ERROR "The \"SOURCES\" parameter is empty.")
    endif()


    # Disable AUTOUIC for the target explicitly to avoid the AUTOUIC
    # error message when the AUTOUIC is enabled for the target.
    set_target_properties(${target} PROPERTIES AUTOUIC OFF)

    if(NOT "${raw_include_prefix}" STREQUAL "")
        # generate dummy `_` folders from the given path
        # e.g. if the given path is `../a/b/c`, then the generated path will be
        # `_`. if the given path is ``../a/../b/c`, then the generated path will
        # be `_/_`
        function(_qt_internal_generate_dash_path_from_input input_path
                 out_generated_dash_path)
            string(REGEX MATCHALL "\\.\\." upper_folder_list "${input_path}")

            list(JOIN upper_folder_list "" upper_folder_list)
            string(REGEX REPLACE "\\.\\." "_/" additional_path
                "${upper_folder_list}")

            # remove last / character
            if(NOT "${additional_path}" STREQUAL "")
                string(LENGTH "${additional_path}" additional_path_length)
                math(EXPR additional_path_length "${additional_path_length} - 1")
                string(SUBSTRING "${additional_path}" 0 ${additional_path_length}
                    additional_path)
            endif()

            set(${out_generated_dash_path} "${additional_path}" PARENT_SCOPE)
        endfunction()
        # Note: If previous folders are less than ../ folder count in
        # ${raw_include_prefix}, relative path calculation will miscalculate,
        # so we need to add dummy folders to calculate the relative path
        # correctly
        _qt_internal_generate_dash_path_from_input("${raw_include_prefix}"
            dummy_path_for_relative_calculation)
        if(NOT "${dummy_path_for_relative_calculation}" STREQUAL "")
            set(dummy_path_for_relative_calculation
                "/${dummy_path_for_relative_calculation}")
            set(raw_include_prefix_to_compare
                "${dummy_path_for_relative_calculation}/${raw_include_prefix}")
        else()
            set(dummy_path_for_relative_calculation "/")
            set(raw_include_prefix_to_compare "/${raw_include_prefix}")
        endif()
        # Note: This relative path calculation could be done just by using the
        # below code
        # cmake_path(RELATIVE_PATH normalized_include_prefix "${CMAKE_CURRENT_SOURCE_DIR}"
        # but due to the backward compatibility, we need to use
        # file(RELATIVE_PATH) and ../ folder calculation the with
        # _qt_internal_generate_dash_path_from_input() function
        if(WIN32)
            set(dummy_path_for_relative_calculation
                "${CMAKE_CURRENT_SOURCE_DIR}/${dummy_path_for_relative_calculation}")
            set(raw_include_prefix_to_compare
                "${CMAKE_CURRENT_SOURCE_DIR}/${raw_include_prefix_to_compare}")
            string(REGEX REPLACE "//" "/" dummy_path_for_relative_calculation
                "${dummy_path_for_relative_calculation}")
            string(REGEX REPLACE "//" "/" raw_include_prefix_to_compare
                "${raw_include_prefix_to_compare}")
        endif()

        _qt_internal_relative_path(raw_include_prefix_to_compare
            BASE_DIRECTORY "${dummy_path_for_relative_calculation}"
            OUTPUT_VARIABLE normalized_include_prefix
        )

        _qt_internal_generate_dash_path_from_input("${normalized_include_prefix}"
            additional_path)
    endif()

    if(ui_options MATCHES "\\$<CONFIG")
        set(is_ui_options_contains_config true)
    endif()
    get_property(is_multi GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if(is_multi AND is_ui_options_contains_config)
        set(include_folder "$<CONFIG>")
    endif()

    set(include_folder_to_add "${include_folder}")
    if(NOT "${additional_path}" STREQUAL "")
        set(include_folder_to_add "${include_folder_to_add}/${additional_path}")
    endif()

    set(prefix_info_file_cmake
        "${CMAKE_CURRENT_BINARY_DIR}/${target}_autogen/prefix_info.cmake")
    if(EXISTS ${prefix_info_file_cmake})
        include(${prefix_info_file_cmake})
        set(prefix_info_file_cmake_exists true)
    else()
        set(prefix_info_file_cmake_exists false)
    endif()

    target_sources(${target} PRIVATE ${sources})

    foreach(source_file ${sources})
        get_filename_component(outfile "${source_file}" NAME_WE)
        get_filename_component(infile "${source_file}" ABSOLUTE)
        string(SHA1 infile_hash "${target}${infile}")
        string(SUBSTRING "${infile_hash}" 0 6 short_hash)
        set(file_ui_folder "${CMAKE_CURRENT_BINARY_DIR}/.qt/${short_hash}")
        target_include_directories(${target} SYSTEM PRIVATE
                    "${file_ui_folder}/${include_folder_to_add}")

        set(target_include_folder_with_add_path
            "${file_ui_folder}/${include_folder}")
        # Add additional path to include folder if it is not empty
        if(NOT "${additional_path}" STREQUAL "")
            set(target_include_folder_with_add_path
                "${target_include_folder_with_add_path}/${additional_path}")
        endif()

        set(inc_dir_to_create "${target_include_folder_with_add_path}")
        if(NOT "${raw_include_prefix}" STREQUAL "")
            set(inc_dir_to_create
                "${target_include_folder_with_add_path}/${raw_include_prefix}")
        endif()

        set(output_directory "${target_include_folder_with_add_path}")
        if(NOT "${normalized_include_prefix}" STREQUAL "")
            set(output_directory
                "${output_directory}/${normalized_include_prefix}")
        endif()

        if(NOT EXISTS "${infile}")
            message(FATAL_ERROR "${infile} does not exist.")
        endif()
        set_source_files_properties(${infile} PROPERTIES SKIP_AUTOUIC ON)

        set(outfile "${output_directory}/ui_${outfile}.h")
        # Before CMake 3.27, there was a bug when using $<CONFIG> in a generated
        # file with Ninja Multi-Config generator. To avoid this issue, we need
        # to add the generated file for each configuration.
        # Explicitly set the output file properties for each configuration
        # to avoid Policy CMP0071 warnings.
        # See https://gitlab.kitware.com/cmake/cmake/-/issues/24848
        # Xcode generator cannot handle $<CONFIG> in the output file name.
        # it changes $<CONFIG> with NOCONFIG. That's why we need to add the
        # generated file for each configuration for Xcode generator as well.
        if(is_multi AND outfile MATCHES "\\$<CONFIG>"
            AND CMAKE_VERSION VERSION_LESS "3.27"
            OR CMAKE_GENERATOR MATCHES "Xcode")
            foreach(config ${CMAKE_CONFIGURATION_TYPES})
                string(REPLACE "$<CONFIG>" "${config}" outfile_with_config
                    "${outfile}")
                _qt_internal_set_source_file_generated(
                    SOURCES ${outfile_with_config}
                    SKIP_AUTOGEN
                )
                target_sources(${target} PRIVATE ${outfile_with_config})
            endforeach()
        else()
            _qt_internal_set_source_file_generated(
                SOURCES ${outfile}
                SKIP_AUTOGEN
            )
            target_sources(${target} PRIVATE ${outfile})
        endif()
        # remove double slashes
        string(REGEX REPLACE "//" "/" outfile "${outfile}")

        # Note: If INCLUDE_PREFIX is changed without changing the corresponding
        # include path in the source file and the Ninja generator is used, this
        # casues the double build issue. To avoid this issue, we need to
        # remove the output file in configure step if the include_prefix is
        # changed.
        if(CMAKE_GENERATOR MATCHES "Ninja")
            if(prefix_info_file_cmake_exists)
                if(NOT "${${short_hash}_prefix}" STREQUAL
                    "${normalized_include_prefix}")
                    file(REMOVE_RECURSE "${file_ui_folder}")
                    list(APPEND prefix_info_list
                        "set(${short_hash}_prefix \"${normalized_include_prefix}\")")
                elseif(NOT DEFINED ${short_hash}_prefix)
                    list(APPEND prefix_info_list
                        "set(${short_hash}_prefix \"${normalized_include_prefix}\")")
                endif()
            else()
                set(prefix_info_list "")
                list(APPEND prefix_info_list
                    "set(${short_hash}_prefix \"${normalized_include_prefix}\")")
            endif()
            file(MAKE_DIRECTORY ${file_ui_folder})
        endif()

        if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.17")
            set(remove_command rm -rf)
        else()
            set(remove_command "remove_directory")
        endif()

        add_custom_command(OUTPUT ${outfile}
            DEPENDS ${QT_CMAKE_EXPORT_NAMESPACE}::uic
            COMMAND ${CMAKE_COMMAND} -E ${remove_command} "${file_ui_folder}/${include_folder}"
            COMMAND ${CMAKE_COMMAND} -E make_directory ${inc_dir_to_create}
            COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::uic ${ui_options} -o
            ${outfile} ${infile}
            COMMAND_EXPAND_LISTS
            MAIN_DEPENDENCY ${infile} VERBATIM)
    endforeach()

    # We define a dummy target to check in the build time whether AUTOUIC is
    # enabled after qt_add_ui is called. If AUTOUIC is enabled, we show an
    # error message to the user.
    get_target_property(is_guard_on ${target} _qt_ui_property_check_guard)
    if(NOT is_guard_on)
        # Ninja fails when a newline is used. That's why message is
        # divided into parts.
        set(error_message_1
            "Error: The target \"${target}\" has \"AUTOUIC\" enabled.")
        set(error_message_2
            "Error: Please disable \"AUTOUIC\" for the target \"${target}\".")

        set(ui_property_check_dummy_file
            "${CMAKE_CURRENT_BINARY_DIR}/${target}_autogen/ui_property_check_timestamp")

        add_custom_command(OUTPUT ${ui_property_check_dummy_file}
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "${CMAKE_CURRENT_BINARY_DIR}/${target}_autogen"
            COMMAND ${CMAKE_COMMAND} -E touch ${ui_property_check_dummy_file}
            COMMAND
                "$<IF:$<BOOL:$<TARGET_PROPERTY:${target},AUTOUIC>>,\
${CMAKE_COMMAND};-E;echo;${error_message_1},\
${CMAKE_COMMAND};-E;true>"
            COMMAND
                "$<IF:$<BOOL:$<TARGET_PROPERTY:${target},AUTOUIC>>,\
${CMAKE_COMMAND};-E;echo;${error_message_2},\
${CMAKE_COMMAND};-E;true>"
            # Remove the dummy file so that the error message is shown until
            # the AUTOUIC is disabled. Otherwise, the error message is shown
            # only once when the AUTOUIC is enabled with Visual Studio generator.
            COMMAND
                "$<IF:$<BOOL:$<TARGET_PROPERTY:${target},AUTOUIC>>,\
${CMAKE_COMMAND};-E;remove;${ui_property_check_dummy_file},\
${CMAKE_COMMAND};-E;true>"
            COMMAND
                "$<IF:$<BOOL:$<TARGET_PROPERTY:${target},AUTOUIC>>,\
${CMAKE_COMMAND};-E;false,\
${CMAKE_COMMAND};-E;true>"
            COMMAND_EXPAND_LISTS
            VERBATIM)

        # When AUTOUIC is enabled, AUTOUIC runs before ${target}_ui_property_check
        # target. So we need to add ${target}_ui_property_check as a dependency
        # to ${target} to make sure that AUTOUIC runs after ${target}_ui_property_check
        if (NOT QT_NO_MIX_UI_AUTOUIC_CHECK)
            set_target_properties(${target} PROPERTIES
                _qt_ui_property_check_guard ON)
            add_custom_target(${target}_ui_property_check
                ALL DEPENDS ${ui_property_check_dummy_file})
            _qt_internal_assign_to_internal_targets_folder(
                ${target}_ui_property_check)
            add_dependencies(${target} ${target}_ui_property_check)
        endif()
    endif()

    # write prefix info file
    if(CMAKE_GENERATOR MATCHES "Ninja")
        list(PREPEND prefix_info_list "include_guard()")
        string(REPLACE ";" "\n" prefix_info_list "${prefix_info_list}")
        file(WRITE "${prefix_info_file_cmake}" "${prefix_info_list}")
    endif()
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_add_ui)
        if(QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_add_ui(${ARGN})
        else()
            message(FATAL_ERROR "qt_add_ui() is only available in Qt 6.")
        endif()
    endfunction()
endif()

