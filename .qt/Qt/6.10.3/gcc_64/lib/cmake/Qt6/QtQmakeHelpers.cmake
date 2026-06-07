# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Create a QMake list (values space-separated) containing paths.
# Entries that contain whitespace characters are quoted.
function(qt_to_qmake_path_list out_var)
    set(quoted_paths "")
    foreach(path ${ARGN})
        if(path MATCHES "[ \t]")
            list(APPEND quoted_paths "\"${path}\"")
        else()
            list(APPEND quoted_paths "${path}")
        endif()
    endforeach()
    list(JOIN quoted_paths " " result)
    set("${out_var}" "${result}" PARENT_SCOPE)
endfunction()


function(qt_generate_qconfig_cpp in_file out_file)
    set(QT_CONFIG_STRS "")

    string(APPEND QT_CONFIG_STRS "    R\"qconfig(${INSTALL_DOCDIR})qconfig\",\n")
    string(APPEND QT_CONFIG_STRS "    R\"qconfig(${INSTALL_INCLUDEDIR})qconfig\",\n")
    string(APPEND QT_CONFIG_STRS "    R\"qconfig(${INSTALL_LIBDIR})qconfig\",\n")
    string(APPEND QT_CONFIG_STRS "    R\"qconfig(${INSTALL_LIBEXECDIR})qconfig\",\n")
    string(APPEND QT_CONFIG_STRS "    R\"qconfig(${INSTALL_BINDIR})qconfig\",\n")
    string(APPEND QT_CONFIG_STRS "    R\"qconfig(${INSTALL_PLUGINSDIR})qconfig\",\n")
    string(APPEND QT_CONFIG_STRS "    R\"qconfig(${INSTALL_QMLDIR})qconfig\",\n")
    string(APPEND QT_CONFIG_STRS "    R\"qconfig(${INSTALL_ARCHDATADIR})qconfig\",\n")
    string(APPEND QT_CONFIG_STRS "    R\"qconfig(${INSTALL_DATADIR})qconfig\",\n")
    string(APPEND QT_CONFIG_STRS "    R\"qconfig(${INSTALL_TRANSLATIONSDIR})qconfig\",\n")
    string(APPEND QT_CONFIG_STRS "    R\"qconfig(${INSTALL_EXAMPLESDIR})qconfig\",\n")
    string(APPEND QT_CONFIG_STRS "    R\"qconfig(${INSTALL_TESTSDIR})qconfig\"")

    # Settings path / sysconf dir.
    set(QT_SYS_CONF_DIR "${INSTALL_SYSCONFDIR}")

    # Compute and set relocation prefixes.
    if(WIN32)
        set(lib_location_absolute_path
            "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${INSTALL_BINDIR}")
    else()
        set(lib_location_absolute_path
            "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${INSTALL_LIBDIR}")
    endif()
    file(RELATIVE_PATH from_lib_location_to_prefix
         "${lib_location_absolute_path}" "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}")
    set(QT_CONFIGURE_LIBLOCATION_TO_PREFIX_PATH "${from_lib_location_to_prefix}")

    # Ensure Windows drive letter is prepended to the install prefix hardcoded
    # into qconfig.cpp, otherwise qmake can't find Qt modules in a static Qt
    # build if there's no qt.conf. Mostly relevant for CI.
    # Given input like
    #    \work/qt/install
    # or
    #    \work\qt\install
    # Expected output is something like
    #   C:/work/qt/install
    # so it includes a drive letter and forward slashes.
    if(QT_FEATURE_relocatable)
        # A relocatable Qt does not need a hardcoded prefix path.
        # This makes reproducible builds a closer reality, because we don't embed a CI path
        # into the binaries.
        set(QT_CONFIGURE_PREFIX_PATH_STR "")
    else()
        set(QT_CONFIGURE_PREFIX_PATH_STR "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}")
        if(CMAKE_HOST_WIN32)
            get_filename_component(
                QT_CONFIGURE_PREFIX_PATH_STR
                "${QT_CONFIGURE_PREFIX_PATH_STR}" REALPATH)
        endif()
    endif()

    configure_file(${in_file} ${out_file} @ONLY)
endfunction()

