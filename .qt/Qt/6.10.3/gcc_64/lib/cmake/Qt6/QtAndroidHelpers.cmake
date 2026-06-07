# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#
# Android specific functions/macros/properties required for building Qt Modules
#

macro(qt_internal_setup_android_target_properties)
    define_property(TARGET
        PROPERTY
            QT_ANDROID_MODULE_INSTALL_DIR
        BRIEF_DOCS
            "Recorded install location for a Qt Module."
        FULL_DOCS
            "Recorded install location for a Qt Module. Used by qt_internal_android_dependencies()."
    )

    define_property(TARGET
        PROPERTY
            QT_ANDROID_JAR_DEPENDENCIES
        BRIEF_DOCS
            "Qt Module Jar dependencies list."
        FULL_DOCS
            "Qt Module Jar dependencies list."
    )

    define_property(TARGET
        PROPERTY
            QT_ANDROID_BUNDLED_JAR_DEPENDENCIES
        BRIEF_DOCS
            "Qt Module Jars that should be bundled with it during packing."
        FULL_DOCS
            "Qt Module Jars that should be bundled with it during packing."
    )

    define_property(TARGET
        PROPERTY
            QT_ANDROID_LIB_DEPENDENCIES
        BRIEF_DOCS
            "Qt Module C++ libraries that should be bundled with it during packing."
        FULL_DOCS
            "Qt Module C++ libraries that should be bundled with it during packing."
    )

    define_property(TARGET
        PROPERTY
            QT_ANDROID_LIB_DEPENDENCY_REPLACEMENTS
        BRIEF_DOCS
            "Qt Module C++ libraries that can replace libraries declared with the QT_ANDROID_LIB_DEPENDENCIES property."
        FULL_DOCS
            "Qt Module C++ libraries that can replace libraries declared with the QT_ANDROID_LIB_DEPENDENCIES property."
    )

    define_property(TARGET
        PROPERTY
            QT_ANDROID_BUNDLED_FILES
        BRIEF_DOCS
            "Qt Module files that need to be bundled during packing."
        FULL_DOCS
            "Qt Module files that need to be bundled during packing."
    )

    define_property(TARGET
        PROPERTY
            QT_ANDROID_PERMISSIONS
        BRIEF_DOCS
            "Qt Module android permission list."
        FULL_DOCS
            "Qt Module android permission list."
    )

    define_property(TARGET
        PROPERTY
            QT_ANDROID_FEATURES
        BRIEF_DOCS
            "Qt Module android feature list."
        FULL_DOCS
            "Qt Module android feature list."
    )

    define_property(TARGET
        PROPERTY
            QT_ANDROID_ABIS
        BRIEF_DOCS
            "List of ABIs that the target packages are built with."
        FULL_DOCS
            "List of ABIs that the target packages are built with."
    )
endmacro()

function(qt_internal_locate_qt_android_base_jar out_var)

    set(datadir "${INSTALL_DATADIR}")
    if(NOT DEFINED datadir OR datadir STREQUAL "")
        set(datadir ".")
    endif()
    set(qt_ns "${QT_CMAKE_EXPORT_NAMESPACE}")
    qt_path_join(
        jar
        "${QT_TOOLCHAIN_RELOCATABLE_INSTALL_PREFIX}"
        "${datadir}"
        "jar"
        "${qt_ns}Android.jar"
    )

    # Optional override
    if(DEFINED ENV{QT_ANDROID_JAR_PATH} AND EXISTS "$ENV{QT_ANDROID_JAR_PATH}")
        set(jar "$ENV{QT_ANDROID_JAR_PATH}")
    endif()

    set(${out_var} "${jar}" PARENT_SCOPE)
endfunction()

function(qt_internal_compute_android_javadoc_classpath out_var)

    if(CMAKE_HOST_WIN32)
        set(sep ";")
    else()
        set(sep ":")
    endif()

    # The target Qt6Android is already defined = use its build-dir jar.
    # The target may not yet exist in some qtbase configuration order =
    # still set the jar path in advance.
    set(qt_ns "${QT_CMAKE_EXPORT_NAMESPACE}")
    if(TARGET ${qt_ns}Android OR (QT_BUILDING_QT AND PROJECT_NAME STREQUAL "QtBase"))
        set(jar "${QT_BUILD_DIR}/jar/${qt_ns}Android.jar")
        set(${out_var} "${QT_ANDROID_JAR}${sep}${jar}" PARENT_SCOPE)
        return()
    endif()

    # Downstream module: use installed jar from the shared locator
    qt_internal_locate_qt_android_base_jar(qt_classes_jar)
    if(NOT EXISTS "${qt_classes_jar}")
        message(FATAL_ERROR
            "Qt Android JAR not found at:\n  ${qt_classes_jar}")
    endif()

    set(${out_var} "${QT_ANDROID_JAR}${sep}${qt_classes_jar}" PARENT_SCOPE)
