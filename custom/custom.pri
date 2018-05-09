message("Adding Auterion Plugin")

#-- Version control
#   Major and minor versions are defined here (manually)

AUTERION_QGC_VER_MAJOR = 0
AUTERION_QGC_VER_MINOR = 0
AUTERION_QGC_VER_FIRST_BUILD = 0

#   Build number is automatic

AUTERION_QGC_VER_BUILD = $$system(git --git-dir ../.git rev-list master --first-parent --count)
win32 {
    AUTERION_QGC_VER_BUILD = $$system("set /a $$AUTERION_QGC_VER_BUILD - $$AUTERION_QGC_VER_FIRST_BUILD")
} else {
    AUTERION_QGC_VER_BUILD = $$system("echo $(($$AUTERION_QGC_VER_BUILD - $$AUTERION_QGC_VER_FIRST_BUILD))")
}
AUTERION_QGC_VERSION = $${AUTERION_QGC_VER_MAJOR}.$${AUTERION_QGC_VER_MINOR}.$${AUTERION_QGC_VER_BUILD}

DEFINES -= GIT_VERSION=\"\\\"$$GIT_VERSION\\\"\"
DEFINES += GIT_VERSION=\"\\\"$$AUTERION_QGC_VERSION\\\"\"

message(AuterionQGC Version: $${AUTERION_QGC_VERSION})

DEFINES += CUSTOMHEADER=\"\\\"AuterionPlugin.h\\\"\"
DEFINES += CUSTOMCLASS=AuterionPlugin

CONFIG  += QGC_DISABLE_APM_PLUGIN QGC_DISABLE_APM_PLUGIN_FACTORY

TARGET   = AuterionQGC
DEFINES += QGC_APPLICATION_NAME=\"\\\"AuterionQGC\\\"\"

DEFINES += QGC_ORG_NAME=\"\\\"auterion.com\\\"\"
DEFINES += QGC_ORG_DOMAIN=\"\\\"com.auterion\\\"\"

RESOURCES += \
    $$QGCROOT/custom/auterion.qrc

QGC_APP_NAME        = "AuterionQGC"
QGC_ORG_NAME        = "Auterion"
QGC_ORG_DOMAIN      = "com.auterion"
QGC_APP_DESCRIPTION = "Auterion Ground Control"
QGC_APP_COPYRIGHT   = "Copyright (C) 2017 Auterion. All rights reserved."

MacBuild {
    QMAKE_INFO_PLIST    = $$PWD/macOS/Info.plist
    ICON                = $$PWD/macOS/icon.icns
    OTHER_FILES        -= $$QGCROOT/Custom-Info.plist
    OTHER_FILES        += $$PWD/macOS/Info.plist
}

WindowsBuild {
    VERSION             = $${AUTERION_QGC_VERSION}.0
    QGCWINROOT          = $$replace(QGCROOT, "/", "\\")
    RC_ICONS            = $$QGCWINROOT\\custom\\Windows\\icon.ico
    QGC_INSTALLER_ICON          = $$QGCWINROOT\\custom\\Windows\\icon.ico
    QGC_INSTALLER_HEADER_BITMAP = $$QGCWINROOT\\custom\\Windows\\banner.bmp
    ReleaseBuild {
        QMAKE_CFLAGS_RELEASE   += /Gy /Ox
        QMAKE_CXXFLAGS_RELEASE += /Gy /Ox
        # Eliminate duplicate COMDATs
        QMAKE_LFLAGS_RELEASE   += /OPT:ICF /LTCG
    }
}

SOURCES += \
    $$PWD/src/AuterionPlugin.cc \
    $$PWD/src/AuterionQuickInterface.cc

HEADERS += \
    $$PWD/src/AuterionPlugin.h \
    $$PWD/src/AuterionQuickInterface.h

INCLUDEPATH += \
    $$PWD/src \

equals(QT_MAJOR_VERSION, 5) {
    greaterThan(QT_MINOR_VERSION, 5) {
        ReleaseBuild {
            QT      += qml-private
            CONFIG  += qtquickcompiler
            message("Using Qt Quick Compiler")
        }
    }
}
