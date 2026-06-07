# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# This function creates a CMake target for a generic console or GUI binary.
# Please consider to use a more specific version target like the one created
# by qt_add_test or qt_add_tool below.
# One-value Arguments:
#     CORE_LIBRARY
#         The argument accepts 'Bootstrap' or 'None' values. If the argument value is set to
#         'Bootstrap' the Qt::Bootstrap library is linked to the executable instead of Qt::Core.
#         The 'None' value points that core library is not necessary and avoids linking neither
#         Qt::Core or Qt::Bootstrap libraries. Otherwise the Qt::Core library will be publicly
#         linked to the executable target by default.
function(qt_internal_add_executable name)
    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${__qt_internal_add_executable_optional_args}"
        "${__qt_internal_add_executable_single_args}"
        "${__qt_internal_add_executable_multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if ("x${arg_OUTPUT_DIRECTORY}" STREQUAL "x")
        set(arg_OUTPUT_DIRECTORY "${QT_BUILD_DIR}/${INSTALL_BINDIR}")
    endif()

    get_filename_component(arg_OUTPUT_DIRECTORY "${arg_OUTPUT_DIRECTORY}"
        ABSOLUTE BASE_DIR "${QT_BUILD_DIR}")

    if ("x${arg_INSTALL_DIRECTORY}" STREQUAL "x")
        set(arg_INSTALL_DIRECTORY "${INSTALL_BINDIR}")
    endif()

    _qt_internal_create_executable(${name})
    qt_internal_mark_as_internal_target(${name})

    set_target_properties(${name} PROPERTIES
        _qt_is_test_executable ${arg_QT_TEST}
        _qt_is_manual_test ${arg_QT_MANUAL_TEST}
        _qt_is_benchmark_test ${arg_QT_BENCHMARK_TEST}
    )

    # Opt out to skip the new way of running test finalizers, and instead use the old way for
    # specific platforms.
    # TODO: Remove once we confirm that the new way of running test finalizers for all platforms
    # doesn't cause any issues.
    if(NOT QT_INTERNAL_SKIP_TEST_FINALIZERS_V2)
        # We don't run finalizers for all executables on all platforms, because there are still
        # some unsolved issues there. One of them is trying to run finalizers for the
        # qmlimportscanner executable would create a circular depenendecy trying to run
        # qmlimportscanner on itself.
        #
        # For now, we only run finalizers for test-like executables on all platforms, and all
        # android and wasm internal executables.
        # For android and wasm all executables, to be behavior compatible with the old way of
        # running finalizers.
        if(ANDROID
            OR WASM
            OR arg_QT_TEST
            OR arg_QT_MANUAL_TEST
            OR arg_QT_BENCHMARK_TEST)
            set(QT_INTERNAL_USE_POOR_MANS_SCOPE_FINALIZER TRUE)
            _qt_internal_finalize_target_defer("${name}")
        endif()
    else()
        if(ANDROID)
            # This direct calls the finalizer, which in the v2 way is deferred.
            _qt_internal_android_executable_finalizer(${name})
        endif()
        if(WASM)
            # This defer calls the finalizer.
            qt_internal_wasm_add_finalizers(${name})
        endif()
    endif()

    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)
    if(arg_QT_APP
            AND QT_FEATURE_debug_and_release
            AND CMAKE_VERSION VERSION_GREATER_EQUAL "3.19.0"
            AND is_multi_config
        )
        set_property(TARGET "${name}"
            PROPERTY EXCLUDE_FROM_ALL "$<NOT:$<CONFIG:${QT_MULTI_CONFIG_FIRST_CONFIG}>>")
    endif()

    if (arg_VERSION)
        if(arg_VERSION MATCHES "[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+")
            # nothing to do
        elseif(arg_VERSION MATCHES "[0-9]+\\.[0-9]+\\.[0-9]+")
            set(arg_VERSION "${arg_VERSION}.0")
        elseif(arg_VERSION MATCHES "[0-9]+\\.[0-9]+")
            set(arg_VERSION "${arg_VERSION}.0.0")
        elseif (arg_VERSION MATCHES "[0-9]+")
            set(arg_VERSION "${arg_VERSION}.0.0.0")
        else()
            message(FATAL_ERROR "Invalid version format")
        endif()
    endif()

    if(arg_DELAY_TARGET_INFO)
        # Delay the setting of target info properties if requested. Needed for scope finalization
        # of Qt apps.
        set_target_properties("${name}" PROPERTIES
            QT_DELAYED_TARGET_VERSION "${arg_VERSION}"
            QT_DELAYED_TARGET_PRODUCT "${arg_TARGET_PRODUCT}"
            QT_DELAYED_TARGET_DESCRIPTION "${arg_TARGET_DESCRIPTION}"
            QT_DELAYED_TARGET_COMPANY "${arg_TARGET_COMPANY}"
            QT_DELAYED_TARGET_COPYRIGHT "${arg_TARGET_COPYRIGHT}"
            )
    else()
        if(NOT arg_TARGET_DESCRIPTION)
            set(arg_TARGET_DESCRIPTION "Qt ${name}")
        endif()
        qt_set_target_info_properties(${name} ${ARGN}
            TARGET_DESCRIPTION ${arg_TARGET_DESCRIPTION}
            TARGET_VERSION ${arg_VERSION})
    endif()

    if (WIN32 AND NOT arg_DELAY_RC)
        _qt_internal_generate_win32_rc_file(${name})
    endif()

    qt_set_common_target_properties(${name})

    qt_internal_add_repo_local_defines(${name})

    if(ANDROID)
        # The above call to qt_set_common_target_properties() sets the symbol
        # visibility to hidden, but for Android, we need main() to not be hidden
        # because it has to be loadable at runtime using dlopen().
        set_property(TARGET ${name} PROPERTY C_VISIBILITY_PRESET default)
        set_property(TARGET ${name} PROPERTY CXX_VISIBILITY_PRESET default)
    endif()

    qt_autogen_tools_initial_setup(${name})

    qt_internal_default_warnings_are_errors("${name}")

    set(extra_libraries "")
    if(arg_CORE_LIBRARY STREQUAL "Bootstrap")
        list(APPEND extra_libraries ${QT_CMAKE_EXPORT_NAMESPACE}::Bootstrap)
    elseif(NOT arg_CORE_LIBRARY STREQUAL "None")
        list(APPEND extra_libraries ${QT_CMAKE_EXPORT_NAMESPACE}::Core)
    endif()

    set(private_includes
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}"
         ${arg_INCLUDE_DIRECTORIES}
    )

    if(arg_PUBLIC_LIBRARIES)
        message(WARNING
            "qt_internal_add_executable's PUBLIC_LIBRARIES option is deprecated, and will be "
            "removed in a future Qt version. Use the LIBRARIES option instead.")
    endif()

    if(arg_NO_UNITY_BUILD)
        set(arg_NO_UNITY_BUILD "NO_UNITY_BUILD")
    else()
        set(arg_NO_UNITY_BUILD "")
    endif()

    qt_internal_extend_target("${name}"
        ${arg_NO_UNITY_BUILD}
        SOURCES ${arg_SOURCES}
        NO_PCH_SOURCES ${arg_NO_PCH_SOURCES}
        NO_UNITY_BUILD_SOURCES ${arg_NO_UNITY_BUILD_SOURCES}
        INCLUDE_DIRECTORIES ${private_includes}
        DEFINES ${arg_DEFINES}
        LIBRARIES
            ${arg_LIBRARIES}
            ${arg_PUBLIC_LIBRARIES}
            Qt::PlatformCommonInternal
            ${extra_libraries}
        DBUS_ADAPTOR_SOURCES ${arg_DBUS_ADAPTOR_SOURCES}
        DBUS_ADAPTOR_FLAGS ${arg_DBUS_ADAPTOR_FLAGS}
        DBUS_INTERFACE_SOURCES ${arg_DBUS_INTERFACE_SOURCES}
        DBUS_INTERFACE_FLAGS ${arg_DBUS_INTERFACE_FLAGS}
        COMPILE_OPTIONS ${arg_COMPILE_OPTIONS}
        LINK_OPTIONS ${arg_LINK_OPTIONS}
        MOC_OPTIONS ${arg_MOC_OPTIONS}
        ENABLE_AUTOGEN_TOOLS ${arg_ENABLE_AUTOGEN_TOOLS}
        DISABLE_AUTOGEN_TOOLS ${arg_DISABLE_AUTOGEN_TOOLS}
    )
    set_target_properties("${name}" PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${arg_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${arg_OUTPUT_DIRECTORY}"
    )

    if(arg_GUI)
        # Only override if GUI is set. Otherwise leave up to
        # CMake defaults, which may be set by the user elsewhere.
        set_target_properties("${name}" PROPERTIES
            MACOSX_BUNDLE ON
            WIN32_EXECUTABLE ON
    )
    endif()

    if(NOT arg_EXCEPTIONS)
        qt_internal_set_exceptions_flags("${name}" "DEFAULT")
    else()
        qt_internal_set_exceptions_flags("${name}" "${arg_EXCEPTIONS}")
    endif()

    # Check if target needs to be excluded from all target. Also affects qt_install.
    # Set by qt_exclude_tool_directories_from_default_target.
    set(exclude_from_all FALSE)
    if(__qt_exclude_tool_directories)
        foreach(absolute_dir ${__qt_exclude_tool_directories})
            _qt_internal_path_is_prefix(absolute_dir "${CMAKE_CURRENT_SOURCE_DIR}"
                in_current_source)
            if(in_current_source)
                set(exclude_from_all TRUE)
                set_target_properties("${name}" PROPERTIES
                    EXCLUDE_FROM_ALL TRUE
                    _qt_internal_excluded_from_default_target TRUE
                )
                break()
            endif()
        endforeach()
    endif()

    if(NOT arg_NO_INSTALL)
        set(additional_install_args "")
        if(exclude_from_all)
            list(APPEND additional_install_args EXCLUDE_FROM_ALL COMPONENT "ExcludedExecutables")
        endif()

        qt_get_cmake_configurations(cmake_configs)
        foreach(cmake_config ${cmake_configs})
            qt_get_install_target_default_args(
                OUT_VAR install_targets_default_args
                CMAKE_CONFIG "${cmake_config}"
                ALL_CMAKE_CONFIGS ${cmake_configs}
                RUNTIME "${arg_INSTALL_DIRECTORY}"
                LIBRARY "${arg_INSTALL_DIRECTORY}"
                BUNDLE "${arg_INSTALL_DIRECTORY}")

            # Make installation optional for targets that are not built by default in this config
            if(NOT exclude_from_all AND arg_QT_APP AND QT_FEATURE_debug_and_release
                    AND NOT (cmake_config STREQUAL QT_MULTI_CONFIG_FIRST_CONFIG))
                set(install_optional_arg "OPTIONAL")
            else()
                unset(install_optional_arg)
            endif()

            qt_install(TARGETS "${name}"
                       ${additional_install_args} # Needs to be before the DESTINATIONS.
                       ${install_optional_arg}
                       CONFIGURATIONS ${cmake_config}
                       ${install_targets_default_args})
        endforeach()

        if(NOT exclude_from_all AND arg_QT_APP AND QT_FEATURE_debug_and_release)
            set(separate_debug_info_executable_arg "QT_EXECUTABLE")
        else()
            unset(separate_debug_info_executable_arg)
        endif()

        qt_internal_defer_separate_debug_info("${name}"
            SEPARATE_DEBUG_INFO_ARGS
                "${arg_INSTALL_DIRECTORY}"
                ${separate_debug_info_executable_arg}
                ADDITIONAL_INSTALL_ARGS ${additional_install_args}
        )
        qt_internal_install_pdb_files(${name} "${arg_INSTALL_DIRECTORY}")
    endif()

    qt_add_list_file_finalizer(qt_internal_finalize_executable "${name}")
endfunction()

# Finalizer for all generic internal executables.
function(qt_internal_finalize_executable target)
    qt_internal_finalize_executable_separate_debug_info("${target}")
endfunction()

# This function compiles the target at configure time the very first time and creates the custom
# ${target}_build that re-runs compilation at build time if necessary. The resulting executable is
# imported under the provided target name. This function should only be used to compile tiny
# executables with system dependencies only.
# One-value Arguments:
#     CMAKELISTS_TEMPLATE
#         The CMakeLists.txt templated that is used to configure the project
#         for an executable. By default the predefined template from the Qt installation is used.
#     INSTALL_DIRECTORY
#         installation directory of the executable. Ignored if NO_INSTALL is set.
#     OUTPUT_NAME
#         the output name of an executable
#     CONFIG
#         the name of configuration that tool needs to be build with.
# Multi-value Arguments:
#     PACKAGES
#         list of system packages are required to successfully build the project.
#     INCLUDES
#         list of include directories are required to successfully build the project.
#     DEFINES
#         list of definitions are required to successfully build the project.
#     COMPILE_OPTIONS
#         list of compiler options are required to successfully build the project.
#     LINK_OPTIONS
#         list of linker options are required to successfully build the project.
#     SOURCES
#         list of project sources.
#     CMAKE_FLAGS
#         specify flags of the form -DVAR:TYPE=VALUE to be passed to the cmake command-line used to
#         drive the test build.
# Options:
#     WIN32
#         reflects the corresponding add_executable argument.
#     MACOSX_BUNDLE
#         reflects the corresponding add_executable argument.
#     NO_INSTALL
#         avoids installing the tool.
function(qt_internal_add_configure_time_executable target)
    set(one_value_args
        CMAKELISTS_TEMPLATE
        INSTALL_DIRECTORY
        OUTPUT_NAME
        CONFIG
    )
    set(multi_value_args
        PACKAGES
        INCLUDES
        DEFINES
        COMPILE_OPTIONS
        LINK_OPTIONS
        SOURCES
        CMAKE_FLAGS
    )
    set(option_args WIN32 MACOSX_BUNDLE NO_INSTALL)
    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${option_args}" "${one_value_args}" "${multi_value_args}")

    set(target_binary_dir "${CMAKE_CURRENT_BINARY_DIR}/configure_time_bins")
    if(arg_CONFIG)
        set(CMAKE_TRY_COMPILE_CONFIGURATION "${arg_CONFIG}")
        string(TOUPPER "_${arg_CONFIG}" config_suffix)
    endif()

    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)
    if(is_multi_config AND CMAKE_TRY_COMPILE_CONFIGURATION)
        set(configuration_path "${CMAKE_TRY_COMPILE_CONFIGURATION}/")
        set(config_build_arg "--config" "${CMAKE_TRY_COMPILE_CONFIGURATION}")
    endif()

    set(configure_time_target "${target}")
    if(arg_OUTPUT_NAME)
        set(configure_time_target "${arg_OUTPUT_NAME}")
    endif()
    set(target_binary "${configure_time_target}${CMAKE_EXECUTABLE_SUFFIX}")

    set(install_dir "${INSTALL_BINDIR}")
    if(arg_INSTALL_DIRECTORY)
        set(install_dir "${arg_INSTALL_DIRECTORY}")
    endif()

    set(output_directory_relative "${install_dir}")
    set(output_directory "${QT_BUILD_DIR}/${install_dir}")

    set(target_binary_path_relative
        "${output_directory_relative}/${configuration_path}${target_binary}")
    set(target_binary_path
        "${output_directory}/${configuration_path}${target_binary}")

    get_filename_component(target_binary_path "${target_binary_path}" ABSOLUTE)

    if(NOT DEFINED arg_SOURCES)
        message(FATAL_ERROR "No SOURCES given to target: ${target}")
    endif()
    set(sources "${arg_SOURCES}")

    # Timestamp file is required because CMake ignores 'add_custom_command' if we use only the
    # binary file as the OUTPUT.
    set(timestamp_file "${target_binary_dir}/${target_binary}_timestamp")
    add_custom_command(OUTPUT "${target_binary_path}" "${timestamp_file}"
        COMMAND
            ${CMAKE_COMMAND} --build "${target_binary_dir}" --clean-first ${config_build_arg}
        COMMAND
            ${CMAKE_COMMAND} -E touch "${timestamp_file}"
        DEPENDS
            ${sources}
        COMMENT
            "Compiling ${target}"
        VERBATIM
    )

    add_custom_target(${target}_build ALL
        DEPENDS
            "${target_binary_path}"
            "${timestamp_file}"
    )

    set(should_build_at_configure_time TRUE)
    if(QT_INTERNAL_HAVE_CONFIGURE_TIME_${target} AND
        EXISTS "${target_binary_path}" AND EXISTS "${timestamp_file}")
        set(last_ts 0)
        foreach(source IN LISTS sources)
            file(TIMESTAMP "${source}" ts "%s")
            if(${ts} GREATER ${last_ts})
                set(last_ts ${ts})
            endif()
        endforeach()

        file(TIMESTAMP "${target_binary_path}" ts "%s")
        if(${ts} GREATER_EQUAL ${last_ts})
            set(should_build_at_configure_time FALSE)
        endif()
    endif()

    set(cmake_flags_arg "")
    if(arg_CMAKE_FLAGS)
        set(cmake_flags_arg CMAKE_FLAGS "${arg_CMAKE_FLAGS}")
    endif()

    qt_internal_get_enabled_languages_for_flag_manipulation(enabled_languages)
    foreach(lang IN LISTS enabled_languages)
        set(compiler_flags_var "CMAKE_${lang}_FLAGS")
        list(APPEND cmake_flags_arg "-D${compiler_flags_var}:STRING=${${compiler_flags_var}}")
        if(arg_CONFIG)
            set(compiler_flags_var_config "${compiler_flags_var}${config_suffix}")
            list(APPEND cmake_flags_arg
                "-D${compiler_flags_var_config}:STRING=${${compiler_flags_var_config}}")
        endif()
    endforeach()

    qt_internal_get_target_link_types_for_flag_manipulation(target_link_types)
    foreach(linker_type IN LISTS target_link_types)
        set(linker_flags_var "CMAKE_${linker_type}_LINKER_FLAGS")
        list(APPEND cmake_flags_arg "-D${linker_flags_var}:STRING=${${linker_flags_var}}")
        if(arg_CONFIG)
            set(linker_flags_var_config "${linker_flags_var}${config_suffix}")
            list(APPEND cmake_flags_arg
                "-D${linker_flags_var_config}:STRING=${${linker_flags_var_config}}")
        endif()
    endforeach()

    if(NOT "${QT_INTERNAL_CMAKE_FLAGS_CONFIGURE_TIME_TOOL_${target}}" STREQUAL "${cmake_flags_arg}")
        set(should_build_at_configure_time TRUE)
    endif()

    if(should_build_at_configure_time)
        foreach(arg IN LISTS multi_value_args)
            string(TOLOWER "${arg}" template_arg_name)
            set(${template_arg_name} "")
            if(DEFINED arg_${arg})
                set(${template_arg_name} "${arg_${arg}}")
            endif()
        endforeach()

        foreach(arg IN LISTS option_args)
            string(TOLOWER "${arg}" template_arg_name)
            set(${template_arg_name} "")
            if(arg_${arg})
                set(${template_arg_name} "${arg}")
            endif()
        endforeach()

        file(MAKE_DIRECTORY "${target_binary_dir}")
        set(template "${QT_CMAKE_DIR}/QtConfigureTimeExecutableCMakeLists.txt.in")
        if(DEFINED arg_CMAKELISTS_TEMPLATE)
            set(template "${arg_CMAKELISTS_TEMPLATE}")
        endif()

        configure_file("${template}" "${target_binary_dir}/CMakeLists.txt" @ONLY)

        if(EXISTS "${target_binary_dir}/CMakeCache.txt")
            file(REMOVE "${target_binary_dir}/CMakeCache.txt")
        endif()

        try_compile(result
            "${target_binary_dir}"
            "${target_binary_dir}"
            ${target}
            ${cmake_flags_arg}
            OUTPUT_VARIABLE try_compile_output
        )

        set(QT_INTERNAL_CMAKE_FLAGS_CONFIGURE_TIME_TOOL_${target}
            "${cmake_flags_arg}" CACHE INTERNAL "")

        file(WRITE "${timestamp_file}" "")
        set(QT_INTERNAL_HAVE_CONFIGURE_TIME_${target} ${result} CACHE INTERNAL
            "Indicates that the configure-time target ${target} was built")
        if(NOT result)
            message(FATAL_ERROR "Unable to build ${target}: ${try_compile_output}")
        endif()
    endif()

    add_executable(${target} IMPORTED GLOBAL)
    add_executable(${QT_CMAKE_EXPORT_NAMESPACE}::${target} ALIAS ${target})
    set_target_properties(${target} PROPERTIES
        _qt_internal_configure_time_target TRUE
        _qt_internal_configure_time_target_build_location "${target_binary_path_relative}"
        IMPORTED_LOCATION "${target_binary_path}"
    )

    if(NOT arg_NO_INSTALL)
        set_target_properties(${target} PROPERTIES
            _qt_internal_configure_time_target_install_location
                "${install_dir}/${target_binary}"
        )
        qt_path_join(target_install_dir ${QT_INSTALL_DIR} ${install_dir})
        qt_install(PROGRAMS "${target_binary_path}" DESTINATION "${target_install_dir}")
    endif()
endfunction()
