#
# [REQUIRED] Add support for <inttypes.h> to Windows.
#
WindowsBuild {
    INCLUDEPATH += libs/lib/msinttypes
}

#
# [REQUIRED] Add support for the MAVLink communications protocol.
# Some logic is involved here in selecting the proper dialect for
# the selected autopilot system.
#
# If the user config file exists, it will be included. If this file
# specifies the MAVLINK_CONF variable with a MAVLink dialect, support 
# for it will be compiled in to QGC. It will also create a 
# QGC_USE_{AUTOPILOT_NAME}_MESSAGES macro for use within the actual code.
#
MAVLINKPATH_REL = libs/mavlink/include/mavlink/v1.0
MAVLINKPATH = $$BASEDIR/$$MAVLINKPATH_REL
DEFINES += MAVLINK_NO_DATA

# First we select the dialect, checking for valid user selection
# Users can override all other settings by specifying MAVLINK_CONF as an argument to qmake
!isEmpty(MAVLINK_CONF) {
    message($$sprintf("Using MAVLink dialect '%1' specified at the command line.", $$MAVLINK_CONF))
}
# Otherwise they can specify MAVLINK_CONF within user_config.pri
else:exists(user_config.pri):infile(user_config.pri, MAVLINK_CONF) {
    MAVLINK_CONF = $$fromfile(user_config.pri, MAVLINK_CONF)
    !isEmpty(MAVLINK_CONF) {
        message($$sprintf("Using MAVLink dialect '%1' specified in user_config.pri", $$MAVLINK_CONF))
    }
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
        warning("No MAVLink dialect specified, only common messages supported.")
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
# [REQUIRED] OPMapControl library from OpenPilot. Provides 2D mapping functionality.
#
include(libs/opmapcontrol/opmapcontrol_external.pri)

INCLUDEPATH += \
    libs/utils \
    libs \
    libs/opmapcontrol

#
# [REQUIRED] QWT plotting library dependency. Provides plotting capabilities.
#
include(libs/qwt.pri)
DEPENDPATH += libs/qwt
INCLUDEPATH += libs/qwt

#
# [OPTIONAL] XBee wireless support. This is not necessary for basic serial/UART communications.
# It's only required for speaking directly to the Xbee using their proprietary API.
# Unsupported on Mac.
# Installation on Windows is unnecessary, as we just link to our included .dlls directly.
# Installing on Linux involves running `make;sudo make install` in `libs/thirdParty/libxbee`
# Uninstalling from Linux can be done with `sudo make uninstall`.
#
XBEE_DEPENDENT_HEADERS += \
	src/comm/XbeeLinkInterface.h \
	src/comm/XbeeLink.h \
	src/comm/HexSpinBox.h \
	src/ui/XbeeConfigurationWindow.h \
	src/comm/CallConv.h
XBEE_DEPENDENT_SOURCES += \
	src/comm/XbeeLink.cpp \
	src/comm/HexSpinBox.cpp \
	src/ui/XbeeConfigurationWindow.cpp
XBEE_DEFINES = QGC_XBEE_ENABLED

contains(DEFINES, DISABLE_XBEE) {
	message("Skipping support for native XBee API (manual override from command line)")
	DEFINES -= DISABLE_XBEE
# Otherwise the user can still disable this feature in the user_config.pri file.
} else:exists(user_config.pri):infile(user_config.pri, DEFINES, DISABLE_XBEE) {
    message("Skipping support for native XBee API (manual override from user_config.pri)")
} else:LinuxBuild {
        linux-g++-64 {
            message("Skipping support for XBee API (64-bit Linux builds not supported)")
        } else:exists(/usr/include/xbee.h) {
		message("Including support for XBee API")

		HEADERS += $$XBEE_DEPENDENT_HEADERS
		SOURCES += $$XBEE_DEPENDENT_SOURCES
		DEFINES += $$XBEE_DEFINES
		LIBS += -L/usr/lib -lxbee
	} else {
		warning("Skipping support for XBee API (missing libraries, see README)")
	}
} else:WindowsBuild {
	message("Including support for XBee API")
	HEADERS += $$XBEE_DEPENDENT_HEADERS
	SOURCES += $$XBEE_DEPENDENT_SOURCES
	DEFINES += $$XBEE_DEFINES
	INCLUDEPATH += libs/thirdParty/libxbee
        LIBS += -l$$BASEDIR/libs/thirdParty/libxbee/lib/libxbee
} else {
	message("Skipping support for XBee API (unsupported platform)")
}

#
# [OPTIONAL] Magellan 3DxWare library. Provides support for 3DConnexion's 3D mice.
#
contains(DEFINES, DISABLE_3DMOUSE) {
	message("Skipping support for 3DConnexion mice (manual override from command line)")
	DEFINES -= DISABLE_3DMOUSE
# Otherwise the user can still disable this feature in the user_config.pri file.
} else:exists(user_config.pri):infile(user_config.pri, DEFINES, DISABLE_3DMOUSE) {
    message("Skipping support for 3DConnexion mice (manual override from user_config.pri)")
} else:LinuxBuild {
	exists(/usr/local/lib/libxdrvlib.so) {
		message("Including support for 3DConnexion mice")

                DEFINES += \
		QGC_MOUSE_ENABLED_LINUX \
                ParameterCheck
                # Hack: Has to be defined for magellan usage

		HEADERS += src/input/Mouse6dofInput.h
		SOURCES += src/input/Mouse6dofInput.cpp
		LIBS += -L/usr/local/lib/ -lxdrvlib
	} else {
		warning("Skipping support for 3DConnexion mice (missing libraries, see README)")
	}
} else:WindowsBuild {
    message("Including support for 3DConnexion mice")

    DEFINES += QGC_MOUSE_ENABLED_WIN

    INCLUDEPATH += libs/thirdParty/3DMouse/win

    HEADERS += \
        libs/thirdParty/3DMouse/win/I3dMouseParams.h \
        libs/thirdParty/3DMouse/win/MouseParameters.h \
        libs/thirdParty/3DMouse/win/Mouse3DInput.h \
        src/input/Mouse6dofInput.h

    SOURCES += \
        libs/thirdParty/3DMouse/win/MouseParameters.cpp \
        libs/thirdParty/3DMouse/win/Mouse3DInput.cpp \
        src/input/Mouse6dofInput.cpp
} else {
	message("Skipping support for 3DConnexion mice (unsupported platform)")
}

#
# [OPTIONAL] Opal RT-LAB Library. Provides integration with Opal-RT's RT-LAB simulator.
#
contains(DEFINES, DISABLE_RTLAB) {
	message("Skipping support for RT-LAB (manual override from command line)")
	DEFINES -= DISABLE_RTLAB
# Otherwise the user can still disable this feature in the user_config.pri file.
} else:exists(user_config.pri):infile(user_config.pri, DEFINES, DISABLE_RTLAB) {
    message("Skipping support for RT-LAB (manual override from user_config.pri)")
} else:WindowsBuild {
	exists(src/lib/opalrt/OpalApi.h) : exists(C:/OPAL-RT/RT-LAB7.2.4/Common/bin) {
		message("Including support for RT-LAB")

		DEFINES += QGC_RTLAB_ENABLED

		INCLUDEPATH +=
			src/lib/opalrt
			libs/lib/opal/include \

		FORMS += src/ui/OpalLinkSettings.ui

		HEADERS += \
			src/comm/OpalRT.h \
			src/comm/OpalLink.h \
			src/comm/Parameter.h \
			src/comm/QGCParamID.h \
			src/comm/ParameterList.h \
			src/ui/OpalLinkConfigurationWindow.h

		SOURCES += \
			src/comm/OpalRT.cc \
			src/comm/OpalLink.cc \
			src/comm/Parameter.cc \
			src/comm/QGCParamID.cc \
			src/comm/ParameterList.cc \
			src/ui/OpalLinkConfigurationWindow.cc

		LIBS += \
			-LC:/OPAL-RT/RT-LAB7.2.4/Common/bin \
			-lOpalApi
	} else {
		warning("Skipping support for RT-LAB (missing libraries, see README)")
	}
} else {
    message("Skipping support for RT-LAB (unsupported platform)")
}

#
# [REQUIRED] SDL dependency. Provides joystick/gamepad support.
# The SDL is packaged with QGC for the Mac and Windows. Linux support requires installing the SDL
# library (development libraries and static binaries).
#
MacBuild {
    INCLUDEPATH += \
        $$BASEDIR/libs/lib/Frameworks/SDL.framework/Headers

    LIBS += \
        -F$$BASEDIR/libs/lib/Frameworks \
        -framework SDL
} else:LinuxBuild {
	PKGCONFIG = sdl
} else:WindowsBuild {
	INCLUDEPATH += \
        $$BASEDIR/libs/lib/sdl/msvc/include \

	LIBS += \
        -L$$BASEDIR/libs/lib/sdl/msvc/lib \
        -lSDLmain \
        -lSDL
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
else:MacBuild {
    message("Including support for speech output")
    DEFINES += QGC_SPEECH_ENABLED
}
# Windows supports speech through native API.
else:WindowsBuild {
    message("Including support for speech output")
    DEFINES += QGC_SPEECH_ENABLED
    LIBS    += -lOle32
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

