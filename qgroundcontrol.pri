#-------------------------------------------------
#
# QGroundControl - Micro Air Vehicle Groundstation
#
# Please see our website at <http://qgroundcontrol.org>
#
# Author:
# Lorenz Meier <mavteam@student.ethz.ch>
#
# (c) 2009-2010 PIXHAWK Team
#
# This file is part of the mav groundstation project
# QGroundControl is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# QGroundControl is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with QGroundControl. If not, see <http://www.gnu.org/licenses/>.
#
#-------------------------------------------------

#$$BASEDIR/lib/qextserialport/include
#               $$BASEDIR/lib/openjaus/libjaus/include \
#               $$BASEDIR/lib/openjaus/libopenJaus/include

message(Qt version $$[QT_VERSION])

release {
#    DEFINES += QT_NO_DEBUG_OUTPUT
#    DEFINES += QT_NO_WARNING_OUTPUT
}

QMAKE_PRE_LINK += echo "Copying files"

#QMAKE_PRE_LINK += && cp -rf $$BASEDIR/models $$TARGETDIR/debug/.
#QMAKE_PRE_LINK += && cp -rf $$BASEDIR/models $$TARGETDIR/release/.

# MAC OS X
macx { 

    COMPILER_VERSION = system(gcc -v)
    message(Using compiler $$COMPILER_VERSION)

    HARDWARE_PLATFORM = $$system(uname -a)
    contains( HARDWARE_PLATFORM, 9.6.0 ) || contains( HARDWARE_PLATFORM, 9.7.0 ) || contains( HARDWARE_PLATFORM, 9.8.0 ) || contains( HARDWARE_PLATFORM, 9.9.0 ) {
        # x86 Mac OS X Leopard 10.5 and earlier
        CONFIG += x86 cocoa phonon
        message(Building for Mac OS X 32bit/Leopard 10.5 and earlier)

                # Enable function-profiling with the OS X saturn tool
                debug {
                        #QMAKE_CXXFLAGS += -finstrument-functions
                        #LIBS += -lSaturn
                }
    } else {
        # x64 Mac OS X Snow Leopard 10.6 and later
        CONFIG += x86_64 cocoa
        CONFIG -= x86 phonon
        message(Building for Mac OS X 64bit/Snow Leopard 10.6 and later)
    }

    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.5

    #DESTDIR = $$BASEDIR/bin/mac
    INCLUDEPATH += -framework SDL

    LIBS += -framework IOKit \
        -framework SDL \
        -framework CoreFoundation \
        -framework ApplicationServices \
        -lm
    
    ICON = $$BASEDIR/images/icons/macx.icns

    # Copy audio files if needed
    QMAKE_PRE_LINK += && cp -rf $$BASEDIR/audio $$TARGETDIR/qgroundcontrol.app/Contents/MacOs/.
    # Copy google earth starter file
    QMAKE_PRE_LINK += && cp -f $$BASEDIR/images/earth.html $$TARGETDIR/qgroundcontrol.app/Contents/MacOs/.
    # Copy model files
    #QMAKE_PRE_LINK += && cp -f $$BASEDIR/models/*.dae $$TARGETDIR/qgroundcontrol.app/Contents/MacOs/.

    exists(/Library/Frameworks/osg.framework):exists(/Library/Frameworks/OpenThreads.framework) {
    # No check for GLUT.framework since it's a MAC default
    message("Building support for OpenSceneGraph")
    DEPENDENCIES_PRESENT += osg
    DEFINES += QGC_OSG_ENABLED
    # Include OpenSceneGraph libraries
    INCLUDEPATH += -framework GLUT \
            -framework Carbon \
            -framework OpenThreads \
            -framework osg \
            -framework osgViewer \
            -framework osgGA \
            -framework osgDB \
            -framework osgText \
            -framework osgWidget

    LIBS += -framework GLUT \
            -framework Carbon \
            -framework OpenThreads \
            -framework osg \
            -framework osgViewer \
            -framework osgGA \
            -framework osgDB \
            -framework osgText \
            -framework osgWidget
    }

    exists(/usr/include/osgEarth) {
    message("Building support for osgEarth")
    DEPENDENCIES_PRESENT += osgearth
    # Include osgEarth libraries
    INCLUDEPATH += -framework GDAL \
            $$IN_PWD/lib/mac32-gcc/include \
            -framework GEOS \
            -framework SQLite3 \
            -framework osgFX \
            -framework osgTerrain

    LIBS += -framework GDAL \
            -framework GEOS \
            -framework SQLite3 \
            -framework osgFX \
            -framework osgTerrain
    DEFINES += QGC_OSGEARTH_ENABLED
    }


    exists(/opt/local/include/libfreenect) {
    message("Building support for libfreenect")
    DEPENDENCIES_PRESENT += libfreenect
    # Include libfreenect libraries
    LIBS += -lfreenect
    DEFINES += QGC_LIBFREENECT_ENABLED
    }

    # osg/osgEarth dynamic casts might fail without this compiler option.
    # see http://osgearth.org/wiki/FAQ for details.
    QMAKE_CXXFLAGS += -Wl,-E
}

