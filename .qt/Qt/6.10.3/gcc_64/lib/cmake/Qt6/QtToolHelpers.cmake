# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# This function is used to define a "Qt tool", such as moc, uic or rcc.
#
# USER_FACING can be passed to mark the tool as a program that is supposed to be
# started directly by users.
#
# We must pass this function a target name obtained from
# qt_get_tool_target_name like this:
#     qt_get_tool_target_name(target_name my_tool)
#     qt_internal_add_tool(${target_name})
#
# Option Arguments:
#     INSTALL_VERSIONED_LINK
#         Prefix build only. On installation, create a versioned hard-link of the installed file.
#         E.g. create a link of "bin/qmake6" to "bin/qmake".
#     TRY_RUN
#         On Windows, it creates a helper batch script that tests whether the tool can be executed
#         successfully or not. If not, build halts and an error will be show, with tips on what
#         might be cause, and how to fix it. TRY_RUN is disabled when cross-compiling.
#     TRY_RUN_FLAGS
#         Command line flags that are going to be passed to the tool for testing its correctness.
#         If no flags were given, we default to `-v`.
#     REQUIRED_FOR_DOCS
#         Specifies that the built tool is required to generate documentation. Examples are qdoc,
#         and qvkgen (because they participate in header file generation, which are needed for
#         documentation generation).
#
# One-value Arguments:
#     EXTRA_CMAKE_FILES
#         List of additional CMake files that will be installed alongside the tool's exported CMake
#         files.
#     EXTRA_CMAKE_INCLUDES
#         List of files that will be included in the Qt6${module}Tools.cmake file.
#         Also see TOOLS_TARGET.
#     INSTALL_DIR
#         Takes a path, relative to the install prefix, like INSTALL_LIBEXECDIR.
#         If this argument is omitted, the default is INSTALL_BINDIR.
#     TOOLS_TARGET
#         Specifies the module this tool belongs to. The module's Qt6${module}Tools.cmake file
#         will then contain targets for this tool.
#     CORE_LIBRARY
#         The argument accepts 'Bootstrap' or 'None' values. If the argument value is set to
#         'Bootstrap' the Qt::Bootstrap library is linked to the executable instead of Qt::Core.
#         The 'None' value points that core library is not necessary and avoids linking neither
#         Qt::Core or Qt::Bootstrap libraries. Otherwise the Qt::Core library will be publicly
#         linked to the executable target by default.
function(qt_internal_add_tool target_name)
    qt_tool_target_to_name(name ${target_name})
    set(option_keywords
        NO_INSTALL
        USER_FACING
        INSTALL_VERSIONED_LINK
        EXCEPTIONS
        NO_UNITY_BUILD
        TRY_RUN
        REQUIRED_FOR_DOCS
        ${__qt_internal_sbom_optional_args}
    )
    set(one_value_keywords
        TOOLS_TARGET
        INSTALL_DIR
        CORE_LIBRARY
        TRY_RUN_FLAGS
        ${__default_target_info_args}
        ${__qt_internal_sbom_single_args}
    )
    set(multi_value_keywords
        EXTRA_CMAKE_FILES
        EXTRA_CMAKE_INCLUDES
        PUBLIC_LIBRARIES
        ${__default_private_args}
        ${__qt_internal_sbom_multi_args}
    )

    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${option_keywords}"
        "${one_value_keywords}"
        "${multi_value_keywords}")
    _qt_internal_validate_all_args_are_parsed(arg)

    qt_internal_find_tool(will_build_tools ${target_name} "${arg_TOOLS_TARGET}")

    if(NOT will_build_tools)
        return()
    endif()

    set(disable_autogen_tools "${arg_DISABLE_AUTOGEN_TOOLS}")
    set(corelib "")
    if(arg_CORE_LIBRARY STREQUAL "Bootstrap" OR arg_CORE_LIBRARY STREQUAL "None")
        set(corelib CORE_LIBRARY ${arg_CORE_LIBRARY})
        list(APPEND disable_autogen_tools "uic" "moc" "rcc")
    endif()

    set(exceptions "")
    if(arg_EXCEPTIONS)
        set(exceptions EXCEPTIONS)
    endif()

    set(install_dir "${INSTALL_BINDIR}")
    if(arg_INSTALL_DIR)
        set(install_dir "${arg_INSTALL_DIR}")
    endif()

    set(output_dir "${QT_BUILD_DIR}/${install_dir}")

    if(arg_PUBLIC_LIBRARIES)
        message(WARNING
            "qt_internal_add_tool's PUBLIC_LIBRARIES option is deprecated, and will be "
            "removed in a future Qt version. Use the LIBRARIES option instead.")
    endif()

    if(arg_NO_UNITY_BUILD)
        set(arg_NO_UNITY_BUILD "NO_UNITY_BUILD")
    else()
        set(arg_NO_UNITY_BUILD "")
    endif()

    _qt_internal_forward_function_args(
        FORWARD_PREFIX arg
        FORWARD_OUT_VAR add_executable_args
        FORWARD_SINGLE
            TARGET_COMPANY
            TARGET_COPYRIGHT
            TARGET_DESCRIPTION
            TARGET_PRODUCT
            TARGET_VERSION
    )

    qt_internal_add_executable("${target_name}"
        OUTPUT_DIRECTORY "${output_dir}"
        ${exceptions}
        NO_INSTALL
        ${arg_NO_UNITY_BUILD}
        SOURCES ${arg_SOURCES}
        NO_PCH_SOURCES ${arg_NO_PCH_SOURCES}
        NO_UNITY_BUILD_SOURCES ${arg_NO_UNITY_BUILD_SOURCES}
        INCLUDE_DIRECTORIES
            ${arg_INCLUDE_DIRECTORIES}
        DEFINES
            ${arg_DEFINES}
            ${deprecation_define}
        ${corelib}
        LIBRARIES
            ${arg_LIBRARIES}
            ${arg_PUBLIC_LIBRARIES}
            Qt::PlatformToolInternal
        COMPILE_OPTIONS ${arg_COMPILE_OPTIONS}
        LINK_OPTIONS ${arg_LINK_OPTIONS}
        MOC_OPTIONS ${arg_MOC_OPTIONS}
        DISABLE_AUTOGEN_TOOLS ${disable_autogen_tools}
        ${add_executable_args}
        # If you are putting anything after these, make sure that
        # qt_set_target_info_properties knows how to process them
    )
    qt_internal_add_target_aliases("${target_name}")
    qt_internal_adjust_main_config_runtime_output_dir("${target_name}" "${output_dir}")

    if (WIN32)
        _qt_internal_generate_longpath_win32_rc_file_and_manifest("${target_name}")
    endif()

    set_target_properties(${target_name} PROPERTIES
        _qt_package_version "${PROJECT_VERSION}"
    )
    set_property(TARGET ${target_name}
                 APPEND PROPERTY
                 EXPORT_PROPERTIES "_qt_package_version")

    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.19.0"
            AND QT_FEATURE_debug_and_release
            AND is_multi_config)
        set_property(TARGET "${target_name}"
            PROPERTY EXCLUDE_FROM_ALL "$<NOT:$<CONFIG:${QT_MULTI_CONFIG_FIRST_CONFIG}>>")
    endif()

    if (NOT target_name STREQUAL name)
        set_target_properties(${target_name} PROPERTIES
            OUTPUT_NAME ${name}
            EXPORT_NAME ${name}
        )
    endif()

    if(TARGET host_tools)
        add_dependencies(host_tools "${target_name}")
        if(arg_REQUIRED_FOR_DOCS)
            add_dependencies(doc_tools "${target_name}")
        endif()
        if(arg_CORE_LIBRARY STREQUAL "Bootstrap")
            add_dependencies(bootstrap_tools "${target_name}")
        endif()
    endif()

    if(arg_EXTRA_CMAKE_FILES)
        set_target_properties(${target_name} PROPERTIES
            EXTRA_CMAKE_FILES "${arg_EXTRA_CMAKE_FILES}"
        )
    endif()

    if(arg_EXTRA_CMAKE_INCLUDES)
        set_target_properties(${target_name} PROPERTIES
            EXTRA_CMAKE_INCLUDES "${arg_EXTRA_CMAKE_INCLUDES}"
            )
    endif()

    if(arg_USER_FACING)
        set_property(GLOBAL APPEND PROPERTY QT_USER_FACING_TOOL_TARGETS ${target_name})
    endif()

    if(QT_GENERATE_SBOM)
        set(sbom_args "")
        list(APPEND sbom_args DEFAULT_SBOM_ENTITY_TYPE QT_TOOL)
    endif()

    if(NOT arg_NO_INSTALL AND arg_TOOLS_TARGET)
        set(will_install TRUE)
    else()
        set(will_install FALSE)
        if(QT_GENERATE_SBOM)
            list(APPEND sbom_args NO_INSTALL)
        endif()
    endif()

    if(will_install)
        # Assign a tool to an export set, and mark the module to which the tool belongs.
        qt_internal_append_known_modules_with_tools("${arg_TOOLS_TARGET}")

        # Also append the tool to the module list.
        qt_internal_append_known_module_tool("${arg_TOOLS_TARGET}" "${target_name}")

        qt_get_cmake_configurations(cmake_configs)

        set(install_initial_call_args
            EXPORT "${INSTALL_CMAKE_NAMESPACE}${arg_TOOLS_TARGET}ToolsTargets"
            COMPONENT host_tools
        )

        foreach(cmake_config ${cmake_configs})
            qt_get_install_target_default_args(
                OUT_VAR install_targets_default_args
                OUT_VAR_RUNTIME runtime_install_destination
                RUNTIME "${install_dir}"
                CMAKE_CONFIG "${cmake_config}"
                ALL_CMAKE_CONFIGS ${cmake_configs})

            # Make installation optional for targets that are not built by default in this config
            if(QT_FEATURE_debug_and_release
                    AND NOT (cmake_config STREQUAL QT_MULTI_CONFIG_FIRST_CONFIG))
                set(install_optional_arg OPTIONAL)
              else()
                unset(install_optional_arg)
            endif()

            if(QT_GENERATE_SBOM)
                _qt_internal_sbom_append_multi_config_aware_single_arg_option(
                    RUNTIME_PATH
                    "${runtime_install_destination}"
                    "${cmake_config}"
                    sbom_args
                )
            endif()

            qt_install(TARGETS "${target_name}"
                       ${install_initial_call_args}
                       ${install_optional_arg}
                       CONFIGURATIONS ${cmake_config}
                       ${install_targets_default_args})
            unset(install_initial_call_args)
        endforeach()

        if(arg_INSTALL_VERSIONED_LINK)
            qt_internal_install_versioned_link(WORKING_DIRECTORY "${install_dir}"
            TARGETS "${target_name}")
        endif()

        qt_apply_rpaths(TARGET "${target_name}" INSTALL_PATH "${install_dir}" RELATIVE_RPATH)
        qt_internal_apply_staging_prefix_build_rpath_workaround()
    endif()

    if(arg_TRY_RUN AND WIN32 AND NOT CMAKE_CROSSCOMPILING)
        if(NOT arg_TRY_RUN_FLAGS)
            set(arg_TRY_RUN_FLAGS "-v")
        endif()
        _qt_internal_add_try_run_post_build("${target_name}" "${arg_TRY_RUN_FLAGS}")
    endif()

    qt_internal_defer_separate_debug_info("${target_name}"
        SEPARATE_DEBUG_INFO_ARGS
            "${install_dir}"
            QT_EXECUTABLE
    )
    qt_internal_install_pdb_files(${target_name} "${install_dir}")

    if(QT_GENERATE_SBOM)
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

        qt_internal_extend_qt_entity_sbom(${target_name} ${sbom_args})
    endif()

    qt_add_list_file_finalizer(qt_internal_finalize_tool ${target_name})