endfunction()

function(qt_internal_write_android_javadoc_args
                    response_file
                    output_dir
                    source_paths
                    package_names_space_separated)

    qt_internal_compute_android_javadoc_classpath(class_path)

    file(CONFIGURE
        OUTPUT  "${response_file}"
        CONTENT "${package_names_space_separated}
--class-path \"${class_path}\"
-d \"${output_dir}\"
--source-path \"${source_paths}\"
"
    )
endfunction()

function(qt_internal_add_android_permission target)
    _qt_internal_add_android_permission(${ARGV})
endfunction()

function(qt_internal_android_dependencies_content target file_content_out)
    get_target_property(arg_JAR_DEPENDENCIES ${target} QT_ANDROID_JAR_DEPENDENCIES)
    get_target_property(arg_BUNDLED_JAR_DEPENDENCIES ${target} QT_ANDROID_BUNDLED_JAR_DEPENDENCIES)
    get_target_property(arg_LIB_DEPENDENCIES ${target} QT_ANDROID_LIB_DEPENDENCIES)
    get_target_property(arg_LIB_DEPENDENCY_REPLACEMENTS ${target} QT_ANDROID_LIB_DEPENDENCY_REPLACEMENTS)
    get_target_property(arg_BUNDLED_FILES ${target} QT_ANDROID_BUNDLED_FILES)
    get_target_property(arg_PERMISSIONS ${target} QT_ANDROID_PERMISSIONS)
    get_target_property(arg_FEATURES ${target} QT_ANDROID_FEATURES)

    if ((NOT arg_JAR_DEPENDENCIES)
        AND (NOT arg_BUNDLED_JAR_DEPENDENCIES)
        AND (NOT arg_LIB_DEPENDENCIES)
        AND (NOT arg_LIB_DEPENDENCY_REPLACEMENTS)
        AND (NOT arg_BUNDLED_FILES)
        AND (NOT arg_PERMISSIONS)
        AND (NOT arg_FEATURES))
        # None of the values were set, so there's nothing to do
        return()
    endif()

    # mimic qmake's section and string splitting from
    # mkspecs/feature/qt_android_deps.prf
    macro(section string delimiter first second)
        string(FIND ${string} ${delimiter} delimiter_location)
        if (NOT ${delimiter_location} EQUAL -1)
            string(SUBSTRING ${string} 0 ${delimiter_location} ${first})
            math(EXPR delimiter_location "${delimiter_location} + 1")
            string(SUBSTRING ${string} ${delimiter_location} -1 ${second})
        else()
            set(${first} ${string})
            set(${second} "")
        endif()
    endmacro()

    set(file_contents "")

    # Jar Dependencies
    if(arg_JAR_DEPENDENCIES)
        foreach(jar_dependency IN LISTS arg_JAR_DEPENDENCIES)
            section(${jar_dependency} ":" jar_file init_class)
            # Use unix path to allow using files on any host platform.
            file(TO_CMAKE_PATH ${jar_file} jar_file_unix_path)
            string(APPEND file_contents "<jar file=\"${jar_file_unix_path}\" />\n")
        endforeach()
    endif()

    # Bundled Jar Dependencies
    if(arg_BUNDLED_JAR_DEPENDENCIES)
        foreach(jar_bundle IN LISTS arg_BUNDLED_JAR_DEPENDENCIES)
            section(${jar_bundle} ":" bundle_file init_class)
            # Use unix path to allow using files on any host platform.
            file(TO_CMAKE_PATH ${bundle_file} jar_bundle_unix_path)
            string(APPEND file_contents
                   "<jar bundling=\"1\" file=\"${jar_bundle_unix_path}\" />\n")
        endforeach()
    endif()

    # Lib Dependencies
    if(arg_LIB_DEPENDENCIES)
        foreach(lib IN LISTS arg_LIB_DEPENDENCIES)
            string(REPLACE ".so" "_${CMAKE_ANDROID_ARCH_ABI}.so" lib ${lib})
            section(${lib} ":" lib_file lib_extends)
            if (lib_extends)
                set(lib_extends "extends=\"${lib_extends}\"")
            endif()
            # Use unix path to allow using files on any host platform.
            file(TO_CMAKE_PATH ${lib_file} lib_file_unix_path)
            string(APPEND file_contents "<lib file=\"${lib_file_unix_path}\" ${lib_extends} />\n")
        endforeach()
    endif()

    # Lib Dependencies Replacements
    if(arg_LIB_DEPENDENCY_REPLACEMENTS)
        foreach(lib IN LISTS arg_LIB_DEPENDENCY_REPLACEMENTS)
            string(REPLACE ".so" "_${CMAKE_ANDROID_ARCH_ABI}.so" lib ${lib})
            section(${lib} ":" lib_file lib_replacement)
            if (lib_replacement)
                # Use unix path to allow using files on any host platform.
                file(TO_CMAKE_PATH ${lib_replacement} lib_replacement_unix_path)
                set(lib_replacement "replaces=\"${lib_replacement_unix_path}\"")
            endif()
            # Use unix path to allow using files on any host platform.
            file(TO_CMAKE_PATH ${lib_file} lib_file_unix_path)
            string(APPEND file_contents
                   "<lib file=\"${lib_file_unix_path}\" ${lib_replacement} />\n")
        endforeach()
    endif()

    # Bundled files
    if(arg_BUNDLED_FILES)
        foreach(bundled_file IN LISTS arg_BUNDLED_FILES)
            # Use unix path to allow using files on any host platform.
            file(TO_CMAKE_PATH ${bundled_file} file_unix_path)
            string(APPEND file_contents "<bundled file=\"${file_unix_path}\" />\n")
        endforeach()
    endif()

    # Android Permissions
    if(arg_PERMISSIONS)
        foreach(permission IN LISTS arg_PERMISSIONS)
            # Check if the permission has also extra attributes in addition to the permission name
            list(LENGTH permission permission_len)
            if(permission_len EQUAL 1)
                string(APPEND file_contents "<permission name=\"${permission}\" />\n")
            elseif(permission_len EQUAL 2)
                list(GET permission 0 name)
                list(GET permission 1 extras)
                string(APPEND file_contents "<permission name=\"${name}\" extras=\"${extras}\"/>\n")
            else()
                message(FATAL_ERROR "Invalid permission format: ${permission} ${permission_len}")
            endif()
        endforeach()
    endif()

    # Android Features
    if(arg_FEATURES)
        foreach(feature IN LISTS arg_FEATURES)
            string(APPEND file_contents "<feature name=\"${feature}\" />\n")
        endforeach()
    endif()

    set(${file_content_out} ${file_contents} PARENT_SCOPE)
