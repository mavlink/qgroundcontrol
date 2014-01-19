#
# Tell the Linux build to look in a few additional places for libs
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
# Add in a few missing headers to windows
#

WindowsBuild {
    INCLUDEPATH += ../libs/lib/msinttypes
}

#
# QUpgrade
#

exists(../qupgrade) {
    message(Including support for QUpgrade)

    DEFINES += QUPGRADE_SUPPORT

    INCLUDEPATH += ../qupgrade/src/apps/qupgrade

    FORMS += \
        ../qupgrade/src/apps/qupgrade/dialog_bare.ui \
        ../qupgrade/src/apps/qupgrade/boardwidget.ui

    HEADERS += \
        ../qupgrade/src/apps/qupgrade/qgcfirmwareupgradeworker.h \
        ../qupgrade/src/apps/qupgrade/uploader.h \
        ../qupgrade/src/apps/qupgrade/dialog_bare.h \
        ../qupgrade/src/apps/qupgrade/boardwidget.h

    SOURCES += \
        ../qupgrade/src/apps/qupgrade/qgcfirmwareupgradeworker.cpp \
        ../qupgrade/src/apps/qupgrade/uploader.cpp \
        ../qupgrade/src/apps/qupgrade/dialog_bare.cpp \
        ../qupgrade/src/apps/qupgrade/boardwidget.cpp

    RESOURCES += \
        ../qupgrade/qupgrade.qrc

    LinuxBuild:CONFIG += qesp_linux_udev

    include(../qupgrade/libs/qextserialport/src/qextserialport.pri)
} else {
    message(Skipping support for QUpgrade)
}

#
# MAVLink
#

MAVLINK_CONF = ""
MAVLINKPATH = $$BASEDIR/libs/mavlink/include/mavlink/v1.0
DEFINES += MAVLINK_NO_DATA

# If the user config file exists, it will be included.
# if the variable MAVLINK_CONF contains the name of an
# additional project, QGroundControl includes the support
# of custom MAVLink messages of this project. It will also
# create a QGC_USE_{AUTOPILOT_NAME}_MESSAGES macro for use
# within the actual code.
exists(user_config.pri) {
    include(user_config.pri)
    message("----- USING CUSTOM USER QGROUNDCONTROL CONFIG FROM user_config.pri -----")
    message("Adding support for additional MAVLink messages for: " $$MAVLINK_CONF)
    message("------------------------------------------------------------------------")
} else {
    MAVLINK_CONF += ardupilotmega
}
INCLUDEPATH += $$MAVLINKPATH
isEmpty(MAVLINK_CONF) {
    INCLUDEPATH += $$MAVLINKPATH/common
} else {
    INCLUDEPATH += $$MAVLINKPATH/$$MAVLINK_CONF
    DEFINES += $$sprintf('QGC_USE_%1_MESSAGES', $$upper($$MAVLINK_CONF))
}

#
# MAVLink generator (deprecated)
#

DEPENDPATH += \
    apps/mavlinkgen

INCLUDEPATH += \
    apps/mavlinkgen \
    apps/mavlinkgen/ui \
    apps/mavlinkgen/generator

include(apps/mavlinkgen/mavlinkgen.pri)

#
# OpenSceneGraph
#

MacBuild {
    # GLUT and OpenSceneGraph are part of standard install on Mac
	CONFIG += OSGDependency

    INCLUDEPATH += \
        $$BASEDIR/libs/lib/mac64/include

	LIBS += \
        -L$$BASEDIR/libs/lib/mac64/lib \
        -losgWidget
}

LinuxBuild {
	exists(/usr/include/osg) | exists(/usr/local/include/osg) {
        CONFIG += OSGDependency
        exists(/usr/include/osg/osgQt) | exists(/usr/include/osgQt) | exists(/usr/local/include/osg/osgQt) | exists(/usr/local/include/osgQt) {
            message("Including support for Linux OpenSceneGraph Qt")
            LIBS += -losgQt
            DEFINES += QGC_OSG_QT_ENABLED
        } else {
            message("Skipping support for Linux OpenSceneGraph Qt")
        }
	}
}

WindowsBuild {
	exists($$BASEDIR/libs/lib/osg123) {
        CONFIG += OSGDependency

		INCLUDEPATH += \
            $$BASEDIR/libs/lib/osgEarth/win32/include \
			$$BASEDIR/libs/lib/osgEarth_3rdparty/win32/OpenSceneGraph-2.8.2/include

		LIBS += -L$$BASEDIR/libs/lib/osgEarth_3rdparty/win32/OpenSceneGraph-2.8.2/lib
	}
}

