#
# [REQUIRED] Add support for <inttypes.h> to Windows.
#
WindowsBuild {
    INCLUDEPATH += libs/lib/msinttypes
}

#
# [REQUIRED] Add support for the MAVLink communications protocol.
#
# By default MAVLink dialect is hardwired to arudpilotmega. The reason being
# the current codebase supports both PX4 and APM flight stack. PX4 flight stack
# only uses common MAVLink specifications, whereas APM flight stack uses custom
# MAVLink specifications which adds to common. So by using the adupilotmega dialect
# QGC can support both in the same codebase.

# Once the mavlink helper routines include support for multiple dialects within
# a single compiled codebase this hardwiring of dialect can go away. But until then
# this "workaround" is needed.

# In the mean time, it’s possible to define a completely different dialect by defining the
# location and name below.

# check for user defined settings in user_config.pri if not already set as qmake argument
isEmpty(MAVLINKPATH_REL) {
    exists(user_config.pri):infile(user_config.pri, MAVLINKPATH_REL) {
        MAVLINKPATH_REL = $$fromfile(user_config.pri, MAVLINKPATH_REL)
        message($$sprintf("Using user-supplied relativ mavlink path '%1' specified in user_config.pri", $$MAVLINKPATH_REL))
    } else {
        MAVLINKPATH_REL = libs/mavlink/include/mavlink/v2.0
    }
}

isEmpty(MAVLINKPATH) {
    exists(user_config.pri):infile(user_config.pri, MAVLINKPATH) {
        MAVLINKPATH     = $$fromfile(user_config.pri, MAVLINKPATH)
        message($$sprintf("Using user-supplied mavlink path '%1' specified in user_config.pri", $$MAVLINKPATH))
    } else {
        MAVLINKPATH     = $$BASEDIR/$$MAVLINKPATH_REL
    }
}

isEmpty(MAVLINK_CONF) {
    exists(user_config.pri):infile(user_config.pri, MAVLINK_CONF) {
        MAVLINK_CONF = $$fromfile(user_config.pri, MAVLINK_CONF)
        message($$sprintf("Using user-supplied mavlink dialect '%1' specified in user_config.pri", $$MAVLINK_CONF))
    } else {
        MAVLINK_CONF = ardupilotmega
    }
}

# If defined, all APM specific MAVLink messages are disabled
contains (CONFIG, QGC_DISABLE_APM_MAVLINK) {
    message("Disable APM MAVLink support")
    DEFINES += NO_ARDUPILOT_DIALECT
}

# First we select the dialect, checking for valid user selection
# Users can override all other settings by specifying MAVLINK_CONF as an argument to qmake
!isEmpty(MAVLINK_CONF) {
    message($$sprintf("Using MAVLink dialect '%1'.", $$MAVLINK_CONF))
}

# Then we add the proper include paths dependent on the dialect.
INCLUDEPATH += $$MAVLINKPATH

exists($$MAVLINKPATH/common) {
    !isEmpty(MAVLINK_CONF) {
        count(MAVLINK_CONF, 1) {
            exists($$MAVLINKPATH/$$MAVLINK_CONF) {
                INCLUDEPATH += $$MAVLINKPATH/$$MAVLINK_CONF
                DEFINES += $$sprintf('QGC_USE_%1_MESSAGES', $$upper($$MAVLINK_CONF))
            } else {
                error($$sprintf("MAVLink dialect '%1' does not exist at '%2'!", $$MAVLINK_CONF, $$MAVLINKPATH_REL))
            }
        } else {
            error(Only a single mavlink dialect can be specified in MAVLINK_CONF)
        }
    } else {
        INCLUDEPATH += $$MAVLINKPATH/common
    }
} else {
    error($$sprintf("MAVLink folder does not exist at '%1'! Run 'git submodule init && git submodule update' on the command line.",$$MAVLINKPATH_REL))
}

#
# [REQUIRED] EIGEN matrix library
# NOMINMAX constant required to make internal min/max work.
INCLUDEPATH += libs/eigen
DEFINES += NOMINMAX

#
# [REQUIRED] shapelib library
INCLUDEPATH += libs/shapelib
SOURCES += \
    libs/shapelib/shpopen.c \
    libs/shapelib/safileio.c

