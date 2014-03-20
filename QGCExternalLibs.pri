#
# [REQUIRED] Tell the Linux build to look in a few additional places for libs
#
LinuxBuild {
	INCLUDEPATH += \
        /usr/include \
        /usr/local/include

	LIBS += \
		-L/usr/lib

    linux-g++-64 {
        LIBS += \
            -L/usr/local/lib64 \
            -L/usr/lib64
	}
}

#
# [REQUIRED] Add support for <inttypes.h> to Windows.
#
WindowsBuild {
    INCLUDEPATH += libs/lib/msinttypes
}

#
# [OPTIONAL] QUpgrade support.
#
# Allow the user to override QUpgrade compilation through a DISABLE_QUPGRADE
# define like: `qmake DEFINES=DISABLE_QUPGRADE`
contains(DEFINES, DISABLE_QUPGRADE) {
    message("Skipping support for QUpgrade (manual override from command line)")
    DEFINES -= DISABLE_QUPGRADE
}
# Otherwise the user can still disable this feature in the user_config.pri file.
else:infile(user_config.pri, DEFINES, DISABLE_QUPGRADE) {
    message("Skipping support for QUpgrade (manual override from user_config.pri)")
}
# If the QUpgrade submodule has been initialized, build in support by default.
else:exists(qupgrade/.git) {
    message("Including support for QUpgrade")

    DEFINES += QGC_QUPGRADE_ENABLED

    INCLUDEPATH += qupgrade/src/apps/qupgrade

    FORMS += \
        qupgrade/src/apps/qupgrade/dialog_bare.ui \
        qupgrade/src/apps/qupgrade/boardwidget.ui

    HEADERS += \
        qupgrade/src/apps/qupgrade/qgcfirmwareupgradeworker.h \
        qupgrade/src/apps/qupgrade/uploader.h \
        qupgrade/src/apps/qupgrade/dialog_bare.h \
        qupgrade/src/apps/qupgrade/boardwidget.h

    SOURCES += \
        qupgrade/src/apps/qupgrade/qgcfirmwareupgradeworker.cpp \
        qupgrade/src/apps/qupgrade/uploader.cpp \
        qupgrade/src/apps/qupgrade/dialog_bare.cpp \
        qupgrade/src/apps/qupgrade/boardwidget.cpp

    RESOURCES += \
        qupgrade/qupgrade.qrc

    LinuxBuild:CONFIG += qesp_linux_udev

    include(qupgrade/libs/qextserialport/src/qextserialport.pri)
}
# Otherwise notify the user and don't compile it.
else {
    warning("Skipping support for QUpgrade (missing submodule, see README)")
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
else:infile(user_config.pri, MAVLINK_CONF) {
    MAVLINK_CONF = $$fromfile(user_config.pri, MAVLINK_CONF)
    !isEmpty(MAVLINK_CONF) {
        message($$sprintf("Using MAVLink dialect '%1' specified in user_config.pri", $$MAVLINK_CONF))
    }
}
# If no valid user selection is found, default to the ardupilotmega if it's available.
# Note: This can be a list of several dialects.
else {
    MAVLINK_CONF=ardupilotmega
    message($$sprintf("Using default MAVLink dialect '%1'.", $$MAVLINK_CONF))
}

# Then we add the proper include paths dependent on the dialect.
INCLUDEPATH += $$MAVLINKPATH
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

#
# [DEPRECATED] MAVLink generator UI. Provides a GUI interface for generating MAVLink dialects.
# Replaced by mavgenerator.py within the MAVLink project.
#
contains(DEFINES, ENABLE_MAVGEN) {
	warning("Including support for MAVLink generator GUI (manual override from command line, CAUTION: deprecated)")
} else:infile(user_config.pri, DEFINES, ENABLE_MAVGEN) {
	DEFINES += ENABLE_MAVGEN # infile doesn't automatically include everything in the specified file
	warning("Including support for MAVLink generator GUI (manual override from user_config.pri, CAUTION: deprecated)")
}

contains(DEFINES, ENABLE_MAVGEN) {
	# Rename the macro to be consistent with other QGC feature existance macros.
	DEFINES -= ENABLE_MAVGEN
	DEFINES += QGC_MAVGEN_ENABLED
	DEPENDPATH += \
		src/apps/mavlinkgen

	INCLUDEPATH += \
		src/apps/mavlinkgen \
		src/apps/mavlinkgen/ui \
		src/apps/mavlinkgen/generator

	include(src/apps/mavlinkgen/mavlinkgen.pri)
} else {
	message("Skipping support for MAVLink generator GUI (deprecated, see README)")
}

#
# [OPTIONAL] OpenSceneGraph
# Allow the user to override OpenSceneGraph compilation through a DISABLE_OPEN_SCENE_GRAPH
# define like: `qmake DEFINES=DISABLE_OPEN_SCENE_GRAPH`
contains(DEFINES, DISABLE_OPEN_SCENE_GRAPH) {
    message("Skipping support for OpenSceneGraph (manual override from command line)")
    DEFINES -= DISABLE_OPEN_SCENE_GRAPH
}
# Otherwise the user can still disable this feature in the user_config.pri file.
else:infile(user_config.pri, DEFINES, DISABLE_OPEN_SCENE_GRAPH) {
    message("Skipping support for OpenSceneGraph (manual override from user_config.pri)")
}
else:MacBuild {
    # GLUT and OpenSceneGraph are part of standard install on Mac
	message("Including support for OpenSceneGraph")
	CONFIG += OSGDependency

    INCLUDEPATH += \
        $$BASEDIR/libs/lib/mac64/include

	LIBS += \
        -L$$BASEDIR/libs/lib/mac64/lib \
        -losgWidget
} else:LinuxBuild {
	exists(/usr/include/osg) | exists(/usr/local/include/osg) {
		message("Including support for OpenSceneGraph")
        CONFIG += OSGDependency
        exists(/usr/include/osg/osgQt) | exists(/usr/include/osgQt) | exists(/usr/local/include/osg/osgQt) | exists(/usr/local/include/osgQt) {
            message("Including support for Linux OpenSceneGraph Qt")
            LIBS += -losgQt
            DEFINES += QGC_OSG_QT_ENABLED
        } else {
            warning("Skipping support for Linux OpenSceneGraph Qt (missing libraries, see README)")
        }
	} else {
		warning("Skipping support for OpenSceneGraph (missing libraries, see README)")
	}
} else:WindowsBuild {
	exists($$BASEDIR/libs/lib/osg123) {
		message("Including support for OpenSceneGraph")
        CONFIG += OSGDependency

		INCLUDEPATH += \
            $$BASEDIR/libs/lib/osgEarth/win32/include \
			$$BASEDIR/libs/lib/osgEarth_3rdparty/win32/OpenSceneGraph-2.8.2/include

		LIBS += -L$$BASEDIR/libs/lib/osgEarth_3rdparty/win32/OpenSceneGraph-2.8.2/lib
	} else {
		warning("Skipping support for OpenSceneGraph (missing libraries, see README)")
	}
} else {
    message("Skipping support for OpenSceneGraph (unsupported platform)")
}

OSGDependency {
	DEFINES += QGC_OSG_ENABLED
    
    LIBS += \
        -losg \
        -losgViewer \
        -losgGA \
        -losgDB \
        -losgText \
        -lOpenThreads

    HEADERS += \
        src/ui/map3D/gpl.h \
        src/ui/map3D/CameraParams.h \
        src/ui/map3D/ViewParamWidget.h \
        src/ui/map3D/SystemContainer.h \
        src/ui/map3D/SystemViewParams.h \
        src/ui/map3D/GlobalViewParams.h \
        src/ui/map3D/SystemGroupNode.h \
        src/ui/map3D/Q3DWidget.h \
        src/ui/map3D/GCManipulator.h \
        src/ui/map3D/ImageWindowGeode.h \
        src/ui/map3D/PixhawkCheetahNode.h \
        src/ui/map3D/Pixhawk3DWidget.h \
        src/ui/map3D/Q3DWidgetFactory.h \
        src/ui/map3D/WebImageCache.h \
        src/ui/map3D/WebImage.h \
        src/ui/map3D/TextureCache.h \
        src/ui/map3D/Texture.h \
        src/ui/map3D/Imagery.h \
        src/ui/map3D/HUDScaleGeode.h \
        src/ui/map3D/WaypointGroupNode.h \
        src/ui/map3D/TerrainParamDialog.h \
        src/ui/map3D/ImageryParamDialog.h
        
    SOURCES += \
        src/ui/map3D/gpl.cc \
        src/ui/map3D/CameraParams.cc \
        src/ui/map3D/ViewParamWidget.cc \
        src/ui/map3D/SystemContainer.cc \
        src/ui/map3D/SystemViewParams.cc \
        src/ui/map3D/GlobalViewParams.cc \
        src/ui/map3D/SystemGroupNode.cc \
        src/ui/map3D/Q3DWidget.cc \
        src/ui/map3D/ImageWindowGeode.cc \
        src/ui/map3D/GCManipulator.cc \
        src/ui/map3D/PixhawkCheetahNode.cc \
        src/ui/map3D/Pixhawk3DWidget.cc \
        src/ui/map3D/Q3DWidgetFactory.cc \
        src/ui/map3D/WebImageCache.cc \
        src/ui/map3D/WebImage.cc \
        src/ui/map3D/TextureCache.cc \
        src/ui/map3D/Texture.cc \
        src/ui/map3D/Imagery.cc \
        src/ui/map3D/HUDScaleGeode.cc \
        src/ui/map3D/WaypointGroupNode.cc \
        src/ui/map3D/TerrainParamDialog.cc \
        src/ui/map3D/ImageryParamDialog.cc
}

#
# [OPTIONAL] Google Earth dependency. Provides Google Earth view to supplement 2D map view.
# Only supported on Mac and Windows where Google Earth can be installed.
#
contains(DEFINES, DISABLE_GOOGLE_EARTH) {
    message("Skipping support for Google Earth view (manual override from command line)")
    DEFINES -= DISABLE_GOOGLE_EARTH
}
# Otherwise the user can still disable this feature in the user_config.pri file.
else:infile(user_config.pri, DEFINES, DISABLE_GOOGLE_EARTH) {
    message("Skipping support for Google Earth view (manual override from user_config.pri)")
} else:MacBuild {
    message("Including support for Google Earth view")
    DEFINES += QGC_GOOGLE_EARTH_ENABLED
    HEADERS += src/ui/map3D/QGCGoogleEarthView.h
    SOURCES += src/ui/map3D/QGCGoogleEarthView.cc
} else:WindowsBuild {
    message("Including support for Google Earth view")
    DEFINES += QGC_GOOGLE_EARTH_ENABLED
    HEADERS += src/ui/map3D/QGCGoogleEarthView.h
    SOURCES += src/ui/map3D/QGCGoogleEarthView.cc
    CONFIG += qaxcontainer
} else {
    message("Skipping support for Google Earth view (unsupported platform)")
}

#
# [OPTIONAL] Protcol Buffers for PixHawk
#
LinuxBuild : contains(MAVLINK_DIALECT, pixhawk) {
    exists(/usr/local/include/google/protobuf) | exists(/usr/include/google/protobuf) {
        message("Including support for Protocol Buffers")

        DEFINES += QGC_PROTOBUF_ENABLED

        LIBS += \
            -lprotobuf \
            -lprotobuf-lite \
            -lprotoc

        HEADERS += \
            libs/mavlink/include/mavlink/v1.0/pixhawk/pixhawk.pb.h \
            src/ui/map3D/ObstacleGroupNode.h \
            src/ui/map3D/GLOverlayGeode.h

        SOURCES += \
            libs/mavlink/share/mavlink/src/v1.0/pixhawk/pixhawk.pb.cc \
            src/ui/map3D/ObstacleGroupNode.cc \
            src/ui/map3D/GLOverlayGeode.cc
    } else {
        warning("Skipping support for Protocol Buffers (missing libraries, see README)")
    }
} else {
    message("Skipping support for Protocol Buffers (unsupported platform)")
}

#
# [REQUIRED] EIGEN matrix library
# NOMINMAX constant required to make internal min/max work.
INCLUDEPATH += libs/eigen
DEFINES += NOMINMAX

#
# [REQUIRED] OPMapControl library from OpenPilot. Provides 2D mapping functionality.
#
include(libs/utils/utils_external.pri)
include(libs/opmapcontrol/opmapcontrol_external.pri)

DEPENDPATH += \
    libs/utils \
    libs/utils/src \
    libs/opmapcontrol \
    libs/opmapcontrol/src \
    libs/opmapcontrol/src/mapwidget

INCLUDEPATH += \
    libs/utils \
    libs \
    libs/opmapcontrol

#
# [REQUIRED] QWT plotting library dependency. Provides plotting capabilities.
#
include(libs/qwt/qwt.pri)

#
# [REQUIRED] QSerialPort library. Provides serial port wrapper library.
#
include(libs/serialport/qserialport.pri)

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
} else:infile(user_config.pri, DEFINES, DISABLE_XBEE) {
    message("Skipping support for native XBee API (manual override from user_config.pri)")
} else:LinuxBuild {
	exists(/usr/include/xbee.h) {
		message("Including support for XBee API")

		HEADERS += $$XBEE_DEPENDENT_HEADERS
		SOURCES += $$XBEE_DEPENDENT_SOURCES
		DEFINES += $$XBEE_DEFINES
		LIBS += -lxbee
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
} else:infile(user_config.pri, DEFINES, DISABLE_3DMOUSE) {
    message("Skipping support for 3DConnexion mice (manual override from user_config.pri)")
} else:LinuxBuild {
	exists(/usr/local/lib/libxdrvlib.so) {
		message("Including support for 3DConnexion mice")

		DEFINES +=
		QGC_MOUSE_ENABLED_LINUX \
		ParameterCheck                      # Hack: Has to be defined for magellan usage

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
} else:infile(user_config.pri, DEFINES, DISABLE_RTLAB) {
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
	LIBS += \
		-lSDL \
		-lSDLmain
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
} else:infile(user_config.pri, DEFINES, DISABLE_SPEECH) {
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
}
