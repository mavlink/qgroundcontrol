# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# This function can be used to add sources/libraries/etc. to the specified CMake target
# if the provided CONDITION evaluates to true.
# One-value Arguments:
#   PRECOMPILED_HEADER
#     Name of the precompiled header that is used for the target.
#   EXTRA_ELF_LINKER_SCRIPT_CONTENT
#     Extra content that should be appended to a target linker script. Applicable for ld only.
#   ELF_LINKER_DYNAMIC_LIST
#     Pass --dynamic-list to the linker.
# Multi-value Arguments:
#   CONDITION
#     The condition under which the target will be extended.
#   CONDITION_INDEPENDENT_SOURCES
#     Source files that should be added to the target unconditionally. Note that if target is Qt
#     module, these files will raise a warning at configure time if the condition is not met.
#   COMPILE_FLAGS
#     Custom compilation flags.
#   EXTRA_ELF_LINKER_SCRIPT_EXPORTS
#     Extra content that should be added to export section of the linker script.
#   NO_PCH_SOURCES
#     Exclude the specified source files from PRECOMPILE_HEADERS and UNITY_BUILD builds.
function(qt_internal_extend_target target)
    if(NOT TARGET "${target}")
        message(FATAL_ERROR "${target} is not a target.")
    endif()
    qt_internal_is_skipped_test(skipped ${target})
    if(skipped)
        return()
    endif()
    qt_internal_is_in_test_batch(in_batch ${target})
    if(in_batch)
        _qt_internal_test_batch_target_name(target)
    endif()

    # Don't try to extend_target when cross compiling an imported host target (like a tool).
    qt_is_imported_target("${target}" is_imported)
    if(is_imported)
        return()
    endif()

    set(option_args
        NO_UNITY_BUILD
        ${__qt_internal_sbom_optional_args}
    )
    set(single_args
        PRECOMPILED_HEADER
        EXTRA_ELF_LINKER_SCRIPT_CONTENT
        ELF_LINKER_DYNAMIC_LIST
        ${__qt_internal_sbom_single_args}
    )
    set(multi_args
        ${__default_public_args}
        ${__default_private_args}
        ${__default_private_module_args}
        CONDITION
        CONDITION_INDEPENDENT_SOURCES
        COMPILE_FLAGS
        EXTRA_ELF_LINKER_SCRIPT_EXPORTS
        ${__qt_internal_sbom_multi_args}
    )

    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${option_args}"
        "${single_args}"
        "${multi_args}"
    )
    _qt_internal_validate_all_args_are_parsed(arg)

    if("x${arg_CONDITION}" STREQUAL "x")
        set(arg_CONDITION ON)
    endif()

    qt_evaluate_config_expression(result ${arg_CONDITION})
    if(${result})
        if(QT_CMAKE_DEBUG_EXTEND_TARGET)
            message("qt_extend_target(${target} CONDITION ${arg_CONDITION} ...): Evaluated")
        endif()
        set(dbus_sources "")
        foreach(adaptor ${arg_DBUS_ADAPTOR_SOURCES})
            _qt_internal_forward_function_args(
                FORWARD_PREFIX arg_DBUS_ADAPTOR
                FORWARD_OUT_VAR forwarded_args
                FORWARD_SINGLE
                    BASENAME
                    FLAGS
            )
            qt_create_qdbusxml2cpp_command("${target}" "${adaptor}" ADAPTOR ${forwarded_args})
            list(APPEND dbus_sources "${adaptor}")
        endforeach()

        foreach(interface ${arg_DBUS_INTERFACE_SOURCES})
            _qt_internal_forward_function_args(
                FORWARD_PREFIX arg_DBUS_INTERFACE
                FORWARD_OUT_VAR forwarded_args
                FORWARD_SINGLE
                    BASENAME
                    FLAGS
            )
            qt_create_qdbusxml2cpp_command("${target}" "${interface}" INTERFACE ${forwarded_args})
            list(APPEND dbus_sources "${interface}")
        endforeach()

        set(all_sources
            ${arg_SOURCES}
            ${dbus_sources}
        )

        get_target_property(target_type ${target} TYPE)
        set(is_library FALSE)
        set(is_interface_lib FALSE)
        set(is_executable FALSE)
        if(${target_type} STREQUAL "STATIC_LIBRARY" OR ${target_type} STREQUAL "SHARED_LIBRARY")
            set(is_library TRUE)
        elseif(target_type STREQUAL "INTERFACE_LIBRARY")
            set(is_interface_lib TRUE)
        elseif(target_type STREQUAL "EXECUTABLE")
            set(is_executable TRUE)
        endif()

        foreach(lib ${arg_PUBLIC_LIBRARIES} ${arg_LIBRARIES})
            # Automatically generate PCH for 'target' using public dependencies.
            # But only if 'target' is a library/module that does not specify its own PCH file.
            if(NOT arg_PRECOMPILED_HEADER AND ${is_library})
                qt_update_precompiled_header_with_library("${target}" "${lib}")
            endif()

            string(REGEX REPLACE "_nolink$" "" base_lib "${lib}")
            if(NOT base_lib STREQUAL lib)
                qt_create_nolink_target("${base_lib}" ${target})
            endif()

            # Collect _sync_headers targets from libraries that the target depends on. This is
            # heuristic way of building the dependency tree between the _sync_headers targets of
            # different Qt modules.
            if(TARGET "${lib}")
                get_target_property(is_imported ${lib} IMPORTED)
                if(NOT is_imported)
                    get_target_property(is_private ${lib} _qt_is_private_module)
                    if(is_private)
                        get_target_property(lib ${lib} _qt_public_module_target_name)
                    endif()
                    set(out_genex "$<TARGET_PROPERTY:${lib},_qt_internal_sync_headers_target>")
                    set_property(TARGET ${target}
                        APPEND PROPERTY _qt_internal_sync_headers_deps "${out_genex}")
                endif()
            endif()
        endforeach()

        list(TRANSFORM arg_PUBLIC_LIBRARIES REPLACE "^Qt::" "${QT_CMAKE_EXPORT_NAMESPACE}::")
        list(TRANSFORM arg_LIBRARIES REPLACE "^Qt::" "${QT_CMAKE_EXPORT_NAMESPACE}::")
        qt_internal_wrap_private_modules("${target}"
            OUT_VAR arg_LIBRARIES
            LIBRARIES ${arg_LIBRARIES})

        # Set-up the target

        # CMake versions less than 3.19 don't support adding the source files to the PRIVATE scope
        # of the INTERFACE libraries. These PRIVATE sources are only needed by IDEs to display
        # them in a project tree, so to avoid build issues and appearing the sources in
        # INTERFACE_SOURCES property of INTERFACE_LIBRARY. Collect them inside the
        # _qt_internal_target_sources property, since they can be useful in the source processing
        # functions. The property itself is not exported and should only be used in the Qt internal
        # build tree.
        if(NOT is_interface_lib OR CMAKE_VERSION VERSION_GREATER_EQUAL "3.19")
            target_sources("${target}" PRIVATE ${all_sources})
            if(arg_COMPILE_FLAGS)
                set_source_files_properties(${all_sources} PROPERTIES
                    COMPILE_FLAGS "${arg_COMPILE_FLAGS}")
            endif()
        else()
            set_property(TARGET ${target} APPEND PROPERTY
                _qt_internal_target_sources ${all_sources})
        endif()

        set(public_visibility_option "PUBLIC")
        set(private_visibility_option "PRIVATE")
        if(is_interface_lib)
            set(public_visibility_option "INTERFACE")
            set(private_visibility_option "INTERFACE")
        endif()
        target_include_directories("${target}"
                                   ${public_visibility_option} ${arg_PUBLIC_INCLUDE_DIRECTORIES}
                                   ${private_visibility_option} ${arg_INCLUDE_DIRECTORIES})
        target_include_directories("${target}" SYSTEM
                                   ${private_visibility_option} ${arg_SYSTEM_INCLUDE_DIRECTORIES})
        target_compile_definitions("${target}"
                                    ${public_visibility_option} ${arg_PUBLIC_DEFINES}
                                    ${private_visibility_option} ${arg_DEFINES})
        target_link_libraries("${target}"
                              ${public_visibility_option} ${arg_PUBLIC_LIBRARIES}
                              ${private_visibility_option} ${arg_LIBRARIES})
        target_compile_options("${target}"
                               ${public_visibility_option} ${arg_PUBLIC_COMPILE_OPTIONS}
                               ${private_visibility_option} ${arg_COMPILE_OPTIONS})
        target_link_options("${target}"
                            ${public_visibility_option} ${arg_PUBLIC_LINK_OPTIONS}
                            ${private_visibility_option} ${arg_LINK_OPTIONS})

        if(NOT is_interface_lib)
            _qt_internal_get_moc_compiler_flavor_flags(flavor_flags)
            set_property(TARGET "${target}" APPEND PROPERTY
                AUTOMOC_MOC_OPTIONS "${arg_MOC_OPTIONS}" ${flavor_flags}
            )

            # Plugin types associated to a module
            if(NOT "x${arg_PLUGIN_TYPES}" STREQUAL "x")
                qt_internal_add_plugin_types("${target}" "${arg_PLUGIN_TYPES}")
            endif()
        endif()

        # When computing the private library dependencies, we need to check not only the known
        # modules added by this repo's qt_build_repo(), but also all module dependencies that
        # were found via find_package().
        qt_internal_get_qt_all_known_modules(known_modules)

        # When a public module depends on a private module (Gui on CorePrivate)
        # make its private module depend on the other private module (GuiPrivate will depend on
        # CorePrivate).
        set(qt_libs_private "")
        foreach(it ${known_modules})
            list(FIND arg_LIBRARIES "Qt::${it}Private" pos)
            if(pos GREATER -1)
                list(APPEND qt_libs_private "Qt::${it}Private")
            endif()
        endforeach()

        if(arg_LIBRARIES)
            _qt_internal_append_to_target_property_without_duplicates(${target}
                _qt_extend_target_libraries "${arg_LIBRARIES}"
            )
        endif()

        if(arg_PUBLIC_LIBRARIES)
            _qt_internal_append_to_target_property_without_duplicates(${target}
                _qt_extend_target_public_libraries "${arg_PUBLIC_LIBRARIES}"
            )
        endif()

        set(all_libraries ${arg_LIBRARIES} ${arg_PUBLIC_LIBRARIES})
        _qt_internal_work_around_autogen_discarded_dependencies(${target} ${all_libraries})

        if(QT_GENERATE_SBOM)
            set(sbom_args "")
            _qt_internal_forward_function_args(
                FORWARD_APPEND
                FORWARD_PREFIX arg
                FORWARD_OUT_VAR sbom_args
                FORWARD_OPTIONS
                    ${__qt_internal_sbom_optional_args}
                FORWARD_SINGLE
                    ${__qt_internal_sbom_single_args}
                FORWARD_MULTI
                    ${__qt_internal_sbom_multi_args}
            )
            # Don't call extend_sbom unless we actually have any SBOM args, to prevent
            # recording of a target if it has no SBOM info. Relevant for QtFooModulePrivate targets
            # which don't appear in the SBOM atm.
            if(sbom_args)
                _qt_internal_extend_sbom(${target} ${sbom_args})
            endif()
        endif()

        set(target_private "${target}Private")
        get_target_property(is_internal_module ${target} _qt_is_internal_module)
        # Internal modules don't have Private targets but we still need to
        # propagate their private dependencies.
        if(is_internal_module)
            set(target_private "${target}")
        endif()
        if(TARGET "${target_private}")
            target_link_libraries("${target_private}"
                                  INTERFACE ${arg_PRIVATE_MODULE_INTERFACE})
            qt_internal_register_target_dependencies("${target_private}"
                PUBLIC ${arg_PRIVATE_MODULE_INTERFACE}
            )
        elseif(arg_PRIVATE_MODULE_INTERFACE)
            set(warning_message "")
            string(APPEND warning_message
                "The PRIVATE_MODULE_INTERFACE option was provided the values:"
                "'${arg_PRIVATE_MODULE_INTERFACE}' "
                "but there is no ${target}Private target to assign them to."
                "Ensure the target exists or remove the option.")
            message(AUTHOR_WARNING "${warning_message}")
        endif()

        set(qt_register_target_dependencies_args "")
        if(arg_PUBLIC_LIBRARIES)
            list(APPEND qt_register_target_dependencies_args
                PUBLIC ${arg_PUBLIC_LIBRARIES})
        endif()
        if(qt_libs_private OR arg_LIBRARIES)
            list(APPEND qt_register_target_dependencies_args
                PRIVATE ${qt_libs_private} ${arg_LIBRARIES})
        endif()
        qt_internal_register_target_dependencies("${target}"
            ${qt_register_target_dependencies_args})

        qt_autogen_tools(${target}
                         ENABLE_AUTOGEN_TOOLS ${arg_ENABLE_AUTOGEN_TOOLS}
                         DISABLE_AUTOGEN_TOOLS ${arg_DISABLE_AUTOGEN_TOOLS})

        qt_update_precompiled_header("${target}" "${arg_PRECOMPILED_HEADER}")
        ## Also exclude them from unity build
        qt_update_ignore_pch_source("${target}" "${arg_NO_PCH_SOURCES}")
        ## Ignore objective-c files for PCH (not supported atm)
        qt_ignore_pch_obj_c_sources("${target}" "${arg_SOURCES}")

        if(arg_NO_UNITY_BUILD)
            set_target_properties("${target}" PROPERTIES UNITY_BUILD OFF)
            qt_update_ignore_unity_build_sources("${target}" "${arg_SOURCES}")
        endif()
        if(arg_NO_UNITY_BUILD_SOURCES)
            qt_update_ignore_unity_build_sources("${target}" "${arg_NO_UNITY_BUILD_SOURCES}")
        endif()
    else()
        if(QT_CMAKE_DEBUG_EXTEND_TARGET)
            message("qt_extend_target(${target} CONDITION ${arg_CONDITION} ...): Skipped")
        endif()
    endif()

    if(arg_CONDITION_INDEPENDENT_SOURCES)
        set_source_files_properties(${arg_CONDITION_INDEPENDENT_SOURCES} PROPERTIES
            _qt_extend_target_condition "${arg_CONDITION}"
            SKIP_AUTOGEN TRUE
        )

        qt_internal_get_target_sources_property(sources_property)
        set_property(TARGET ${target} APPEND PROPERTY
            ${sources_property} "${arg_CONDITION_INDEPENDENT_SOURCES}")
    endif()

    if(arg_EXTRA_ELF_LINKER_SCRIPT_CONTENT)
        set_target_properties(${target} PROPERTIES
            _qt_extra_elf_linker_script_content "${arg_EXTRA_ELF_LINKER_SCRIPT_CONTENT}")
    endif()
    if(arg_EXTRA_ELF_LINKER_SCRIPT_EXPORTS)
        set_target_properties(${target} PROPERTIES
            _qt_extra_elf_linker_script_exports "${arg_EXTRA_ELF_LINKER_SCRIPT_EXPORTS}")
    endif()
    if(arg_ELF_LINKER_DYNAMIC_LIST)
        qt_internal_apply_dynamic_list_linker_flags(${target} "${arg_ELF_LINKER_DYNAMIC_LIST}")
    endif()

    if(is_executable)
        # If linking against Gui, make sure to also build the default QPA plugin.
        # This makes the experience of an initial Qt configuration to build and run one single
        # test / executable nicer.
        set(linked_libs ${arg_PUBLIC_LIBRARIES} ${arg_LIBRARIES})
        if(linked_libs MATCHES "(^|;)(${QT_CMAKE_EXPORT_NAMESPACE}::|Qt::)?Gui($|;)" AND
            TARGET qpa_default_plugins)
            add_dependencies("${target}" qpa_default_plugins)
        endif()

        # For executables collect static plugins that these targets depend on.
        qt_internal_import_plugins(${target} ${linked_libs})
    endif()
