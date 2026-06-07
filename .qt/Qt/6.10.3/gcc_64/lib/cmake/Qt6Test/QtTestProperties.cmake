# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# This file contains the property definitions that are known by Qt Test

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.23")
    set(qt_skip_default_testcase_dirs_extra_agrs
        INITIALIZE_FROM_VARIABLE
        QT_SKIP_DEFAULT_TESTCASE_DIRS
    )
elseif(DEFINED QT_SKIP_DEFAULT_TESTCASE_DIRS)
    message(WARNING "QT_SKIP_DEFAULT_TESTCASE_DIRS is set to ${QT_SKIP_DEFAULT_TESTCASE_DIRS},"
        " but the variable is not supported by this CMake version. Please set the"
        " QT_SKIP_DEFAULT_TESTCASE_DIRS target property where is required.")
endif()

define_property(TARGET
    PROPERTY
        QT_SKIP_DEFAULT_TESTCASE_DIRS
    BRIEF_DOCS
        "Disables the test case directory definitions for the Qt Test targets."
    FULL_DOCS
        "By default the definitions QT_TESTCASE_SOURCEDIR and QT_TESTCASE_BUILDDIR point to the
        target source and build directories of the target accordingly. If
        QT_SKIP_DEFAULT_TESTCASE_DIRS is set to TRUE the macros remain empty."
    ${qt_skip_default_testcase_dirs_extra_agrs}
)
