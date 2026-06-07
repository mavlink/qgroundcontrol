# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.23")
    set(_qt_qml_debug_extra_args
        INITIALIZE_FROM_VARIABLE
        QT_ENABLE_QML_DEBUG
    )
elseif(DEFINED QT_ENABLE_QML_DEBUG)
    message(WARNING "QT_ENABLE_QML_DEBUG is set to ${QT_ENABLE_QML_DEBUG},"
        " but the variable is not supported by this CMake version. Please set the"
        " QT_ENABLE_QML_DEBUG target property where is required.")
endif()

define_property(TARGET
    PROPERTY
        QT_ENABLE_QML_DEBUG
    BRIEF_DOCS
        "Enables QML debugging for the specific target"
    FULL_DOCS
        "The property controls the QT_ENABLE_QML_DEBUG definition for the respective QML module.
        The property is only applicable for Qt QML module applications."
    ${_qt_qml_debug_extra_args}
)

unset(_qt_qml_debug_extra_args)