endfunction()

# Generate Qt Module -android-dependencies.xml required by the
# androiddeploytoolqt to successfully copy all the plugins and other dependent
# items into the APK
function(qt_internal_android_dependencies target)
    get_target_property(target_type "${target}" TYPE)
    if(target_type STREQUAL "INTERFACE_LIBRARY")
        return()
    endif()

    # Get plugins for the current module
    get_target_property(module_plugin_types ${target} MODULE_PLUGIN_TYPES)

    # Get depends for the current module
    qt_internal_android_dependencies_content(${target} file_contents)

    # Get plugins from the module's plugin types and get their dependencies
    foreach(plugin ${QT_KNOWN_PLUGINS})
        get_target_property(iter_known_plugin_type ${plugin} QT_PLUGIN_TYPE)
        foreach(plugin_type ${module_plugin_types})
            if (plugin_type STREQUAL iter_known_plugin_type)
                qt_internal_android_dependencies_content(${plugin} plugin_file_contents)
                string(APPEND file_contents ${plugin_file_contents})
            endif()
        endforeach()
    endforeach()

    if ((NOT module_plugin_types)
        AND (NOT file_contents))
        # None of the values were set, so there's nothing to do
        return()
    endif()

    get_target_property(target_output_name ${target} OUTPUT_NAME)
    if (NOT target_output_name)
        set(target_name ${target})
    else()
        set(target_name ${target_output_name})
    endif()

    string(PREPEND file_contents "<lib name=\"${target_name}_${CMAKE_ANDROID_ARCH_ABI}\"><depends>\n")
    string(PREPEND file_contents "<rules><dependencies>\n")

    # Module plugins
    if(module_plugin_types)
        foreach(plugin IN LISTS module_plugin_types)
            string(APPEND file_contents
                "<bundled file=\"${INSTALL_PLUGINSDIR}/${plugin}\" type=\"plugin_dir\"/>\n")
        endforeach()
    endif()

    string(APPEND file_contents "</depends></lib>\n")
    string(APPEND file_contents "</dependencies></rules>")

    qt_path_join(dependency_file "${QT_BUILD_DIR}" "${INSTALL_LIBDIR}" "${target_name}_${CMAKE_ANDROID_ARCH_ABI}-android-dependencies.xml")
    file(WRITE ${dependency_file} ${file_contents})

    get_target_property(target_install_dir ${target} QT_ANDROID_MODULE_INSTALL_DIR)
    if (NOT target_install_dir)
        message(SEND_ERROR "qt_internal_android_dependencies: Target ${target} is either not a Qt Module or has no recorded install location")
        return()
    endif()

    # Copy file into install directory, required by the androiddeployqt tool.
    qt_install(FILES
        ${dependency_file}
        DESTINATION
            ${target_install_dir}
        COMPONENT
            Devel)