OSGDependency {
    message("Including support for OpenSceneGraph")

	DEFINES += QGC_OSG_ENABLED
    
    LIBS += \
        -losg \
        -losgViewer \
        -losgGA \
        -losgDB \
        -losgText \
        -lOpenThreads

    HEADERS += \
        ui/map3D/gpl.h \
        ui/map3D/CameraParams.h \
        ui/map3D/ViewParamWidget.h \
        ui/map3D/SystemContainer.h \
        ui/map3D/SystemViewParams.h \
        ui/map3D/GlobalViewParams.h \
        ui/map3D/SystemGroupNode.h \
        ui/map3D/Q3DWidget.h \
        ui/map3D/GCManipulator.h \
        ui/map3D/ImageWindowGeode.h \
        ui/map3D/PixhawkCheetahNode.h \
        ui/map3D/Pixhawk3DWidget.h \
        ui/map3D/Q3DWidgetFactory.h \
        ui/map3D/WebImageCache.h \
        ui/map3D/WebImage.h \
        ui/map3D/TextureCache.h \
        ui/map3D/Texture.h \
        ui/map3D/Imagery.h \
        ui/map3D/HUDScaleGeode.h \
        ui/map3D/WaypointGroupNode.h \
        ui/map3D/TerrainParamDialog.h \
        ui/map3D/ImageryParamDialog.h
        
    SOURCES += \
        ui/map3D/gpl.cc \
        ui/map3D/CameraParams.cc \
        ui/map3D/ViewParamWidget.cc \
        ui/map3D/SystemContainer.cc \
        ui/map3D/SystemViewParams.cc \
        ui/map3D/GlobalViewParams.cc \
        ui/map3D/SystemGroupNode.cc \
        ui/map3D/Q3DWidget.cc \
        ui/map3D/ImageWindowGeode.cc \
        ui/map3D/GCManipulator.cc \
        ui/map3D/PixhawkCheetahNode.cc \
        ui/map3D/Pixhawk3DWidget.cc \
        ui/map3D/Q3DWidgetFactory.cc \
        ui/map3D/WebImageCache.cc \
        ui/map3D/WebImage.cc \
        ui/map3D/TextureCache.cc \
        ui/map3D/Texture.cc \
        ui/map3D/Imagery.cc \
        ui/map3D/HUDScaleGeode.cc \
        ui/map3D/WaypointGroupNode.cc \
        ui/map3D/TerrainParamDialog.cc \
        ui/map3D/ImageryParamDialog.cc
} else {
    message("Skipping support for OpenSceneGraph")
}

#
# Google Earth
#

MacBuild | WindowsBuild {
    message(Including support for Google Earth)

    HEADERS += ui/map3D/QGCGoogleEarthView.h
    SOURCES += ui/map3D/QGCGoogleEarthView.cc
    WindowsBuild {
        CONFIG += qaxcontainer
    }
} else {
    message(Skipping support for Google Earth)
}

#
# Protcol Buffers for PixHawk
#

LinuxBuild : contains(MAVLINK_CONF, pixhawk) {
    exists(/usr/local/include/google/protobuf) | exists(/usr/include/google/protobuf) {
        message("Including support for Protocol Buffers")

        DEFINES += QGC_PROTOBUF_ENABLED

        LIBS += \
            -lprotobuf \
            -lprotobuf-lite \
            -lprotoc

        HEADERS += \
            ../libs/mavlink/include/mavlink/v1.0/pixhawk/pixhawk.pb.h \
            ui/map3D/ObstacleGroupNode.h \
            ui/map3D/GLOverlayGeode.h

        SOURCES += \
            ../libs/mavlink/share/mavlink/v1.0/pixhawk/pixhawk.pb.cc \
            ui/map3D/ObstacleGroupNode.cc \
            ui/map3D/GLOverlayGeode.cc
    } else {
        message("Skipping support for Protocol Buffers")
    }
} else {
    message("Skipping support for Protocol Buffers")
}

#
# libfreenect Kinect support
#

MacBuild | LinuxBuild {
    exists(/opt/local/include/libfreenect) | exists(/usr/local/include/libfreenect) {
        message("Including support for libfreenect")

        #INCLUDEPATH += /usr/include/libusb-1.0
        DEFINES += QGC_LIBFREENECT_ENABLED
        LIBS += -lfreenect
        HEADERS += input/Freenect.h
        SOURCES += input/Freenect.cc
    } else {
        message("Skipping support for libfreenect")
    }
} else {
    message("Skipping support for libfreenect")
}

#
# EIGEN matrix library (NOMINMAX needed to make internal min/max work)
#

INCLUDEPATH += ../libs/eigen
DEFINES += NOMINMAX

#
# OPMapControl library (from OpenPilot)
#

include(../libs/utils/utils_external.pri)
include(../libs/opmapcontrol/opmapcontrol_external.pri)

