# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

macro(qt_internal_get_internal_add_module_keywords option_args single_args multi_args)
    set(${option_args}
        STATIC
        EXCEPTIONS
        FIND_PRIVATE_MODULE
        INTERNAL_MODULE
        HEADER_MODULE
        DISABLE_TOOLS_EXPORT
        SKIP_DEPENDS_INCLUDE
        NO_MODULE_HEADERS
        NO_SYNC_QT
        NO_PRIVATE_MODULE
        NO_CONFIG_HEADER_FILE
        NO_ADDITIONAL_TARGET_INFO
        NO_GENERATE_METATYPES
        NO_HEADERSCLEAN_CHECK
        NO_GENERATE_CPP_EXPORTS
        NO_UNITY_BUILD
        NO_PACKAGE_CONFIG_FILE
        NO_MODULE_JSON_FILE
        NO_QMAKE_SUPPORT_FILES
        ${__qt_internal_sbom_optional_args}
    )
    set(${single_args}
        MODULE_INCLUDE_NAME
        MODULE_INTERFACE_NAME
        CONFIG_MODULE_NAME
        PRECOMPILED_HEADER
        CONFIGURE_FILE_PATH
        CPP_EXPORT_HEADER_BASE_NAME
        EXTERNAL_HEADERS_DIR
        PRIVATE_HEADER_FILTERS
        QPA_HEADER_FILTERS
        RHI_HEADER_FILTERS
        SSG_HEADER_FILTERS
        HEADER_SYNC_SOURCE_DIRECTORY
        ELF_LINKER_DYNAMIC_LIST
        ${__default_target_info_args}
        ${__qt_internal_sbom_single_args}
    )
    set(${multi_args}
        QMAKE_MODULE_CONFIG
        EXTRA_CMAKE_FILES
        EXTRA_CMAKE_INCLUDES
        EXTERNAL_HEADERS
        POLICIES
        ${__default_private_args}
        ${__default_public_args}
        ${__default_private_module_args}
        ${__qt_internal_sbom_multi_args}
    )
endmacro()

# The function helps to wrap module include paths with the header existence check.
function(qt_internal_append_include_directories_with_headers_check target list_to_append type)
    string(TOLOWER "${type}" type)
    string(JOIN "" has_headers_check
        "$<BOOL:"
            "$<TARGET_PROPERTY:"
                "$<TARGET_NAME:${target}>,"
                "_qt_module_has_${type}_headers"
            ">"
        ">"
    )
    foreach(directory IN LISTS ARGN)
        list(APPEND ${list_to_append}
            "$<${has_headers_check}:${directory}>")
    endforeach()
    set(${list_to_append} "${${list_to_append}}" PARENT_SCOPE)
endfunction()

