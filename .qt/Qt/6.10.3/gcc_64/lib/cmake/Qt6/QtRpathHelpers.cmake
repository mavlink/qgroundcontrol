# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Returns the platform-specific relative rpath base token, if it's supported.
# If it's not supported, returns the string NO_KNOWN_RPATH_REL_BASE.
function(qt_internal_get_relative_rpath_base_token out_var)
    if(APPLE)
        set(rpath_rel_base "@loader_path")
    elseif(LINUX OR SOLARIS OR FREEBSD OR HURD OR OPENBSD)
        set(rpath_rel_base "$ORIGIN")
    else()
        set(rpath_rel_base "NO_KNOWN_RPATH_REL_BASE")
    endif()
    set(${out_var} "${rpath_rel_base}" PARENT_SCOPE)
endfunction()

# Computes a relative rpath between ${rpath} and ${install_location} using tokens
# like $ORIGIN / @loader_path
# Not all platforms support such tokens though, in which case the returned rpath will be invalid.
#
# install_location: a directory relative to CMAKE_INSTALL_PREFIX, where the binary will be installed
# rpath:            an rpath to embed, can be either an absolute path or a path relative to
# ${CMAKE_INSTALL_PREFIX}/${INSTALL_LIBDIR}.
function(qt_compute_relative_rpath_base rpath install_location out_var)
    set(install_lib_dir_absolute "${CMAKE_INSTALL_PREFIX}/${INSTALL_LIBDIR}")
    get_filename_component(rpath_absolute "${rpath}"
                           ABSOLUTE BASE_DIR "${install_lib_dir_absolute}")

    set(install_location_absolute "${install_location}")
    if(NOT IS_ABSOLUTE "${install_location_absolute}")
        set(install_location_absolute "${CMAKE_INSTALL_PREFIX}/${install_location}")
    endif()
    # Compute relative rpath from where the target will be installed, to the place where libraries
    # will be placed (INSTALL_LIBDIR).
    file(RELATIVE_PATH rpath_relative "${install_location_absolute}" "${rpath_absolute}")

    if("${rpath_relative}" STREQUAL "")
        # file(RELATIVE_PATH) returns an empty string if the given absolute paths are equal
        set(rpath_relative ".")
    endif()

    # Prepend $ORIGIN / @loader_path style tokens (qmake's QMAKE_REL_RPATH_BASE), to make the
    # relative rpaths work. qmake does this automatically when generating a project, so it wasn't
    # needed in the .prf files, but for CMake we need to prepend them ourselves.
    qt_internal_get_relative_rpath_base_token(rpath_rel_base)
    if(rpath_rel_base STREQUAL "NO_KNOWN_RPATH_REL_BASE")
        message(WARNING "No known RPATH_REL_BASE for target platform.")
    endif()

    if(rpath_relative STREQUAL ".")
        set(rpath_relative "${rpath_rel_base}")
    else()
        set(rpath_relative "${rpath_rel_base}/${rpath_relative}")
    endif()

    set("${out_var}" "${rpath_relative}" PARENT_SCOPE)
endfunction()

# Applies necessary rpaths to a target upon target installation.
# No-op when targeting Windows, Android.
#
# Since abf72395411b135054b5820f64f93dfbcda430b8 rpaths are also applied in non-prefix builds,
# to address -rpath-link issues when cross-compiling, although this might not be needed anymore
# due to 606124c5cceba0dd4a406a9278588b58bb9f9800.
# See QTBUG-86533 for the whole saga.
#
# If no RELATIVE_RPATH option is given, embeds an absolute path rpath to
# ${CMAKE_INSTALL_PREFIX}/${INSTALL_LIBDIR} into the target.

# If RELATIVE_RPATH is given, the INSTALL_PATH value is used to compute the relative path from
# ${CMAKE_INSTALL_PREFIX}/${INSTALL_LIBDIR} to wherever the target will be installed
# (the value of INSTALL_PATH).
# INSTALL_PATH is expected to be a relative directory where the binary / library will be installed.