# In the cross-compiling case, creates a wrapper around the host Qt's
# qmake and qtpaths executables. Also creates a qmake configuration file that sets
# up the host qmake's properties for cross-compiling with this Qt
# build.
function(qt_generate_qmake_and_qtpaths_wrapper_for_target)
    if(NOT CMAKE_CROSSCOMPILING OR QT_NO_GENERATE_QMAKE_WRAPPER_FOR_TARGET)
        return()
    endif()

    # Call the configuration file something else but qt.conf to avoid
    # being picked up by the qmake executable that's created if
    # QT_FORCE_BUILD_TOOLS is enabled.
    qt_path_join(qt_conf_path "${INSTALL_BINDIR}" "target_qt.conf")

    set(prefix "${CMAKE_INSTALL_PREFIX}")
    set(ext_prefix "${QT_STAGING_PREFIX}")
    set(host_prefix "${QT_HOST_PATH}")
    file(RELATIVE_PATH host_prefix_relative_to_conf_file "${ext_prefix}/${INSTALL_BINDIR}"
        "${host_prefix}")
    file(RELATIVE_PATH ext_prefix_relative_to_conf_file "${ext_prefix}/${INSTALL_BINDIR}"
        "${ext_prefix}")
    file(RELATIVE_PATH ext_datadir_relative_to_host_prefix "${host_prefix}"
        "${ext_prefix}/${INSTALL_MKSPECSDIR}/..")

    set(content "")

    # On Android CMAKE_SYSROOT is set, but from qmake's point of view it should not be set, because
    # then qmake generates incorrect Qt module include flags (among other things). Do the same for
    # darwin uikit cross-compilation.
    set(sysroot "")
    if(CMAKE_SYSROOT AND NOT ANDROID AND NOT UIKIT)
        set(sysroot "${CMAKE_SYSROOT}")
    endif()

    # Detect if automatic sysrootification should happen. All of the following must be true:
    # sysroot is set (CMAKE_SYSRROT)
    # prefix is set (CMAKE_INSTALL_PREFIX)
    # extprefix is explicitly NOT set (CMAKE_STAGING_PREFIX, not QT_STAGING_PREFIX because that
    #                                  always ends up having a value)
    if(NOT CMAKE_STAGING_PREFIX AND sysroot)
        set(sysrootify_prefix true)
    else()
        set(sysrootify_prefix false)
        if(NOT ext_prefix STREQUAL prefix)
            string(APPEND content "[DevicePaths]
Prefix=${prefix}
")
        endif()
    endif()

    qt_internal_get_host_info_var_prefix(host_info_var_prefix)
    string(APPEND content
        "[Paths]
Prefix=${ext_prefix_relative_to_conf_file}
Documentation=${INSTALL_DOCDIR}
Headers=${INSTALL_INCLUDEDIR}
Libraries=${INSTALL_LIBDIR}
LibraryExecutables=${INSTALL_LIBEXECDIR}
Binaries=${INSTALL_BINDIR}
Plugins=${INSTALL_PLUGINSDIR}
QmlImports=${INSTALL_QMLDIR}
ArchData=${INSTALL_ARCHDATADIR}
Data=${INSTALL_DATADIR}
Translations=${INSTALL_TRANSLATIONSDIR}
Examples=${INSTALL_EXAMPLESDIR}
Tests=${INSTALL_TESTSDIR}
Settings=${INSTALL_SYSCONFDIR}
HostPrefix=${host_prefix_relative_to_conf_file}
HostBinaries=${${host_info_var_prefix}_BINDIR}
HostLibraries=${${host_info_var_prefix}_LIBDIR}
HostLibraryExecutables=${${host_info_var_prefix}_LIBEXECDIR}
HostData=${ext_datadir_relative_to_host_prefix}
Sysroot=${sysroot}
SysrootifyPrefix=${sysrootify_prefix}
TargetSpec=${QT_QMAKE_TARGET_MKSPEC}
HostSpec=${QT_QMAKE_HOST_MKSPEC}
")
    file(GENERATE OUTPUT "${qt_conf_path}" CONTENT "${content}")
    qt_install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${qt_conf_path}"
        DESTINATION "${INSTALL_BINDIR}")

    if(QT_GENERATE_WRAPPER_SCRIPTS_FOR_ALL_HOSTS)
        set(hosts "unix" "non-unix")
    elseif(CMAKE_HOST_UNIX)
        set(hosts "unix")
    else()
        set(hosts "non-unix")
    endif()

    set(wrapper_prefix)
    if(QT_FORCE_BUILD_TOOLS)
        # Avoid collisions with the cross-compiled qmake/qtpaths binaries.
        set(wrapper_prefix "host-")
    endif()

    set(host_qt_bindir "${host_prefix}/${${host_info_var_prefix}_BINDIR}")
    file(TO_NATIVE_PATH "${host_qt_bindir}" host_qt_bindir)

    if(QT_CREATE_VERSIONED_HARD_LINK AND QT_WILL_INSTALL)
        set(tool_version "${PROJECT_VERSION_MAJOR}")
    endif()

    foreach(host_type ${hosts})
        foreach(tool_name qmake qtpaths)
            set(wrapper_extension)
            set(newline_style LF)

            if(host_type STREQUAL "non-unix")
                set(wrapper_extension ".bat")
                set(newline_style CRLF)
            endif()

            set(wrapper_in_file
                "${CMAKE_CURRENT_SOURCE_DIR}/bin/qmake-and-qtpaths-wrapper${wrapper_extension}.in")

            set(wrapper "preliminary/${wrapper_prefix}${tool_name}${wrapper_extension}")
            configure_file("${wrapper_in_file}" "${wrapper}" @ONLY NEWLINE_STYLE ${newline_style})
            qt_copy_or_install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/${wrapper}"
                DESTINATION "${INSTALL_BINDIR}")

            # Configuring a new wrapper file, this type setting the tool_version
            if(QT_CREATE_VERSIONED_HARD_LINK AND QT_WILL_INSTALL)
                set(versioned_wrapper "preliminary/${wrapper_prefix}${tool_name}${tool_version}${wrapper_extension}")
                configure_file("${wrapper_in_file}" "${versioned_wrapper}" @ONLY NEWLINE_STYLE ${newline_style})
                qt_copy_or_install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/${versioned_wrapper}"
                    DESTINATION "${INSTALL_BINDIR}")
            endif()
        endforeach()
    endforeach()
