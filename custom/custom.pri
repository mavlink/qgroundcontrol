message("Adding Auterion Plugin")

exists($$PWD/custom/custom.pri) {
    # Nested Custom Build
    message("Found nested custom build")
    include($$PWD/custom/custom.pri)
} else {

#
# Standard Auterion custom build
#

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

    message(Auterion GS Version: $${AUTERION_QGC_VERSION})

    #   MAVLink Development
    exists($$PWD/mavlink_dev) {
        MAVLINKPATH_REL = $$PWD/mavlink_dev
        MAVLINKPATH = $$PWD/mavlink_dev
        message($$sprintf("Using user-supplied mavlink development path '%1'", $$MAVLINKPATH))
    }

    #   Disable APM support
    MAVLINK_CONF = common
    CONFIG  += QGC_DISABLE_APM_MAVLINK

    #   Disable UVC support
    DEFINES += QGC_DISABLE_UVC

    #   We are a native QML app
    #CONFIG  += MobileBuild
    #DEFINES += __mobile__

    #   Branding

    DEFINES += CUSTOMHEADER=\"\\\"AuterionPlugin.h\\\"\"
    DEFINES += CUSTOMCLASS=AuterionPlugin

    CONFIG  += QGC_DISABLE_APM_PLUGIN QGC_DISABLE_APM_PLUGIN_FACTORY QGC_DISABLE_PX4_PLUGIN_FACTORY

    TARGET   = AuterionGS
    DEFINES += QGC_APPLICATION_NAME=\"\\\"AuterionGS\\\"\"

    DEFINES += QGC_ORG_NAME=\"\\\"auterion.com\\\"\"
    DEFINES += QGC_ORG_DOMAIN=\"\\\"com.auterion\\\"\"

    RESOURCES += \
        $$QGCROOT/custom/auterion.qrc

    QGC_APP_NAME        = "Auterion GS"
    QGC_BINARY_NAME     = "AuterionGS"
    QGC_ORG_NAME        = "Auterion"
    QGC_ORG_DOMAIN      = "com.auterion"
    QGC_APP_DESCRIPTION = "Auterion Ground Station"
    QGC_APP_COPYRIGHT   = "Copyright (C) 2018 Auterion AG. All rights reserved."

    MacBuild {
        QMAKE_INFO_PLIST    = $$PWD/macOS/AGSInfo.plist
        ICON                = $$PWD/macOS/icon.icns
        OTHER_FILES        += $$PWD/macOS/AGSInfo.plist
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

    iOSBuild {
        CONFIG               += DISABLE_BUILTIN_IOS
        LIBS                 += -framework AVFoundation
        #-- Info.plist (need an "official" one for the App Store)
        ForAppStore {
            message(App Store Build)
            #-- Create official, versioned Info.plist
            APP_STORE = $$system(cd $${BASEDIR} && $${BASEDIR}/tools/update_ios_version.sh $$PWD/ios/iOSForAppStore-Info-Source.plist $$PWD/ios/iOSForAppStore-Info.plist)
            APP_ERROR = $$find(APP_STORE, "Error")
            count(APP_ERROR, 1) {
                error("Error building .plist file. 'ForAppStore' builds are only possible through the official build system.")
            }
            QMAKE_INFO_PLIST  = $$PWD/ios/iOSForAppStore-Info.plist
            OTHER_FILES      += $$PWD/ios/iOSForAppStore-Info.plist
        } else {
            QMAKE_INFO_PLIST  = $$PWD/ios/iOS-Info.plist
            OTHER_FILES      += $$PWD/ios/iOS-Info.plist
        }
        QMAKE_ASSET_CATALOGS += $$PWD/ios/Images.xcassets
        BUNDLE.files          = $$PWD/ios/QGCLaunchScreen.xib $$QMAKE_INFO_PLIST
        QMAKE_BUNDLE_DATA    += BUNDLE
    }

    # Multimedia is disabled for non UVC builds but we need it.
    contains (DEFINES, QGC_DISABLE_UVC) {
        QT += \
            multimedia
    }

    QML_IMPORT_PATH += \
        $$QGCROOT/custom/res

    SOURCES += \
        $$PWD/src/AuterionPlugin.cc \
        $$PWD/src/AuterionQuickInterface.cc

    HEADERS += \
        $$PWD/src/AuterionPlugin.h \
        $$PWD/src/AuterionQuickInterface.h

    INCLUDEPATH += \
        $$PWD/src \

    #-------------------------------------------------------------------------------------
    # Firmware/AutoPilot Plugin

    INCLUDEPATH += \
        $$QGCROOT/custom/src/FirmwarePlugin \
        $$QGCROOT/custom/src/AutoPilotPlugin

    HEADERS+= \
        $$QGCROOT/custom/src/AutoPilotPlugin/AuterionAutoPilotPlugin.h \
        $$QGCROOT/custom/src/FirmwarePlugin/AuterionCameraControl.h \
        $$QGCROOT/custom/src/FirmwarePlugin/AuterionCameraManager.h \
        $$QGCROOT/custom/src/FirmwarePlugin/AuterionFirmwarePlugin.h \
        $$QGCROOT/custom/src/FirmwarePlugin/AuterionFirmwarePluginFactory.h \

    SOURCES += \
        $$QGCROOT/custom/src/AutoPilotPlugin/AuterionAutoPilotPlugin.cc \
        $$QGCROOT/custom/src/FirmwarePlugin/AuterionCameraControl.cc \
        $$QGCROOT/custom/src/FirmwarePlugin/AuterionCameraManager.cc \
        $$QGCROOT/custom/src/FirmwarePlugin/AuterionFirmwarePlugin.cc \
        $$QGCROOT/custom/src/FirmwarePlugin/AuterionFirmwarePluginFactory.cc \

    #-------------------------------------------------------------------------------------
    # Android

    AndroidBuild {
        CONFIG += DISABLE_BUILTIN_ANDROID
        CONFIG += QGC_DISABLE_INSTALLER_SETUP
        ANDROID_EXTRA_LIBS += $${PLUGIN_SOURCE}
        include($$QGCROOT/libs/qtandroidserialport/src/qtandroidserialport.pri)
        message("Adding Custom Serial Java Classes")
        QT += androidextras
        ANDROID_PACKAGE_SOURCE_DIR = $$QGCROOT/custom/android
        OTHER_FILES += \
            $$QGCROOT/custom/android/AndroidManifest.xml \
            $$QGCROOT/custom/android/res/xml/device_filter.xml \
            $$QGCROOT/custom/android/src/com/hoho/android/usbserial/driver/CdcAcmSerialDriver.java \
            $$QGCROOT/custom/android/src/com/hoho/android/usbserial/driver/CommonUsbSerialDriver.java \
            $$QGCROOT/custom/android/src/com/hoho/android/usbserial/driver/Cp2102SerialDriver.java \
            $$QGCROOT/custom/android/src/com/hoho/android/usbserial/driver/FtdiSerialDriver.java \
            $$QGCROOT/custom/android/src/com/hoho/android/usbserial/driver/ProlificSerialDriver.java \
            $$QGCROOT/custom/android/src/com/hoho/android/usbserial/driver/UsbId.java \
            $$QGCROOT/custom/android/src/com/hoho/android/usbserial/driver/UsbSerialDriver.java \
            $$QGCROOT/custom/android/src/com/hoho/android/usbserial/driver/UsbSerialProber.java \
            $$QGCROOT/custom/android/src/com/hoho/android/usbserial/driver/UsbSerialRuntimeException.java \
            $$QGCROOT/custom/android/src/org/mavlink/qgroundcontrol/QGCActivity.java \
            $$QGCROOT/custom/android/src/org/mavlink/qgroundcontrol/UsbIoManager.java \
            $$QGCROOT/custom/android/src/org/mavlink/qgroundcontrol/TaiSync.java

        DISTFILES += \
            $$QGCROOT/custom/android/gradle/wrapper/gradle-wrapper.jar \
            $$QGCROOT/custom/android/gradlew \
            $$QGCROOT/custom/android/res/values/libs.xml \
            $$QGCROOT/custom/android/build.gradle \
            $$QGCROOT/custom/android/gradle/wrapper/gradle-wrapper.properties \
            $$QGCROOT/custom/android/gradlew.bat

        include(customQGCInstaller.pri)
    }

    #-------------------------------------------------------------------------------------
    # Custom setup
    CONFIG += QGC_DISABLE_BUILD_SETUP
    include($$QGCROOT/QGCSetup.pri)
    LinxuBuild {
        QMAKE_POST_LINK += && $$QMAKE_COPY $$BASEDIR/deploy/qgroundcontrol-start.sh $$DESTDIR
        QMAKE_POST_LINK += && $$QMAKE_COPY $$BASEDIR/custom/deploy/qgroundcontrol.desktop $$DESTDIR
        QMAKE_POST_LINK += && $$QMAKE_COPY $$BASEDIR/custom/res/src/Auterion_Icon.png $$DESTDIR/qgroundcontrol.png
    }
}
