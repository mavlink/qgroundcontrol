# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#
# Q6QmlMacros
#

set(__qt_qml_macros_module_base_dir "${CMAKE_CURRENT_LIST_DIR}" CACHE INTERNAL "")

# Install support uses the CMAKE_INSTALL_xxxDIR variables. Include this here
# so that it is more likely to get pulled in earlier at a higher level, and also
# to avoid re-including it many times later
include(GNUInstallDirs)
_qt_internal_add_deploy_support("${CMAKE_CURRENT_LIST_DIR}/Qt6QmlDeploySupport.cmake")

# This function is used to parse DEPENDENCY and IMPORT entries passed to qt_add_qml_module
# It takes the entry as a mandatory argument, and then sets the following
# user provided properties in the callers scope
# OUTPUT_URI <property>: the URI of the module; mandatory argument
# OUTPUT_VERSION <property>: the requested version of the module; will potentially be empty; mandatory
# OUTPUT_MODULE_LOCATION <property>: the folder in which the module is located; optional; can be used to extract potential import path
# OUTPUT_MODULE_TARGET <property>: the target corresponding to the module

function(_qt_internal_parse_qml_module_dependency dependency was_marked_as_target)
    set(args_option "")
    set(args_single OUTPUT_URI OUTPUT_VERSION OUTPUT_MODULE_LOCATION OUTPUT_MODULE_TARGET)
    set(args_multi QML_FILES IMPORT_PATHS)

    cmake_parse_arguments(PARSE_ARGV 2 arg
        "${args_option}" "${args_single}" "${args_multi}"
    )
    if(arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown/unexpected arguments: ${arg_UNPARSED_ARGUMENTS}")
    endif()

    if(NOT arg_OUTPUT_URI)
        message(FATAL_ERROR "Missing output URI variable")
    endif()
    if(NOT arg_OUTPUT_VERSION)
        message(FATAL_ERROR "Missing output version variable")
    endif()

    set(dep_version "")
    set(dep_target_or_uri "")
    string(FIND "${dependency}" "/" slash_position REVERSE)
    if(slash_position EQUAL -1)
        set(dep_target_or_uri "${dependency}")
    else()
        string(SUBSTRING "${dependency}" 0 ${slash_position} dep_module)
        math(EXPR slash_position "${slash_position} + 1")
        string(SUBSTRING "${dependency}" ${slash_position} -1 dep_version)
        if(NOT dep_version MATCHES "^([0-9]+(\\.[0-9]+)?|auto)$")
            message(FATAL_ERROR
                "Invalid module dependency version number. "
                "Expected 'VersionMajor', 'VersionMajor.VersionMinor' or 'auto'."
            )
        endif()
        set(dep_target_or_uri "${dep_module}")
    endif()
    if("${was_marked_as_target}" AND NOT TARGET ${dep_target_or_uri})
        message(FATAL_ERROR "Argument ${dep_target_or_uri} is not a target!")
    endif()
    if("${was_marked_as_target}")
        qt6_query_qml_module(${dep_target_or_uri}
            URI dependency_uri
            QMLDIR qmldir_location
        )
        set(dep_module ${dependency_uri})
    else()
        set(dep_module "${dep_target_or_uri}")
    endif()
    set(${arg_OUTPUT_URI} ${dep_module} PARENT_SCOPE)
    set(${arg_OUTPUT_VERSION} ${dep_version} PARENT_SCOPE)
    if(arg_OUTPUT_MODULE_LOCATION)
        if(was_marked_as_target)
            if(NOT qmldir_location)
                message(FATAL_ERROR "module has no qmldir! Given target was ${dep_target_or_uri}")
            endif()
            set(module_location "${qmldir_location}")
            string(REGEX MATCHALL "\\." matches "${dependency_uri}")
            list(LENGTH matches go_up_count)
            # inclusive, which is what we want here: go up once for the qmldir,
            # and then once per separated component
            foreach(i RANGE ${go_up_count})
                get_filename_component(module_location "${module_location}" DIRECTORY)
            endforeach()
            set(${arg_OUTPUT_MODULE_LOCATION} "${module_location}" PARENT_SCOPE)
        else()
            set(${arg_OUTPUT_MODULE_LOCATION} "NOTFOUND" PARENT_SCOPE)
        endif()
    endif()
    if (arg_OUTPUT_MODULE_TARGET AND was_marked_as_target)
        set(${arg_OUTPUT_MODULE_TARGET} "${dep_target_or_uri}" PARENT_SCOPE)
    else()
        set(${arg_OUTPUT_MODULE_TARGET} "" PARENT_SCOPE)
    endif()
endfunction()

# Helper function to get the import path needed to find the '${target}' Qml module.
# TLDR: Subtracts the qml module's TARGET_PATH from its qmldir location.
function(_qt_internal_get_qml_module_import_path target out_var)
    # To get the module import path, we need to subtract the target path from the qmldir
    # dir path.
    qt6_query_qml_module("${target}" QMLDIR qmldir_location)
    get_filename_component(module_location "${qmldir_location}" DIRECTORY)

    get_target_property(module_target_path "${target}" _qt_qml_module_target_path)
    string(REPLACE "/" ";" module_target_path_parts "${module_target_path}")

    # Go up one subdir from the module location, for each subdir in the target path.
    set(module_import_path "${module_location}")
    foreach(part IN LISTS module_target_path_parts)
        get_filename_component(module_import_path "${module_import_path}" DIRECTORY)
    endforeach()

    set(${out_var} "${module_import_path}" PARENT_SCOPE)
endfunction()

function(_qt_internal_write_qmldir_part target qt_cmake_export_namespace)
    set(qt_all_qml_output_dirs "")

    # _qt_internal_write_qmldir_part is deferred to be called in the root
    # CMAKE_BINARY_DIR. If find_package(Qt6) is not called in the root of the project (like in the
    # Qt Creator super repo), QT_CMAKE_EXPORT_NAMESPACE will not be defined.
    # Manually define it here, based on the value passed to the function, from a scope where it
    # is available.
    set(QT_CMAKE_EXPORT_NAMESPACE "${qt_cmake_export_namespace}")

    get_target_property(target_binary_dir "${target}" BINARY_DIR)
    set(config_infix "")
    get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if(is_multi_config)
        set(config_infix "_$<CONFIG>")
    endif()

    # Location for creating the partial qt conf files.
    set(qt_part_conf_file "${target_binary_dir}/.qt/qtconfs${config_infix}/${target}_qt.part.conf")

    # Put the final qt.conf file next to the target binary, depending on the where the platform
    # expects it.
    get_target_property(is_apple_bundle "${target}" MACOSX_BUNDLE)

    if(APPLE AND is_apple_bundle)
        if(NOT CMAKE_SYSTEM_NAME OR CMAKE_SYSTEM_NAME STREQUAL "Darwin")
            # macOS has a Resources subdir in app bundles.
            set(resources_suffix "/Resources")
        else()
            # Other Apple platforms don't.
            set(resources_suffix "")
        endif()
        set(effective_outdir "$<TARGET_BUNDLE_CONTENT_DIR:${target}>${resources_suffix}")
    else()
        set(effective_outdir "$<TARGET_FILE_DIR:${target}>")
    endif()
    set(qt_conf_file_final "${effective_outdir}/qt.conf")

    get_target_property(dependency_targets "${target}" QT_QML_DEPENDENT_QML_MODULE_TARGETS)

    # Add the current executable's qml module location as an import path as well.
    # Helps finding the executable qml module for macOS app bundles.
    _qt_internal_get_qml_module_import_path("${target}" own_module_import_path)
    list(APPEND qt_all_qml_output_dirs ${own_module_import_path})

    get_directory_property(counter
         DIRECTORY ${PROJECT_SOURCE_DIR}
         QT_QMLDIR_DEFERRED_WRITEOUT_COUNTER
    )
    math(EXPR counter "${counter} - 1")
    set_property(
        DIRECTORY ${PROJECT_SOURCE_DIR}
        PROPERTY QT_QMLDIR_DEFERRED_WRITEOUT_COUNTER "${counter}"
    )
    if(dependency_targets)
        foreach(dep_target ${dependency_targets})
            _qt_internal_get_qml_module_import_path("${dep_target}" module_import_path)
            list(APPEND qt_all_qml_output_dirs ${module_import_path})
        endforeach()
        if (qt_all_qml_output_dirs)
            list(REMOVE_DUPLICATES qt_all_qml_output_dirs)
            # TODO: this will break for paths containing a newline character..
            list(JOIN qt_all_qml_output_dirs "\n"  qt_all_qml_output_dirs)

            file(GENERATE
                OUTPUT "${qt_part_conf_file}"
                CONTENT "${qt_all_qml_output_dirs}\n"
            )
            set_property(
                DIRECTORY ${PROJECT_SOURCE_DIR}
                APPEND
                PROPERTY QT_QMLDIR_ALL_PARTS "${qt_part_conf_file}"
            )
            set_source_files_properties(
                "${qt_part_conf_file}"
                PROPERTIES _qt_qml_final_qt_conf_path "${qt_conf_file_final}"
            )
            set_property(
                DIRECTORY ${PROJECT_SOURCE_DIR}
                APPEND
                PROPERTY QT_QMLDIR_DEFERRED_WRITEOUT_ALL_TARGETS "${target}")
        endif()
    endif()

    if (counter EQUAL 0)
        # counter reached zero, all relevant finalizers are done
        get_directory_property(all_parts DIRECTORY ${PROJECT_SOURCE_DIR} QT_QMLDIR_ALL_PARTS)
        if (NOT all_parts)
            return()
        endif()
        list(JOIN all_parts "\n"  all_parts_string)

        set(qt_conf_list_path "${PROJECT_BINARY_DIR}/.qt/qtconf_list${config_infix}")
        file(GENERATE
            OUTPUT "${qt_conf_list_path}"
            CONTENT "${all_parts_string}\n"
        )

        # Collect the mappings from partial qt.conf files to final qt.conf file location.
        set(all_parts_final_conf_paths "")
        foreach(qt_conf_part_file IN LISTS all_parts)
            get_source_file_property(final_qt_conf_path "${qt_conf_part_file}"
                _qt_qml_final_qt_conf_path)
            string(APPEND all_parts_final_conf_paths "${qt_conf_part_file};${final_qt_conf_path}\n")
        endforeach()
        set(qt_conf_final_paths_path "${PROJECT_BINARY_DIR}/.qt/qtconf_path_list${config_infix}")
        file(GENERATE
            OUTPUT "${qt_conf_final_paths_path}"
            CONTENT "${all_parts_final_conf_paths}"
        )

        _qt_internal_get_tool_wrapper_script_path(tool_wrapper)
        add_custom_command(
            OUTPUT "${qt_conf_list_path}.done"
            COMMAND
            ${tool_wrapper}
            $<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::qmltyperegistrar>
            --merge-qt-conf "${qt_conf_list_path}"
            --merge-qt-conf-merged-paths "${qt_conf_final_paths_path}"
            COMMENT "Generating qt.conf file"
            DEPENDS
                ${all_parts}
                ${QT_CMAKE_EXPORT_NAMESPACE}::qmltyperegistrar
        )
        # actually not specific to $target, but we need a unique identifier
        set(customtargetname "generate_finalqtconf_${target}")
        add_custom_target("${customtargetname}" DEPENDS "${qt_conf_list_path}.done")
        get_directory_property(all_targets
            DIRECTORY ${PROJECT_SOURCE_DIR}
            QT_QMLDIR_DEFERRED_WRITEOUT_ALL_TARGETS
        )
        foreach (moduleTarget ${all_targets})
            add_dependencies(${moduleTarget} "${customtargetname}")
        endforeach()
    endif()
endfunction()

function(_qt_internal_writebuilddir_qtconf_nondeferred property_folder writeout_folder)
    set(qt_all_qml_output_dirs "")
    get_directory_property(targets
        DIRECTORY "${property_folder}"
        QT_QML_TARGETS_FOR_DEFERRED_QTCONF_WRITEOUT
    )
    set(qtconf_file "${writeout_folder}/qt.conf")

    # Add the current executable's qml module location as an import path as well.
    # Helps finding the executable qml module for macOS app bundles.
    _qt_internal_get_qml_module_import_path("${target}" own_module_import_path)
    list(APPEND qt_all_qml_output_dirs ${own_module_import_path})

    foreach(target IN LISTS targets)
        get_target_property(dependency_targets "${target}" QT_QML_DEPENDENT_QML_MODULE_TARGETS)
        if(NOT dependency_targets)
            continue()
        endif()
        foreach(dep_target ${dependency_targets})
            _qt_internal_get_qml_module_import_path("${dep_target}" module_import_path)
            list(APPEND qt_all_qml_output_dirs ${module_import_path})
        endforeach()
    endforeach()
    if (NOT qt_all_qml_output_dirs)
        return()
    endif()

    list(REMOVE_DUPLICATES qt_all_qml_output_dirs)
    # add quotes to deal with whitespace
    list(TRANSFORM qt_all_qml_output_dirs APPEND "\"")
    list(TRANSFORM qt_all_qml_output_dirs PREPEND "\"")
    list(JOIN qt_all_qml_output_dirs ","  qt_all_qml_output_dirs)

    configure_file(
        ${__qt_qml_macros_module_base_dir}/Qt6qt.conf.in ${qtconf_file}
        @ONLY
    )
endfunction()


function(qt6_add_qml_module target)
    set(args_option
        STATIC
        SHARED
        DESIGNER_SUPPORTED
        FOLLOW_FOREIGN_VERSIONING
        AUTO_RESOURCE_PREFIX
        NO_PLUGIN
        NO_PLUGIN_OPTIONAL
        NO_CREATE_PLUGIN_TARGET
        NO_GENERATE_PLUGIN_SOURCE
        NO_GENERATE_QMLTYPES
        NO_GENERATE_QMLDIR
        NO_GENERATE_EXTRA_QMLDIRS
        NO_LINT
        NO_CACHEGEN
        NO_RESOURCE_TARGET_PATH
        NO_IMPORT_SCAN
        DISCARD_QML_CONTENTS
        ENABLE_TYPE_COMPILER

        # Used to mark modules as having static side effects (i.e. if they install an image provider)
        # The main effect is that we don't warn about such modules being unused. This is also
        # applied to the builtins since we never want to warn about them being unused.
        __QT_INTERNAL_STATIC_MODULE

        # Used to mark modules as being a system module that provides all builtins.
        # This also includes the JavaScript root object as denoted by a jsroot.qmltypes file.
        __QT_INTERNAL_SYSTEM_MODULE

        # Give the resource for the qmldir a unique name; TODO: Remove once we can
        __QT_INTERNAL_DISAMBIGUATE_QMLDIR_RESOURCE
    )

    set(args_single
        PLUGIN_TARGET
        INSTALLED_PLUGIN_TARGET  # Internal option only, it may be removed
        INSTALLED_BACKING_TARGET # Internal option only, it may be removed
        INSTALLED_QMLDIR_PATH # Internal option only, it may be removed
        OUTPUT_TARGETS
        RESOURCE_PREFIX
        URI
        TARGET_PATH   # Internal option only, it may be removed
        VERSION
        OUTPUT_DIRECTORY
        CLASS_NAME
        TYPEINFO
        NAMESPACE
        # TODO: We don't handle installation, warn if callers used these with the old
        #       API and eventually remove them once we have updated all other repos
        RESOURCE_EXPORT
        INSTALL_DIRECTORY
        INSTALL_LOCATION
        TYPE_COMPILER_NAMESPACE
        QMLTC_EXPORT_DIRECTIVE
        QMLTC_EXPORT_FILE_NAME
    )

    set(args_multi
        SOURCES
        QML_FILES
        RESOURCES
        IMPORTS
        IMPORT_PATH
        OPTIONAL_IMPORTS
        DEFAULT_IMPORTS
        DEPENDENCIES
        PAST_MAJOR_VERSIONS
    )

    cmake_parse_arguments(PARSE_ARGV 1 arg
       "${args_option}"
       "${args_single}"
       "${args_multi}"
    )
    if(arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown/unexpected arguments: ${arg_UNPARSED_ARGUMENTS}")
    endif()

    # Warn about options we no longer need/use (these were used by the internal
    # targets and examples, but the logic has been shifted to
    # qt_internal_add_qml_module() or left as a responsibility of the caller).
    if(DEFINED arg_RESOURCE_EXPORT)
        message(AUTHOR_WARNING
            "RESOURCE_EXPORT will be ignored. This function does not handle "
            "installation, which is what RESOURCE_EXPORT was previously used "
            "for. Please update your project to install the target directly."
        )
    endif()

    if(DEFINED arg_INSTALL_DIRECTORY)
        message(AUTHOR_WARNING
            "INSTALL_DIRECTORY will be ignored. This function does not handle "
            "installation, please update your project to install the target "
            "directly."
        )
    endif()

    if(DEFINED arg_INSTALL_LOCATION)
        message(AUTHOR_WARNING
            "INSTALL_LOCATION will be ignored. This function does not handle "
            "installation, please update your project to install the target "
            "directly."
        )
    endif()

    # Mandatory arguments
    if (NOT arg_URI)
        message(FATAL_ERROR
            "Called without a module URI. Please specify one using the URI argument."
        )
    endif()

    _qt_internal_require_qml_uri_valid("${arg_URI}")

    if (NOT arg_VERSION)
        set(arg_VERSION "254.254")
    elseif ("${arg_VERSION}" MATCHES "^([0-9]+\\.[0-9]+)\\.[0-9]+$")
        set(arg_VERSION "${CMAKE_MATCH_1}")
    elseif (NOT "${arg_VERSION}" MATCHES "^[0-9]+\\.[0-9]+$")
        message(FATAL_ERROR
            "Called with an invalid version argument: '${arg_VERSION}'. "
            "Expected version in the form: VersionMajor.VersionMinor."
        )
    endif()

    if(QT_QML_NO_CACHEGEN)
        set(arg_NO_CACHEGEN TRUE)
    endif()

    # Other arguments and checking for invalid combinations
    if (NOT arg_TARGET_PATH)
        # NOTE: This will always be used for copying things to the build
        #       directory, but it will not be used for resource paths if
        #       NO_RESOURCE_TARGET_PATH was given.
        string(REPLACE "." "/" arg_TARGET_PATH ${arg_URI})
    endif()

    if(arg_NO_PLUGIN AND DEFINED arg_PLUGIN_TARGET)
        message(FATAL_ERROR
            "NO_PLUGIN was specified, but PLUGIN_TARGET was also given. "
            "At most one of these can be present."
            )
    endif()

    if (NOT arg_ENABLE_TYPE_COMPILER)
        if (DEFINED arg_TYPE_COMPILER_NAMESPACE)
            message(WARNING
                "TYPE_COMPILER_NAMESPACE is set, but ENABLE_TYPE_COMPILER is not specified. "
                "The TYPE_COMPILER_NAMESPACE value will be ignored."
            )
        endif()

        if (DEFINED arg_QMLTC_EXPORT_DIRECTIVE)
            message(WARNING
                "QMLTC_EXPORT_DIRECTIVE is set, but ENABLE_TYPE_COMPILER is not specified. "
                "The QMLTC_EXPORT_DIRECTIVE value will be ignored."
            )
        endif()
        if (DEFINED arg_QMLTC_EXPORT_FILE_NAME)
            message(WARNING
                "QMLTC_EXPORT_FILE_NAME is set, but ENABLE_TYPE_COMPILER is not specified. "
                "The QMLTC_EXPORT_FILE_NAME will be ignored."
            )
        endif()
    else()
        if ((DEFINED arg_QMLTC_EXPORT_FILE_NAME) AND (NOT (DEFINED arg_QMLTC_EXPORT_DIRECTIVE)))
            message(FATAL_ERROR
                "Specifying a value for QMLTC_EXPORT_FILE_NAME also requires one for QMLTC_EXPORT_DIRECTIVE."
            )
        endif()
    endif()

    set(is_executable FALSE)
    if(TARGET ${target})
        if(arg_STATIC OR arg_SHARED)
            message(FATAL_ERROR
                "Cannot use STATIC or SHARED keyword when passed an existing target (${target})"
                )
        endif()

        # With CMake 3.17 and earlier, a source file's generated property isn't
        # visible outside of the directory scope in which it is set. That can
        # lead to build errors for things like type registration due to CMake
        # thinking nothing will create a missing file on the first run. With
        # CMake 3.18 or later, we can force that visibility. Policy CMP0118
        # added in CMake 3.20 should have made this unnecessary, but we can't
        # rely on that because the user project controls what it is set to at
        # the point where it matters, which is the end of the target's
        # directory scope (hence we can't even test for it here).
        get_target_property(source_dir ${target} SOURCE_DIR)
        if(NOT source_dir STREQUAL CMAKE_CURRENT_SOURCE_DIR AND
           CMAKE_VERSION VERSION_LESS "3.18")
            message(WARNING
                "qt6_add_qml_module() is being called in a different "
                "directory scope to the one in which the target \"${target}\" "
                "was created. CMake 3.18 or later is required to generate a "
                "project robustly for this scenario, but you are using "
                "CMake ${CMAKE_VERSION}. Ideally, qt6_add_qml_module() should "
                "only be called from the same scope as the one the target was "
                "created in to avoid dependency and visibility problems."
            )
        endif()

        get_target_property(backing_target_type ${target} TYPE)
        get_target_property(android_type "${target}" _qt_android_target_type)
        if (backing_target_type STREQUAL "EXECUTABLE" OR android_type STREQUAL "APPLICATION")
            if(DEFINED arg_PLUGIN_TARGET)
                message(FATAL_ERROR
                    "A QML module with an executable as its backing target "
                    "cannot have a plugin."
                )
            endif()
            if(arg_NO_CREATE_PLUGIN_TARGET)
                message(WARNING
                    "A QML module with an executable as its backing target "
                    "cannot have a plugin. The NO_CREATE_PLUGIN_TARGET option "
                    "has no effect and should be removed."
                )
            endif()
            set(arg_NO_PLUGIN TRUE)
            set(lib_type "")
            set(is_executable TRUE)
        elseif(arg_NO_RESOURCE_TARGET_PATH)
            message(FATAL_ERROR
                "NO_RESOURCE_TARGET_PATH cannot be used for a backing target "
                "that is not an executable"
            )
        elseif(backing_target_type STREQUAL "STATIC_LIBRARY")
            set(lib_type STATIC)
        elseif(backing_target_type MATCHES "(SHARED|MODULE)_LIBRARY")
            set(lib_type SHARED)
        else()
            message(FATAL_ERROR "Unsupported backing target type: ${backing_target_type}")
        endif()
    else()
        if(arg_STATIC AND arg_SHARED)
            message(FATAL_ERROR
                "Both STATIC and SHARED specified, at most one can be given"
                )
        endif()

        if(arg_NO_RESOURCE_TARGET_PATH)
            message(FATAL_ERROR
                "NO_RESOURCE_TARGET_PATH can only be provided when an existing "
                "executable target is passed in as the backing target"
            )
        endif()

        # Explicit arguments take precedence, otherwise default to using the same
        # staticality as what Qt was built with. This follows the already
        # established default behavior for building ordinary Qt plugins.
        # We don't allow the standard CMake BUILD_SHARED_LIBS variable to control
        # the default because that can lead to different defaults depending on
        # whether you build with a separate backing target or not.
        if(arg_STATIC)
            set(lib_type STATIC)
        elseif(arg_SHARED)
            set(lib_type SHARED)
        elseif(QT6_IS_SHARED_LIBS_BUILD)
            set(lib_type SHARED)
        else()
            set(lib_type STATIC)
        endif()
    endif()

    set(has_plugin TRUE)
    if(arg_NO_PLUGIN)
        # Simplifies things a bit further below
        set(arg_PLUGIN_TARGET "")
        set(has_plugin FALSE)
    elseif(NOT DEFINED arg_PLUGIN_TARGET)
        if(arg_NO_CREATE_PLUGIN_TARGET)
            # We technically could allow this and rely on the project using the
            # default plugin target name, but not doing so gives us the
            # flexibility to potentially change that default later if needed.
            message(FATAL_ERROR
                "PLUGIN_TARGET must also be provided when NO_CREATE_PLUGIN_TARGET "
                "is used. If you want to disable creating a plugin altogether, "
                "use the NO_PLUGIN option instead."
            )
        endif()
        set(arg_PLUGIN_TARGET ${target}plugin)
    endif()

    if(arg_PLUGIN_TARGET STREQUAL "${target}")
        set(is_backing_and_plugin_same TRUE)
    else()
        set(is_backing_and_plugin_same FALSE)
    endif()

    if(arg_NO_CREATE_PLUGIN_TARGET AND is_backing_and_plugin_same AND NOT TARGET ${target})
        message(FATAL_ERROR
            "PLUGIN_TARGET is the same as the backing target, which is allowed, "
            "but NO_CREATE_PLUGIN_TARGET was also given and the target does not "
            "exist. Either ensure the target is already created or do not "
            "specify NO_CREATE_PLUGIN_TARGET."
        )
    endif()
    if(NOT arg_INSTALLED_PLUGIN_TARGET)
        set(arg_INSTALLED_PLUGIN_TARGET ${arg_PLUGIN_TARGET})
    endif()
    if(NOT arg_INSTALLED_BACKING_TARGET)
        set(arg_INSTALLED_BACKING_TARGET ${target})
    endif()

    set(no_gen_source)
    if(arg_NO_GENERATE_PLUGIN_SOURCE)
        set(no_gen_source NO_GENERATE_PLUGIN_SOURCE)
    endif()

    if(arg_OUTPUT_DIRECTORY)
        get_filename_component(arg_OUTPUT_DIRECTORY "${arg_OUTPUT_DIRECTORY}"
            ABSOLUTE BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}"
        )
    else()
        if("${QT_QML_OUTPUT_DIRECTORY}" STREQUAL "")
            set(arg_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
            # For libraries, we assume/require that the source directory
            # structure is consistent with the target path. For executables,
            # the source directory will usually not reflect the target path
            # and the project will often expect to be able to use resource
            # paths that don't include the target path (they need the
            # NO_RESOURCE_TARGET_PATH option if they do that). Tooling always
            # needs the target path in the file system though, so the output
            # directory should always have it. Handle the special case for
            # executables to ensure this is what we get.
            if(is_executable)
                string(APPEND arg_OUTPUT_DIRECTORY "/${arg_TARGET_PATH}")
            endif()
        else()
            if(NOT IS_ABSOLUTE "${QT_QML_OUTPUT_DIRECTORY}")
                message(FATAL_ERROR
                    "QT_QML_OUTPUT_DIRECTORY must be an absolute path, but given: "
                    "${QT_QML_OUTPUT_DIRECTORY}"
                )
            endif()
            # This inherently does what we want for libraries and executables
            set(arg_OUTPUT_DIRECTORY ${QT_QML_OUTPUT_DIRECTORY}/${arg_TARGET_PATH})
        endif()
    endif()

    # Sanity check that we are not trying to have two different QML modules use
    # the same output directory.
    _qt_internal_find_qml_module(other_target BY_OUTPUT_DIR "${arg_OUTPUT_DIRECTORY}")
    if(other_target)
        message(FATAL_ERROR
            "Output directory for target \"${target}\" is already used by "
            "another QML module (target \"${other_target}\"). "
            "Output directory is:\n  ${arg_OUTPUT_DIRECTORY}\n"
        )
    endif()

    set_property(GLOBAL APPEND PROPERTY _qt_all_qml_uris ${arg_URI})
    set_property(GLOBAL APPEND PROPERTY _qt_all_qml_output_dirs ${arg_OUTPUT_DIRECTORY})
    set_property(GLOBAL APPEND PROPERTY _qt_all_qml_targets     ${target})

    if(NOT arg_CLASS_NAME AND TARGET "${arg_PLUGIN_TARGET}")
        get_target_property(class_name ${arg_PLUGIN_TARGET} QT_PLUGIN_CLASS_NAME)
        if(class_name)
            set(arg_CLASS_NAME)
        endif()
    endif()
    if(NOT arg_CLASS_NAME)
        _qt_internal_compute_qml_plugin_class_name_from_uri("${arg_URI}" arg_CLASS_NAME)
    endif()

    if(TARGET ${target})
        if(is_backing_and_plugin_same)
            # Insert the plugin's URI into its meta data to enable usage
            # of static plugins in QtDeclarative (like in mkspecs/features/qml_plugin.prf).
            set_property(TARGET ${target} APPEND PROPERTY
                AUTOMOC_MOC_OPTIONS "-Muri=${arg_URI}"
            )
        endif()
    else()
        if(is_backing_and_plugin_same)
            set(conditional_args ${no_gen_source})
            if(arg_NAMESPACE)
                list(APPEND conditional_args NAMESPACE ${arg_NAMESPACE})
            endif()
            qt6_add_qml_plugin(${target}
                ${lib_type}
                OUTPUT_DIRECTORY ${arg_OUTPUT_DIRECTORY}
                URI ${arg_URI}
                CLASS_NAME ${arg_CLASS_NAME}
                ${conditional_args}
            )
        else()
            qt6_add_library(${target} ${lib_type})
        endif()
    endif()

    if(NOT target STREQUAL Qml)
        target_link_libraries(${target} PRIVATE ${QT_CMAKE_EXPORT_NAMESPACE}::Qml)
    endif()

    if(NOT arg_TYPEINFO AND NOT arg_NO_GENERATE_QMLTYPES)
        set(arg_TYPEINFO ${target}.qmltypes)
    endif()

    set(all_qml_import_paths "${arg_IMPORT_PATH}")
    set(all_dependency_targets)

    set(original_no_show_policy_value "${QT_NO_SHOW_OLD_POLICY_WARNINGS}")
    # silent by default, we only warn if someone uses TARGET as a URI
    set(QT_NO_SHOW_OLD_POLICY_WARNINGS TRUE)
    __qt_internal_setup_policy(QTP0005 "6.8.0"
    "" # intentionally empty as we silence the warning anyway
    )
    qt6_policy(GET QTP0005 allow_targets_for_dependencies_policy)
    set(QT_NO_SHOW_OLD_POLICY_WARNINGS "${original_no_show_policy_value}")
    string(COMPARE EQUAL "${allow_targets_for_dependencies_policy}" "NEW" target_is_keyword)


    set(target_keyword_was_set FALSE)
    foreach(import_set IN ITEMS IMPORTS OPTIONAL_IMPORTS DEFAULT_IMPORTS)
        foreach(import IN LISTS arg_${import_set})
            if (import STREQUAL "TARGET")
                if (target_is_keyword)
                    set(target_keyword_was_set TRUE)
                    continue()
                else()
                    message(AUTHOR_WARNING "TARGET is treated as a URI because QTP0005 is set to OLD. This is deprecated behavior. Check https://doc.qt.io/qt-6/qt-cmake-policy-qtp0005.html for policy details.")
                    set(target_keyword_was_set FALSE)
                endif()
            endif()
            _qt_internal_parse_qml_module_dependency(${import} ${target_keyword_was_set}
                OUTPUT_URI import_uri
                OUTPUT_VERSION import_version
                OUTPUT_MODULE_LOCATION module_location
                OUTPUT_MODULE_TARGET dependency_target
            )
            get_filename_component(module_import_path "${module_location}" DIRECTORY)
            list(APPEND all_qml_import_paths "${module_import_path}")

            if (NOT "${import_version}" STREQUAL "")
                set_property(TARGET ${target} APPEND PROPERTY
                    QT_QML_MODULE_${import_set} "${import_uri} ${import_version}"
                )
            else()
                set_property(TARGET ${target} APPEND PROPERTY
                    QT_QML_MODULE_${import_set} "${import_uri}"
                )
            endif()
            if(TARGET "${dependency_target}")
                list(APPEND all_dependency_targets "${dependency_target}")
            endif()
            set(target_keyword_was_set FALSE)
        endforeach()
    endforeach()

    foreach(dependency IN LISTS arg_DEPENDENCIES)

        if (dependency STREQUAL "TARGET")
            if (target_is_keyword)
                set(target_keyword_was_set TRUE)
                continue()
            else()
                message(AUTHOR_WARNING "TARGET is treated as a URI because QTP0005 is set to OLD. This is deprecated behavior. Check https://doc.qt.io/qt-6/qt-cmake-policy-qtp0005.html for policy details.")
                set(target_keyword_was_set FALSE)
            endif()
        endif()
        _qt_internal_parse_qml_module_dependency(${dependency} "${target_keyword_was_set}"
            OUTPUT_URI dep_uri
            OUTPUT_VERSION dep_version
            OUTPUT_MODULE_LOCATION module_location
            OUTPUT_MODULE_TARGET dependency_target
        )
        get_filename_component(module_import_path "${module_location}" DIRECTORY)
        list(APPEND all_qml_import_paths "${module_import_path}")
        if (NOT "${dep_version}" STREQUAL "")
            set_property(TARGET ${target} APPEND PROPERTY
                QT_QML_MODULE_DEPENDENCIES "${dep_uri} ${dep_version}"
            )
        else()
            set_property(TARGET ${target} APPEND PROPERTY
                QT_QML_MODULE_DEPENDENCIES "${dep_uri}"
            )
        endif()
        set(target_keyword_was_set FALSE)
        if(TARGET "${dependency_target}")
            list(APPEND all_dependency_targets "${dependency_target}")
        endif()
    endforeach()
    ### TODO: add support for transitive dependencies, too
    list(REMOVE_DUPLICATES all_dependency_targets)
    set_property(TARGET ${target} PROPERTY QT_QML_DEPENDENT_QML_MODULE_TARGETS "${all_dependency_targets}")
    _qt_internal_collect_qml_module_dependencies(${target})
    # add a dependency at the build system level, too - we know that
    # the module wouldn't run if the dependencies
    # do not exist yet
    if(all_dependency_targets)
        add_dependencies(${target} ${all_dependency_targets})
    endif()

    if(arg_AUTO_RESOURCE_PREFIX)
        if(arg_RESOURCE_PREFIX)
            message(FATAL_ERROR
                "Both RESOURCE_PREFIX and AUTO_RESOURCE_PREFIX are specified for ${target}. "
                "You can only have one."
            )
        else()
            set(arg_RESOURCE_PREFIX "/qt/qml")
            message(DEPRECATION "AUTO_RESOURCE_PREFIX is deprecated. "
                    "Please use the qt_policy(SET) command to set the QTP0001 policy, "
                    "or use the qt_standard_project_setup() command to set your preferred "
                    "REQUIRES to get the preferred behavior. "
                    "Check https://doc.qt.io/qt-6/qt-cmake-policy-qtp0001.html for policy details.")
        endif()
    elseif(arg_RESOURCE_PREFIX)
        _qt_internal_canonicalize_resource_path("${arg_RESOURCE_PREFIX}" arg_RESOURCE_PREFIX)
    elseif(arg_NO_RESOURCE_TARGET_PATH)
        # Suppress the warning if NO_RESOURCE_TARGET_PATH is given.
        # In that case, we assume the user knows what they want.
        set(arg_RESOURCE_PREFIX "/")
    else()
        __qt_internal_setup_policy(QTP0001 "6.5.0"
"':/qt/qml/' is the default resource prefix for QML modules. \
Check https://doc.qt.io/qt-6/qt-cmake-policy-qtp0001.html for policy details."
        )
        qt6_policy(GET QTP0001 use_auto_prefix_policy)
        if ("${use_auto_prefix_policy}" STREQUAL "NEW")
            set(arg_RESOURCE_PREFIX "/qt/qml")
        else()
            set(arg_RESOURCE_PREFIX "/")
        endif()
    endif()

    if(arg_NO_RESOURCE_TARGET_PATH)
        set(qt_qml_module_resource_prefix "${arg_RESOURCE_PREFIX}")
    else()
        if(arg_RESOURCE_PREFIX STREQUAL "/")   # Checked so we prevent double-slash
            set(qt_qml_module_resource_prefix "/${arg_TARGET_PATH}")
        else()
            set(qt_qml_module_resource_prefix "${arg_RESOURCE_PREFIX}/${arg_TARGET_PATH}")
        endif()
    endif()

    list(REMOVE_DUPLICATES all_qml_import_paths)

    set_target_properties(${target} PROPERTIES
        QT_QML_MODULE_NO_LINT "${arg_NO_LINT}"
        QT_QML_MODULE_NO_CACHEGEN "${arg_NO_CACHEGEN}"
        QT_QML_MODULE_NO_GENERATE_QMLDIR "${arg_NO_GENERATE_QMLDIR}"
        QT_QML_MODULE_NO_GENERATE_EXTRA_QMLDIRS "${arg_NO_GENERATE_EXTRA_QMLDIRS}"
        QT_QML_MODULE_NO_PLUGIN "${arg_NO_PLUGIN}"
        QT_QML_MODULE_NO_PLUGIN_OPTIONAL "${arg_NO_PLUGIN_OPTIONAL}"
        QT_QML_MODULE_NO_IMPORT_SCAN "${arg_NO_IMPORT_SCAN}"
        QT_QML_MODULE_DISCARD_QML_CONTENTS "${arg_DISCARD_QML_CONTENTS}"
        _qt_qml_module_follow_foreign_versioning "${arg_FOLLOW_FOREIGN_VERSIONING}"
        QT_QML_MODULE_URI "${arg_URI}"
        QT_QML_MODULE_TARGET_PATH "${arg_TARGET_PATH}"
        QT_QML_MODULE_VERSION "${arg_VERSION}"
        QT_QML_MODULE_CLASS_NAME "${arg_CLASS_NAME}"

        QT_QML_MODULE_PLUGIN_TARGET "${arg_PLUGIN_TARGET}"
        QT_QML_MODULE_INSTALLED_PLUGIN_TARGET "${arg_INSTALLED_PLUGIN_TARGET}"

        # Also Save the PLUGIN_TARGET values in a separate property to circumvent
        # https://gitlab.kitware.com/cmake/cmake/-/issues/21484 when exporting the properties
        _qt_qml_module_plugin_target "${arg_PLUGIN_TARGET}"
        _qt_qml_module_installed_plugin_target "${arg_INSTALLED_PLUGIN_TARGET}"

        _qt_qml_module_uri "${arg_URI}"
        _qt_qml_module_target_path "${arg_TARGET_PATH}"
        _qt_qml_module_version "${arg_VERSION}"
        _qt_qml_module_class_name "${arg_CLASS_NAME}"
        _qt_qml_module_is_backing_target "TRUE"
        _qt_qml_module_has_plugin "${has_plugin}"
        _qt_qml_backing_library_and_plugin_are_same_target "${is_backing_and_plugin_same}"
        _qt_qml_module_installed_qmldir_path "${arg_INSTALLED_QMLDIR_PATH}"

        QT_QML_MODULE_DESIGNER_SUPPORTED "${arg_DESIGNER_SUPPORTED}"
        QT_QML_MODULE_IS_STATIC "${arg___QT_INTERNAL_STATIC_MODULE}"
        QT_QML_MODULE_IS_SYSTEM "${arg___QT_INTERNAL_SYSTEM_MODULE}"
        QT_QML_MODULE_OUTPUT_DIRECTORY "${arg_OUTPUT_DIRECTORY}"
        QT_QML_MODULE_RESOURCE_PREFIX "${qt_qml_module_resource_prefix}"
        QT_QML_MODULE_PAST_MAJOR_VERSIONS "${arg_PAST_MAJOR_VERSIONS}"
        QT_QML_MODULE_NAMESPACE "${arg_NAMESPACE}"

        # TODO: Check how this is used by qt6_android_generate_deployment_settings()
        QT_QML_IMPORT_PATH "${all_qml_import_paths}"
    )

    if(arg_TYPEINFO)
        set_target_properties(${target} PROPERTIES
            QT_QML_MODULE_TYPEINFO "${arg_TYPEINFO}"
        )
    endif()

    # Executables don't have a plugin target, so no need to export the properties.
    if(NOT backing_target_type STREQUAL "EXECUTABLE" AND NOT is_android_executable)
        set_property(TARGET ${target} APPEND PROPERTY
            EXPORT_PROPERTIES _qt_qml_module_plugin_target _qt_qml_module_installed_plugin_target
        )
    endif()

    _qt_internal_qml_export_common_qml_properties("${target}")
    set_property(TARGET ${target} APPEND PROPERTY
        EXPORT_PROPERTIES
            _qt_qml_module_is_backing_target
            _qt_qml_module_has_plugin
    )

    if(NOT arg_NO_GENERATE_QMLTYPES)
        set(type_registration_extra_args "")
        if(arg_NAMESPACE)
            list(APPEND type_registration_extra_args NAMESPACE ${arg_NAMESPACE})
        endif()
        set_target_properties(${target} PROPERTIES _qt_internal_has_qmltypes TRUE)
        _qt_internal_qml_type_registration(${target} ${type_registration_extra_args})
    else()
        set_target_properties(${target} PROPERTIES _qt_internal_has_qmltypes FALSE)
    endif()

    set(output_targets)

    if(NOT arg_NO_GENERATE_QMLDIR)
        _qt_internal_target_generate_qmldir(${target})
        _qt_internal_set_source_file_generated(SOURCES "${arg_OUTPUT_DIRECTORY}/qmldir")

        if(${arg___QT_INTERNAL_DISAMBIGUATE_QMLDIR_RESOURCE})
            # TODO: Make this the default and remove the option
            string(REPLACE "/" "_" qmldir_resource_name "${target}_qmldir_${arg_TARGET_PATH}")
        else()
            # Embed qmldir in qrc. The following comments relate mostly to Qt5->6 transition.
            # The requirement to keep the same resource name might no longer apply, but it doesn't
            # currently appear to cause any hinderance to keep it.
            # The qmldir resource name needs to match the one generated by qmake's qml_module.prf,
            # to ensure that all Q_INIT_RESOURCE(resource_name) calls in Qt code don't lead to
            # undefined symbol errors when linking an application project.
            # The Q_INIT_RESOURCE() calls are not strictly necessary anymore because the CMake Qt
            # build passes around the compiled resources as object files.
            # These object files have global initiliazers that don't get discared when linked into
            # an application (as opposed to when the resource libraries were embedded into the
            # static libraries when Qt was built with qmake).
            # The reason to match the naming is to ensure that applications link successfully
            # regardless if Qt was built with CMake or qmake, while the build system transition
            # phase is still happening.
            string(REPLACE "/" "_" qmldir_resource_name "qmake_${arg_TARGET_PATH}")
        endif()

        # The qmldir file ALWAYS has to be under the target path, even in the
        # resources. If it isn't, an explicit import can't find it. We need a
        # second copy NOT under the target path if NO_RESOURCE_TARGET_PATH is
        # given so that the implicit import will work.
        set(prefixes "${qt_qml_module_resource_prefix}")
        if(arg_NO_RESOURCE_TARGET_PATH)
            # The above prefixes item won't include the target path, so add a
            # second one that does.
            if(qt_qml_module_resource_prefix STREQUAL "/")
                list(APPEND prefixes "/${arg_TARGET_PATH}")
            else()
                list(APPEND prefixes "${qt_qml_module_resource_prefix}/${arg_TARGET_PATH}")
            endif()
        endif()
        set_source_files_properties(${arg_OUTPUT_DIRECTORY}/qmldir
            PROPERTIES QT_RESOURCE_ALIAS "qmldir"
        )

        foreach(prefix IN LISTS prefixes)
            set(resource_targets)
            qt6_add_resources(${target} ${qmldir_resource_name}
                FILES ${arg_OUTPUT_DIRECTORY}/qmldir
                PREFIX "${prefix}"
                OUTPUT_TARGETS resource_targets
            )

            # Save the resource name in a property so we can reference it later in a qml plugin
            # constructor, to avoid discarding the resource if it's in a static library.
            __qt_internal_sanitize_resource_name(
                sanitized_qmldir_resource_name "${qmldir_resource_name}")
            set_property(TARGET ${target} APPEND PROPERTY
                _qt_qml_module_sanitized_resource_names "${sanitized_qmldir_resource_name}")

            list(APPEND output_targets ${resource_targets})
            # If we are adding the same file twice, we need a different resource
            # name for the second one. It has the same QT_RESOURCE_ALIAS but a
            # different prefix, so we can't put it in the same resource.
            string(APPEND qmldir_resource_name "_copy")
        endforeach()
    endif()

    if(NOT arg_NO_PLUGIN AND NOT arg_NO_CREATE_PLUGIN_TARGET)
        # This also handles the case where ${arg_PLUGIN_TARGET} already exists,
        # including where it is the same as ${target}. If ${arg_PLUGIN_TARGET}
        # already exists, it will update the necessary things that are specific
        # to qml plugins.
        if(TARGET ${arg_PLUGIN_TARGET})
            set(plugin_args "")
        else()
            set(plugin_args ${lib_type})
        endif()
        list(APPEND plugin_args ${no_gen_source})
        if(arg_NAMESPACE)
            list(APPEND plugin_args NAMESPACE ${arg_NAMESPACE})
        endif()
        qt6_add_qml_plugin(${arg_PLUGIN_TARGET}
            ${plugin_args}
            OUTPUT_DIRECTORY ${arg_OUTPUT_DIRECTORY}
            BACKING_TARGET ${target}
            CLASS_NAME ${arg_CLASS_NAME}
        )
    endif()

    if(TARGET "${arg_PLUGIN_TARGET}" AND NOT is_backing_and_plugin_same)
        target_link_libraries(${arg_PLUGIN_TARGET} PRIVATE ${target})
    endif()

    target_sources(${target} PRIVATE ${arg_SOURCES})

    # QML tooling might need to map build dir paths to source dir paths. Create
    # a mapping file before qt6_target_qml_sources() to be able to use it
    _qt_internal_qml_map_build_files(${target} ${qt_qml_module_resource_prefix} dir_map_qrc)
    # use different property from _qt_generated_qrc_files since this qrc is
    # special (and is not a real resource file)
    set_property(TARGET ${target} APPEND PROPERTY _qt_qml_meta_qrc_files "${dir_map_qrc}")

    set(do_qml_aotstats OFF)
    if(NOT DEFINED QT_QML_GENERATE_AOTSTATS OR QT_QML_GENERATE_AOTSTATS)
        if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.19)
            set(do_qml_aotstats ON)
        else()
            message(WARNING "aotstats is not supported on CMake versions < 3.19")
        endif()
    endif()

    # Ensure that a top-level target-tooling that accumulates the target sources
    # Before 3.19 interface targets could not have sources, so a different approach is used
    # for that.
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.19)
        _qt_internal_ensure_tooling_target("${target}_tooling" DEPENDENT_TARGET "${target}")
    endif()

    set(cache_target)
    qt6_target_qml_sources(${target}
        __QT_INTERNAL_FORCE_DEFER_QMLDIR
        QML_FILES ${arg_QML_FILES}
        RESOURCES ${arg_RESOURCES}
        OUTPUT_TARGETS cache_target
        PREFIX "${qt_qml_module_resource_prefix}"
    )
    list(APPEND output_targets ${cache_target})

    # Setup the qml/resource files copy instructions
    _qt_internal_qml_copy_files_to_build_dir("${target}"
        CUSTOM_TARGET_SUFFIX qml
        PROP_WITH_ENTRIES _qt_qml_files_to_copy
        PROP_WITH_SRCS _qt_qml_files_absolute_src_paths
        FILE_TYPE "sources"
    )
    _qt_internal_qml_copy_files_to_build_dir("${target}"
        CUSTOM_TARGET_SUFFIX res
        PROP_WITH_ENTRIES _qt_qml_resources_to_copy
        PROP_WITH_SRCS _qt_qml_resources_absolute_src_paths
        FILE_TYPE "resources"
    )

    # Build an init object library for static plugins and propagate it along with the plugin
    # target.
    # TODO: Figure out if we can move this code block into qt_add_qml_plugin. Need to consider
    #       various corner cases.
    #       QTBUG-96937
    if(TARGET "${arg_PLUGIN_TARGET}")
        get_target_property(plugin_lib_type ${arg_PLUGIN_TARGET} TYPE)
        if(plugin_lib_type STREQUAL "STATIC_LIBRARY")
            __qt_internal_add_static_plugin_init_object_library(
                "${arg_PLUGIN_TARGET}" plugin_init_target)
            list(APPEND output_targets ${plugin_init_target})

            __qt_internal_propagate_object_library("${arg_PLUGIN_TARGET}" "${plugin_init_target}")
        endif()
    endif()

    if(NOT arg_NO_GENERATE_QMLDIR)
        if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.19.0")
            # Defer the write to allow more qml files to be added later by calls to
            # qt6_target_qml_sources(). We wrap the deferred call with EVAL CODE
            # so that ${target} is evaluated now rather than the end of the scope.
            # We also delay target finalization until after our deferred write
            # because the qmldir file must be written before any finalizer
            # might call qt_import_qml_plugins().
            cmake_language(EVAL CODE
                "cmake_language(DEFER ID_VAR write_id CALL _qt_internal_write_deferred_qmldir_file ${target})"
            )
            _qt_internal_delay_finalization_until_after(${write_id})
        else()
            # Can't defer the write, have to do it now
            _qt_internal_write_deferred_qmldir_file(${target})
        endif()
    endif()

    if (arg_ENABLE_TYPE_COMPILER)
        if (DEFINED arg_TYPE_COMPILER_NAMESPACE AND NOT $<STREQUAL:"${arg_TYPE_COMPILER_NAMESPACE}","">)
            set(qmltc_namespace ${arg_TYPE_COMPILER_NAMESPACE})
        else()
            string(REPLACE "." "::" qmltc_namespace "${arg_URI}")
        endif()
        _qt_internal_target_enable_qmltc(${target}
            QML_FILES ${arg_QML_FILES}
            IMPORT_PATHS ${arg_IMPORT_PATH}
            NAMESPACE ${qmltc_namespace}
            EXPORT_MACRO_NAME ${arg_QMLTC_EXPORT_DIRECTIVE}
            EXPORT_FILE_NAME ${arg_QMLTC_EXPORT_FILE_NAME}
            MODULE ${arg_URI}
        )
    endif()

    if(arg_OUTPUT_TARGETS)
        set(${arg_OUTPUT_TARGETS} ${output_targets} PARENT_SCOPE)
    endif()


    set(ensure_set_properties
        QT_QML_MODULE_PLUGIN_TYPES_FILE
        QT_QML_MODULE_QML_FILES
        QT_QML_MODULE_RESOURCES       # Original files as provided by the project (absolute)
        QT_QML_MODULE_RESOURCE_PATHS  # By qmlcachegen (resource paths)
        QT_QMLCACHEGEN_DIRECT_CALLS
        QT_QMLCACHEGEN_EXECUTABLE
        QT_QMLCACHEGEN_ARGUMENTS
    )
    foreach(prop IN LISTS ensure_set_properties)
        get_target_property(val ${target} ${prop})
        if("${val}" MATCHES "-NOTFOUND$")
            set_target_properties(${target} PROPERTIES ${prop} "")
        endif()
    endforeach()

    if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.19.0")
        # collect all build dirs obtained from all the qt_add_qml_module calls and
        # write the .qmlls.ini file in a deferred call

        if(NOT "${arg_OUTPUT_DIRECTORY}" STREQUAL "")
            set(output_folder "${arg_OUTPUT_DIRECTORY}")
        else()
            set(output_folder "${CMAKE_CURRENT_BINARY_DIR}")
        endif()
        string(REPLACE "." ";" uri_bits "${arg_URI}")
        list(REVERSE uri_bits)
        set(build_folder_current "${output_folder}")
        set(use_build_folder true)
        foreach(bit IN LISTS uri_bits)
            get_filename_component(current_folder "${build_folder_current}" NAME)
            get_filename_component(build_folder_current "${build_folder_current}" DIRECTORY)
            if(NOT current_folder STREQUAL bit)
                set(use_build_folder false)
                break()
            endif()
        endforeach()
        if(use_build_folder)
            set(build_folder "${build_folder_current}")
        else()
            set(build_folder "${output_folder}")
        endif()


        if(QT_QML_GENERATE_QMLLS_INI)
            set_property(DIRECTORY APPEND PROPERTY _qmlls_ini_build_folders "${build_folder}")
            set_property(DIRECTORY APPEND PROPERTY _qmlls_ini_import_path_targets "${target}")

            # if no call with id 'qmlls_ini_generation_id' was deferred for this directory, do it now
            cmake_language(DEFER GET_CALL qmlls_ini_generation_id call)
            if("${call}" STREQUAL "")
                cmake_language(EVAL CODE
                    "cmake_language(DEFER ID qmlls_ini_generation_id CALL _qt_internal_write_deferred_qmlls_ini_file ${target})"
                )
            endif()
        endif()

        set_property(GLOBAL APPEND PROPERTY _qmlls_build_ini_targets "${target}")

        cmake_language(DEFER DIRECTORY "${CMAKE_BINARY_DIR}" GET_CALL qmlls_build_ini_generation_id call)
        if("${call}" STREQUAL "")
            cmake_language(EVAL CODE
                "cmake_language(DEFER DIRECTORY \"${CMAKE_BINARY_DIR}\" "
                    "ID qmlls_build_ini_generation_id "
                    "CALL _qt_internal_write_deferred_qmlls_build_ini_file"
                    "${QT_CMAKE_EXPORT_NAMESPACE}"
                ")"
            )
        endif()
    else()
        if(QT_QML_GENERATE_QMLLS_INI)
            get_property(__qt_internal_generate_qmlls_ini_warning
                GLOBAL
                PROPERTY __qt_internal_generate_qmlls_ini_warning
            )
            if (NOT "${__qt_internal_generate_qmlls_ini_warning}")
                message(WARNING "QT_QML_GENERATE_QMLLS_INI is not supported on CMake versions < 3.19, disabling...")
                set_property(GLOBAL PROPERTY __qt_internal_generate_qmlls_ini_warning ON)
            endif()
        endif()
    endif()

    # write out extra qtconf files for executables to automatically set up import paths;
    # but avoid needless warnings and work if there are no relevant dependencies specified
    if((backing_target_type STREQUAL "EXECUTABLE")
        AND all_dependency_targets)
        if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.19.0")
            # The general logic is that every target writes out a "partial" file,
            # containing its needed imports.
            # Once all partial files are written,
            # we run a helper process which consolidates them.
            # We need the counter to know when the last finalizer runs,
            # as only then we'll know that all partial files have been
            # written out, and we can start merging them.
            get_directory_property(counter
                DIRECTORY ${PROJECT_SOURCE_DIR}
                QT_QMLDIR_DEFERRED_WRITEOUT_COUNTER
            )
            if (NOT counter)
                set(counter "0")
            endif()
            math(EXPR counter "${counter} + 1")
            set_property(
                DIRECTORY ${PROJECT_SOURCE_DIR}
                PROPERTY QT_QMLDIR_DEFERRED_WRITEOUT_COUNTER
                "${counter}"
            )
            cmake_language(EVAL CODE "
                    cmake_language(DEFER DIRECTORY [[${PROJECT_SOURCE_DIR}]]
                        CALL _qt_internal_write_qmldir_part \"${target}\"
                        \"${QT_CMAKE_EXPORT_NAMESPACE}\")
                ")
        else()
            # Before CMake 3.19, we don't have DEFER, so we immediately write out the qt.conf
            # file, overwriting its content if necessary.
            # That would be fine, as we build up a list and just end up overwriting the file again
            # Unfortunately, CMake < 3.19 doesn't allow us to set directory properties on
            # binary dirs, either.
            # So we'll end up having to set the properties on the source directory
            get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
            if (is_multi_config)
                message(FATAL
                "Using TARGET based dependencies in qt_add_qml_module requires at least CMake 3.19 "
                "when using multi-config generators")
            else()
                message(WARNING
                "Using TARGET based dependencies in qt_add_qml_module with CMake < 3.19 "
                "might result in missing import paths if they are not added manually.")
            endif()
            get_target_property(effective_outdir ${target} RUNTIME_OUTPUT_DIRECTORY)
            if (NOT "${effective_outdir}")
                set(effective_outdir "${CMAKE_CURRENT_BINARY_DIR}")
            endif()

            set_property(
                DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                APPEND
                PROPERTY QT_QML_TARGETS_FOR_DEFERRED_QTCONF_WRITEOUT
                ${target}
            )
            _qt_internal_writebuilddir_qtconf_nondeferred("${CMAKE_CURRENT_SOURCE_DIR}" "${effective_outdir}")
        endif()
    endif()

    if(do_qml_aotstats)
        set_property(GLOBAL APPEND PROPERTY _qt_qml_aotstats_module_targets ${target})
        set_target_properties(${target} PROPERTIES
            QT_QML_MODULE_RCC_QMLCACHE_PATH "${CMAKE_CURRENT_BINARY_DIR}/.rcc/qmlcache"
        )

        get_cmake_property(aotstats_setup_called _qt_internal_deferred_aotstats_setup)
        if(NOT aotstats_setup_called)
            set_property(GLOBAL PROPERTY _qt_internal_deferred_aotstats_setup TRUE)
            cmake_language(EVAL CODE "cmake_language(DEFER DIRECTORY \"${CMAKE_BINARY_DIR}\" "
                "CALL _qt_internal_deferred_aotstats_setup \"${QT_CMAKE_EXPORT_NAMESPACE}\"
            )")
        endif()
    endif()
endfunction()

function(_qt_internal_deferred_aotstats_setup qt_cmake_export_namespace)
    # _qt_internal_deferred_aotstats_setup is deferred to be called in the root
    # CMAKE_BINARY_DIR. If find_package(Qt6) is not called in the root of the project (like in the
    # Qt Creator super repo), QT_CMAKE_EXPORT_NAMESPACE will not be defined.
    # Manually define it here, based on the value passed to the function, from a scope where it
    # is available.
    set(QT_CMAKE_EXPORT_NAMESPACE "${qt_cmake_export_namespace}")

    get_property(module_targets GLOBAL PROPERTY _qt_qml_aotstats_module_targets)

    set(onlybytecode_modules "")
    set(empty_modules "")
    set(module_aotstats_targets "")
    set(module_aotstats_files "")

    _qt_internal_get_tool_wrapper_script_path(tool_wrapper)
    foreach(module_target IN LISTS module_targets)
        get_target_property(qmlcachegen_args    ${module_target} QT_QMLCACHEGEN_ARGUMENTS)
        get_target_property(uri                 ${module_target} QT_QML_MODULE_URI)
        get_target_property(aotstats_files      ${module_target} QT_QML_MODULE_AOTSTATS_FILES)
        get_target_property(rcc_qmlcache_path   ${module_target} QT_QML_MODULE_RCC_QMLCACHE_PATH)

        if("--only-bytecode" IN_LIST qmlcachegen_args)
            list(APPEND onlybytecode_modules "${uri}(${module_target})")
            continue()
        elseif(NOT aotstats_files)
            list(APPEND empty_modules "${uri}(${module_target})")
            continue()
        endif()

        list(JOIN aotstats_files "\n" aotstats_files_lines)
        set(module_aotstats_list_file "${rcc_qmlcache_path}/module_${module_target}.aotstatslist")
        file(WRITE ${module_aotstats_list_file} "${aotstats_files_lines}")

        set(output "${rcc_qmlcache_path}/module_${module_target}.aotstats")
        add_custom_command(
            OUTPUT ${output}
            DEPENDS ${aotstats_files} ${module_aotstats_list_file}
            COMMAND
                ${tool_wrapper}
                $<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::qmlaotstats>
                aggregate
                ${module_aotstats_list_file}
                ${output}
        )

        set(module_aotstats_target_name "module_${module_target}_aotstats_target")
        add_custom_target(${module_aotstats_target_name}
            DEPENDS ${output}
        )
        _qt_internal_assign_to_internal_targets_folder(${module_aotstats_target_name})
        if(TARGET ${module_target}_qmltyperegistration)
            add_dependencies(${module_aotstats_target_name} ${module_target}_qmltyperegistration)
        else()
            add_dependencies(${module_aotstats_target_name} ${module_target})
        endif()

        list(APPEND module_aotstats_targets ${module_aotstats_target_name})
        list(APPEND module_aotstats_files ${output})
    endforeach()


    set(project_rcc_qmlcache "${PROJECT_BINARY_DIR}/.rcc/qmlcache")

    set(qmlaotstats_options "")
    if(onlybytecode_modules)
        set(onlybytecode_modules_file "${project_rcc_qmlcache}/aotstats_onlybytecode_modules.txt")
        list(JOIN onlybytecode_modules "\n" onlybytecode_modules_lines)
        file(WRITE ${onlybytecode_modules_file} "${onlybytecode_modules_lines}")
        list(APPEND qmlaotstats_options "--only-bytecode-modules" "${onlybytecode_modules_file}")
    endif()
    if(empty_modules)
        set(empty_modules_file "${project_rcc_qmlcache}/aotstats_empty_modules.txt")
        list(JOIN empty_modules "\n" empty_modules_lines)
        file(WRITE ${empty_modules_file} "${empty_modules_lines}")
        list(APPEND qmlaotstats_options "--empty-modules" "${empty_modules_file}")
    endif()

    list(JOIN module_aotstats_files "\n" module_aotstats_lines)
    set(aotstats_list_file "${project_rcc_qmlcache}/all_aotstats.aotstatslist")
    file(WRITE "${aotstats_list_file}" "${module_aotstats_lines}")

    set(all_aotstats_file "${project_rcc_qmlcache}/all_aotstats.aotstats")
    set(formatted_stats_file "${project_rcc_qmlcache}/all_aotstats.txt")

    _qt_internal_get_tool_wrapper_script_path(tool_wrapper)
    add_custom_command(
        OUTPUT
            "${all_aotstats_file}"
            "${formatted_stats_file}"
        DEPENDS ${module_aotstats_targets} ${module_aotstats_files}
        COMMAND
            ${tool_wrapper}
            $<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::qmlaotstats>
            aggregate
            "${aotstats_list_file}"
            "${all_aotstats_file}"
        COMMAND
            ${tool_wrapper}
            $<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::qmlaotstats>
            format
            "${all_aotstats_file}"
            "${formatted_stats_file}"
            "${qmlaotstats_options}"
        COMMAND_EXPAND_LISTS
    )

    if(NOT TARGET all_aotstats)
        add_custom_target(all_aotstats
            DEPENDS "${formatted_stats_file}"
            COMMAND "${CMAKE_COMMAND}" -E cat "${formatted_stats_file}"
        )
    endif()
endfunction()

function(_qt_internal_list_to_ini list_var)
    list(REMOVE_DUPLICATES ${list_var})

    if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
        # replace cmake list separator ';' with unix path separator ':'
        list(JOIN ${list_var} ":" result)
    else()
        # cmake list separator and windows path separator are both ';', so no replacement needed
        set(result "${${list_var}}")
    endif()
    set(${list_var} "${result}" PARENT_SCOPE)
endfunction()

function(_qt_internal_write_deferred_qmlls_build_ini_file qt_cmake_export_namespace)
    # _qt_internal_write_deferred_qmlls_build_ini_file is deferred to be called in the root
    # CMAKE_BINARY_DIR. If find_package(Qt6) is not called in the root of the project (like in the
    # Qt Creator super repo), QT_CMAKE_EXPORT_NAMESPACE will not be defined.
    # Manually define it here, based on the value passed to the function, from a scope where it
    # is available.
    set(QT_CMAKE_EXPORT_NAMESPACE "${qt_cmake_export_namespace}")

    get_property(_qmlls_build_ini_targets GLOBAL PROPERTY _qmlls_build_ini_targets)
    list(REMOVE_DUPLICATES _qmlls_build_ini_targets)

    set(qmlls_build_ini_file "${CMAKE_BINARY_DIR}/.qt/.qmlls.build.ini")
    add_custom_command(
        OUTPUT
            ${qmlls_build_ini_file}
        COMMAND ${CMAKE_COMMAND} -E echo "[General]" > ${qmlls_build_ini_file}
        COMMENT "Populating .qmlls.ini file at ${qmlls_build_ini_file}"
        VERBATIM
    )
    # qtpaths only supports --query when the settings config is enabled
    if(QT_FEATURE_settings)
        add_custom_command(
        OUTPUT
            ${qmlls_build_ini_file}
        COMMAND ${CMAKE_COMMAND} -E echo_append "docDir=" >> ${qmlls_build_ini_file}
        COMMAND
            ${tool_wrapper}
            $<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::qtpaths>
            --query QT_INSTALL_DOCS >> ${qmlls_build_ini_file}
            APPEND
        )
    endif()

    foreach(current_target IN LISTS _qmlls_build_ini_targets)
        # prepare import paths
        _qt_internal_collect_qml_import_paths(_import_paths ${current_target})

        # Note that standalone builds will have the installation path twice in _import_paths: _qt_internal_list_to_ini
        # takes care of removing these duplicates.
        _qt_internal_list_to_ini(_import_paths)

        # prepare source paths: replace / with <SLASH> as .ini files do not support / in group names
        get_target_property(source_path "${current_target}" SOURCE_DIR)
        string(REPLACE "/" "<SLASH>" source_path "${source_path}")

        add_custom_command(
            OUTPUT
                ${qmlls_build_ini_file}
            COMMAND ${CMAKE_COMMAND} -E
                echo "[${source_path}]" >> ${qmlls_build_ini_file}
            COMMAND ${CMAKE_COMMAND} -E
                echo "importPaths=\"${_import_paths}\"" >> ${qmlls_build_ini_file}
            APPEND
        )
    endforeach()

    add_custom_target(generate_qmlls_build_ini_file
        DEPENDS ${qmlls_build_ini_file}
        VERBATIM
    )
    foreach(current_target IN LISTS _qmlls_build_ini_targets)
        add_dependencies(${current_target} generate_qmlls_build_ini_file)
    endforeach()
endfunction()

function(_qt_internal_write_deferred_qmlls_ini_file target)
    set(qmlls_ini_file "${CMAKE_CURRENT_SOURCE_DIR}/.qmlls.ini")

    get_directory_property(_qmlls_ini_build_folders _qmlls_ini_build_folders)
    _qt_internal_list_to_ini(_qmlls_ini_build_folders)

    get_directory_property(_qmlls_ini_import_path_targets _qmlls_ini_import_path_targets)
    set(_import_paths "")
    foreach(import_path_target IN LISTS _qmlls_ini_import_path_targets)
        get_target_property(import_path ${import_path_target} QT_QML_IMPORT_PATH)
        list(APPEND _import_paths "${import_path}")
    endforeach()

    _qt_internal_get_main_qt_qml_import_paths(installation_paths)
    list(APPEND _import_paths ${installation_paths})
    _qt_internal_list_to_ini(_import_paths)

    _populate_qmlls_ini_file(
        ${target}
        "${qmlls_ini_file}"
        "${_qmlls_ini_build_folders}"
        "${_import_paths}")
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_add_qml_module)
        qt6_add_qml_module(${ARGV})
        cmake_parse_arguments(PARSE_ARGV 1 arg "" "OUTPUT_TARGETS" "")
        if(arg_OUTPUT_TARGETS)
            set(${arg_OUTPUT_TARGETS} ${${arg_OUTPUT_TARGETS}} PARENT_SCOPE)
        endif()
    endfunction()
endif()

function(_populate_qmlls_ini_file target qmlls_ini_file concatenated_build_dirs import_paths)
    set(qtpaths $<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::qtpaths>)
    _qt_internal_get_tool_wrapper_script_path(tool_wrapper)

    string(REPLACE "\"" "\\\"" concatenated_build_dirs "${concatenated_build_dirs}")
    string(REPLACE "\"" "\\\"" import_paths "${import_paths}")

    if(QT_QML_GENERATE_QMLLS_INI_NO_CMAKE_CALLS)
        set(no_cmake_calls "true")
    else()
        set(no_cmake_calls "false")
    endif()

    set(qmlls_ini_build_file "${CMAKE_CURRENT_BINARY_DIR}/.qt/qmlls.ini.build")

    #note: delete this file to mark .qmlls.ini as outdated
    set(qmlls_ini_timestamp_file
        "${CMAKE_CURRENT_BINARY_DIR}/.qt/qmlls.ini.timestamp")

    # generate the .qmlls.ini file content in the build folder
    add_custom_command(
        OUTPUT
            ${qmlls_ini_build_file}
        COMMAND ${CMAKE_COMMAND} -E echo "[General]" > ${qmlls_ini_build_file}
        COMMAND ${CMAKE_COMMAND} -E echo "buildDir=\"${concatenated_build_dirs}\""
                    >> ${qmlls_ini_build_file}
        COMMAND ${CMAKE_COMMAND} -E echo "no-cmake-calls=${no_cmake_calls}"
                    >> ${qmlls_ini_build_file}
        COMMAND ${CMAKE_COMMAND} -E echo_append "docDir=" >> ${qmlls_ini_build_file}
        COMMAND
            ${tool_wrapper}
            ${qtpaths}
            --query QT_INSTALL_DOCS >> ${qmlls_ini_build_file}
        COMMAND ${CMAKE_COMMAND} -E echo "importPaths=\"${import_paths}\""
                >> ${qmlls_ini_build_file}
        COMMENT "Populating .qmlls.ini file at ${qmlls_ini_build_file}"
        VERBATIM
    )
    # delete timestamps from previous configuration, if available, then copy qmlls to source folder and create timestamp
    add_custom_command(
        OUTPUT
            ${qmlls_ini_timestamp_file}
        BYPRODUCTS
            ${qmlls_ini_file}
        DEPENDS
            ${qmlls_ini_build_file}
            COMMAND ${CMAKE_COMMAND}
                        -DqmllsIniPath=${qmlls_ini_file}
                        -P "${__qt_qml_macros_module_base_dir}/Qt6UpdateQmllsIni.cmake"
        COMMAND ${CMAKE_COMMAND} -E copy ${qmlls_ini_build_file} ${qmlls_ini_file}
        COMMAND ${CMAKE_COMMAND} -E touch ${qmlls_ini_timestamp_file}
        COMMENT "Copying .qmlls.ini file at ${qmlls_ini_build_file} to ${qmlls_ini_file}"
        VERBATIM
    )
    add_custom_target(${target}_generate_qmlls_ini_file
        DEPENDS ${qmlls_ini_timestamp_file}
        VERBATIM
    )
    add_dependencies(${target} ${target}_generate_qmlls_ini_file)
endfunction()

# Make the prefix conform to the following:
#   - Starts with a "/"
#   - Does not end with a "/" unless the prefix is exactly "/"
function(_qt_internal_canonicalize_resource_path path out_var)
    if(NOT path)
        set(path "/")
    endif()
    if(NOT path MATCHES "^/")
        string(PREPEND path "/")
    endif()
    if(path MATCHES [[(.+)/$]])
        set(path "${CMAKE_MATCH_1}")
    endif()
    set(${out_var} "${path}" PARENT_SCOPE)
endfunction()

function(_qt_internal_get_escaped_uri uri out_var)
    string(REGEX REPLACE "[^A-Za-z0-9]" "_" escaped_uri "${uri}")
    set(${out_var} "${escaped_uri}" PARENT_SCOPE)
endfunction()

function(_qt_internal_compute_qml_plugin_class_name_from_uri uri out_var)
    _qt_internal_get_escaped_uri("${uri}" escaped_uri)
    set(${out_var} "${escaped_uri}Plugin" PARENT_SCOPE)
endfunction()

macro(_qt_internal_genex_getproperty var target property)
    set(${var} "$<TARGET_PROPERTY:${target},${property}>")
    set(have_${var} "$<BOOL:${${var}}>")
endmacro()

macro(_qt_internal_genex_getjoinedproperty var target property item_prefix glue)
    _qt_internal_genex_getproperty(${var} ${target} ${property})
    set(${var} "$<${have_${var}}:${item_prefix}$<JOIN:${${var}},${glue}${item_prefix}>>")
endmacro()


# Creates a genex that will call a c++ macro on each of the list values.
# Handles empty lists.
macro(_qt_internal_genex_get_list_joined_with_macro var input_list macro_name with_ending_semicolon)
    set(${var} "${input_list}")
    set(have_${var} "$<BOOL:${${var}}>")

    set(_macro_begin "${macro_name}(")
    set(_macro_end ")")
    if(with_ending_semicolon)
        string(APPEND _macro_end ";")
    endif()
    set(_macro_glue "${_macro_end}\n${_macro_begin}")

    string(JOIN "" ${var}
        "${_macro_begin}"
        "$<JOIN:${${var}},${_macro_glue}>"
        "${_macro_end}")

    set(${var} "$<${have_${var}}:${${var}}>")

    unset(_macro_begin)
    unset(_macro_end)
    unset(_macro_glue)
endmacro()

# Reads a target property that contains a list of values and creates a genex that will
# call a c++ macro on each of the values.
# Handles empty properties.
macro(_qt_internal_genex_get_property_joined_with_macro var target property macro_name
        with_ending_semicolon)
    _qt_internal_genex_getproperty(${var} ${target} ${property})
    _qt_internal_genex_get_list_joined_with_macro("${var}" "${${var}}" "${macro_name}"
        "${with_ending_semicolon}")
endmacro()

macro(_qt_internal_genex_getoption var target property)
    set(${var} "$<BOOL:$<TARGET_PROPERTY:${target},${property}>>")
endmacro()

# Gets the Qt import paths, prepends -I, appends the values to the given import_paths_var output
# variable.
function(_qt_internal_extend_qml_import_paths import_paths_var)
    set(local_var ${${import_paths_var}})

    _qt_internal_get_main_qt_qml_import_paths(qt_import_paths)

    # Append the paths instead of prepending them, to ensure Qt import paths are searched after
    # any target or build dir specific import paths.
    foreach(import_path IN LISTS qt_import_paths)
        list(APPEND local_var -I "${import_path}")
    endforeach()

    set(${import_paths_var} ${local_var} PARENT_SCOPE)
endfunction()

function(_qt_internal_assign_to_qmllint_targets_folder target)
    get_property(folder_name GLOBAL PROPERTY QT_QMLLINTER_TARGETS_FOLDER)
    if("${folder_name}" STREQUAL "")
        get_property(__qt_qt_targets_folder GLOBAL PROPERTY QT_TARGETS_FOLDER)
        set(folder_name "${__qt_qt_targets_folder}/QmlLinter")
        set_property(GLOBAL PROPERTY QT_QMLLINTER_TARGETS_FOLDER ${folder_name})
    endif()
    set_property(TARGET ${target} PROPERTY FOLDER "${folder_name}")
endfunction()

function(_qt_internal_target_enable_qmllint target)
    set(lint_target ${target}_qmllint)
    set(lint_target_json ${target}_qmllint_json)
    set(lint_target_module ${target}_qmllint_module)
    if(TARGET ${lint_target} OR TARGET ${target}_qmllint_json OR TARGET ${target}_qmllint_module)
        return()
    endif()

    _qt_internal_genex_getproperty(qmllint_files ${target} QT_QML_LINT_FILES)
    _qt_internal_genex_getjoinedproperty(qrc_args ${target}
        _qt_generated_qrc_files "--resource$<SEMICOLON>" "$<SEMICOLON>"
    )

    _qt_internal_collect_qml_import_paths(import_args ${target})
    list(TRANSFORM import_args PREPEND "-I\n")

    _qt_internal_get_tool_wrapper_script_path(tool_wrapper)

    set(qmllint_args
        --bare
        ${import_args}
        ${qrc_args}
        ${qmllint_files}
    )

    get_target_property(target_binary_dir ${target} BINARY_DIR)
    set(qmllint_dir ${target_binary_dir}/.rcc/qmllint)
    set(qmllint_rsp_path ${qmllint_dir}/${target}.rsp)

    file(GENERATE
        OUTPUT "${qmllint_rsp_path}"
        CONTENT "$<JOIN:${qmllint_args},\n>\n"
    )

    set(cmd
       "${tool_wrapper}"
        $<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::qmllint>
        @${qmllint_rsp_path}
    )

    set(cmd_dummy ${CMAKE_COMMAND} -E echo "Nothing to do for target ${lint_target}.")

    # Enable an easy way to always run qmllint on examples, without having to modify
    # the examples themselves
    set(lint_target_part_of_all "")
    if(QT_LINT_EXAMPLES)
        get_target_property(target_source_dir ${target} SOURCE_DIR)
        string(FIND "${target_source_dir}" "examples" examples_position)
        if (NOT (examples_position EQUAL -1))
            set(lint_target_part_of_all ALL)
        endif()
    endif()

    # We need this target to depend on all qml type registrations. This is the
    # only way we can be sure that all *.qmltypes files for any QML modules we
    # depend on will have been generated.
    add_custom_target(${lint_target} ${lint_target_part_of_all}
        COMMAND "$<IF:${have_qmllint_files},${cmd},${cmd_dummy}>"
        COMMAND_EXPAND_LISTS
        DEPENDS
            ${QT_CMAKE_EXPORT_NAMESPACE}::qmllint
            ${qmllint_files}
            ${qmllint_rsp_path}
            $<TARGET_NAME_IF_EXISTS:all_qmltyperegistrations>
        WORKING_DIRECTORY "$<TARGET_PROPERTY:${target},SOURCE_DIR>"
    )
    _qt_internal_assign_to_qmllint_targets_folder(${lint_target})

    list(APPEND qmllint_args "--json" "${CMAKE_BINARY_DIR}/${lint_target}.json")

    set(qmllint_rsp_path ${qmllint_dir}/${target}_json.rsp)

    file(GENERATE
        OUTPUT "${qmllint_rsp_path}"
        CONTENT "$<JOIN:${qmllint_args},\n>\n"
    )

    set(cmd
        "${tool_wrapper}"
        $<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::qmllint>
        @${qmllint_rsp_path}
    )

    add_custom_target(${lint_target_json}
        COMMAND "$<${have_qmllint_files}:${cmd}>"
        COMMAND_EXPAND_LISTS
        DEPENDS
            ${QT_CMAKE_EXPORT_NAMESPACE}::qmllint
            ${qmllint_files}
            ${qmllint_rsp_path}
            $<TARGET_NAME_IF_EXISTS:all_qmltyperegistrations>
        WORKING_DIRECTORY "$<TARGET_PROPERTY:${target},SOURCE_DIR>"
    )
    _qt_internal_assign_to_qmllint_targets_folder(${lint_target_json})

   set_target_properties(${lint_target_json} PROPERTIES EXCLUDE_FROM_ALL TRUE)

   get_target_property(module_uri ${target} QT_QML_MODULE_URI)

   _qt_internal_get_tool_wrapper_script_path(tool_wrapper)

   set(qmllint_args
       ${import_args}
       ${qrc_args}
       --module
       ${module_uri}
   )

   set(qmllint_rsp_path ${qmllint_dir}/${target}_module.rsp)

   file(GENERATE
       OUTPUT "${qmllint_rsp_path}"
       CONTENT "$<JOIN:${qmllint_args},\n>\n"
   )

   set(cmd
       "${tool_wrapper}"
       $<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::qmllint>
       @${qmllint_rsp_path}
   )

   add_custom_target(${lint_target_module}
       COMMAND ${cmd}
       COMMAND_EXPAND_LISTS
       DEPENDS
           ${QT_CMAKE_EXPORT_NAMESPACE}::qmllint
           ${qmllint_files}
           ${qmllint_rsp_path}
           $<TARGET_NAME_IF_EXISTS:all_qmltyperegistrations>
       WORKING_DIRECTORY "$<TARGET_PROPERTY:${target},SOURCE_DIR>"
   )
    _qt_internal_assign_to_qmllint_targets_folder(${lint_target_module})

    # Make the global linting target depend on the one we add here.
    # Note that the caller is free to change the value of QT_QMLLINT_ALL_TARGET
    # for different QML modules if they wish, which means they can implement
    # their own grouping of the ${target}_qmllint targets.
    _qt_internal_add_all_qmllint_target(QT_QMLLINT_ALL_TARGET
        all_qmllint ${lint_target})
    _qt_internal_add_all_qmllint_target(QT_QMLLINT_JSON_ALL_TARGET
        all_qmllint_json ${lint_target_json})
    _qt_internal_add_all_qmllint_target(QT_QMLLINT_MODULE_ALL_TARGET
        all_qmllint_module ${lint_target_module})
endfunction()

# Create an 'all_qmllint' target. The target's name can be user-controlled by ${target_var} with the
# default name ${default_target_name}. The parameter ${lint_target} holds the name of the single
# foo_qmllint target that should be triggered by the all_qmllint target.
function(_qt_internal_add_all_qmllint_target target_var default_target_name lint_target)
    set(target_name "${${target_var}}")
    if("${target_name}" STREQUAL "")
        set(target_name ${default_target_name})
    endif()
    _qt_internal_add_phony_target(${target_name}
        WARNING_VARIABLE QT_NO_QMLLINT_CREATION_WARNING
        TARGET_CREATED_HOOK _qt_internal_assign_to_qmllint_targets_folder
    )
    _qt_internal_add_phony_target_dependencies(${target_name} ${lint_target})
endfunction()

# Hack for the Visual Studio generator. Create the all_qmllint target named ${target} and work
# around the lack of a working add_dependencies by calling 'cmake --build' for every dependency.
function(_qt_internal_add_all_qmllint_target_deferred target)
    get_property(target_dependencies GLOBAL PROPERTY _qt_target_${target}_dependencies)
    set(target_commands "")
    foreach(dependency IN LISTS target_dependencies)
        list(APPEND target_commands
            COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}" -t ${dependency}
        )
    endforeach()
    add_custom_target(${target} ${target_commands})
    _qt_internal_assign_to_qmllint_targets_folder(${target})