endfunction()

# Takes the consuming target, an output variable and a list of libraries that the consuming target
# links to.
#
# Every library that is a private module is wrapped in $<BUILD_INTERFACE> or
# $<BUILD_LOCAL_INTERFACE> if CMake is new enough.
#
# This is necessary for static builds, because if Qt6Foo links to Qt6BarPrivate, this link
# dependency is purely internal. If we don't do this, CMake adds a target check for Qt6BarPrivate
# in Qt6FooTargets.cmake. This breaks if Qt6BarPrivate is not find_package'ed before.
#
# Every library that is an internal module is wrapped in a similar way, unless the consuming target
# is STATIC, in which case the internal module is added as-is, otherwise linking would fail.
# That's because internal modules are private modules that don't have a public counterpart, so
# if we don't export the linkage, user builds will fail. In this case it's preferable to find
# the Private package, rather than fail the build.
function(qt_internal_wrap_private_modules target)
    set(option_args "")
    set(single_args
        OUT_VAR
    )
    set(multi_args
        LIBRARIES
    )

    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${option_args}"
        "${single_args}"
        "${multi_args}"
    )
    _qt_internal_validate_all_args_are_parsed(arg)

    set(result "")

    if(CMAKE_VERSION VERSION_LESS "3.26")
        set(wrapper_genex "BUILD_INTERFACE")
    else()
        set(wrapper_genex "BUILD_LOCAL_INTERFACE")
    endif()

    get_target_property(target_type "${target}" TYPE)

    foreach(lib IN LISTS arg_LIBRARIES)
        if(TARGET "${lib}")
            get_target_property(lib_is_private_module ${lib} _qt_is_private_module)
            get_target_property(lib_is_internal_module ${lib} _qt_is_internal_module)

            if(lib_is_private_module)
                # Add the public module as non-wrapped link dependency. This is necessary for
                # targets that link only to the private module. Consumers of this target would then
                # get a linker error about missing symbols from that Qt module.
                get_target_property(lib_public_module_target ${lib} _qt_public_module_target_name)
                list(APPEND result "${INSTALL_CMAKE_NAMESPACE}::${lib_public_module_target}")
            endif()

            if(lib_is_private_module
                    OR
                    (lib_is_internal_module
                        AND NOT target_type STREQUAL "STATIC_LIBRARY")
                )
                # Wrap the private or internal module in BUILD_LOCAL_INTERFACE.
                set(lib "$<${wrapper_genex}:${lib}>")
            endif()
        endif()
        list(APPEND result "${lib}")
    endforeach()

    set("${arg_OUT_VAR}" "${result}" PARENT_SCOPE)
endfunction()

# Given CMAKE_CONFIG and ALL_CMAKE_CONFIGS, determines if a directory suffix needs to be appended
# to each destination, and sets the computed install target destination arguments in OUT_VAR.
# Defaults used for each of the destination types, and can be configured per destination type.
function(qt_get_install_target_default_args)
    cmake_parse_arguments(PARSE_ARGV 0 arg
       ""
       "OUT_VAR;OUT_VAR_RUNTIME;CMAKE_CONFIG;RUNTIME;LIBRARY;ARCHIVE;INCLUDES;BUNDLE"
       "ALL_CMAKE_CONFIGS")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_CMAKE_CONFIG)
        message(FATAL_ERROR "No value given for CMAKE_CONFIG.")
    endif()
    if(NOT arg_ALL_CMAKE_CONFIGS)
        message(FATAL_ERROR "No value given for ALL_CMAKE_CONFIGS.")
    endif()
    list(LENGTH arg_ALL_CMAKE_CONFIGS all_configs_count)
    list(GET arg_ALL_CMAKE_CONFIGS 0 first_config)

    set(suffix "")
    if(all_configs_count GREATER 1 AND NOT arg_CMAKE_CONFIG STREQUAL first_config)
        set(suffix "/${arg_CMAKE_CONFIG}")
    endif()

    set(runtime "${INSTALL_BINDIR}")
    if(arg_RUNTIME)
        set(runtime "${arg_RUNTIME}")
    endif()

    set(library "${INSTALL_LIBDIR}")
    if(arg_LIBRARY)
        set(library "${arg_LIBRARY}")
    endif()

    set(archive "${INSTALL_LIBDIR}")
    if(arg_ARCHIVE)
        set(archive "${arg_ARCHIVE}")
    endif()

    set(includes "${INSTALL_INCLUDEDIR}")
    if(arg_INCLUDES)
        set(includes "${arg_INCLUDES}")
    endif()

    set(bundle "${INSTALL_BINDIR}")
    if(arg_BUNDLE)
        set(bundle "${arg_BUNDLE}")
    endif()

    set(args
        RUNTIME DESTINATION  "${runtime}${suffix}"
        LIBRARY DESTINATION  "${library}${suffix}"
        ARCHIVE DESTINATION  "${archive}${suffix}" COMPONENT Devel
        BUNDLE DESTINATION   "${bundle}${suffix}"
        INCLUDES DESTINATION "${includes}${suffix}")
    set(${arg_OUT_VAR} "${args}" PARENT_SCOPE)

    if(arg_OUT_VAR_RUNTIME)
        set(args
            "${runtime}${suffix}"
        )
        set(${arg_OUT_VAR_RUNTIME} "${args}" PARENT_SCOPE)
    endif()
endfunction()

macro(qt_internal_setup_default_target_function_options)
    set(__default_private_args
        SOURCES
        LIBRARIES
        INCLUDE_DIRECTORIES
        SYSTEM_INCLUDE_DIRECTORIES
        DEFINES
        DBUS_ADAPTOR_BASENAME
        DBUS_ADAPTOR_FLAGS
        DBUS_ADAPTOR_SOURCES
        DBUS_INTERFACE_BASENAME
        DBUS_INTERFACE_FLAGS
        DBUS_INTERFACE_SOURCES
        COMPILE_OPTIONS
        LINK_OPTIONS
        MOC_OPTIONS
        DISABLE_AUTOGEN_TOOLS
        ENABLE_AUTOGEN_TOOLS
        PLUGIN_TYPES
        NO_PCH_SOURCES
        NO_UNITY_BUILD_SOURCES
    )
    set(__default_public_args
        PUBLIC_LIBRARIES
        PUBLIC_INCLUDE_DIRECTORIES
        PUBLIC_DEFINES
        PUBLIC_COMPILE_OPTIONS
        PUBLIC_LINK_OPTIONS
    )
    set(__default_private_module_args
        PRIVATE_MODULE_INTERFACE
    )
    set(__default_target_info_args
        TARGET_VERSION
        TARGET_PRODUCT
        TARGET_DESCRIPTION
        TARGET_COMPANY
        TARGET_COPYRIGHT
    )

    _qt_internal_get_sbom_add_target_common_options(
        __qt_internal_sbom_optional_args
        __qt_internal_sbom_single_args
        __qt_internal_sbom_multi_args
    )

    # Collection of arguments so they can be shared across qt_internal_add_executable
    # and qt_internal_add_test_helper.
    set(__qt_internal_add_executable_optional_args
        GUI
        NO_INSTALL
        EXCEPTIONS
        DELAY_RC
        DELAY_TARGET_INFO
        QT_APP
        QT_TEST
        QT_MANUAL_TEST
        QT_BENCHMARK_TEST
        NO_UNITY_BUILD
        ${__qt_internal_sbom_optional_args}
    )
    set(__qt_internal_add_executable_single_args
        CORE_LIBRARY
        OUTPUT_DIRECTORY
        INSTALL_DIRECTORY
        VERSION
        ${__default_target_info_args}
        ${__qt_internal_sbom_single_args}
    )
    set(__qt_internal_add_executable_multi_args
        ${__default_private_args}
        ${__default_public_args}
        ${__qt_internal_sbom_multi_args}
    )