endfunction()

# Transforms a CMake Qt module name to a qmake Qt module name.
# Example: Qt6FooPrivate becomes foo_private
function(qt_get_qmake_module_name result module)
    string(REGEX REPLACE "^Qt6" "" module "${module}")
    string(REGEX REPLACE "Private$" "_private" module "${module}")
    string(REGEX REPLACE "Qpa$" "_qpa_lib_private" module "${module}")
    string(REGEX REPLACE "Rhi$" "_rhi_lib_private" module "${module}")
    string(REGEX REPLACE "Ssg$" "_ssg_lib_private" module "${module}")
    string(TOLOWER "${module}" module)
    set(${result} ${module} PARENT_SCOPE)
endfunction()

function(qt_cmake_build_type_to_qmake_build_config out_var build_type)
    if(build_type STREQUAL "Debug")
        set(cfg debug)
    else()
        set(cfg release)
    endif()
    set(${out_var} ${cfg} PARENT_SCOPE)
endfunction()

function(qt_guess_qmake_build_config out_var)
    if(QT_GENERATOR_IS_MULTI_CONFIG)
        unset(cfg)
        foreach(config_type ${CMAKE_CONFIGURATION_TYPES})
            qt_cmake_build_type_to_qmake_build_config(tmp ${config_type})
            list(APPEND cfg ${tmp})
        endforeach()
        if(cfg)
            list(REMOVE_DUPLICATES cfg)
        else()
            set(cfg debug)
        endif()
    else()
        qt_cmake_build_type_to_qmake_build_config(cfg ${CMAKE_BUILD_TYPE})
    endif()
    set(${out_var} ${cfg} PARENT_SCOPE)
endfunction()

macro(qt_add_qmake_lib_dependency lib dep)
    string(REPLACE "-" "_" dep ${dep})
    string(TOUPPER "${dep}" ucdep)
    list(APPEND QT_QMAKE_LIB_DEPS_${lib} ${ucdep})
endmacro()
