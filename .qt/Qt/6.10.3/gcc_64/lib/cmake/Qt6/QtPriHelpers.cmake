# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Extracts the 3rdparty libraries for the module ${module_name}
# and stores the information in cmake language in
# ${output_root_dir}/$<CONFIG>/${output_file_name}.
#
# This function "follows" INTERFACE_LIBRARY targets to "real" targets
# and collects defines, include dirs and lib dirs on the way.
function(qt_generate_qmake_libraries_pri_content module_name output_root_dir output_file_name)
    set(content "")

    # Set up a regular expression that matches all implicit include dirs
    set(implicit_include_dirs_regex "")
    foreach(dir ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
        qt_re_escape(regex "${dir}")
        list(APPEND implicit_include_dirs_regex "^${regex}$")
    endforeach()
    list(JOIN implicit_include_dirs_regex "|" implicit_include_dirs_regex)

    foreach(lib ${QT_QMAKE_LIBS_FOR_${module_name}})
        set(lib_targets ${QT_TARGETS_OF_QMAKE_LIB_${lib}})
        string(TOUPPER ${lib} uclib)
        string(REPLACE "-" "_" uclib "${uclib}")
        set(lib_defines "")
        set(lib_incdir "")
        set(lib_libdir "")
        set(lib_libs "")
        set(seen_targets "")
        while(lib_targets)
            list(POP_BACK lib_targets lib_target)
            if(TARGET ${lib_target})
                if(${lib_target} IN_LIST seen_targets)
                    continue()
                endif()
                list(APPEND seen_targets ${lib_target})
                get_target_property(lib_target_type ${lib_target} TYPE)
                if(lib_target_type MATCHES "^(INTERFACE|UNKNOWN)_LIBRARY")
                    get_target_property(iface_libs ${lib_target} INTERFACE_LINK_LIBRARIES)
                    if(iface_libs)
                        list(PREPEND lib_targets ${iface_libs})
                    endif()
                endif()
                if(NOT lib_target_type STREQUAL "INTERFACE_LIBRARY")
                    list(APPEND lib_libs "$<TARGET_LINKER_FILE:${lib_target}>")
                endif()
                list(APPEND lib_libdir  "$<TARGET_PROPERTY:${lib_target},INTERFACE_LINK_DIRECTORIES>")

                get_target_property(skip_include_dir "${lib_target}" _qt_skip_include_dir_for_pri)
                if(skip_include_dir)
                    set(target_include_dir "")
                else()
                    set(target_include_dir
                        "$<TARGET_PROPERTY:${lib_target},INTERFACE_INCLUDE_DIRECTORIES>")
                endif()

                list(APPEND lib_incdir "${target_include_dir}")
                list(APPEND lib_defines "$<TARGET_PROPERTY:${lib_target},INTERFACE_COMPILE_DEFINITIONS>")
            else()
                # Strip any directory scope tokens.
                __qt_internal_strip_target_directory_scope_token("${lib_target}" lib_target)

                # Skip CMAKE_DIRECTORY_ID_SEP. If a target_link_libraries is applied to a target
                # that was defined in a different scope, CMake appends and prepends a special
                # directory id separator. Filter those out.
                if(lib_target MATCHES "^::@")
                    continue()

                elseif(lib_target MATCHES "^\\$<TARGET_OBJECTS:")
                    # Skip object files.
                    continue()

                elseif(lib_target MATCHES "/([^/]+).framework$")
                    # Handle frameworks
                    list(APPEND lib_libs "-framework ${CMAKE_MATCH_1}")

                elseif(lib_target MATCHES "^\\$<LINK_ONLY:(.*)>$")
                    # Extract value of LINK_ONLY genex, because it can't be used in file(GENERATE)
                    set(lib_target "${CMAKE_MATCH_1}")
                    if(lib_target)
                        list(PREPEND lib_targets ${lib_target})
                    endif()
                else()
                    # Regular library name, library path, or -lfoo-like flag. Check for emptiness.
                    if(lib_target)
                        list(APPEND lib_libs "${lib_target}")
                    endif()
                endif()
            endif()
        endwhile()

        # Wrap in $<REMOVE_DUPLICATES:...> but not the libs, because
        # we would have to preserve the right order for the linker.
        foreach(sfx libdir incdir defines)
            string(PREPEND lib_${sfx} "$<REMOVE_DUPLICATES:")
            string(APPEND lib_${sfx} ">")
        endforeach()

        # Filter out implicit include directories
        if(implicit_include_dirs_regex)
            string(PREPEND lib_incdir "$<FILTER:")
            string(APPEND lib_incdir ",EXCLUDE,${implicit_include_dirs_regex}>")
        endif()

        set(uccfg $<UPPER_CASE:$<CONFIG>>)
        string(APPEND content "list(APPEND known_libs ${uclib})
set(QMAKE_LIBS_${uclib}_${uccfg} \"${lib_libs}\")
set(QMAKE_LIBDIR_${uclib}_${uccfg} \"${lib_libdir}\")
set(QMAKE_INCDIR_${uclib}_${uccfg} \"${lib_incdir}\")
set(QMAKE_DEFINES_${uclib}_${uccfg} \"${lib_defines}\")
")
        if(QT_QMAKE_LIB_DEPS_${lib})
            string(APPEND content "set(QMAKE_DEPENDS_${uclib}_CC, ${deps})
set(QMAKE_DEPENDS_${uclib}_LD, ${deps})
")
        endif()
    endforeach()

    file(GENERATE
        OUTPUT "${output_root_dir}/$<CONFIG>/${output_file_name}"
        CONTENT "${content}"
    )
endfunction()

# Retrieves the public Qt module dependencies of the given Qt module or Qt Private module.
# The returned dependencies are "config module names", not target names.
#
# PRIVATE (OPTIONAL):
#     Retrieve private dependencies only. Dependencies that appear in both, LINK_LIBRARIES and
#     INTERFACE_LINK_LIBRARIES are discarded.
#
function(qt_get_direct_module_dependencies target out_var)
    cmake_parse_arguments(arg "PRIVATE" "" "" ${ARGN})
    set(dependencies "")
    get_target_property(target_type ${target} TYPE)
    if(arg_PRIVATE)
        if(target_type STREQUAL "INTERFACE_LIBRARY")
            set(libs)
        else()
            get_target_property(libs ${target} LINK_LIBRARIES)
            list(REMOVE_DUPLICATES libs)
        endif()
        get_target_property(public_libs ${target} INTERFACE_LINK_LIBRARIES)
        list(REMOVE_DUPLICATES public_libs)

        # Remove all Qt::Foo and Qt6::Foo from libs that also appear in public_libs.
        set(libs_to_remove "")
        foreach(lib IN LISTS public_libs)
            list(APPEND libs_to_remove "${lib}")
            if(lib MATCHES "^Qt::(.*)")
                list(APPEND libs_to_remove "${QT_CMAKE_EXPORT_NAMESPACE}::${CMAKE_MATCH_1}")
            elseif(lib MATCHES "^{QT_CMAKE_EXPORT_NAMESPACE}::(.*)")
                list(APPEND libs_to_remove "Qt::${CMAKE_MATCH_1}")
            endif()
        endforeach()

        list(REMOVE_DUPLICATES libs_to_remove)
        list(REMOVE_ITEM libs ${libs_to_remove})
    else()
        get_target_property(libs ${target} INTERFACE_LINK_LIBRARIES)
    endif()
    if(NOT libs)
        set(libs "")
    endif()
    while(libs)
        list(POP_FRONT libs lib)
        string(GENEX_STRIP "${lib}" lib)
        if(NOT lib OR NOT TARGET "${lib}")
            continue()
        endif()
        get_target_property(lib_type ${lib} TYPE)
        get_target_property(aliased_target ${lib} ALIASED_TARGET)
        if(TARGET "${aliased_target}")
            # If versionless target is alias, use what the alias points to.
            list(PREPEND libs "${aliased_target}")
            continue()
        endif()
        if(lib_type STREQUAL "OBJECT_LIBRARY")
            # Skip object libraries, because they're already part of ${target}.
            continue()
        elseif(lib_type STREQUAL "STATIC_LIBRARY" AND target_type STREQUAL "SHARED_LIBRARY")
            # Skip static libraries if ${target} is a shared library.
            continue()
        endif()
        get_target_property(lib_config_module_name ${lib} "_qt_config_module_name")
        if(lib_config_module_name)
            list(APPEND dependencies ${lib_config_module_name})
        endif()
    endwhile()
    set(${out_var} ${dependencies} PARENT_SCOPE)
endfunction()

# Return a list of qmake library names for a given list of targets.
# For example, Foo::Foo_nolink is mapped to foo/nolink.
# Also targets with the _qt_is_nolink_target property are mapped to nolink as well.
function(qt_internal_map_targets_to_qmake_libs out_var)
    set(result "")
    foreach(target ${ARGN})
        # Unwrap optional targets. Needed for Vulkan.
        if(target MATCHES "^\\$<TARGET_NAME_IF_EXISTS:(.*)>$")
            set(target ${CMAKE_MATCH_1})
        endif()

        set(is_no_link_target FALSE)

        # First case of detecting nolink targets (possibly not needed anymore)
        string(REGEX REPLACE "_nolink$" "" stripped_target "${target}")
        if(NOT target STREQUAL stripped_target)
            set(is_no_link_target TRUE)
        endif()

        # Second case of detecting nolink targets.
        if(TARGET "${target}")
            get_target_property(marked_as_no_link_target "${target}" _qt_is_nolink_target)
            if(marked_as_no_link_target)
                set(is_no_link_target TRUE)
            endif()
        endif()

        qt_internal_map_target_to_qmake_lib(${stripped_target} qmake_lib)
        if(NOT "${qmake_lib}" STREQUAL "")
            if(is_no_link_target)
                string(APPEND qmake_lib "/nolink")
            endif()
            list(APPEND result "${qmake_lib}")
        endif()
    endforeach()
    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

# Retrieve the runtime dependencies of module ${target}.
# The runtime dependencies are what in CMake is called IMPORTED_LINK_DEPENDENT_LIBRARIES.
# This function returns the dependencies in out_var, separated by space.
#
# PUBLIC_DEPENDENCIES:
#     List of the module's public dependencies.
#     The dependencies are expected to be config module names.
#
# PUBLIC (OPTIONAL):
#     Specifies that target is a public module.
#     If not specified, a private module is assumed.
#
function(qt_internal_get_module_run_dependencies out_var target)
    cmake_parse_arguments(arg "PUBLIC" "" "PUBLIC_DEPENDENCIES" ${ARGN})

    # Private dependencies of the module are runtime dependencies.
    qt_get_direct_module_dependencies(${target} run_dependencies PRIVATE)

    # If ${target} is a public module then public dependencies
    # of the private module are also runtime dependencies.
    if(arg_PUBLIC AND TARGET ${target}Private)
        qt_get_direct_module_dependencies(${target}Private qt_for_private)

        # FooPrivate depends on Foo, but we must not record this dependency in run_depends.
        get_target_property(config_module_name ${target} _qt_config_module_name)
        list(REMOVE_ITEM qt_for_private ${config_module_name})

        list(APPEND run_dependencies ${qt_for_private})
    endif()

    list(REMOVE_DUPLICATES run_dependencies)

    # If foo-private is a private dependency and foo is a public dependency,
    # we don't have to add foo-private as runtime dependency.
    set(deps_to_remove "")
    foreach(dep IN LISTS run_dependencies)
        if(NOT dep MATCHES "(.*)_private$")
            continue()
        endif()

        # Is foo a public dependency?
        list(FIND arg_PUBLIC_DEPENDENCIES "${CMAKE_MATCH_1}" idx)
        if(idx GREATER -1)
            list(APPEND deps_to_remove "${dep}")
        endif()
    endforeach()
    if(NOT "${deps_to_remove}" STREQUAL "")
        list(REMOVE_ITEM run_dependencies ${deps_to_remove})
    endif()

    list(JOIN run_dependencies " " run_dependencies)
    set("${out_var}" "${run_dependencies}" PARENT_SCOPE)
endfunction()

# Generates module .pri files for consumption by qmake
function(qt_generate_module_pri_file target)
    set(flags INTERNAL_MODULE NO_PRIVATE_MODULE)
    set(options)
    set(multiopts)
    cmake_parse_arguments(arg "${flags}" "${options}" "${multiopts}" ${ARGN})

    qt_internal_module_info(module "${target}")
    set(pri_files)

    set(property_prefix)

    get_target_property(target_type ${target} TYPE)
    set(is_interface_lib FALSE)
    if(target_type STREQUAL "INTERFACE_LIBRARY")
        set(is_interface_lib TRUE)
    endif()
    if(is_interface_lib)
        set(property_prefix "INTERFACE_")
    endif()

    get_target_property(enabled_features "${target}"
                        "${property_prefix}QT_ENABLED_PUBLIC_FEATURES")
    get_target_property(disabled_features "${target}"
                        "${property_prefix}QT_DISABLED_PUBLIC_FEATURES")
    get_target_property(enabled_private_features "${target}"
                        "${property_prefix}QT_ENABLED_PRIVATE_FEATURES")
    get_target_property(disabled_private_features "${target}"
                        "${property_prefix}QT_DISABLED_PRIVATE_FEATURES")
    qt_correct_features(enabled_features "${enabled_features}")
    qt_correct_features(disabled_features "${disabled_features}")
    qt_correct_features(enabled_private_features "${enabled_private_features}")
    qt_correct_features(disabled_private_features "${disabled_private_features}")

    get_target_property(module_internal_config "${target}"
                        "${property_prefix}QT_MODULE_INTERNAL_CONFIG")
    get_target_property(module_pri_extra_content "${target}"
                        "${property_prefix}QT_MODULE_PRI_EXTRA_CONTENT")
    get_target_property(module_ldflags "${target}"
                        "${property_prefix}QT_MODULE_LDFLAGS")
    get_target_property(module_depends "${target}"
                        "${property_prefix}QT_MODULE_DEPENDS")

    foreach(var enabled_features disabled_features enabled_private_features disabled_private_features
            module_internal_config module_pri_extra_content module_ldflags module_depends)
        if(${var} STREQUAL "${var}-NOTFOUND")
            set(${var} "")
        else()
            string (REPLACE ";" " " ${var} "${${var}}")
        endif()
    endforeach()

    list(APPEND module_internal_config v2)

    if(arg_INTERNAL_MODULE)
        list(APPEND module_internal_config internal_module)
    endif()
    get_target_property(target_type ${target} TYPE)
    if (NOT target_type STREQUAL "INTERFACE_LIBRARY")
        get_target_property(is_fw ${target} FRAMEWORK)
        if(is_fw)
            list(APPEND module_internal_config lib_bundle)
        endif()
    endif()
    if(target_type STREQUAL "STATIC_LIBRARY")
       list(APPEND module_internal_config staticlib)
    endif()

    if(QT_FEATURE_ltcg)
        list(APPEND module_internal_config ltcg)
    endif()

    list(JOIN module_internal_config " " joined_module_internal_config)

    get_target_property(config_module_name ${target} _qt_config_module_name)
    get_target_property(qmake_module_config ${target} ${property_prefix}QT_QMAKE_MODULE_CONFIG)
    if (is_interface_lib)
        list(APPEND qmake_module_config "no_link")
    endif()
    if(qmake_module_config)
        string(REPLACE ";" " " module_build_config "${qmake_module_config}")
        set(module_build_config "\nQT.${config_module_name}.CONFIG = ${module_build_config}")
    else()
        set(module_build_config "")
    endif()

    if(is_fw)
        qt_internal_get_framework_info(fw ${target})
        set(framework_base_path "$$QT_MODULE_LIB_BASE")
        set(public_module_includes "${framework_base_path}/${fw_header_dir}")
        set(public_module_frameworks "${framework_base_path}")
        set(private_module_includes "")
        qt_internal_append_include_directories_with_headers_check(${target}
            private_module_includes PRIVATE
            "${framework_base_path}/${fw_private_header_dir} \
${framework_base_path}/${fw_private_module_header_dir}")
    else()
        set(public_module_includes "$$QT_MODULE_INCLUDE_BASE $$QT_MODULE_INCLUDE_BASE/${module}")
        set(public_module_frameworks "")
        set(private_module_includes "")
        qt_internal_append_include_directories_with_headers_check(${target}
            private_module_includes PRIVATE
            "$$QT_MODULE_INCLUDE_BASE/${module_versioned_include_dir} \
$$QT_MODULE_INCLUDE_BASE/${module_versioned_inner_include_dir}")
    endif()

    if(is_interface_lib)
        set(module_name_in_pri "")
    else()
        get_target_property(module_name_in_pri ${target} OUTPUT_NAME)
    endif()

    get_target_property(hasModuleHeaders ${target} _qt_module_has_headers)
    if (NOT hasModuleHeaders)
        unset(public_module_includes)
        unset(private_module_includes)
    endif()

    set(pri_data_cmake_file "qt_lib_${config_module_name}_private.cmake")

    set(config_module_name_base "${config_module_name}")

    if (arg_INTERNAL_MODULE)
        # Internal module pri needs to provide private headers
        set(public_module_includes "${public_module_includes} ${private_module_includes}")
    endif()

    qt_path_join(target_path ${QT_BUILD_DIR} ${INSTALL_MKSPECSDIR}/modules)

    unset(private_module_frameworks)
    if(is_interface_lib)
        set(module_plugin_types "")
    else()
        get_target_property(module_plugin_types ${target} QMAKE_MODULE_PLUGIN_TYPES)
        if(module_plugin_types)
            list(JOIN module_plugin_types " " module_plugin_types)
        else()
            set(module_plugin_types "")
        endif()
    endif()

    set(module_plugin_types_assignment "")
    if(module_plugin_types)
        set(module_plugin_types_assignment
            "\nQT.${config_module_name}.plugin_types = ${module_plugin_types}")
    endif()

    qt_get_direct_module_dependencies(${target} public_module_dependencies_list)
    list(JOIN public_module_dependencies_list " " public_module_dependencies)
    set(public_module_dependencies "${module_depends} ${public_module_dependencies}")

    qt_path_join(pri_file_name "${target_path}" "qt_lib_${config_module_name}.pri")
    list(APPEND pri_files "${pri_file_name}")

    # Don't use $<TARGET_PROPERTY:${target},INTERFACE_COMPILE_DEFINITIONS> genex because that
    # will compute the transitive list of all defines for a module (so Gui would get Core
    #defines too). Instead query just the public defines on the target.
    get_target_property(target_defines "${target}" INTERFACE_COMPILE_DEFINITIONS)

    # We must filter out expressions of the form $<TARGET_PROPERTY:name>, because
    # 1. They cannot be used in file(GENERATE) content.
    # 2. They refer to the consuming target we have no access to here.
    list(FILTER target_defines EXCLUDE REGEX "\\$<TARGET_PROPERTY:[^,>]+>")
    list(JOIN target_defines " " joined_target_defines)

    set(extra_assignments "")
    if(NOT QT_BUILD_SHARED_LIBS AND target STREQUAL Gui)
        set(extra_assignments "\nQT_DEFAULT_QPA_PLUGIN = q${QT_QPA_DEFAULT_PLATFORM}")
    endif()

    # Map the public dependencies of the target to qmake library names.
    get_target_property(dep_targets ${target} INTERFACE_LINK_LIBRARIES)
    qt_internal_map_targets_to_qmake_libs(module_uses ${dep_targets})
    list(JOIN module_uses " " joined_module_uses)

    # Retrieve the public module's runtime dependencies.
    qt_internal_get_module_run_dependencies(public_module_run_dependencies ${target}
        PUBLIC
        PUBLIC_DEPENDENCIES "${public_module_dependencies_list}")

    set(content "QT.${config_module_name}.VERSION = ${PROJECT_VERSION}
QT.${config_module_name}.name = ${module}
QT.${config_module_name}.module = ${module_name_in_pri}
QT.${config_module_name}.libs = $$QT_MODULE_LIB_BASE
QT.${config_module_name}.ldflags = ${module_ldflags}
QT.${config_module_name}.includes = ${public_module_includes}
QT.${config_module_name}.frameworks = ${public_module_frameworks}
QT.${config_module_name}.bins = $$QT_MODULE_BIN_BASE${module_plugin_types_assignment}
QT.${config_module_name}.depends = ${public_module_dependencies}
")
    if(NOT "${public_module_run_dependencies}" STREQUAL "")
        string(APPEND content
            "QT.${config_module_name}.run_depends = ${public_module_run_dependencies}\n")
    endif()
    string(APPEND content
        "QT.${config_module_name}.uses = ${joined_module_uses}
QT.${config_module_name}.module_config = ${joined_module_internal_config}${module_build_config}
QT.${config_module_name}.DEFINES = ${joined_target_defines}
QT.${config_module_name}.enabled_features = ${enabled_features}
QT.${config_module_name}.disabled_features = ${disabled_features}${extra_assignments}
QT_CONFIG += ${enabled_features}
QT_MODULES += ${config_module_name_base}
${module_pri_extra_content}
")
    file(GENERATE OUTPUT "${pri_file_name}" CONTENT "${content}")

    if (NOT arg_NO_PRIVATE_MODULE AND NOT arg_INTERNAL_MODULE)
        set(pri_data_cmake_file "qt_lib_${config_module_name}_private.cmake")
        qt_generate_qmake_libraries_pri_content(${config_module_name} "${CMAKE_CURRENT_BINARY_DIR}"
            ${pri_data_cmake_file})

        set(private_pri_file_name "qt_lib_${config_module_name}_private.pri")

        set(private_module_dependencies "")
        if(NOT is_interface_lib)
            qt_get_direct_module_dependencies(${target}Private private_module_dependencies)
        endif()
        list(JOIN private_module_dependencies " " private_module_dependencies)

        # Private modules always have internal_module config set, as per qmake.
        list(APPEND module_internal_config internal_module)
        list(JOIN module_internal_config " " joined_module_internal_config)

        # Map the public dependencies of the private module to qmake library names.
        get_target_property(dep_targets ${target}Private INTERFACE_LINK_LIBRARIES)
        qt_internal_map_targets_to_qmake_libs(private_module_uses ${dep_targets})
        list(JOIN private_module_uses " " joined_private_module_uses)

        # Retrieve the private module's runtime dependencies.
        qt_internal_get_module_run_dependencies(private_module_run_dependencies ${target}Private
            PUBLIC_DEPENDENCIES "${dep_targets}")

        # Generate a preliminary qt_lib_XXX_private.pri file
        set(content
            "QT.${config_module_name}_private.VERSION = ${PROJECT_VERSION}
QT.${config_module_name}_private.name = ${module}
QT.${config_module_name}_private.module =
QT.${config_module_name}_private.libs = $$QT_MODULE_LIB_BASE
QT.${config_module_name}_private.includes = ${private_module_includes}
QT.${config_module_name}_private.frameworks = ${private_module_frameworks}
QT.${config_module_name}_private.depends = ${private_module_dependencies}
")
        if(NOT "${private_module_run_dependencies}" STREQUAL "")
            string(APPEND content
                "QT.${config_module_name}_private.run_depends = ${private_module_run_dependencies}
")
        endif()
        string(APPEND content
            "QT.${config_module_name}_private.uses = ${joined_private_module_uses}
QT.${config_module_name}_private.module_config = ${joined_module_internal_config}
QT.${config_module_name}_private.enabled_features = ${enabled_private_features}
QT.${config_module_name}_private.disabled_features = ${disabled_private_features}")
        file(GENERATE
            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${private_pri_file_name}"
            CONTENT "${content}")

        if(QT_GENERATOR_IS_MULTI_CONFIG)
            set(configs ${CMAKE_CONFIGURATION_TYPES})
        else()
            set(configs ${CMAKE_BUILD_TYPE})
        endif()
        set(inputs "${CMAKE_CURRENT_BINARY_DIR}/${private_pri_file_name}")
        foreach(cfg ${configs})
            list(APPEND inputs "${CMAKE_CURRENT_BINARY_DIR}/${cfg}/${pri_data_cmake_file}")
        endforeach()

        qt_path_join(private_pri_file_path "${target_path}" "${private_pri_file_name}")
        list(APPEND pri_files "${private_pri_file_path}")

        set(library_prefixes ${CMAKE_SHARED_LIBRARY_PREFIX} ${CMAKE_STATIC_LIBRARY_PREFIX})
        set(library_suffixes
            ${CMAKE_SHARED_LIBRARY_SUFFIX}
            ${CMAKE_EXTRA_SHARED_LIBRARY_SUFFIXES}
            ${CMAKE_STATIC_LIBRARY_SUFFIX})
        if(MSVC)
            set(link_library_flag "-l")
            file(TO_CMAKE_PATH "$ENV{LIB};${CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES}" implicit_link_directories)
        else()
            set(link_library_flag ${CMAKE_LINK_LIBRARY_FLAG})
            set(implicit_link_directories ${CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES})
        endif()
        add_custom_command(
            OUTPUT "${private_pri_file_path}"
            DEPENDS ${inputs}
                    "${QT_CMAKE_DIR}/QtGenerateLibPri.cmake"
                    "${QT_CMAKE_DIR}/QtGenerateLibHelpers.cmake"
            COMMAND ${CMAKE_COMMAND} "-DIN_FILES=${inputs}" "-DOUT_FILE=${private_pri_file_path}"
                    "-DLIBRARY_PREFIXES=${library_prefixes}"
                    "-DLIBRARY_SUFFIXES=${library_suffixes}"
                    "-DLINK_LIBRARY_FLAG=${link_library_flag}"
                    "-DCONFIGS=${configs}"
                    "-DIMPLICIT_LINK_DIRECTORIES=${implicit_link_directories}"
                    -P "${QT_CMAKE_DIR}/QtGenerateLibPri.cmake"
            VERBATIM)
        # add_dependencies has no effect when adding interface libraries. So need to add the
        # '_lib_pri' targets to ALL to make sure that the related rules executed.
        unset(add_pri_target_to_all)
        if(is_interface_lib)
            set(add_pri_target_to_all ALL)
        endif()
        add_custom_target(${target}_lib_pri ${add_pri_target_to_all}
            DEPENDS "${private_pri_file_path}")
        add_dependencies(${target} ${target}_lib_pri)
    endif()

    qt_install(FILES "${pri_files}" DESTINATION ${INSTALL_MKSPECSDIR}/modules)
endfunction()

# Generates qt_ext_XXX.pri files for consumption by qmake
function(qt_generate_3rdparty_lib_pri_file target lib pri_file_var)
    if(NOT lib)
        # Don't write a pri file for projects that don't set QMAKE_LIB_NAME yet.
        return()
    endif()

    if(QT_GENERATOR_IS_MULTI_CONFIG)
        set(configs ${CMAKE_CONFIGURATION_TYPES})
    else()
        set(configs ${CMAKE_BUILD_TYPE})
    endif()

    file(GENERATE
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/qt_ext_${lib}.cmake"
        CONTENT "set(cfg $<CONFIG>)
set(incdir $<TARGET_PROPERTY:${target},INTERFACE_INCLUDE_DIRECTORIES>)
set(defines $<TARGET_PROPERTY:${target},INTERFACE_COMPILE_DEFINITIONS>)
set(libs $<TARGET_FILE:${target}>)
")

    set(inputs "")
    foreach(cfg ${configs})
        list(APPEND inputs "${CMAKE_CURRENT_BINARY_DIR}/${cfg}/qt_ext_${lib}.cmake")
    endforeach()

    qt_path_join(pri_target_path ${QT_BUILD_DIR} ${INSTALL_MKSPECSDIR}/modules)
    qt_path_join(pri_file "${pri_target_path}" "qt_ext_${lib}.pri")
    qt_path_join(qt_build_libdir ${QT_BUILD_DIR} ${INSTALL_LIBDIR})
    add_custom_command(
        OUTPUT "${pri_file}"
        DEPENDS ${inputs} "${QT_CMAKE_DIR}/QtGenerateExtPri.cmake"
        COMMAND ${CMAKE_COMMAND} "-DIN_FILES=${inputs}" "-DOUT_FILE=${pri_file}" -DLIB=${lib}
                "-DCONFIGS=${configs}"
                "-DQT_BUILD_LIBDIR=${qt_build_libdir}"
                -P "${QT_CMAKE_DIR}/QtGenerateExtPri.cmake"
        VERBATIM)
    add_custom_target(${target}_ext_pri DEPENDS "${pri_file}")
    add_dependencies(${target} ${target}_ext_pri)
    set(${pri_file_var} ${pri_file} PARENT_SCOPE)
endfunction()

# Generates qt_plugin_XXX.pri files for consumption by qmake
#
# QT_PLUGIN.XXX.EXTENDS is set to "-" for the following plugin types:
#   - generic
#   - platform, if the plugin is not the default QPA plugin
# Otherwise, this variable is empty.
function(qt_generate_plugin_pri_file target)
    get_target_property(plugin_name ${target} OUTPUT_NAME)
    get_target_property(plugin_type ${target} QT_PLUGIN_TYPE)
    get_target_property(qmake_plugin_type ${target} QT_QMAKE_PLUGIN_TYPE)
    get_target_property(default_plugin ${target} QT_DEFAULT_PLUGIN)
    get_target_property(plugin_class_name ${target} QT_PLUGIN_CLASS_NAME)
    get_target_property(plugin_pri_extra_content ${target} QT_PLUGIN_PRI_EXTRA_CONTENT)

    foreach(var plugin_pri_extra_content)
        if(${var} STREQUAL "${var}-NOTFOUND")
            set(${var} "")
        else()
            string(REPLACE ";" "\n" ${var} "${${var}}")
        endif()
    endforeach()

    set(plugin_extends "")
    if(NOT default_plugin)
        set(plugin_extends "-")
    endif()

    set(plugin_deps "")
    get_target_property(target_deps ${target} _qt_target_deps)
    foreach(dep ${target_deps})
        list(GET dep 0 dep_name)
        qt_get_qmake_module_name(dep_name ${dep_name})
        list(APPEND plugin_deps ${dep_name})
    endforeach()
    list(REMOVE_DUPLICATES plugin_deps)
    list(JOIN plugin_deps " " plugin_deps)

    list(APPEND module_config v2)
    get_target_property(target_type ${target} TYPE)
    if(target_type STREQUAL "STATIC_LIBRARY")
       list(APPEND module_config staticlib)
    endif()
    list(JOIN module_config " " module_config)

    qt_path_join(pri_target_path ${QT_BUILD_DIR} ${INSTALL_MKSPECSDIR}/modules)
    qt_path_join(pri_file "${pri_target_path}" "qt_plugin_${plugin_name}.pri")

    set(content "QT_PLUGIN.${plugin_name}.TYPE = ${qmake_plugin_type}
QT_PLUGIN.${plugin_name}.EXTENDS = ${plugin_extends}
QT_PLUGIN.${plugin_name}.DEPENDS = ${plugin_deps}
QT_PLUGIN.${plugin_name}.CLASS_NAME = ${plugin_class_name}
QT_PLUGIN.${plugin_name}.module_config = ${module_config}
QT_PLUGINS += ${plugin_name}
${plugin_pri_extra_content}"
)

    file(GENERATE OUTPUT "${pri_file}" CONTENT "${content}")

    qt_install(FILES "${pri_file}" DESTINATION "${INSTALL_MKSPECSDIR}/modules")
endfunction()

# Creates mkspecs/qconfig.pri which contains public global features among other things.
function(qt_generate_global_config_pri_file)
    qt_path_join(qconfig_pri_target_path ${PROJECT_BINARY_DIR} ${INSTALL_MKSPECSDIR})
    qt_path_join(qconfig_pri_target_path "${qconfig_pri_target_path}" "qconfig.pri")

    get_target_property(enabled_features GlobalConfig INTERFACE_QT_ENABLED_PUBLIC_FEATURES)
    get_target_property(disabled_features GlobalConfig INTERFACE_QT_DISABLED_PUBLIC_FEATURES)

    qt_correct_features(corrected_enabled_features "${enabled_features}")
    qt_correct_features(corrected_disabled_features "${disabled_features}")

    string (REPLACE ";" " " corrected_enabled_features "${corrected_enabled_features}")
    string (REPLACE ";" " " corrected_disabled_features "${corrected_disabled_features}")

    # Add some required CONFIG entries.
    set(config_entries "")
    if(CMAKE_BUILD_TYPE STREQUAL Debug)
        list(APPEND config_entries "debug")
    elseif(CMAKE_BUILD_TYPE STREQUAL Release)
        list(APPEND config_entries "release")
    endif()
    list(APPEND config_entries "${qt_build_config_type}")
    string (REPLACE ";" " " config_entries "${config_entries}")

    get_target_property(public_config GlobalConfig INTERFACE_QT_QMAKE_PUBLIC_CONFIG)
    get_target_property(qt_public_config GlobalConfig INTERFACE_QT_QMAKE_PUBLIC_QT_CONFIG)
    qt_correct_config(corrected_public_config "${public_config}")
    qt_correct_config(corrected_qt_public_config "${qt_public_config}")
    qt_guess_qmake_build_config(qmake_build_config)
    list(APPEND corrected_qt_public_config ${qmake_build_config})

    list(JOIN corrected_public_config " " public_config_joined)
    list(JOIN corrected_qt_public_config " " qt_public_config_joined)

    set(content "")
    if(GCC OR CLANG AND NOT "${CMAKE_SYSROOT}" STREQUAL "")
        string(APPEND content "!host_build {
    QMAKE_CFLAGS    += --sysroot=\$\$[QT_SYSROOT]
    QMAKE_CXXFLAGS  += --sysroot=\$\$[QT_SYSROOT]
    QMAKE_LFLAGS    += --sysroot=\$\$[QT_SYSROOT]
}
")
    endif()

    if(CMAKE_CROSSCOMPILING)
        qt_internal_get_host_info_var_prefix(host_info_var_prefix)
        string(APPEND content "host_build {
    QT_ARCH = ${${host_info_var_prefix}_ARCH}
    QT_BUILDABI = ${${host_info_var_prefix}_BUILDABI}
    QT_TARGET_ARCH = ${TEST_architecture_arch}
    QT_TARGET_BUILDABI = ${TEST_buildAbi}
} else {
    QT_ARCH = ${TEST_architecture_arch}
    QT_BUILDABI = ${TEST_buildAbi}
    QT_LIBCPP_ABI_TAG = ${TEST_libcppabiTag}
}
")
    else()
        string(APPEND content "QT_ARCH = ${TEST_architecture_arch}
QT_BUILDABI = ${TEST_buildAbi}
QT_LIBCPP_ABI_TAG = ${TEST_libcppabiTag}
")
    endif()

    string(APPEND content "QT.global.enabled_features = ${corrected_enabled_features}
QT.global.disabled_features = ${corrected_disabled_features}
QT.global.disabled_features += release build_all
QT_CONFIG += ${qt_public_config_joined}
CONFIG += ${config_entries} ${public_config_joined}
QT_VERSION = ${PROJECT_VERSION}
QT_MAJOR_VERSION = ${PROJECT_VERSION_MAJOR}
QT_MINOR_VERSION = ${PROJECT_VERSION_MINOR}
QT_PATCH_VERSION = ${PROJECT_VERSION_PATCH}
")

    set(extra_statements "")
    if(QT_NAMESPACE)
        list(APPEND extra_statements "QT_NAMESPACE = ${QT_NAMESPACE}")
    endif()

    if(QT_LIBINFIX)
        list(APPEND extra_statements "QT_LIBINFIX = ${QT_LIBINFIX}")
    endif()

    if(APPLECLANG)
        set(compiler_version_major_var_name "QT_APPLE_CLANG_MAJOR_VERSION")
        set(compiler_version_minor_var_name "QT_APPLE_CLANG_MINOR_VERSION")
        set(compiler_version_patch_var_name "QT_APPLE_CLANG_PATCH_VERSION")
    elseif(IntelLLVM)
        set(compiler_version_major_var_name "QT_INTELLLVM_MAJOR_VERSION")
        set(compiler_version_minor_var_name "QT_INTELLLVM_MINOR_VERSION")
        set(compiler_version_patch_var_name "QT_INTELLLVM_PATCH_VERSION")
    elseif(CLANG)
        set(compiler_version_major_var_name "QT_CLANG_MAJOR_VERSION")
        set(compiler_version_minor_var_name "QT_CLANG_MINOR_VERSION")
        set(compiler_version_patch_var_name "QT_CLANG_PATCH_VERSION")
    elseif(GCC)
        set(compiler_version_major_var_name "QT_GCC_MAJOR_VERSION")
        set(compiler_version_minor_var_name "QT_GCC_MINOR_VERSION")
        set(compiler_version_patch_var_name "QT_GCC_PATCH_VERSION")
    elseif(MSVC)
        set(compiler_version_major_var_name "QT_MSVC_MAJOR_VERSION")
        set(compiler_version_minor_var_name "QT_MSVC_MINOR_VERSION")
        set(compiler_version_patch_var_name "QT_MSVC_PATCH_VERSION")
    endif()

    if(compiler_version_major_var_name)
        list(APPEND extra_statements
            "${compiler_version_major_var_name} = ${QT_COMPILER_VERSION_MAJOR}")
        list(APPEND extra_statements
            "${compiler_version_minor_var_name} = ${QT_COMPILER_VERSION_MINOR}")
        list(APPEND extra_statements
            "${compiler_version_patch_var_name} = ${QT_COMPILER_VERSION_PATCH}")
    endif()

    if(APPLE)
        list(APPEND extra_statements "QT_MAC_SDK_VERSION = ${QT_MAC_SDK_VERSION}")
        if(NOT CMAKE_SYSTEM_NAME OR CMAKE_SYSTEM_NAME STREQUAL "Darwin")
            # macOS
            list(APPEND extra_statements
                "QMAKE_MACOSX_DEPLOYMENT_TARGET = ${CMAKE_OSX_DEPLOYMENT_TARGET}")
            list(APPEND extra_statements
                 "QT_MAC_SDK_VERSION_MIN = ${QT_SUPPORTED_MIN_MACOS_SDK_VERSION}")
            list(APPEND extra_statements
                 "QT_MAC_SDK_VERSION_MAX = ${QT_SUPPORTED_MAX_MACOS_SDK_VERSION}")
        elseif(CMAKE_SYSTEM_NAME STREQUAL iOS)
            list(APPEND extra_statements
                "QMAKE_IOS_DEPLOYMENT_TARGET = ${CMAKE_OSX_DEPLOYMENT_TARGET}")
            list(APPEND extra_statements
                 "QT_MAC_SDK_VERSION_MIN = ${QT_SUPPORTED_MIN_IOS_SDK_VERSION}")
            list(APPEND extra_statements
                 "QT_MAC_SDK_VERSION_MAX = ${QT_SUPPORTED_MAX_IOS_SDK_VERSION}")
         elseif(CMAKE_SYSTEM_NAME STREQUAL visionOS)
            list(APPEND extra_statements
                "QMAKE_VISIONOS_DEPLOYMENT_TARGET = ${CMAKE_OSX_DEPLOYMENT_TARGET}")
            list(APPEND extra_statements
                 "QT_MAC_SDK_VERSION_MIN = ${QT_SUPPORTED_MIN_VISIONOS_SDK_VERSION}")
            list(APPEND extra_statements
                 "QT_MAC_SDK_VERSION_MAX = ${QT_SUPPORTED_MAX_VISIONOS_SDK_VERSION}")
        endif()

        if (CMAKE_OSX_ARCHITECTURES)
            list(APPEND architectures "${CMAKE_OSX_ARCHITECTURES}")
            string (REPLACE ";" " " architectures "${architectures}")
        else()
            set(architectures "$$QT_ARCH")
        endif()
        list(APPEND extra_statements "QT_ARCHS = ${architectures}")
    endif()

    if(WASM)
        list(APPEND extra_statements
            "QT_EMCC_VERSION = ${EMCC_VERSION}")
    endif()
    if(extra_statements)
        string(REPLACE ";" "\n" extra_statements "${extra_statements}")
        string(APPEND content "\n${extra_statements}\n")
    endif()

    file(GENERATE
        OUTPUT "${qconfig_pri_target_path}"
        CONTENT "${content}"
    )
    qt_install(FILES "${qconfig_pri_target_path}" DESTINATION ${INSTALL_MKSPECSDIR})
endfunction()

# Creates mkspecs/qdevice.pri which contains target device information for cross-builds.
# The content of QT_QMAKE_DEVICE_OPTIONS is written verbatim into qdevice.pri.
function(qt_generate_global_device_pri_file)
    if(NOT CMAKE_CROSSCOMPILING)
        return()
    endif()

    qt_path_join(qdevice_pri_target_path ${PROJECT_BINARY_DIR} ${INSTALL_MKSPECSDIR} "qdevice.pri")

    set(content "")
    foreach(opt ${QT_QMAKE_DEVICE_OPTIONS})
        string(APPEND content "${opt}\n")
    endforeach()

    # Write android specific device info.
    if(ANDROID)
        file(TO_CMAKE_PATH ${ANDROID_SDK_ROOT} ANDROID_SDK_ROOT)
        string(APPEND content "DEFAULT_ANDROID_SDK_ROOT = ${ANDROID_SDK_ROOT}\n")
        file(TO_CMAKE_PATH ${ANDROID_NDK} ANDROID_NDK)
        string(APPEND content "DEFAULT_ANDROID_NDK_ROOT = ${ANDROID_NDK}\n")

        set(android_platform "android-28")
        if(ANDROID_PLATFORM)
            set(android_platform "${ANDROID_PLATFORM}")
        elseif(ANDROID_NATIVE_API_LEVEL)
            set(android_platform "android-${ANDROID_NATIVE_API_LEVEL}")
        endif()
        string(APPEND content "DEFAULT_ANDROID_PLATFORM = ${android_platform}\n")

        string(APPEND content "DEFAULT_ANDROID_NDK_HOST = ${ANDROID_HOST_TAG}\n")

        # TODO: QTBUG-80943 When we eventually support Android multi-abi, this should be changed.
        string(APPEND content "DEFAULT_ANDROID_ABIS = ${ANDROID_ABI}\n")

        if(QT_ANDROID_JAVAC_SOURCE)
            string(APPEND content "ANDROID_JAVAC_SOURCE_VERSION = ${QT_ANDROID_JAVAC_SOURCE}\n")
        endif()
        if(QT_ANDROID_JAVAC_TARGET)
            string(APPEND content "ANDROID_JAVAC_TARGET_VERSION = ${QT_ANDROID_JAVAC_TARGET}\n")
        endif()
    endif()

    if(QT_APPLE_SDK)
        string(APPEND content "QMAKE_MAC_SDK = ${QT_APPLE_SDK}\n")
    endif()

    set(gcc_machine_dump "")
    if(TEST_machine_tuple)
        set(gcc_machine_dump "${TEST_machine_tuple}")
    endif()
    string(APPEND content "GCC_MACHINE_DUMP = ${gcc_machine_dump}\n")

    file(GENERATE OUTPUT "${qdevice_pri_target_path}" CONTENT "${content}")
    qt_install(FILES "${qdevice_pri_target_path}" DESTINATION ${INSTALL_MKSPECSDIR})
endfunction()

function(qt_get_build_parts out_var)
    set(parts "libs")

    if(QT_BUILD_EXAMPLES AND QT_BUILD_EXAMPLES_BY_DEFAULT)
        list(APPEND parts "examples")
    endif()

    if(QT_BUILD_TESTS AND QT_BUILD_TESTS_BY_DEFAULT)
        list(APPEND parts "tests")
    endif()

    if(NOT CMAKE_CROSSCOMPILING OR QT_FORCE_BUILD_TOOLS)
        list(APPEND parts "tools")
    endif()

    set(${out_var} ${parts} PARENT_SCOPE)
endfunction()

# Creates mkspecs/qmodule.pri which contains private global features among other things.
function(qt_generate_global_module_pri_file)
    qt_path_join(qmodule_pri_target_path ${PROJECT_BINARY_DIR} ${INSTALL_MKSPECSDIR})
    qt_path_join(qmodule_pri_target_path "${qmodule_pri_target_path}" "qmodule.pri")

    get_target_property(enabled_features GlobalConfig INTERFACE_QT_ENABLED_PRIVATE_FEATURES)
    get_target_property(disabled_features GlobalConfig INTERFACE_QT_DISABLED_PRIVATE_FEATURES)

    qt_correct_features(corrected_enabled_features "${enabled_features}")
    qt_correct_features(corrected_disabled_features "${disabled_features}")

    string (REPLACE ";" " " corrected_enabled_features "${corrected_enabled_features}")
    string (REPLACE ";" " " corrected_disabled_features "${corrected_disabled_features}")

    set(corrected_private_config "")
    get_target_property(private_config GlobalConfig INTERFACE_QT_QMAKE_PRIVATE_CONFIG)
    qt_correct_config(corrected_private_config "${private_config}")
    list(JOIN corrected_private_config " " private_config_joined)

    set(content "")
    if(DEFINED QT_EXTRA_DEFINES)
        list(JOIN QT_EXTRA_DEFINES " " value)
        string(APPEND content "EXTRA_DEFINES += ${value}\n")
    endif()
    if(DEFINED QT_EXTRA_INCLUDEPATHS)
        qt_to_qmake_path_list(value ${QT_EXTRA_INCLUDEPATHS})
        string(APPEND content "EXTRA_INCLUDEPATH += ${value}\n")
    endif()
    if(DEFINED QT_EXTRA_LIBDIRS)
        qt_to_qmake_path_list(value ${QT_EXTRA_LIBDIRS})
        string(APPEND content "EXTRA_LIBDIR += ${value}\n")
    endif()
    if(DEFINED QT_EXTRA_FRAMEWORKPATHS)
        qt_to_qmake_path_list(value ${QT_EXTRA_FRAMEWORKPATHS})
        string(APPEND content "EXTRA_FRAMEWORKPATH += ${value}\n")
    endif()

    set(arch "${TEST_architecture_arch}")
    list(JOIN TEST_subarch_result " " subarchs)
    if(CMAKE_CROSSCOMPILING)
        qt_internal_get_host_info_var_prefix(host_info_var_prefix)
        set(host_arch "${${host_info_var_prefix}_ARCH}")
        list(JOIN ${host_info_var_prefix}_SUBARCHS " " host_subarchs)
        string(APPEND content "host_build {
    QT_CPU_FEATURES.${host_arch} = ${host_subarchs}
} else {
    QT_CPU_FEATURES.${arch} = ${subarchs}
}
")
    else()
        string(APPEND content "QT_CPU_FEATURES.${arch} = ${subarchs}\n")
    endif()

    string(APPEND content "QT.global_private.enabled_features = ${corrected_enabled_features}
QT.global_private.disabled_features = ${corrected_disabled_features}
CONFIG += ${private_config_joined}
")
    if(PKG_CONFIG_FOUND)
        string(APPEND content "PKG_CONFIG_EXECUTABLE = ${PKG_CONFIG_EXECUTABLE}\n")
    endif()

    string(APPEND content "QT_COORD_TYPE = ${QT_COORD_TYPE}\n")

    qt_get_build_parts(build_parts)
    string(REPLACE ";" " " build_parts "${build_parts}")
    string(APPEND content "QT_BUILD_PARTS = ${build_parts}\n")

    if(QT_EXTRA_RPATHS)
        list(JOIN QT_EXTRA_RPATHS " " extra_rpaths)
        string(APPEND content "EXTRA_RPATHS += ${extra_rpaths}")
    endif()

    set(preliminary_pri_root "${CMAKE_CURRENT_BINARY_DIR}/mkspecs/preliminary")
    set(pri_data_cmake_file "qmodule.cmake")
    qt_generate_qmake_libraries_pri_content(global ${preliminary_pri_root} ${pri_data_cmake_file})

    # Generate a preliminary qmodule.pri file
    set(preliminary_pri_file_path "${preliminary_pri_root}/qmodule.pri")
    file(GENERATE
        OUTPUT ${preliminary_pri_file_path}
        CONTENT "${content}"
    )

    if(QT_GENERATOR_IS_MULTI_CONFIG)
        set(configs ${CMAKE_CONFIGURATION_TYPES})
    else()
        set(configs ${CMAKE_BUILD_TYPE})
    endif()
    set(inputs ${preliminary_pri_file_path})
    foreach(cfg ${configs})
        list(APPEND inputs "${preliminary_pri_root}/${cfg}/${pri_data_cmake_file}")
    endforeach()

    set(library_prefixes ${CMAKE_SHARED_LIBRARY_PREFIX} ${CMAKE_STATIC_LIBRARY_PREFIX})
    set(library_suffixes
        ${CMAKE_SHARED_LIBRARY_SUFFIX}
        ${CMAKE_EXTRA_SHARED_LIBRARY_SUFFIXES}
        ${CMAKE_STATIC_LIBRARY_SUFFIX})
    if(MSVC)
        set(link_library_flag "-l")
        file(TO_CMAKE_PATH "$ENV{LIB};${CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES}" implicit_link_directories)
    else()
        set(link_library_flag ${CMAKE_LINK_LIBRARY_FLAG})
        set(implicit_link_directories ${CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES})
    endif()
    add_custom_command(
        OUTPUT "${qmodule_pri_target_path}"
        DEPENDS ${inputs}
                "${QT_CMAKE_DIR}/QtGenerateLibPri.cmake"
                "${QT_CMAKE_DIR}/QtGenerateLibHelpers.cmake"
        COMMAND ${CMAKE_COMMAND} "-DIN_FILES=${inputs}" "-DOUT_FILE=${qmodule_pri_target_path}"
                "-DLIBRARY_PREFIXES=${library_prefixes}"
                "-DLIBRARY_SUFFIXES=${library_suffixes}"
                "-DLINK_LIBRARY_FLAG=${link_library_flag}"
                "-DCONFIGS=${configs}"
                "-DIMPLICIT_LINK_DIRECTORIES=${implicit_link_directories}"
                -P "${QT_CMAKE_DIR}/QtGenerateLibPri.cmake"
        VERBATIM)
    add_custom_target(qmodule_pri DEPENDS "${qmodule_pri_target_path}")
    qt_install(FILES "${qmodule_pri_target_path}" DESTINATION ${INSTALL_MKSPECSDIR})
endfunction()

function(qt_correct_features out_var features)
    set(corrected_features "")
    foreach(feature ${features})
        get_property(feature_original_name GLOBAL PROPERTY "QT_FEATURE_ORIGINAL_NAME_${feature}")
        list(APPEND corrected_features "${feature_original_name}")
    endforeach()
    set(${out_var} ${corrected_features} PARENT_SCOPE)
endfunction()

# Get original names for config values (which correspond to feature names) and use them if they
# exist, otherwise just use the config value (which might be the case when a config value has
# a custom name).
function(qt_correct_config out_var config)
    set(corrected_config "")
    foreach(name ${config})
        # Is the config value a known feature?
        get_property(feature_original_name GLOBAL PROPERTY "QT_FEATURE_ORIGINAL_NAME_${name}")
        if(feature_original_name)
            list(APPEND corrected_config "${feature_original_name}")
            continue()
        endif()

        # Is the config value a negated known feature, e.g. no_foo?
        # Then add the config value no-foo.
        if(name MATCHES "^no_(.*)")
            get_property(feature_original_name GLOBAL PROPERTY
                "QT_FEATURE_ORIGINAL_NAME_${CMAKE_MATCH_1}")
            if(feature_original_name)
                list(APPEND corrected_config "no-${feature_original_name}")
                continue()
            endif()
        endif()

        # The config value is no known feature. Add the value as is.
        list(APPEND corrected_config "${name}")
    endforeach()
    set(${out_var} ${corrected_config} PARENT_SCOPE)
endfunction()