endfunction()

function(_qt_internal_target_enable_qmlcachegen target qmlcachegen)
    set_target_properties(${target} PROPERTIES _qt_cachegen_set_up TRUE)

    get_target_property(target_binary_dir ${target} BINARY_DIR)
    set(qmlcache_dir ${target_binary_dir}/.rcc/qmlcache)
    set(qmlcache_resource_name qmlcache_${target})

    # Save the resource name in a property so we can reference it later in a qml plugin
    # constructor, to avoid discarding the resource if it's in a static library.
    __qt_internal_sanitize_resource_name(
        sanitized_qmlcache_resource_name "${qmlcache_resource_name}")
    set_target_properties(${target} PROPERTIES _qt_cachegen_sanitized_resource_name
        "${sanitized_qmlcache_resource_name}")

    # INTEGRITY_SYMBOL_UNIQUENESS
    # The cache loader file name has to be unique, because the Integrity compiler uses the file name
    # for the generation of the translation unit static constructor symbol name.
    #    e.g. __sti___19_qmlcache_loader_cpp_11acedbd
    # For some reason the symbol is created with global visibility.
    #
    # When an application links against the Basic and Fusion static qml plugins, the linker
    # fails with duplicate symbol errors because both of those plugins will contain the same symbol.
    #
    # With gcc on regular Linux, the symbol names are also the same, but it's not a problem because
    # they have local (hidden) visbility.
    #
    # Make the file name unique by prepending the target name.
    set(qmlcache_loader_cpp ${qmlcache_dir}/${target}_qmlcache_loader.cpp)

    set(qmlcache_loader_list ${qmlcache_dir}/${target}_qml_loader_file_list.rsp)
    set(qmlcache_resource_paths "$<TARGET_PROPERTY:${target},QT_QML_MODULE_RESOURCE_PATHS>")

    _qt_internal_genex_getjoinedproperty(qrc_resource_args ${target}
        _qt_generated_qrc_files "--resource$<SEMICOLON>" "$<SEMICOLON>"
    )

    if(CMAKE_GENERATOR STREQUAL "Ninja Multi-Config" AND CMAKE_VERSION VERSION_GREATER_EQUAL "3.20")
        set(qmlcachegen "$<COMMAND_CONFIG:${qmlcachegen}>")
    endif()

    _qt_internal_get_tool_wrapper_script_path(tool_wrapper)
    set(cmd
        "${tool_wrapper}"
        ${qmlcachegen}
        --resource-name "${qmlcache_resource_name}"
        -o "${qmlcache_loader_cpp}"
        "@${qmlcache_loader_list}"
    )

    file(GENERATE
        OUTPUT ${qmlcache_loader_list}
        CONTENT "$<JOIN:${qrc_resource_args},\n>\n$<JOIN:${qmlcache_resource_paths},\n>\n"
    )

    add_custom_command(
        OUTPUT ${qmlcache_loader_cpp}
        COMMAND "${cmd}"
        COMMAND_EXPAND_LISTS
        DEPENDS
            ${qmlcachegen}
            ${qmlcache_loader_list}
            $<TARGET_PROPERTY:${target},_qt_generated_qrc_files>
        VERBATIM
    )
    # We can't rely on policy CMP0118 since user project controls it
    set(scope_args)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
        set(scope_args TARGET_DIRECTORY ${target})
    endif()

    _qt_internal_set_source_file_generated(
        SOURCES ${qmlcache_loader_cpp}
        ${scope_args}
        SKIP_AUTOGEN
    )
    get_target_property(target_source_dir ${target} SOURCE_DIR)
    if(NOT target_source_dir STREQUAL CMAKE_CURRENT_SOURCE_DIR)
        add_custom_target(${target}_qmlcachegen DEPENDS ${qmlcache_loader_cpp})
        add_dependencies(${target} ${target}_qmlcachegen)
    endif()

    # TODO: Probably need to reject ${target} being an object library as unsupported
    target_sources(${target} PRIVATE "${qmlcache_loader_cpp}")
    target_link_libraries(${target} PRIVATE
        ${QT_CMAKE_EXPORT_NAMESPACE}::Qml
        ${QT_CMAKE_EXPORT_NAMESPACE}::Core
    )
