message("Adding Custom Plugin")

#-- Version control
#   Major and minor versions are defined here (manually)

CUSTOM_QGC_VER_MAJOR = 1
CUSTOM_QGC_VER_MINOR = 4
CUSTOM_QGC_VER_FIRST_BUILD = 26

exists ($$PWD/.git) {
    CUSTOM_GIT_DESCRIBE = $$system(git --git-dir $$PWD/.git --work-tree $$PWD describe --always --tags)
    CUSTOM_GIT_BRANCH   = $$system(git --git-dir $$PWD/.git --work-tree $$PWD rev-parse --abbrev-ref HEAD)
    CUSTOM_GIT_HASH     = $$system(git --git-dir $$PWD/.git --work-tree $$PWD rev-parse --short HEAD)
    CUSTOM_GIT_TIME     = $$system(git --git-dir $$PWD/.git --work-tree $$PWD show --oneline --format=\"%ci\" -s HEAD)

    # determine if we're on a tag matching vX.Y.Z (stable release)
    contains(CUSTOM_GIT_DESCRIBE, v[0-9]+.[0-9]+.[0-9]+) {
        # release version "vX.Y.Z"
        CUSTOM_GIT_VERSION = "Git tag: $${CUSTOM_GIT_DESCRIBE}"
    } else {
        # development version "Development branch:sha date"
        CUSTOM_GIT_VERSION = "$${CUSTOM_GIT_BRANCH}:$${CUSTOM_GIT_HASH} - $${CUSTOM_GIT_TIME}"
    }

    # Build number is automatic
    # Uses the current branch. This way it works on any branch including build-server's PR branches
    CUSTOM_QGC_VER_BUILD = $$system(git --git-dir $$PWD/.git rev-list $$CUSTOM_GIT_BRANCH --first-parent --count)
    win32 {
        CUSTOM_QGC_VER_BUILD = $$system("set /a $$CUSTOM_QGC_VER_BUILD - $$CUSTOM_QGC_VER_FIRST_BUILD")
    } else {
        CUSTOM_QGC_VER_BUILD = $$system("echo $(($$CUSTOM_QGC_VER_BUILD - $$CUSTOM_QGC_VER_FIRST_BUILD))")
    }
    CUSTOM_QGC_VERSION = $${CUSTOM_QGC_VER_MAJOR}.$${CUSTOM_QGC_VER_MINOR}.$${CUSTOM_QGC_VER_BUILD}
} else {
    CUSTOM_QGC_VER_BUILD     = None
}

QMAKE_SUBSTITUTES += $$PWD/deploy/custom_build_version.txt.in

DEFINES += CUSTOM_QGC_VERSION=\"\\\"v$$CUSTOM_QGC_VERSION\\\"\"
DEFINES += CUSTOM_GIT_VERSION=\"\\\"$$CUSTOM_GIT_VERSION\\\"\"

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