endmacro()

# Append a config-specific postfix to library names to ensure distinct names
# in a multi-config build.
# e.g. lib/libQt6DBus_relwithdebinfo.6.3.0.dylib
# Don't apply the postfix to the first encountered release-like config, so we have at least one
# config without a postifx.
# If postfixes are set by user warn about potential issues.
function(qt_internal_setup_cmake_config_postfix)
    # Collect configuration that require postfix in Qt library names.
    if(QT_GENERATOR_IS_MULTI_CONFIG)
        set(postfix_configurations ${CMAKE_CONFIGURATION_TYPES})
    else()
        set(postfix_configurations ${CMAKE_BUILD_TYPE})

        # Set the default postfix to empty by default for single-config builds.
        string(TOLOWER "${CMAKE_BUILD_TYPE}" build_type_lower)
        set(default_cmake_${build_type_lower}_postfix "")
    endif()

    # Override the generic debug postfixes above with custom debug postfixes (even in a single
    # config build) to follow the conventions we had since Qt 5.
    # e.g. lib/libQt6DBus_debug.6.3.0.dylib
    if(WIN32)
        if(MINGW)
            # On MinGW we don't have "d" suffix for debug libraries like on Linux,
            # unless we're building debug and release libraries in one go.
            if(QT_GENERATOR_IS_MULTI_CONFIG)
                set(default_cmake_debug_postfix "d")
            endif()
        else()
            set(default_cmake_debug_postfix "d")
        endif()
    elseif(APPLE)
        # Only add a suffix for explicit no-framework builds.
        # For framework builds the library inside the framework
        # is always unsuffixed, and we want to match that for
        # plugins and other non-framework (static) libraries.
        if(NOT (QT_FEATURE_framework OR FEATURE_framework
           # Account for default in configure.json being ON
           OR (NOT DEFINED QT_FEATURE_framework AND NOT DEFINED FEATURE_framework)))
            set(default_cmake_debug_postfix "_debug")
        endif()
    endif()

    set(custom_postfix_vars "")
    set(release_configs Release RelWithDebInfo MinSizeRel)
    set(found_first_release_config FALSE)
    foreach(config_type IN LISTS postfix_configurations)
        string(TOLOWER "${config_type}" config_type_lower)
        string(TOUPPER "${config_type}" config_type_upper)
        set(postfix_var CMAKE_${config_type_upper}_POSTFIX)

        # Skip assigning postfix for the first release-like config.
        if(NOT found_first_release_config
                AND config_type IN_LIST release_configs)
            set(found_first_release_config TRUE)
            if(NOT "${${postfix_var}}" STREQUAL "")
                list(APPEND custom_postfix_vars ${postfix_var})
            endif()
            continue()
        endif()

        # Check if the default postfix is set, use '_<config_type_lower>' otherwise.
        set(default_postfix_var
            default_cmake_${config_type_lower}_postfix)
        if(NOT DEFINED ${default_postfix_var})
            set(${default_postfix_var}
                "_${config_type_lower}")
        endif()

        # If postfix is set by user avoid changing it, but save postfix variable that has
        # a non-default value for further warning.
        if("${${postfix_var}}" STREQUAL "")
            set(${postfix_var} "${${default_postfix_var}}")
            set(${postfix_var} "${${default_postfix_var}}" PARENT_SCOPE)
        elseif(NOT "${${postfix_var}}" STREQUAL "${${default_postfix_var}}")
            list(APPEND custom_postfix_vars ${postfix_var})
        endif()

        # Adjust framework postfixes accordingly
        if(APPLE)
            set(CMAKE_FRAMEWORK_MULTI_CONFIG_POSTFIX_${config_type_upper}
                "${${postfix_var}}"  PARENT_SCOPE)
        endif()
    endforeach()
    if(custom_postfix_vars)
        list(REMOVE_DUPLICATES custom_postfix_vars)
        list(JOIN custom_postfix_vars ", " postfix_vars_string)

        message(WARNING "You are using custom library postfixes: '${postfix_vars_string}' which are"
            " considered experimental and are not officially supported by Qt."
            " Expect unforeseen issues and user projects built with qmake to be broken."
        )
    endif()
endfunction()

function(qt_is_imported_target target out_var)
    if(NOT TARGET "${target}")
        set(target "${QT_CMAKE_EXPORT_NAMESPACE}::${target}")
    endif()
    if(NOT TARGET "${target}")
        message(FATAL_ERROR "Invalid target given to qt_is_imported_target: ${target}")
    endif()
    get_target_property(is_imported "${target}" IMPORTED)
    set(${out_var} "${is_imported}" PARENT_SCOPE)
endfunction()

# Add Qt::target and Qt6::target as aliases for the target
function(qt_internal_add_target_aliases target)
    set(versionless_alias "Qt::${target}")
    set(versionfull_alias "Qt${PROJECT_VERSION_MAJOR}::${target}")
    set_target_properties("${target}" PROPERTIES _qt_versionless_alias "${versionless_alias}")
    set_target_properties("${target}" PROPERTIES _qt_versionfull_alias "${versionfull_alias}")

    get_target_property(type "${target}" TYPE)
    if (type STREQUAL EXECUTABLE)
        add_executable("${versionless_alias}" ALIAS "${target}")
        add_executable("${versionfull_alias}" ALIAS "${target}")
    else()
        add_library("${versionless_alias}" ALIAS "${target}")
        add_library("${versionfull_alias}" ALIAS "${target}")
    endif()
endfunction()

function(qt_get_cmake_configurations out_var)
    set(possible_configs "${CMAKE_BUILD_TYPE}")
    if(CMAKE_CONFIGURATION_TYPES)
        set(possible_configs "${CMAKE_CONFIGURATION_TYPES}")
    endif()
    set(${out_var} "${possible_configs}" PARENT_SCOPE)
endfunction()

function(qt_clone_property_for_configs target property configs)
    get_target_property(value "${target}" "${property}")
    foreach(config ${configs})
        string(TOUPPER "${config}" upper_config)
        set_property(TARGET "${target}" PROPERTY "${property}_${upper_config}" "${value}")
    endforeach()
endfunction()

function(qt_handle_multi_config_output_dirs target)
    qt_get_cmake_configurations(possible_configs)
    qt_clone_property_for_configs(${target} LIBRARY_OUTPUT_DIRECTORY "${possible_configs}")
    qt_clone_property_for_configs(${target} RUNTIME_OUTPUT_DIRECTORY "${possible_configs}")
    qt_clone_property_for_configs(${target} ARCHIVE_OUTPUT_DIRECTORY "${possible_configs}")
endfunction()

# Set target properties that are the same for all modules, plugins, executables
# and 3rdparty libraries.
function(qt_set_common_target_properties target)
    if(QT_FEATURE_reduce_exports)
        set_target_properties(${target} PROPERTIES
            C_VISIBILITY_PRESET hidden
            CXX_VISIBILITY_PRESET hidden
            OBJC_VISIBILITY_PRESET hidden
            OBJCXX_VISIBILITY_PRESET hidden
            VISIBILITY_INLINES_HIDDEN 1)
    endif()
    qt_internal_set_compile_pdb_names("${target}")
endfunction()

# Set common, informational target properties.
#
# On Windows, these properties are used to generate the version information resource.
function(qt_set_target_info_properties target)
    cmake_parse_arguments(arg "" "${__default_target_info_args}" "" ${ARGN})
    if(NOT arg_TARGET_VERSION)
        set(arg_TARGET_VERSION "${PROJECT_VERSION}.0")
    endif()
    if(NOT arg_TARGET_PRODUCT)
        set(arg_TARGET_PRODUCT "Qt6")
    endif()
    if(NOT arg_TARGET_DESCRIPTION)
        set(arg_TARGET_DESCRIPTION "C++ Application Development Framework")
    endif()
    if(NOT arg_TARGET_COMPANY)
        set(arg_TARGET_COMPANY "The Qt Company Ltd.")
    endif()
    if(NOT arg_TARGET_COPYRIGHT)
        set(arg_TARGET_COPYRIGHT "${QT_COPYRIGHT}")
    endif()
    set_target_properties(${target} PROPERTIES
        QT_TARGET_VERSION "${arg_TARGET_VERSION}"
        QT_TARGET_COMPANY_NAME "${arg_TARGET_COMPANY}"
        QT_TARGET_DESCRIPTION "${arg_TARGET_DESCRIPTION}"
        QT_TARGET_COPYRIGHT "${arg_TARGET_COPYRIGHT}"
        QT_TARGET_PRODUCT_NAME "${arg_TARGET_PRODUCT}")
endfunction()

# Uses the QT_DELAYED_TARGET_* property values to set the final QT_TARGET_* properties.
# Needed when doing executable finalization at the end of a subdirectory scope
# (aka scope finalization).
function(qt_internal_set_target_info_properties_from_delayed_properties target)
    set(args "")
    foreach(prop ${__default_target_info_args})
        get_target_property(prop_value "${target}" "QT_DELAYED_${prop}")
        list(APPEND args "${prop}" "${prop_value}")
    endforeach()
    qt_set_target_info_properties(${target} ${args})
endfunction()

# Updates the QT_DELAYED_ properties with values from the QT_ variants, in case if they were
# set in-between a qt_add_* call and before scope finalization.
function(qt_internal_update_delayed_target_info_properties target)
    foreach(prop ${__default_target_info_args})
        get_target_property(prop_value "${target}" "QT_${prop}")
        get_target_property(delayed_prop_value ${target} "QT_DELAYED_${prop}")
        set(final_value "${delayed_prop_value}")
        if(prop_value)
            set(final_value "${prop_value}")
        endif()
        set_target_properties(${target} PROPERTIES "QT_DELAYED_${prop}" "${final_value}")
    endforeach()
endfunction()

function(qt_internal_check_directory_or_type name dir type default result_var)
    if ("x${dir}" STREQUAL x)
        if("x${type}" STREQUAL x)
            message(FATAL_ERROR
                "qt_internal_add_plugin called without setting either PLUGIN_TYPE or ${name}.")
        endif()
        set(${result_var} "${default}" PARENT_SCOPE)
    else()
        set(${result_var} "${dir}" PARENT_SCOPE)
    endif()