# RELATIVE_RPATH is the equivalent of qmake's relative_qt_rpath.
# INSTALL_PATH is used to implement the equivalent of qmake's $$qtRelativeRPathBase().
#
# QT_DISABLE_RPATH can be set to disable embedding any Qt specific rpaths.
function(qt_apply_rpaths)
    # No rpath support for win32 and android.
    if(WIN32 OR ANDROID)
        return()
    endif()

    # Rpaths explicitly disabled (like for uikit), equivalent to qmake's no_qt_rpath.
    # Or feature was turned OFF.
    if(QT_DISABLE_RPATH OR NOT QT_FEATURE_rpath)
        return()
    endif()

    cmake_parse_arguments(PARSE_ARGV 0 arg
        "RELATIVE_RPATH"
        "TARGET;INSTALL_PATH"
        "")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_TARGET)
        message(FATAL_ERROR "No target given to qt_apply_rpaths.")
    else()
        set(target "${arg_TARGET}")
    endif()

    # If a target is not built (which can happen for tools when crosscompiling, we shouldn't try
    # to apply properties.
    if(NOT TARGET "${target}")
        return()
    endif()

    # Protect against interface libraries.
    get_target_property(target_type "${target}" TYPE)
    if (target_type STREQUAL "INTERFACE_LIBRARY")
        return()
    endif()

    if(NOT arg_INSTALL_PATH)
        message(FATAL_ERROR "No INSTALL_PATH given to qt_apply_rpaths.")
    endif()

    set(rpaths "")

    # Modify the install path to contain the nested structure of a framework.
    get_target_property(is_framework "${target}" FRAMEWORK)
    if(is_framework)
        qt_internal_get_framework_info(fw ${target})
        if(UIKIT)
            # Shallow framework
            string(APPEND arg_INSTALL_PATH "/${fw_dir}")
        else()
            # Full framework
            string(APPEND arg_INSTALL_PATH "/${fw_dir}/Versions/Current")
        endif()
    endif()

    # Same but for an app bundle.
    get_target_property(is_bundle "${target}" MACOSX_BUNDLE)
    if(is_bundle AND NOT is_framework)
        if(UIKIT)
            # Shallow bundle
            string(APPEND arg_INSTALL_PATH "/${target}.app")
        else()
            # Full bundle
            string(APPEND arg_INSTALL_PATH "/${target}.app/Contents/MacOS")
        endif()
    endif()

    qt_internal_get_relative_rpath_base_token(rpath_base_token)
    if(rpath_base_token STREQUAL "NO_KNOWN_RPATH_REL_BASE")
        set(relative_rpath_supported FALSE)
    else()
        set(relative_rpath_supported TRUE)
    endif()

    # Somewhat similar to mkspecs/features/qt.prf
    # Embed either an absolute path to the installed Qt lib dir, or a relative one, based on
    # where ${target} is installed.
    # Don't embed relative rpaths if the platform does not support it.
    if(arg_RELATIVE_RPATH AND relative_rpath_supported)
        qt_compute_relative_rpath_base(
            "${_default_install_rpath}" "${arg_INSTALL_PATH}" relative_rpath)
        list(APPEND rpaths "${relative_rpath}")
    else()
        list(APPEND rpaths "${_default_install_rpath}")
    endif()

    # Somewhat similar to mkspecs/features/qt_build_extra.prf.
    foreach(rpath ${QT_EXTRA_RPATHS})
        if(IS_ABSOLUTE "${rpath}")
            list(APPEND rpaths "${rpath}")
        else()
            if(relative_rpath_supported)
                qt_compute_relative_rpath_base("${rpath}" "${arg_INSTALL_PATH}" relative_rpath)
                list(APPEND rpaths "${relative_rpath}")
            else()
                # Any extra relative rpaths on a platform that does not support relative rpaths,
                # need to be transformed into absolute ones.
                set(install_lib_dir_absolute "${CMAKE_INSTALL_PREFIX}/${INSTALL_LIBDIR}")
                get_filename_component(rpath_absolute "${rpath}"
                                       ABSOLUTE BASE_DIR "${install_lib_dir_absolute}")
                list(APPEND rpaths "${rpath_absolute}")
            endif()
        endif()
    endforeach()

    if(rpaths)
        list(REMOVE_DUPLICATES rpaths)
        if(QT_WILL_INSTALL)
            set(prop_name "INSTALL_RPATH")
        else()
            set(prop_name "BUILD_RPATH")
        endif()
        set_property(TARGET "${target}" APPEND PROPERTY "${prop_name}" ${rpaths})
    endif()
endfunction()