endfunction()

function(qt_internal_set_up_build_host_java_docs)
    if("${ANDROID_SDK_ROOT}" STREQUAL "")
        message(FATAL_ERROR
            "QT_HOST_DOCUMENT_JAVA_SOURCES=ON requires setting ANDROID_SDK_ROOT."
        )
    endif()

    _qt_internal_locate_android_jar()
    set(QT_ANDROID_JAR "${QT_ANDROID_JAR}" PARENT_SCOPE)
    set(QT_ANDROID_API_USED_FOR_JAVA "${QT_ANDROID_API_USED_FOR_JAVA}" PARENT_SCOPE)
endfunction()

# Collect the Java source files that were recorded by qt_internal_add_jar.
# If we're not building for Android, qt_internal_add_jar is not called, and we simple collect
# all java files under the current directory.
function(qt_internal_collect_jar_sources out_var)
    if(NOT ANDROID)
        file(GLOB_RECURSE sources LIST_DIRECTORIES FALSE "*.java")
        set("${out_var}" "${sources}" PARENT_SCOPE)
        return()
    endif()

    set(no_value_options "")
    set(single_value_options DIRECTORY)
    set(multi_value_options "")
    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )
    _qt_internal_validate_all_args_are_parsed(arg)

    set(directory_arg "")
    if(DEFINED arg_DIRECTORY)
        set(directory_arg DIRECTORY ${arg_DIRECTORY})
    endif()

    get_directory_property(result ${directory_arg} _qt_jar_sources)
    get_directory_property(subdirs ${directory_arg} SUBDIRECTORIES)
    foreach(subdir IN LISTS subdirs)
        qt_internal_collect_jar_sources(subdir_result DIRECTORY ${subdir})
        if(NOT "${subdir_result}" STREQUAL "")
            list(APPEND result ${subdir_result})
        endif()
    endforeach()
    set("${out_var}" "${result}" PARENT_SCOPE)
endfunction()