endfunction()

# We cannot defer writing out the qmldir file to generation time because the
# qmlimportscanner runs at configure time as part of target finalizers.
# Therefore, the best we can do is defer writing the qmldir file if we are
# using a recent enough CMake version, otherwise we write it out progressively
# on each call that adds qml sources. The immediate progressive writes will
# trigger some unnecessary rebuilds after reconfiguring due to the qmldir
# file's timestamp being updated even though its contents might not change,
# but that's the cost of not having deferred write capability.
function(_qt_internal_target_generate_qmldir target)

    macro(_qt_internal_qmldir_item prefix property)
        get_target_property(_value ${target} ${property})
        if(_value)
            string(APPEND content "${prefix} ${_value}\n")
        endif()
    endmacro()

    macro(_qt_internal_qmldir_item_list prefix property)
        get_target_property(_values ${target} ${property})
        if(_values)
            foreach(_value IN LISTS _values)
                string(APPEND content "${prefix} ${_value}\n")
            endforeach()
        endif()
    endmacro()

    get_target_property(uri ${target} QT_QML_MODULE_URI)
    if(NOT uri)
        message(FATAL_ERROR "Target ${target} has no URI set, cannot create qmldir")
    endif()
    set(content "module ${uri}\n")

    _qt_internal_qmldir_item(linktarget QT_QML_MODULE_INSTALLED_PLUGIN_TARGET)

    get_target_property(plugin_target ${target} QT_QML_MODULE_PLUGIN_TARGET)
    if(plugin_target)
        get_target_property(no_plugin_optional ${target} QT_QML_MODULE_NO_PLUGIN_OPTIONAL)
        if(NOT no_plugin_optional MATCHES "NOTFOUND" AND NOT no_plugin_optional)
            string(APPEND content "optional ")
        endif()

        get_target_property(target_path ${target} QT_QML_MODULE_TARGET_PATH)
        _qt_internal_get_qml_plugin_output_name(plugin_output_name ${plugin_target}
            TARGET_PATH "${target_path}"
            URI "${uri}"
        )
        string(APPEND content "plugin ${plugin_output_name}\n")

        _qt_internal_qmldir_item(classname QT_QML_MODULE_CLASS_NAME)
    endif()

    get_target_property(designer_supported ${target} QT_QML_MODULE_DESIGNER_SUPPORTED)
    if(designer_supported)
        string(APPEND content "designersupported\n")
    endif()

    get_target_property(static_module ${target} QT_QML_MODULE_IS_STATIC)
    if (static_module)
       string(APPEND content "static\n")
    endif()

    get_target_property(system_module ${target} QT_QML_MODULE_IS_SYSTEM)
    if (system_module)
       string(APPEND content "system\n")
    endif()

    _qt_internal_qmldir_item(typeinfo QT_QML_MODULE_TYPEINFO)

    _qt_internal_qmldir_item_list(import QT_QML_MODULE_IMPORTS)
    _qt_internal_qmldir_item_list("optional import" QT_QML_MODULE_OPTIONAL_IMPORTS)
    _qt_internal_qmldir_item_list("default import" QT_QML_MODULE_DEFAULT_IMPORTS)

    _qt_internal_qmldir_item_list(depends QT_QML_MODULE_DEPENDENCIES)

    get_target_property(prefix ${target} QT_QML_MODULE_RESOURCE_PREFIX)
    if(prefix)
        # Ensure we use a path that ends with a "/", but handle the special case
        # of "/" without anything after it
        if(NOT prefix STREQUAL "/" AND NOT prefix MATCHES "/$")
            string(APPEND prefix "/")
        endif()
        string(APPEND content "prefer :${prefix}\n")
    endif()

    # TODO: What about multi-config generators? Would we need per-config qmldir
    #       files (because we will have per-config plugin targets)?

    # Record the contents but defer the actual write. We will write the file
    # later, either at the end of qt6_add_qml_module() or the end of the
    # directory scope (depending on the CMake version being used).
    set_property(TARGET ${target} PROPERTY _qt_internal_qmldir_content "${content}")

    # NOTE: qt6_target_qml_sources() may append further content later.
endfunction()

function(_qt_internal_write_deferred_qmldir_file target)
    get_target_property(__qt_qmldir_content ${target} _qt_internal_qmldir_content)
    # User convenience: Add QtQuick to dependencies if it hasn't been added so far
    # and the target links against  Qt::Quick
    # Must happen here, because _qt_internal_target_generate_qmldir runs at
    # qt_add_qml_module time, likely before target_link_libraries has been
    # called
    set(has_qtquick_dependency FALSE)
    get_target_property(module_dependencies ${target} QT_QML_MODULE_DEPENDENCIES)
    if(module_dependencies)
        foreach(dependency IN LISTS module_dependencies)
            if(dependency STREQUAL "QtQuick" OR dependency MATCHES "^QtQuick ")
                set(has_qtquick_dependency TRUE)
                break()
            endif()
        endforeach()
    endif()
    if(NOT has_qtquick_dependency)
        get_target_property(linked_libraries ${target} LINK_LIBRARIES)
        if((TARGET Qt6::Quick AND Qt6::Quick IN_LIST linked_libraries) OR
            (TARGET Qt::Quick AND Qt::Quick IN_LIST linked_libraries))
            string(APPEND __qt_qmldir_content "depends QtQuick\n")
        endif()
    endif()
    get_target_property(out_dir ${target} QT_QML_MODULE_OUTPUT_DIRECTORY)
    set(qmldir_file "${out_dir}/qmldir")
    configure_file(${__qt_qml_macros_module_base_dir}/Qt6qmldirTemplate.cmake.in ${qmldir_file} @ONLY)
endfunction()