macro(qt_internal_set_default_rpath_settings)
    # the default RPATH to be used when installing, but only if it's not a system directory
    list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES
        "${CMAKE_INSTALL_PREFIX}/${INSTALL_LIBDIR}" isSystemDir)
    if("${isSystemDir}" STREQUAL "-1")
       set(_default_install_rpath "${CMAKE_INSTALL_PREFIX}/${INSTALL_LIBDIR}")
    endif("${isSystemDir}" STREQUAL "-1")

    # The default rpath settings for installed targets is empty.
    # The rpaths will instead be computed for each target separately using qt_apply_rpaths().
    # Additional rpaths can be passed via QT_EXTRA_RPATHS.
    # By default this will include $ORIGIN / @loader_path, so the installation is relocatable.
    # Bottom line: No need to pass anything to CMAKE_INSTALL_RPATH.
    set(CMAKE_INSTALL_RPATH "" CACHE STRING "RPATH for installed binaries")

    # By default, don't embed auto-determined RPATHs pointing to directories
    # outside of the build tree, into the installed binaries.
    # This ended up adding rpaths like ${CMAKE_INSTALL_PREFIX}/lib (or /Users/qt/work/install/lib
    # into the official libraries created by the CI) into the non-qtbase libraries, plugins, etc.
    #
    # It should not be necessary, given that qt_apply_rpaths() already adds the necessary rpaths,
    # either relocatable ones or absolute ones, depending on what the platform supports.
    if(NOT QT_NO_DISABLE_CMAKE_INSTALL_RPATH_USE_LINK_PATH)
        set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
    endif()

    # If Qt is built without rpath support, we should not add "user-project default rpaths" to
    # qt qml plugins. Do this by setting QT_NO_QML_PLUGIN_RPATH to TRUE, which is
    # then read by qt6_add_qml_plugin.
    # We do this as part of the internal API, because we still want to allow user project qml
    # plugins to have sensible default rpaths, even if Qt qml plugins were built without support
    # for rpaths.
    #
    # Note that feature evaluation is not done yet in qtbase at this point, so we check both
    # feature variable variants. In practice it doesn't really matter, because the variable is only
    # read during qtdeclarative configuration time when the feature is already evaluated.
    #
    # We also make sure not to set it as a cache var just in case somebody wants to override it
    # per directory scope.
    if(NOT DEFINED QT_NO_QML_PLUGIN_RPATH
            AND (QT_DISABLE_RPATH OR (NOT FEATURE_rpath) OR (NOT QT_FEATURE_rpath)))
        set(QT_NO_QML_PLUGIN_RPATH "TRUE")
    endif()
endmacro()

# Overrides the CMAKE_STAGING_PREFIX in a subdirectory scope, to stop CMake from rewriting build
# rpaths to point into the original staging prefix, and thus breaking running executables from
# the build directory.
# See details at https://bugreports.qt.io/browse/QTBUG-102592
# and https://gitlab.kitware.com/cmake/cmake/-/issues/23421
#
# This macro is only meant to be called in functions like
# qt_internal_add_module / qt_internal_add_tool to ensure the variable is set in the
# subdirectory scope of the calling function, and not in the actual function scope (where the
# variable assignment would have no effect).
#
# This is the best workaround we can currently do, but it comes with the disadvantage that calling
# subdirectory-scoped install targets does not work anymore.
# e.g. calling ninja src/gui/install will try to install to the fake prefix and fail.
# A regular ninja install call works fine.
#
# Usage of this macro assumes there are no binaries or libraries added in the root CMakeLists.txt
# of the project because that would mean the macro is called at root level scope, which would
# break installation.
#
# The implementation has to be a macro, so we can propagate the variable into the calling
# subdirectory scope. The implementation can't use return().
macro(qt_internal_apply_staging_prefix_build_rpath_workaround)
    set(__qt_internal_should_apply_staging_prefix_build_rpath_workaround TRUE)
    # Allow an opt out.
    if(QT_NO_STAGING_PREFIX_BUILD_RPATH_WORKAROUND)
        set(__qt_internal_should_apply_staging_prefix_build_rpath_workaround FALSE)
    endif()

    # No need for workaround if CMAKE_STAGING_PREFIX is not set.
    if(NOT CMAKE_STAGING_PREFIX)
        set(__qt_internal_should_apply_staging_prefix_build_rpath_workaround FALSE)
    endif()

    # No rpath support for win32, android, ios, so nothing to do.
    if(WIN32 OR ANDROID OR UIKIT)
        set(__qt_internal_should_apply_staging_prefix_build_rpath_workaround FALSE)
    endif()

    # Set the staging prefix to a non-existent directory, which is unlikely to have permissions
    # for installation.
    # The verbose directory name is chosen to attract the user's attention in case if they end up
    # calling a subdirectory-scope install file.
    if(__qt_internal_should_apply_staging_prefix_build_rpath_workaround)
        set_property(GLOBAL PROPERTY _qt_internal_staging_prefix_build_rpath_workaround TRUE)
        set(CMAKE_STAGING_PREFIX
            "/qt_fake_staging_prefix/check_qt_internal_apply_staging_prefix_build_rpath_workaround"
            PARENT_SCOPE)
    endif()
endmacro()
