# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Propagate common variables via BuildInternals package.
set(QT_BUILD_SHARED_LIBS ON)
option(BUILD_SHARED_LIBS "Build Qt statically or dynamically" ON)
set(QT_CMAKE_EXPORT_NAMESPACE Qt6)
set(INSTALL_CMAKE_NAMESPACE Qt6)
set(QT_BUILD_INTERNALS_PATH "${CMAKE_CURRENT_LIST_DIR}")

# The relocatable install prefix is meant to be used to find things like host binaries (syncqt),
# when the CMAKE_INSTALL_PREFIX is overridden to point to a different path (like when building a
# a Qt repo using Conan, which will set a random install prefix instead of installing into the
# original Qt install prefix).
get_filename_component(QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX
                       ${CMAKE_CURRENT_LIST_DIR}/../../../
                       ABSOLUTE)

# Stores in out_var the new install/staging prefix for this build.
#
# new_prefix: the new prefix for this repository
# orig_prefix: the prefix that was used when qtbase was configured
#
# On Windows hosts: if the original prefix does not start with a drive letter, this function removes
# the drive letter from the new prefix. This is needed for installation with DESTDIR set.
function(qt_internal_new_prefix out_var new_prefix orig_prefix)
    if(CMAKE_HOST_WIN32)
        set(drive_letter_regexp "^[a-zA-Z]:")
        if(new_prefix MATCHES "${drive_letter_regexp}"
            AND NOT orig_prefix MATCHES "${drive_letter_regexp}")
            string(SUBSTRING "${new_prefix}" 2 -1 new_prefix)
        endif()
    endif()
    set(${out_var} "${new_prefix}" PARENT_SCOPE)
endfunction()

# If no explicit CMAKE_INSTALL_PREFIX is provided, force set the original Qt installation prefix,
# so that further modules / repositories are  installed into same original location.
# This means by default when configuring qtsvg / qtdeclarative, they will be installed the regular
# Qt installation prefix.
# If an explicit installation prefix is specified,  honor it.
# This is an attempt to support Conan, aka handle installation of modules into a
# different installation prefix than the original one. Also allow to opt out via a special variable.
# In a top-level build, QtSetup.cmake takes care of setting CMAKE_INSTALL_PREFIX.
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT AND
        NOT QT_BUILD_INTERNALS_NO_FORCE_SET_INSTALL_PREFIX
        AND NOT QT_SUPERBUILD)
    set(qtbi_orig_prefix "/home/qt/work/install")
    set(qtbi_orig_staging_prefix "")
    qt_internal_new_prefix(qtbi_new_prefix
        "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}"
        "${qtbi_orig_prefix}")
    if(NOT qtbi_orig_staging_prefix STREQUAL ""
            AND "${CMAKE_STAGING_PREFIX}" STREQUAL ""
            AND NOT QT_BUILD_INTERNALS_NO_FORCE_SET_STAGING_PREFIX)
        qt_internal_new_prefix(qtbi_new_staging_prefix
            "${qtbi_new_prefix}"
            "${qtbi_orig_staging_prefix}")
        set(CMAKE_STAGING_PREFIX "${qtbi_new_staging_prefix}" CACHE PATH
            "Staging path prefix, prepended onto install directories on the host machine." FORCE)
        set(qtbi_new_prefix "${qtbi_orig_prefix}")
    endif()
    set(CMAKE_INSTALL_PREFIX "${qtbi_new_prefix}" CACHE PATH
        "Install path prefix, prepended onto install directories." FORCE)
    unset(qtbi_orig_prefix)
    unset(qtbi_new_prefix)
    unset(qtbi_orig_staging_prefix)
    unset(qtbi_new_staging_prefix)
endif()

# Propagate developer builds to other modules via BuildInternals package.
if(OFF)
    set(FEATURE_developer_build ON CACHE BOOL "Developer build." FORCE)
endif()

# Propagate non-prefix builds.
set(QT_WILL_INSTALL ON CACHE BOOL
    "Boolean indicating if doing a Qt prefix build (vs non-prefix build)." FORCE)

set(QT_SOURCE_TREE "/home/qt/work/qt/qtbase" CACHE PATH
"A path to the source tree of the previously configured QtBase project." FORCE)

# Propagate decision of building tests and examples to other repositories.
set(QT_BUILD_TESTS OFF CACHE BOOL "Build the testing tree.")
set(QT_BUILD_EXAMPLES FALSE CACHE BOOL "Build Qt examples")
set(QT_BUILD_BENCHMARKS OFF CACHE BOOL "Build Qt Benchmarks")
set(QT_BUILD_MANUAL_TESTS OFF CACHE BOOL "Build Qt manual tests")
set(QT_BUILD_MINIMAL_STATIC_TESTS OFF CACHE BOOL
    "Build minimal subset of tests for static Qt builds")