# With a macOS framework Qt build, moc needs to be passed -F<qt-framework-path>
# arguments to resolve framework style includes like #include <QtCore/qobject.h>
# Extract the location of the Qt frameworks by querying the imported location of
# the target (where target is a Qt library). Do not care about non-Qt targets.
function(_qt_internal_qml_get_qt_framework_path target out_var)
    set(value "")
    # NOTE: only exercise IMPORTED_LOCATION of various flavors. this seems to be
    # good enough in other places (e.g. when locating qmlimportscanner)
    get_target_property(target_path ${target} IMPORTED_LOCATION)
    if(NOT target_path)
        set(configs "RELWITHDEBINFO;RELEASE;MINSIZEREL;DEBUG")
        foreach(config ${configs})
            get_target_property(target_path ${target} IMPORTED_LOCATION_${config})
            # NOTE: to be fair, any location is good enough. the macro
            # definitions we need must not vary between configurations
            if(target_path)
                break()
            endif()
        endforeach()
    endif()
    string(REGEX REPLACE "(.*)/Qt[^/]+\\.framework.*" "\\1" target_fw_path "${target_path}")
    if(target_fw_path)
        set(value "${target_fw_path}")
    endif()
    set(${out_var} "${value}" PARENT_SCOPE)
endfunction()

function(_qt_internal_qml_get_qt_framework_path_moc_option target out_var)
    _qt_internal_qml_get_qt_framework_path(${target} target_fw_path)
    if(target_fw_path)
        set(${out_var} "-F${target_fw_path}" PARENT_SCOPE)
    else()
        set(${out_var} "" PARENT_SCOPE)
    endif()
endfunction()

# creates a QRC mapping between QML files in build directory and QML files in
# source directory
function(_qt_internal_qml_map_build_files target qml_module_prefix qrc_file_out_var)
    get_target_property(output_dir ${target} QT_QML_MODULE_OUTPUT_DIRECTORY)
    if(NOT output_dir)
        # TODO: we might want to support non-qml modules here (think QML
        # language server) but it is unclear whether the below code would
        # succeed, so just abort here until we have a proper test case
        message(WARNING "Target ${target} is not a QML module. Cannot detect its output directory")
        return()
    endif()

    set(qrcContents "")
    string(APPEND qrcContents "    <file alias=\"${qml_module_prefix}\">${output_dir}</file>\n")

    # dump the contents into the .qrc file
    set(template_file "${__qt_qml_macros_module_base_dir}/Qt6QmlModuleDirMappingTemplate.qrc.in")
    set(generated_qrc_file "${output_dir}/${target}_qml_module_dir_map.qrc")
    set(qt_qml_module_dir_mapping_contents "${qrcContents}")
    configure_file(${template_file} ${generated_qrc_file})

    set(${qrc_file_out_var} ${generated_qrc_file} PARENT_SCOPE)
endfunction()

# Creates a qrc file that maps QML file path to generated C++ header file name
function(_qt_internal_qml_add_qmltc_file_mapping_resource qrc_file target qml_files)
    get_target_property(output_dir ${target} QT_QML_MODULE_OUTPUT_DIRECTORY)
    set(qrcContents "")

    # go over all QML_FILES, mapping QML files to generated C++ sources
    foreach(qml_file IN LISTS qml_files)
        get_filename_component(file_absolute ${qml_file} ABSOLUTE)
        get_filename_component(file_basename ${file_absolute} NAME_WLE) # extension is always .qml
        string(REGEX REPLACE "[$#?]+" "_" compiled_file ${file_basename})

        # NB: use <lowercase(file_name)>.<extension> pattern. if
        # lowercase(file_name) is already taken (e.g. project has main.qml and
        # main.h/main.cpp), the compilation might fail. in this case, expect
        # user to specify QT_QMLTC_FILE_BASENAME
        string(TOLOWER ${compiled_file} file_name)

        get_source_file_property(specified_file_name ${qml_file} QT_QMLTC_FILE_BASENAME)
        if (specified_file_name)
            get_filename_component(file_name ${specified_file_name} NAME_WLE)
        endif()

        # <file_name>.h is enough as corresponding .cpp is not interesting (and
        # we can get it easily by replacing the extension)
        string(APPEND qrcContents "    <file alias=\"${file_name}.h\">${file_absolute}</file>\n")
    endforeach()

    # dump the contents into the .qrc file
    set(template_file "${__qt_qml_macros_module_base_dir}/Qt6QmltcFileMappingTemplate.qrc.in")
    set(generated_qrc_file "${output_dir}/.qmltc/${target}/${target}_qmltc_file_map.qrc")
    set(qt_qml_qmltc_file_mapping_contents "${qrcContents}")
    configure_file(${template_file} ${generated_qrc_file})

    set(${qrc_file} ${generated_qrc_file} PARENT_SCOPE)
endfunction()

# Compile Qml files (.qml) to C++ source files with QML type compiler (qmltc).
function(_qt_internal_target_enable_qmltc target)
    set(args_option "")
    set(args_single NAMESPACE EXPORT_MACRO_NAME EXPORT_FILE_NAME MODULE)
    set(args_multi QML_FILES IMPORT_PATHS)

    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${args_option}" "${args_single}" "${args_multi}"
    )
    if(arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown/unexpected arguments: ${arg_UNPARSED_ARGUMENTS}")
    endif()

    if (NOT arg_QML_FILES)
        message(FATAL_ERROR "FILES option not given or contains empty list for target ${target}")
    endif()

    if(NOT TARGET "${target}")
        message(FATAL_ERROR "\"${target}\" is not a known target")
    endif()

    get_target_property(target_source_dir ${target} SOURCE_DIR)
    get_target_property(target_binary_dir ${target} BINARY_DIR)

    set(generated_sources_other_scope)

    set(compiled_files) # compiled files list to be used to generate MOC C++
    set(qmltc_executable "$<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::qmltc>")
    if(CMAKE_GENERATOR STREQUAL "Ninja Multi-Config" AND CMAKE_VERSION VERSION_GREATER_EQUAL "3.20")
        set(qmltc_executable "$<COMMAND_CONFIG:${qmltc_executable}>")
    endif()

    set(common_args "")
    if(arg_NAMESPACE)
        list(APPEND common_args --namespace "${arg_NAMESPACE}")
    endif()
    if(arg_MODULE)
        list(APPEND common_args --module "${arg_MODULE}")
    endif()
    if(arg_EXPORT_MACRO_NAME)
        list(APPEND common_args --export "${arg_EXPORT_MACRO_NAME}")
        if(arg_EXPORT_FILE_NAME)
            list(APPEND common_args --exportInclude "${arg_EXPORT_FILE_NAME}")
        endif()
    endif()

    get_target_property(output_dir ${target} QT_QML_MODULE_OUTPUT_DIRECTORY)
    set(qmldir_file ${output_dir}/qmldir)
    # TODO: we still need to specify the qmldir here for _explicit_ imports of
    # own module. in theory this could be pushed to the user side
    list(APPEND common_args "-i" ${qmldir_file})

    foreach(import_path IN LISTS arg_IMPORT_PATHS)
        list(APPEND common_args -I "${import_path}")
    endforeach()

    _qt_internal_extend_qml_import_paths(common_args)

    # we explicitly depend on qmldir (due to `-i ${qmldir_file}`) but also
    # implicitly on the generated qmltypes file, which is a part of qmldir
    set(qml_module_files)
    list(APPEND qml_module_files ${qmldir_file})
    get_target_property(qmltypes_file ${target} QT_QML_MODULE_TYPEINFO)
    if(qmltypes_file)
        list(APPEND qml_module_files ${output_dir}/${qmltypes_file})
    endif()

    get_target_property(potential_qml_modules ${target} LINK_LIBRARIES)
    foreach(lib ${potential_qml_modules})
        if(NOT TARGET ${lib})
            continue()
        endif()

        # if we have a versionless Qt lib, find the public one with a version
        if(lib MATCHES "^Qt::(.*)")
            set(lib "${CMAKE_MATCH_1}")
            if(lib MATCHES "^(.*)Private") # remove "Private"
                set(lib "${CMAKE_MATCH_1}")
            endif()
            set(lib ${QT_CMAKE_EXPORT_NAMESPACE}::${lib})
            if(NOT TARGET ${lib})
                continue()
            endif()
        endif()

        # when we have a suitable lib, ignore INTERFACE_LIBRARY and IMPORTED
        get_target_property(lib_type ${lib} TYPE)
        get_target_property(lib_is_imported ${lib} IMPORTED)
        if(lib_type STREQUAL "INTERFACE_LIBRARY" OR lib_is_imported)
            continue()
        endif()

        # get any QT_QML_MODULE_ property, this way we can tell whether we deal
        # with QML module target or not. use output dir as it's used later
        get_target_property(external_output_dir ${lib} QT_QML_MODULE_OUTPUT_DIRECTORY)
        if(NOT external_output_dir) # not a QML module, so not interesting
            continue()
        endif()

        get_target_property(external_qmltypes_file ${lib} QT_QML_MODULE_TYPEINFO)
        if(external_qmltypes)
            # add linked module's qmltypes file to a list of target
            # dependencies. unlike qmllint or other tooling, qmltc only cares
            # about explicitly linked libraries. things like plugins are not
            # supported by design and would result in C++ compilation errors
            list(APPEND qml_module_files ${external_output_dir}/${external_qmltypes_file})
        endif()
    endforeach()

    # ignore non-QML and to-be-skipped QML files
    set(filtered_qml_files "")
    foreach(qml_file_src IN LISTS arg_QML_FILES)
        if(NOT qml_file_src MATCHES "\\.(qml)$")
            continue()
        endif()
        get_source_file_property(skip_qmltc ${qml_file_src} QT_QML_SKIP_TYPE_COMPILER)
        if(skip_qmltc)
            continue()
        endif()
        list(APPEND filtered_qml_files ${qml_file_src})
    endforeach()

    # qmltc needs qrc files to supply to the QQmlJSResourceFileMapper
    _qt_internal_genex_getjoinedproperty(qrc_args ${target}
        _qt_generated_qrc_files "--resource$<SEMICOLON>" "$<SEMICOLON>"
    )
    # qmltc also needs meta data qrc files when importing types from own module
    _qt_internal_genex_getjoinedproperty(metadata_qrc_args ${target}
        _qt_qml_meta_qrc_files "--meta-resource$<SEMICOLON>" "$<SEMICOLON>"
    )
    list(APPEND qrc_args ${metadata_qrc_args})

    # NB: pass qml files variable as string to preserve its list nature
    # (otherwise we lose all but first element of the list inside a function)
    _qt_internal_qml_add_qmltc_file_mapping_resource(
        qmltc_file_map_qrc ${target} "${filtered_qml_files}")
    # append --resource <qrc> to qmltc's command line
    list(APPEND qrc_args --resource "${qmltc_file_map_qrc}")

    list(APPEND common_args ${qrc_args})

    foreach(qml_file_src IN LISTS filtered_qml_files)
        get_filename_component(file_absolute ${qml_file_src} ABSOLUTE)

        get_filename_component(file_basename ${file_absolute} NAME_WLE) # extension is always .qml
        string(REGEX REPLACE "[$#?]+" "_" compiled_file ${file_basename})
        string(TOLOWER ${compiled_file} file_name)

        # NB: use <lowercase(file_name)>.<extension> pattern. if
        # lowercase(file_name) is already taken (e.g. project has main.qml and
        # main.h/main.cpp), the compilation might fail. in this case, expect
        # user to specify QT_QMLTC_FILE_BASENAME
        get_source_file_property(specified_file_name ${qml_file_src} QT_QMLTC_FILE_BASENAME)
        if (specified_file_name)
            get_filename_component(file_name ${specified_file_name} NAME_WLE)
        endif()

        # Note: add '${target}' to path to avoid potential conflicts where 2+
        # distinct targets use the same ${target_binary_dir}/.qmltc/ output dir
        set(compiled_header "${target_binary_dir}/.qmltc/${target}/${file_name}.h")
        set(compiled_cpp "${target_binary_dir}/.qmltc/${target}/${file_name}.cpp")
        get_filename_component(out_dir ${compiled_header} DIRECTORY)

        _qt_internal_get_tool_wrapper_script_path(tool_wrapper)
        add_custom_command(
            OUTPUT ${compiled_header} ${compiled_cpp}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${out_dir}
            COMMAND
                ${tool_wrapper}
                ${qmltc_executable}
                --bare
                --header "${compiled_header}"
                --impl "${compiled_cpp}"
                ${common_args}
                ${file_absolute}
            COMMAND_EXPAND_LISTS
            DEPENDS
                ${qmltc_executable}
                "${file_absolute}"
                ${qml_module_files}
                $<TARGET_PROPERTY:${target},_qt_generated_qrc_files>
                $<TARGET_PROPERTY:${target},_qt_qml_meta_qrc_files>
                ${qmltc_file_map_qrc}
            COMMENT "Compiling ${qml_file_src} with qmltc"
            VERBATIM
        )

        _qt_internal_set_source_file_generated(
            SOURCES ${compiled_header} ${compiled_cpp}
            SKIP_AUTOGEN
        )
        set_source_files_properties(${compiled_header} ${compiled_cpp}
            PROPERTIES
                SKIP_UNITY_BUILD_INCLUSION ON)
        target_sources(${target} PRIVATE ${compiled_header} ${compiled_cpp})
        target_include_directories(${target} PUBLIC ${out_dir})
        if(NOT target_source_dir STREQUAL CMAKE_CURRENT_SOURCE_DIR)
            list(APPEND generated_sources_other_scope ${compiled_header} ${compiled_cpp})
        endif()

        list(APPEND compiled_files ${compiled_header})
    endforeach()

    set(extra_moc_options "")
    if(APPLE AND QT_FEATURE_framework)
        # this is a special case, where we need -F options passed to manual moc.
        # since we're in qmltc code, we only ever need to check QtCore and QtQml
        # for framework path
        list(APPEND link_libs ${QT_CMAKE_EXPORT_NAMESPACE}::Core ${QT_CMAKE_EXPORT_NAMESPACE}::Qml)
        foreach(lib ${link_libs})
            _qt_internal_qml_get_qt_framework_path_moc_option(${lib} moc_option)
            if(moc_option)
                list(APPEND extra_moc_options ${moc_option})
            endif()
        endforeach()
    endif()

    # run MOC manually for the generated files
    qt6_wrap_cpp(compiled_moc_files ${compiled_files} TARGET ${target} OPTIONS ${extra_moc_options})

    # We can't rely on policy CMP0118 since user project controls it
    set(scope_args)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
        set(scope_args TARGET_DIRECTORY ${target})
    endif()
    _qt_internal_set_source_file_generated(
        SOURCES ${generated_sources_other_scope} ${compiled_moc_files}
        ${scope_args}
        SKIP_AUTOGEN
    )
    set_source_files_properties(${compiled_moc_files}
        ${scope_args}
        PROPERTIES
            SKIP_UNITY_BUILD_INCLUSION ON
    )
    target_sources(${target} PRIVATE ${compiled_moc_files})
    if(NOT target_source_dir STREQUAL CMAKE_CURRENT_SOURCE_DIR)
        if(NOT TARGET ${target}_tooling)
            message(FATAL_ERROR
                    "${target}_tooling is not found, although it should be in this function.")
        endif()
        # adding sources to ${target}_tooling would ensure that these sources
        # become a dependency of ${target} in this weird case that we have.
        # add_dependencies() for ${target} and ${target}_tooling must have been
        # added as part of qt_add_qml_module() command run.
        target_sources(${target}_tooling PRIVATE
            ${generated_sources_other_scope} ${compiled_moc_files}
        )
    endif()

endfunction()

function(qt6_target_compile_qml_to_cpp target)
    message(WARNING
        "qt6_target_compile_qml_to_cpp() can no longer be used standalone and is planned to be "
        "removed in a future Qt release. To enable qmltc compilation, use:\n"
        "qt6_add_qml_module(${target} ... ENABLE_TYPE_COMPILER)"
    )
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_target_compile_qml_to_cpp)
        qt6_target_compile_qml_to_cpp(${ARGV})
    endfunction()
endif()

# Get extern declaration for types registration function.
function(_qt_internal_qml_get_types_extern_declaration register_types_function_name out_var)
    set(with_ending_semicolon FALSE)
    _qt_internal_genex_get_list_joined_with_macro(
        content
        "${register_types_function_name}"
        "QT_DECLARE_EXTERN_SYMBOL_VOID"
        ${with_ending_semicolon}
    )

    string(PREPEND content "\n")
    set(${out_var} "${content}" PARENT_SCOPE)
endfunction()

# Get code block that should reference the types registration function in the plugin constructor.
# Ensures the symbol is not discarded when linking.
function(_qt_internal_qml_get_types_keep_reference register_types_function_name out_var)
    set(with_ending_semicolon FALSE)
    _qt_internal_genex_get_list_joined_with_macro(
        content
        "${register_types_function_name}"
        "QT_KEEP_SYMBOL"
        ${with_ending_semicolon}
    )

    string(PREPEND content "\n")
    set(${out_var} "${content}" PARENT_SCOPE)
endfunction()

# Get extern declaration for cachegen resource.
function(_qt_internal_qml_get_cachegen_extern_resource_declaration target out_var)
    set(with_ending_semicolon FALSE)
    _qt_internal_genex_get_property_joined_with_macro(
        content
        "${target}"
        "_qt_cachegen_sanitized_resource_name"
        "QT_DECLARE_EXTERN_RESOURCE"
        ${with_ending_semicolon}
    )

    string(PREPEND content "\n")
    set(${out_var} "${content}" PARENT_SCOPE)
endfunction()

# Get code block that should reference the cachegen resource in the plugin constructor.
# Ensures the resource is not discarded when linking.
function(_qt_internal_qml_get_cachegen_resource_keep_reference target out_var)
    set(with_ending_semicolon FALSE)
    _qt_internal_genex_get_property_joined_with_macro(
        content
        "${target}"
        "_qt_cachegen_sanitized_resource_name"
        "QT_KEEP_RESOURCE"
        ${with_ending_semicolon}
    )

    string(PREPEND content "\n")
    set(${out_var} "${content}" PARENT_SCOPE)
endfunction()

# Get extern declaration for a regular resource.
function(_qt_internal_qml_get_resource_extern_declarations target out_var)
    set(with_ending_semicolon FALSE)
    _qt_internal_genex_get_property_joined_with_macro(
        content
        "${target}"
        "_qt_qml_module_sanitized_resource_names"
        "QT_DECLARE_EXTERN_RESOURCE"
        ${with_ending_semicolon}
    )

    string(PREPEND content "\n")
    set(${out_var} "${content}" PARENT_SCOPE)
endfunction()

# Get code block that should reference regular resources in the plugin constructor.
# Ensures the resources are not discarded when linking.
function(_qt_internal_qml_get_resource_keep_references target out_var)
    set(with_ending_semicolon FALSE)
    _qt_internal_genex_get_property_joined_with_macro(
        content
        "${target}"
        "_qt_qml_module_sanitized_resource_names"
        "QT_KEEP_RESOURCE"
        ${with_ending_semicolon}
    )

    string(PREPEND content "\n")
    set(${out_var} "${content}" PARENT_SCOPE)
endfunction()

# Get 3 different code blocks that should be inserted into various locations of the generated
# qml plugin cpp file. The code blocks ensure that if a target links to a plugin backed by a static
# library and initializes the plugin using Q_IMPORT_PLUGIN, none of the resources or type
# registration functions in the backing library are discarded by the linker.
function(_qt_internal_qml_get_symbols_to_keep
        target
        backing_target_type
        register_types_function_name
        out_var_intro
        out_var_intro_namespaced
        out_var_constructor)
    set(intro_content "")
    set(intro_namespaced_content "")

    # External symbol declarations.
    _qt_internal_qml_get_types_extern_declaration("${register_types_function_name}" output)
    string(APPEND intro_namespaced_content "${output}")

    if(backing_target_type STREQUAL "STATIC_LIBRARY")
        _qt_internal_qml_get_cachegen_extern_resource_declaration(${target} output)
        string(APPEND intro_content "${output}")

        _qt_internal_qml_get_resource_extern_declarations(${target} output)
        string(APPEND intro_content "${output}")
    endif()


    # Reference the external symbols in the plugin constructor to prevent the linker from
    # discarding the symbols.
    # The types symbol is an exported symbol, so we always reference it.
    set(constructor_content "")

    _qt_internal_qml_get_types_keep_reference("${register_types_function_name}" output)
    string(APPEND constructor_content "${output}")

    # Only reference the resources if the backing library is static.
    # If the backing library is shared, the symbols will not be found at link time because they
    # are not exported symbols.
    if(backing_target_type STREQUAL "STATIC_LIBRARY")
        _qt_internal_qml_get_cachegen_resource_keep_reference(${target} output)
        string(APPEND constructor_content "${output}")

        _qt_internal_qml_get_resource_keep_references(${target} output)
        string(APPEND constructor_content "${output}")
    endif()

    set(${out_var_intro} "${intro_content}" PARENT_SCOPE)
    set(${out_var_intro_namespaced} "${intro_namespaced_content}" PARENT_SCOPE)
    set(${out_var_constructor} "${constructor_content}" PARENT_SCOPE)
endfunction()

function(_qt_internal_set_qml_target_multi_config_output_directory target output_directory)
    # In multi-config builds we need to make sure that at least one configuration has the dynamic
    # plugin that is located next to qmldir file, otherwise QML engine won't be able to load the
    # plugin.
    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)
    if(is_multi_config)
        # We don't care about static plugins here since, they are linked at build time and
        # their location doesn't affect the runtime.
        get_target_property(target_type ${target} TYPE)
        if(target_type STREQUAL "SHARED_LIBRARY" OR target_type STREQUAL "MODULE_LIBRARY")
            if(NOT "${output_directory}")
                set(output_directory "${CMAKE_CURRENT_BINARY_DIR}")
            endif()

            list(GET CMAKE_CONFIGURATION_TYPES 0 default_config)
            string(JOIN "" output_directory_with_default_config
                "$<IF:$<CONFIG:${default_config}>,"
                    "${output_directory},"
                    "${output_directory}/$<CONFIG>"
                ">"
            )
            set_target_properties(${target} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${output_directory_with_default_config}"
                LIBRARY_OUTPUT_DIRECTORY "${output_directory_with_default_config}"
            )
        endif()
    endif()
endfunction()

