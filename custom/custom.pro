message("Adding Yuneec Typhoon H plugin")

linux : android-g++ {
    equals(ANDROID_TARGET_ARCH, x86)  {
        CONFIG  += AndroidBuild MobileBuild
        DEFINES += __android__
        DEFINES += __STDC_LIMIT_MACROS
        CONFIG += Androidx86Build
        DEFINES += __androidx86__
        DEFINES += __mobile__
        message("Android x86 build")
    } else {
        error("Unsuported Android toolchain, only x86 supported")
    }
} else {
    error("Unsuported Platform, only Android x86 supported")
}

CONFIG(debug, debug|release) {
    message(Debug flavor)
    CONFIG += DebugBuild
} else:CONFIG(release, debug|release) {
    message(Release flavor)
    CONFIG += ReleaseBuild
} else {
    error(Unsupported build flavor)
}

DebugBuild {
    DESTDIR  = $${OUT_PWD}/debug
} else {
    DESTDIR  = $${OUT_PWD}/release
}

TEMPLATE = lib
CONFIG  += plugin

DESTDIR = $$fromfile($$PWD/custom-config.pri, PLUGIN_DESTDIR)
TARGET  = $$fromfile($$PWD/custom-config.pri, PLUGIN_TARGET)

CONFIG += \
    thread \
    c++11 \

QT += \
    concurrent \
    gui \
    network \
    qml \
    quick \
    svg \

RESOURCES += \
    typhoonh.qrc \

SOURCES += \
    $$PWD/src/m4.cc \
    $$PWD/src/m4serial.cc \
    $$PWD/src/m4util.cc \
    $$PWD/src/typhoonh.cc \

HEADERS += \
    $$PWD/src/m4.h \
    $$PWD/src/m4channeldata.h \
    $$PWD/src/m4def.h \
    $$PWD/src/m4serial.h \
    $$PWD/src/m4util.h \
    $$PWD/src/typhoonh.h \

INCLUDEPATH += \
    $$PWD/src \
    $$PWD/../api

OTHER_FILES = \
    $$PWD/src/typhoonh.json \
    $$PWD/custom-config.pri

