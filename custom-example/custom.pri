message("Adding Custom Plugin")

#-- Version control
#   Take version form the last SRR tag
CUSTOM_QGC_VERSION=$${GIT_VERSION}
CUSTOM_PLATFORM_VERSION=0.0.0.0
CUSTOM_QGC_GIT_PATCH=0

exists ($$PWD/../.git) {
    CUSTOM_GIT_DESCRIBE = $$system(git --git-dir $$PWD/../.git --work-tree $$PWD/.. describe --always --tags --match="srr-v[0-9]*.*[0-9].*[0-9]")

    # determine if we're on a custom tag: srr-vX.Y.Z otherwise git will add the number of
    # commits on top and the commit hash
    contains(CUSTOM_GIT_DESCRIBE, "^srr-v[0-9]+.[0-9]+.[0-9]+(?:-[0-9]+-g[0-9a-f]{5,40})?$") {
        # If HEAD not at tag include patches on top as well
        contains(CUSTOM_GIT_DESCRIBE, "^srr-v[0-9]+.[0-9]+.[0-9]+-[0-9]+-g[0-9a-f]{5,40}$") {
            CUSTOM_QGC_VERSION= $$section(CUSTOM_GIT_DESCRIBE, -, 1, 2)
            CUSTOM_QGC_GIT_PATCH= $$section(CUSTOM_GIT_DESCRIBE, -, 2, 2)
        } else {
            CUSTOM_QGC_VERSION= $$section(CUSTOM_GIT_DESCRIBE, -, 1, 1)
        }
        CUSTOM_QGC_VERSION= $$replace(CUSTOM_QGC_VERSION, "v", "")
        CUSTOM_QGC_VERSION_MAJOR= $$section(CUSTOM_QGC_VERSION, ., 0, 0)
        CUSTOM_QGC_VERSION_MINOR= $$section(CUSTOM_QGC_VERSION, ., 1, 1)
        CUSTOM_QGC_VERSION_PATCH= $$section(CUSTOM_QGC_VERSION, ., 2, 2)

        CUSTOM_PLATFORM_VERSION = $${CUSTOM_QGC_VERSION_MAJOR}.$${CUSTOM_QGC_VERSION_MINOR}.$${CUSTOM_QGC_VERSION_PATCH}.$${CUSTOM_QGC_GIT_PATCH}
    }

    MacBuild {
        MAC_VERSION = $$section(CUSTOM_PLATFORM_VERSION, ".", 0, 2)
        MAC_BUILD = $${CUSTOM_QGC_GIT_PATCH}
    }

    # Used by external scripts to extract custom build version
    QMAKE_SUBSTITUTES += $$PWD/deploy/custom_build_version.txt.in
}

DEFINES += CUSTOM_QGC_VERSION=\"\\\"v$$CUSTOM_QGC_VERSION\\\"\"

message(Custom QGC Version: $${CUSTOM_QGC_VERSION})

# We implement our own PX4 plugin factory
CONFIG  += QGC_DISABLE_PX4_PLUGIN_FACTORY

# Branding

DEFINES += CUSTOMHEADER=\"\\\"CustomPlugin.h\\\"\"
DEFINES += CUSTOMCLASS=CustomPlugin

TARGET   = CustomQGC
DEFINES += QGC_APPLICATION_NAME=\"\\\"CustomQGC\\\"\"

DEFINES += QGC_ORG_NAME=\"\\\"qgroundcontrol.org\\\"\"
DEFINES += QGC_ORG_DOMAIN=\"\\\"org.qgroundcontrol\\\"\"

QGC_APP_NAME        = "Custom GS"
QGC_BINARY_NAME     = "CustomQGC"
QGC_ORG_NAME        = "Custom"
QGC_ORG_DOMAIN      = "org.qgroundcontrol"
QGC_APP_DESCRIPTION = "Custom QGC Ground Station"
QGC_APP_COPYRIGHT   = "Copyright (C) 2019 QGroundControl Development Team. All rights reserved."