#
# [REQUIRED] QWT plotting library dependency. Provides plotting capabilities.
#
!MobileBuild {
include(libs/qwt.pri)
DEPENDPATH += libs/qwt
INCLUDEPATH += libs/qwt
}

#
# [REQUIRED] SDL dependency. Provides joystick/gamepad support.
# The SDL is packaged with QGC for the Mac and Windows. Linux support requires installing the SDL
# library (development libraries and static binaries).
#
MacBuild {
    INCLUDEPATH += \
        $$BASEDIR/libs/lib/Frameworks/SDL2.framework/Headers

    LIBS += \
        -F$$BASEDIR/libs/lib/Frameworks \
        -framework SDL2
} else:LinuxBuild {
    PKGCONFIG = sdl2
} else:WindowsBuild {
    INCLUDEPATH += $$BASEDIR/libs/lib/sdl2/msvc/include

    contains(QT_ARCH, i386) {
        LIBS += -L$$BASEDIR/libs/lib/sdl2/msvc/lib/x86
    } else {
        LIBS += -L$$BASEDIR/libs/lib/sdl2/msvc/lib/x64
    }
    LIBS += \
        -lSDL2main \
        -lSDL2
}

AndroidBuild {
    contains(QT_ARCH, arm) {
        ANDROID_EXTRA_LIBS += $$BASEDIR/libs/AndroidOpenSSL/arch-armeabi-v7a/lib/libcrypto.so
        ANDROID_EXTRA_LIBS += $$BASEDIR/libs/AndroidOpenSSL/arch-armeabi-v7a/lib/libssl.so
    } else {
        ANDROID_EXTRA_LIBS += $$BASEDIR/libs/AndroidOpenSSL/arch-x86/lib/libcrypto.so
        ANDROID_EXTRA_LIBS += $$BASEDIR/libs/AndroidOpenSSL/arch-x86/lib/libssl.so
    }
}

#
# [OPTIONAL] Zeroconf for UDP links
#
contains (DEFINES, DISABLE_ZEROCONF) {
    message("Skipping support for Zeroconf (manual override from command line)")
    DEFINES -= DISABLE_ZEROCONF
# Otherwise the user can still disable this feature in the user_config.pri file.
} else:exists(user_config.pri):infile(user_config.pri, DEFINES, DISABLE_ZEROCONF) {
    message("Skipping support for Zeroconf (manual override from user_config.pri)")
# Mac support is built into OS
} else:MacBuild|iOSBuild {
    message("Including support for Zeroconf (Bonjour)")
    DEFINES += QGC_ZEROCONF_ENABLED
} else {
    message("Skipping support for Zeroconf (unsupported platform)")
}


#
# [OPTIONAL] AirMap Support
#
contains (DEFINES, DISABLE_AIRMAP) {
    message("Skipping support for AirMap (manual override from command line)")
# Otherwise the user can still disable this feature in the user_config.pri file.
} else:exists(user_config.pri):infile(user_config.pri, DEFINES, DISABLE_AIRMAP) {
    message("Skipping support for AirMap (manual override from user_config.pri)")
} else {
    AIRMAPD_PATH = $$PWD/libs/airmapd
    contains(QT_VERSION, ˆ5\\.11\..*) {
        MacBuild {
            exists($${AIRMAPD_PATH}/macOS/Qt.5.11.0) {
                message("Including support for AirMap for macOS")
                LIBS += -L$${AIRMAPD_PATH}/macOS/Qt.5.11.0 -lairmap-qt
                DEFINES += QGC_AIRMAP_ENABLED
            }
        } else:LinuxBuild {
            exists($${AIRMAPD_PATH}/linux/Qt.5.11.0) {
                message("Including support for AirMap for Linux")
                LIBS += -L$${AIRMAPD_PATH}/linux/Qt.5.11.0 -lairmap-qt
                DEFINES += QGC_AIRMAP_ENABLED
            }
        } else {
            message("Skipping support for Airmap (unsupported platform)")
        }
    }
    contains (DEFINES, QGC_AIRMAP_ENABLED) {
        INCLUDEPATH += \
            $${AIRMAPD_PATH}/include
    }
}