function(qt6_add_qml_plugin target)
    set(args_option
        STATIC
        SHARED
        NO_GENERATE_PLUGIN_SOURCE
    )

    set(args_single
        OUTPUT_DIRECTORY
        URI
        BACKING_TARGET
        CLASS_NAME
        NAMESPACE
        # The following is only needed on Android, and even then, only if the
        # default conversion from the URI is not applicable. It is an internal
        # option, it may be removed.
        TARGET_PATH
    )

    set(args_multi "")

    cmake_parse_arguments(PARSE_ARGV 1 arg
       "${args_option}"
       "${args_single}"
       "${args_multi}"
    )

    if(arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unexpected arguments: ${arg_UNPARSED_ARGUMENTS}")
    endif()

    if(NOT arg_URI)
        if(NOT arg_BACKING_TARGET)
            message(FATAL_ERROR "No URI or BACKING_TARGET provided")
        endif()
        if(NOT TARGET ${arg_BACKING_TARGET})
            if(arg_BACKING_TARGET STREQUAL target)
                message(FATAL_ERROR
                    "Plugin ${target} is its own backing target, URI must be provided"
                )
            else()
                message(FATAL_ERROR
                    "No URI provided and unable to obtain it from the BACKING_TARGET "
                    "(${arg_BACKING_TARGET}) because no such target exists"
                )
            endif()
        endif()
        get_target_property(arg_URI ${arg_BACKING_TARGET} QT_QML_MODULE_URI)
        if(NOT arg_URI)
            message(FATAL_ERROR
                "No URI provided and the BACKING_TARGET (${arg_BACKING_TARGET}) "
                "does not have one set either"
            )
        endif()
    endif()

    _qt_internal_require_qml_uri_valid("${arg_URI}")

    # TODO: Probably should remove TARGET_PATH as a supported keyword now
    if(NOT arg_TARGET_PATH AND TARGET "${arg_BACKING_TARGET}")
        get_target_property(arg_TARGET_PATH ${arg_BACKING_TARGET} QT_QML_MODULE_TARGET_PATH)
    endif()
    if(NOT arg_TARGET_PATH)
        string(REPLACE "." "/" arg_TARGET_PATH "${arg_URI}")
    endif()

    _qt_internal_get_escaped_uri("${arg_URI}" escaped_uri)

    if(NOT arg_CLASS_NAME)
        if(NOT "${arg_BACKING_TARGET}" STREQUAL "")
            get_target_property(arg_CLASS_NAME ${arg_BACKING_TARGET} QT_QML_MODULE_CLASS_NAME)
        endif()
        if(NOT arg_CLASS_NAME)
            _qt_internal_compute_qml_plugin_class_name_from_uri("${arg_URI}" arg_CLASS_NAME)
        endif()
    endif()

    if(arg_BACKING_TARGET AND TARGET "${arg_BACKING_TARGET}")
        get_target_property(backing_type ${arg_BACKING_TARGET} TYPE)
    endif()

    if(TARGET ${target})
        # Plugin target already exists. Perform a few sanity checks, but we
        # otherwise trust that the target is appropriate for use as a plugin.
        get_target_property(target_type ${target} TYPE)
        if(target_type STREQUAL "EXECUTABLE")
            message(FATAL_ERROR "Plugins cannot be executables (target: ${target})")
        endif()
        foreach(arg IN ITEMS STATIC SHARED)
            if(arg_${arg})
                message(FATAL_ERROR
                    "Cannot specify ${arg} keyword, target ${target} already exists"
                )
            endif()
        endforeach()

        get_target_property(existing_class_name ${target} QT_PLUGIN_CLASS_NAME)
        if(existing_class_name)
            if(NOT existing_class_name STREQUAL arg_CLASS_NAME)
                message(FATAL_ERROR
                    "An existing plugin target was given, but it has a different class name "
                    "(${existing_class_name}) to that being used here (${arg_CLASS_NAME})"
                )
            endif()
        elseif(arg_CLASS_NAME)
            set_property(TARGET ${target} PROPERTY QT_PLUGIN_CLASS_NAME "${arg_CLASS_NAME}")
        else()
            message(FATAL_ERROR
                "An existing '${target}' plugin target was given, but it has no class name set "
                "and no new class name was provided."
            )
        endif()
    else()
        if(arg_STATIC AND arg_SHARED)
            message(FATAL_ERROR
                "Cannot specify both STATIC and SHARED for target ${target}"
            )
        endif()
        set(lib_type "")
        if(arg_STATIC)
            set(lib_type STATIC)
        elseif(arg_SHARED)
            set(lib_type SHARED)
        endif()

        if(TARGET "${arg_BACKING_TARGET}")
            # Ensure that the plugin type we create will be compatible with the
            # type of backing target we were given
            if(backing_type STREQUAL "STATIC_LIBRARY")
                if(lib_type STREQUAL "")
                    set(lib_type STATIC)
                elseif(lib_type STREQUAL "SHARED")
                    message(FATAL_ERROR
                        "Mixing a static backing library with a non-static plugin "
                        "is not supported"
                    )
                endif()
            elseif(backing_type STREQUAL "SHARED_LIBRARY")
                if(lib_type STREQUAL "")
                    set(lib_type SHARED)
                elseif(lib_type STREQUAL "STATIC")
                    message(FATAL_ERROR
                        "Mixing a non-static backing library with a static plugin "
                        "is not supported"
                    )
                endif()
            elseif(backing_type STREQUAL "EXECUTABLE")
                message(FATAL_ERROR
                    "A separate plugin should not be needed when the backing target "
                    "is an executable. Pre-create the plugin target before calling "
                    "this command if you really must have a separate plugin."
                )
            else()
                # Object libraries, utility/custom targets
                message(FATAL_ERROR "Unsupported backing target type: ${backing_type}")
            endif()
        endif()

        qt6_add_plugin(${target} ${lib_type}
            PLUGIN_TYPE qml_plugin
            CLASS_NAME ${arg_CLASS_NAME}
        )
    endif()

    get_target_property(install_rpath ${target} INSTALL_RPATH)
    # Ignore any CMAKE_INSTALL_RPATH and set a better default RPATH on platforms
    # that support it, if allowed. Projects will often set CMAKE_INSTALL_RPATH
    # for executables or backing libraries, but forget about plugins. Because
    # the path for QML plugins depends on their URI, it is unlikely that
    # CMAKE_INSTALL_RPATH would ever be intended for use with QML plugins.
    #
    # Avoid setting INSTALL_RPATH if it was set before. This is mostly
    # applicable for the Qml plugins built in Qt tree, that got INSTALL_RPATH
    # from the qt_internal_add_plugin function, but also can be the case for the
    # user Qml plugins created manually.
    if(NOT WIN32 AND NOT QT_NO_QML_PLUGIN_RPATH AND NOT install_rpath)
        # Construct a relative path from a default install location (assumed to
        # be qml/target-path) to ${CMAKE_INSTALL_LIBDIR}. This would be
        # applicable for Apple too (although unusual) if this is a bare install
         # (i.e. not part of an app bundle).
        string(REPLACE "/" ";" path "qml/${arg_TARGET_PATH}")
        list(LENGTH path path_count)
        string(REPEAT "../" ${path_count} rel_path)
        string(APPEND rel_path "${CMAKE_INSTALL_LIBDIR}")
        if(APPLE)
            set(install_rpath
                # If embedded in an app bundle, search in a bundle-local path
                # first. This path should always be the same for every app
                # bundle because plugin binaries should live in the PlugIns
                # directory, not a subdirectory of it or anywhere else.
                # Similarly, frameworks and bare shared libraries should always
                # be in the bundle's Frameworks directory.
                "@loader_path/../Frameworks"

                # This will be needed if the plugin is not installed as part of
                # an app bundle, such as when used by a command-line tool.
                "@loader_path/${rel_path}"
            )
        else()
            set(install_rpath "$ORIGIN/${rel_path}")
        endif()
        set_target_properties(${target} PROPERTIES INSTALL_RPATH "${install_rpath}")
    endif()

    get_target_property(moc_opts ${target} AUTOMOC_MOC_OPTIONS)
    set(already_set FALSE)
    if(moc_opts)
        foreach(opt IN LISTS moc_opts)
            if("${opt}" MATCHES "^-Muri=")
                set(already_set TRUE)
                break()
            endif()
        endforeach()
    endif()
    if(NOT already_set)
        # Insert the plugin's URI into its meta data to enable usage
        # of static plugins in QtDeclarative (like in mkspecs/features/qml_plugin.prf).
        set_property(TARGET ${target} APPEND PROPERTY
            AUTOMOC_MOC_OPTIONS "-Muri=${arg_URI}"
        )
    endif()

    # Work around QTBUG-115152.
    # Assign / duplicate the backing library's metatypes file to the qml plugin.
    # This ensures that the metatypes are passed as foreign types to qmltyperegistrar when
    # a consumer links against the plugin, but not the backing library.
    # This is needed because the plugin links PRIVATEly to the backing library and thus the
    # backing library's INTERFACE_SOURCES don't end up in the final consumer's SOURCES.
    # Arguably doing this is cleaner than changing the linkage to PUBLIC, because that will
    # propagate not only the INTERFACE_SOURCES, but also other link dependencies, which might
    # be unwanted.
    # In general this is a workaround due to CMake's limitations around support for propagating
    # custom properties across targets to a final consumer target.
    # https://gitlab.kitware.com/cmake/cmake/-/issues/20416
    if(arg_BACKING_TARGET
            AND TARGET "${arg_BACKING_TARGET}" AND NOT arg_BACKING_TARGET STREQUAL target)
        get_target_property(plugin_meta_types_file "${target}" INTERFACE_QT_META_TYPES_BUILD_FILE)
        get_target_property(
            backing_meta_types_file "${arg_BACKING_TARGET}" INTERFACE_QT_META_TYPES_BUILD_FILE)
        get_target_property(
            backing_meta_types_file_name
            "${arg_BACKING_TARGET}" INTERFACE_QT_META_TYPES_FILE_NAME)
        get_target_property(backing_has_qmltypes "${arg_BACKING_TARGET}" _qt_internal_has_qmltypes)
        if(backing_has_qmltypes AND NOT plugin_meta_types_file)
            _qt_internal_assign_build_metatypes_files_and_properties(
                "${target}"
                METATYPES_FILE_NAME "${backing_meta_types_file_name}"
                METATYPES_FILE_PATH "${backing_meta_types_file}"
            )
        endif()
    endif()

    if(arg_BACKING_TARGET)
        set(has_backing_target TRUE)

        set_target_properties(${target} PROPERTIES
            _qt_qml_module_backing_target "${arg_BACKING_TARGET}"
        )

        get_target_property(installed_backing_target
            "${arg_BACKING_TARGET}" QT_QML_MODULE_INSTALLED_BACKING_TARGET)

        if(installed_backing_target)
            set_target_properties(${target} PROPERTIES
                _qt_qml_module_installed_backing_target "${installed_backing_target}"
            )
        endif()

        get_target_property(installed_qmldir_path
            "${arg_BACKING_TARGET}" QT_QML_MODULE_INSTALLED_QMLDIR_PATH)

        if(installed_qmldir_path)
            set_target_properties(${target} PROPERTIES
                _qt_qml_module_installed_qmldir_path "${installed_qmldir_path}"
            )
        endif()

        if(target STREQUAL "${arg_BACKING_TARGET}")
            set(is_backing_and_plugin_same TRUE)
        else()
            set(is_backing_and_plugin_same FALSE)
        endif()
    else()
        set(has_backing_target FALSE)
        set(is_backing_and_plugin_same FALSE)
    endif()

    set_target_properties(${target} PROPERTIES
        _qt_qml_module_uri "${arg_URI}"
        _qt_qml_module_target_path "${arg_TARGET_PATH}"
        _qt_qml_module_version "${arg_VERSION}"
        _qt_qml_module_class_name "${arg_CLASS_NAME}"
        _qt_qml_module_is_plugin_target "TRUE"
        _qt_qml_module_plugin_has_backing_library "${has_backing_target}"
        _qt_qml_backing_library_and_plugin_are_same_target "${is_backing_and_plugin_same}"
    )

    _qt_internal_qml_export_common_qml_properties("${target}")
    set_property(TARGET ${target} APPEND PROPERTY
        EXPORT_PROPERTIES
            _qt_qml_module_is_plugin_target
            _qt_qml_module_backing_target
            _qt_qml_module_installed_backing_target
            _qt_qml_module_plugin_has_backing_library
    )

    if(ANDROID)
        _qt_internal_get_qml_plugin_output_name(plugin_output_name ${target}
            BACKING_TARGET "${arg_BACKING_TARGET}"
            TARGET_PATH "${arg_TARGET_PATH}"
            URI "${arg_URI}"
        )
        set_target_properties(${target}
            PROPERTIES
            LIBRARY_OUTPUT_NAME "${plugin_output_name}"
        )
        set_property(TARGET "${target}"
                     PROPERTY _qt_android_apply_arch_suffix_called_from_qt_impl TRUE)
        qt6_android_apply_arch_suffix(${target})
    endif()

    if(NOT arg_OUTPUT_DIRECTORY AND arg_BACKING_TARGET AND TARGET ${arg_BACKING_TARGET})
        get_target_property(arg_OUTPUT_DIRECTORY ${arg_BACKING_TARGET} QT_QML_MODULE_OUTPUT_DIRECTORY)
    endif()
    if(arg_OUTPUT_DIRECTORY)
        # Plugin target must be in the output directory. The backing target,
        # if it is different to the plugin target, can be anywhere.
        set_target_properties(${target} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${arg_OUTPUT_DIRECTORY}
            LIBRARY_OUTPUT_DIRECTORY ${arg_OUTPUT_DIRECTORY}
            ARCHIVE_OUTPUT_DIRECTORY ${arg_OUTPUT_DIRECTORY}
        )
    endif()

    _qt_internal_set_qml_target_multi_config_output_directory(${target} "${arg_OUTPUT_DIRECTORY}")

    if(NOT arg_NO_GENERATE_PLUGIN_SOURCE)
        set(generated_cpp_file_name_base "${target}_${arg_CLASS_NAME}")
        set(register_types_function_name "qml_register_types_${escaped_uri}")

        # These are all substituted in the template file used further below
        set(qt_qml_plugin_class_name "${arg_CLASS_NAME}")
        set(qt_qml_plugin_moc_include_name "${generated_cpp_file_name_base}.moc")
        set(qt_qml_plugin_intro "")
        set(qt_qml_plugin_outro "")
        set(qt_qml_plugin_constructor_content "")

        # Get the target that contains the resource names.
        set(target_with_prop "${target}")
        if(NOT arg_BACKING_TARGET STREQUAL "" AND TARGET "${arg_BACKING_TARGET}")
            set(target_with_prop "${arg_BACKING_TARGET}")
        endif()

        _qt_internal_qml_get_symbols_to_keep(
            "${target_with_prop}"
            "${backing_type}"
            "${register_types_function_name}"
            extra_intro_content
            extra_intro_namespaced_content
            extra_costructor_content
        )

        string(APPEND qt_qml_plugin_intro "${extra_intro_content}\n\n")

        if (arg_NAMESPACE)
            string(APPEND qt_qml_plugin_intro "namespace ${arg_NAMESPACE} {\n")
            string(APPEND qt_qml_plugin_outro "} // namespace ${arg_NAMESPACE}")
        endif()

        string(APPEND qt_qml_plugin_intro "${extra_intro_namespaced_content}")

        string(APPEND qt_qml_plugin_constructor_content "${extra_costructor_content}")

        # Configure file from the template.
        set(generated_cpp_file_in
            "${CMAKE_CURRENT_BINARY_DIR}/${generated_cpp_file_name_base}_in.cpp"
        )
        configure_file(
            "${__qt_qml_macros_module_base_dir}/Qt6QmlPluginTemplate.cpp.in"
            "${generated_cpp_file_in}"
            @ONLY
        )

        get_target_property(template_generated ${target} _qt_qml_plugin_template_generated)

        if(NOT template_generated)
            # Generate the file to allow adding generator expressions.
            set(generated_cpp_file
                "${CMAKE_CURRENT_BINARY_DIR}/${generated_cpp_file_name_base}.cpp"
            )
            file(GENERATE OUTPUT "${generated_cpp_file}"
                 INPUT "${generated_cpp_file_in}"
            )

            # We can't rely on policy CMP0118 since user project controls it
            set(scope_args)
            if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
                set(scope_args TARGET_DIRECTORY ${target})
            endif()
            _qt_internal_set_source_file_generated(
                SOURCES "${generated_cpp_file}"
                ${scope_args}
            )
            if(WIN32)
                set_source_files_properties("${generated_cpp_file}" ${scope_args}
                    PROPERTIES SKIP_UNITY_BUILD_INCLUSION TRUE
                )
            endif()

            # Because the add_qml_plugin function might be called multiple times on the same target,
            # Only generate and add the cpp file once.
            set_target_properties(${target} PROPERTIES _qt_qml_plugin_template_generated TRUE)
            target_sources(${target} PRIVATE "${generated_cpp_file}")
        endif()

        # The generated cpp file expects to include its moc-ed output file.
        set_target_properties(${target} PROPERTIES AUTOMOC TRUE)
    endif()

    target_link_libraries(${target} PRIVATE ${QT_CMAKE_EXPORT_NAMESPACE}::Qml)

    # Link plugin against its backing lib if it has one.
    if(NOT arg_BACKING_TARGET STREQUAL "" AND NOT arg_BACKING_TARGET STREQUAL target)
        target_link_libraries(${target} PRIVATE ${arg_BACKING_TARGET})
    endif()

    if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.19.0")
        # Defer the collection of plugin dependencies until after any extra target_link_libraries
        # calls that a user project might do.
        # We wrap the deferred call with EVAL CODE
        # so that ${target} is evaluated now rather than the end of the scope.
        cmake_language(EVAL CODE
            "cmake_language(DEFER CALL _qt_internal_add_static_qml_plugin_dependencies \"${target}\" \"${arg_BACKING_TARGET}\")"
        )
    else()
        # Can't defer, have to do it now.
        _qt_internal_add_static_qml_plugin_dependencies("${target}" "${arg_BACKING_TARGET}")
    endif()
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_add_qml_plugin)
        qt6_add_qml_plugin(${ARGV})
    endfunction()
endif()

function(_qt_internal_qml_export_common_qml_properties target)
    set_property(TARGET "${target}" APPEND PROPERTY
        EXPORT_PROPERTIES
            _qt_qml_module_uri
            _qt_qml_module_target_path
            _qt_qml_module_version
            _qt_qml_module_class_name
            _qt_qml_backing_library_and_plugin_are_same_target
            _qt_qml_module_installed_qmldir_path
    )
endfunction()