endfunction()

function(qt_internal_finalize_tool target)
    _qt_internal_finalize_sbom(${target})
endfunction()

function(_qt_internal_add_try_run_post_build target try_run_flags)
    qt_internal_get_upper_case_main_cmake_configuration(main_cmake_configuration)
    get_target_property(target_out_dir ${target}
                        RUNTIME_OUTPUT_DIRECTORY_${main_cmake_configuration})
    get_target_property(target_bin_dir ${target}
                        BINARY_DIR)

    set(try_run_scripts_path "${target_bin_dir}/${target}_try_run.bat")
    # The only reason -h is passed is because some of the tools, e.g., moc
    # wait for an input without any arguments.

    qt_configure_file(OUTPUT "${try_run_scripts_path}"
        CONTENT "@echo off

${target_out_dir}/${target}.exe ${try_run_flags} > nul 2>&1

if \"%errorlevel%\" == \"-1073741515\" (
echo
echo     '${target}' is built successfully, but some of the libraries
echo     necessary for running it are missing. If you are building Qt with
echo     3rdparty libraries, make sure that you add their directory to the
echo     PATH environment variable.
echo
exit /b %errorlevel%
)
echo. > ${target_bin_dir}/${target}_try_run_passed"
        )

    add_custom_command(
        OUTPUT
            ${target_bin_dir}/${target}_try_run_passed
        DEPENDS
            ${target}
        COMMAND
            ${CMAKE_COMMAND} -E env QT_COMMAND_LINE_PARSER_NO_GUI_MESSAGE_BOXES=1
            ${try_run_scripts_path}
        COMMENT
            "Testing ${target} by trying to run it."
        VERBATIM
    )

    add_custom_target(${target}_try_run ALL
                      DEPENDS ${target_bin_dir}/${target}_try_run_passed)