# GNU/Linux
linux-g++ {

    debug {
        #DESTDIR = $$BUILDDIR/debug
        CONFIG += debug
    }

    release {
        #DESTDIR = $$BUILDDIR/release
    }

    QMAKE_PRE_LINK += cp -rf $$BASEDIR/audio $$DESTDIR/.

    INCLUDEPATH += /usr/include \
                   /usr/include/qt4/phonon
              # $$BASEDIR/lib/flite/include \
              # $$BASEDIR/lib/flite/lang


    message(Building for GNU/Linux 32bit/i386)

    LIBS += \
        -L/usr/lib \
        -lm \
        -lflite_cmu_us_kal \
        -lflite_usenglish \
        -lflite_cmulex \
        -lflite \
        -lSDL \
        -lSDLmain

    exists(/usr/include/osg) {
    message("Building support for OpenSceneGraph")
    DEPENDENCIES_PRESENT += osg
    # Include OpenSceneGraph libraries
    LIBS += -losg
    DEFINES += QGC_OSG_ENABLED
    }

    exists(/usr/include/osgEarth) | exists(/usr/local/include/osgEarth) {
    message("Building support for osgEarth")
    DEPENDENCIES_PRESENT += osgearth
    # Include osgEarth libraries
    LIBS += -losgViewer \
            -losgEarth \
            -losgEarthUtil
    DEFINES += QGC_OSGEARTH_ENABLED
    }

    exists(/usr/local/include/libfreenect/libfreenect.h) {
    message("Building support for libfreenect")
    DEPENDENCIES_PRESENT += libfreenect
    INCLUDEPATH += /usr/include/libusb-1.0
    # Include libfreenect libraries
    LIBS += -lfreenect
    DEFINES += QGC_LIBFREENECT_ENABLED
    }

    QMAKE_PRE_LINK += && cp -rf $$BASEDIR/models $$TARGETDIR/debug/.
    QMAKE_PRE_LINK += && cp -rf $$BASEDIR/models $$TARGETDIR/release/.
    QMAKE_PRE_LINK += && cp -rf $$BASEDIR/data $$TARGETDIR/debug/.
    QMAKE_PRE_LINK += && cp -rf $$BASEDIR/data $$TARGETDIR/release/.

    # osg/osgEarth dynamic casts might fail without this compiler option.
    # see http://osgearth.org/wiki/FAQ for details.
    QMAKE_CXXFLAGS += -Wl,-E
}

linux-g++-64 {

    debug {
        #DESTDIR = $$BUILDDIR/debug
        CONFIG += debug
    }

    release {
        #DESTDIR = $$BUILDDIR/release
    }

    QMAKE_PRE_LINK += cp -rf $$BASEDIR/audio $$DESTDIR/.

    INCLUDEPATH += /usr/include \
                   /usr/include/qt4/phonon
              # $$BASEDIR/lib/flite/include \
              # $$BASEDIR/lib/flite/lang


    # 64-bit Linux
    message(Building for GNU/Linux 64bit/x64 (g++-64))

    LIBS += \
        -L/usr/lib \
        -lm \
        -lflite_cmu_us_kal \
        -lflite_usenglish \
        -lflite_cmulex \
        -lflite \
        -lSDL \
        -lSDLmain

    exists(/usr/include/osg) {
    message("Building support for OpenSceneGraph")
    DEPENDENCIES_PRESENT += osg
    # Include OpenSceneGraph libraries
    LIBS += -losg
    DEFINES += QGC_OSG_ENABLED
    }

    exists(/usr/include/osgEarth) {
    message("Building support for osgEarth")
    DEPENDENCIES_PRESENT += osgearth
    # Include osgEarth libraries
    LIBS += -losgViewer \
            -losgEarth \
            -losgEarthUtil
    DEFINES += QGC_OSGEARTH_ENABLED
    }

    exists(/usr/local/include/libfreenect) {
    message("Building support for libfreenect")
    DEPENDENCIES_PRESENT += libfreenect
    INCLUDEPATH += /usr/include/libusb-1.0
    # Include libfreenect libraries
    LIBS += -lfreenect
    DEFINES += QGC_LIBFREENECT_ENABLED
    }

    # osg/osgEarth dynamic casts might fail without this compiler option.
    # see http://osgearth.org/wiki/FAQ for details.
    QMAKE_CXXFLAGS += -Wl,-E
}