DEPENDPATH += \
    ../libs/utils \
    ../libs/utils/src \
    ../libs/opmapcontrol \
    ../libs/opmapcontrol/src \
    ../libs/opmapcontrol/mapwidget

INCLUDEPATH += \
    ../libs/utils \
    ../libs \
    ../libs/opmapcontrol

#
# QWT plotting library
#
INCLUDEPATH += ../libs/qwt
LIBS += -L../libs/qwt -lqwt

#
# QSerialPort - serial port library
#
include(../libs/serialport/qserialport.pri)

WindowsBuild {
    # Used to enumerate serial ports by QSerialPort
    LIBS += -lsetupapi
}


#
# XBee wireless
#

WindowsBuild | LinuxBuild {
    message(Including support for XBee)

    DEFINES += XBEELINK

    INCLUDEPATH += ../libs/thirdParty/libxbee

    HEADERS += \
        comm/XbeeLinkInterface.h \
        comm/XbeeLink.h \
        comm/HexSpinBox.h \
        ui/XbeeConfigurationWindow.h \
        comm/CallConv.h

    SOURCES += \
        comm/XbeeLink.cpp \
        comm/HexSpinBox.cpp \
        ui/XbeeConfigurationWindow.cpp

    WindowsBuild {
        LIBS += -l$$BASEDIR/libs/thirdParty/libxbee/lib/libxbee
    }

    LinuxBuild {
        LIBS += -L$$BASEDIR/libs/thirdParty/libxbee/lib \
    		-lxbee
    }
} else {
    message(Skipping support for XBee)
}

#
# 3DConnexion 3d Mice support
#

LinuxBuild : exists(/usr/local/lib/libxdrvlib.so) {
    message("Including support for Magellan 3DxWare")

    DEFINES +=
        MOUSE_ENABLED_LINUX \
        ParameterCheck                      # Hack: Has to be defined for magellan usage

    INCLUDEPATH *= /usr/local/include
    HEADERS += input/Mouse6dofInput.h
    SOURCES += input/Mouse6dofInput.cpp
    LIBS += -L/usr/local/lib/ -lxdrvlib
}

WindowsBuild {
    message("Including support for Magellan 3DxWare")

    DEFINES += MOUSE_ENABLED_WIN

    INCLUDEPATH += ../libs/thirdParty/3DMouse/win

    HEADERS += \
        ../libs/thirdParty/3DMouse/win/I3dMouseParams.h \
        ../libs/thirdParty/3DMouse/win/MouseParameters.h \
        ../libs/thirdParty/3DMouse/win/Mouse3DInput.h \
        input/Mouse6dofInput.h

    SOURCES += \
        ../libs/thirdParty/3DMouse/win/MouseParameters.cpp \
        ../libs/thirdParty/3DMouse/win/Mouse3DInput.cpp \
        input/Mouse6dofInput.cpp
}

#
# Opal RT-LAB Library
#

WindowsBuild : win32 : exists(lib/opalrt/OpalApi.h) : exists(C:/OPAL-RT/RT-LAB7.2.4/Common/bin) {
    message("Including support for Opal-RT")

    DEFINES += OPAL_RT

    INCLUDEPATH += 
        lib/opalrt
        ../libs/lib/opal/include \

    FORMS += ui/OpalLinkSettings.ui

    HEADERS += \
        comm/OpalRT.h \
        comm/OpalLink.h \
        comm/Parameter.h \
        comm/QGCParamID.h \
        comm/ParameterList.h \
        ui/OpalLinkConfigurationWindow.h

    SOURCES += \
        comm/OpalRT.cc \
        comm/OpalLink.cc \
        comm/Parameter.cc \
        comm/QGCParamID.cc \
        comm/ParameterList.cc \
        ui/OpalLinkConfigurationWindow.cc

    LIBS += \
        -LC:/OPAL-RT/RT-LAB7.2.4/Common/bin \
        -lOpalApi
} else {
    message("Skipping support for Opal-RT")
}

#
# SDL
#

MacBuild {
    INCLUDEPATH += \
        $$BASEDIR/libs/lib/Frameworks/SDL.framework/Headers

    LIBS += \
        -F$$BASEDIR/libs/lib/Frameworks \
        -framework SDL
}

LinuxBuild {
	LIBS += \
		-lSDL \
		-lSDLmain
}

WindowsBuild {
	INCLUDEPATH += \
        $$BASEDIR/libs/lib/sdl/msvc/include \

	LIBS += \
        -L$$BASEDIR/libs/lib/sdl/msvc/lib \
        -lSDLmain \
        -lSDL
}

#
# Festival Lite speech synthesis engine
#

LinuxBuild {
	LIBS += \
		-lflite_cmu_us_kal \
		-lflite_usenglish \
		-lflite_cmulex \
		-lflite
}