# Set up custom targets to copy qml files or resources of a target into its build directory.
# The custom targets run a cmake script that will go through each source file and copy it only if
# it doesn't exist or is modified.
# This is done with a single script per-target, rather than one command per file, to significantly
# decrease build time in a project when there are many files to copy, especially for the
# Xcode generator which is more susceptible to it.
#
# The new way of copying files can be opted out by setting the QT_COPY_QML_FILES_OLD_WAY variable
# to TRUE.
# TODO: Remove this opt out once we confirm there are no regressions.
#
# CUSTOM_TARGET_SUFFIX - suffix used in the name of the custom target created
# PROP_WITH_ENTRIES - the target property that contains the (src;dest) tuples of absolute
# paths
# PROP_WITH_SRCS - the target property that contains the list of all absolute source file paths, to
# be used to setup dependency information.
# FILE_TYPE - a label to show in the custom target COMMAND COMMENT field
function(_qt_internal_qml_copy_files_to_build_dir target)
    if(QT_COPY_QML_FILES_OLD_WAY)
        return()
    endif()

    set(opt_args "")
    set(single_args
        CUSTOM_TARGET_SUFFIX
        PROP_WITH_ENTRIES
        PROP_WITH_SRCS
        FILE_TYPE
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_CUSTOM_TARGET_SUFFIX)
        message(FATAL_ERROR "CUSTOM_TARGET_SUFFIX must be provided")
    endif()

    if(NOT arg_PROP_WITH_ENTRIES)
        message(FATAL_ERROR "PROP_WITH_ENTRIES must be provided")
    endif()

    if(NOT arg_PROP_WITH_SRCS)
        message(FATAL_ERROR "PROP_WITH_SRCS must be provided")
    endif()

    if(NOT arg_FILE_TYPE)
        message(FATAL_ERROR "FILE_TYPE must be provided")
    endif()

    set(setup_done_prop "_qt_qml_copy${arg_CUSTOM_TARGET_SUFFIX}_setup_done")
    get_target_property(copy_files_setup_done "${target}" "${setup_done_prop}")

    # Protect against multiple calls of qt6_add_qml_sources. It's enough to setup once,
    # because we use generator expressions to get all the files.
    if(copy_files_setup_done)
        message(FATAL_ERROR
            "_qt_internal_qml_copy_files_to_build_dir was called more than once for "
            "target ${target} CUSTOM_TARGET_SUFFIX ${arg_CUSTOM_TARGET_SUFFIX}."
        )
    endif()

    set_property(TARGET "${target}" PROPERTY "${setup_done_prop}" TRUE)

    get_target_property(target_source_dir "${target}" SOURCE_DIR)

    set(copy_files_script_path
        "${__qt_qml_macros_module_base_dir}/Qt6QmlCopyFiles.cmake")

    set(generated_copy_files_info_path
        "${CMAKE_CURRENT_BINARY_DIR}/.qt/${target}_${arg_CUSTOM_TARGET_SUFFIX}.cmake")
    set(generated_copy_files_info_path_timestamp
        "${CMAKE_CURRENT_BINARY_DIR}/.qt/${target}_${arg_CUSTOM_TARGET_SUFFIX}.txt")


    set(generated_copy_files_info
        "
set(target \"${target}\")
set(working_dir \"${target_source_dir}\")
set(src_and_dest_list
$<TARGET_PROPERTY:${target},${arg_PROP_WITH_ENTRIES}>
)
set(timestamp_file \"${generated_copy_files_info_path_timestamp}\")
"
    )

    file(GENERATE
        OUTPUT "${generated_copy_files_info_path}"
        CONTENT "${generated_copy_files_info}"
    )

    # In case there are no files, the dependencies should eval to an empty list.
    set(sources_genex "$<TARGET_PROPERTY:${target},${arg_PROP_WITH_SRCS}>")
    set(wrapped_sources_genex "$<$<BOOL:${sources_genex}>:${sources_genex}>")

    add_custom_command(OUTPUT "${generated_copy_files_info_path_timestamp}"
        COMMAND
            "${CMAKE_COMMAND}"
            "-DFILES_INFO_PATH=${generated_copy_files_info_path}"
            -P "${copy_files_script_path}"
        DEPENDS
            "${copy_files_script_path}"
            ${wrapped_sources_genex}
        VERBATIM
        COMMENT "Copying ${target} qml ${arg_FILE_TYPE} into build dir"
    )

    set(copy_files_target "${target}_copy_${arg_CUSTOM_TARGET_SUFFIX}")

    add_custom_target("${copy_files_target}"
        DEPENDS
            "${generated_copy_files_info_path_timestamp}"
    )

    # The ${target}_tooling target might not always exist, e.g. if qmlcachegen is disabled and thus
    # no files are added to generated_sources_other_scope, thus the tooling target creation is
    # skipped. Or when doing an in-source build of qtdeclarative, in which case the source and
    # build dir coincide, and no files will be copied.
    # We can't detect that no files will be copied at this point, because we rely on evaluating the
    # property with files during generation time, and files might be added in subsequent calls
    # of qt_target_qml_sources, after the first call to this function, and a second invocation
    # will just return, because copy_files_setup_done will be true.
    # In such cases, make this target a dependency of the main target directly.
    if(TARGET "${target}_tooling")
        set(dependent_target "${target}_tooling")
    else()
        set(dependent_target "${target}")
    endif()
    add_dependencies("${dependent_target}" "${copy_files_target}")

    _qt_internal_assign_to_internal_targets_folder("${copy_files_target}")
endfunction()

function(_qt_internal_ensure_tooling_target tooling_target)
    if(TARGET ${tooling_target})
        return()
    endif()

    set(no_value_options "")
    set(single_value_options DEPENDENT_TARGET)
    set(multi_value_options "")
    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )
    if(arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unexpected arguments: ${arg_UNPARSED_ARGUMENTS}")
    endif()

    add_library(${tooling_target} INTERFACE)
    if(DEFINED arg_DEPENDENT_TARGET)
        add_dependencies(${arg_DEPENDENT_TARGET} ${tooling_target})
    endif()
    set_target_properties(${tooling_target} PROPERTIES QT_EXCLUDE_FROM_TRANSLATION ON)
    _qt_internal_assign_to_internal_targets_folder(${tooling_target})
endfunction()

function(qt6_target_qml_sources target)

    get_target_property(uri        ${target} QT_QML_MODULE_URI)
    get_target_property(output_dir ${target} QT_QML_MODULE_OUTPUT_DIRECTORY)
    if(NOT uri OR NOT output_dir)
        message(FATAL_ERROR "Target ${target} is not a QML module")
    endif()

    set(args_option
        NO_LINT
        NO_CACHEGEN
        NO_QMLDIR_TYPES
        __QT_INTERNAL_FORCE_DEFER_QMLDIR  # Used only by qt6_add_qml_module()
    )

    set(args_single
        PREFIX
        OUTPUT_TARGETS
    )

    set(args_multi
        QML_FILES
        RESOURCES
    )

    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${args_option}" "${args_single}" "${args_multi}"
    )
    if(arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown/unexpected arguments: ${arg_UNPARSED_ARGUMENTS}")
    endif()

    # Filter out duplicates in QML_FILES and RESOURCES
    set(valid_qml_file_ext .qml .js .mjs)
    # TODO: maybe there is a better way to query for all languages enabled?
    set(other_source_file_ext
        .cpp .cxx .cc .c .c++
        .hpp .hxx .hh .h .h++
    )
    set(warning_message_file_prefix "    ")
    # Note: The order of the file_set is important
    #   - can add an additional warning check for qml files present in RESOURCES
    #   - non-valid QML_FILES should still be considered as RESOURCES
    #     (change postponed to keep backwards compatibility, using `skip_qml_processing` instead)
    set(skip_qml_processing "")
    foreach(file_set IN ITEMS QML_FILES RESOURCES)
        # For now, we have to keep the path in `actual_${file_set}` unnormalized for the qmldir
        # entry consistency
        set(actual_${file_set} "")
        set(actual_${file_set}_absolute "")
        set(duplicate_in_${file_set} "")
        set(duplicate_input_in_${file_set} "")
        get_target_property(current_${file_set} ${target} QT_QML_MODULE_${file_set})
        foreach(file_src IN LISTS arg_${file_set})
            # TODO: For now we check against absolute path to avoid surprises with other
            #       duplications
            get_filename_component(file_absolute ${file_src} ABSOLUTE)
            # Check for duplicates in the current list
            if(file_absolute IN_LIST actual_${file_set}_absolute)
                list(APPEND duplicate_input_in_${file_set})
                continue()
            endif()
            # Check for duplicates in the target property added so far
            if(current_${file_set} AND file_absolute IN_LIST current_${file_set})
                list(APPEND duplicate_in_${file_set} "${file_src}")
                continue()
            endif()
            # Further checks specific to QML_FILES
            if(file_set STREQUAL "QML_FILES")
                # Check if file has an appropriate qml file extension
                get_filename_component(file_ext "${file_src}" LAST_EXT)
                if(NOT file_ext IN_LIST valid_qml_file_ext)
                    if(file_ext IN_LIST other_source_file_ext)
                        list(APPEND sources_in_qml_files "${file_src}")
                    else()
                        list(APPEND resources_in_qml_files "${file_src}")
                    endif()
                    # TODO: It would be cleaner to move invalid QML_FILES to RESOURCES, but for
                    # now we add them to `skip_qml_processing` instead
                    list(APPEND skip_qml_processing "${file_src}")
                endif()
            endif()
            # TODO: Shouldn't there be an extra check that a qml file is in QML_FILES and not
            #   in RESOURCES?
            # If all is ok so far add them to the actual file set to process
            list(APPEND actual_${file_set} "${file_src}")
            list(APPEND actual_${file_set}_absolute "${file_absolute}")
        endforeach()

        # Display warnings for all invalid files
        # Warn about duplicated files in input
        if(NOT QT_QML_IGNORE_DUPLICATE_FILES AND duplicate_input_in_${file_set})
            # We trigger a warning here because it was added before with potentially different
            # parameters, properties
            set(warning_message
                "The following files are passed more than once ${file_set}:"
            )
            list(JOIN duplicate_input_in_${file_set}
                "\n${warning_message_file_prefix}"
                warning_file_set
            )
            string(APPEND warning_message
                "\n${warning_message_file_prefix}"
                "${warning_file_set}\n"
                "(this message can be suppressed by setting QT_QML_IGNORE_DUPLICATE_FILES=ON)"
            )
            message(WARNING "${warning_message}")
        endif()
        # Warn about duplicated files overall
        if(NOT QT_QML_IGNORE_DUPLICATE_FILES AND duplicate_in_${file_set})
            # We trigger a warning here because it was added before with potentially different
            # parameters, properties
            set(warning_message
                "The following files have already been included as a ${file_set} for ${target}:"
            )
            list(JOIN duplicate_in_${file_set}
                "\n${warning_message_file_prefix}"
                warning_file_set
            )
            string(APPEND warning_message
                "\n${warning_message_file_prefix}"
                "${warning_file_set}\n"
                "(this message can be suppressed by setting QT_QML_IGNORE_DUPLICATE_FILES=ON)"
            )
            message(WARNING "${warning_message}")
        endif()
        # Additional warnings specific to QML_FILES
        if(file_set STREQUAL "QML_FILES")
            # Warn about non-qml files added in QML_FILES
            if(sources_in_qml_files OR resources_in_qml_files)
                set(warning_message
                    "Only .qml, .js or .mjs files should be added with QML_FILES. "
                    "The following files are added as RESOURCES only, but should "
                    "instead be added:"
                )
                set(warning_file_set "")
                if(sources_in_qml_files)
                    list(JOIN sources_in_qml_files
                        "\n${warning_message_file_prefix}"
                        warning_file_set
                    )
                endif()
                string(APPEND warning_message
                    "\n- as source files using `target_sources()` or with SOURCES."
                    "\n  Possible candidates:"
                    "\n${warning_message_file_prefix}"
                    "${warning_file_set}"
                )
                set(warning_file_set "")
                if(resources_in_qml_files)
                    list(JOIN resources_in_qml_files
                        "\n${warning_message_file_prefix}"
                        warning_file_set
                    )
                endif()
                string(APPEND warning_message
                    "\n- as resources with RESOURCES:"
                    "\n  Possible candidates:"
                    "\n${warning_message_file_prefix}"
                    "${warning_file_set}"
                )
                message(WARNING "${warning_message}")
            endif()
        endif()
    endforeach()

    get_target_property(no_lint ${target} QT_QML_MODULE_NO_LINT)
    if(NOT actual_QML_FILES AND NOT actual_RESOURCES)
        if(NOT arg_NO_LINT AND NOT no_lint)
            _qt_internal_target_enable_qmllint(${target})
        endif()

        if(arg_OUTPUT_TARGETS)
            set(${arg_OUTPUT_TARGETS} "" PARENT_SCOPE)
        endif()
        return()
    endif()

    if(NOT arg___QT_INTERNAL_FORCE_DEFER_QMLDIR AND ${CMAKE_VERSION} VERSION_LESS "3.19.0")
        message(FATAL_ERROR
            "You are using CMake ${CMAKE_VERSION}, but CMake 3.19 or later "
            "is required to add qml files with this function. Either pass "
            "the qml files to qt6_add_qml_module() instead or update to "
            "CMake 3.19 or later."
        )
    endif()

    get_target_property(no_cachegen            ${target} QT_QML_MODULE_NO_CACHEGEN)
    get_target_property(no_qmldir              ${target} QT_QML_MODULE_NO_GENERATE_QMLDIR)
    get_target_property(no_extra_qmldirs       ${target} QT_QML_MODULE_NO_GENERATE_EXTRA_QMLDIRS)
    get_target_property(discard_qml_contents   ${target} QT_QML_MODULE_DISCARD_QML_CONTENTS)
    get_target_property(resource_prefix        ${target} QT_QML_MODULE_RESOURCE_PREFIX)
    get_target_property(qml_module_version     ${target} QT_QML_MODULE_VERSION)
    get_target_property(past_major_versions    ${target} QT_QML_MODULE_PAST_MAJOR_VERSIONS)

    if(NOT output_dir)
        # Probably not a qml module. We still want to support tooling for this
        # scenario, it's just that we will be relying solely on the implicit
        # imports to find things.
        set(output_dir ${CMAKE_CURRENT_BINARY_DIR})
        set(no_qmldir TRUE)
    endif()

    if(NOT arg_PREFIX)
        if(resource_prefix)
            set(arg_PREFIX ${resource_prefix})
        else()
            message(FATAL_ERROR
                "PREFIX option not given and target ${target} does not have a "
                "QT_QML_MODULE_RESOURCE_PREFIX property set."
            )
        endif()
    endif()
    _qt_internal_canonicalize_resource_path("${arg_PREFIX}" arg_PREFIX)
    if(arg_PREFIX STREQUAL resource_prefix)
        set(prefix_override "")
    else()
        set(prefix_override "${arg_PREFIX}")
    endif()
    if(NOT arg_PREFIX STREQUAL "/")
        string(APPEND arg_PREFIX "/")
    endif()

    if (qml_module_version MATCHES "^([0-9]+)\\.")
        set(qml_module_files_versions "${CMAKE_MATCH_1}.0")
    else()
        message(FATAL_ERROR
            "No major version found in '${qml_module_version}'."
        )
    endif()
    if (past_major_versions OR past_major_versions STREQUAL "0")
        foreach (past_major_version ${past_major_versions})
            list(APPEND qml_module_files_versions "${past_major_version}.0")
        endforeach()
    endif()

    # Linting and cachegen can still occur for a target that isn't a qml module,
    # but for such targets, there is no qmldir file to update.
    if(arg_NO_LINT)
        set(no_lint TRUE)
    endif()
    if(arg_NO_CACHEGEN OR QT_QML_NO_CACHEGEN)
        set(no_cachegen TRUE)
    endif()
    if(no_qmldir MATCHES "NOTFOUND" OR arg_NO_QMLDIR_TYPES)
        set(no_qmldir TRUE)
    endif()

    if(NOT no_cachegen AND actual_QML_FILES)

        # Even if we don't generate a qmldir file, it still should be here, manually written.
        # We can pass it unconditionally. If it's not there, qmlcachegen or qmlsc might warn,
        # but that's not fatal.
        set(qmldir_file ${output_dir}/qmldir)

        _qt_internal_genex_getproperty(qmltypes_file ${target} QT_QML_MODULE_PLUGIN_TYPES_FILE)
        _qt_internal_genex_getproperty(qmlcachegen   ${target} QT_QMLCACHEGEN_EXECUTABLE)
        _qt_internal_genex_getproperty(direct_calls  ${target} QT_QMLCACHEGEN_DIRECT_CALLS)
        _qt_internal_genex_getjoinedproperty(arguments ${target}
            QT_QMLCACHEGEN_ARGUMENTS "$<SEMICOLON>" "$<SEMICOLON>"
        )
        _qt_internal_genex_getjoinedproperty(import_paths ${target}
            QT_QML_IMPORT_PATH "-I$<SEMICOLON>" "$<SEMICOLON>"
        )
        _qt_internal_genex_getjoinedproperty(qrc_resource_args ${target}
            _qt_generated_qrc_files "--resource$<SEMICOLON>" "$<SEMICOLON>"
        )
        get_target_property(target_type ${target} TYPE)
        get_target_property(android_type ${target} _qt_android_target_type)
        if(target_type STREQUAL "EXECUTABLE" OR android_type STREQUAL "APPLICATION")
            # The application binary directory is part of the default import path.
            list(APPEND import_paths -I "$<TARGET_PROPERTY:${target},BINARY_DIR>")
        else()
            string(REPLACE "." "/" uri_path "${uri}")
            string(FIND "${output_dir}" "${uri_path}" position REVERSE)
            string(LENGTH "${output_dir}" output_dir_length)
            string(LENGTH "${uri_path}" uri_path_length)
            math(EXPR import_path_length "${output_dir_length} - ${uri_path_length}")
            if("${position}" EQUAL "${import_path_length}")
                string(SUBSTRING "${output_dir}" "0" "${import_path_length}" import_path)
                list(APPEND import_paths -I "${import_path}")
            endif()
        endif()
        _qt_internal_extend_qml_import_paths(import_paths)
        set(cachegen_args
            ${import_paths}
            -i "${qmldir_file}"
            "$<${have_direct_calls}:--direct-calls>"
            "$<${have_arguments}:${arguments}>"
            ${qrc_resource_args}
        )

        if(do_qml_aotstats)
            # The --only-bytecode argument is mutually exclusive with aotstats and can
            # be added after qt_add_qml_module. Conditionally add aotstats flags via genex.
            set(aotstats_args
                "$<$<NOT:$<IN_LIST:--only-bytecode,${arguments}>>:--dump-aot-stats>"
                "$<$<NOT:$<IN_LIST:--only-bytecode,${arguments}>>:--module-id=${uri}(${target})>"
            )
            list(APPEND cachegen_args ${aotstats_args})
        endif()

        # For direct evaluation in if() below
        get_target_property(cachegen_prop ${target} QT_QMLCACHEGEN_EXECUTABLE)
        if(cachegen_prop)
            if(cachegen_prop STREQUAL "qmlcachegen" OR cachegen_prop STREQUAL "qmlsc")
                # If it's qmlcachegen or qmlsc, don't go looking for other programs of that name
                set(qmlcachegen "$<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::${cachegen_prop}>")
            else()
                find_program(${target}_QMLCACHEGEN ${cachegen_prop})
                if(${target}_QMLCACHEGEN)
                    set(qmlcachegen "${${target}_QMLCACHEGEN}")
                else()
                    message(FATAL_ERROR "Invalid qmlcachegen binary ${cachegen_prop} for ${target}")
                endif()
            endif()
        else()
            set(have_qmlsc "$<TARGET_EXISTS:${QT_CMAKE_EXPORT_NAMESPACE}::qmlsc>")
            set(cachegen_name "$<IF:${have_qmlsc},qmlsc,qmlcachegen>")
            set(qmlcachegen "$<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::${cachegen_name}>")
        endif()
    endif()

    set(output_targets "")
    set(copied_files "")

    # We want to set source file properties in the target's own scope if we can.
    # That's the canonical place the properties will be read from.
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.18)
        set(scope_option TARGET_DIRECTORY ${target})
    else()
        set(scope_option "")
    endif()

    set(set_should_create_tooling_target FALSE)

    foreach(file_set IN ITEMS QML_FILES RESOURCES)
        foreach(file_src IN LISTS actual_${file_set})
            get_filename_component(file_absolute ${file_src} ABSOLUTE)

            # Store the original files so the project can query them later.
            set_property(TARGET ${target} APPEND PROPERTY
                QT_QML_MODULE_${file_set} ${file_absolute}
            )
            if(prefix_override)
                set_source_files_properties(${file_absolute} ${scope_option}
                    PROPERTIES
                        QT_QML_MODULE_PREFIX_OVERRIDE "${prefix_override}"
                )
            endif()

            # We need to copy the file to the build directory now so that when
            # qmlimportscanner is run in qt6_import_qml_plugins() as part of
            # target finalizers, the files will be there. We need to do this
            # in a way that CMake doesn't create a dependency on the source or it
            # will re-run CMake every time the file is modified. We also don't
            # want to update the file's timestamp if its contents won't change.
            # We still enforce the dependency on the source file by adding a
            # build-time rule. This avoids having to re-run CMake just to re-copy
            # the file.
            __qt_get_relative_resource_path_for_file(file_resource_path ${file_src})
            set(file_out ${output_dir}/${file_resource_path})

            # Don't generate or copy the file in an in-source build if the source
            # and destination paths are the same, it will cause a ninja dependency
            # cycle at build time.
            if(NOT file_out STREQUAL file_absolute)
                # Don't try to copy the file if it does not exist at configure time, it might only
                # be created at generation or build time.
                # The assumption is we copy the file at configure time initially, and then at build
                # time later, to make IDEs happy so they can see the files before an initial build.
                # TODO: Clarify if it is actually necessary to copy the files at configure time,
                # should be done as part of QTBUG-128323 .
                if(EXISTS "${file_absolute}")
                    _qt_internal_qml_copy_file("${file_absolute}" "${file_out}")
                else()
                    # If the file does not exist at configure time, make sure we at least
                    # create the destination path directory, so that the 'old way' build time copy
                    # does not error out due to missing destination directory. It's faster to do
                    # it at configure time to avoid extra process spawning.
                    get_filename_component(file_out_dir "${file_out}" DIRECTORY)
                    file(MAKE_DIRECTORY "${file_out_dir}")
                endif()

                # Currently we need to copy the qml and js files into the build dir because most
                # tooling depends on them being there: qmllint, qmlcachegen, qmlsc, qmltc, qmlls,
                # old Qt Creator code model, etc.
                # There don't seem to be any consumers of the image files in the build dir right
                # now, but a qml module is considered incomplete without them.
                # Someone might use them in a qmllint plugin or some IDE may want to use them for
                # some "preview" feature.
                # TODO: Investigate approach of not copying files at build time, but rather
                # creating an index file in the build dir that can map to the source dir, and
                # updating tooling to use the index file.
                # Should be done as part of QTBUG-128323 .
                if(QT_COPY_QML_FILES_OLD_WAY)
                    # Old style creation of one command per file copy.
                    add_custom_command(OUTPUT ${file_out}
                        COMMAND ${CMAKE_COMMAND} -E copy ${file_absolute} ${file_out}
                        DEPENDS ${file_absolute}
                        WORKING_DIRECTORY $<TARGET_PROPERTY:${target},SOURCE_DIR>
                        VERBATIM
                        COMMENT "Copying ${file_src} to ${file_out}"
                    )
                    list(APPEND copied_files ${file_out})
                else()
                    # New style where we collect all files of a particular type, and then create
                    # a single custom command to copy them all. This way is faster when there are
                    # many files.
                    set(set_should_create_tooling_target TRUE)
                    if(file_set STREQUAL "QML_FILES")
                        set(copy_prop "files")
                    elseif(file_set STREQUAL "RESOURCES")
                        set(copy_prop "resources")
                    else()
                        message(FATAL_ERROR "Unsupported file_set")
                    endif()

                    set_property(TARGET "${target}"
                        APPEND PROPERTY _qt_qml_${copy_prop}_absolute_src_paths
                        "${file_absolute}")

                    # Manually formatted string, so each path is on a separate line in the
                    # generated file.
                    set(copy_entry "    \"${file_absolute}\"\n    \"${file_out}\"\n")
                    set_property(TARGET "${target}"
                        APPEND_STRING PROPERTY _qt_qml_${copy_prop}_to_copy
                        "${copy_entry}")
                endif()
            endif()
        endforeach()
    endforeach()

    set(generated_sources_other_scope)
    set(extra_qmldirs)
    foreach(qml_file_src IN LISTS actual_QML_FILES)
        # Skip invalid qml files
        if(qml_file_src IN_LIST skip_qml_processing)
            continue()
        endif()
        # Mark QML files as source files, so that they do not appear in <Other Locations> in Creator
        # or other IDEs
        set_source_files_properties(${qml_file_src} HEADER_FILE_ONLY ON)
        target_sources(${target} PRIVATE ${qml_file_src})

        get_filename_component(file_absolute ${qml_file_src} ABSOLUTE)
        __qt_get_relative_resource_path_for_file(file_resource_path ${qml_file_src})

        get_filename_component(qml_file_resource_dir ${file_resource_path} DIRECTORY)
        if(qml_file_resource_dir)
            list(APPEND extra_qmldirs "${output_dir}/${qml_file_resource_dir}/qmldir")
        endif()

        # For the tooling steps below, run the tools on the copied qml file in
        # the build directory, not the source directory. This is required
        # because the tools may need to reference imported modules from
        # subdirectories, which would require those subdirectories to have
        # their generated qmldir files present. They also need to use the right
        # resource paths and the source locations might be structured quite
        # differently.

        # Add file to those processed by qmllint
        get_source_file_property(skip_qmllint ${qml_file_src} QT_QML_SKIP_QMLLINT)
        if(NOT no_lint AND NOT skip_qmllint)
            # The set of qml files to run qmllint on may be a subset of the
            # full set of files, so record these in a separate property.
            _qt_internal_target_enable_qmllint(${target})
            set_property(TARGET ${target} APPEND PROPERTY QT_QML_LINT_FILES ${file_absolute})
        endif()

        # Add qml file's type to qmldir
        get_source_file_property(skip_qmldir ${qml_file_src} QT_QML_SKIP_QMLDIR_ENTRY)
        if(NOT no_qmldir AND NOT skip_qmldir)
            get_source_file_property(qml_file_typename ${qml_file_src} QT_QML_SOURCE_TYPENAME)
            if (NOT qml_file_typename)
                get_filename_component(qml_file_ext ${qml_file_src} EXT)
                get_filename_component(qml_file_typename ${qml_file_src} NAME_WE)
            endif()

            # Do not add qmldir entries for lowercase names. Those are not components.
            if (qml_file_typename AND qml_file_typename MATCHES "^[A-Z]")
                if (qml_file_ext AND NOT qml_file_ext STREQUAL ".qml" AND NOT qml_file_ext STREQUAL ".ui.qml"
                        AND NOT qml_file_ext STREQUAL ".js" AND NOT qml_file_ext STREQUAL ".mjs")
                    message(AUTHOR_WARNING
                        "${qml_file_src} has a file extension different from .qml, .ui.qml, .js, "
                        "and .mjs. This leads to unexpected component names."
                    )
                endif()
                if(qml_file_ext AND qml_file_ext STREQUAL ".js" AND skip_qmldir STREQUAL "NOTFOUND")
                    file(
                        STRINGS ${qml_file_src} pragma_library
                        REGEX "^\\.pragma library$"
                        LIMIT_COUNT 1
                        LIMIT_INPUT 128
                    )
                    if(NOT pragma_library)
                        message(AUTHOR_WARNING
                            "${qml_file_src} is not an ECMAScript module and also doesn't contain "
                            "'.pragma library'. It will be re-evaluated in the context of every "
                            "QML document that explicitly or implicitly imports ${uri}. Set its "
                            "QT_QML_SKIP_QMLDIR_ENTRY source file property to FALSE if you really "
                            "want this to happen. Set it to TRUE to prevent it."
                        )
                    endif()
                endif()

                # We previously accepted the singular form of this property name
                # during tech preview. Issue a warning for that, but still
                # honor it. The plural form will override it if both are set.
                get_property(have_singular_property SOURCE ${qml_file_src}
                    PROPERTY QT_QML_SOURCE_VERSION SET
                )
                if(have_singular_property)
                    message(AUTHOR_WARNING
                        "The QT_QML_SOURCE_VERSION source file property has been replaced "
                        "by QT_QML_SOURCE_VERSIONS (i.e. plural rather than singular). "
                        "The singular form will eventually be removed, please update "
                        "the project to use the plural form instead for the file at:\n"
                        "  ${qml_file_src}"
                    )
                endif()
                get_source_file_property(qml_file_versions ${qml_file_src} QT_QML_SOURCE_VERSIONS)
                if(NOT qml_file_versions AND have_singular_property)
                    get_source_file_property(qml_file_versions ${qml_file_src} QT_QML_SOURCE_VERSION)
                endif()

                get_source_file_property(qml_file_singleton ${qml_file_src} QT_QML_SINGLETON_TYPE)
                get_source_file_property(qml_file_internal  ${qml_file_src} QT_QML_INTERNAL_TYPE)

                if (qml_file_singleton AND qml_file_internal)
                   message(FATAL_ERROR
                       "${qml_file_src} is marked as both internal and as a "
                       "singleton, but singletons cannot be internal!")
                endif()

                if (NOT qml_file_versions)
                    set(qml_file_versions ${qml_module_files_versions})
                endif()

                set(qmldir_file_contents "")
                foreach(qml_file_version IN LISTS qml_file_versions)
                    if (qml_file_singleton)
                        string(APPEND qmldir_file_contents "singleton ")
                    elseif (qml_file_internal)
                        continue()
                    endif()
                    string(APPEND qmldir_file_contents "${qml_file_typename} ${qml_file_version} ${file_resource_path}\n")
                endforeach()

                if (qml_file_internal)
                    # TODO: Remove when all qmldir parsers can parse internal types with versions.
                    #       Instead handle internal types like singletons above.
                    #       See QTCREATORBUG-28755
                    string(APPEND qmldir_file_contents "internal ${qml_file_typename} ${file_resource_path}\n")
                endif()

                set_property(TARGET ${target} APPEND_STRING PROPERTY
                    _qt_internal_qmldir_content "${qmldir_file_contents}"
                )

                if(ANDROID AND QT_ANDROID_GENERATE_JAVA_QML_COMPONENTS)
                    get_source_file_property(qml_file_generate_java_classes ${qml_file_src}
                        QT_QML_GENERATE_ANDROID_JAVA_CLASS
                    )
                    if(qml_file_generate_java_classes)
                        get_target_property(qml_module_uri ${target} QT_QML_MODULE_URI)
                        set_property(TARGET ${target} APPEND PROPERTY
                            _qt_qml_files_for_java_generator "${qml_module_uri}.${qml_file_typename}")
                    endif()
                endif()
            endif()
        endif()

        # Run cachegen on the qml file, or if disabled, store the raw qml file in the resources
        get_source_file_property(skip_cachegen ${qml_file_src} QT_QML_SKIP_CACHEGEN)
        if(NOT no_cachegen AND NOT skip_cachegen)
            # We delay this to here to ensure that we only ever enable cachegen
            # after we know there will be at least one file to compile.
            get_target_property(is_cachegen_set_up ${target} _qt_cachegen_set_up)
            if(NOT is_cachegen_set_up)
                _qt_internal_target_enable_qmlcachegen(${target} ${qmlcachegen})
            endif()

            # We ensured earlier that arg_PREFIX always ends with "/"
            file(TO_CMAKE_PATH "${arg_PREFIX}${file_resource_path}" file_resource_path)

            set_property(TARGET ${target} APPEND PROPERTY
                QT_QML_MODULE_RESOURCE_PATHS ${file_resource_path}
            )

            file(RELATIVE_PATH file_relative ${CMAKE_CURRENT_SOURCE_DIR} ${file_absolute})
            string(REGEX REPLACE "\\.(js|mjs|qml)$" "_\\1" compiled_file ${file_relative})
            string(REGEX REPLACE "[$#?]+" "_" compiled_file ${compiled_file})

            # The file name needs to be unique to work around an Integrity compiler issue.
            # Search for INTEGRITY_SYMBOL_UNIQUENESS in this file for details.
            set(compiled_file
                "${CMAKE_CURRENT_BINARY_DIR}/.rcc/qmlcache/${target}_${compiled_file}.cpp")
            get_filename_component(out_dir ${compiled_file} DIRECTORY)

            if(CMAKE_GENERATOR STREQUAL "Ninja Multi-Config" AND CMAKE_VERSION VERSION_GREATER_EQUAL "3.20")
                set(qmlcachegen_cmd "$<COMMAND_CONFIG:${qmlcachegen}>")
            else()
                set(qmlcachegen_cmd "${qmlcachegen}")
            endif()

            set(aotstats_file "")
            if(do_qml_aotstats AND "${qml_file_src}" MATCHES ".+\\.qml")
                set(aotstats_file "${compiled_file}.aotstats")
                list(APPEND aotstats_files ${aotstats_file})
            endif()

            _qt_internal_get_tool_wrapper_script_path(tool_wrapper)
            add_custom_command(
                OUTPUT
                    ${compiled_file}
                    ${aotstats_file}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${out_dir}
                COMMAND
                    ${tool_wrapper}
                    ${qmlcachegen_cmd}
                    --bare
                    --resource-path "${file_resource_path}"
                    ${cachegen_args}
                    -o "${compiled_file}"
                    "${file_absolute}"
                COMMAND_EXPAND_LISTS
                DEPENDS
                    ${qmlcachegen_cmd}
                    "${file_absolute}"
                    $<TARGET_PROPERTY:${target},_qt_generated_qrc_files>
                    "$<$<BOOL:${qmltypes_file}>:${qmltypes_file}>"
                    "${qmldir_file}"
                VERBATIM
            )

            target_sources(${target} PRIVATE ${compiled_file})
            # We can't rely on policy CMP0118 since user project controls it
            set(scope_args)
            if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
                set(scope_args TARGET_DIRECTORY ${target})
            endif()
            _qt_internal_set_source_file_generated(
                SOURCES ${compiled_file}
                ${scope_args}
                SKIP_AUTOGEN
            )
            get_target_property(target_source_dir ${target} SOURCE_DIR)
            if(NOT target_source_dir STREQUAL CMAKE_CURRENT_SOURCE_DIR)
                list(APPEND generated_sources_other_scope ${compiled_file})
            endif()
        endif()
    endforeach()

    if(do_qml_aotstats)
        set_property(TARGET ${target} APPEND PROPERTY
            QT_QML_MODULE_AOTSTATS_FILES ${aotstats_files})
    endif()

    if(ANDROID)
        _qt_internal_collect_qml_root_paths("${target}" ${actual_QML_FILES})
    endif()

    if(set_should_create_tooling_target OR copied_files OR generated_sources_other_scope)
        if(CMAKE_VERSION VERSION_LESS 3.19)
            # Called from qt6_add_qml_module() and we know there can only be
            # this one call. With those constraints, we can use a custom target
            # to implement the necessary dependencies to get files copied to the
            # build directory when their source files change.
            add_custom_target(${target}_tooling ALL
                DEPENDS
                    ${copied_files}
                    ${generated_sources_other_scope}
            )
            add_dependencies(${target} ${target}_tooling)
            _qt_internal_assign_to_internal_targets_folder(${target}_tooling)
        else()
            # We could be called multiple times and a custom target can only
            # have file-level dependencies added at the time the target is
            # created. Use an interface library instead, since we can add
            # private sources to those and have the library act as a build
            # system target from CMake 3.19 onward, and we can add the sources
            # progressively over multiple calls.
            set(tooling_target "${target}_tooling")

            # If the tooling target was created in another directory scope, we must create another
            # interface library in this directory scope to drive the custom target. A dependency
            # from ${target} to this new interface library doesn't seem necessary.
            get_target_property(tooling_target_source_dir ${tooling_target} SOURCE_DIR)
            if(NOT tooling_target_source_dir STREQUAL CMAKE_CURRENT_SOURCE_DIR)
                # Unfortunately, this doesn't work for Xcode nor Unix Makefiles.
                if(NOT CMAKE_GENERATOR MATCHES "^(Ninja|Visual Studio)")
                    message(FATAL_ERROR
                        "When using the '${CMAKE_GENERATOR}' generator, this function "
                        "must be called in the directory scope where '${target}' is defined."
                    )
                endif()

                get_target_property(counter ${target} QT_QML_MODULE_RAW_QML_SETS)
                if(NOT counter)
                    set(counter 0)
                endif()
                set(tooling_target "${target}_tooling_${counter}")
                _qt_internal_ensure_tooling_target("${tooling_target}")
            endif()

            target_sources(${tooling_target} PRIVATE
                ${copied_files}
                ${generated_sources_other_scope}
            )
        endif()
    endif()

    # Batch all the non-compiled qml sources into a single resource for this
    # call. Subsequent calls for the same target will be in their own separate
    # resource file.
    get_target_property(counter ${target} QT_QML_MODULE_RAW_QML_SETS)
    if(NOT counter)
        set(counter 0)
    endif()

    set(qml_resource_name ${target}_raw_qml_${counter})
    if(actual_QML_FILES)
        set(qml_resource_targets "")
        set(qml_resource_options "")
        if(discard_qml_contents)
            list(APPEND qml_resource_options DISCARD_FILE_CONTENTS)
        endif()
        qt6_add_resources(${target} ${qml_resource_name}
            PREFIX ${arg_PREFIX}
            FILES ${actual_QML_FILES}
            OUTPUT_TARGETS qml_resource_targets
            ${qml_resource_options}
        )
        list(APPEND output_targets ${qml_resource_targets})
        # Save the resource name in a property so we can reference it later in a qml plugin
        # constructor, to avoid discarding the resource if it's in a static library.
        __qt_internal_sanitize_resource_name(
            sanitized_qml_resource_name "${qml_resource_name}")
        set_property(TARGET ${target} APPEND PROPERTY
            _qt_qml_module_sanitized_resource_names "${sanitized_qml_resource_name}")
    endif()

    if(actual_RESOURCES)
        set(resources_resource_name ${target}_raw_res_${counter})
        set(resources_resource_targets)
        qt6_add_resources(${target} ${resources_resource_name}
            PREFIX ${arg_PREFIX}
            FILES ${actual_RESOURCES}
            OUTPUT_TARGETS resources_resource_targets
        )
        list(APPEND output_targets ${resources_resource_targets})
        # Save the resource name in a property so we can reference it later in a qml plugin
        # constructor, to avoid discarding the resource if it's in a static library.
        __qt_internal_sanitize_resource_name(
            sanitized_resources_resource_name "${resources_resource_name}")
        set_property(TARGET ${target} APPEND PROPERTY
            _qt_qml_module_sanitized_resource_names "${sanitized_resources_resource_name}")
    endif()

    if(extra_qmldirs AND NOT no_extra_qmldirs)
        list(REMOVE_DUPLICATES extra_qmldirs)
        __qt_internal_setup_policy(QTP0004 "6.8.0"
"You need qmldir files for each extra directory that contains .qml files for your module. \
Check https://doc.qt.io/qt-6/qt-cmake-policy-qtp0004.html for policy details."
        )
        qt6_policy(GET QTP0004 generate_extra_qmldirs_policy)
        if ("${generate_extra_qmldirs_policy}" STREQUAL "NEW" AND NOT no_qmldir)
            foreach(extra_qmldir IN LISTS extra_qmldirs)
                set(__qt_qmldir_content "prefer :${arg_PREFIX}")
                configure_file(
                    ${__qt_qml_macros_module_base_dir}/Qt6qmldirTemplate.cmake.in
                    "${extra_qmldir}"
                    @ONLY
                )

                _qt_internal_set_source_file_generated(
                    SOURCES "${extra_qmldir}"
                )
            endforeach()

            set(extra_qmldirs_targets)
            qt6_add_resources(${target} "${qml_resource_name}_extra_qmldirs"
                PREFIX ${arg_PREFIX}
                FILES ${extra_qmldirs}
                BASE ${output_dir}
                OUTPUT_TARGETS extra_qmldirs_targets
            )
            list(APPEND output_targets ${extra_qmldirs_targets})

            set_property(TARGET ${target} PROPERTY _qt_internal_extra_qmldirs ${extra_qmldirs})

            # Save the resource name in a property so we can reference it later in a qml plugin
            # constructor, to avoid discarding the resource if it's in a static library.
            __qt_internal_sanitize_resource_name(
                sanitized_extra_qmldirs_resource_name "${qml_resource_name}_extra_qmldirs")
            set_property(TARGET ${target} APPEND PROPERTY
                _qt_qml_module_sanitized_resource_names "${sanitized_extra_qmldirs_resource_name}")
        endif()
    endif()

    math(EXPR counter "${counter} + 1")
    set_target_properties(${target} PROPERTIES QT_QML_MODULE_RAW_QML_SETS ${counter})

    if(arg_OUTPUT_TARGETS)
        set(${arg_OUTPUT_TARGETS} ${output_targets} PARENT_SCOPE)
    endif()

endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_target_qml_sources)
        qt6_target_qml_sources(${ARGV})
        cmake_parse_arguments(PARSE_ARGV 1 arg  "" "OUTPUT_TARGETS" "")
        if(arg_OUTPUT_TARGETS)
            set(${arg_OUTPUT_TARGETS} ${${arg_OUTPUT_TARGETS}} PARENT_SCOPE)
        endif()
    endfunction()
endif()

function(qt6_generate_foreign_qml_types source_target destination_qml_target)
    qt6_extract_metatypes(${source_target})
    get_target_property(target_metatypes_json_file ${source_target}
                        INTERFACE_QT_META_TYPES_BUILD_FILE)
    if (NOT target_metatypes_json_file)
        message(FATAL_ERROR "Need target metatypes.json file")
    endif()

    get_target_property(automoc_enabled ${destination_qml_target} AUTOMOC)
    if(NOT automoc_enabled)
        message(FATAL_ERROR "qt6_generate_foreign_qml_types requires AUTOMOC to be enabled "
            "on target '${destination_qml_target}'.")
    endif()

    set(registration_files_base ${source_target}_${destination_qml_target})
    set(additional_sources
        "${CMAKE_CURRENT_BINARY_DIR}/${registration_files_base}.cpp"
        "${CMAKE_CURRENT_BINARY_DIR}/${registration_files_base}.h"
    )

    set(namespace_args "")
    get_target_property(namespace ${destination_qml_target} QT_QML_MODULE_NAMESPACE)
    if (namespace)
        list(APPEND namespace_args "--namespace" "${namespace}")
    endif()

    _qt_internal_get_tool_wrapper_script_path(tool_wrapper)
    add_custom_command(
        OUTPUT
            ${additional_sources}
        DEPENDS
            ${source_target}
            ${target_metatypes_json_file}
            ${QT_CMAKE_EXPORT_NAMESPACE}::qmltyperegistrar
        COMMAND
            ${tool_wrapper}
            $<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::qmltyperegistrar>
            "--extract"
            ${namespace_args}
            -o ${registration_files_base}
            ${target_metatypes_json_file}
        COMMENT "Generate QML registration code for target ${source_target}"
        VERBATIM
    )

    _qt_internal_set_source_file_generated(
        SOURCES ${additional_sources}
    )
    target_sources(${destination_qml_target} PRIVATE ${additional_sources})
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    if(QT_DEFAULT_MAJOR_VERSION EQUAL 6)
        function(qt_generate_foreign_qml_types)
            qt6_generate_foreign_qml_types(${ARGV})
        endfunction()
    else()
        message(FATAL_ERROR "qt_generate_foreign_qml_types() is only available in Qt 6.")
    endif()
endif()

# target: Expected to be the backing target for a qml module. Certain target
#   properties normally set by qt6_add_qml_module() will be retrieved from this
#   target. (REQUIRED)
#
# MANUAL_MOC_JSON_FILES: Specifies a list of json files, generated by a manual
#   moc call, to extract metatypes. (OPTIONAL)
#
# NAMESPACE: Specifies a namespace the type registration function shall be
#   generated into. (OPTIONAL)
#
# REGISTRATIONS_TARGET: Specifies a separate target the generated .cpp file
#   shall be added to. If not given, the main backing target is used. (OPTIONAL)
#
function(_qt_internal_qml_type_registration target)
    set(args_option)
    set(args_single NAMESPACE REGISTRATIONS_TARGET)
    set(args_multi  MANUAL_MOC_JSON_FILES)

    get_target_property(skipped ${target} _qt_is_skipped_test)
    if(skipped)
        return()
    endif()

    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${args_option}" "${args_single}" "${args_multi}"
    )
    if(arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown/unexpected arguments: ${arg_UNPARSED_ARGUMENTS}")
    endif()

    get_target_property(import_name ${target} QT_QML_MODULE_URI)
    if (NOT import_name)
        message(FATAL_ERROR "Target ${target} is not a QML module")
    endif()
    get_target_property(qmltypes_output_name ${target} QT_QML_MODULE_TYPEINFO)
    if (NOT qmltypes_output_name)
        get_target_property(compile_definitions_list ${target} COMPILE_DEFINITIONS)
        list(FIND compile_definitions_list QT_PLUGIN is_a_plugin)
        if (is_a_plugin GREATER_EQUAL 0)
            set(qmltypes_output_name "plugins.qmltypes")
        else()
            set(qmltypes_output_name ${target}.qmltypes)
        endif()
    endif()

    set(meta_types_json_args "")
    if(arg_MANUAL_MOC_JSON_FILES)
        list(APPEND meta_types_json_args "MANUAL_MOC_JSON_FILES" ${arg_MANUAL_MOC_JSON_FILES})
    endif()
    qt6_extract_metatypes(${target} ${meta_types_json_args})

    get_target_property(import_version ${target} QT_QML_MODULE_VERSION)
    get_target_property(output_dir ${target} QT_QML_MODULE_OUTPUT_DIRECTORY)
    get_target_property(target_source_dir ${target} SOURCE_DIR)
    get_target_property(target_binary_dir ${target} BINARY_DIR)
    get_target_property(target_metatypes_file ${target} INTERFACE_QT_META_TYPES_BUILD_FILE)
    if (NOT target_metatypes_file)
        message(FATAL_ERROR "Target ${target} does not have a meta types file")
    endif()

    # Extract major and minor version (could also have patch part, but we don't
    # need that here)
    if (import_version MATCHES "^([0-9]+)\\.([0-9]+)")
        set(major_version ${CMAKE_MATCH_1})
        set(minor_version ${CMAKE_MATCH_2})
    else()
        message(FATAL_ERROR
            "Invalid module version number '${import_version}'. "
            "Expected VersionMajor.VersionMinor."
        )
    endif()

    # check if plugins.qmltypes is already defined
    get_target_property(target_plugin_qmltypes ${target} QT_QML_MODULE_PLUGIN_TYPES_FILE)
    if (target_plugin_qmltypes)
        message(FATAL_ERROR "Target ${target} already has a qmltypes file set.")
    endif()

    set(cmd_args)
    set(plugin_types_file "${output_dir}/${qmltypes_output_name}")
    set(generated_marker_file "${target_binary_dir}/.qt/qmltypes/${qmltypes_output_name}")
    get_filename_component(generated_marker_dir "${generated_marker_file}" DIRECTORY)
    set_target_properties(${target} PROPERTIES
        QT_QML_MODULE_PLUGIN_TYPES_FILE ${plugin_types_file}
    )

    if (arg_NAMESPACE)
        list(APPEND cmd_args
            --namespace=${arg_NAMESPACE}
        )
    endif()

    list(APPEND cmd_args
        --generate-qmltypes=${plugin_types_file}
        --import-name=${import_name}
        --major-version=${major_version}
        --minor-version=${minor_version}
    )


    # Add --follow-foreign-versioning if requested
    get_target_property(follow_foreign_versioning ${target}
                        _qt_qml_module_follow_foreign_versioning)

    if (follow_foreign_versioning)
        list(APPEND cmd_args
            --follow-foreign-versioning
        )
    endif()

    # Add past minor versions
    get_target_property(past_major_versions ${target} QT_QML_MODULE_PAST_MAJOR_VERSIONS)

    if (past_major_versions OR past_major_versions STREQUAL "0")
        foreach (past_major_version ${past_major_versions})
            list(APPEND cmd_args
                --past-major-version ${past_major_version}
            )
        endforeach()
    endif()

    # Run a script to recursively evaluate all the metatypes.json files in order
    # to collect all foreign types.
    string(TOLOWER "${target}_qmltyperegistrations.cpp" type_registration_cpp_file_name)
    set(foreign_types_file "${target_binary_dir}/qmltypes/${target}_foreign_types.txt")
    set(type_registration_cpp_file "${target_binary_dir}/${type_registration_cpp_file_name}")

    # Enable evaluation of metatypes.json source interfaces
    set_target_properties(${target} PROPERTIES QT_CONSUMES_METATYPES TRUE)
    set(genex_list "$<REMOVE_DUPLICATES:$<FILTER:$<TARGET_PROPERTY:${target},SOURCES>,INCLUDE,metatypes.json$>>")
    set(genex_main "$<JOIN:${genex_list},$<COMMA>>")
    file(GENERATE OUTPUT "${foreign_types_file}"
        CONTENT "$<IF:$<BOOL:${genex_list}>,--foreign-types=${genex_main},\n>"
    )

    list(APPEND cmd_args
        "@${foreign_types_file}"
    )

    if (TARGET ${target}Private)
        list(APPEND cmd_args --private-includes)
    else()
        string(REGEX MATCH "Private$" privateSuffix ${target})
        if (privateSuffix)
            list(APPEND cmd_args --private-includes)
        endif()
    endif()

    get_target_property(target_metatypes_json_file ${target} INTERFACE_QT_META_TYPES_BUILD_FILE)
    if (NOT target_metatypes_json_file)
        message(FATAL_ERROR "Need target metatypes.json file")
    endif()

    _qt_internal_get_tool_wrapper_script_path(tool_wrapper)
    add_custom_command(
        OUTPUT
            ${type_registration_cpp_file}
            ${plugin_types_file}
        DEPENDS
            ${foreign_types_file}
            ${target_metatypes_json_file}
            ${QT_CMAKE_EXPORT_NAMESPACE}::qmltyperegistrar
            "$<$<BOOL:${genex_list}>:${genex_list}>"
        COMMAND
            ${tool_wrapper}
            $<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::qmltyperegistrar>
            ${cmd_args}
            -o ${type_registration_cpp_file}
            ${target_metatypes_json_file}
        COMMAND
            ${CMAKE_COMMAND} -E make_directory "${generated_marker_dir}"
        COMMAND
            ${CMAKE_COMMAND} -E touch "${generated_marker_file}"
        COMMENT "Automatic QML type registration for target ${target}"
        VERBATIM
    )

    # The ${target}_qmllint targets need to depend on the generation of all
    # *.qmltypes files in the build. We have no way of reliably working out
    # which QML modules a given target depends on at configure time, so we
    # have to be conservative and make ${target}_qmllint targets depend on all
    # *.qmltypes files. We need to provide a target for those dependencies
    # here. Note that we can't use ${target} itself for those dependencies
    # because the user might want to run qmllint without having to build the
    # QML module.
    add_custom_target(${target}_qmltyperegistration
        DEPENDS
            ${type_registration_cpp_file}
            ${plugin_types_file}
    )
    _qt_internal_assign_to_internal_targets_folder(${target}_qmltyperegistration)
    if(NOT TARGET all_qmltyperegistrations)
        add_custom_target(all_qmltyperegistrations)
        _qt_internal_assign_to_internal_targets_folder(all_qmltyperegistrations)
    endif()
    add_dependencies(all_qmltyperegistrations ${target}_qmltyperegistration)

    set(effective_target ${target})
    if(arg_REGISTRATIONS_TARGET)
        set(effective_target ${arg_REGISTRATIONS_TARGET})
    endif()

    # Both ${effective_target} (via target_sources) and ${target}_qmltyperegistration (via
    # add_custom_target DEPENDS option) depend on ${type_registration_cpp_file}.
    # The new Xcode build system requires a common target to drive the generation of files,
    # otherwise project configuration fails.
    # Make ${effective_target} the common target, by adding it as a dependency for
    # ${target}_qmltyperegistration.
    # The consequence is that the ${target}_qmllint target will now first build ${effective_target}
    # when using the Xcode generator (mostly only relevant for projects using Qt for iOS).
    # See QTBUG-95763.
    if(CMAKE_GENERATOR STREQUAL "Xcode")
        add_dependencies(${target}_qmltyperegistration ${effective_target})
    endif()

    target_sources(${effective_target} PRIVATE ${type_registration_cpp_file})

    # FIXME: The generated .cpp file has usually lost the path information for
    #        the headers it #include's. Since these generated .cpp files are in
    #        the build directory away from those headers, the header search path
    #        has to be augmented to ensure they can be found. We don't know what
    #        paths are needed, but add the source directory to at least handle
    #        the common case of headers in the same directory as the target.
    #        See QTBUG-93443.
    target_include_directories(${effective_target} PRIVATE ${target_source_dir})

    # Circumvent "too many sections" error when doing a 32 bit debug build on Windows with
    # MinGW.
    set(additional_source_files_properties "")
    if(MINGW)
        set(additional_source_files_properties "COMPILE_OPTIONS" "-Wa,-mbig-obj")
    elseif(MSVC)
        set(additional_source_files_properties "COMPILE_OPTIONS" "/bigobj")
    endif()

    # We can't rely on policy CMP0118 since user project controls it
    set(scope_args)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
        set(scope_args TARGET_DIRECTORY ${target})
    endif()
    _qt_internal_set_source_file_generated(
        SOURCES ${type_registration_cpp_file}
        ${scope_args}
        SKIP_AUTOGEN
    )
    if(additional_source_files_properties)
        set_source_files_properties(
            ${type_registration_cpp_file}
            ${scope_args}
            PROPERTIES
            ${additional_source_files_properties}
        )
    endif()

    target_include_directories(${effective_target} PRIVATE
        $<TARGET_PROPERTY:${QT_CMAKE_EXPORT_NAMESPACE}::Qml,INTERFACE_INCLUDE_DIRECTORIES>
    )
endfunction()

function(qt6_qml_type_registration)
    message(FATAL_ERROR
        "This function, previously available under Technical Preview, has been removed. "
        "Please use qt6_add_qml_module() instead."
    )
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_qml_type_registration)
        message(FATAL_ERROR
            "This function, previously available under Technical Preview, has been removed. "
            "Please use qt_add_qml_module() instead."
        )
    endfunction()