# Windows (32bit)
win32-msvc2008 {

    message(Building for Windows Visual Studio 2008 (32bit))

    CONFIG += qaxcontainer

    # Special settings for debug
    #CONFIG += CONSOLE

    INCLUDEPATH += $$BASEDIR/lib/sdl/msvc/include \
                   $$BASEDIR/lib/opal/include \
                   $$BASEDIR/lib/msinttypes
                   #"C:\Program Files\Microsoft SDKs\Windows\v7.0\Include"

    LIBS += -L$$BASEDIR/lib/sdl/msvc/lib \
             -lSDLmain -lSDL

exists($$BASEDIR/lib/osg123) {
message("Building support for OSG")
DEPENDENCIES_PRESENT += osg

# Include OpenSceneGraph and osgEarth libraries
INCLUDEPATH += $$BASEDIR/lib/osgEarth/win32/include \
    $$BASEDIR/lib/osgEarth_3rdparty/win32/OpenSceneGraph-2.8.2/include
LIBS += -L$$BASEDIR/lib/osgEarth_3rdparty/win32/OpenSceneGraph-2.8.2/lib \
    -losg \
    -losgViewer \
	-losgGA \
	-losgDB \
	-losgText \
        -lOpenThreads
DEFINES += QGC_OSG_ENABLED
exists($$BASEDIR/lib/osgEarth123) {
    DEPENDENCIES_PRESENT += osgearth
    message("Building support for osgEarth")
    DEFINES += QGC_OSGEARTH_ENABLED
    LIBS += -L$$BASEDIR/lib/osgEarth/win32/lib \
        -losgEarth \
        -losgEarthUtil
}
}

    RC_FILE = $$BASEDIR/qgroundcontrol.rc

    # Copy dependencies
    BASEDIR_WIN = $$replace(BASEDIR,"/","\\")
    TARGETDIR_WIN = $$replace(TARGETDIR,"/","\\")

    QMAKE_PRE_LINK += && copy /Y \"$$BASEDIR_WIN\lib\sdl\win32\SDL.dll\" \"$$TARGETDIR_WIN\debug\SDL.dll\"
    QMAKE_PRE_LINK += && copy /Y \"$$BASEDIR_WIN\lib\sdl\win32\SDL.dll\" \"$$TARGETDIR_WIN\release\SDL.dll\"
    QMAKE_PRE_LINK += && xcopy \"$$BASEDIR_WIN\audio\" \"$$TARGETDIR_WIN\debug\audio\" /S /E /Y
    QMAKE_PRE_LINK += && xcopy \"$$BASEDIR_WIN\audio\" \"$$TARGETDIR_WIN\release\audio\" /S /E /Y
    QMAKE_PRE_LINK += && xcopy \"$$BASEDIR_WIN\models\" \"$$TARGETDIR_WIN\debug\models\" /S /E /Y
    QMAKE_PRE_LINK += && xcopy \"$$BASEDIR_WIN\models\" \"$$TARGETDIR_WIN\release\models\" /S /E /Y

    # Copy google earth starter file
    QMAKE_PRE_LINK += && copy /Y \"$$BASEDIR/images/earth.html $$TARGETDIR_WIN\release\"
    QMAKE_PRE_LINK += && copy /Y \"$$BASEDIR/images/earth.html $$TARGETDIR_WIN\debug\"

}

# Windows (32bit)
win32-g++ {

    message(Building for Windows Platform (32bit))
    
    # Special settings for debug
    #CONFIG += CONSOLE

    INCLUDEPATH += $$BASEDIR/lib/sdl/include \
                   $$BASEDIR/lib/opal/include #\ #\
                   #"C:\Program Files\Microsoft SDKs\Windows\v7.0\Include"

    LIBS += -L$$BASEDIR/lib/sdl/win32 \
             -lmingw32 -lSDLmain -lSDL -mwindows



    debug {
        #DESTDIR = $$BUILDDIR/debug
    }

    release {
        #DESTDIR = $$BUILDDIR/release
    }
        
    RC_FILE = $$BASEDIR/qgroundcontrol.rc

    # Copy dependencies
    BASEDIR_WIN = $$replace(BASEDIR,"/","\\")
    TARGETDIR_WIN = $$replace(TARGETDIR,"/","\\")

    QMAKE_PRE_LINK += && copy /Y \"$$BASEDIR_WIN\lib\sdl\win32\SDL.dll\" \"$$TARGETDIR_WIN\debug\SDL.dll\"
    QMAKE_PRE_LINK += && copy /Y \"$$BASEDIR_WIN\lib\sdl\win32\SDL.dll\" \"$$TARGETDIR_WIN\release\SDL.dll\"
    QMAKE_PRE_LINK += && xcopy \"$$BASEDIR_WIN\audio\" \"$$TARGETDIR_WIN\debug\audio\" /S /E /Y
    QMAKE_PRE_LINK += && xcopy \"$$BASEDIR_WIN\audio\" \"$$TARGETDIR_WIN\release\audio\" /S /E /Y
    QMAKE_PRE_LINK += && xcopy \"$$BASEDIR_WIN\models\" \"$$TARGETDIR_WIN\debug\models\" /S /E /Y
    QMAKE_PRE_LINK += && xcopy \"$$BASEDIR_WIN\models\" \"$$TARGETDIR_WIN\release\models\" /S /E /Y

    # osg/osgEarth dynamic casts might fail without this compiler option.
    # see http://osgearth.org/wiki/FAQ for details.
    QMAKE_CXXFLAGS += -Wl,-E
}