endfunction()

macro(qt_internal_get_export_additional_targets_keywords option_args single_args multi_args)
    set(${option_args}
    )
    set(${single_args}
        EXPORT_NAME_PREFIX
    )
    set(${multi_args}
        TARGETS
        TARGET_EXPORT_NAMES
    )
endmacro()

# Create a Qt*AdditionalTargetInfo.cmake file that is included by Qt*Config.cmake
# and sets IMPORTED_*_<CONFIG> properties on the exported targets.
#
# The file also makes the targets global if the QT_PROMOTE_TO_GLOBAL_TARGETS property is set in the
# consuming project.
# When using a CMake version lower than 3.21, only the specified TARGETS are made global.
# E.g. transitive non-Qt 3rd party targets of the specified targets are not made global.
#
# EXPORT_NAME_PREFIX:
#    The portion of the file name before AdditionalTargetInfo.cmake
# CONFIG_INSTALL_DIR:
#    Installation location for the target info file
# TARGETS:
#    The internal target names. Those must be actual targets.
# TARGET_EXPORT_NAMES:
#    The target names how they appear in the QtXXXTargets.cmake files.
#    The names get prefixed by ${QT_CMAKE_EXPORT_NAMESPACE}:: unless they already are.
#    This argument may be empty, then the target export names are the same as the internal ones.
#
# TARGETS and TARGET_EXPORT_NAMES must contain exactly the same number of elements.
# Example: TARGETS = qmljs_native
#          TARGET_EXPORT_NAMES = Qt6::qmljs
#
function(qt_internal_export_additional_targets_file)
    qt_internal_get_export_additional_targets_keywords(option_args single_args multi_args)
    cmake_parse_arguments(arg
        "${option_args}"
        "${single_args};CONFIG_INSTALL_DIR"
        "${multi_args}"
        ${ARGN})

    qt_internal_append_export_additional_targets()

    set_property(GLOBAL APPEND
        PROPERTY _qt_export_additional_targets_export_name_prefix_${id} "${arg_EXPORT_NAME_PREFIX}")
    set_property(GLOBAL APPEND
        PROPERTY _qt_export_additional_targets_config_install_dir_${id} "${arg_CONFIG_INSTALL_DIR}")

    qt_add_list_file_finalizer(qt_internal_export_additional_targets_file_finalizer "${id}")
endfunction()

function(qt_internal_get_export_additional_targets_id export_name out_var)
    string(MAKE_C_IDENTIFIER "${export_name}" id)
    set(${out_var} "${id}" PARENT_SCOPE)
endfunction()

# Uses outer-scope variables to keep the implementation less verbose.
macro(qt_internal_append_export_additional_targets)
    qt_internal_validate_export_additional_targets(
        EXPORT_NAME_PREFIX "${arg_EXPORT_NAME_PREFIX}"
        TARGETS ${arg_TARGETS}
        TARGET_EXPORT_NAMES ${arg_TARGET_EXPORT_NAMES})

    qt_internal_get_export_additional_targets_id("${arg_EXPORT_NAME_PREFIX}" id)

    set_property(GLOBAL APPEND
        PROPERTY _qt_export_additional_targets_${id} "${arg_TARGETS}")
    set_property(GLOBAL APPEND
        PROPERTY _qt_export_additional_target_export_names_${id} "${arg_TARGET_EXPORT_NAMES}")
endmacro()

# Can be called to add additional targets to the file after the initial setup call.
# Used for resources.
function(qt_internal_add_targets_to_additional_targets_export_file)
    qt_internal_get_export_additional_targets_keywords(option_args single_args multi_args)
    cmake_parse_arguments(arg
        "${option_args}"
        "${single_args}"
        "${multi_args}"
        ${ARGN})

    qt_internal_append_export_additional_targets()
endfunction()

function(qt_internal_validate_export_additional_targets)
    qt_internal_get_export_additional_targets_keywords(option_args single_args multi_args)
    cmake_parse_arguments(arg
        "${option_args}"
        "${single_args}"
        "${multi_args}"
        ${ARGN})

    if(NOT arg_EXPORT_NAME_PREFIX)
        message(FATAL_ERROR "qt_internal_validate_export_additional_targets: "
            "Missing EXPORT_NAME_PREFIX argument.")
    endif()

    list(LENGTH arg_TARGETS num_TARGETS)
    list(LENGTH arg_TARGET_EXPORT_NAMES num_TARGET_EXPORT_NAMES)
    if(num_TARGET_EXPORT_NAMES GREATER 0)
        if(NOT num_TARGETS EQUAL num_TARGET_EXPORT_NAMES)
            message(FATAL_ERROR "qt_internal_validate_export_additional_targets: "
                "TARGET_EXPORT_NAMES is set but has ${num_TARGET_EXPORT_NAMES} elements while "
                "TARGETS has ${num_TARGETS} elements. "
                "They must contain the same number of elements.")
        endif()
    else()
        set(arg_TARGET_EXPORT_NAMES ${arg_TARGETS})
    endif()

    set(arg_TARGETS "${arg_TARGETS}" PARENT_SCOPE)
    set(arg_TARGET_EXPORT_NAMES "${arg_TARGET_EXPORT_NAMES}" PARENT_SCOPE)
endfunction()