endfunction()

function(qt_export_tools module_name)
    # Bail out when not building tools.
    if(NOT QT_WILL_BUILD_TOOLS)
        return()
    endif()

    # If no tools were defined belonging to this module, don't create a config and targets file.
    if(NOT "${module_name}" IN_LIST QT_KNOWN_MODULES_WITH_TOOLS)
        return()
    endif()

    # The tools target name. For example: CoreTools
    set(target "${module_name}Tools")

    set(path_suffix "${INSTALL_CMAKE_NAMESPACE}${target}")
    qt_path_join(config_build_dir ${QT_CONFIG_BUILD_DIR} ${path_suffix})
    qt_path_join(config_install_dir ${QT_CONFIG_INSTALL_DIR} ${path_suffix})

    # Add the extra cmake statements to make the tool targets global, so it doesn't matter where
    # find_package is called.
    # Also assemble a list of tool targets to expose in the config file for informational purposes.
    set(extra_cmake_statements "")
    set(tool_targets "")
    set(tool_targets_non_prefixed "")

    # List of package dependencies that need be find_package'd when using the Tools package.
    set(package_deps "")
    set(third_party_deps "")

    # Additional cmake files to install
    set(extra_cmake_files "")
    set(extra_cmake_includes "")

    set(first_tool_package_version "")

    foreach(tool_name ${QT_KNOWN_MODULE_${module_name}_TOOLS})
        # Specific tools can have package dependencies.
        # e.g. qtwaylandscanner depends on WaylandScanner (non-qt package).
        get_target_property(extra_packages "${tool_name}" QT_EXTRA_PACKAGE_DEPENDENCIES)
        if(extra_packages)
            foreach(third_party_dep IN LISTS extra_packages)
                list(GET third_party_dep 0 third_party_dep_name)
                list(GET third_party_dep 1 third_party_dep_version)

                # Assume that all tool thirdparty deps are mandatory.
                # TODO: Components are not supported
                list(APPEND third_party_deps
                    "${third_party_dep_name}\\\;FALSE\\\;${third_party_dep_version}\\\;\\\;")
            endforeach()
        endif()

        get_target_property(_extra_cmake_files "${tool_name}" EXTRA_CMAKE_FILES)
        if (_extra_cmake_files)
            foreach(cmake_file ${_extra_cmake_files})
                file(COPY "${cmake_file}" DESTINATION "${config_build_dir}")
                list(APPEND extra_cmake_files "${cmake_file}")
            endforeach()
        endif()

        get_target_property(_extra_cmake_includes "${tool_name}" EXTRA_CMAKE_INCLUDES)
        if(_extra_cmake_includes)
            list(APPEND extra_cmake_includes "${_extra_cmake_includes}")
        endif()

        if (QT_WILL_RENAME_TOOL_TARGETS)
            string(REGEX REPLACE "_native$" "" tool_name ${tool_name})
        endif()
        set(extra_cmake_statements "${extra_cmake_statements}
if(NOT QT_NO_CREATE_TARGETS AND ${INSTALL_CMAKE_NAMESPACE}${target}_FOUND)
    __qt_internal_promote_target_to_global(${INSTALL_CMAKE_NAMESPACE}::${tool_name})
endif()
")
        list(APPEND tool_targets "${QT_CMAKE_EXPORT_NAMESPACE}::${tool_name}")
        list(APPEND tool_targets_non_prefixed "${tool_name}")

        if(NOT first_tool_package_version)
            qt_internal_get_package_version_of_target("${tool_name}" tool_package_version)
            if(tool_package_version)
                set(first_tool_package_version "${tool_package_version}")
            endif()
        endif()
    endforeach()

    string(APPEND extra_cmake_statements
"set(${QT_CMAKE_EXPORT_NAMESPACE}${module_name}Tools_TARGETS \"${tool_targets}\")")

    # Extract package dependencies that were determined in QtPostProcess, but only if ${module_name}
    # is an actual target.
    # module_name can be a non-existent target, if the tool doesn't have an existing associated
    # module, e.g. qtwaylandscanner.
    if(TARGET "${module_name}")
        get_target_property(module_package_deps "${module_name}" _qt_tools_package_deps)
        if(module_package_deps)
            list(APPEND package_deps "${module_package_deps}")
        endif()
    endif()

    # Configure and install dependencies file for the ${module_name}Tools package.
    configure_file(
        "${QT_CMAKE_DIR}/QtModuleToolsDependencies.cmake.in"
        "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}Dependencies.cmake"
        @ONLY
    )

    qt_install(FILES
        "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}Dependencies.cmake"
        DESTINATION "${config_install_dir}"
        COMPONENT Devel
    )

    if(extra_cmake_files)
        qt_install(FILES
           ${extra_cmake_files}
           DESTINATION "${config_install_dir}"
           COMPONENT Devel
        )
    endif()

    # Configure and install the ${module_name}Tools package Config file.
    qt_internal_get_min_new_policy_cmake_version(min_new_policy_version)
    qt_internal_get_max_new_policy_cmake_version(max_new_policy_version)
    configure_package_config_file(
        "${QT_CMAKE_DIR}/QtModuleToolsConfig.cmake.in"
        "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}Config.cmake"
        INSTALL_DESTINATION "${config_install_dir}"
    )

    # There might be Tools packages which don't have a corresponding real module_name target, like
    # WaylandScannerTools.
    # In that case we'll use the package version of the first tool that belongs to that package.
    if(TARGET "${module_name}")
        qt_internal_get_package_version_of_target("${module_name}" tools_package_version)
    elseif(first_tool_package_version)
        set(tools_package_version "${first_tool_package_version}")
    else()
        # This should never happen, because tools_package_version should always have at least some
        # value. Issue an assertion message just in case the pre-condition ever changes.
        set(tools_package_version "${PROJECT_VERSION}")
        if(FEATURE_developer_build)
            message(WARNING
                "Could not determine package version of tools package ${module_name}. "
                "Defaulting to project version ${PROJECT_VERSION}.")
        endif()
    endif()
    message(TRACE
        "Exporting tools package ${module_name}Tools with package version ${tools_package_version}"
        "\n     included targets: ${tool_targets_non_prefixed}")
    write_basic_package_version_file(
        "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}ConfigVersionImpl.cmake"
        VERSION "${tools_package_version}"
        COMPATIBILITY AnyNewerVersion
        ARCH_INDEPENDENT
    )
    qt_internal_write_qt_package_version_file(
        "${INSTALL_CMAKE_NAMESPACE}${target}"
        "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}ConfigVersion.cmake"
    )

    qt_install(FILES
        "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}Config.cmake"
        "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}ConfigVersion.cmake"
        "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}ConfigVersionImpl.cmake"
        DESTINATION "${config_install_dir}"
        COMPONENT Devel
    )

    set(export_name "${INSTALL_CMAKE_NAMESPACE}${target}Targets")
    qt_install(EXPORT "${export_name}"
               NAMESPACE "${QT_CMAKE_EXPORT_NAMESPACE}::"
               DESTINATION "${config_install_dir}")

    qt_internal_export_additional_targets_file(
        TARGETS ${QT_KNOWN_MODULE_${module_name}_TOOLS}
        TARGET_EXPORT_NAMES ${tool_targets}
        EXPORT_NAME_PREFIX ${INSTALL_CMAKE_NAMESPACE}${target}
        CONFIG_INSTALL_DIR "${config_install_dir}")

    # Create versionless targets file.
    configure_file(
        "${QT_CMAKE_DIR}/QtModuleToolsVersionlessTargets.cmake.in"
        "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}VersionlessTargets.cmake"
        @ONLY
    )

    qt_install(FILES
        "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}VersionlessTargets.cmake"
        DESTINATION "${config_install_dir}"
        COMPONENT Devel
    )
endfunction()

# Returns the target name for the tool with the given name.
#
# In most cases, the target name is the same as the tool name.
# If the user specifies to build tools when cross-compiling, then the
# suffix "_native" is appended.
function(qt_get_tool_target_name out_var name)
    if (QT_WILL_RENAME_TOOL_TARGETS)
        set(${out_var} ${name}_native PARENT_SCOPE)
    else()
        set(${out_var} ${name} PARENT_SCOPE)
    endif()
endfunction()

# Returns the tool name for a given tool target.
# This is the inverse of qt_get_tool_target_name.
function(qt_tool_target_to_name out_var target)
    set(name ${target})
    if (QT_WILL_RENAME_TOOL_TARGETS)
        string(REGEX REPLACE "_native$" "" name ${target})
    endif()
    set(${out_var} ${name} PARENT_SCOPE)
endfunction()

# Sets QT_WILL_BUILD_TOOLS if tools will be built and QT_WILL_RENAME_TOOL_TARGETS
# if those tools have replaced naming.
function(qt_check_if_tools_will_be_built)
    # By default, we build our own tools unless we're cross-building or QT_HOST_PATH is set.
    set(need_target_rename FALSE)
    set(require_find_tools FALSE)
    if(CMAKE_CROSSCOMPILING)
        set(will_build_tools FALSE)
        if(QT_FORCE_BUILD_TOOLS)
            set(will_build_tools TRUE)
            set(need_target_rename TRUE)
        endif()
        set(require_find_tools TRUE)
    else()
        if(QT_HOST_PATH)
            set(will_build_tools FALSE)
        else()
            set(will_build_tools TRUE)
        endif()
        if(QT_FORCE_FIND_TOOLS)
            set(will_build_tools FALSE)
            set(require_find_tools TRUE)
        endif()
        if(QT_FORCE_BUILD_TOOLS)
            set(will_build_tools TRUE)
            set(need_target_rename TRUE)
        endif()
    endif()

    set_property(GLOBAL PROPERTY qt_require_find_tools "${require_find_tools}")

    set(QT_WILL_BUILD_TOOLS ${will_build_tools} CACHE INTERNAL "Are tools going to be built" FORCE)
    set(QT_WILL_RENAME_TOOL_TARGETS ${need_target_rename} CACHE INTERNAL
        "Do tool targets need to be renamed" FORCE)
endfunction()

# Use this macro to exit a file or function scope unless we're building tools. This is supposed to
# be called after qt_internal_add_tools() to avoid special-casing operations on imported targets.
macro(qt_internal_return_unless_building_tools)
    if(NOT QT_WILL_BUILD_TOOLS)
        return()
    endif()
endmacro()

# Equivalent of qmake's qtNomakeTools(directory1 directory2).
# If QT_BUILD_TOOLS_BY_DEFAULT is true, then targets within the given directories will be excluded
# from the default 'all' target, as well as from install phase. The private variable is checked by
# qt_internal_add_executable.
function(qt_exclude_tool_directories_from_default_target)
    if(NOT QT_BUILD_TOOLS_BY_DEFAULT)
        set(absolute_path_directories "")
        foreach(directory ${ARGV})
            list(APPEND absolute_path_directories "${CMAKE_CURRENT_SOURCE_DIR}/${directory}")
        endforeach()
        set(__qt_exclude_tool_directories "${absolute_path_directories}" PARENT_SCOPE)
    endif()
endfunction()

function(qt_internal_find_tool out_var target_name tools_target)
    qt_tool_target_to_name(name ${target_name})

    # Handle case when a tool does not belong to a module and it can't be built either (like
    # during a cross-compile).
    if(NOT tools_target AND NOT QT_WILL_BUILD_TOOLS)
        message(FATAL_ERROR "The tool \"${name}\" has not been assigned to a module via"
                            " TOOLS_TARGET (so it can't be found) and it can't be built"
                            " (QT_WILL_BUILD_TOOLS is ${QT_WILL_BUILD_TOOLS}).")
    endif()

    if(NOT CMAKE_CROSSCOMPILING)
        if(QT_INTERNAL_FORCE_FIND_HOST_TOOLS_MODULE_LIST AND
            NOT "${tools_target}" IN_LIST QT_INTERNAL_FORCE_FIND_HOST_TOOLS_MODULE_LIST)
            message(STATUS "Tool '${full_name}' will be built from source.")
            set(${out_var} "TRUE" PARENT_SCOPE)
            return()
        endif()
    endif()

    if(QT_WILL_RENAME_TOOL_TARGETS AND (name STREQUAL target_name))
        message(FATAL_ERROR
            "qt_internal_add_tool must be passed a target obtained from qt_get_tool_target_name.")
    endif()

    set(full_name "${QT_CMAKE_EXPORT_NAMESPACE}::${name}")
    set(imported_tool_target_already_found FALSE)

    # This condition can only be TRUE if a previous find_package(Qt6${tools_target}Tools)
    # was already done. That can happen if QT_FORCE_FIND_TOOLS was ON or we're cross-compiling.
    # In such a case, we need to exit early if we're not going to also build the tools.
    if(TARGET ${full_name})
        get_property(path TARGET ${full_name} PROPERTY LOCATION)
        message(STATUS "Tool '${full_name}' was found at ${path}.")
        set(imported_tool_target_already_found TRUE)
        if(NOT QT_WILL_BUILD_TOOLS)
            set(${out_var} "FALSE" PARENT_SCOPE)
            return()
        endif()
    endif()

    # We need to search for the host Tools package when doing a cross-build
    # or when QT_FORCE_FIND_TOOLS is ON.
    # As an optimiziation, we don't search for the package one more time if the target
    # was already brought into scope from a previous find_package.
    set(search_for_host_package FALSE)
    if(NOT QT_WILL_BUILD_TOOLS OR QT_WILL_RENAME_TOOL_TARGETS)
        set(search_for_host_package TRUE)
    endif()
    if(search_for_host_package AND NOT imported_tool_target_already_found)
        set(tools_package_name "${INSTALL_CMAKE_NAMESPACE}${tools_target}Tools")
        message(STATUS "Searching for tool '${full_name}' in package ${tools_package_name}.")

        # Create the tool targets, even if QT_NO_CREATE_TARGETS is set.
        # Otherwise targets like Qt6::moc are not available in a top-level cross-build.
        set(BACKUP_QT_NO_CREATE_TARGETS ${QT_NO_CREATE_TARGETS})
        set(QT_NO_CREATE_TARGETS OFF)

        # When cross-compiling, we want to search for Tools packages in QT_HOST_PATH.
        # To do that, we override CMAKE_PREFIX_PATH and CMAKE_FIND_ROOT_PATH.
        #
        # We don't use find_package + PATHS option because any recursive find_dependency call
        # inside a Tools package would not inherit the initial PATHS value given.
        # TODO: Potentially we could set a global __qt_cmake_host_dir var like we currently
        # do with _qt_cmake_dir in Qt6Config and change all our host tool find_package calls
        # everywhere to specify that var in PATHS.
        #
        # Note though that due to path rerooting issue in
        # https://gitlab.kitware.com/cmake/cmake/-/issues/21937
        # we have to append a lib/cmake suffix to CMAKE_PREFIX_PATH so the value does not get
        # rerooted on top of CMAKE_FIND_ROOT_PATH.
        # Use QT_HOST_PATH_CMAKE_DIR for the suffix when available (it would be set by
        # the qt.toolchain.cmake file when building other repos or given by the user when
        # configuring qtbase) or derive it from from the Qt6HostInfo package which is
        # found in QtSetup.
        set(${tools_package_name}_BACKUP_CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH})
        set(${tools_package_name}_BACKUP_CMAKE_FIND_ROOT_PATH "${CMAKE_FIND_ROOT_PATH}")
        if(QT_HOST_PATH_CMAKE_DIR)
            set(qt_host_path_cmake_dir_absolute "${QT_HOST_PATH_CMAKE_DIR}")
        elseif(${INSTALL_CMAKE_NAMESPACE}HostInfo_DIR)
            get_filename_component(qt_host_path_cmake_dir_absolute
                "${${INSTALL_CMAKE_NAMESPACE}HostInfo_DIR}/.." ABSOLUTE)
        else()
            # This should never happen, serves as an assert.
            message(FATAL_ERROR
                "Neither QT_HOST_PATH_CMAKE_DIR nor "
                "${INSTALL_CMAKE_NAMESPACE}HostInfo_DIR available.")
        endif()
        set(CMAKE_PREFIX_PATH "${qt_host_path_cmake_dir_absolute}")

        # Look for tools in additional host Qt installations. This is done for conan support where
        # we have separate installation prefixes per package. For simplicity, we assume here that
        # all host Qt installations use the same value of INSTALL_LIBDIR.
        if(DEFINED QT_ADDITIONAL_HOST_PACKAGES_PREFIX_PATH)
            file(RELATIVE_PATH rel_host_cmake_dir "${QT_HOST_PATH}"
                "${qt_host_path_cmake_dir_absolute}")
            foreach(host_path IN LISTS QT_ADDITIONAL_HOST_PACKAGES_PREFIX_PATH)
                set(host_cmake_dir "${host_path}/${rel_host_cmake_dir}")
                list(PREPEND CMAKE_PREFIX_PATH "${host_cmake_dir}")
            endforeach()

            list(PREPEND CMAKE_FIND_ROOT_PATH "${QT_ADDITIONAL_HOST_PACKAGES_PREFIX_PATH}")
        endif()
        list(PREPEND CMAKE_FIND_ROOT_PATH "${QT_HOST_PATH}")

        find_package(
            ${tools_package_name}
            ${PROJECT_VERSION}
            NO_PACKAGE_ROOT_PATH
            NO_CMAKE_ENVIRONMENT_PATH
            NO_SYSTEM_ENVIRONMENT_PATH
            NO_CMAKE_PACKAGE_REGISTRY
            NO_CMAKE_SYSTEM_PATH
            NO_CMAKE_SYSTEM_PACKAGE_REGISTRY)

        # Restore backups.
        set(CMAKE_FIND_ROOT_PATH "${${tools_package_name}_BACKUP_CMAKE_FIND_ROOT_PATH}")
        set(CMAKE_PREFIX_PATH "${${tools_package_name}_BACKUP_CMAKE_PREFIX_PATH}")
        set(QT_NO_CREATE_TARGETS ${BACKUP_QT_NO_CREATE_TARGETS})

        if(${${tools_package_name}_FOUND} AND TARGET ${full_name})
            # Even if the tool is already visible, make sure that our modules remain associated
            # with the tools.
            qt_internal_append_known_modules_with_tools("${tools_target}")
            get_property(path TARGET ${full_name} PROPERTY LOCATION)
            message(STATUS "${full_name} was found at ${path} using package ${tools_package_name}.")
            if (NOT QT_FORCE_BUILD_TOOLS)
                set(${out_var} "FALSE" PARENT_SCOPE)
                return()
            endif()
        endif()
    endif()

    get_property(require_find_tools GLOBAL PROPERTY qt_require_find_tools)
    if(require_find_tools AND NOT TARGET ${full_name})
        if(${${tools_package_name}_FOUND})
            set(pkg_found_msg "")
            string(APPEND pkg_found_msg
                "the ${tools_package_name} package, but the package did not contain the tool. "
                "Make sure that the host module ${tools_target} was built with all features "
                "enabled (no explicitly disabled tools).")
        else()
            set(pkg_found_msg "")
            string(APPEND pkg_found_msg
                "the ${tools_package_name} package, but the package could not be found. "
                "Make sure you have built and installed the host ${tools_target} module, "
                "which will ensure the creation of the ${tools_package_name} package.")
        endif()
        message(FATAL_ERROR
            "Failed to find the host tool \"${full_name}\". It is part of "
            ${pkg_found_msg})
    endif()

    if(QT_WILL_BUILD_TOOLS)
        message(STATUS "Tool '${full_name}' will be built from source.")
    endif()
    set(${out_var} "TRUE" PARENT_SCOPE)
