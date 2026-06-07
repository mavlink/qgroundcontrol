# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Includes QtSetup and friends for private CMake API.
set(QT_INTERNAL_IS_STANDALONE_TEST TRUE)

# Make find_package(Qt6 COMPONENTS Foo) pull in FooPrivate too.
set(QT_FIND_PRIVATE_MODULES TRUE)

# Checks minimum CMake version and upgrades policies.
qt_internal_project_setup()

# Look for the Qt6 package before trying to call qt_build_internals_set_up_private_api,
# otherwise it will fail when using a cross-compiled Qt, because QT_HOST_PATH will not be set.
# QT_HOST_PATH is set by Qt6Dependencies.cmake.
find_package(Qt6 REQUIRED)

# Includes QtSetup.cmake.
qt_build_internals_set_up_private_api()

# Find all StandaloneTestsConfig.cmake files, and include them
# This will find all Qt packages that are required for standalone tests.
# It will find more packages that needed for a certain test, but will ensure any test can
# be built.
qt_get_standalone_parts_config_files_path(standalone_parts_config_path)

file(GLOB config_files "${standalone_parts_config_path}/*")
foreach(file ${config_files})
    include("${file}")
endforeach()

# Set language standards after finding Core, because that's when the relevant
# feature variables are available.
qt_set_language_standards()

# Just before adding the test, change the local (non-cache) install prefix to something other than
# the Qt install prefix, so that tests don't try to install and pollute the Qt install prefix.
# Needs to be called after qt_get_standalone_parts_config_files_path().
qt_internal_set_up_fake_standalone_parts_install_prefix()