# This is the main entry function for creating a Qt module, that typically
# consists of a library, public header files, private header files and configurable
# features.
#
# A CMake target with the specified target parameter is created. If the current source
# directory has a configure.cmake file, then that is also processed for feature definition
# and testing. Any features defined as well as any features coming from dependencies to
# this module are imported into the scope of the calling feature.
#
# Target is without leading "Qt". So e.g. the "QtCore" module has the target "Core".
#
# Options:
#   NO_ADDITIONAL_TARGET_INFO
#     Don't generate a Qt6*AdditionalTargetInfo.cmake file.
#
#   MODULE_INTERFACE_NAME
#     The custom name of the module interface. This name is used as a part of the include paths
#     associated with the module and other interface names. The default value is the target name.
#     If the INTERNAL_MODULE option is specified, MODULE_INTERFACE_NAME is not specified and the
#     target name ends with the suffix 'Private', the MODULE_INTERFACE_NAME value defaults to the
#     non-suffixed target name, e.g.:
#        For the SomeInternalModulePrivate target, the MODULE_INTERFACE_NAME will be
#        SomeInternalModule
#
#   HEADER_MODULE
#     Creates an interface library instead of following the Qt configuration default. Mutually
#     exclusive with STATIC.
#
#   STATIC
#     Creates a static library instead of following the Qt configuration default. Mutually
#     exclusive with HEADER_MODULE.
#
#   EXTERNAL_HEADERS
#     A explicit list of non qt headers (like 3rdparty) to be installed.
#     Note this option overrides install headers used as PUBLIC_HEADER by cmake install(TARGET)
#     otherwise set by syncqt.
#
#   EXTERNAL_HEADERS_DIR
#     A module directory with non qt headers (like 3rdparty) to be installed.
#     Note this option overrides install headers used as PUBLIC_HEADER by cmake install(TARGET)
#     otherwise set by syncqt.
#
#   PRIVATE_HEADER_FILTERS
#     The regular expressions that filter private header files out of target sources.
#     The value must use the following format 'regex1|regex2|regex3'.
#
#   QPA_HEADER_FILTERS
#     The regular expressions that filter QPA header files out of target sources.
#     The value must use the following format 'regex1|regex2|regex3'.
#
#   RHI_HEADER_FILTERS
#     The regular expressions that filter RHI header files out of target sources.
#     The value must use the following format 'regex1|regex2|regex3'.
#
#   SSG_HEADER_FILTERS
#     The regular expressions that filter ssg header files out of target sources.
#     The value must use the following format 'regex1|regex2|regex3'.
#
#   HEADER_SYNC_SOURCE_DIRECTORY
#     The source directory for header sync procedure. Header files outside this directory will be
#     ignored by syncqt. The specifying this directory allows to skip the parsing of the whole
#     CMAKE_CURRENT_SOURCE_DIR for the header files that needs to be synced and only parse the
#     single subdirectory, that meanwhile can be outside the CMAKE_CURRENT_SOURCE_DIR tree.
#
#   FIND_PRIVATE_MODULE
#     A call to find_package(Qt6Foo) will imply a call to find_package(Qt6FooPrivate).
function(qt_internal_add_module target)
    qt_internal_get_internal_add_module_keywords(
        module_option_args
        module_single_args
        module_multi_args
    )

    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${module_option_args}"
        "${module_single_args}"
        "${module_multi_args}"
    )
    _qt_internal_validate_all_args_are_parsed(arg)

    set(is_internal_module FALSE)
    if(arg_INTERNAL_MODULE)
        set(is_internal_module TRUE)
        set(arg_INTERNAL_MODULE "INTERNAL_MODULE")
        set(arg_NO_PRIVATE_MODULE TRUE)
        # Assume the interface name of the internal module should be the module name without the
        # 'Private' suffix.
        if(NOT arg_MODULE_INTERFACE_NAME)
            if(target MATCHES "(.*)Private$")
                set(arg_MODULE_INTERFACE_NAME "${CMAKE_MATCH_1}")
            else()
                message(WARNING "The internal module target should end with the 'Private' suffix.")
            endif()
        endif()
    else()
        unset(arg_INTERNAL_MODULE)
    endif()

    if(NOT arg_MODULE_INTERFACE_NAME)
        set(arg_MODULE_INTERFACE_NAME "${target}")
    endif()

    ### Define Targets:
    if(arg_HEADER_MODULE)
        set(type_to_create INTERFACE)
    elseif(arg_STATIC)
        set(type_to_create STATIC)
    else()
        # Use default depending on Qt configuration.
        set(type_to_create "")
    endif()

    _qt_internal_add_library("${target}" ${type_to_create})
    qt_internal_mark_as_internal_library("${target}")

    get_target_property(target_type ${target} TYPE)

    set(is_interface_lib 0)
    set(is_shared_lib 0)
    set(is_static_lib 0)
    if(target_type STREQUAL "INTERFACE_LIBRARY")
        set(is_interface_lib 1)
    elseif(target_type STREQUAL "STATIC_LIBRARY")
        set(is_static_lib 1)
    elseif(target_type STREQUAL "SHARED_LIBRARY")
        set(is_shared_lib 1)
    else()
        message(FATAL_ERROR "Invalid target type '${target_type}' for Qt module '${target}'")
    endif()

    if(NOT arg_NO_SYNC_QT AND NOT arg_NO_MODULE_HEADERS AND arg_MODULE_INCLUDE_NAME)
        # qt_internal_module_info uses this property if it's set, so it must be
        # specified before the qt_internal_module_info call.
        set_target_properties(${target} PROPERTIES
            _qt_module_include_name ${arg_MODULE_INCLUDE_NAME}
        )
    endif()

    set_target_properties(${target} PROPERTIES
        _qt_module_interface_name "${arg_MODULE_INTERFACE_NAME}"
        _qt_package_version "${PROJECT_VERSION}"
        _qt_package_name "${INSTALL_CMAKE_NAMESPACE}${target}"
    )
    set(export_properties
        "_qt_module_interface_name"
        "_qt_package_version"
        "_qt_package_name"
    )

    if(CMAKE_VERSION VERSION_LESS 3.30)
        # For the CMake versions higher than 3.30 the corresponding INTERFACE_
        # properties will be in Qt<Module>Targets.cmake without extra code
        # needed.
        list(APPEND export_properties
            "_qt_transitive_compile_properties"
            "_qt_transitive_link_properties"
        )
    endif()

    if(NOT is_internal_module)
        set_target_properties(${target} PROPERTIES
            _qt_is_public_module TRUE
        )
        list(APPEND export_properties
            "_qt_is_public_module"
        )
        if(NOT ${arg_NO_PRIVATE_MODULE})
            set_target_properties(${target} PROPERTIES
                _qt_private_module_target_name "${target}Private"
            )
            list(APPEND export_properties
                "_qt_private_module_target_name"
            )
        endif()
    endif()

    set_property(TARGET ${target} APPEND PROPERTY EXPORT_PROPERTIES "${export_properties}")

    qt_internal_module_info(module "${target}")
    qt_internal_add_qt_repo_known_module("${target}")
    if(arg_INTERNAL_MODULE)
        set_target_properties(${target} PROPERTIES _qt_is_internal_module TRUE)
        set_property(TARGET ${target} APPEND PROPERTY EXPORT_PROPERTIES _qt_is_internal_module)
    endif()
    if(arg_HEADER_MODULE)
        set_target_properties(${target} PROPERTIES _qt_is_header_module TRUE)
        set_property(TARGET ${target} APPEND PROPERTY EXPORT_PROPERTIES _qt_is_header_module)
    endif()

    # Record whether the target will be installed. We don't have to export this property.
    set_target_properties(${target} PROPERTIES _qt_will_install "${QT_WILL_INSTALL}")

    if(NOT arg_CONFIG_MODULE_NAME)
        set(arg_CONFIG_MODULE_NAME "${module_lower}")
    endif()

    set(module_config_header "qt${arg_CONFIG_MODULE_NAME}-config.h")
    set(module_config_private_header "qt${arg_CONFIG_MODULE_NAME}-config_p.h")
    # qt<module>-config.h/-config_p.h header files are not marked as GENERATED automatically
    # for old CMake versions. Set the property explicitly here.
    _qt_internal_set_source_file_generated(
        SOURCES "${module_config_header}" "${module_config_private_header}"
        SKIP_AUTOGEN
    )

    # Module define needs to take into account the config module name.
    string(TOUPPER "${arg_CONFIG_MODULE_NAME}" module_define_infix)
    string(REPLACE "-" "_" module_define_infix "${module_define_infix}")
    string(REPLACE "." "_" module_define_infix "${module_define_infix}")

    set(property_prefix "INTERFACE_")
    if(NOT arg_HEADER_MODULE)
        set(property_prefix "")
    endif()

    if(arg_INTERNAL_MODULE)
        string(APPEND arg_CONFIG_MODULE_NAME "_private")
    endif()
    set_target_properties(${target} PROPERTIES
        _qt_config_module_name "${arg_CONFIG_MODULE_NAME}"
        ${property_prefix}QT_QMAKE_MODULE_CONFIG "${arg_QMAKE_MODULE_CONFIG}")
    set_property(TARGET "${target}" APPEND PROPERTY EXPORT_PROPERTIES _qt_config_module_name)

    set(is_framework 0)
    if(QT_FEATURE_framework AND NOT ${arg_HEADER_MODULE} AND NOT ${arg_STATIC})
        set(is_framework 1)
        set_target_properties(${target} PROPERTIES
            FRAMEWORK TRUE
            FRAMEWORK_VERSION "A" # Not based on Qt major version
            MACOSX_FRAMEWORK_IDENTIFIER org.qt-project.${module}
            MACOSX_FRAMEWORK_BUNDLE_VERSION ${PROJECT_VERSION}
            MACOSX_FRAMEWORK_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        )
        qt_internal_get_framework_info(fw ${target})
    endif()

    if(QT_FEATURE_reduce_relocations AND NOT is_interface_lib)
        if(QT_FEATURE_no_direct_extern_access)
            # Modern support, we don't need to add workarounds
            if(is_shared_lib)
                target_link_options(${target} PRIVATE LINKER:-Bsymbolic)
            endif()
        else()
            if(is_shared_lib)
                target_link_options(${target} PRIVATE LINKER:-Bsymbolic-functions)
            endif()

            # Without FEATURE_no_direct_extern_access, applications cannot use
            # -fPIE any more and must use -fPIC. Even then, this may fail.
            # Consider upgrading.
            #
            # When a Swift library links to a Qt module, it fails to link because cmake passes
            # -fPIC to it, and the swiftc compiler doesn't know that option.
            # Prevent passing it.
            set(fpic_flag "$<$<NOT:$<COMPILE_LANGUAGE:Swift>>:-fPIC>")
            target_compile_options(${target} INTERFACE "${fpic_flag}")
        endif()
    endif()

    if((FEATURE_ltcg OR CMAKE_INTERPROCEDURAL_OPTIMIZATION) AND GCC AND is_static_lib)
        # CMake <= 3.19 appends -fno-fat-lto-objects for all library types if
        # CMAKE_INTERPROCEDURAL_OPTIMIZATION is enabled. Static libraries need
        # the opposite compiler option.
        # (https://gitlab.kitware.com/cmake/cmake/-/issues/21696)
        target_compile_options(${target} PRIVATE -ffat-lto-objects)
    endif()

    qt_internal_add_target_aliases("${target}")

    qt_internal_default_warnings_are_errors("${target}")

    # No need to compile Q_IMPORT_PLUGIN-containing files for non-executables.
    if(is_static_lib)
        _qt_internal_disable_static_default_plugins("${target}")
    endif()

    # Add _private target to link against the private headers:
    set(target_private "${target}Private")
    if(NOT ${arg_NO_PRIVATE_MODULE})
        add_library("${target_private}" INTERFACE)
        qt_internal_add_target_aliases("${target_private}")
        set_target_properties(${target_private} PROPERTIES
            _qt_config_module_name ${arg_CONFIG_MODULE_NAME}_private
            _qt_package_version "${PROJECT_VERSION}"
            _qt_package_name "${INSTALL_CMAKE_NAMESPACE}${target}Private"
            _qt_is_private_module TRUE
            _qt_public_module_target_name "${target}"
        )
        set(export_properties
            "_qt_config_module_name"
            "_qt_package_version"
            "_qt_package_name"
            "_qt_is_private_module"
            "_qt_public_module_target_name"
        )
        set_property(TARGET "${target_private}" APPEND PROPERTY
                     EXPORT_PROPERTIES "${export_properties}")

        # Record whether the target will be installed. We don't have to export this property.
        set_target_properties(${target_private} PROPERTIES _qt_will_install "${QT_WILL_INSTALL}")

        # Let find_package(Qt6FooPrivate) also find_package(Qt6Foo).
        qt_internal_register_target_dependencies("${target_private}" PUBLIC "Qt::${target}")
    endif()

    # FIXME: This workaround is needed because the deployment logic
    # for iOS and WASM just copies/embeds the directly linked library,
    # which will just be a versioned symlink to the actual library.
    if((UIKIT OR WASM) AND BUILD_SHARED_LIBS)
        set(version_args "")
    else()
        set(version_args
            VERSION ${PROJECT_VERSION}
            SOVERSION ${PROJECT_VERSION_MAJOR})
    endif()

    if(NOT arg_HEADER_MODULE)
        set_target_properties(${target} PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY "${QT_BUILD_DIR}/${INSTALL_LIBDIR}"
            RUNTIME_OUTPUT_DIRECTORY "${QT_BUILD_DIR}/${INSTALL_BINDIR}"
            ARCHIVE_OUTPUT_DIRECTORY "${QT_BUILD_DIR}/${INSTALL_LIBDIR}"
            ${version_args}
        )
        qt_set_target_info_properties(${target} ${ARGN})
        qt_handle_multi_config_output_dirs("${target}")

        if(NOT BUILD_SHARED_LIBS AND (LINUX OR VXWORKS))
            # Horrible workaround for static build failures due to incorrect static library link
            # order. By increasing the multiplicity to 3, each library cycle will be repeated
            # 3 times on the link line, reducing the probability of undefined symbols at
            # link time.
            # These failures are only observed on Linux with the ld linker (not sure about
            # ld.gold).
            # Allow opting out and modifying the value via cache value,  in case if we urgently
            # need to increase it without waiting for the qtbase change to propagate to
            # other dependent repos.
            # The proper fix will be to get rid of the cycles in the future.
            # See QTBUG-83498 for details.
            set(default_link_cycle_multiplicity "3")
            if(DEFINED QT_LINK_CYCLE_MULTIPLICITY)
                set(default_link_cycle_multiplicity "${QT_LINK_CYCLE_MULTIPLICITY}")
            endif()
            if(default_link_cycle_multiplicity)
                set_property(TARGET "${target}"
                             PROPERTY
                             LINK_INTERFACE_MULTIPLICITY "${default_link_cycle_multiplicity}")
            endif()
        endif()

        if (arg_SKIP_DEPENDS_INCLUDE)
            set_target_properties(${target} PROPERTIES _qt_module_skip_depends_include TRUE)
            set_property(TARGET "${target}" APPEND PROPERTY
                         EXPORT_PROPERTIES _qt_module_skip_depends_include)
        endif()
        if(is_framework)
            set_target_properties(${target} PROPERTIES
                OUTPUT_NAME ${fw_name}
            )
        else()
            set_target_properties(${target} PROPERTIES
                OUTPUT_NAME "${INSTALL_CMAKE_NAMESPACE}${module_interface_name}${QT_LIBINFIX}"
            )
        endif()

        qt_set_common_target_properties(${target})

        if (WIN32 AND BUILD_SHARED_LIBS)
            _qt_internal_generate_win32_rc_file(${target})
        endif()
    endif()

    # Module headers:
    set_property(TARGET "${target}" APPEND PROPERTY EXPORT_PROPERTIES _qt_module_has_headers)
    if(${arg_NO_MODULE_HEADERS} OR ${arg_NO_SYNC_QT})
        set_target_properties("${target}" PROPERTIES
            _qt_module_has_headers OFF)
    else()
        set_property(TARGET ${target} APPEND PROPERTY EXPORT_PROPERTIES _qt_module_include_name)
        set_target_properties("${target}" PROPERTIES
            _qt_module_include_name "${module_include_name}"
            _qt_module_has_headers ON
        )

        set(sync_source_directory "${CMAKE_CURRENT_SOURCE_DIR}")
        if(arg_HEADER_SYNC_SOURCE_DIRECTORY)
            set(sync_source_directory "${arg_HEADER_SYNC_SOURCE_DIRECTORY}")
        endif()
        set_target_properties(${target} PROPERTIES
            _qt_sync_source_directory "${sync_source_directory}")
        # We should not generate export headers if module is defined as pure STATIC.
        # Static libraries don't need to export their symbols, and corner cases when sources are
        # also used in shared libraries, should be handled manually.
        if(NOT arg_NO_GENERATE_CPP_EXPORTS AND NOT arg_STATIC AND NOT arg_HEADER_MODULE)
            if(arg_CPP_EXPORT_HEADER_BASE_NAME)
                set(cpp_export_header_base_name
                    "CPP_EXPORT_HEADER_BASE_NAME;${arg_CPP_EXPORT_HEADER_BASE_NAME}"
                )
            endif()
            qt_internal_generate_cpp_global_exports(${target} ${module_define_infix}
                "${cpp_export_header_base_name}"
            )
        endif()

        set(module_depends_header
            "${module_build_interface_include_dir}/${module_include_name}Depends")
        _qt_internal_set_source_file_generated(SOURCES "${module_depends_header}")
        set_target_properties(${target} PROPERTIES _qt_module_depends_header
            "${module_depends_header}")
        if(NOT arg_HEADER_MODULE)
            set(module_header "${module_build_interface_include_dir}/${module_include_name}")
            set_property(TARGET "${target}" PROPERTY MODULE_HEADER
                    "${module_header}")

            if(QT_FEATURE_no_prefix)
                file(RELATIVE_PATH relative_include_dir
                    "/${QT_CONFIG_INSTALL_DIR}/${module_include_name}"
                    "/${module_build_interface_include_dir}"
                )
            else()
                file(RELATIVE_PATH relative_include_dir
                    "/${QT_CONFIG_INSTALL_DIR}/${module_include_name}"
                    "/${module_install_interface_include_dir}"
                )
            endif()
            set_property(TARGET "${target}" PROPERTY
                _qt_module_relative_include_dir "${relative_include_dir}"
            )
            set_property(TARGET ${target} APPEND PROPERTY
                EXPORT_PROPERTIES _qt_module_relative_include_dir
            )
        endif()

        set(qpa_filter_regex "")
        if(arg_QPA_HEADER_FILTERS)
            set(qpa_filter_regex "${arg_QPA_HEADER_FILTERS}")
        endif()
        set_target_properties(${target}
            PROPERTIES _qt_module_qpa_headers_filter_regex "${qpa_filter_regex}")

        set(rhi_filter_regex "")
        if(arg_RHI_HEADER_FILTERS)
            set(rhi_filter_regex "${arg_RHI_HEADER_FILTERS}")
        endif()
        set_target_properties(${target}
            PROPERTIES _qt_module_rhi_headers_filter_regex "${rhi_filter_regex}")

        set(ssg_filter_regex "")
        if(arg_SSG_HEADER_FILTERS)
            set(ssg_filter_regex "${arg_SSG_HEADER_FILTERS}")
        endif()
        set_target_properties(${target}
            PROPERTIES _qt_module_ssg_headers_filter_regex "${ssg_filter_regex}")

        set(private_filter_regex ".+_p(ch)?\\.h")
        if(arg_PRIVATE_HEADER_FILTERS)
            set(private_filter_regex "${private_filter_regex}|${arg_PRIVATE_HEADER_FILTERS}")
        endif()
        set_target_properties(${target}
            PROPERTIES _qt_module_private_headers_filter_regex "${private_filter_regex}")

        # If EXTERNAL_HEADERS_DIR is set we install the specified directory and keep the structure
        # without taking into the account the CMake source tree and syncqt outputs.
        if(arg_EXTERNAL_HEADERS_DIR)
            set_property(TARGET ${target}
                PROPERTY _qt_external_headers_dir "${arg_EXTERNAL_HEADERS_DIR}")
            qt_install(DIRECTORY "${arg_EXTERNAL_HEADERS_DIR}/"
                DESTINATION "${module_install_interface_include_dir}"
            )
        endif()
    endif()

    if(arg_NO_HEADERSCLEAN_CHECK OR arg_NO_MODULE_HEADERS OR arg_NO_SYNC_QT
        OR NOT INPUT_headersclean)
        set_target_properties("${target}" PROPERTIES _qt_no_headersclean_check ON)
    endif()

    if(NOT arg_HEADER_MODULE)
        # Plugin types associated to a module
        if(NOT "x${arg_PLUGIN_TYPES}" STREQUAL "x")
            qt_internal_add_plugin_types("${target}" "${arg_PLUGIN_TYPES}")
            # Ensure that QT_PLUGIN_TARGETS is a known transitive compile property. Works with CMake
            # versions >= 3.30.
            _qt_internal_add_transitive_property(${target} COMPILE QT_PLUGIN_TARGETS)
        endif()
    endif()

    if(NOT arg_HEADER_MODULE)
        qt_autogen_tools_initial_setup(${target})
    endif()

    set(private_includes
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
         ${arg_INCLUDE_DIRECTORIES}
    )

    set(public_includes "")
    set(public_headers_list "public_includes")
    if(is_framework)
        set(public_headers_list "private_includes")
    endif()

    # Make sure the BUILD_INTERFACE include paths come before the framework headers, so that the
    # the compiler prefers the build dir includes.
    #
    # Make sure to add non-framework "build_dir/include" as an include path for moc to find the
    # currently built module headers. qmake does this too.
    # Framework-style include paths are found by moc when cmQtAutoMocUic.cxx detects frameworks by
    # looking at an include path and detecting a "QtFoo.framework/Headers" path.
    # Make sure to create such paths for both the the BUILD_INTERFACE and the INSTALL_INTERFACE.
    #
    # Only add syncqt headers if they exist.
    # This handles cases like QmlDevToolsPrivate which do not have their own headers, but borrow
    # them from another module.
    if(NOT arg_NO_SYNC_QT AND NOT arg_NO_MODULE_HEADERS)
        # Don't include private headers unless they exist, aka syncqt created them.
        qt_internal_append_include_directories_with_headers_check(${target}
            private_includes PRIVATE
            "$<BUILD_INTERFACE:${module_build_interface_versioned_include_dir}>"
            "$<BUILD_INTERFACE:${module_build_interface_versioned_inner_include_dir}>"
        )

        list(APPEND public_includes
                    # For the syncqt headers
                    "$<BUILD_INTERFACE:${repo_build_interface_include_dir}>"
                    "$<BUILD_INTERFACE:${module_build_interface_include_dir}>")
    endif()

    if(is_framework)
        set(fw_install_dir "${INSTALL_LIBDIR}/${fw_dir}")
        set(fw_install_header_dir "${INSTALL_LIBDIR}/${fw_header_dir}")
        set(fw_output_header_dir "${QT_BUILD_DIR}/${fw_install_header_dir}")
        list(APPEND public_includes
            # Add the framework Headers subdir, so that non-framework-style includes work. The
            # BUILD_INTERFACE Headers symlink was previously claimed not to exist at the relevant
            # time, and a fully specified Header path was used instead. This doesn't seem to be a
            # problem anymore.
            "$<BUILD_INTERFACE:${fw_output_header_dir}>"
            "$<INSTALL_INTERFACE:${fw_install_header_dir}>"

            # Add the lib/Foo.framework dir as an include path to let CMake generate
            # the -F compiler flag for framework-style includes to work.
            # Make sure it is added AFTER the lib/Foo.framework/Headers include path,
            # to mitigate issues like QTBUG-101718 and QTBUG-101775 where an include like
            # #include <QtCore> might cause moc to include the QtCore framework shared library
            # instead of the actual header.
            "$<INSTALL_INTERFACE:${fw_install_dir}>"
        )
    endif()

    if(NOT arg_NO_MODULE_HEADERS AND NOT arg_NO_SYNC_QT)
        # For the syncqt headers
        list(APPEND ${public_headers_list}
            "$<INSTALL_INTERFACE:${module_install_interface_include_dir}>")

        # To support finding Qt module includes that are not installed into the main Qt prefix.
        # Use case: A Qt module built by Conan installed into a prefix other than the main prefix.
        # This does duplicate the include path set on Qt6::Platform target, but CMake is smart
        # enough to deduplicate the include paths on the command line.
        # Frameworks are automatically handled by CMake in cmLocalGenerator::GetIncludeFlags()
        # by additionally passing the 'QtFoo.framework/..' dir with an -iframework argument.
        list(APPEND ${public_headers_list} "$<INSTALL_INTERFACE:${INSTALL_INCLUDEDIR}>")
    endif()
    list(APPEND ${public_headers_list} ${arg_PUBLIC_INCLUDE_DIRECTORIES})

    set(defines_for_extend_target "")

    if(NOT arg_HEADER_MODULE)
        list(APPEND defines_for_extend_target
            QT_NO_CAST_TO_ASCII QT_ASCII_CAST_WARNINGS
            QT_MOC_COMPAT #we don't need warnings from calling moc code in our generated code
            QT_DEPRECATED_WARNINGS
            QT_BUILDING_QT
            QT_BUILD_${module_define_infix}_LIB ### FIXME: use QT_BUILD_ADDON for Add-ons or remove if we don't have add-ons anymore
            ${deprecation_define}
            )
        list(APPEND arg_LIBRARIES Qt::PlatformModuleInternal)
    endif()

    qt_internal_add_repo_local_defines("${target}")

    if(arg_NO_UNITY_BUILD)
        set(arg_NO_UNITY_BUILD "NO_UNITY_BUILD")
    else()
        set(arg_NO_UNITY_BUILD "")
    endif()

    if(NOT arg_EXTERNAL_HEADERS)
        set(arg_EXTERNAL_HEADERS "")
    endif()

    _qt_internal_forward_function_args(
        FORWARD_PREFIX arg
        FORWARD_OUT_VAR extend_target_args
        FORWARD_SINGLE
            ELF_LINKER_DYNAMIC_LIST
            PRECOMPILED_HEADER
    )

    # Put this behind a scoped var for now, to allow disabling it per repo.
    if(NOT DEFINED QT_WARN_PUBLIC_MODULE_LINKS_PRIVATE_MODULES_PUBLICLY)
        set(QT_WARN_PUBLIC_MODULE_LINKS_PRIVATE_MODULES_PUBLICLY ON)
    endif()
    if(QT_WARN_PUBLIC_MODULE_LINKS_PRIVATE_MODULES_PUBLICLY
            AND QT_FEATURE_developer_build
            AND NOT is_internal_module
            AND NOT "${target}" MATCHES "Private$"
        )
        foreach(public_lib IN LISTS arg_PUBLIC_LIBRARIES)
            if("${public_lib}" MATCHES "^(Qt|${QT_CMAKE_EXPORT_NAMESPACE})::(.*)Private$")
                message(AUTHOR_WARNING
                    "${target} specfies ${public_lib} in its PUBLIC_LIBRARIES option. "
                    "Public modules should not link publicly to Private modules, because that "
                    "exposes private module headers to user projects without them opting into it. "
                    "Update the project to use the LIBRARIES keyword instead."
                )
            endif()
        endforeach()
    endif()

    qt_internal_extend_target("${target}"
        ${arg_NO_UNITY_BUILD}
        SOURCES
            ${arg_SOURCES}
            ${arg_EXTERNAL_HEADERS}
        NO_UNITY_BUILD_SOURCES
            ${arg_NO_UNITY_BUILD_SOURCES}
        INCLUDE_DIRECTORIES
            ${private_includes}
        SYSTEM_INCLUDE_DIRECTORIES
            ${arg_SYSTEM_INCLUDE_DIRECTORIES}
        PUBLIC_INCLUDE_DIRECTORIES
            ${public_includes}
        PUBLIC_DEFINES
            ${arg_PUBLIC_DEFINES}
        DEFINES
            ${arg_DEFINES}
            ${defines_for_extend_target}
        PUBLIC_LIBRARIES ${arg_PUBLIC_LIBRARIES}
        LIBRARIES ${arg_LIBRARIES}
        PRIVATE_MODULE_INTERFACE ${arg_PRIVATE_MODULE_INTERFACE}
        DBUS_ADAPTOR_SOURCES ${arg_DBUS_ADAPTOR_SOURCES}
        DBUS_ADAPTOR_FLAGS ${arg_DBUS_ADAPTOR_FLAGS}
        DBUS_INTERFACE_SOURCES ${arg_DBUS_INTERFACE_SOURCES}
        DBUS_INTERFACE_FLAGS ${arg_DBUS_INTERFACE_FLAGS}
        COMPILE_OPTIONS ${arg_COMPILE_OPTIONS}
        PUBLIC_COMPILE_OPTIONS ${arg_PUBLIC_COMPILE_OPTIONS}
        LINK_OPTIONS ${arg_LINK_OPTIONS}
        PUBLIC_LINK_OPTIONS ${arg_PUBLIC_LINK_OPTIONS}
        MOC_OPTIONS ${arg_MOC_OPTIONS}
        ENABLE_AUTOGEN_TOOLS ${arg_ENABLE_AUTOGEN_TOOLS}
        DISABLE_AUTOGEN_TOOLS ${arg_DISABLE_AUTOGEN_TOOLS}
        NO_PCH_SOURCES ${arg_NO_PCH_SOURCES}
        ${extend_target_args}
    )

    # The public module define is not meant to be used when building the module itself,
    # it's only meant to be used for consumers of the module,
    # thus we can't use qt_internal_extend_target()'s PUBLIC_DEFINES option.
    target_compile_definitions(${target} INTERFACE QT_${module_define_infix}_LIB)

    if(NOT arg_EXCEPTIONS)
        qt_internal_set_exceptions_flags("${target}" "DEFAULT")
    else()
        qt_internal_set_exceptions_flags("${target}" "${arg_EXCEPTIONS}")
    endif()

    set(configureFile "${CMAKE_CURRENT_SOURCE_DIR}/configure.cmake")
    if(arg_CONFIGURE_FILE_PATH)
        set(configureFile "${arg_CONFIGURE_FILE_PATH}")
    endif()
    if(EXISTS "${configureFile}" AND NOT arg_NO_CONFIG_HEADER_FILE)
        qt_feature_module_begin(
            LIBRARY "${target}"
            PUBLIC_FILE "${module_config_header}"
            PRIVATE_FILE "${module_config_private_header}"
        )
        include(${configureFile})
        qt_feature_module_end("${target}")

        qt_internal_extend_target("${target}"
            SOURCES
                "${CMAKE_CURRENT_BINARY_DIR}/${module_config_header}"
                "${CMAKE_CURRENT_BINARY_DIR}/${module_config_private_header}"
        )
    endif()

    # Handle creation of cmake files for consumers of find_package().
    set(path_suffix "${INSTALL_CMAKE_NAMESPACE}${target}")
    qt_path_join(config_build_dir ${QT_CONFIG_BUILD_DIR} ${path_suffix})
    qt_path_join(config_install_dir ${QT_CONFIG_INSTALL_DIR} ${path_suffix})
    if(NOT arg_NO_PRIVATE_MODULE)
        set(path_suffix "${INSTALL_CMAKE_NAMESPACE}${target_private}")
        qt_path_join(private_config_build_dir ${QT_CONFIG_BUILD_DIR} ${path_suffix})
        qt_path_join(private_config_install_dir ${QT_CONFIG_INSTALL_DIR} ${path_suffix})
    endif()
    unset(path_suffix)

    set(extra_cmake_files)
    set(extra_cmake_includes)
    if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/${INSTALL_CMAKE_NAMESPACE}${target}Macros.cmake")
        list(APPEND extra_cmake_files "${CMAKE_CURRENT_LIST_DIR}/${INSTALL_CMAKE_NAMESPACE}${target}Macros.cmake")
        list(APPEND extra_cmake_includes "${INSTALL_CMAKE_NAMESPACE}${target}Macros.cmake")
    endif()

    if(CMAKE_VERSION VERSION_LESS 3.30)
        # For the CMake versions higher than 3.30 the corresponding INTERFACE_
        # properties will be in Qt<Module>Targets.cmake without extra code
        # needed.
        configure_file(
            "${QT_CMAKE_DIR}/QtTransitiveExtras.cmake.in"
            "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}TransitiveExtras.cmake"
            @ONLY
        )
        list(APPEND extra_cmake_files
            "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}TransitiveExtras.cmake")
        list(APPEND extra_cmake_includes
            "${INSTALL_CMAKE_NAMESPACE}${target}TransitiveExtras.cmake")
    endif()

    if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/${INSTALL_CMAKE_NAMESPACE}${target}ConfigExtras.cmake.in")
        if(target STREQUAL Core)
            if(NOT "${QT_NAMESPACE}" STREQUAL "")
                string(MAKE_C_IDENTIFIER "${QT_NAMESPACE}" qt_namespace_sanity)
                if(NOT "${QT_NAMESPACE}" STREQUAL "${qt_namespace_sanity}")
                    message(FATAL_ERROR "QT_NAMESPACE is not a valid C++ identifier: "
                        "${QT_NAMESPACE}.")
                endif()
                string(JOIN "" qtcore_namespace_definition
                    "set_property(TARGET \${__qt_core_target} "
                    "APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS QT_NAMESPACE=${QT_NAMESPACE})"
                )
            endif()

            set(extra_cmake_code "")
            # Add some variables for compatibility with Qt5 config files.
            if(QT_FEATURE_reduce_exports)
                string(APPEND qtcore_extra_cmake_code "
set(QT_VISIBILITY_AVAILABLE TRUE)")
            endif()
            if(QT_LIBINFIX)
                string(APPEND qtcore_extra_cmake_code "
set(QT_LIBINFIX \"${QT_LIBINFIX}\")")
            endif()

            # Store whether find_package(Qt6Foo) should succeed if Qt6FooTools is missing.
            if(QT_ALLOW_MISSING_TOOLS_PACKAGES)
                string(APPEND qtcore_extra_cmake_code "
set(QT_ALLOW_MISSING_TOOLS_PACKAGES TRUE)")
            endif()
        endif()

        configure_file("${CMAKE_CURRENT_LIST_DIR}/${INSTALL_CMAKE_NAMESPACE}${target}ConfigExtras.cmake.in"
            "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}ConfigExtras.cmake"
            @ONLY)
        list(APPEND extra_cmake_files "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}ConfigExtras.cmake")
        list(APPEND extra_cmake_includes "${INSTALL_CMAKE_NAMESPACE}${target}ConfigExtras.cmake")
    endif()

    foreach(cmake_file IN LISTS arg_EXTRA_CMAKE_FILES)
        get_source_file_property(install_path ${cmake_file} QT_INSTALL_PATH)
        if(NOT install_path)
            # Sanitize the install_path from `NOTFOUND` to ""
            set(install_path "")
        endif()
        file(COPY ${cmake_file} DESTINATION "${config_build_dir}/${install_path}")
        qt_install(FILES
            ${cmake_file}
            DESTINATION "${config_install_dir}/${install_path}"
            COMPONENT Devel
        )

        # Make sure touched extra cmake files cause a reconfigure, so they get re-copied.
        _qt_internal_append_cmake_configure_depends("${cmake_file}")
    endforeach()
    list(APPEND extra_cmake_includes ${arg_EXTRA_CMAKE_INCLUDES})

    set(extra_cmake_code "")

    if(arg_POLICIES)
        set(policies "")
        foreach(policy IN LISTS arg_POLICIES)
            list(APPEND policies "set(QT_KNOWN_POLICY_${policy} TRUE)")

            # When building Qt, tests and examples might expect a policy to be known, but they
            # won't be known depending on which scope or when a find_package(Module) with the
            # respective policy is called. Check the global list of known policies to accommodate
            # that.
            set_property(GLOBAL APPEND PROPERTY _qt_global_known_policies "${policy}")
        endforeach()
        list(JOIN policies "\n" policies_str)
        string(APPEND extra_cmake_code "${policies_str}\n")
    endif()

    # Generate metatypes
    if (NOT ${arg_NO_GENERATE_METATYPES} AND NOT target_type STREQUAL "INTERFACE_LIBRARY")
        set(args "")
        if(QT_WILL_INSTALL)
            set(metatypes_install_dir "${INSTALL_ARCHDATADIR}/metatypes")
            if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.31")
                cmake_path(SET metatypes_install_dir NORMALIZE "${metatypes_install_dir}")
            endif()
            list(APPEND args
                __QT_INTERNAL_INSTALL __QT_INTERNAL_INSTALL_DIR "${metatypes_install_dir}")
        endif()
        qt6_extract_metatypes(${target} ${args})
    endif()

    qt_internal_get_min_new_policy_cmake_version(min_new_policy_version)
    qt_internal_get_max_new_policy_cmake_version(max_new_policy_version)

    if(is_static_lib)
        set(write_basic_module_package_args IS_STATIC_LIB)
    else()
        set(write_basic_module_package_args "")
    endif()

    if(arg_FIND_PRIVATE_MODULE)
        set(write_basic_public_module_package_args FIND_PRIVATE_MODULE)
    else()
        set(write_basic_public_module_package_args "")
    endif()

    qt_internal_write_basic_module_package("${target}" "${target_private}"
        ${write_basic_module_package_args}
        ${write_basic_public_module_package_args}
        CONFIG_BUILD_DIR ${config_build_dir}
        CONFIG_INSTALL_DIR ${config_install_dir}
    )

    if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/${INSTALL_CMAKE_NAMESPACE}${target}BuildInternals.cmake")
        configure_file("${CMAKE_CURRENT_LIST_DIR}/${INSTALL_CMAKE_NAMESPACE}${target}BuildInternals.cmake"
            "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}BuildInternals.cmake"
            @ONLY)
        list(APPEND extra_cmake_files "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}BuildInternals.cmake")
    endif()

    qt_install(FILES
        ${extra_cmake_files}
        DESTINATION "${config_install_dir}"
        COMPONENT Devel
    )

    if(NOT arg_NO_PRIVATE_MODULE)
        qt_internal_write_basic_module_package("${target}" "${target_private}"
            ${write_basic_module_package_args}
            PRIVATE
            CONFIG_BUILD_DIR ${private_config_build_dir}
            CONFIG_INSTALL_DIR ${private_config_install_dir}
        )
    endif()

    file(COPY ${extra_cmake_files} DESTINATION "${config_build_dir}")

    _qt_internal_forward_function_args(
        FORWARD_PREFIX arg
        FORWARD_OUT_VAR export_module_args
        FORWARD_OPTIONS NO_ADDITIONAL_TARGET_INFO
    )
    qt_internal_export_module(${target}
        ${export_module_args}
        CONFIG_BUILD_DIR ${config_build_dir}
        CONFIG_INSTALL_DIR ${config_install_dir}
    )
    if(NOT arg_NO_PRIVATE_MODULE)
        qt_internal_export_module(${target_private}
            ${export_module_args}
            CONFIG_BUILD_DIR ${private_config_build_dir}
            CONFIG_INSTALL_DIR ${private_config_install_dir}
        )
    endif()

    if(BUILD_SHARED_LIBS)
        qt_apply_rpaths(TARGET "${target}" INSTALL_PATH "${INSTALL_LIBDIR}" RELATIVE_RPATH)
        qt_internal_apply_staging_prefix_build_rpath_workaround()
    endif()

    if (ANDROID AND NOT arg_HEADER_MODULE)
        # Record install library location so it can be accessed by
        # qt_internal_android_dependencies without having to specify it again.
        set_target_properties(${target} PROPERTIES
            QT_ANDROID_MODULE_INSTALL_DIR ${INSTALL_LIBDIR})
    endif()

    ### fixme: cmake is missing a built-in variable for this. We want to apply it only to modules and plugins
    # that belong to Qt.
    if(NOT arg_HEADER_MODULE)
        qt_internal_add_link_flags_no_undefined("${target}")
    endif()

    set(interface_includes "")

    # Handle cases like QmlDevToolsPrivate which do not have their own headers, but rather borrow them
    # from another module.
    if(NOT arg_NO_SYNC_QT AND NOT arg_NO_MODULE_HEADERS)
        list(APPEND interface_includes "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>")

        # syncqt does not create a private header directory like 'include/6.0/QtFoo' unless
        # the module has foo_p.h header files. For QtZlib, there are no such private headers, so we
        # need to make sure not to add such include paths unless the directory exists, otherwise
        # consumers of the module will fail at CMake generation time stating that
        # INTERFACE_INCLUDE_DIRECTORIES contains a non-existent path.
        qt_internal_append_include_directories_with_headers_check(${target}
            interface_includes PRIVATE
            "$<BUILD_INTERFACE:${module_build_interface_versioned_include_dir}>"
            "$<BUILD_INTERFACE:${module_build_interface_versioned_inner_include_dir}>"
        )

        if(is_framework)
            set(fw_install_private_header_dir "${INSTALL_LIBDIR}/${fw_private_header_dir}")
            set(fw_install_private_module_header_dir
                "${INSTALL_LIBDIR}/${fw_private_module_header_dir}")
            qt_internal_append_include_directories_with_headers_check(${target}
                interface_includes PRIVATE
                "$<INSTALL_INTERFACE:${fw_install_private_header_dir}>"
                "$<INSTALL_INTERFACE:${fw_install_private_module_header_dir}>"
            )
        else()
            qt_internal_append_include_directories_with_headers_check(${target}
                interface_includes PRIVATE
                "$<INSTALL_INTERFACE:${module_install_interface_versioned_include_dir}>"
                "$<INSTALL_INTERFACE:${module_install_interface_versioned_inner_include_dir}>"
            )
        endif()
    endif()

    if(arg_INTERNAL_MODULE)
        target_include_directories("${target}" INTERFACE ${interface_includes})
    elseif(NOT ${arg_NO_PRIVATE_MODULE})
        target_include_directories("${target_private}" INTERFACE ${interface_includes})
        target_link_libraries("${target_private}" INTERFACE "${target}")
    endif()

    set(debug_install_dir "${INSTALL_LIBDIR}")
    if (MINGW)
        set(debug_install_dir "${INSTALL_BINDIR}")
    endif()
    qt_enable_separate_debug_info(${target} "${debug_install_dir}")
    set(pdb_install_dir "${INSTALL_BINDIR}")
    if(NOT is_shared_lib)
        set(pdb_install_dir "${INSTALL_LIBDIR}")
    endif()
    qt_internal_install_pdb_files(${target} "${pdb_install_dir}")

    if (arg_NO_PRIVATE_MODULE)
        set(arg_NO_PRIVATE_MODULE "NO_PRIVATE_MODULE")
    else()
        unset(arg_NO_PRIVATE_MODULE)
    endif()

    if(NOT arg_NO_MODULE_JSON_FILE)
        qt_describe_module(${target})
    endif()

    if(QT_GENERATE_SBOM)
        set(sbom_args "")

        # 3rd party header modules should not be treated as Qt modules.
        if(arg_IS_QT_3RD_PARTY_HEADER_MODULE)
            list(APPEND sbom_args DEFAULT_SBOM_ENTITY_TYPE QT_THIRD_PARTY_MODULE)
        else()
            list(APPEND sbom_args DEFAULT_SBOM_ENTITY_TYPE QT_MODULE)
        endif()

        qt_get_cmake_configurations(configs)
        foreach(config IN LISTS configs)
            _qt_internal_sbom_append_multi_config_aware_single_arg_option(
                RUNTIME_PATH
                "${INSTALL_BINDIR}"
                "${config}"
                sbom_args
            )
            _qt_internal_sbom_append_multi_config_aware_single_arg_option(
                LIBRARY_PATH
                "${INSTALL_LIBDIR}"
                "${config}"
                sbom_args
            )
            _qt_internal_sbom_append_multi_config_aware_single_arg_option(
                ARCHIVE_PATH
                "${INSTALL_LIBDIR}"
                "${config}"
                sbom_args
            )
            _qt_internal_sbom_append_multi_config_aware_single_arg_option(
                FRAMEWORK_PATH
                "${INSTALL_LIBDIR}/${fw_versioned_binary_dir}"
                "${config}"
                sbom_args
            )
        endforeach()

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

        qt_internal_extend_qt_entity_sbom(${target} ${sbom_args})
    endif()

    if(arg_NO_PACKAGE_CONFIG_FILE)
        set_target_properties("${target}" PROPERTIES _qt_no_package_config_file TRUE)
    endif()

    if(arg_NO_QMAKE_SUPPORT_FILES)
        set_target_properties("${target}" PROPERTIES _qt_no_qmake_support_files TRUE)
    endif()

    qt_add_list_file_finalizer(qt_finalize_module
        ${target}
        ${arg_INTERNAL_MODULE}
        ${arg_NO_PRIVATE_MODULE}
    )
endfunction()

# Write and install the basic Qt6Foo and Qt6FooPrivate packages.
#
# If PRIVATE is specified, write Qt6FooPrivate.
# Otherwise write its public counterpart.
function(qt_internal_write_basic_module_package target target_private)
    set(no_value_options
        FIND_PRIVATE_MODULE
        IS_STATIC_LIB
        PRIVATE
    )
    set(single_value_options
        CONFIG_BUILD_DIR
        CONFIG_INSTALL_DIR
    )
    set(multi_value_options "")
    cmake_parse_arguments(PARSE_ARGV 0 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )

    set(always_load_private_module OFF)
    if(arg_PRIVATE)
        set(package_name "${INSTALL_CMAKE_NAMESPACE}${target_private}")
        set(module_config_input_file "QtModuleConfigPrivate.cmake.in")
    else()
        set(package_name "${INSTALL_CMAKE_NAMESPACE}${target}")
        set(module_config_input_file "QtModuleConfig.cmake.in")
        if(arg_FIND_PRIVATE_MODULE)
            set(always_load_private_module ON)
        endif()
    endif()

    if((QT_FEATURE_no_prefix OR arg_IS_STATIC_LIB)
            AND NOT arg_PRIVATE
            AND CMAKE_VERSION VERSION_LESS "3.26")
        # We auto-load the private module package from the public module package if we have a
        # statically built module or a non-prefix build and CMake's version is < 3.26. This is
        # needed for the case "Qt6Foo links against Qt6BarPrivate", because CMake generates a check
        # for the target Qt6::BarPrivate in Qt6FooTargets.cmake. Once we can require CMake 3.26, we
        # can simply link against $<BUILD_LOCAL_INTERFACE:Qt6BarPrivate> in
        # qt_internal_extend_target.
        #
        # For older CMake versions, we create an additional CMake file that's optionally included by
        # Qt6FooConfig.cmake to work around the lack of BUILD_LOCAL_INTERFACE.
        file(CONFIGURE
            OUTPUT "${arg_CONFIG_BUILD_DIR}/${package_name}-build.cmake"
            CONTENT "# This file marks this directory as part of Qt's build tree.
set(__qt_${target}_always_load_private_module ON)
"
        )
    endif()

    configure_package_config_file(
        "${QT_CMAKE_DIR}/${module_config_input_file}"
        "${arg_CONFIG_BUILD_DIR}/${package_name}Config.cmake"
        INSTALL_DESTINATION "${arg_CONFIG_INSTALL_DIR}"
    )
    write_basic_package_version_file(
        "${arg_CONFIG_BUILD_DIR}/${package_name}ConfigVersionImpl.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
    )
    qt_internal_write_qt_package_version_file(
        "${package_name}"
        "${arg_CONFIG_BUILD_DIR}/${package_name}ConfigVersion.cmake"
    )
    qt_install(FILES
        "${arg_CONFIG_BUILD_DIR}/${package_name}Config.cmake"
        "${arg_CONFIG_BUILD_DIR}/${package_name}ConfigVersion.cmake"
        "${arg_CONFIG_BUILD_DIR}/${package_name}ConfigVersionImpl.cmake"
        DESTINATION "${arg_CONFIG_INSTALL_DIR}"
        COMPONENT Devel
    )
endfunction()

function(qt_internal_export_module target)
    set(no_value_options
        NO_ADDITIONAL_TARGET_INFO
    )
    set(single_value_options
        CONFIG_BUILD_DIR
        CONFIG_INSTALL_DIR
    )
    set(multi_value_options "")
    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )
    _qt_internal_validate_all_args_are_parsed(arg)

    set(export_name "${INSTALL_CMAKE_NAMESPACE}${target}Targets")
    qt_install(TARGETS ${target}
        EXPORT ${export_name}
        RUNTIME DESTINATION ${INSTALL_BINDIR}
        LIBRARY DESTINATION ${INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${INSTALL_LIBDIR}
        FRAMEWORK DESTINATION ${INSTALL_LIBDIR}
    )

    qt_install(EXPORT ${export_name}
               NAMESPACE ${QT_CMAKE_EXPORT_NAMESPACE}::
               DESTINATION ${arg_CONFIG_INSTALL_DIR})

    if(NOT arg_NO_ADDITIONAL_TARGET_INFO)
        qt_internal_export_additional_targets_file(
            TARGETS ${target}
            EXPORT_NAME_PREFIX ${INSTALL_CMAKE_NAMESPACE}${target}
            CONFIG_INSTALL_DIR "${arg_CONFIG_INSTALL_DIR}")
    endif()

    qt_internal_export_modern_cmake_config_targets_file(
        TARGETS ${target}
        EXPORT_NAME_PREFIX ${INSTALL_CMAKE_NAMESPACE}${target}
        CONFIG_BUILD_DIR "${arg_CONFIG_BUILD_DIR}"
        CONFIG_INSTALL_DIR "${arg_CONFIG_INSTALL_DIR}"
    )

    qt_internal_export_genex_properties(TARGETS ${target}
        EXPORT_NAME_PREFIX ${INSTALL_CMAKE_NAMESPACE}${target}
        CONFIG_INSTALL_DIR "${arg_CONFIG_INSTALL_DIR}"
    )
endfunction()

function(qt_internal_apply_apple_privacy_manifest target)
    # Avoid "INTERFACE_LIBRARY targets may only have whitelisted properties" error on CMake < 3.17.
    get_target_property(target_type ${target} TYPE)
    if("${target_type}" STREQUAL "INTERFACE_LIBRARY")
        return()
    endif()

    if(APPLE)
        # Privacy manifest
        get_target_property(is_framework ${target} FRAMEWORK)
        if(is_framework)
            get_target_property(privacy_manifest ${target} _qt_privacy_manifest)
            if(NOT privacy_manifest)
                 set(privacy_manifest
                    "${__qt_internal_cmake_apple_support_files_path}/PrivacyInfo.xcprivacy")
            endif()
            target_sources("${target}" PRIVATE "${privacy_manifest}")
            set_property(TARGET "${target}" APPEND PROPERTY RESOURCE "${privacy_manifest}")
        endif()
    endif()
endfunction()

function(qt_finalize_module target)
    set(no_value_options
        INTERNAL_MODULE
        NO_PRIVATE_MODULE
    )
    set(single_value_options "")
    set(multi_value_options "")
    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )
    _qt_internal_validate_all_args_are_parsed(arg)

    qt_internal_collect_module_headers(module_headers ${target})

    # Issue a warning if we
    # - suppressed creation of the private module but have private headers
    # - created a private module but don't have any private headers
    if(NOT arg_INTERNAL_MODULE)
        get_target_property(has_private_headers ${target} _qt_module_has_private_headers)
        if(arg_NO_PRIVATE_MODULE AND has_private_headers)
            message(AUTHOR_WARNING
                "Module ${target} has private headers. "
                "Please remove NO_PRIVATE_MODULE from its creation flags."
            )
        elseif(NOT arg_NO_PRIVATE_MODULE AND NOT has_private_headers)
            message(AUTHOR_WARNING
                "Module ${target} does not have private headers. "
                "Please add NO_PRIVATE_MODULE to its creation flags."
            )
        endif()
    endif()

    # qt_internal_install_module_headers needs to be called before
    # qt_finalize_framework_headers_copy, because the last uses the QT_COPIED_FRAMEWORK_HEADERS
    # property which supposed to be updated inside every qt_internal_install_module_headers
    # call.
    qt_internal_add_headersclean_target(${target} "${module_headers_public}")
    qt_internal_target_sync_headers(${target}
        "${module_headers_all}"
        "${module_headers_generated}"
        "${module_headers_exclude_from_docs}"
    )
    get_target_property(module_depends_header ${target} _qt_module_depends_header)
    qt_internal_install_module_headers(${target}
        PUBLIC ${module_headers_public} "${module_depends_header}"
        PRIVATE ${module_headers_private}
        QPA ${module_headers_qpa}
        RHI ${module_headers_rhi}
        SSG ${module_headers_ssg}
    )

    qt_finalize_framework_headers_copy(${target})

    get_target_property(no_qmake_support_files "${target}" _qt_no_qmake_support_files)
    if(NOT no_qmake_support_files)
        qt_generate_prl_file(${target} "${INSTALL_LIBDIR}")
        qt_generate_module_pri_file("${target}" ${ARGN})
    endif()

    get_target_property(no_package_config_file "${target}" _qt_no_package_config_file)
    if(NOT no_package_config_file)
        qt_internal_generate_pkg_config_file(${target})
    endif()

    qt_internal_apply_apple_privacy_manifest(${target})
    _qt_internal_finalize_sbom(${target})
endfunction()

# Get a set of Qt module related values based on the target.
#
# The function uses the _qt_module_interface_name and _qt_module_include_name target properties to
# preform values for the output variables. _qt_module_interface_name it's the basic name of module
# without "Qtfication" and the "Private" suffix if we speak about INTERNAL_MODULEs. Typical value of
# the _qt_module_interface_name is the provided to qt_internal_add_module ${target} name, e.g. Core.
# _qt_module_interface_name is used to preform all the include paths unless the
# _qt_module_include_name property is specified. _qt_module_include_name is legacy property that
# replaces the module name in include paths and has a higher priority than the
# _qt_module_interface_name property.
#
# When doing qt_internal_module_info(foo Core) this method will set the following variables in
# the caller's scope:
#  * foo with the value "QtCore"
#  * foo_versioned with the value "Qt6Core" (based on major Qt version)
#  * foo_upper with the value "CORE"
#  * foo_lower with the value "core"
#  * foo_include_name with the value"QtCore"
#    Usually the module name from ${foo} is used, but the name might be different if the
#    MODULE_INCLUDE_NAME argument is set when creating the module.
#  * foo_versioned_include_dir with the value "QtCore/6.2.0"
#  * foo_versioned_inner_include_dir with the value "QtCore/6.2.0/QtCore"
#  * foo_private_include_dir with the value "QtCore/6.2.0/QtCore/private"
#  * foo_qpa_include_dir with the value "QtCore/6.2.0/QtCore/qpa"
#  * foo_rhi_include_dir with the value "QtCore/6.2.0/QtCore/rhi"
#  * foo_ssg_include_dir with the value "QtQuick3D/6.2.0/QtQuick3D/ssg"
#  * foo_interface_name the interface name of the module stored in _qt_module_interface_name
#    property, e.g. Core.
#
# The function also sets a bunch of module include paths for the build and install interface.
# Variables that contains these paths start with foo_build_interface_ and foo_install_interface_
# accordingly.
# The following variables are set in the caller's scope:
#  * foo_<build|install>_interface_include_dir with
#    qtbase_build_dir/include/QtCore for build interface and
#    include/QtCore for install interface.
#  * foo_<build|install>_interface_versioned_include_dir with
#    qtbase_build_dir/include/QtCore/6.2.0 for build interface and
#    include/QtCore/6.2.0 for install interface.
#  * foo_<build|install>_versioned_inner_include_dir with
#    qtbase_build_dir/include/QtCore/6.2.0/QtCore for build interface and
#    include/QtCore/6.2.0/QtCore for install interface.
#  * foo_<build|install>_private_include_dir with
#    qtbase_build_dir/include/QtCore/6.2.0/QtCore/private for build interface and
#    include/QtCore/6.2.0/QtCore/private for install interface.
#  * foo_<build|install>_qpa_include_dir with
#    qtbase_build_dir/include/QtCore/6.2.0/QtCore/qpa for build interface and
#    include/QtCore/6.2.0/QtCore/qpa for install interface.
#  * foo_<build|install>_rhi_include_dir with
#    qtbase_build_dir/include/QtCore/6.2.0/QtCore/rhi for build interface and
#    include/QtCore/6.2.0/QtCore/rhi for install interface.
#  * foo_<build|install>_ssg_include_dir with
#    qtbase_build_dir/include/<module>/x.y.z/<module>/ssg for build interface and
#    include/<module>/x.y.z/<module>/ssg for install interface.
# The following values are set by the function and might be useful in caller's scope:
#  * repo_install_interface_include_dir contains path to the top-level repository include directory,
#    e.g. qtbase_build_dir/include
#  * repo_install_interface_include_dir contains path to the non-prefixed top-level include
#    directory is used for the installation, e.g. include
# Note: that for non-prefixed Qt configurations the build interface paths will start with
# <build_directory>/qtbase/include, e.g foo_build_interface_include_dir of the Qml module looks
# like qt_toplevel_build_dir/qtbase/include/QtQml
function(qt_internal_module_info result target)
    if(result STREQUAL "repo")
        message(FATAL_ERROR "'repo' keyword is reserved for internal use, please specify \
the different base name for the module info variables.")
    endif()

    get_target_property(module_interface_name ${target} _qt_module_interface_name)
    if(NOT module_interface_name)
        message(FATAL_ERROR "${target} is not a module.")
    endif()

    qt_internal_qtfy_target(module ${module_interface_name})

    get_target_property("${result}_include_name" ${target} _qt_module_include_name)
    if(NOT ${result}_include_name)
        set("${result}_include_name" "${module}")
    endif()

    set("${result}_versioned_include_dir"
        "${${result}_include_name}/${PROJECT_VERSION}")
    set("${result}_versioned_inner_include_dir"
        "${${result}_versioned_include_dir}/${${result}_include_name}")
    set("${result}_private_include_dir"
        "${${result}_versioned_inner_include_dir}/private")
    set("${result}_qpa_include_dir"
        "${${result}_versioned_inner_include_dir}/qpa")
    set("${result}_rhi_include_dir"
        "${${result}_versioned_inner_include_dir}/rhi")
    set("${result}_ssg_include_dir"
        "${${result}_versioned_inner_include_dir}/ssg")

    # Module build interface directories
    set(repo_build_interface_include_dir "${QT_BUILD_DIR}/include")
    set("${result}_build_interface_include_dir"
        "${repo_build_interface_include_dir}/${${result}_include_name}")
    set("${result}_build_interface_versioned_include_dir"
        "${repo_build_interface_include_dir}/${${result}_versioned_include_dir}")
    set("${result}_build_interface_versioned_inner_include_dir"
        "${repo_build_interface_include_dir}/${${result}_versioned_inner_include_dir}")
    set("${result}_build_interface_private_include_dir"
        "${repo_build_interface_include_dir}/${${result}_private_include_dir}")
    set("${result}_build_interface_qpa_include_dir"
        "${repo_build_interface_include_dir}/${${result}_qpa_include_dir}")
    set("${result}_build_interface_rhi_include_dir"
        "${repo_build_interface_include_dir}/${${result}_rhi_include_dir}")
    set("${result}_build_interface_ssg_include_dir"
        "${repo_build_interface_include_dir}/${${result}_ssg_include_dir}")

    # Module install interface directories
    set(repo_install_interface_include_dir "${INSTALL_INCLUDEDIR}")
    set("${result}_install_interface_include_dir"
        "${repo_install_interface_include_dir}/${${result}_include_name}")
    set("${result}_install_interface_versioned_include_dir"
        "${repo_install_interface_include_dir}/${${result}_versioned_include_dir}")
    set("${result}_install_interface_versioned_inner_include_dir"
        "${repo_install_interface_include_dir}/${${result}_versioned_inner_include_dir}")
    set("${result}_install_interface_private_include_dir"
        "${repo_install_interface_include_dir}/${${result}_private_include_dir}")
    set("${result}_install_interface_qpa_include_dir"
        "${repo_install_interface_include_dir}/${${result}_qpa_include_dir}")
    set("${result}_install_interface_rhi_include_dir"
        "${repo_install_interface_include_dir}/${${result}_rhi_include_dir}")
    set("${result}_install_interface_ssg_include_dir"
        "${repo_install_interface_include_dir}/${${result}_ssg_include_dir}")

    set("${result}" "${module}" PARENT_SCOPE)
    set("${result}_versioned" "${module_versioned}" PARENT_SCOPE)
    string(TOUPPER "${module_interface_name}" upper)
    string(TOLOWER "${module_interface_name}" lower)
    set("${result}_upper" "${upper}" PARENT_SCOPE)
    set("${result}_lower" "${lower}" PARENT_SCOPE)
    set("${result}_include_name" "${${result}_include_name}" PARENT_SCOPE)
    set("${result}_versioned_include_dir" "${${result}_versioned_include_dir}" PARENT_SCOPE)
    set("${result}_versioned_inner_include_dir"
        "${${result}_versioned_inner_include_dir}" PARENT_SCOPE)
    set("${result}_private_include_dir" "${${result}_private_include_dir}" PARENT_SCOPE)
    set("${result}_qpa_include_dir" "${${result}_qpa_include_dir}" PARENT_SCOPE)
    set("${result}_rhi_include_dir" "${${result}_rhi_include_dir}" PARENT_SCOPE)
    set("${result}_ssg_include_dir" "${${result}_ssg_include_dir}" PARENT_SCOPE)
    set("${result}_interface_name" "${module_interface_name}" PARENT_SCOPE)

    # Setting module build interface directories in parent scope
    set(repo_build_interface_include_dir "${repo_build_interface_include_dir}" PARENT_SCOPE)
    set("${result}_build_interface_include_dir"
        "${${result}_build_interface_include_dir}" PARENT_SCOPE)
    set("${result}_build_interface_versioned_include_dir"
        "${${result}_build_interface_versioned_include_dir}" PARENT_SCOPE)
    set("${result}_build_interface_versioned_inner_include_dir"
        "${${result}_build_interface_versioned_inner_include_dir}" PARENT_SCOPE)
    set("${result}_build_interface_private_include_dir"
        "${${result}_build_interface_private_include_dir}" PARENT_SCOPE)
    set("${result}_build_interface_qpa_include_dir"
        "${${result}_build_interface_qpa_include_dir}" PARENT_SCOPE)
    set("${result}_build_interface_rhi_include_dir"
        "${${result}_build_interface_rhi_include_dir}" PARENT_SCOPE)
    set("${result}_build_interface_ssg_include_dir"
        "${${result}_build_interface_ssg_include_dir}" PARENT_SCOPE)

    # Setting module install interface directories in parent scope
    set(repo_install_interface_include_dir "${repo_install_interface_include_dir}" PARENT_SCOPE)
    set("${result}_install_interface_include_dir"
        "${${result}_install_interface_include_dir}" PARENT_SCOPE)
    set("${result}_install_interface_versioned_include_dir"
        "${${result}_install_interface_versioned_include_dir}" PARENT_SCOPE)
    set("${result}_install_interface_versioned_inner_include_dir"
        "${${result}_install_interface_versioned_inner_include_dir}" PARENT_SCOPE)
    set("${result}_install_interface_private_include_dir"
        "${${result}_install_interface_private_include_dir}" PARENT_SCOPE)
    set("${result}_install_interface_qpa_include_dir"
        "${${result}_install_interface_qpa_include_dir}" PARENT_SCOPE)
    set("${result}_install_interface_rhi_include_dir"
        "${${result}_install_interface_rhi_include_dir}" PARENT_SCOPE)
    set("${result}_install_interface_ssg_include_dir"
        "${${result}_install_interface_ssg_include_dir}" PARENT_SCOPE)
endfunction()

function(qt_internal_list_to_json_array out_var list_var)
    set(result "")
    foreach(item IN LISTS ${list_var})
        if(NOT "${result}" STREQUAL "")
            string(APPEND result ", ")
        endif()
        string(APPEND result "\"${item}\"")
    endforeach()
    set("${out_var}" "[${result}]" PARENT_SCOPE)
endfunction()

# Generate a module description file based on the template in ModuleDescription.json.in
function(qt_describe_module target)
    set(path_suffix "${INSTALL_DESCRIPTIONSDIR}")
    qt_path_join(build_dir ${QT_BUILD_DIR} ${path_suffix})
    qt_path_join(install_dir ${QT_INSTALL_DIR} ${path_suffix})

    set(descfile_in "${QT_CMAKE_DIR}/ModuleDescription.json.in")

    # IMPORTANT: If you adjust the file name not to be the exact target name and thus the CMake
    # package name, it needs to consider also the code in QtConfig.cmake.in that globs the json
    # files.
    set(descfile_out "${build_dir}/${target}.json")

    string(TOLOWER "${PROJECT_NAME}" lower_case_project_name)
    set(extra_module_information "")
    set(platforms_information "")

    get_target_property(target_type ${target} TYPE)
    if(NOT target_type STREQUAL "INTERFACE_LIBRARY")
        get_target_property(plugin_types ${target} QMAKE_MODULE_PLUGIN_TYPES)
        if(plugin_types)
            qt_internal_list_to_json_array(plugin_types plugin_types)
            string(APPEND extra_module_information "\n    \"plugin_types\": ${plugin_types},")
        endif()
    endif()

    # Generate extra module information
    get_target_property(is_internal ${target} _qt_is_internal_module)
    if(is_internal)
        string(APPEND extra_module_information "\n    \"internal\": true,")
    endif()
    if(APPLE)
        set(bundle_type "none")
        if(QT_FEATURE_framework)
            set(bundle_type "framework")
        endif()
        string(APPEND extra_module_information "\n    \"bundle_type\": \"framework\",")
    endif()
    if(NOT QT_NAMESPACE STREQUAL "")
        string(APPEND extra_module_information "\n    \"namespace\": \"${QT_NAMESPACE}\",")
    endif()
    if(target STREQUAL "Gui")
        qt_internal_list_to_json_array(qpa_platforms_array QT_QPA_PLATFORMS)
        string(APPEND extra_module_information "
    \"qpa\": {
        \"platforms\": ${qpa_platforms_array},
        \"default_platform\": \"${QT_QPA_DEFAULT_PLATFORM}\"
    },")
    endif()

    # Set up indentation helper variables.
    set(indent1 "    ")
    set(k 1)
    foreach(i RANGE 2 5)
        set(indent${i} "${indent${k}}${indent1}")
        set(k ${i})
    endforeach()

    # Set up the platforms to write.
    set(nr_of_platforms 1)
    set(platform_0_name "${CMAKE_SYSTEM_NAME}")
    set(platform_0_variant "")
    set(platform_0_architectures "${TEST_architecture_architectures}")

    # Handle iOS builds specially.
    if(platform_0_name STREQUAL "iOS")
        if(QT_FEATURE_simulator_and_device)
            # This must match the setup done in qt_auto_detect_apple.
            set(nr_of_platforms 2)
            set(platform_0_name "iOS")
            set(platform_0_variant "iphoneos")
            set(platform_0_architectures "arm64")
            set(platform_1_name "iOS")
            set(platform_1_variant "iphonesimulator")
            set(platform_1_architectures "x86_64")
        elseif(NOT "${QT_APPLE_SDK}" STREQUAL "")
            # Explicit SDK requested.
            set(platform_0_variant "${QT_APPLE_SDK}")
        endif()
    endif()

    # Write platform information. At the moment, we write exactly one platform. With xcframeworks
    # for example, we'd support multiple platforms.
    math(EXPR last_platform_idx "${nr_of_platforms} - 1")
    foreach(i RANGE 0 ${last_platform_idx})
        # Write target architecture information.
        set(platform_name "${platform_${i}_name}")
        set(platform_variant "${platform_${i}_variant}")
        set(platform_architectures "${platform_${i}_architectures}")
        set(targets_information "")
        foreach(architecture IN LISTS platform_architectures)
            if(NOT targets_information STREQUAL "")
                string(APPEND targets_information ",")
            endif()
            string(APPEND targets_information "\n${indent4}{\n")
            if(NOT QT_FEATURE_shared)
                string(APPEND targets_information "${indent5}\"static\": true,\n")
            endif()
            if(ANDROID)
                string(APPEND targets_information
                    "${indent5}\"api_version\": \"${QT_ANDROID_API_USED_FOR_JAVA}\",\n"
                    "${indent5}\"ndk_version\": \"${ANDROID_NDK_REVISION}\",\n"
                    "${indent5}\"android_platform\": \"${ANDROID_PLATFORM}\",\n")
            endif()
            string(APPEND targets_information "${indent5}\"architecture\": \"${architecture}\",\n")
            string(APPEND targets_information "${indent5}\"abi\": \"${TEST_arch_${architecture}_abi}\"\n")
            string(APPEND targets_information "${indent4}}")
        endforeach()

        if(i GREATER 0)
            string(APPEND platforms_information ",")
        endif()
        string(APPEND platforms_information "
${indent2}{
${indent3}\"name\": \"${platform_name}\",")
        if(NOT platform_variant STREQUAL "")
            string(APPEND platforms_information "
${indent3}\"variant\": \"${platform_variant}\",")
        endif()
        if(NOT "${CMAKE_SYSTEM_VERSION}" STREQUAL "")
            string(APPEND platforms_information "
${indent3}\"version\": \"${CMAKE_SYSTEM_VERSION}\",")
        endif()
        string(APPEND platforms_information "
${indent3}\"compiler_id\": \"${CMAKE_CXX_COMPILER_ID}\",
${indent3}\"compiler_version\": \"${CMAKE_CXX_COMPILER_VERSION}\",
${indent3}\"targets\": [${targets_information}
${indent3}]
${indent2}}")
    endforeach()

    configure_file("${descfile_in}" "${descfile_out}")
    qt_install(FILES "${descfile_out}" DESTINATION "${install_dir}")
endfunction()

function(qt_internal_generate_cpp_global_exports target module_define_infix)
    cmake_parse_arguments(arg
        ""
        "CPP_EXPORT_HEADER_BASE_NAME"
        "" ${ARGN}
    )

    qt_internal_module_info(module "${target}")

    set(header_base_name "qt${module_lower}exports")
    if(arg_CPP_EXPORT_HEADER_BASE_NAME)
        set(header_base_name "${arg_CPP_EXPORT_HEADER_BASE_NAME}")
    endif()
    # Is used as a part of the header guard define.
    string(TOUPPER "${header_base_name}" header_base_name_upper)

    set(generated_header_path
        "${module_build_interface_include_dir}/${header_base_name}.h"
    )

    configure_file("${QT_CMAKE_DIR}/modulecppexports.h.in"
        "${generated_header_path}" @ONLY
    )

    set(${out_public_header} "${generated_header_path}" PARENT_SCOPE)
    target_sources(${target} PRIVATE "${generated_header_path}")
    _qt_internal_set_source_file_generated(
        SOURCES "${generated_header_path}"
        CONFIGURE_GENERATED
    )
    # Make sure the file is installed when processed by `qt_internal_collect_module_headers`
    set_source_files_properties("${generated_header_path}" PROPERTIES _qt_syncqt_force_include TRUE)
endfunction()

function(qt_internal_install_module_headers target)
    set(options)
    set(one_value_args)
    set(multi_value_args PUBLIC PRIVATE QPA RHI SSG)
    cmake_parse_arguments(arg "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    qt_internal_module_info(module ${target})

    get_target_property(target_type ${target} TYPE)
    set(is_interface_lib FALSE)
    if(target_type STREQUAL "INTERFACE_LIBRARY")
        set(is_interface_lib TRUE)
    else()
        get_target_property(is_framework ${target} FRAMEWORK)
    endif()


    foreach(header_type IN LISTS multi_value_args)
        if(NOT arg_${header_type})
            set(arg_${header_type} "")
        endif()
    endforeach()

    if(is_framework)
        qt_copy_framework_headers(${target}
            PUBLIC ${arg_PUBLIC}
            PRIVATE ${arg_PRIVATE}
            QPA ${arg_QPA}
            RHI ${arg_RHI}
            SSG ${arg_SSG}
        )
    else()
        if(arg_PUBLIC)
            qt_install(FILES ${arg_PUBLIC}
                 DESTINATION "${module_install_interface_include_dir}")
        endif()
        if(arg_PRIVATE)
            qt_install(FILES ${arg_PRIVATE}
                 DESTINATION "${module_install_interface_private_include_dir}")
        endif()
        if(arg_QPA)
            qt_install(FILES ${arg_QPA} DESTINATION "${module_install_interface_qpa_include_dir}")
        endif()
        if(arg_RHI)
            qt_install(FILES ${arg_RHI} DESTINATION "${module_install_interface_rhi_include_dir}")
        endif()
        if(arg_SSG)
            qt_install(FILES ${arg_SSG} DESTINATION "${module_install_interface_ssg_include_dir}")
        endif()
    endif()
endfunction()

function(qt_internal_collect_module_headers out_var target)
    set(${out_var}_public "")
    set(${out_var}_private "")
    set(${out_var}_exclude_from_docs "")
    set(${out_var}_qpa "")
    set(${out_var}_rhi "")
    set(${out_var}_ssg "")
    set(${out_var}_all "")

    qt_internal_get_target_sources(sources ${target})

    get_target_property(target_type ${target} TYPE)
    if(target_type STREQUAL "INTERFACE_LIBRARY")
        set(source_dir "${CMAKE_CURRENT_SOURCE_DIR}")
        set(binary_dir "${CMAKE_CURRENT_BINARY_DIR}")
    else()
        get_target_property(source_dir ${target} SOURCE_DIR)
        get_target_property(binary_dir ${target} BINARY_DIR)
    endif()
    get_filename_component(source_dir "${source_dir}" ABSOLUTE)
    get_filename_component(binary_dir "${binary_dir}" ABSOLUTE)

    get_target_property(is_3rdparty_library ${target} _qt_module_is_3rdparty_header_library)

    get_target_property(public_filter ${target} _qt_module_public_headers_filter_regex)
    get_target_property(private_filter ${target} _qt_module_private_headers_filter_regex)
    get_target_property(qpa_filter ${target} _qt_module_qpa_headers_filter_regex)
    get_target_property(rhi_filter ${target} _qt_module_rhi_headers_filter_regex)
    get_target_property(ssg_filter ${target} _qt_module_ssg_headers_filter_regex)

    set(condition_independent_headers_warning "")
    foreach(file_path IN LISTS sources)
        get_filename_component(file_name "${file_path}" NAME)
        if(NOT file_name MATCHES ".+\\.h$")
            continue()
        endif()

        get_source_file_property(non_module_header ${file_path} _qt_non_module_header)
        if(non_module_header)
            continue()
        endif()

        get_filename_component(file_path "${file_path}" ABSOLUTE)

        _qt_internal_path_is_prefix(source_dir "${file_path}" is_inside_module_source_dir)

        get_source_file_property(forced_include "${file_path}" _qt_syncqt_force_include)
        get_source_file_property(is_generated "${file_path}" GENERATED)
        # Skip all header files outside the module source directory, except the generated files,
        # or files explicitly marked to be included for syncqt.
        if(NOT (forced_include OR is_inside_module_source_dir OR is_generated))
            continue()
        endif()

        get_source_file_property(condition ${file_path} _qt_extend_target_condition)
        if(NOT condition STREQUAL "" AND NOT condition STREQUAL "NOTFOUND")
            list(JOIN condition " " condition_string)
            string(APPEND condition_independent_headers_warning
                "\nFile:\n    ${file_path}"
                "\nCondition:\n    ${condition_string}")
        endif()

        if(is_inside_module_source_dir)
            set(base_dir "${source_dir}")
        else()
            set(base_dir "${binary_dir}")
        endif()

        file(RELATIVE_PATH file_path_rel "${base_dir}" "${file_path}")
        if(file_path_rel MATCHES "3rdparty/.+" AND NOT is_3rdparty_library)
            set(is_3rdparty_header TRUE)
        else()
            set(is_3rdparty_header FALSE)
        endif()
        list(APPEND ${out_var}_all "${file_path}")

        get_source_file_property(exclude_from_docs "${file_path}" _qt_syncqt_exclude_from_docs)
        if(exclude_from_docs)
            list(APPEND ${out_var}_exclude_from_docs "${file_path}")
        endif()

        if(qpa_filter AND file_name MATCHES "${qpa_filter}")
            list(APPEND ${out_var}_qpa "${file_path}")
        elseif(rhi_filter AND file_name MATCHES "${rhi_filter}")
            list(APPEND ${out_var}_rhi "${file_path}")
        elseif(ssg_filter AND file_name MATCHES "${ssg_filter}")
            list(APPEND ${out_var}_ssg "${file_path}")
        elseif(private_filter AND file_name MATCHES "${private_filter}")
            list(APPEND ${out_var}_private "${file_path}")
        elseif((NOT public_filter OR file_name MATCHES "${public_filter}")
            AND NOT is_3rdparty_header)
            list(APPEND ${out_var}_public "${file_path}")
        endif()
        if(is_generated)
            list(APPEND ${out_var}_generated "${file_path}")
        endif()
    endforeach()

    if(NOT condition_independent_headers_warning STREQUAL "" AND QT_FEATURE_developer_build)
        message(AUTHOR_WARNING "Condition is ignored when adding the following header file(s) to"
            " the ${target} module:"
            "${condition_independent_headers_warning}"
            "\nThe usage of the file(s) is not properly isolated in this or other modules according"
            " to the condition. This warning is for the Qt maintainers. Please make sure that file"
            " include(s) are guarded with the appropriate macros in the Qt code. If files should be"
            " added to the module unconditionally, please move them to the common SOURCES section"
            " in the qt_internal_add_module call.")
    endif()


    set(header_types public private qpa rhi ssg)
    set(has_header_types_properties "")
    foreach(header_type IN LISTS header_types)
        get_target_property(current_propety_value ${target} _qt_module_has_${header_type}_headers)
        if(${out_var}_${header_type})
            list(APPEND has_header_types_properties
                    _qt_module_has_${header_type}_headers TRUE)
        endif()

        set(${out_var}_${header_type} "${${out_var}_${header_type}}" PARENT_SCOPE)
    endforeach()
    set(${out_var}_all "${${out_var}_all}" PARENT_SCOPE)
    set(${out_var}_generated "${${out_var}_generated}" PARENT_SCOPE)
    set(${out_var}_exclude_from_docs "${${out_var}_exclude_from_docs}" PARENT_SCOPE)

    if(has_header_types_properties)
        set_target_properties(${target} PROPERTIES ${has_header_types_properties})
    endif()
    set_property(TARGET ${target} APPEND PROPERTY
        EXPORT_PROPERTIES
            _qt_module_has_public_headers
            _qt_module_has_private_headers
            _qt_module_has_qpa_headers
            _qt_module_has_rhi_headers
            _qt_module_has_ssg_headers
    )
endfunction()

# Set the value of the respective module properties and make the properties
# transitive. The property is not stored as target property, but is set as
# INTERFACE property, so its value is not considered by target itself, but only
# by depending targets. Also this require all properties have the
# INTERFACE_<property_name> name format.
#
# Synopsis
#   qt_internal_set_module_transitive_properties(<target>
#       PROPERTIES <prop1> <value1> [<prop2> <value2>] ...
#   )
#
# Arguments
#
# `target` Qt module target. Unlike CMake set_target_properties this function
#   accepts only one target as argument.
#
# `PROPERTIES` List of the property name-value pairs.
#
# `TYPE` The transitive property type: COMPILE or LINK.
function(qt_internal_set_module_transitive_properties target)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "TYPE" "PROPERTIES")

    if(NOT arg_PROPERTIES)
        message(FATAL_ERROR "PROPERTIES argument is missing.")
    endif()

    if(NOT arg_TYPE)
        message(FATAL_ERROR "TYPE argument is missing.")
    endif()

    list(LENGTH arg_PROPERTIES count)
    math(EXPR even_args_count "${count} % 2")
    if(NOT even_args_count EQUAL 0)
        message(FATAL_ERROR "Insufficient number of PROPERTIES values.")
    endif()

    _qt_internal_dealias_target(target)

    set(property_names "")
    set(internal_property_names "")

    math(EXPR last "${count} - 1")
    foreach(name_idx RANGE 0 ${last} 2)
        list(GET arg_PROPERTIES ${name_idx} interface_property_name)
        if(interface_property_name MATCHES "^INTERFACE_(.+)$")
            set(property_name "${CMAKE_MATCH_1}")
        else()
            message(FATAL_ERROR "Incorrect property name ${interface_property_name}. The property"
            " name must have the INTERFACE_ prefix. Use regular set_target_properties call to set"
            " the non-transitive property.")
        endif()

        string(TOLOWER "${property_name}" property_name_lower)
        list(APPEND property_names ${property_name})

        math(EXPR value_idx "${name_idx} + 1")
        list(GET arg_PROPERTIES ${value_idx} property_value)

        if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.30)
            # The collected interface properties exposed in module build tree and have the
            # same transitive capabilities as after the module is installed. Supported for
            # CMake version >= 3.30
            set_property(TARGET ${target} PROPERTY ${interface_property_name} "${property_value}")
        else()
            # Internal properties are exported within the Qt module. They have limitations that
            # EXPORT_PROPERTIES apply. These properties are exported even if we are building Qt
            # with CMake versions that do not support transitive properties. It allows using
            # them as transitive properties in user projects if CMake allows this.
            list(APPEND internal_property_names _qt_internal_${property_name_lower})
            set_property(TARGET ${target} PROPERTY
                _qt_internal_${property_name_lower} "${property_value}")
        endif()

        _qt_internal_add_transitive_property(${target} ${arg_TYPE} ${property_name})
    endforeach()

    get_target_property(transitive_properties ${target} _qt_transitive_${type_lower}_properties)
    if(NOT transitive_properties)
        set(transitive_properties "")
    endif()
    list(APPEND transitive_properties ${property_names})
    list(REMOVE_DUPLICATES transitive_properties)

    get_target_property(export_properties ${target} EXPORT_PROPERTIES)
    if(NOT export_properties)
        set(export_properties "")
    endif()
    list(APPEND export_properties ${internal_property_names})
    list(REMOVE_DUPLICATES export_properties)

    string(TOLOWER "${arg_TYPE}" type_lower)
    set_target_properties(${target} PROPERTIES
        EXPORT_PROPERTIES "${export_properties}"
        _qt_transitive_${type_lower}_properties "${transitive_properties}"
    )
endfunction()