set(QT_BUILD_MINIMAL_ANDROID_MULTI_ABI_TESTS OFF CACHE BOOL
    "Build minimal subset of tests for Android multi-ABI Qt builds")

set(QT_BUILD_TESTS_BATCHED OFF CACHE BOOL
    "Should all tests be batched into a single binary.")

set(QT_BUILD_TESTS_BY_DEFAULT ON CACHE BOOL
    "Should tests be built as part of the default 'all' target.")
set(QT_BUILD_EXAMPLES_BY_DEFAULT ON CACHE BOOL
    "Should examples be built as part of the default 'all' target.")
set(QT_BUILD_TOOLS_BY_DEFAULT ON CACHE BOOL
    "Should tools be built as part of the default 'all' target.")

set(QT_BUILD_EXAMPLES_AS_EXTERNAL "OFF" CACHE BOOL
    "Should examples be built as ExternalProjects.")

# Propagate usage of ccache.
set(QT_USE_CCACHE OFF CACHE BOOL "Enable the use of ccache")

# Propagate usage of vcpkg, ON by default.
set(QT_USE_VCPKG OFF CACHE BOOL "Enable the use of vcpkg")

# Propagate usage of unity build.
set(QT_UNITY_BUILD OFF CACHE BOOL "Enable unity (jumbo) build")
set(QT_UNITY_BUILD_BATCH_SIZE "32" CACHE STRING "Unity build batch size")

# Propragate the value of WARNINGS_ARE_ERRORS.
set(WARNINGS_ARE_ERRORS "OFF" CACHE BOOL "Build Qt with warnings as errors")

# Propagate usage of versioned hard link.
set(QT_CREATE_VERSIONED_HARD_LINK "ON" CACHE BOOL
    "Enable the use of versioned hard link")

# The minimum version required to build Qt.
set(QT_SUPPORTED_MIN_CMAKE_VERSION_FOR_BUILDING_QT "3.22")
set(QT_COMPUTED_MIN_CMAKE_VERSION_FOR_BUILDING_QT "3.22")

# The lower and upper CMake version policy range as computed by qtbase.
# These values are inherited when building other Qt repositories, unless overridden
# in the respective repository .cmake.conf file.
# These are not cache variables, so that they can be overridden in each repo directory scope.
if(NOT DEFINED QT_MIN_NEW_POLICY_CMAKE_VERSION)
    set(QT_MIN_NEW_POLICY_CMAKE_VERSION "3.16")
endif()
if(NOT DEFINED QT_MAX_NEW_POLICY_CMAKE_VERSION)
    set(QT_MAX_NEW_POLICY_CMAKE_VERSION "3.21")
endif()

# Extra set of exported variables
set(TEST_architecture_arch "x86_64" CACHE INTERNAL "")
set(TEST_architecture_architectures "x86_64" CACHE INTERNAL "")
set(TEST_subarch_result "cx16;popcnt;sse3;ssse3;sse4.1;sse4.2;sse4" CACHE INTERNAL "")
set(TEST_arch_x86_64_abi "x86_64-little_endian-lp64" CACHE INTERNAL "")
set(TEST_arch_x86_64_subarch_cx16 "1" CACHE INTERNAL "")
set(TEST_arch_x86_64_subarch_popcnt "1" CACHE INTERNAL "")
set(TEST_arch_x86_64_subarch_sse3 "1" CACHE INTERNAL "")
set(TEST_arch_x86_64_subarch_ssse3 "1" CACHE INTERNAL "")
set(TEST_arch_x86_64_subarch_sse4.1 "1" CACHE INTERNAL "")
set(TEST_arch_x86_64_subarch_sse4.2 "1" CACHE INTERNAL "")
set(TEST_arch_x86_64_subarch_sse4 "1" CACHE INTERNAL "")
set(TEST_buildAbi "" CACHE INTERNAL "")
set(TEST_ld_version_script "1" CACHE INTERNAL "")
set(TEST_ld_version_script "1" CACHE INTERNAL "")

# Used by qt_internal_set_cmake_build_type.
set(__qt_internal_initial_qt_cmake_build_type "RelWithDebInfo")
set(BUILD_WITH_PCH "ON" CACHE STRING "")
set(QT_QPA_DEFAULT_PLATFORM "xcb" CACHE STRING "")
set(QT_QPA_PLATFORMS "xcb" CACHE STRING "")
set(CMAKE_INSTALL_RPATH "" CACHE STRING "")

if(NOT QT_SKIP_BUILD_INTERNALS_PKG_CONFIG_FEATURE)
    set(FEATURE_pkg_config "ON" CACHE BOOL "Using pkg-config" FORCE)
endif()