# Our own, custom resources
RESOURCES += \
    $$QGCROOT/$$QGC_CUSTOM_BUILD_FOLDER/custom.qrc

QML_IMPORT_PATH += \
    $$QGCROOT/$$QGC_CUSTOM_BUILD_FOLDER/res

# Our own, custom sources
SOURCES += \
    $$PWD/src/CustomPlugin.cc \
    $$PWD/src/CustomQuickInterface.cc \
    $$PWD/src/CustomVideoManager.cc

HEADERS += \
    $$PWD/src/CustomPlugin.h \
    $$PWD/src/CustomQuickInterface.h \
    $$PWD/src/CustomVideoManager.h

INCLUDEPATH += \
    $$PWD/src \

#-------------------------------------------------------------------------------------
# Custom Firmware/AutoPilot Plugin

INCLUDEPATH += \
    $$QGCROOT/$$QGC_CUSTOM_BUILD_FOLDER/src/FirmwarePlugin \
    $$QGCROOT/$$QGC_CUSTOM_BUILD_FOLDER/src/AutoPilotPlugin

HEADERS+= \
    $$QGCROOT/$$QGC_CUSTOM_BUILD_FOLDER/src/AutoPilotPlugin/CustomAutoPilotPlugin.h \
    $$QGCROOT/$$QGC_CUSTOM_BUILD_FOLDER/src/FirmwarePlugin/CustomCameraControl.h \
    $$QGCROOT/$$QGC_CUSTOM_BUILD_FOLDER/src/FirmwarePlugin/CustomCameraManager.h \
    $$QGCROOT/$$QGC_CUSTOM_BUILD_FOLDER/src/FirmwarePlugin/CustomFirmwarePlugin.h \
    $$QGCROOT/$$QGC_CUSTOM_BUILD_FOLDER/src/FirmwarePlugin/CustomFirmwarePluginFactory.h \

SOURCES += \
    $$QGCROOT/$$QGC_CUSTOM_BUILD_FOLDER/src/AutoPilotPlugin/CustomAutoPilotPlugin.cc \
    $$QGCROOT/$$QGC_CUSTOM_BUILD_FOLDER/src/FirmwarePlugin/CustomCameraControl.cc \
    $$QGCROOT/$$QGC_CUSTOM_BUILD_FOLDER/src/FirmwarePlugin/CustomCameraManager.cc \
    $$QGCROOT/$$QGC_CUSTOM_BUILD_FOLDER/src/FirmwarePlugin/CustomFirmwarePlugin.cc \
    $$QGCROOT/$$QGC_CUSTOM_BUILD_FOLDER/src/FirmwarePlugin/CustomFirmwarePluginFactory.cc \

#-------------------------------------------------------------------------------------
# Android

AndroidBuild {
    CONFIG += QGC_DISABLE_BUILD_SETUP

    # For now only android uses install customization
    # It's important to keep the right order because QGCSetup.pri offers the first command for QMAKE_POST_LINK
    include($$QGCROOT/QGCSetup.pri)
    # Disable only for Android
    CONFIG += QGC_DISABLE_INSTALLER_SETUP
}

include($$QGCROOT/$$QGC_CUSTOM_BUILD_FOLDER/customQGCInstaller.pri)


#-------------------------------------------------------------------------------------
# Map Grid

INCLUDEPATH += \
    $$QGCROOT/$$QGC_CUSTOM_BUILD_FOLDER/src/MapGrid

HEADERS+= \
    $$QGCROOT/$$QGC_CUSTOM_BUILD_FOLDER/src/MapGrid/MapGrid.h \
    $$QGCROOT/$$QGC_CUSTOM_BUILD_FOLDER/src/MapGrid/MapGridMGRS.h

SOURCES += \
    $$QGCROOT/$$QGC_CUSTOM_BUILD_FOLDER/src/MapGrid/MapGrid.cc \
    $$QGCROOT/$$QGC_CUSTOM_BUILD_FOLDER/src/MapGrid/MapGridMGRS.cc