endif()

# Get Qt provided QML import paths.
# The appending order is important. Build dirs are preferred over install dirs.
function(_qt_internal_get_main_qt_qml_import_paths out_var)
    set(qml_import_paths "")

    # When building a qml module as part of a prefix Qt build that is not yet installed, add an
    # extra import path which is the current module's build dir: we need
    # this to ensure correct importing of QML modules when having a prefix-build
    # with QLibraryInfo::path(QLibraryInfo::QmlImportsPath) pointing to the
    # install location.
    if(QT_BUILDING_QT AND QT_WILL_INSTALL)
        set(build_dir_import_path "${QT_BUILD_DIR}/${INSTALL_QMLDIR}")
        if(IS_DIRECTORY "${build_dir_import_path}")
            list(APPEND qml_import_paths "${build_dir_import_path}")
        endif()
    endif()

    # We could have multiple additional installation prefixes: one per Qt repository (conan).
    # Or just extra ones, that might be needed during cross-compilation or example building.
    # Add those that have a "qml" subdirectory.
    # The additional prefixes have priority over the main Qt prefix, so they come first.
    if(NOT "${_qt_additional_packages_prefix_paths}" STREQUAL "")
        __qt_internal_prefix_paths_to_roots(
            additional_root_paths "${_qt_additional_packages_prefix_paths}")
        foreach(root IN ITEMS ${additional_root_paths})
            set(candidate "${root}/${QT6_INSTALL_QML}")
            if(IS_DIRECTORY "${candidate}")
                list(APPEND qml_import_paths "${candidate}")
            endif()
        endforeach()
    endif()

    # Add the main Qt "<prefix>/qml" import path.
    if(QT6_INSTALL_PREFIX)
        set(main_import_path "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_QML}")
        if(IS_DIRECTORY "${main_import_path}")
            list(APPEND qml_import_paths "${main_import_path}")
        endif()
    endif()

    set(${out_var} "${qml_import_paths}" PARENT_SCOPE)
endfunction()

function(_qt_internal_collect_target_qml_import_paths out_var target)
    set(qml_import_paths "")
    # Get custom import paths provided during qt_add_qml_module call.
    get_target_property(qml_import_path ${target} QT_QML_IMPORT_PATH)
    if(qml_import_path)
        list(APPEND qml_import_paths ${qml_import_path})
    endif()

    # Facilitate self-import so it can find the qmldir file. We also try to walk
    # back up the directory structure to find a base path under which this QML
    # module is located. Such a base path is likely to be used for other QML
    # modules that we might need to find, so add it to the import path if we
    # find a compatible directory structure. It doesn't make sense to do this
    # for an executable though, since it can never be found as a QML module for
    # a different QML module/target.
    get_target_property(target_type ${target} TYPE)
    get_target_property(android_type ${target} _qt_android_target_type)
    if(target_type STREQUAL "EXECUTABLE" OR android_type STREQUAL "APPLICATION")
        # The executable's own QML module's qmldir file will usually be under a
        # subdirectory (matching the module's target path) below the target's
        # build directory.

        get_target_property(qml_import_path ${target} BINARY_DIR)
        if (qml_import_path)
            list(APPEND qml_import_paths ${qml_import_path})
        endif()
    elseif(target_type MATCHES "LIBRARY")
        get_target_property(output_dir  ${target} QT_QML_MODULE_OUTPUT_DIRECTORY)
        get_target_property(target_path ${target} QT_QML_MODULE_TARGET_PATH)
        if(output_dir MATCHES "${target_path}$")
            string(REGEX REPLACE "(.*)/${target_path}" "\\1" base_dir "${output_dir}")
            list(APPEND qml_import_paths "${base_dir}")
        else()
            message(WARNING
                "The ${target} target is a QML module with target path ${target_path}. "
                "It uses an OUTPUT_DIRECTORY of ${output_dir}, which should end in the "
                "same target path, but doesn't. Tooling such as qmllint may not work "
                "correctly."
            )
        endif()
    endif()

    # Facilitate self-import so we can find the qmldir file
    get_target_property(module_out_dir ${target} QT_QML_MODULE_OUTPUT_DIRECTORY)
    if(module_out_dir)
        list(APPEND qml_import_paths "${module_out_dir}")
    endif()

    # Find qmldir files we copied to the build directory
    if(NOT "${QT_QML_OUTPUT_DIRECTORY}" STREQUAL "")
        if(EXISTS "${QT_QML_OUTPUT_DIRECTORY}")
            list(APPEND qml_import_paths "${QT_QML_OUTPUT_DIRECTORY}")
        endif()
    else()
        list(APPEND qml_import_paths "${CMAKE_CURRENT_BINARY_DIR}")
    endif()

    set(${out_var} "${qml_import_paths}" PARENT_SCOPE)
endfunction()

function(_qt_internal_collect_qml_import_paths out_var target)
    _qt_internal_collect_target_qml_import_paths(qml_import_paths ${target})

    # Add the Qt import paths last.
    _qt_internal_get_main_qt_qml_import_paths(qt_main_import_paths)
    list(APPEND qml_import_paths ${qt_main_import_paths})

    set(${out_var} "${qml_import_paths}" PARENT_SCOPE)
endfunction()