endfunction()

# This function adds an internal tool that should be compiled at configure time.
#     TOOLS_TARGET
#         Specifies the module this tool belongs to. The Qt6${TOOLS_TARGET}Tools module
#         will then expose targets for this tool. Ignored if NO_INSTALL is set.
function(qt_internal_add_configure_time_tool target_name)
    set(one_value_args INSTALL_DIRECTORY TOOLS_TARGET CONFIG)
    set(multi_value_args)
    set(option_args NO_INSTALL)
    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${option_args}" "${one_value_args}" "${multi_value_args}")

    qt_internal_find_tool(will_build_tools ${target_name} "${arg_TOOLS_TARGET}")
    if(NOT will_build_tools)
        return()
    endif()

    qt_tool_target_to_name(name ${target_name})
    set(extra_args "")
    if(arg_NO_INSTALL OR NOT arg_TOOLS_TARGET)
        list(APPEND extra_args "NO_INSTALL")
    else()
        set(install_dir "${INSTALL_BINDIR}")
        if(arg_INSTALL_DIRECTORY)
            set(install_dir "${arg_INSTALL_DIRECTORY}")
        endif()
        set(extra_args "INSTALL_DIRECTORY" "${install_dir}")
    endif()

    if(arg_CONFIG)
        set(tool_config "${arg_CONFIG}")
    elseif(QT_MULTI_CONFIG_FIRST_CONFIG)
        set(tool_config "${arg_QT_MULTI_CONFIG_FIRST_CONFIG}")
    else()
        set(tool_config "${CMAKE_BUILD_TYPE}")
    endif()

    string(REPLACE "\\\;" "\\\\\\\;" unparsed_arguments "${arg_UNPARSED_ARGUMENTS}")
    qt_internal_add_configure_time_executable(${target_name}
        OUTPUT_NAME ${name}
        CONFIG ${tool_config}
        ${extra_args}
        ${unparsed_arguments}
    )

    if(TARGET host_tools)
        add_dependencies(host_tools "${target_name}_build")
    endif()

    if(NOT arg_NO_INSTALL AND arg_TOOLS_TARGET)
        qt_internal_add_targets_to_additional_targets_export_file(
            TARGETS ${target_name}
            TARGET_EXPORT_NAMES ${QT_CMAKE_EXPORT_NAMESPACE}::${name}
            EXPORT_NAME_PREFIX ${INSTALL_CMAKE_NAMESPACE}${arg_TOOLS_TARGET}Tools
        )
    endif()
endfunction()