# The finalizer might be called multiple times in the same directory scope, but it will only process
# one specific id.
function(qt_internal_export_additional_targets_file_finalizer id)
    get_property(arg_EXPORT_NAME_PREFIX GLOBAL PROPERTY
        _qt_export_additional_targets_export_name_prefix_${id})
    get_property(arg_CONFIG_INSTALL_DIR GLOBAL PROPERTY
        _qt_export_additional_targets_config_install_dir_${id})
    get_property(arg_TARGETS GLOBAL PROPERTY
        _qt_export_additional_targets_${id})
    get_property(arg_TARGET_EXPORT_NAMES GLOBAL PROPERTY
        _qt_export_additional_target_export_names_${id})

    list(LENGTH arg_TARGETS num_TARGETS)

    # Determine the release configurations we're currently building
    if(QT_GENERATOR_IS_MULTI_CONFIG)
        set(active_configurations ${CMAKE_CONFIGURATION_TYPES})
    else()
        set(active_configurations ${CMAKE_BUILD_TYPE})
    endif()
    unset(active_release_configurations)
    foreach(config ${active_configurations})
        string(TOUPPER ${config} ucconfig)
        if(NOT ucconfig STREQUAL "DEBUG")
            list(APPEND active_release_configurations ${config})
        endif()
    endforeach()

    if(active_release_configurations)
        # Use the first active release configuration as *the* release config for imported targets
        # and for QT_DEFAULT_IMPORT_CONFIGURATION.
        list(GET active_release_configurations 0 release_cfg)
        string(TOUPPER ${release_cfg} uc_release_cfg)
        set(uc_default_cfg ${uc_release_cfg})

        # Determine the release configurations we do *not* build currently
        set(configurations_to_export Release;RelWithDebInfo;MinSizeRel)
        list(REMOVE_ITEM configurations_to_export ${active_configurations})
    else()
        # There are no active release configurations.
        # Use the first active configuration for QT_DEFAULT_IMPORT_CONFIGURATION.
        unset(uc_release_cfg)
        list(GET active_configurations 0 default_cfg)
        string(TOUPPER ${default_cfg} uc_default_cfg)
        unset(configurations_to_export)
    endif()

    set(content "# Additional target information for ${arg_EXPORT_NAME_PREFIX}
if(NOT DEFINED QT_DEFAULT_IMPORT_CONFIGURATION)
    set(QT_DEFAULT_IMPORT_CONFIGURATION ${uc_default_cfg})
endif()
")

    math(EXPR n "${num_TARGETS} - 1")
    foreach(i RANGE ${n})
        list(GET arg_TARGETS ${i} target)
        list(GET arg_TARGET_EXPORT_NAMES ${i} target_export_name)

        set(full_target ${target_export_name})
        if(NOT full_target MATCHES "^${QT_CMAKE_EXPORT_NAMESPACE}::")
            string(PREPEND full_target "${QT_CMAKE_EXPORT_NAMESPACE}::")
        endif()

        # Tools are already made global unconditionally in QtFooToolsConfig.cmake.
        # And the
        get_target_property(target_type ${target} TYPE)
        if(NOT target_type STREQUAL "EXECUTABLE")
            string(APPEND content
                "__qt_internal_promote_target_to_global_checked(${full_target})\n")
        endif()

        # INTERFACE libraries don't have IMPORTED_LOCATION-like properties.
        # Skip the rest of the processing for those.
        if(target_type STREQUAL "INTERFACE_LIBRARY")
            continue()
        endif()

        set(properties_retrieved TRUE)

        get_target_property(is_configure_time_target ${target} _qt_internal_configure_time_target)
        if(is_configure_time_target)
            # For Multi-config developer builds we should simply reuse IMPORTED_LOCATION of the
            # target.
            if(NOT QT_WILL_INSTALL AND QT_FEATURE_debug_and_release)
                set(configure_time_target_build_location "")
                get_target_property(configure_time_target_install_location ${target}
                    IMPORTED_LOCATION)
            else()
                if(IS_ABSOLUTE "${arg_CONFIG_INSTALL_DIR}")
                    file(RELATIVE_PATH reverse_relative_prefix_path
                        "${arg_CONFIG_INSTALL_DIR}" "${CMAKE_INSTALL_PREFIX}")
                else()
                    file(RELATIVE_PATH reverse_relative_prefix_path
                        "${CMAKE_INSTALL_PREFIX}/${arg_CONFIG_INSTALL_DIR}"
                        "${CMAKE_INSTALL_PREFIX}")
                endif()

                get_target_property(configure_time_target_build_location ${target}
                    _qt_internal_configure_time_target_build_location)
                string(TOUPPER "${QT_CMAKE_EXPORT_NAMESPACE}_INSTALL_PREFIX" install_prefix_var)
                string(JOIN "" configure_time_target_build_location
                    "$\{CMAKE_CURRENT_LIST_DIR}/"
                    "${reverse_relative_prefix_path}"
                    "${configure_time_target_build_location}")

                get_target_property(configure_time_target_install_location ${target}
                    _qt_internal_configure_time_target_install_location)

                string(JOIN "" configure_time_target_install_location
                    "$\{CMAKE_CURRENT_LIST_DIR}/"
                    "${reverse_relative_prefix_path}"
                    "${configure_time_target_install_location}")
            endif()
            if(configure_time_target_install_location)
                string(APPEND content "
# Import configure-time executable ${full_target}
if(NOT TARGET ${full_target})
    set(_qt_imported_build_location \"${configure_time_target_build_location}\")
    set(_qt_imported_install_location \"${configure_time_target_install_location}\")
    set(_qt_imported_location \"\${_qt_imported_install_location}\")
    if(NOT EXISTS \"$\{_qt_imported_location}\"
          AND NOT \"$\{_qt_imported_build_location}\" STREQUAL \"\")
        set(_qt_imported_location \"\${_qt_imported_build_location}\")
    endif()
    if(NOT EXISTS \"$\{_qt_imported_location}\")
        message(FATAL_ERROR \"Unable to add configure time executable ${full_target}\"
            \" $\{_qt_imported_location} doesn't exists\")
    endif()
    add_executable(${full_target} IMPORTED)
    set_property(TARGET ${full_target} APPEND PROPERTY IMPORTED_CONFIGURATIONS ${default_cfg})
    set_target_properties(${full_target} PROPERTIES IMPORTED_LOCATION_${uc_default_cfg}
        \"$\{_qt_imported_location}\")
    set_property(TARGET ${full_target} PROPERTY IMPORTED_GLOBAL TRUE)
    unset(_qt_imported_location)
    unset(_qt_imported_build_location)
    unset(_qt_imported_install_location)
endif()
\n")
            endif()
        endif()

        # Non-prefix debug-and-release builds: add check for the existence of the debug binary of
        # the target.  It is not built by default.
        if(NOT QT_WILL_INSTALL AND QT_FEATURE_debug_and_release)
            get_target_property(excluded_genex ${target} EXCLUDE_FROM_ALL)
            if(excluded_genex)
                string(APPEND content "
# ${full_target} is not built by default in the Debug configuration. Check existence.
get_target_property(_qt_imported_location ${full_target} IMPORTED_LOCATION_DEBUG)
if(NOT EXISTS \"$\{_qt_imported_location}\")
    get_target_property(_qt_imported_configs ${full_target} IMPORTED_CONFIGURATIONS)
    list(REMOVE_ITEM _qt_imported_configs DEBUG)
    set_property(TARGET ${full_target} PROPERTY IMPORTED_CONFIGURATIONS $\{_qt_imported_configs})
    set_property(TARGET ${full_target} PROPERTY IMPORTED_LOCATION_DEBUG)
endif()\n")
            endif()
        endif()

        set(write_implib FALSE)
        set(write_soname FALSE)
        set(write_objects FALSE)
        set(write_link_dependencies FALSE)
        set(write_location TRUE)

        if(target_type STREQUAL "SHARED_LIBRARY")
            if(WIN32)
                set(write_implib TRUE)
            elseif(WASM)
                # Keep write_soname at FALSE
            else()
                set(write_soname TRUE)
            endif()
            set(write_link_dependencies TRUE)
        elseif(target_type STREQUAL "OBJECT_LIBRARY")
            set(write_objects TRUE)
            set(write_location FALSE)
        endif()

        if(NOT "${uc_release_cfg}" STREQUAL "")
            if(write_location)
                string(APPEND content "get_target_property(_qt_imported_location ${full_target} IMPORTED_LOCATION_${uc_release_cfg})\n")
            endif()
            if(write_implib)
                string(APPEND content "get_target_property(_qt_imported_implib ${full_target} IMPORTED_IMPLIB_${uc_release_cfg})\n")
            endif()
            if(write_soname)
                string(APPEND content "get_target_property(_qt_imported_soname ${full_target} IMPORTED_SONAME_${uc_release_cfg})\n")
            endif()
            if(write_link_dependencies)
                string(APPEND content "get_target_property(_qt_imported_link_dependencies ${full_target} IMPORTED_LINK_DEPENDENT_LIBRARIES_${uc_release_cfg})\n")
            endif()
            if(write_objects)
                string(APPEND content "get_target_property(_qt_imported_objects ${full_target} IMPORTED_OBJECTS_${uc_release_cfg})\n")
                # We generate CLR props as well, because that's what CMake generates for object
                # libraries with CMake 3.27. They are usually empty strings though, aka "".
                string(APPEND content "get_target_property(_qt_imported_clr ${full_target} IMPORTED_COMMON_LANGUAGE_RUNTIME_${uc_release_cfg})\n")
            endif()
        endif()
        if(write_location)
            string(APPEND content "get_target_property(_qt_imported_location_default ${full_target} IMPORTED_LOCATION_$\{QT_DEFAULT_IMPORT_CONFIGURATION})\n")
        endif()
        if(write_implib)
            string(APPEND content "get_target_property(_qt_imported_implib_default ${full_target} IMPORTED_IMPLIB_$\{QT_DEFAULT_IMPORT_CONFIGURATION})\n")
        endif()
        if(write_soname)
            string(APPEND content "get_target_property(_qt_imported_soname_default ${full_target} IMPORTED_SONAME_$\{QT_DEFAULT_IMPORT_CONFIGURATION})\n")
        endif()
        if(write_link_dependencies)
            string(APPEND content "get_target_property(_qt_imported_link_dependencies_default ${full_target} IMPORTED_LINK_DEPENDENT_LIBRARIES_$\{QT_DEFAULT_IMPORT_CONFIGURATION})\n")
        endif()
        if(write_objects)
            string(APPEND content "get_target_property(_qt_imported_objects_default ${full_target} IMPORTED_OBJECTS_$\{QT_DEFAULT_IMPORT_CONFIGURATION})\n")
            string(APPEND content "get_target_property(_qt_imported_clr_default ${full_target} IMPORTED_COMMON_LANGUAGE_RUNTIME_$\{QT_DEFAULT_IMPORT_CONFIGURATION})\n")
        endif()
        foreach(config ${configurations_to_export} "")
            string(TOUPPER "${config}" ucconfig)
            if("${config}" STREQUAL "")
                set(property_suffix "")
                set(var_suffix "_default")
                string(APPEND content "\n# Default configuration")
            else()
                set(property_suffix "_${ucconfig}")
                set(var_suffix "")
                string(APPEND content "
# Import target \"${full_target}\" for configuration \"${config}\"
set_property(TARGET ${full_target} APPEND PROPERTY IMPORTED_CONFIGURATIONS ${ucconfig})
")
            endif()
            if(write_location)
                string(APPEND content "
if(_qt_imported_location${var_suffix})
    set_property(TARGET ${full_target} PROPERTY IMPORTED_LOCATION${property_suffix} \"$\{_qt_imported_location${var_suffix}}\")
endif()")
            endif()
            if(write_implib)
                string(APPEND content "
if(_qt_imported_implib${var_suffix})
    set_property(TARGET ${full_target} PROPERTY IMPORTED_IMPLIB${property_suffix} \"$\{_qt_imported_implib${var_suffix}}\")
endif()")
            endif()
            if(write_soname)
                string(APPEND content "
if(_qt_imported_soname${var_suffix})
    set_property(TARGET ${full_target} PROPERTY IMPORTED_SONAME${property_suffix} \"$\{_qt_imported_soname${var_suffix}}\")
endif()")
            endif()
            if(write_link_dependencies)
                string(APPEND content "
if(_qt_imported_link_dependencies${var_suffix})
    set_property(TARGET ${full_target} PROPERTY IMPORTED_LINK_DEPENDENT_LIBRARIES${property_suffix} \"$\{_qt_imported_link_dependencies${var_suffix}}\")
endif()")
            endif()
            if(write_objects)
                string(APPEND content "
if(_qt_imported_objects${var_suffix})
    set_property(TARGET ${full_target} PROPERTY IMPORTED_OBJECTS${property_suffix} \"$\{_qt_imported_objects${var_suffix}}\")
endif()")
            string(APPEND content "
if(_qt_imported_clr${var_suffix})
    set_property(TARGET ${full_target} PROPERTY IMPORTED_COMMON_LANGUAGE_RUNTIME${property_suffix} \"$\{_qt_imported_clr${var_suffix}}\")
endif()")
            endif()
            string(APPEND content "\n")
        endforeach()
    endforeach()

    if(properties_retrieved)
        string(APPEND content "
unset(_qt_imported_location)
unset(_qt_imported_location_default)
unset(_qt_imported_soname)
unset(_qt_imported_soname_default)
unset(_qt_imported_link_dependencies)
unset(_qt_imported_link_dependencies_default)
unset(_qt_imported_objects)
unset(_qt_imported_objects_default)
unset(_qt_imported_clr)
unset(_qt_imported_clr_default)
unset(_qt_imported_configs)")
    endif()

    qt_path_join(output_file "${arg_CONFIG_INSTALL_DIR}"
        "${arg_EXPORT_NAME_PREFIX}AdditionalTargetInfo.cmake")
    if(NOT IS_ABSOLUTE "${output_file}")
        qt_path_join(output_file "${QT_BUILD_DIR}" "${output_file}")
    endif()
    qt_configure_file(OUTPUT "${output_file}" CONTENT "${content}")
    qt_install(FILES "${output_file}" DESTINATION "${arg_CONFIG_INSTALL_DIR}")
endfunction()

function(qt_internal_export_modern_cmake_config_targets_file)
    cmake_parse_arguments(arg
        ""
        "EXPORT_NAME_PREFIX;CONFIG_BUILD_DIR;CONFIG_INSTALL_DIR"
        "TARGETS"
        ${ARGN}
    )

    if("${arg_TARGETS}" STREQUAL "")
        message(FATAL_ERROR "Target list is empty")
    endif()

    if("${arg_CONFIG_BUILD_DIR}" STREQUAL "")
        message(FATAL_ERROR "CONFIG_BUILD_DIR is not specified")
    endif()

    if("${arg_CONFIG_INSTALL_DIR}" STREQUAL "")
        message(FATAL_ERROR "CONFIG_INSTALL_DIR is not specified")
    endif()

    if("${arg_EXPORT_NAME_PREFIX}" STREQUAL "")
        message(FATAL_ERROR "EXPORT_NAME_PREFIX is not specified")
    endif()

    set(versionless_targets ${arg_TARGETS})

    # CMake versions < 3.18 compatibility code. Creates the mimics of the versioned libraries.
    set(versionless_targets_export "${arg_CONFIG_BUILD_DIR}/${arg_EXPORT_NAME_PREFIX}VersionlessTargets.cmake")
    configure_file("${QT_CMAKE_DIR}/QtVersionlessTargets.cmake.in"
        "${versionless_targets_export}"
        @ONLY
    )

    # CMake versions >= 3.18 code. Create the versionless ALIAS targets.
    set(alias_export "${arg_CONFIG_BUILD_DIR}/${arg_EXPORT_NAME_PREFIX}VersionlessAliasTargets.cmake")
    configure_file("${QT_CMAKE_DIR}/QtVersionlessAliasTargets.cmake.in"
        "${alias_export}"
        @ONLY
    )

    qt_install(FILES
        "${alias_export}"
        "${versionless_targets_export}"
        DESTINATION "${arg_CONFIG_INSTALL_DIR}"
        COMPONENT Devel
    )
endfunction()

function(qt_internal_create_tracepoints name tracepoints_file)
    string(TOLOWER "${name}" provider_name)
    string(PREPEND provider_name "qt")
    set(header_filename "${provider_name}_tracepoints_p.h")
    set(header_path "${CMAKE_CURRENT_BINARY_DIR}/${header_filename}")

    if(QT_FEATURE_lttng OR QT_FEATURE_etw OR QT_FEATURE_ctf)
        set(source_path "${CMAKE_CURRENT_BINARY_DIR}/${provider_name}_tracepoints.cpp")
        qt_configure_file(OUTPUT "${source_path}"
            CONTENT "#define TRACEPOINT_CREATE_PROBES
#define TRACEPOINT_DEFINE
#include \"${header_filename}\"")
        target_sources(${name} PRIVATE "${source_path}")
        target_compile_definitions(${name} PUBLIC Q_TRACEPOINT)

        if(QT_FEATURE_lttng)
            set(tracegen_arg "lttng")
            target_link_libraries(${name} PRIVATE LTTng::UST)
        elseif(QT_FEATURE_etw)
            set(tracegen_arg "etw")
        elseif(QT_FEATURE_ctf)
            set(tracegen_arg "ctf")
        endif()

        if(NOT "${QT_HOST_PATH}" STREQUAL "")
            qt_internal_get_host_info_var_prefix(host_info_var_prefix)
            qt_path_join(tracegen
                "${QT_HOST_PATH}"
                "${${host_info_var_prefix}_LIBEXECDIR}"
                "tracegen")
        else()
            set(tracegen "${QT_CMAKE_EXPORT_NAMESPACE}::tracegen")
        endif()

        get_filename_component(tracepoints_filepath "${tracepoints_file}" ABSOLUTE)
        add_custom_command(OUTPUT "${header_path}"
            COMMAND ${tracegen} ${tracegen_arg} "${tracepoints_filepath}" "${header_path}"
            VERBATIM)
        add_custom_target(${name}_tracepoints_header DEPENDS "${header_path}")
        add_dependencies(${name} ${name}_tracepoints_header)
    else()
        qt_configure_file(OUTPUT "${header_path}" CONTENT "#include <private/qtrace_p.h>\n")
    endif()
endfunction()

function(qt_internal_generate_tracepoints name provider)
    cmake_parse_arguments(arg "" "" "SOURCES" ${ARGN} )
    set(provider_name ${provider})
    string(PREPEND provider_name "qt")
    set(tracepoint_filename "${provider_name}.tracepoints")
    set(tracepoints_path "${CMAKE_CURRENT_BINARY_DIR}/${tracepoint_filename}")
    set(header_filename "${provider_name}_tracepoints_p.h")
    set(header_path "${CMAKE_CURRENT_BINARY_DIR}/${header_filename}")

    if(QT_FEATURE_lttng OR QT_FEATURE_etw OR QT_FEATURE_ctf)

        set(absolute_file_paths "")
        foreach(file IN LISTS arg_SOURCES)
            get_filename_component(absolute_file ${file} ABSOLUTE)
            list(APPEND absolute_file_paths ${absolute_file})
        endforeach()

        if(NOT "${QT_HOST_PATH}" STREQUAL "")
            qt_internal_get_host_info_var_prefix(host_info_var_prefix)
            qt_path_join(tracepointgen
                "${QT_HOST_PATH}"
                "${${host_info_var_prefix}_LIBEXECDIR}"
                "tracepointgen")
        else()
            set(tracepointgen "${QT_CMAKE_EXPORT_NAMESPACE}::tracepointgen")
        endif()

        add_custom_target(${name}_${provider_name}_sync)
        qt_internal_add_sync_header_dependencies(${name}_${provider_name}_sync ${name})

        add_custom_command(OUTPUT "${tracepoints_path}"
            COMMAND ${tracepointgen} ${provider_name} "${tracepoints_path}"  "I$<JOIN:$<TARGET_PROPERTY:${name},INCLUDE_DIRECTORIES>,;>" ${absolute_file_paths}
            DEPENDS ${absolute_file_paths} ${name}_${provider_name}_sync
            VERBATIM)
        add_custom_target(${name}_${provider_name}_tracepoints_file DEPENDS "${tracepoints_path}")
        add_dependencies(${name} ${name}_${provider_name}_tracepoints_file)

            set(source_path "${CMAKE_CURRENT_BINARY_DIR}/${provider_name}_tracepoints.cpp")
            qt_configure_file(OUTPUT "${source_path}"
                CONTENT "#define TRACEPOINT_CREATE_PROBES
    #define TRACEPOINT_DEFINE
    #include \"${header_filename}\"")
        target_sources(${name} PRIVATE "${source_path}")
        target_compile_definitions(${name} PUBLIC Q_TRACEPOINT)

        if(QT_FEATURE_lttng)
            set(tracegen_arg "lttng")
            target_link_libraries(${name} PRIVATE LTTng::UST)
        elseif(QT_FEATURE_etw)
            set(tracegen_arg "etw")
        elseif(QT_FEATURE_ctf)
            set(tracegen_arg "ctf")
        endif()

        if(NOT "${QT_HOST_PATH}" STREQUAL "")
            qt_internal_get_host_info_var_prefix(host_info_var_prefix)
            qt_path_join(tracegen
                "${QT_HOST_PATH}"
                "${${host_info_var_prefix}_LIBEXECDIR}"
                "tracegen")
        else()
            set(tracegen "${QT_CMAKE_EXPORT_NAMESPACE}::tracegen")
        endif()

        get_filename_component(tracepoints_filepath "${tracepoints_path}" ABSOLUTE)
        add_custom_command(OUTPUT "${header_path}"
            COMMAND ${tracegen} ${tracegen_arg} "${tracepoints_filepath}" "${header_path}"
            DEPENDS "${tracepoints_path}"
            VERBATIM)
        add_custom_target(${name}_${provider_name}_tracepoints_header DEPENDS "${header_path}")
        add_dependencies(${name} ${name}_${provider_name}_tracepoints_header)
    else()
        qt_configure_file(OUTPUT "${header_path}" CONTENT "#include <private/qtrace_p.h>\n")
    endif()
endfunction()

function(qt_internal_set_compile_pdb_names target)
    if(MSVC)
        get_target_property(target_type ${target} TYPE)
        if(target_type STREQUAL "STATIC_LIBRARY" OR target_type STREQUAL "OBJECT_LIBRARY")
            get_target_property(output_name ${target} OUTPUT_NAME)
            if(NOT output_name)
                set(output_name "${INSTALL_CMAKE_NAMESPACE}${target}")
            endif()
            set_target_properties(${target} PROPERTIES COMPILE_PDB_NAME "${output_name}")
            set_target_properties(${target} PROPERTIES COMPILE_PDB_NAME_DEBUG "${output_name}d")
        endif()
    endif()
endfunction()

# Installs pdb files for given target into the specified install dir.
#
# MSVC generates 2 types of pdb files:
#  - compile-time generated pdb files (compile flag /Zi + /Fd<pdb_name>)
#  - link-time generated pdb files (link flag /debug + /PDB:<pdb_name>)
#
# CMake allows changing the names of each of those pdb file types by setting
# the COMPILE_PDB_NAME_<CONFIG> and PDB_NAME_<CONFIG> properties. If they are
# left empty, CMake will compute the default names itself (or rather in certain cases
# leave it up to the compiler), without actually setting the property values.
#
# For installation purposes, CMake only provides a generator expression to the
# link time pdb file path, not the compile path one, which means we have to compute the
# path to the compile path pdb files ourselves.
# See https://gitlab.kitware.com/cmake/cmake/-/issues/18393 for details.
#
# For shared libraries and executables, we install the linker provided pdb file via the
# TARGET_PDB_FILE generator expression.
#
# For static libraries there is no linker invocation, so we need to install the compile
# time pdb file. We query the ARCHIVE_OUTPUT_DIRECTORY property of the target to get the
# path to the pdb file, and reconstruct the file name. We use a generator expression
# to append a possible debug suffix, in order to allow installation of all Release and
# Debug pdb files when using Ninja Multi-Config.
function(qt_internal_install_pdb_files target install_dir_path)
    if(MSVC)
        get_target_property(target_type ${target} TYPE)

        if(target_type STREQUAL "EXECUTABLE")
            qt_get_cmake_configurations(cmake_configs)
            list(LENGTH cmake_configs all_configs_count)
            list(GET cmake_configs 0 first_config)
            foreach(cmake_config ${cmake_configs})
                set(suffix "")
                if(all_configs_count GREATER 1 AND NOT cmake_config STREQUAL first_config)
                    set(suffix "/${cmake_config}")
                endif()
                qt_install(FILES "$<TARGET_PDB_FILE:${target}>"
                           CONFIGURATIONS ${cmake_config}
                           DESTINATION "${install_dir_path}${suffix}"
                           OPTIONAL)
            endforeach()

        elseif(target_type STREQUAL "SHARED_LIBRARY"
                OR target_type STREQUAL "MODULE_LIBRARY")
            qt_install(FILES "$<TARGET_PDB_FILE:${target}>"
                       DESTINATION "${install_dir_path}"
                       OPTIONAL)

        elseif(target_type STREQUAL "STATIC_LIBRARY")
            get_target_property(lib_dir "${target}" ARCHIVE_OUTPUT_DIRECTORY)
            if(NOT lib_dir)
                message(FATAL_ERROR
                        "Can't install pdb file for static library ${target}. "
                        "The ARCHIVE_OUTPUT_DIRECTORY path is not known.")
            endif()
            get_target_property(pdb_name "${target}" COMPILE_PDB_NAME)
            qt_path_join(compile_time_pdb_file_path
                         "${lib_dir}" "${pdb_name}$<$<CONFIG:Debug>:d>.pdb")

            qt_install(FILES "${compile_time_pdb_file_path}"
                       DESTINATION "${install_dir_path}" OPTIONAL)
        elseif(target_type STREQUAL "OBJECT_LIBRARY")
            get_target_property(pdb_dir "${target}" COMPILE_PDB_OUTPUT_DIRECTORY)
            if(NOT pdb_dir)
                get_target_property(pdb_dir "${target}" BINARY_DIR)
                if(QT_GENERATOR_IS_MULTI_CONFIG)
                    qt_path_join(pdb_dir "${pdb_dir}" "$<CONFIG>")
                endif()
            endif()
            get_target_property(pdb_name "${target}" COMPILE_PDB_NAME)
            qt_path_join(compile_time_pdb_file_path
                         "${pdb_dir}" "${pdb_name}$<$<CONFIG:Debug>:d>.pdb")

            qt_install(FILES "${compile_time_pdb_file_path}"
                DESTINATION "${install_dir_path}" OPTIONAL)
        endif()
    endif()
endfunction()

# Certain targets might have dependencies on libraries that don't have an Apple Silicon arm64
# slice. When doing a universal macOS build, force those targets to be built only for the
# Intel x86_64 arch.
# This behavior can be disabled for all targets by setting the QT_FORCE_MACOS_ALL_ARCHES cache
# variable to TRUE or by setting the target specific cache variable
# QT_FORCE_MACOS_ALL_ARCHES_${target} to TRUE.
#
# TODO: Ideally we'd use something like _apple_resolve_supported_archs_for_sdk_from_system_lib
# from CMake's codebase to parse which architectures are available in a library, but it's
# not straightforward to extract the library absolute file path from a CMake target. Furthermore
# Apple started using a built-in dynamic linker cache of all system-provided libraries as per
# https://gitlab.kitware.com/cmake/cmake/-/issues/20863
# so if the target is a library in the dynamic cache, that might further complicate how to get
# the list of arches in it.
function(qt_internal_force_macos_intel_arch target)
    if(MACOS AND QT_IS_MACOS_UNIVERSAL AND NOT QT_FORCE_MACOS_ALL_ARCHES
            AND NOT QT_FORCE_MACOS_ALL_ARCHES_${target})
        set(arches "x86_64")
        set_target_properties(${target} PROPERTIES OSX_ARCHITECTURES "${arches}")
    endif()
endfunction()

function(qt_disable_apple_app_extension_api_only target)
    set_target_properties("${target}" PROPERTIES QT_NO_APP_EXTENSION_ONLY_API TRUE)
endfunction()

# Common function to add Qt prefixes to the target name
function(qt_internal_qtfy_target out_var target)
    set(${out_var} "Qt${target}" PARENT_SCOPE)
    set(${out_var}_versioned "Qt${PROJECT_VERSION_MAJOR}${target}" PARENT_SCOPE)
endfunction()

function(qt_internal_get_main_cmake_configuration out_var)
    if(CMAKE_BUILD_TYPE)
        set(config "${CMAKE_BUILD_TYPE}")
    elseif(QT_MULTI_CONFIG_FIRST_CONFIG)
        set(config "${QT_MULTI_CONFIG_FIRST_CONFIG}")
    endif()
    set("${out_var}" "${config}" PARENT_SCOPE)
endfunction()

function(qt_internal_get_upper_case_main_cmake_configuration out_var)
    qt_internal_get_main_cmake_configuration("${out_var}")
    string(TOUPPER "${${out_var}}" upper_config)
    set("${out_var}" "${upper_config}" PARENT_SCOPE)
endfunction()

function(qt_internal_adjust_main_config_runtime_output_dir target output_dir)
    # When building Qt with multiple configurations, place the main configuration executable
    # directly in ${output_dir}, rather than a ${output_dir}/<CONFIG> subdirectory.
    qt_internal_get_upper_case_main_cmake_configuration(main_cmake_configuration)
    set_target_properties("${target}" PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${main_cmake_configuration} "${output_dir}"
    )
endfunction()

# Marks a target with a property that it is a library (shared or static) which was built using the
# internal Qt API (qt_internal_add_module, qt_internal_add_plugin, etc) as opposed to it being
# a user project library (qt_add_library, qt_add_plugin, etc).
#
# Needed to allow selectively applying certain flags via PlatformXInternal targets.
function(qt_internal_mark_as_internal_library target)
    set_target_properties(${target} PROPERTIES _qt_is_internal_library TRUE)
    set_property(TARGET "${target}" APPEND PROPERTY EXPORT_PROPERTIES "_qt_is_internal_library")
    qt_internal_mark_as_internal_target(${target})
endfunction()

# Marks a target with a property that it was built using the internal Qt API (qt_internal_*) as
# opposed to it being a user project library or executable(qt_add_*, etc).
#
# Needed to allow selectively applying certain flags via PlatformXInternal targets.
function(qt_internal_mark_as_internal_target target)
    set_target_properties(${target} PROPERTIES _qt_is_internal_target TRUE)
    set_property(TARGET "${target}" APPEND PROPERTY EXPORT_PROPERTIES "_qt_is_internal_target")
endfunction()

# Marks a target with a property to skip it adding it as a dependency when building examples as
# ExternalProjects.
# Needed to create a ${repo}_src global target that examples can depend on in multi-config builds
# due to a bug in AUTOUIC.
#
# See QTBUG-110369.
function(qt_internal_skip_dependency_for_examples target)
    set_target_properties(${target} PROPERTIES _qt_skip_dependency_for_examples TRUE)
endfunction()

function(qt_internal_is_target_skipped_for_examples target out_var)
    get_property(is_skipped TARGET ${target} PROPERTY _qt_skip_dependency_for_examples)
    if(NOT is_skipped)
        set(is_skipped FALSE)
    endif()
    set(${out_var} "${is_skipped}" PARENT_SCOPE)
endfunction()

function(qt_internal_link_internal_platform_for_object_library target)
    cmake_parse_arguments(arg "" "PARENT_TARGET" "" ${ARGN})
    # We need to apply iOS bitcode flags to object libraries that are associated with internal
    # modules or plugins (e.g. object libraries added by qt_internal_add_resource,
    # qt_internal_add_plugin, etc.)
    # The flags are needed when building iOS apps because Xcode expects bitcode to be
    # present by default.
    # Achieve this by compiling the cpp files with the PlatformModuleInternal compile flags.
    target_link_libraries("${target}" PRIVATE Qt::PlatformModuleInternal)

    if(arg_PARENT_TARGET AND TARGET "${arg_PARENT_TARGET}")
        foreach(property QT_SKIP_WARNINGS_ARE_ERRORS _qt_internal_use_exceptions)
            set_property(TARGET "${target}" PROPERTY
                ${property} "$<BOOL:$<TARGET_PROPERTY:${arg_PARENT_TARGET},${property}>>")
        endforeach()
    endif()
endfunction()

# Use ${dep_target}'s include dirs when building ${target}.
#
# Assumes ${dep_target} is an INTERFACE_LIBRARY that only propagates include dirs and ${target}
# is a Qt module / plugin.
#
# Building ${target} requires ${dep_target}'s include dirs.
# Using ${target} does not require ${dep_target}'s include dirs.
#
# The main use case is adding the private header-only dependency PkgConfig::ATSPI2.
function(qt_internal_add_target_include_dirs target dep_target)
    if(NOT TARGET "${target}")
        message(FATAL_ERROR "${target} is not a valid target.")
    endif()
    if(NOT TARGET "${dep_target}")
        message(FATAL_ERROR "${dep_target} is not a valid target.")
    endif()

    target_include_directories("${target}" PRIVATE
        "$<TARGET_PROPERTY:${dep_target},INTERFACE_INCLUDE_DIRECTORIES>")
endfunction()

# Use ${dep_target}'s include dirs when building ${target} and optionally propagate the include
# dirs to consumers of ${target}.

# Assumes ${dep_target} is an INTERFACE_LIBRARY that only propagates include dirs and ${target}
# is a Qt module / plugin.
#
# Building ${target} requires ${dep_target}'s include dirs.
#
# User projects that don't have ${dep_target}'s headers installed in their system should still
# configure successfully.
#
# To achieve that, consumers of ${target} will only get the include directories of ${dep_target}
# if the latter package and target exists.
#
# A find_package(dep_target) dependency is added to ${target}'s *Dependencies.cmake file.
#
# We use target_include_directories(PRIVATE) instead of target_link_libraries(PRIVATE) because the
# latter would propagate a mandatory LINK_ONLY dependency on the ${dep_target} in a static Qt build.
#
# The main use case is for propagating WrapVulkanHeaders::WrapVulkanHeaders.
function(qt_internal_add_target_include_dirs_and_optionally_propagate target dep_target)
    qt_internal_add_target_include_dirs(${target} ${dep_target})

    target_link_libraries("${target}" INTERFACE "$<TARGET_NAME_IF_EXISTS:${dep_target}>")

    qt_record_extra_third_party_dependency("${target}" "${dep_target}")
endfunction()

# The function disables one or multiple internal global definitions that are defined by the
# qt_internal_add_global_definition function for a specific 'target'.
function(qt_internal_undefine_global_definition target)
    if(NOT TARGET "${target}")
        message(FATAL_ERROR "${target} is not a target.")
    endif()
    qt_internal_is_skipped_test(skipped ${target})
    if(skipped)
        return()
    endif()
    qt_internal_is_in_test_batch(in_batch ${target})
    if(in_batch)
        _qt_internal_test_batch_target_name(target)
    endif()

    if("${ARGN}" STREQUAL "")
        message(FATAL_ERROR "The function expects at least one definition as an argument.")
    endif()

    foreach(definition IN LISTS ARGN)
        set(undef_property_name "QT_INTERNAL_UNDEF_${definition}")
        set_target_properties(${target} PROPERTIES "${undef_property_name}" TRUE)
    endforeach()
endfunction()

# This function adds any defines which are local to the current repository (e.g. qtbase,
# qtmultimedia). Those can be defined in the corresponding .cmake.conf file via
# QT_EXTRA_INTERNAL_TARGET_DEFINES. QT_EXTRA_INTERNAL_TARGET_DEFINES accepts a list of definitions.
# The definitions are passed to target_compile_definitions, which means that values can be provided
# via the FOO=Bar syntax
# This does nothing for interface targets
function(qt_internal_add_repo_local_defines target)
    get_target_property(type "${target}" TYPE)
    if (${type} STREQUAL "INTERFACE_LIBRARY")
        return()
    endif()
    if(DEFINED QT_EXTRA_INTERNAL_TARGET_DEFINES)
        target_compile_definitions("${target}" PRIVATE ${QT_EXTRA_INTERNAL_TARGET_DEFINES})
    endif()
endfunction()

# The function returns the value of the target's SOURCES property. The function takes into account
# the limitation of the CMake version less than 3.19, that restricts to add non-interface sources
# to an interface target.
# Note: The function works correctly only if qt_internal_extend_target is used when adding source
# files.
function(qt_internal_get_target_sources out_var target)
    qt_internal_get_target_sources_property(sources_property)
    get_target_property(${out_var} ${target} ${sources_property})
    set(${out_var} "${${out_var}}" PARENT_SCOPE)
endfunction()

# The function distinguishes what property supposed to store target sources, based on target TYPE
# and the CMake version.
function(qt_internal_get_target_sources_property out_var)
    set(${out_var} "SOURCES")
    get_target_property(target_type ${target} TYPE)
    if(CMAKE_VERSION VERSION_LESS "3.19" AND target_type STREQUAL "INTERFACE_LIBRARY")
        set(${out_var} "_qt_internal_target_sources")
    endif()
    set(${out_var} "${${out_var}}" PARENT_SCOPE)
endfunction()

# This function collects target properties that need to be exported without using CMake's
# EXPORT_PROPERTIES.
# Use cases:
#  - Properties named INTERFACE_foo (which CMake doesn't allow exporting)
#  - Properties that contain generator expressions (need special handling for multi-config builds)
# Usage:
#  qt_internal_add_custom_properties_to_export(target
#      PROPERTIES property1 [property2 ...]
#      PROPERTIES_WITHOUT_GENEXES property3 [property4 ...]
#  )
# Arguments:
#   PROPERTIES
#     should contain names of properties that can differ in multi-config builds (e.g. paths)
#   PROPERTIES_WITHOUT_GENEXES
#     should contain names of properties that will always have the same value in multi config
#     builds (e.g, feature values).
function(qt_internal_add_custom_properties_to_export target)
    set(opt_args "")
    set(single_args "")
    set(multi_args
        PROPERTIES
        PROPERTIES_WITHOUT_GENEXES
    )
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)

    # Prepare multi-config helper genexes.
    if(is_multi_config)
        list(GET CMAKE_CONFIGURATION_TYPES 0 first_config_type)

        # The genex snippet is evaluated to '$<NOT:$<BOOL:$<CONFIG>>>' in the generated cmake file.
        # The check is only applicable to the 'main' configuration. If user project doesn't use
        # multi-config generator, then the check supposed to return true and the value from the
        # 'main' configuration supposed to be used.
        string(CONCAT check_if_config_empty
            "$<1:$><NOT:"
                "$<1:$><BOOL:"
                    "$<1:$><CONFIG$<ANGLE-R>"
                "$<ANGLE-R>"
            "$<ANGLE-R>"
        )

        # The genex snippet is evaluated to '$<CONFIG:'Qt config type'>' in the generated cmake
        # file and checks if the config that user uses matches the generated cmake file config.
        string(CONCAT check_user_config
            "$<1:$><CONFIG:$<CONFIG>$<ANGLE-R>"
        )

        # The genex snippet is evaluated to '$<$<OR:$<CONFIG:'Qt config type'>>:'Property content'>
        # for non-main Qt configs and to
        # $<$<OR:$<CONFIG:'Qt config type'>,$<NOT:$<BOOL:$<CONFIG>>>>:'Property content'> for the
        # main Qt config. This guard is required to choose the correct value of the property for the
        # user project according to the user config type.
        # All genexes need to be escaped properly to protect them from evaluation by the
        # file(GENERATE call in the qt_internal_export_genex_properties function.
        string(CONCAT config_check_begin_multi
            "$<1:$><"
                "$<1:$><OR:"
                    "${check_user_config}"
                    "$<$<CONFIG:${first_config_type}>:$<COMMA>${check_if_config_empty}>"
                "$<ANGLE-R>:"
        )
        set(config_check_end_multi "$<ANGLE-R>")
    endif()
    set(target_name "${QT_CMAKE_EXPORT_NAMESPACE}::${target}")

    set(property_sources
        PROPERTIES
        PROPERTIES_WITHOUT_GENEXES
    )

    foreach(property_source IN LISTS property_sources)
        if(property_source STREQUAL "PROPERTIES")
            # Properties with genexes need multi-config specific handling.
            set(config_check_begin "${config_check_begin_multi}")
            set(config_check_end "${config_check_end_multi}")

            set(output_property "_qt_export_custom_properties_content")
        elseif(property_source STREQUAL "PROPERTIES_WITHOUT_GENEXES")
            # Properties without genexes don't need the config checks.
            set(config_check_begin "")
            set(config_check_end "")

            set(output_property "_qt_export_custom_properties_no_genexes_content")
        else()
            message(FATAL_ERROR "Invalid type of property source" ${property_source}"")
        endif()

        foreach(property IN LISTS arg_${property_source})
            set(target_property_genex "$<TARGET_PROPERTY:${target_name},${property}>")
            # All properties that contain lists need to be protected of processing by JOIN genex
            # calls. So this escapes the semicolons for these list.
            set(target_property_list_escape
                "$<JOIN:$<GENEX_EVAL:${target_property_genex}>,\;>")
            set(property_value
                "\"${config_check_begin}${target_property_list_escape}${config_check_end}\"")
            set_property(TARGET ${target} APPEND PROPERTY "${output_property}"
                "${property} ${property_value}")
        endforeach()
    endforeach()
endfunction()

# This function generates and installs ${EXPORT_NAME_PREFIX}ExportProperties-$<CONFIG>.cmake files
# to be included from inside a FooConfig.cmake file.
#
# The file contains set_property(TARGET PROPERTY) assignments that append values to a given target's
# properties as added by the qt_internal_add_custom_properties_to_export function.
#
# The assigned values are computed from the result of executing the generator expressions that were
# stored in the properties, and are wrapped in config-specific genexes in a multi-config build.
#
# Example output:
# set_property(TARGET Qt6::Foo PROPERTY MY_GENEX_PROP
#                     "$<$<OR:$<CONFIG:RelWithDebInfo>,$<NOT:$<BOOL:$<CONFIG>>>>:OneReleaseVal>")
# set_property(TARGET Qt6::Foo PROPERTY MY_REGULAR_PROP "SecondValue")
# include("${CMAKE_CURRENT_LIST_DIR}/Qt6FooExtraProperties-Debug.cmake")
# inside the include
# set_property(TARGET Qt6::Foo APPEND PROPERTY MY_GENEX_PROP "$<$<OR:$<CONFIG:Debug>>:OneDebugVal>")
#
# Arguments:
# EXPORT_NAME_PREFIX:
#    The portion of the file name before ExtraProperties.cmake
# CONFIG_INSTALL_DIR:
#    Installation location for the file.
# TARGETS:
#    The internal target names.
function(qt_internal_export_genex_properties)
    set(opt_args "")
    set(single_args
        EXPORT_NAME_PREFIX
        CONFIG_INSTALL_DIR
    )
    set(multi_args
        TARGETS
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")

    if(NOT arg_EXPORT_NAME_PREFIX)
        message(FATAL_ERROR "qt_internal_export_genex_properties: "
            "Missing EXPORT_NAME_PREFIX argument.")
    endif()

    if(NOT arg_TARGETS)
        message(FATAL_ERROR "qt_internal_export_genex_properties: "
            "TARGETS argument must contain at least one target")
    endif()

    # TODO: Handling more than one target won't work correctly atm due to trying to create and
    # install the same file name multiple times for each target.
    list(LENGTH arg_TARGETS targets_count)
    if(targets_count GREATER 1)
        message(AUTHOR_WARNING "qt_internal_export_genex_properties: "
            "Specifying more than one target is not fully supported yet.")
    endif()

    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)

    set(should_append "")
    set(config_suffix "")
    set(is_first_config "1")
    if(is_multi_config)
        list(GET CMAKE_CONFIGURATION_TYPES 0 first_config_type)

        # The non-genex properties should only go to the first config file.
        set(is_first_config "$<CONFIG:${first_config_type}>")

        set(config_suffix "$<$<NOT:${is_first_config}>:-$<CONFIG>>")
        # If the generated file belongs to the 'main' config type, we should set property
        # but not append it.
        string(JOIN "" should_append
            "$<$<NOT:${is_first_config}>: APPEND>")
    endif()

    foreach(target IN LISTS arg_TARGETS)
        set(output_file_base_name "${arg_EXPORT_NAME_PREFIX}ExtraProperties")
        set(file_name "${output_file_base_name}${config_suffix}.cmake")
        qt_path_join(output_file "${arg_CONFIG_INSTALL_DIR}" "${file_name}")

        if(NOT IS_ABSOLUTE "${output_file}")
            qt_path_join(output_file "${QT_BUILD_DIR}" "${output_file}")
        endif()

        set(target_name "${QT_CMAKE_EXPORT_NAMESPACE}::${target}")

        # Common genex helpers.
        string(JOIN "" set_property_begin "set_property(TARGET "
            "${target_name}${should_append} PROPERTY "
        )
        set(set_property_end ")")
        set(set_property_glue "${set_property_end}\n${set_property_begin}")
        set(t_prop "TARGET_PROPERTY:${target}")

        # Handle the properties that contain genexes.
        set(property_list
            "$<GENEX_EVAL:$<${t_prop},_qt_export_custom_properties_content>>")
        set(property_has_values "$<BOOL:${property_list}>")
        string(CONCAT set_property_content
            "${set_property_begin}"
            "$<JOIN:${property_list},${set_property_glue}>"
            "${set_property_end}")

        string(CONCAT set_property_content_conditional
            "$<${property_has_values}:"
            "\n${set_property_content}"
            ">")

        # We need to ensure the no genexes content only gets added to the first config file.
        set(property_no_genexes_list
            "$<GENEX_EVAL:$<${t_prop},_qt_export_custom_properties_no_genexes_content>>")
        set(property_no_genexes_has_values "$<BOOL:${property_no_genexes_list}>")
        string(CONCAT property_no_genexes_has_values_and_first_config
            "$<AND:${property_no_genexes_has_values},${is_first_config}>")

        string(CONCAT set_property_no_genexes_content
            "${set_property_begin}"
            "$<JOIN:${property_no_genexes_list},${set_property_glue}>"
            "${set_property_end}")

        string(CONCAT set_property_no_genexes_content_conditional
            "$<${property_no_genexes_has_values_and_first_config}:"
            "\n${set_property_no_genexes_content}"
            ">")

        # Final content is generated if at least one genex-carrying property has a value,
        # or if we are in the first config and at least one no-genex property has a value.
        set(content_available_condition
            "$<OR:${property_has_values},${property_no_genexes_has_values_and_first_config}>")

        set(config_includes_string "")
        if(is_multi_config)
            set(config_includes "")
            foreach(config IN LISTS CMAKE_CONFIGURATION_TYPES)
                if(NOT first_config_type STREQUAL config)
                    set(include_file_name
                        "${output_file_base_name}-${config}.cmake")
                    list(APPEND config_includes
                        "include(\"\${CMAKE_CURRENT_LIST_DIR}/${include_file_name}\")")
                endif()
            endforeach()
            list(JOIN config_includes "\n" config_includes_string)
            set(config_includes_string
                "\n$<${is_first_config}:${config_includes_string}>")

            # Config includes should be included if we have properties with genexes, which are
            # config specific.
            string(CONCAT config_includes_string_conditional
                "$<${property_has_values}:"
                "${config_includes_string}"
                ">")
        endif()

        string(CONCAT final_content
            "$<${content_available_condition}:"
            "${set_property_content_conditional}"
            "${set_property_no_genexes_content_conditional}"
            "${config_includes_string_conditional}"
            ">")

        file(GENERATE OUTPUT "${output_file}"
            CONTENT "${final_content}"
            CONDITION "${content_available_condition}"
        )

        qt_install(FILES "$<${content_available_condition}:${output_file}>"
            DESTINATION "${arg_CONFIG_INSTALL_DIR}"
            COMPONENT Devel
        )
    endforeach()
endfunction()

# A small wrapper for adding the Platform target, and a building block for the PlatformXInternal
# and GlobalConfig INTERFACE targets to apply common options.
function(qt_internal_add_platform_target target)
    _qt_internal_add_library("${target}" INTERFACE)
    qt_internal_add_target_aliases("${target}")

    # The platform targets should never be promoted to global via the
    # _qt_internal_promote_3rd_party_target_to_global command.
    set_property(TARGET "${target}" PROPERTY _qt_should_skip_3rd_party_global_promotion TRUE)
    set_property(TARGET "${target}" APPEND PROPERTY EXPORT_PROPERTIES
        "_qt_should_skip_3rd_party_global_promotion")
endfunction()

# A small wrapper for adding the PlatformXInternal and GlobalConfig INTERFACE targets to apply
# common options.
# They can't be added via qt_internal_add_module, because it automatically links to the
# PlatformInternal targets creating a cyclic dependency.
function(qt_internal_add_platform_internal_target target)
    qt_internal_add_platform_target("${target}")
    qt_internal_mark_as_internal_library("${target}")

    qt_internal_add_sbom("${target}"
        SBOM_ENTITY_TYPE QT_MODULE
    )
endfunction()

# A small wrapper for passing --dynamic-list to the linker. It will ensure that the symbols will
# be mangled when qt is compiled in a namespace
function(qt_internal_apply_dynamic_list_linker_flags target dynlist_template)
    if(NOT QT_FEATURE_reduce_relocations)
        return()
    endif()

    string(REPLACE ".in" "" dynlist_file "${dynlist_template}")
    set(dynlist_file_abspath "${CMAKE_CURRENT_BINARY_DIR}/${dynlist_file}")

    if(QT_NAMESPACE)
        set(QT_NAMESPACE_PREFIX "${QT_NAMESPACE}::")
        set(QT_NAMESPACE_MANGLE_SUFFIX "_${QT_NAMESPACE}")
    else()
        set(QT_NAMESPACE_PREFIX "")
        set(QT_NAMESPACE_MANGLE_SUFFIX "")
    endif()

    configure_file(
        "${dynlist_template}"
        "${dynlist_file_abspath}"
    )

    qt_internal_extend_target(${target}
        SOURCES
            "${dynlist_template}"
            "${dynlist_file_abspath}"
    )

    target_link_options(${target} PRIVATE "LINKER:--dynamic-list=${dynlist_file_abspath}")
endfunction()