# This function returns the path to the qmlimportscanner executable.
# The are a few cases to handle:
# When used in a user project, the tool should already be built and we can find the path
# via the imported target.
# When used in an example that is built as part of the qtdeclarative build (or top-level build),
# there is no imported target yet. Here we have to differentiate whether the tool will be run at
# build time or configure time.
# If at configure time, we show an error, there is nothing to run yet. Such a setup is currently not
# supported.
# If at build time, we return the path where the built tool will be located after it is built.
function(_qt_internal_find_qmlimportscanner_path out_path scan_at_configure_time)
    # Find location of qmlimportscanner via the imported target.
    set(tool_path "")
    set(tool_name "qmlimportscanner")
    set(import_scanner_target "${QT_CMAKE_EXPORT_NAMESPACE}::${tool_name}")
    if(TARGET "${import_scanner_target}")
        get_target_property(tool_path "${import_scanner_target}" IMPORTED_LOCATION)
        if(NOT tool_path)
            set(configs "RELWITHDEBINFO;RELEASE;MINSIZEREL;DEBUG")
            foreach(config ${configs})
                get_target_property(tool_path
                    "${import_scanner_target}" IMPORTED_LOCATION_${config})
                if(tool_path)
                    break()
                endif()
            endforeach()
        endif()
    endif()

    if(NOT QT_BUILDING_QT)
        set(building_user_project TRUE)
    else()
        set(building_user_project FALSE)
    endif()

    if(NOT EXISTS "${tool_path}"
            AND (building_user_project OR scan_at_configure_time))
        message(FATAL_ERROR "The qmlimportscanner tool could not be found.
Possible reasons include:
* The file was deleted, renamed, or moved to another location.
* An install or uninstall procedure did not complete successfully.
* The installation was faulty.
")
    endif()

    # We are building Qt, the tool is not built yet and we need to run it at build time.
    if(NOT tool_path)
        qt_path_join(tool_path
            "${QT_BUILD_DIR}/${INSTALL_LIBEXECDIR}"
            "${tool_name}${CMAKE_EXECUTABLE_SUFFIX}")
    endif()

    set(${out_path} "${tool_path}" PARENT_SCOPE)
endfunction()

function(_qt_internal_scan_qml_imports target imports_file_var when_to_scan)
    if(NOT "${ARGN}" STREQUAL "")
        message(FATAL_ERROR "Unknown/unexpected arguments: ${ARGN}")
    endif()

    if(when_to_scan STREQUAL "BUILD_PHASE")
        set(scan_at_build_time TRUE)
        set(scan_at_configure_time FALSE)
        set(imports_file_infix "build")
    elseif(when_to_scan STREQUAL "IMMEDIATELY")
        set(scan_at_build_time FALSE)
        set(scan_at_configure_time TRUE)
        set(imports_file_infix "conf")
    else()
        message(FATAL_ERROR "Unexpected value for when_to_scan: ${when_to_scan}")
    endif()

    _qt_internal_find_qmlimportscanner_path(tool_path "${scan_at_configure_time}")

    get_target_property(target_source_dir ${target} SOURCE_DIR)
    get_target_property(target_binary_dir ${target} BINARY_DIR)
    set(out_dir "${target_binary_dir}/.qt/qml_imports")

    # Create separate files for scanning at build time vs configure time. Otherwise calling
    # ninja clean will re-run qmlimportscanner directly after the clean, which is
    # both weird and sometimes prints warnings due to the tool not finding qml files that were
    # cleaned from the build dir.
    set(file_base_name "${target}_${imports_file_infix}")

    set(imports_file "${out_dir}/${file_base_name}.cmake")
    set(${imports_file_var} "${imports_file}" PARENT_SCOPE)
    file(MAKE_DIRECTORY ${out_dir})

    set(cmd_args
        -rootPath "${target_source_dir}"
        -cmake-output
        -output-file "${imports_file}"
    )

    _qt_internal_collect_qml_import_paths(qml_import_paths ${target})
    list(REMOVE_DUPLICATES qml_import_paths)

    # Construct the -importPath arguments.
    set(import_path_arguments)
    foreach(path IN LISTS qml_import_paths)
        if(EXISTS "${path}" OR scan_at_build_time)
            list(APPEND import_path_arguments -importPath ${path})
        else()
            message(DEBUG "The import path ${path} is mentioned for ${target}, but it doesn't"
                " exists.")
        endif()
    endforeach()

    list(APPEND cmd_args ${import_path_arguments})

    # All of the module's .qml files will be listed in one of the generated
    # .qrc files, so there's no need to list the files individually. We provide
    # the .qrc files instead because they have the additional information for
    # each file's resource alias.
    get_property(qrc_files TARGET ${target} PROPERTY _qt_generated_qrc_files)
    if (qrc_files)
        list(APPEND cmd_args "-qrcFiles" ${qrc_files})
    endif()

    # Allow passing extra root paths, for cases when the qml sources might be outside the target
    # source directory, which is the case for some QuickControls tests.
    # Check for both target properties and directory variables.
    get_target_property(extra_root_paths "${target}" QT_QML_IMPORT_SCANNER_EXTRA_ROOT_PATHS)
    if(extra_root_paths)
        foreach(extra_root_path IN LISTS extra_root_paths)
            list(APPEND cmd_args -rootPath "${extra_root_path}")
        endforeach()
    endif()
    if(QT_QML_IMPORT_SCANNER_EXTRA_ROOT_PATHS)
        foreach(extra_root_path IN LISTS QT_QML_IMPORT_SCANNER_EXTRA_ROOT_PATHS)
            list(APPEND cmd_args -rootPath "${extra_root_path}")
        endforeach()
    endif()

    # Allow passing extra command args, because it might be useful during troubleshooting or for
    # very specific test cases.
    get_target_property(extra_args "${target}" QT_QML_IMPORT_SCANNER_EXTRA_ARGS)
    if(extra_args)
        list(APPEND cmd_args ${extra_args})
    endif()
    if(QT_QML_IMPORT_SCANNER_EXTRA_ARGS)
        list(APPEND cmd_args ${QT_QML_IMPORT_SCANNER_EXTRA_ARGS})
    endif()

    # Use a response file to avoid command line length issues if we have a lot
    # of arguments on the command line
    string(LENGTH "${cmd_args}" length)
    if(length GREATER 240)
        set(rsp_file "${out_dir}/${file_base_name}.rsp")
        list(JOIN cmd_args "\n" rsp_file_content)
        file(WRITE ${rsp_file} "${rsp_file_content}")
        set(cmd_args "@${rsp_file}")
    endif()

    _qt_internal_get_tool_wrapper_script_path(tool_wrapper)
    set(import_scanner_args "${tool_path}" ${cmd_args})

    # Run qmlimportscanner to generate the cmake file that records the import entries
    if(scan_at_build_time)
        add_custom_command(
            OUTPUT "${imports_file}"
            COMMENT "Running qmlimportscanner for ${target}"
            COMMAND ${tool_wrapper} ${import_scanner_args}
            WORKING_DIRECTORY ${target_source_dir}
            DEPENDS
                ${tool_path}
                ${qrc_files}
                $<TARGET_PROPERTY:${target},QT_QML_MODULE_QML_FILES>
            VERBATIM
        )
        add_custom_target(${target}_qmlimportscan DEPENDS "${imports_file}")
        _qt_internal_assign_to_internal_targets_folder(${target}_qmlimportscan)
        add_dependencies(${target} ${target}_qmlimportscan)
    else()
        message(VERBOSE "Running qmlimportscanner for ${target}.")
        list(JOIN import_scanner_args " " import_scanner_args_string)
        message(DEBUG "qmlimportscanner command: ${import_scanner_args_string}")

        # Pack arguments to avoid escaping issues.
        set(import_scanner_execute_process_args
            COMMAND ${import_scanner_args}
            WORKING_DIRECTORY "${target_source_dir}"
            RESULT_VARIABLE result
        )
        _qt_internal_execute_proccess_in_qt_env(import_scanner_execute_process_args)
        if(result)
            message(FATAL_ERROR
                "Failed to scan target ${target} for QML imports: ${result}"
            )
        endif()
    endif()
endfunction()

# Parse the entry at the specified index, assuming the caller already included
# the file generated by a call to _qt_internal_scan_qml_imports()
macro(_qt_internal_parse_qml_imports_entry prefix index)
    cmake_parse_arguments("${prefix}"
        ""
        "CLASSNAME;NAME;PATH;PLUGIN;RELATIVEPATH;TYPE;VERSION;LINKTARGET;PREFER"
        "COMPONENTS;SCRIPTS"
        ${qml_import_scanner_import_${index}}
    )
endmacro()


# This function is called as a finalizer in qt6_finalize_executable() for any
# target that links against the Qml library.
function(qt6_import_qml_plugins target)
    if(NOT TARGET ${QT_CMAKE_EXPORT_NAMESPACE}::qmlimportscanner)
        return()
    endif()

    get_target_property(is_imported ${QT_CMAKE_EXPORT_NAMESPACE}::qmlimportscanner IMPORTED)
    if(NOT is_imported)
        message(DEBUG "qt6_import_qml_plugins is called before qmlimportscanner is built."
            " Skip calling qmlimportscanner because it doesn't yet exist.")
        return()
    endif()

    # Protect against being called multiple times in case we are being called
    # explicitly before the finalizer is invoked.
    get_target_property(already_imported ${target} _QT_QML_PLUGINS_IMPORTED)
    get_target_property(no_import_scan   ${target} QT_QML_MODULE_NO_IMPORT_SCAN)
    if(already_imported OR no_import_scan)
        return()
    endif()

    # Return early for test-like executables only for shared qt builds,
    # to to avoid very long re-configuration times (many tests running qmlimportscanner takes a
    # long time).
    get_target_property(is_test_executable "${target}" _qt_is_test_executable)
    get_target_property(is_benchmark_test "${target}" _qt_is_benchmark_test)
    if((is_test_executable OR is_benchmark_test)
        AND BUILD_SHARED_LIBS
        AND NOT QT_INTERNAL_FORCE_QML_IMPORT_SCAN_FOR_TESTS
        )
        return()
    endif()

    set_target_properties(${target} PROPERTIES _QT_QML_PLUGINS_IMPORTED TRUE)

    _qt_internal_scan_qml_imports(${target} imports_file IMMEDIATELY)
    include("${imports_file}")

    # Parse the generated cmake file.
    # It is possible for the scanner to find no usage of QML, in which case the import count is 0.
    if(qml_import_scanner_imports_count GREATER 0)
        set(added_plugins "")
        set(plugins_to_link "")
        set(plugin_inits_to_link "")

        math(EXPR last_index "${qml_import_scanner_imports_count} - 1")
        foreach(index RANGE 0 ${last_index})
            _qt_internal_parse_qml_imports_entry(entry ${index})
            if(entry_PATH)
                if(NOT entry_PLUGIN)
                    # Check if qml module is built within the build tree, and should have a plugin
                    # target, but its qmldir file is not generated yet.
                    _qt_internal_find_qml_module(qml_module BY_OUTPUT_DIRS "${entry_PATH}")
                    if(qml_module)
                        get_target_property(entry_LINKTARGET
                            ${qml_module} QT_QML_MODULE_PLUGIN_TARGET)
                        if(entry_LINKTARGET AND TARGET ${entry_LINKTARGET})
                            get_target_property(entry_PLUGIN ${entry_LINKTARGET} OUTPUT_NAME)
                        endif()
                    endif()
                endif()

                if(entry_PLUGIN)
                    # Sometimes a plugin appears multiple times with different versions.
                    # Make sure to process it only once.
                    list(FIND added_plugins "${entry_PLUGIN}" _index)
                    if(NOT _index EQUAL -1)
                        continue()
                    endif()
                    list(APPEND added_plugins "${entry_PLUGIN}")

                    # Link against the Qml plugin.
                    # For plugins provided by Qt, we assume those plugin targets are already defined
                    # (typically brought in via find_package(Qt6...) ).
                    # For other plugins, the targets can come from the project itself.
                    #
                    if(entry_LINKTARGET)
                        if(TARGET ${entry_LINKTARGET})
                            get_target_property(target_type ${entry_LINKTARGET} TYPE)
                            if(target_type STREQUAL "STATIC_LIBRARY")
                                list(APPEND plugins_to_link "${entry_LINKTARGET}")
                            endif()
                        else()
                            message(WARNING
                                "The qml plugin '${entry_PLUGIN}' is a dependency of '${target}', "
                                "but the link target it defines (${entry_LINKTARGET}) does not "
                                "exist in the current scope. The plugin will not be linked."
                            )
                        endif()
                    elseif(TARGET ${entry_PLUGIN})
                        get_target_property(target_type ${entry_PLUGIN} TYPE)
                        if(target_type STREQUAL "STATIC_LIBRARY")
                            list(APPEND plugins_to_link "${entry_PLUGIN}")
                        endif()
                    else()
                        # TODO: QTBUG-94605 Figure out if this is a reasonable scenario to support
                        message(WARNING
                            "The qml plugin '${entry_PLUGIN}' is a dependency of '${target}', "
                            "but there is no target by that name in the current scope. The plugin "
                            "will not be linked."
                        )
                    endif()
                endif()
            endif()
        endforeach()

        if(plugins_to_link)
            # If ${target} is an executable or a shared library, link the plugins directly to
            # the target.
            # If ${target} is a static or INTERFACE library, the plugins should be propagated
            # across those libraries to the end target (executable or shared library).
            # The plugin initializers will be linked via usage requirements from the plugin target.
            get_target_property(target_type ${target} TYPE)
            if(target_type STREQUAL "EXECUTABLE" OR target_type STREQUAL "SHARED_LIBRARY"
                OR target_type STREQUAL "MODULE_LIBRARY")
                set(link_type "PRIVATE")
            else()
                set(link_type "INTERFACE")
            endif()
            target_link_libraries("${target}" ${link_type} ${plugins_to_link})
        endif()
    endif()
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_import_qml_plugins)
        if(QT_DEFAULT_MAJOR_VERSION EQUAL 5)
            qt5_import_qml_plugins(${ARGV})
        elseif(QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_import_qml_plugins(${ARGV})
        endif()
    endfunction()
endif()

function(_qt_internal_add_import_qml_plugins_finalizer target)
    get_property(finalizer_added TARGET ${target} PROPERTY
        _qt_qml_import_qml_plugins_finalizer_added)
    if(NOT finalizer_added)
        set_property(TARGET ${target} APPEND PROPERTY
            INTERFACE_QT_EXECUTABLE_FINALIZERS
            qt6_import_qml_plugins
        )
        set_property(TARGET ${target} PROPERTY _qt_qml_import_qml_plugins_finalizer_added TRUE)
    endif()
endfunction()

function(_qt_internal_add_qml_deploy_info_finalizer target)
    get_property(finalizer_added TARGET ${target} PROPERTY _qt_qml_deploy_finalizer_added)
    if(NOT finalizer_added)
        set_property(TARGET ${target} APPEND PROPERTY
            INTERFACE_QT_EXECUTABLE_FINALIZERS
            _qt_internal_generate_deploy_qml_imports_script
        )
        set_property(TARGET ${target} PROPERTY _qt_qml_deploy_finalizer_added TRUE)
    endif()
endfunction()

# This function may be called as a finalizer in qt6_finalize_executable() for any
# target that links against the Qml library for a shared Qt.
function(_qt_internal_generate_deploy_qml_imports_script target)
    if(NOT QT6_IS_SHARED_LIBS_BUILD)
        return()
    endif()
    get_target_property(target_type ${target} TYPE)
    # TODO: Handle Android where executables are module libraries instead
    if(NOT target_type STREQUAL "EXECUTABLE")
        return()
    endif()

    # Protect against being called multiple times in case we are being called
    # explicitly before the finalizer is invoked.
    get_target_property(already_generated ${target} _QT_QML_PLUGIN_SCAN_GENERATED)
    get_target_property(no_import_scan    ${target} QT_QML_MODULE_NO_IMPORT_SCAN)
    if(already_generated OR no_import_scan)
        return()
    endif()

    # Return early for test-like executables, because deployment of auto tests doesn't make sense.
    get_target_property(is_test_executable "${target}" _qt_is_test_executable)
    get_target_property(is_benchmark_test "${target}" _qt_is_benchmark_test)
    if((is_test_executable OR is_benchmark_test)
        AND NOT QT_INTERNAL_FORCE_QML_DEPLOY_SCAN_FOR_TESTS
        )
        return()
    endif()

    set_target_properties(${target} PROPERTIES _QT_QML_PLUGIN_SCAN_GENERATED TRUE)

    # Defer actually running qmlimportscanner until build time. This keeps the
    # configure step fast and takes advantage of the build step supporting
    # parallel execution if there are multiple targets that need scanning.
    _qt_internal_scan_qml_imports(${target} imports_file BUILD_PHASE)

    set(is_bundle FALSE)
    if(APPLE)
        if(IOS)
            message(FATAL_ERROR "Install support not available for iOS builds")
        endif()
        get_target_property(is_bundle ${target} MACOSX_BUNDLE)
    endif()
    set(is_bundle "$<BOOL:${is_bundle}>")

    # For macOS app bundles, the directory layout must conform to Apple's
    # requirements, so we hard-code the required structure. This assumes the
    # app bundle is installed to the base dir with an install command like:
    #   install(TARGETS ${target} BUNDLE DESTINATION .)
    set(bundle_qml_dir     "$<TARGET_FILE_NAME:${target}>.app/Contents/Resources/qml")
    set(bundle_plugins_dir "$<TARGET_FILE_NAME:${target}>.app/Contents/PlugIns")

    _qt_internal_get_deploy_impl_dir(deploy_impl_dir)
    string(MAKE_C_IDENTIFIER "${target}" target_id)
    set(filename "${deploy_impl_dir}/deploy_qml_imports/${target_id}")
    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)
    if(is_multi_config)
        string(APPEND filename "-$<CONFIG>")
    endif()
    string(APPEND filename ".cmake")

    # TODO: Fix macOS multi-config bundles to work.
    file(GENERATE OUTPUT "${filename}" CONTENT
"# Auto-generated deploy QML imports script for target \"${target}\".
# Do not edit, all changes will be lost.
# This file should only be included by qt6_deploy_qml_imports().

set(__qt_opts $<${is_bundle}:BUNDLE>)
if(arg_NO_QT_IMPORTS)
    list(APPEND __qt_opts NO_QT_IMPORTS)
endif()

_qt_internal_deploy_qml_imports_for_target(
    \${__qt_opts}
    IMPORTS_FILE \"${imports_file}\"
    PLUGINS_FOUND __qt_internal_plugins_found
    QML_DIR     \"$<IF:${is_bundle},${bundle_qml_dir},\${arg_QML_DIR}>\"
    PLUGINS_DIR \"$<IF:${is_bundle},${bundle_plugins_dir},\${arg_PLUGINS_DIR}>\"
)

if(arg_PLUGINS_FOUND)
    set(\${arg_PLUGINS_FOUND} \"\${__qt_internal_plugins_found}\" PARENT_SCOPE)
endif()
")

endfunction()

function(qt6_generate_deploy_qml_app_script)
    # We take the target using a TARGET keyword instead of as the first
    # positional argument so that we have a consistent signature with the
    # qt6_generate_deploy_app_script() from qtbase. That function might accept
    # an executable instead of a target in the future, but we can't because we
    # need information associated with the target (scanning all its .qml files
    # for imported QML modules).
    set(no_value_options
        NO_PLUGINS
        NO_UNSUPPORTED_PLATFORM_ERROR
        NO_TRANSLATIONS
        NO_COMPILER_RUNTIME
        MACOS_BUNDLE_POST_BUILD
        DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
    )
    set(single_value_options
        TARGET
        OUTPUT_SCRIPT
    )
    set(qt_deploy_runtime_dependencies_options
        # These options are forwarded as is to qt_deploy_runtime_dependencies.
        EXCLUDE_PLUGINS
        EXCLUDE_PLUGIN_TYPES
        INCLUDE_PLUGINS
        INCLUDE_PLUGIN_TYPES
        PRE_INCLUDE_REGEXES
        PRE_EXCLUDE_REGEXES
        POST_INCLUDE_REGEXES
        POST_EXCLUDE_REGEXES
        POST_INCLUDE_FILES
        POST_EXCLUDE_FILES
        DEPLOY_TOOL_OPTIONS
    )
    set(multi_value_options
        ${qt_deploy_runtime_dependencies_options}
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )
    if(arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unexpected arguments: ${arg_UNPARSED_ARGUMENTS}")
    endif()
    if(NOT arg_TARGET)
        message(FATAL_ERROR "TARGET must be specified")
    endif()

    if(NOT arg_OUTPUT_SCRIPT)
        message(FATAL_ERROR "OUTPUT_SCRIPT must be specified")
    endif()

    # Check that the target was defer-finalized, and not immediately finalized when using
    # CMake < 3.19. This is important because if it's immediately finalized, Qt::Qml is likely
    # not in the dependency list, and thus _qt_internal_generate_deploy_qml_imports_script will
    # not be executed, leading to an error at install time
    # 'No QML imports information recorded for target X'.
    # _qt_is_immediately_finalized is set by qt6_add_executable.
    # TODO: Remove once minimum required CMAKE_VERSION is 3.19+.
    get_target_property(is_immediately_finalized "${arg_TARGET}" _qt_is_immediately_finalized)
    if(is_immediately_finalized)
        message(FATAL_ERROR
            "QML app deployment requires CMake version 3.19, or later, or manual executable "
            "finalization. For manual finalization, pass the MANUAL_FINALIZATION option to "
            "qt_add_executable() and then call qt_finalize_target(${arg_TARGET}) just before
            calling qt_generate_deploy_qml_app_script().")
    endif()

    # Generate a descriptive deploy script name.
    string(MAKE_C_IDENTIFIER "${arg_TARGET}" target_id)
    set(deploy_script_name "qml_app_${target_id}")

    get_target_property(is_bundle ${arg_TARGET} MACOSX_BUNDLE)

    set(unsupported_platform_extra_message "")
    if(QT6_IS_SHARED_LIBS_BUILD)
        set(qt_build_type_string "shared Qt libs")
    else()
        set(qt_build_type_string "static Qt libs")
    endif()

    if(CMAKE_CROSSCOMPILING)
        string(APPEND qt_build_type_string ", cross-compiled")
    endif()

    if(NOT is_bundle)
        string(APPEND qt_build_type_string ", non-bundle app")
        set(unsupported_platform_extra_message
            "Executable targets have to be app bundles to use this command on Apple platforms.")
    endif()

    set(skip_message
        "_qt_internal_show_skip_runtime_deploy_message(\"${qt_build_type_string}\"")
    if(unsupported_platform_extra_message)
        string(APPEND skip_message
            "\n    EXTRA_MESSAGE \"${unsupported_platform_extra_message}\"")
    endif()
    string(APPEND skip_message "\n)")

    set(common_deploy_args "")
    if(arg_NO_PLUGINS)
        string(APPEND common_deploy_args "    NO_PLUGINS\n")
    endif()
    if(arg_NO_TRANSLATIONS)
        string(APPEND common_deploy_args "    NO_TRANSLATIONS\n")
    endif()
    if(arg_NO_COMPILER_RUNTIME)
        string(APPEND common_deploy_args "    NO_COMPILER_RUNTIME\n")
    endif()

    # Forward the arguments that are exactly the same for qt_deploy_runtime_dependencies.
    foreach(var IN LISTS qt_deploy_runtime_dependencies_options)
        if(NOT "${arg_${var}}" STREQUAL "")
            list(APPEND common_deploy_args ${var} ${arg_${var}})
        endif()
    endforeach()

    _qt_internal_should_skip_deployment_api(skip_deployment skip_reason)
    _qt_internal_should_skip_post_build_deployment_api(skip_post_build_deployment
        post_build_skip_reason)

    if(APPLE AND NOT IOS AND QT6_IS_SHARED_LIBS_BUILD AND is_bundle)
        # TODO: Consider handling non-bundle applications in the future using the generic cmake
        # runtime dependency feature.

        set(should_post_build FALSE)
        if(arg_MACOS_BUNDLE_POST_BUILD AND NOT skip_post_build_deployment)
            set(should_post_build TRUE)
        endif()

        # Generate the real deployment script when both post build step either deployment are
        # enabled.
        # If we skip deployment, but not the POST_BUILD step, we still need to generate the
        # regular deploy script to run it during POST_BUILD time.
        if(NOT skip_deployment OR should_post_build)
            qt6_generate_deploy_script(
                TARGET ${arg_TARGET}
                NAME ${deploy_script_name}
                OUTPUT_SCRIPT real_deploy_script
                CONTENT "
qt6_deploy_qml_imports(TARGET ${arg_TARGET} PLUGINS_FOUND plugins_found)
if(NOT DEFINED __QT_DEPLOY_POST_BUILD)
    qt6_deploy_runtime_dependencies(
        EXECUTABLE \"$<TARGET_FILE_NAME:${arg_TARGET}>.app\"
        ADDITIONAL_MODULES \${plugins_found}
    ${common_deploy_args})
endif()")
        endif()

        # Generate a no-op script either if we skip deployment or the post build step.
        if(skip_deployment)
            _qt_internal_generate_no_op_deploy_script(
                FUNCTION_NAME "qt6_generate_deploy_qml_app_script"
                SKIP_REASON "${skip_reason}"
                TARGET ${arg_TARGET}
                NAME ${deploy_script_name}
                OUTPUT_SCRIPT no_op_deploy_script
            )
        endif()

        if(skip_post_build_deployment)
            _qt_internal_generate_no_op_deploy_script(
                FUNCTION_NAME "qt6_generate_deploy_qml_app_script"
                SKIP_REASON "${post_build_skip_reason}"
                TARGET ${arg_TARGET}
                NAME ${deploy_script_name}
                OUTPUT_SCRIPT no_op_post_build_script
            )
        endif()

        # Choose which deployment script to use during installation.
        if(skip_deployment)
            set(deploy_script "${no_op_deploy_script}")
        else()
            set(deploy_script "${real_deploy_script}")
        endif()

        # Choose which deployment script to use during the post build step.
        if(should_post_build)
            set(post_build_deploy_script "${real_deploy_script}")
        elseif(skip_post_build_deployment)
            # Explicitly asked to skip post build, show a no-op message.
            set(post_build_deploy_script "${no_op_post_build_script}")
        endif()

        if(should_post_build OR skip_post_build_deployment)
            # We must not deploy the runtime dependencies, otherwise we interfere
            # with CMake's RPATH rewriting at install time. We only need the QML
            # imports deployed to the bundle anyway, the build RPATHs will allow
            # the regular libraries, frameworks and non-QML plugins to still be
            # found, even if they are outside the app bundle.

            # Support Xcode, which places the application build dir into a configuration specific
            # subdirectory. Override both the deploy prefix and install prefix, because we
            # differentiate them in the qml installation implementation due to ENV{DESTDIR}
            # handling.
            set(deploy_path_suffix "")
            get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)
            if(is_multi_config)
                set(deploy_path_suffix "/$<CONFIG>")
            endif()

            set(target_binary_dir_with_config_prefix
                "$<TARGET_PROPERTY:${arg_TARGET},BINARY_DIR>${deploy_path_suffix}")

            set(post_build_install_prefix
                "CMAKE_INSTALL_PREFIX=${target_binary_dir_with_config_prefix}")

            set(post_build_deploy_prefix
                "QT_DEPLOY_PREFIX=${target_binary_dir_with_config_prefix}")

            add_custom_command(TARGET ${arg_TARGET} POST_BUILD
                COMMAND ${CMAKE_COMMAND}
                -D "${post_build_install_prefix}"
                -D "${post_build_deploy_prefix}"
                -D "__QT_DEPLOY_POST_BUILD=TRUE"
                -P "${post_build_deploy_script}"
                VERBATIM
            )
        endif()
    elseif(skip_deployment)
            _qt_internal_generate_no_op_deploy_script(
                FUNCTION_NAME "qt6_generate_deploy_qml_app_script"
                SKIP_REASON "${skip_reason}"
                TARGET ${arg_TARGET}
                NAME ${deploy_script_name}
                OUTPUT_SCRIPT deploy_script
            )
    elseif(WIN32 AND QT6_IS_SHARED_LIBS_BUILD)
        qt6_generate_deploy_script(
            TARGET ${arg_TARGET}
            NAME ${deploy_script_name}
            OUTPUT_SCRIPT deploy_script
            CONTENT "
qt6_deploy_qml_imports(TARGET ${arg_TARGET} PLUGINS_FOUND plugins_found)
qt6_deploy_runtime_dependencies(
    EXECUTABLE \"$<TARGET_FILE:${arg_TARGET}>\"
    ADDITIONAL_MODULES \${plugins_found}
    GENERATE_QT_CONF
${common_deploy_args})")
    elseif(UNIX AND NOT APPLE AND NOT ANDROID AND NOT CMAKE_CROSSCOMPILING
            AND QT6_IS_SHARED_LIBS_BUILD)
        qt6_generate_deploy_script(
            TARGET ${arg_TARGET}
            NAME ${deploy_script_name}
            OUTPUT_SCRIPT deploy_script
            CONTENT "
qt6_deploy_qml_imports(TARGET ${arg_TARGET} PLUGINS_FOUND plugins_found)
qt6_deploy_runtime_dependencies(
    EXECUTABLE \"$<TARGET_FILE:${arg_TARGET}>\"
    ADDITIONAL_MODULES \${plugins_found}
    GENERATE_QT_CONF
${common_deploy_args})")
    elseif((arg_NO_UNSUPPORTED_PLATFORM_ERROR OR
            QT_INTERNAL_NO_UNSUPPORTED_PLATFORM_ERROR)
        AND (arg_DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
            OR QT_INTERNAL_DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM)
        AND QT6_IS_SHARED_LIBS_BUILD)
        # User project explicitly requested to deploy only user QML modules on a shared Qt libs
        # platform where qt_deploy_runtime_dependencies does not work.
        # This is useful for projects that will deploy the Qt QML and runtime libraries manually.
        # This also offers a migration path to enable qt_deploy_runtime_dependencies for
        # unsupported platforms without breaking projects that already handle runtime libs manually.
        # But for it to work cleanly, projects will have to enable both
        # NO_UNSUPPORTED_PLATFORM_ERROR and DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
        # conditionally per platform.
        qt6_generate_deploy_script(
            TARGET ${arg_TARGET}
            NAME ${deploy_script_name}
            OUTPUT_SCRIPT deploy_script
            CONTENT "
${skip_message}
qt6_deploy_qml_imports(TARGET ${arg_TARGET} NO_QT_IMPORTS)
")
    elseif(NOT arg_NO_UNSUPPORTED_PLATFORM_ERROR AND NOT QT_INTERNAL_NO_UNSUPPORTED_PLATFORM_ERROR)
        # Currently we don't deploy runtime dependencies if cross-compiling or using a static Qt.
        # Error out by default unless the project opted out of the error.
        # This provides us a migration path in the future without breaking compatibility promises.
        message(FATAL_ERROR
            "Support for installing runtime dependencies is not implemented for "
            "this target platform (${CMAKE_SYSTEM_NAME}, ${qt_build_type_string}). "
            ${unsupported_platform_extra_message}
        )
    else()
        qt6_generate_deploy_script(
            TARGET ${arg_TARGET}
            NAME ${deploy_script_name}
            OUTPUT_SCRIPT deploy_script
            CONTENT "
${skip_message}
_qt_internal_show_skip_qml_runtime_deploy_message()
")
    endif()

    set(${arg_OUTPUT_SCRIPT} ${deploy_script} PARENT_SCOPE)
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    macro(qt_generate_deploy_qml_app_script)
        qt6_generate_deploy_qml_app_script(${ARGV})
    endmacro()
endif()

function(qt6_query_qml_module target)

    if(NOT TARGET ${target})
        message(FATAL_ERROR "\"${target}\" is not a target")
    endif()

    get_target_property(is_imported ${target} IMPORTED)
    if(is_imported)
        message(FATAL_ERROR
            "Only targets built by the project can be used with this command, "
            "but target \"${target}\" is imported."
        )
    endif()

    get_target_property(uri ${target} QT_QML_MODULE_URI)
    if(NOT uri)
        message(FATAL_ERROR
            "Target \"${target}\" does not appear to be a QML module"
            )
    endif()

    set(no_value_options "")
    set(single_value_options
        URI
        VERSION
        PLUGIN_TARGET
        MODULE_RESOURCE_PATH
        TARGET_PATH
        QMLDIR
        TYPEINFO
        QML_FILES
        QML_FILES_DEPLOY_PATHS   # relative to target path
        QML_FILES_PREFIX_OVERRIDES
        RESOURCES
        RESOURCES_DEPLOY_PATHS   # relative to target path
        RESOURCES_PREFIX_OVERRIDES
    )
    set(multi_value_options "")
    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )
    if(arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unexpected arguments: ${arg_UNPARSED_ARGUMENTS}")
    endif()

    if(arg_URI)
        set(${arg_URI} "${uri}" PARENT_SCOPE)
    endif()

    if(arg_VERSION)
        get_property(version TARGET ${target} PROPERTY QT_QML_MODULE_VERSION)
        set(${arg_VERSION} "${version}" PARENT_SCOPE)
    endif()

    if(arg_PLUGIN_TARGET)
        # There might not be a plugin target, so return an empty string for that
        get_property(plugin_target TARGET ${target} PROPERTY QT_QML_MODULE_PLUGIN_TARGET)
        set(${arg_PLUGIN_TARGET} "${plugin_target}" PARENT_SCOPE)
    endif()

    if(arg_MODULE_RESOURCE_PATH)
        # Note that QT_QML_MODULE_RESOURCE_PREFIX is not the RESOURCE_PREFIX
        # passed to qt6_add_qml_module(). It is that plus the target path, which
        # corresponds to what we mean by the MODULE_RESOURCE_PATH.
        get_property(prefix TARGET ${target} PROPERTY QT_QML_MODULE_RESOURCE_PREFIX)
        set(${arg_MODULE_RESOURCE_PATH} "${prefix}" PARENT_SCOPE)
    endif()

    string(REPLACE "." "/" target_path "${uri}")
    if(arg_TARGET_PATH)
        set(${arg_TARGET_PATH} "${target_path}" PARENT_SCOPE)
    endif()

    get_target_property(output_dir ${target} QT_QML_MODULE_OUTPUT_DIRECTORY)

    if(arg_QMLDIR)
        set(${arg_QMLDIR} "${output_dir}/qmldir" PARENT_SCOPE)
    endif()

    # This should always be set to something non-empty
    # unless we've explicitly said NO_GENERATE_QMLTYPES
    get_target_property(typeinfo ${target} QT_QML_MODULE_TYPEINFO)
    if(arg_TYPEINFO AND typeinfo)
        set(${arg_TYPEINFO} "${output_dir}/${typeinfo}" PARENT_SCOPE)
    endif()

    get_target_property(target_source_dir ${target} SOURCE_DIR)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.18)
        set(scope_option TARGET_DIRECTORY ${target})
    else()
        set(scope_option "")
        if(NOT target_source_dir STREQUAL CMAKE_CURRENT_SOURCE_DIR AND
           (arg_QML_FILES_DEPLOY_PATHS OR arg_RESOURCES_DEPLOY_PATHS))
            # This isn't a fatal error because it will only be a problem if any
            # qml or resource files actually have source file properties set.
            message(WARNING
                "Calling qt6_query_qml_module() from a different directory scope "
                "to the one in which target \"${target}\" was created. "
                "This requires CMake 3.18 or later to be robust, but you are using "
                "CMake ${CMAKE_VERSION}. Deployment paths may not be correct."
            )
        endif()
    endif()

    # Because of how CMake lists work, in particular appending empty strings,
    # we have to use a placeholder to represent empty values and then replace
    # them at the end. If we don't do this, any list that starts with an empty
    # value ends up discarding that empty value because it is indistinguishable
    # from an empty list.
    set(empty_placeholder "__qt_empty_placeholder__")

    foreach(file_set IN ITEMS QML_FILES RESOURCES)
        # NOTE: We converted these files to absolute paths already when storing them
        get_target_property(files ${target} QT_QML_MODULE_${file_set})

        if(arg_${file_set})
            set(${arg_${file_set}} "${files}" PARENT_SCOPE)
        endif()

        if(arg_${file_set}_DEPLOY_PATHS OR arg_${file_set}_PREFIX_OVERRIDES)
            set(deploy_paths "")
            set(prefix_overrides "")
            foreach(abs_file IN LISTS files)
                # The QT_QML_MODULE_PREFIX_OVERRIDE is the PREFIX value that was passed to
                # qt_target_qml_sources. It has no relation to the QT_QML_MODULE_RESOURCE_PREFIX
                # property or the computed MODULE_RESOURCE_PATH variable above.
                get_property(prefix_override SOURCE ${abs_file} ${scope_option}
                    PROPERTY QT_QML_MODULE_PREFIX_OVERRIDE
                )
                if("${prefix_override}" STREQUAL "")
                    list(APPEND prefix_overrides "${empty_placeholder}")
                else()
                    list(APPEND prefix_overrides "${prefix_override}")
                endif()

                # We can't provide a deploy path when the resource prefix is
                # overridden. We still need to store an empty deploy path for it
                # though so that the file lists all line up correctly.
                if(NOT "${prefix_override}" STREQUAL "")
                    list(APPEND deploy_paths "${empty_placeholder}")
                else()
                    # Careful how we check whether this property is set. Projects might
                    # use a resource alias that matches one of CMake's false constants,
                    # so we must use get_property(), not get_source_file_property(),
                    # then compare the result with an empty string.
                    get_property(alias
                                 SOURCE ${abs_file} ${scope_option} PROPERTY QT_RESOURCE_ALIAS)
                    if(NOT "${alias}" STREQUAL "")
                        list(APPEND deploy_paths "${alias}")
                    else()
                        file(RELATIVE_PATH rel_file ${target_source_dir} ${abs_file})
                        list(APPEND deploy_paths "${rel_file}")
                    endif()
                endif()
            endforeach()
            string(REPLACE "${empty_placeholder}" "" deploy_paths "${deploy_paths}")
            string(REPLACE "${empty_placeholder}" "" prefix_overrides "${prefix_overrides}")
            if(arg_${file_set}_DEPLOY_PATHS)
                set(${arg_${file_set}_DEPLOY_PATHS} "${deploy_paths}" PARENT_SCOPE)
            endif()
            if(arg_${file_set}_PREFIX_OVERRIDES)
                set(${arg_${file_set}_PREFIX_OVERRIDES} "${prefix_overrides}" PARENT_SCOPE)
            endif()
        endif()
    endforeach()
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    macro(qt_query_qml_module)
        qt6_query_qml_module(${ARGV})
    endmacro()
endif()


function(_qt_internal_add_static_qml_plugin_dependencies plugin_target backing_target)
    # Protect against multiple calls of qt_add_qml_plugin.
    get_target_property(plugin_deps_added "${plugin_target}" _qt_extra_static_qml_plugin_deps_added)
    if(plugin_deps_added)
        return()
    endif()
    set_target_properties("${plugin_target}" PROPERTIES _qt_extra_static_qml_plugin_deps_added TRUE)

    # Get the install plugin target name, which we will need for filtering later on.
    if(TARGET "${backing_target}")
        get_target_property(installed_plugin_target
                            "${backing_target}" _qt_qml_module_installed_plugin_target)
    endif()

    if(NOT backing_target STREQUAL plugin_target AND TARGET "${backing_target}")
        set(has_backing_lib TRUE)
    else()
        set(has_backing_lib FALSE)
    endif()

    get_target_property(plugin_type ${plugin_target} TYPE)
    set(skip_prl_marker "$<BOOL:QT_IS_PLUGIN_GENEX>")

    # If ${plugin_target} is a static qml plugin, recursively get its private dependencies (and its
    # backing lib private deps), identify which of those are qml modules, extract any associated qml
    # plugin target from those qml modules and make them dependencies of ${plugin_target}.
    #
    # E.g. this ensures that if a user project links directly to the static qtquick2plugin plugin
    # target (note the plugin target, not the backing lib) it will automatically also link to
    # Quick's transitive plugin dependencies: qmlplugin, modelsplugin and workerscriptplugin, in
    # addition to the the Qml, QmlModels and QmlWorkerScript backing libraries.
    #
    # Note this logic is not specific to qtquick2plugin, it applies to all static qml plugins.
    #
    # This eliminates the needed boilerplate to link to the full transitive closure of qml plugins
    # in user projects that don't want to use qmlimportscanner / qt_import_qml_plugins.
    set(additional_plugin_deps "")

    if(plugin_type STREQUAL "STATIC_LIBRARY")
        set(all_private_deps "")

        # We walk both plugin_target and backing_lib private deps because they can have differing
        # dependencies and we want to consider all of them.
        __qt_internal_collect_all_target_dependencies(
            "${plugin_target}" plugin_private_deps)
        if(plugin_private_deps)
            list(APPEND all_private_deps ${plugin_private_deps})
        endif()

        if(has_backing_lib)
            __qt_internal_collect_all_target_dependencies(
                "${backing_target}" backing_lib_private_deps)
            if(backing_lib_private_deps)
                list(APPEND all_private_deps ${backing_lib_private_deps})
            endif()
        endif()

        foreach(dep IN LISTS all_private_deps)
            if(NOT TARGET "${dep}")
                continue()
            endif()
            get_target_property(dep_type ${dep} TYPE)
            if(dep_type STREQUAL "STATIC_LIBRARY")
                set(associated_qml_plugin "")

                # Check if the target has an associated imported qml plugin (like a Qt-provided
                # one).
                get_target_property(associated_qml_plugin_candidate ${dep}
                    _qt_qml_module_installed_plugin_target)

                if(associated_qml_plugin_candidate AND TARGET "${associated_qml_plugin_candidate}")
                    set(associated_qml_plugin "${associated_qml_plugin_candidate}")
                endif()

                # Check if the target has an associated qml plugin that's built as part of the
                # current project (non-installed one, so without a target namespace prefix).
                get_target_property(associated_qml_plugin_candidate ${dep}
                    _qt_qml_module_plugin_target)

                if(NOT associated_qml_plugin AND
                        associated_qml_plugin_candidate
                        AND TARGET "${associated_qml_plugin_candidate}")
                    set(associated_qml_plugin "${associated_qml_plugin_candidate}")
                endif()

                # We need to filter out adding the plugin_target as a dependency to itself,
                # when walking the backing lib of the plugin_target.
                if(associated_qml_plugin
                        AND NOT associated_qml_plugin STREQUAL plugin_target
                        AND NOT associated_qml_plugin STREQUAL installed_plugin_target)
                    # Abuse a genex marker, to skip the dependency to be added into prl files.
                    # TODO: Introduce a more generic marker name in qtbase specifically
                    # for skipping deps in prl file deps generation.
                    set(wrapped_associated_qml_plugin
                        "$<${skip_prl_marker}:$<TARGET_NAME:${associated_qml_plugin}>>")

                    if(NOT wrapped_associated_qml_plugin IN_LIST additional_plugin_deps)
                        list(APPEND additional_plugin_deps "${wrapped_associated_qml_plugin}")
                    endif()
                endif()
            endif()
        endforeach()
    endif()

    if(additional_plugin_deps)
        target_link_libraries(${plugin_target} PRIVATE ${additional_plugin_deps})
    endif()
endfunction()

# The function returns the output name of a qml plugin that will be used as library output
# name and in a qmldir file as the 'plugin <plugin_output_name>' record.
function(_qt_internal_get_qml_plugin_output_name out_var plugin_target)
    cmake_parse_arguments(arg
        ""
        "BACKING_TARGET;TARGET_PATH;URI"
        ""
        ${ARGN}
    )
    set(plugin_name)
    if(TARGET ${plugin_target})
        get_target_property(plugin_name ${plugin_target} OUTPUT_NAME)
    endif()
    if(NOT plugin_name)
        set(plugin_name "${plugin_target}")
    endif()

    if(ANDROID)
        # In Android all plugins are stored in directly the /libs directory. This means that plugin
        # names must be unique in scope of apk. To make this work we prepend uri-based prefix to
        # each qml plugin in case if users don't use the manually written qmldir files.
        get_target_property(no_generate_qmldir ${target} QT_QML_MODULE_NO_GENERATE_QMLDIR)
        if(TARGET "${arg_BACKING_TARGET}")
            get_target_property(no_generate_qmldir ${arg_BACKING_TARGET}
                QT_QML_MODULE_NO_GENERATE_QMLDIR)

            # Adjust Qml plugin names on Android similar to qml_plugin.prf which calls
            # $$qt5LibraryTarget($$TARGET, "qml/$$TARGETPATH/").
            # Example plugin names:
            # qtdeclarative
            #   TARGET_PATH: QtQml/Models
            #   file name:   libqml_QtQml_Models_modelsplugin_x86_64.so
            # qtquickcontrols2
            #   TARGET_PATH: QtQuick/Controls.2/Material
            #   file name:
            #     libqml_QtQuick_Controls.2_Material_qtquickcontrols2materialstyleplugin_x86_64.so
            if(NOT arg_TARGET_PATH)
                get_target_property(arg_TARGET_PATH ${arg_BACKING_TARGET}
                QT_QML_MODULE_TARGET_PATH)
            endif()
        endif()
        if(arg_TARGET_PATH)
            string(REPLACE "/" "_" android_plugin_name_infix_name "${arg_TARGET_PATH}")
        else()
            string(REPLACE "." "_" android_plugin_name_infix_name "${arg_URI}")
        endif()

        # If plugin supposed to use manually written qmldir file we don't prepend the uri-based
        # prefix to the plugin output name. User should keep the file name of a QML plugin in
        # qmldir the same as the name of plugin on a file system. Exception is the
        # ABI-/platform-specific suffix that has the separate processing and should not be
        # a part of plugin name in qmldir.
        if(NOT no_generate_qmldir)
            set(plugin_name
                "qml_${android_plugin_name_infix_name}_${plugin_name}")
        endif()
    endif()

    set(${out_var} "${plugin_name}" PARENT_SCOPE)
endfunction()

# Used to add extra dependencies between ${target} and ${dep_target} qml plugins in a static
# Qt build, without creating a dependency in the genereated qmake .prl files.
# These dependencies make manual linking to static plugins a nicer experience for users that don't
# want to use qt_import_qml_plugins.
function(_qt_internal_add_qml_static_plugin_dependency target dep_target)
    if(NOT BUILD_SHARED_LIBS)
        # Abuse a genex marker, to skip the dependency to be added into prl files.
        # TODO: Introduce a more generic marker name in qtbase specifically
        # for skipping deps in prl file deps generation.
        set(skip_prl_marker "$<BOOL:QT_IS_PLUGIN_GENEX>")
        target_link_libraries("${target}" PRIVATE
            "$<${skip_prl_marker}:$<TARGET_NAME:${dep_target}>>")
    endif()
endfunction()

function(_qt_internal_collect_qml_module_dependencies target)
    if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.19.0")
        string(JOIN " " collect_qml_module_dependencies_code
            "cmake_language(DEFER DIRECTORY \"${CMAKE_BINARY_DIR}\""
            "CALL _qt_internal_collect_qml_module_dependencies_deferred \"${target}\")"
        )
        cmake_language(EVAL CODE "${collect_qml_module_dependencies_code}")
    else()
        _qt_internal_collect_qml_module_dependencies_deferred("${target}")
    endif()
endfunction()

function(_qt_internal_collect_qml_module_dependencies_deferred target)
    # Add all dependencies that are known as targets first
    get_target_property(dep_targets ${target} QT_QML_DEPENDENT_QML_MODULE_TARGETS)
    set(known_dep_ids "")
    if(dep_targets)
        foreach(dep_target IN LISTS dep_targets)
            if(NOT TARGET "${dep_target}")
                message(FATAL_ERROR "${dep_target} is listed in"
                    " QT_QML_DEPENDENT_QML_MODULE_TARGETS property of ${target}, but not a target")
            endif()
            get_target_property(is_imported ${dep_target} IMPORTED)
            if(NOT is_imported)
                add_dependencies(${target} ${dep_target})
                qt6_query_qml_module(${dep_target} VERSION dep_target_version URI dep_target_URI)
                if(dep_target_version)
                    list(APPEND known_dep_ids "${dep_target_URI} ${dep_target_version}")
                else()
                    list(APPEND known_dep_ids "${dep_target_URI}")
                endif()
            endif()
        endforeach()
    endif()

    # Attempt adding the dependencies that are specified as URI's next.
    get_target_property(deps ${target} QT_QML_MODULE_DEPENDENCIES)
    if(NOT deps)
        return()
    endif()
    if(known_dep_ids)
        list(REMOVE_ITEM deps ${known_dep_ids})
    endif()
    foreach(dep IN LISTS deps)
        string(REPLACE " " ";" dep "${dep}")
        list(GET dep 0 dep_module_uri)

        list(LENGTH dep dep_length)
        if(dep_length GREATER_EQUAL 2)
            list(GET dep 1 dev_module_version)
            if(dev_module_version MATCHES "^[0-9]+\\.[0-9]+$")
                set(dev_module_version_arg VERSION "${dev_module_version}")
            endif()
        endif()
        _qt_internal_find_qml_module(dep_module BY_URI "${dep_module_uri}"
            ${dev_module_version_arg})
        if(dep_module)
            add_dependencies(${target} ${dep_module})
        endif()
    endforeach()
endfunction()

# Function searches the qml module target by the bound 'by' property.
#
# The supported 'by' arguments:
# BY_OUTPUT_DIR - searches the module using the _qt_all_qml_output_dirs GLOBAL property
# BY_URI - searches the module using _qt_all_qml_uris GLOBAL property
# Single value arguments:
# VERSION - look for the exact version or version higher then the VERSION
# EXACT_VERSION - look for the exact version
function(_qt_internal_find_qml_module out_target by filter)
    if(NOT by MATCHES "^BY_(URI|OUTPUT_DIR)")
        message(FATAL_ERROR "Unknow lookup argument ${by}")
    else()
        string(TOLOWER "${CMAKE_MATCH_1}" lookup_property)
        set(lookup_property "_qt_all_qml_${lookup_property}s")
    endif()

    cmake_parse_arguments(arg "" "VERSION;EXACT_VERSION" "" ${ARGN})

    if(arg_EXACT_VERSION AND arg_VERSION)
        message(FATAL_ERROR "Both VERSION and EXACT_VERSION are specified.")
    endif()

    get_property(lookup_property_value GLOBAL PROPERTY ${lookup_property})
    set(${out_target} "${out_target}-NOTFOUND")
    set(prev_module_version 0)
    if(lookup_property_value)
        set(index 0)
        foreach(lookup_value IN LISTS lookup_property_value)
            set(module_index "${index}")
            math(EXPR index "${index} + 1")
            if(NOT "${lookup_value}" STREQUAL "${filter}")
                continue()
            endif()
            get_property(qml_targets GLOBAL PROPERTY _qt_all_qml_targets)
            list(GET qml_targets ${module_index} qml_target)
            if(NOT TARGET "${qml_target}")
                continue()
            endif()
            if(NOT arg_VERSION AND NOT arg_EXACT_VERSION)
                set(${out_target} "${qml_target}")
                break()
            endif()

            qt6_query_qml_module("${qml_target}" VERSION module_version)
            if(arg_EXACT_VERSION)
                if(module_version VERSION_EQUAL arg_EXACT_VERSION)
                    set(${out_target} "${qml_target}")
                    break()
                endif()
            else()
                if(prev_module_version VERSION_GREATER module_version AND ${out_target})
                    continue()
                endif()
                set(prev_module_version "${module_version}")
                if(module_version VERSION_GREATER_EQUAL arg_VERSION)
                    set(${out_target} "${qml_target}")
                    if(module_version VERSION_EQUAL arg_VERSION)
                        break()
                    endif()
                endif()
            endif()
        endforeach()
    endif()

    set(${out_target} "${${out_target}}" PARENT_SCOPE)
endfunction()