function(qt_internal_add_javadoc_target)
    set(no_value_options "")
    set(single_value_options
        MODULE
        OUTPUT_DIR
    )
    set(multi_value_options
        SOURCES
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )
    _qt_internal_validate_all_args_are_parsed(arg)

    if(TARGET ${arg_MODULE})
        get_target_property(skip ${arg_MODULE} _qt_skip_javadoc)
        if(skip)
            message(VERBOSE "Skipping generation of Android HTML docs for ${arg_MODULE}.")
            return()
        endif()
    endif()

    # Collect source directories from source file paths.
    set(source_dirs "")
    foreach(source_path IN LISTS arg_SOURCES)
        get_filename_component(dir_path "${source_path}" DIRECTORY)
        list(APPEND source_dirs "${dir_path}")
    endforeach()
    list(REMOVE_DUPLICATES source_dirs)

    # Retrieve package names from source dirs.
    set(package_names "")
    foreach(source_dir IN LISTS source_dirs)
        string(REGEX MATCH "/(org/qtproject/qt/android(/.*|$))" package_dir "${source_dir}")
        if(package_dir STREQUAL "")
            message(VERBOSE "Java source dir is not a package directory: ${source_dir}")
            continue()
        endif()

        # Store package_dir without leading slash.
        set(package_dir "${CMAKE_MATCH_1}")

        # Use dots instead of slashes for the package name.
        string(REPLACE "/" "." package_name "${package_dir}")

        list(APPEND package_names "${package_name}")
    endforeach()

    # Strip package paths from the source dirs.
    list(TRANSFORM source_dirs REPLACE "/org/qtproject/qt/android.*" "")
    list(REMOVE_DUPLICATES source_dirs)

    # Use the correct separator for the --source-path argument.
    if(NOT CMAKE_HOST_WIN32)
        string(REPLACE ";" ":" source_dirs "${source_dirs}")
    endif()

    # Use a response file to avoid quoting issues with the space-separated package names.
    set(javadoc_output_dir "${arg_OUTPUT_DIR}/android")
    set(response_file "${CMAKE_CURRENT_BINARY_DIR}/doc/.javadocargs")
    string(REPLACE ";" " " package_names_space_separated "${package_names}")

    set(module ${arg_MODULE})
    set(javadoc_target android_html_docs_${module})

    # Write the args file
    qt_internal_write_android_javadoc_args(
        "${response_file}"
        "${javadoc_output_dir}"
        "${source_dirs}"
        "${package_names_space_separated}"
    )

    add_custom_target(${javadoc_target}
        COMMAND ${Java_JAVADOC_EXECUTABLE} "@${response_file}"
        COMMENT "Generating Java documentation"
        VERBATIM
    )

    set(qt_ns "${QT_CMAKE_EXPORT_NAMESPACE}")
    if(QT_BUILDING_QT OR PROJECT_NAME STREQUAL "QtBase")
        if(TARGET ${qt_ns}Android)
            add_dependencies(${javadoc_target} ${qt_ns}Android)
        endif()
    endif()

    add_dependencies(docs_android ${javadoc_target})

    if (QT_WILL_INSTALL)
        install(DIRECTORY "${arg_OUTPUT_DIR}/"
            DESTINATION "${INSTALL_DOCDIR}/${module}"
            COMPONENT _install_docs_android_${module}
            EXCLUDE_FROM_ALL
        )

        add_custom_target(install_docs_android_${module}
            COMMAND ${CMAKE_COMMAND}
                --install "${CMAKE_BINARY_DIR}"
                --component _install_docs_android_${module}
            COMMENT "Installing Android html docs for ${module}"
        )
    else()
        add_custom_target(install_docs_android_${module})
    endif()

    add_dependencies(install_docs_android_${module} ${javadoc_target})
    add_dependencies(install_docs_android install_docs_android_${module})
endfunction()

function(qt_internal_create_source_jar)
    set(no_value_options "")
    set(single_value_options MODULE)
    set(multi_value_options SOURCES)
    cmake_parse_arguments(PARSE_ARGV 0 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )
    _qt_internal_validate_all_args_are_parsed(arg)

    set(module ${arg_MODULE})
    set(jar_target android_source_jar_${module})
    set(jar_name ${CMAKE_INSTALL_NAMESPACE}AndroidSources${module})

    qt_internal_locate_qt_android_base_jar(qt_classes_jar)

    set(include_jars "${QT_ANDROID_JAR}")
    if(EXISTS "${qt_classes_jar}")
        list(APPEND include_jars "${qt_classes_jar}")
    endif()

    add_jar(${jar_target}
        SOURCES ${arg_SOURCES}
        VERSION ${PROJECT_VERSION}
        INCLUDE_JARS ${include_jars}
        OUTPUT_NAME ${jar_name}
    )
    set_target_properties(${jar_target} PROPERTIES EXCLUDE_FROM_ALL ON)
    add_dependencies(android_source_jars ${jar_target})

    if(QT_WILL_INSTALL)
        qt_path_join(destination "${INSTALL_DATADIR}" "android" "${module}")
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${jar_name}-${PROJECT_VERSION}.jar"
            DESTINATION "${destination}"
            COMPONENT _install_android_source_jar_${module}
            EXCLUDE_FROM_ALL
        )
        add_custom_target(install_android_source_jar_${module}
            COMMAND ${CMAKE_COMMAND}
                --install "${CMAKE_BINARY_DIR}"
                --component _install_android_source_jar_${module}
            COMMENT "Installing Android source jar for ${module}"
        )
    else()
        add_custom_target(install_android_source_jar_${module})
    endif()

    add_dependencies(install_android_source_jar_${module} ${jar_target})
    add_dependencies(install_android_source_jars install_android_source_jar_${module})
endfunction()
