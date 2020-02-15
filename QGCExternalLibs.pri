################################################################################
#
# (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
#
# QGroundControl is licensed according to the terms in the file
# COPYING.md in the root of the source code directory.
#
################################################################################

#
# [REQUIRED] Add support for <inttypes.h> to Windows.
#
WindowsBuild {
    INCLUDEPATH += libs/msinttypes
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
    CONFIG  += ArdupilotDisabled
} else {
    CONFIG  += ArdupilotEnabled
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
# [REQUIRED] SDL dependency. Provides joystick/gamepad support.
# The SDL is packaged with QGC for the Mac and Windows. Linux support requires installing the SDL
# library (development libraries and static binaries).
#
MacBuild {
    INCLUDEPATH += \
        $$BASEDIR/libs/Frameworks/SDL2.framework/Headers
    LIBS += \
        -F$$BASEDIR/libs/Frameworks \
        -framework SDL2
} else:LinuxBuild {
    PKGCONFIG = sdl2
} else:WindowsBuild {
    INCLUDEPATH += $$BASEDIR/libs/sdl2/msvc/include
    INCLUDEPATH += $$BASEDIR/libs/OpenSSL/Windows/x64/include
    LIBS += -L$$BASEDIR/libs/sdl2/msvc/lib/x64
    LIBS += -lSDL2
}

# Include Android OpenSSL libs
AndroidBuild {
    include($$BASEDIR/libs/OpenSSL/android_openssl/openssl.pri)
    message("ANDROID_EXTRA_LIBS")
    message($$ANDROID_TARGET_ARCH)
    message($$ANDROID_EXTRA_LIBS)
}

# Pairing
contains(DEFINES, QGC_ENABLE_PAIRING) {
    MacBuild {
        #- Pairing is generally not supported on macOS. This is here solely for development.
        exists(/usr/local/Cellar/openssl/1.0.2t/include) {
            INCLUDEPATH += /usr/local/Cellar/openssl/1.0.2t/include
            LIBS += -L/usr/local/Cellar/openssl/1.0.2t/lib
            LIBS += -lcrypto -lz
        } else {
            # There is some circular reference settings going on between QGCExternalLibs.pri and gqgroundcontrol.pro.
            # So this duplicates some of the enable/disable logic which would normally be in qgroundcontrol.pro.
            DEFINES -= QGC_ENABLE_PAIRING
        }
    } else:WindowsBuild {
        #- Pairing is not supported on Windows
        DEFINES -= QGC_ENABLE_PAIRING
    } else {
        LIBS += -lcrypto -lz
        AndroidBuild {
            contains(QT_ARCH, arm) {
                LIBS += $$ANDROID_EXTRA_LIBS
                INCLUDEPATH += $$BASEDIR/libs/OpenSSL/Android/arch-armeabi-v7a/include
            } else {
                LIBS += $$ANDROID_EXTRA_LIBS
                INCLUDEPATH += $$BASEDIR/libs/OpenSSL/Android/arch-x86/include
            }
        }
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
    AIRMAPD_PATH    = $$PWD/libs/airmapd
    AIRMAP_QT_PATH  = Qt.$${QT_MAJOR_VERSION}.$${QT_MINOR_VERSION}
    message('Looking for Airmap in folder "$${AIRMAPD_PATH}", variant: "$$AIRMAP_QT_PATH"')
    MacBuild {
        exists($${AIRMAPD_PATH}/macOS/$$AIRMAP_QT_PATH) {
            message("Including support for AirMap for macOS")
            LIBS += -L$${AIRMAPD_PATH}/macOS/$$AIRMAP_QT_PATH -lairmap-qt
            DEFINES += QGC_AIRMAP_ENABLED
        }
    } else:LinuxBuild {
        exists($${AIRMAPD_PATH}/linux/$$AIRMAP_QT_PATH) {
            message("Including support for AirMap for Linux")
            LIBS += -L$${AIRMAPD_PATH}/linux/$$AIRMAP_QT_PATH -lairmap-qt
            DEFINES += QGC_AIRMAP_ENABLED
        }
    } else {
        message("Skipping support for Airmap (unsupported platform)")
    }
    contains (DEFINES, QGC_AIRMAP_ENABLED) {
        INCLUDEPATH += \
            $${AIRMAPD_PATH}/include
    }
}
