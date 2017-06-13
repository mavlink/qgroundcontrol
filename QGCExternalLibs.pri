#
# [REQUIRED] Add support for <inttypes.h> to Windows.
#
WindowsBuild {
    INCLUDEPATH += libs/lib/msinttypes
}

#
# [REQUIRED] Add support for the MAVLink communications protocol.
# Mavlink dialect is hardwired to arudpilotmega for now. The reason being
# the current codebase supports both PX4 and APM flight stack. PX4 flight stack
# only usese common mavlink specifications, whereas APM flight stack uses custom
# mavlink specifications which add to common. So by using the adupilotmega dialect
# QGC can support both in the same codebase.
#
# Once the mavlink helper routines include support for multiple dialects within
# a single compiled codebase this hardwiring of dialect can go away. But until then
# this "workaround" is needed.

MAVLINKPATH_REL = libs/mavlink/include/mavlink/v2.0
MAVLINKPATH = $$BASEDIR/$$MAVLINKPATH_REL
MAVLINK_CONF = ardupilotmega
DEFINES += MAVLINK_NO_DATA

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
    INCLUDEPATH += \
        $$BASEDIR/libs/lib/sdl2/msvc/include \

    LIBS += \
        -L$$BASEDIR/libs/lib/sdl2/msvc/lib/x86 \
        -lSDL2main \
        -lSDL2
}

##
# [OPTIONAL] Speech synthesis library support.
# Can be forcibly disabled by adding a `DEFINES+=DISABLE_SPEECH` argument to qmake.
# Linux support requires the eSpeak speech synthesizer (espeak).
# Mac support is provided in Snow Leopard and newer (10.6+)
# Windows is supported as of Windows 7
#
contains (DEFINES, DISABLE_SPEECH) {
    message("Skipping support for speech output (manual override from command line)")
    DEFINES -= DISABLE_SPEECH
# Otherwise the user can still disable this feature in the user_config.pri file.
} else:exists(user_config.pri):infile(user_config.pri, DEFINES, DISABLE_SPEECH) {
    message("Skipping support for speech output (manual override from user_config.pri)")
} else:LinuxBuild {
    exists(/usr/include/espeak) | exists(/usr/local/include/espeak) {
        message("Including support for speech output")
        DEFINES += QGC_SPEECH_ENABLED
        LIBS += \
        -lespeak
    } else {
        warning("Skipping support for speech output (missing libraries, see README)")
    }
}
# Mac support is built into OS 10.6+.
else:MacBuild|iOSBuild {
    message("Including support for speech output")
    DEFINES += QGC_SPEECH_ENABLED
}
# Windows supports speech through native API.
else:WindowsBuild {
    message("Including support for speech output")
    DEFINES += QGC_SPEECH_ENABLED
    LIBS    += -lOle32
}
# Android supports speech through native (Java) API.
else:AndroidBuild {
    message("Including support for speech output")
    DEFINES += QGC_SPEECH_ENABLED
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

