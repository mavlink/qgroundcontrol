# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

macro(qt_internal_setup_default_install_prefix)
    # Detect non-prefix builds: either when the qtbase install prefix is set to the binary dir
    # or when a developer build is explicitly enabled and no install prefix (or staging prefix)
    # is specified.
    # This detection only happens when building qtbase, and later is propagated via the generated
    # QtBuildInternalsExtra.cmake file.
    if(PROJECT_NAME STREQUAL "QtBase" AND NOT QT_INTERNAL_BUILD_STANDALONE_PARTS)
        if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
            # Handle both FEATURE_ and QT_FEATURE_ cases when they are specified on the command line
            # explicitly. It's possible for one to be set, but not the other, because
            # qtbase/configure.cmake is not processed by this point.
            if((FEATURE_developer_build
                OR QT_FEATURE_developer_build
                OR FEATURE_no_prefix
                OR QT_FEATURE_no_prefix
                )
                AND NOT CMAKE_STAGING_PREFIX)
                # Handle non-prefix builds by setting the CMake install prefix to point to qtbase's
                # build dir. While building another repo (like qtsvg) the CMAKE_PREFIX_PATH should
                # be set on the command line to point to the qtbase build dir.
                set(__qt_default_prefix "${QtBase_BINARY_DIR}")
            else()
                if(CMAKE_HOST_WIN32)
                    set(__qt_default_prefix "C:/Qt/")
                else()
                    set(__qt_default_prefix "/usr/local/")
                endif()
                string(APPEND __qt_default_prefix
                    "Qt-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
            endif()
            set(CMAKE_INSTALL_PREFIX ${__qt_default_prefix} CACHE PATH
                "Install path prefix, prepended onto install directories." FORCE)
            unset(__qt_default_prefix)
        endif()
        if(CMAKE_STAGING_PREFIX)
            set(__qt_prefix "${CMAKE_STAGING_PREFIX}")
        else()
            set(__qt_prefix "${CMAKE_INSTALL_PREFIX}")
        endif()
        if(__qt_prefix STREQUAL QtBase_BINARY_DIR)
            set(__qt_will_install_value OFF)
        else()
            set(__qt_will_install_value ON)
        endif()
        set(QT_WILL_INSTALL ${__qt_will_install_value} CACHE BOOL
            "Boolean indicating if doing a Qt prefix build (vs non-prefix build)." FORCE)
        unset(__qt_prefix)
        unset(__qt_will_install_value)
    endif()
endmacro()

function(qt_internal_setup_build_and_install_paths)
    # Compute the values of QT_BUILD_DIR, QT_INSTALL_DIR, QT_CONFIG_BUILD_DIR, QT_CONFIG_INSTALL_DIR
    # taking into account whether the current build is a prefix build or a non-prefix build,
    # and whether it is a superbuild or non-superbuild.
    # A third case is when another module or standalone tests/examples are built against a
    # super-built Qt.
    # The layout for the third case is the same as for non-superbuilds.
    #
    # These values should be prepended to file paths in commands or properties,
    # in order to correctly place generated Config files, generated Targets files,
    # executables / libraries, when copying / installing files, etc.
    #
    # The build dir variables will always be absolute paths.
    # The QT_INSTALL_DIR variable will have a relative path in a prefix build,
    # which means that it can be empty, so use qt_join_path to prevent accidental absolute paths.
    if(QT_SUPERBUILD)
        # In this case, we always copy all the build products in qtbase/{bin,lib,...}
        if(QT_WILL_INSTALL)
            set(QT_BUILD_DIR "${QtBase_BINARY_DIR}")
            set(QT_INSTALL_DIR "")
        else()
            if("${CMAKE_STAGING_PREFIX}" STREQUAL "")
                set(QT_BUILD_DIR "${QtBase_BINARY_DIR}")
                set(QT_INSTALL_DIR "${QtBase_BINARY_DIR}")
            else()
                set(QT_BUILD_DIR "${CMAKE_STAGING_PREFIX}")
                set(QT_INSTALL_DIR "${CMAKE_STAGING_PREFIX}")
            endif()
        endif()
    else()
        if(QT_WILL_INSTALL)
            # In the usual prefix build case, the build dir is the current module build dir,
            # and the install dir is the prefix, so we don't set it.
            set(QT_BUILD_DIR "${CMAKE_BINARY_DIR}")
            set(QT_INSTALL_DIR "")
        else()
            # When doing a non-prefix build, both the build dir and install dir are the same,
            # pointing to the qtbase build dir.
            set(QT_BUILD_DIR "${QT_STAGING_PREFIX}")
            set(QT_INSTALL_DIR "${QT_BUILD_DIR}")
        endif()
    endif()

    set(__config_path_part "${INSTALL_LIBDIR}/cmake")
    set(QT_CONFIG_BUILD_DIR "${QT_BUILD_DIR}/${__config_path_part}")
    set(QT_CONFIG_INSTALL_DIR "${QT_INSTALL_DIR}")
    if(QT_CONFIG_INSTALL_DIR)
        string(APPEND QT_CONFIG_INSTALL_DIR "/")
    endif()
    string(APPEND QT_CONFIG_INSTALL_DIR ${__config_path_part})

    set(QT_BUILD_DIR "${QT_BUILD_DIR}" PARENT_SCOPE)
    set(QT_INSTALL_DIR "${QT_INSTALL_DIR}" PARENT_SCOPE)
    set(QT_CONFIG_BUILD_DIR "${QT_CONFIG_BUILD_DIR}" PARENT_SCOPE)
    set(QT_CONFIG_INSTALL_DIR "${QT_CONFIG_INSTALL_DIR}" PARENT_SCOPE)
endfunction()

function(qt_configure_process_path name default docstring)
    # Values are computed once for qtbase, and then exported and reused for other projects.
    if(NOT PROJECT_NAME STREQUAL "QtBase")
        return()
    endif()

    # No value provided, set the default.
    # Make sure to process the paths even for the defaults to ensure we have canonicalized paths
    # on the first configure run and thus don't regenerate qconfig.cpp on a subsequent
    # configure run when the paths might become different if we didn't do it on first run.
    # e.g. './plugins' would get turned to 'plugins'.
    if(NOT DEFINED "${name}")
        set("${name}" "${default}")
    endif()

    get_filename_component(given_path_as_abs "${${name}}" ABSOLUTE BASE_DIR
                           "${CMAKE_INSTALL_PREFIX}")
    file(RELATIVE_PATH rel_path "${CMAKE_INSTALL_PREFIX}"
                                "${given_path_as_abs}")

    # If absolute path given, check that it's inside the prefix (error out if not).
    # If relative path given, it's relative to the install prefix (rather than the binary dir,
    # which is what qmake does for some reason).
    # In both cases, store the value as a relative path, unless we're processing INSTALL_SYSCONFDIR
    # which should stay abslute.
    if("${rel_path}" STREQUAL "")
        # file(RELATIVE_PATH) returns an empty string if the given absolute paths are equal,
        # so manually set it to '.'
        set(new_value ".")
    elseif(rel_path MATCHES "^\.\./")
        # INSTALL_SYSCONFDIR is allowed to be outside the prefix.
        if(NOT name STREQUAL "INSTALL_SYSCONFDIR")
            message(FATAL_ERROR
                "Path component '${name}' is outside computed install prefix: ${rel_path} ")
        endif()
        # Keep the absolute path.
        set(new_value "${${name}}")
    else()
        # Use the canonicalized path.
        set(new_value "${rel_path}")
    endif()

    set("${name}" "${new_value}" CACHE STRING "${docstring}" FORCE)
endfunction()

macro(qt_internal_setup_configure_install_paths)
    # Install locations:
    qt_configure_process_path(INSTALL_BINDIR "bin" "Executables [PREFIX/bin]")
    qt_configure_process_path(INSTALL_INCLUDEDIR "include" "Header files [PREFIX/include]")
    qt_configure_process_path(INSTALL_LIBDIR "lib" "Libraries [PREFIX/lib]")
    qt_configure_process_path(INSTALL_MKSPECSDIR "mkspecs" "Mkspecs files [PREFIX/mkspecs]")
    qt_configure_process_path(INSTALL_SHAREDIR "share" "Share files [PREFIX/share]")
    qt_configure_process_path(INSTALL_QT_SHAREDIR "${INSTALL_SHAREDIR}/qt6"
        "Qt namespaced sharedir [SHAREDIR/qt6]")
    qt_configure_process_path(INSTALL_ARCHDATADIR "." "Arch-dependent data [PREFIX]")
    qt_configure_process_path(INSTALL_PLUGINSDIR
                              "${INSTALL_ARCHDATADIR}/plugins"
                              "Plugins [ARCHDATADIR/plugins]")

    if(NOT INSTALL_MKSPECSDIR MATCHES "(^|/)mkspecs")
        message(FATAL_ERROR "INSTALL_MKSPECSDIR must end with '/mkspecs'")
    endif()

    if (WIN32)
        set(_default_libexec "${INSTALL_ARCHDATADIR}/bin")
    else()
        set(_default_libexec "${INSTALL_ARCHDATADIR}/libexec")
    endif()

    qt_configure_process_path(
        INSTALL_LIBEXECDIR
        "${_default_libexec}"
        "Helper programs [ARCHDATADIR/bin on Windows, ARCHDATADIR/libexec otherwise]")
    qt_configure_process_path(INSTALL_QMLDIR
                              "${INSTALL_ARCHDATADIR}/qml"
                               "QML imports [ARCHDATADIR/qml]")
    qt_configure_process_path(INSTALL_DATADIR "." "Arch-independent data [PREFIX]")
    qt_configure_process_path(INSTALL_DOCDIR "${INSTALL_DATADIR}/doc" "Documentation [DATADIR/doc]")
    qt_configure_process_path(INSTALL_TRANSLATIONSDIR "${INSTALL_DATADIR}/translations"
        "Translations [DATADIR/translations]")
    if(APPLE)
        set(QT_DEFAULT_SYS_CONF_DIR "/Library/Preferences/Qt")
    else()
        set(QT_DEFAULT_SYS_CONF_DIR "etc/xdg")
    endif()
    qt_configure_process_path(
        INSTALL_SYSCONFDIR
        "${QT_DEFAULT_SYS_CONF_DIR}"
        "Settings used by Qt programs [PREFIX/etc/xdg]/[/Library/Preferences/Qt]")
    qt_configure_process_path(INSTALL_EXAMPLESDIR "examples" "Examples [PREFIX/examples]")
    qt_configure_process_path(INSTALL_TESTSDIR "tests" "Tests [PREFIX/tests]")
    qt_configure_process_path(INSTALL_DESCRIPTIONSDIR
                             "${INSTALL_ARCHDATADIR}/modules"
                              "Module description files directory")
    qt_configure_process_path(INSTALL_SBOMDIR "${INSTALL_ARCHDATADIR}/sbom"
        "SBOM [PREFIX/sbom]")

    # INSTALL_PUBLICBINDIR is processed only if it is not empty
    # See usage in qt_internal_generate_user_facing_tools_info
    if(NOT "${INSTALL_PUBLICBINDIR}" STREQUAL "")
        # A default value is not needed because it is always manually defined
        # but as per the documentation it is typically `bin`
        qt_configure_process_path(INSTALL_PUBLICBINDIR "bin"
                "Symlinked user-facing executables")
    endif()
endmacro()

macro(qt_internal_set_cmake_install_libdir)
    # Ensure that GNUInstallDirs's CMAKE_INSTALL_LIBDIR points to the same lib dir that Qt was
    # configured with. Currently this is important for QML plugins, which embed an rpath based
    # on that value.
    set(CMAKE_INSTALL_LIBDIR "${INSTALL_LIBDIR}")
endmacro()

macro(qt_internal_set_qt_cmake_dir)
    set(QT_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}")
endmacro()

macro(qt_internal_set_qt_apple_support_files_path)
    # This is analogous to what we have in QtConfig.cmake.in. It's copied here so that iOS
    # tests can be built in tree.
    if(APPLE)
        if(NOT CMAKE_SYSTEM_NAME OR CMAKE_SYSTEM_NAME STREQUAL "Darwin")
            set(__qt_internal_cmake_apple_support_files_path "${QT_CMAKE_DIR}/macos")
        elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS")
            set(__qt_internal_cmake_apple_support_files_path "${QT_CMAKE_DIR}/ios")
        elseif(CMAKE_SYSTEM_NAME STREQUAL "visionOS")
            set(__qt_internal_cmake_apple_support_files_path "${QT_CMAKE_DIR}/visionos")
        endif()
    endif()
endmacro()

macro(qt_internal_set_qt_staging_prefix)
    if(NOT "${CMAKE_STAGING_PREFIX}" STREQUAL "")
        set(QT_STAGING_PREFIX "${CMAKE_STAGING_PREFIX}")
    else()
        set(QT_STAGING_PREFIX "${CMAKE_INSTALL_PREFIX}")
    endif()
endmacro()

macro(qt_internal_setup_paths_and_prefixes)
    qt_internal_setup_configure_install_paths()

    qt_internal_set_qt_staging_prefix()

    # Depends on QT_STAGING_PREFIX being set.
    qt_internal_setup_build_and_install_paths()

    qt_get_relocatable_install_prefix(QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX)

    # Depends on INSTALL_LIBDIR being set.
    qt_internal_set_cmake_install_libdir()

    qt_internal_set_qt_cmake_dir()

    qt_internal_set_qt_apple_support_files_path()
endmacro()

# Returns the prefix for the variables defined by HostInfo package. The prefix
# is based on version information provided by HostInfo. Falls back to current
# project version if ${INSTALL_CMAKE_NAMESPACE}HostInfo_VERSION_MAJOR is not
# defined.
function(qt_internal_get_host_info_var_prefix out_var)
    if(${INSTALL_CMAKE_NAMESPACE}HostInfo_VERSION_MAJOR)
        set(${out_var} "QT${${INSTALL_CMAKE_NAMESPACE}HostInfo_VERSION_MAJOR}_HOST_INFO"
            PARENT_SCOPE)
    else()
        # This is not a valid way to define the host info versioned prefix, but
        # it's backward compatible with Qt versions older than 6.10.
        #
        # TODO: remove once Qt LTS versions older than 6.10 reach end of life.
        set(${out_var} "QT${PROJECT_VERSION_MAJOR}_HOST_INFO" PARENT_SCOPE)
    endif()
endfunction()
