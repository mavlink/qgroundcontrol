# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

define_property(TARGET
    PROPERTY
        QT_PLUGINS
    BRIEF_DOCS
        "List of Qt plug-ins associated with a given Qt module."
    FULL_DOCS
        "This is a property on Qt modules.
        For instance, sqlite;odbc for Sql"
)

define_property(TARGET
    PROPERTY
        MODULE_PLUGIN_TYPES
    BRIEF_DOCS
        "List of plugin categories associated to the Qt module"
    FULL_DOCS
        "This is a property on Qt modules.
        For instance, sqldrivers for Sql."
)

define_property(TARGET
    PROPERTY
        QT_PLUGIN_CLASS_NAME
    BRIEF_DOCS
        "Class name of the Qt plug-in"
    FULL_DOCS
        "This is a property on Qt plug-ins.
        For instance, QICOPlugin for the qico plug-in"
)

define_property(TARGET
    PROPERTY
        QT_PLUGIN_TYPE
    BRIEF_DOCS
        "Type of the Qt plug-in"
    FULL_DOCS
        "This is a property on Qt plug-ins.
        For example, the value of the QT_PLUGIN_TYPE property on the qico plugin is \"imageformats\""
)

define_property(TARGET
    PROPERTY
        QT_MODULE
    BRIEF_DOCS
        "Qt module associated with a plug-in."
    FULL_DOCS
        "This is a property on Qt plug-ins.
        For instance, Sql for qsqlite"
)

define_property(TARGET
    PROPERTY
        QT_DEFAULT_PLUGIN
    BRIEF_DOCS
        "Indicates whether a plug-in is added by default."
    FULL_DOCS
        "This is a property on Qt plug-ins.
        It is mainly used to indicate if a plug-in should be added
        to the default set of plug-ins when building a static app -
        for instance, which QPA should be linked."
)

define_property(TARGET
    PROPERTY
        QT_QML_MODULE_TARGET_PATH
    BRIEF_DOCS
        "Specifies the target path for a qml module"
    FULL_DOCS
        "Specifies the target path for a qml module"
)

define_property(TARGET
    PROPERTY
        QT_QML_MODULE_URI
    BRIEF_DOCS
        "Specifies the URI for a qml module"
    FULL_DOCS
        "Specifies the URI for a qml module"
)

define_property(TARGET
    PROPERTY
        QT_RESOURCE_PREFIX
    BRIEF_DOCS
        "Specifies the default Qt resource prefix."
    FULL_DOCS
        "When using qt_add_resource() without a PREFIX, then prefix of this target property
        will be used."
)

define_property(TARGET
    PROPERTY
        QT_QML_MODULE_VERSION
    BRIEF_DOCS
        "Specifies the qml module's version."
    FULL_DOCS
        "Specifies the qml module's version."
)

define_property(GLOBAL
    PROPERTY
        QT_TARGETS_FOLDER
    BRIEF_DOCS
        "Name of the FOLDER for targets internally created by AUTOGEN and Qt's CMake API."
    FULL_DOCS
        "This property is used to initialize AUTOGEN_TARGETS_FOLDER and the FOLDER property of
        internal targets created by Qt's CMake commands."
)