set(INSTALL_BINDIR "bin" CACHE STRING "Executables [PREFIX/bin]" FORCE)
set(INSTALL_INCLUDEDIR "include" CACHE STRING "Header files [PREFIX/include]" FORCE)
set(INSTALL_LIBDIR "lib" CACHE STRING "Libraries [PREFIX/lib]" FORCE)
set(INSTALL_MKSPECSDIR "mkspecs" CACHE STRING "Mkspecs files [PREFIX/mkspecs]" FORCE)
set(INSTALL_ARCHDATADIR "." CACHE STRING "Arch-dependent data [PREFIX]" FORCE)
set(INSTALL_PLUGINSDIR "plugins" CACHE STRING "Plugins [ARCHDATADIR/plugins]" FORCE)
set(INSTALL_LIBEXECDIR "libexec" CACHE STRING "Helper programs [ARCHDATADIR/bin on Windows, ARCHDATADIR/libexec otherwise]" FORCE)
set(INSTALL_QMLDIR "qml" CACHE STRING "QML imports [ARCHDATADIR/qml]" FORCE)
set(INSTALL_DATADIR "." CACHE STRING "Arch-independent data [PREFIX]" FORCE)
set(INSTALL_DOCDIR "doc" CACHE STRING "Documentation [DATADIR/doc]" FORCE)
set(INSTALL_TRANSLATIONSDIR "translations" CACHE STRING "Translations [DATADIR/translations]" FORCE)
set(INSTALL_SYSCONFDIR "etc/xdg" CACHE STRING "Settings used by Qt programs [PREFIX/etc/xdg]/[/Library/Preferences/Qt]" FORCE)
set(INSTALL_EXAMPLESDIR "examples" CACHE STRING "Examples [PREFIX/examples]" FORCE)
set(INSTALL_TESTSDIR "tests" CACHE STRING "Tests [PREFIX/tests]" FORCE)
set(INSTALL_DESCRIPTIONSDIR "modules" CACHE STRING "Module description files directory" FORCE)
set(INSTALL_SBOMDIR "sbom" CACHE STRING "SBOM [PREFIX/sbom]" FORCE)

# Use the OpenGL_GL_PREFERENCE value qtbase was built with. But do not FORCE it.
set(OpenGL_GL_PREFERENCE "LEGACY" CACHE STRING "")

set(QT_COPYRIGHT "Copyright (C) The Qt Company Ltd. and other contributors." CACHE STRING "")


if(NOT QT_SUPPORTED_MIN_IOS_SDK_VERSION)
    set(QT_SUPPORTED_MIN_IOS_SDK_VERSION "17")
endif()

if(NOT QT_SUPPORTED_MAX_IOS_SDK_VERSION)
    set(QT_SUPPORTED_MAX_IOS_SDK_VERSION "26")
endif()

if(NOT QT_SUPPORTED_MIN_IOS_XCODE_VERSION)
    set(QT_SUPPORTED_MIN_IOS_XCODE_VERSION "15")
endif()

if(NOT QT_SUPPORTED_MIN_IOS_VERSION)
    set(QT_SUPPORTED_MIN_IOS_VERSION "17")
endif()

if(NOT QT_SUPPORTED_MAX_IOS_VERSION_TESTED)
    set(QT_SUPPORTED_MAX_IOS_VERSION_TESTED "26")
endif()

if(NOT QT_SUPPORTED_MIN_VISIONOS_SDK_VERSION)
    set(QT_SUPPORTED_MIN_VISIONOS_SDK_VERSION "1")
endif()

if(NOT QT_SUPPORTED_MAX_VISIONOS_SDK_VERSION)
    set(QT_SUPPORTED_MAX_VISIONOS_SDK_VERSION "2")
endif()

if(NOT QT_SUPPORTED_MIN_VISIONOS_XCODE_VERSION)
    set(QT_SUPPORTED_MIN_VISIONOS_XCODE_VERSION "15")
endif()

if(NOT QT_SUPPORTED_MIN_VISIONOS_VERSION)
    set(QT_SUPPORTED_MIN_VISIONOS_VERSION "1")
endif()

if(NOT QT_SUPPORTED_MAX_VISIONOS_VERSION_TESTED)
    set(QT_SUPPORTED_MAX_VISIONOS_VERSION_TESTED "2")
endif()

if(NOT QT_SUPPORTED_MIN_MACOS_SDK_VERSION)
    set(QT_SUPPORTED_MIN_MACOS_SDK_VERSION "14")
endif()

if(NOT QT_SUPPORTED_MAX_MACOS_SDK_VERSION)
    set(QT_SUPPORTED_MAX_MACOS_SDK_VERSION "26")
endif()

if(NOT QT_SUPPORTED_MIN_MACOS_XCODE_VERSION)
    set(QT_SUPPORTED_MIN_MACOS_XCODE_VERSION "15")
endif()

if(NOT QT_SUPPORTED_MIN_MACOS_VERSION)
    set(QT_SUPPORTED_MIN_MACOS_VERSION "13")
endif()

if(NOT QT_SUPPORTED_MAX_MACOS_VERSION_TESTED)
    set(QT_SUPPORTED_MAX_MACOS_VERSION_TESTED "26")
endif()

