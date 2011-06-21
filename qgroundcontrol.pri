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
message(Using Qt from $$[QTDIR])

release {
#    DEFINES += QT_NO_DEBUG_OUTPUT
#    DEFINES += QT_NO_WARNING_OUTPUT
}

QMAKE_POST_LINK += echo "Copying files"

# Turn off serial port warnings
DEFINES += _TTY_NOWARN_

#QMAKE_POST_LINK += && cp -rf $$BASEDIR/models $$TARGETDIR/debug/.
#QMAKE_POST_LINK += && cp -rf $$BASEDIR/models $$TARGETDIR/release/.

# MAC OS X
macx {

   # COMPILER_VERSION = $$system(gcc -v)
    #message(Using compiler $$COMPILER_VERSION)

        CONFIG += x86 cocoa phonon
        CONFIG -= x86_64

    #HARDWARE_PLATFORM = $$system(uname -a)
    #contains( $$HARDWARE_PLATFORM, "9.6.0" ) || contains( $$HARDWARE_PLATFORM, "9.7.0" ) || contains( $$HARDWARE_PLATFORM, "9.8.0" ) || contains( $$HARDWARE_PLATFORM, "9.9.0" ) {
        # x86 Mac OS X Leopard 10.5 and earlier

        #message(Building for Mac OS X 32bit/Leopard 10.5 and earlier)

                # Enable function-profiling with the OS X saturn tool
                #debug {
                        #QMAKE_CXXFLAGS += -finstrument-functions
                        #LIBS += -lSaturn
                       # CONFIG += console
                #}
    #} else {
        # x64 Mac OS X Snow Leopard 10.6 and later
     #   CONFIG += x86_64 x86 cocoa phonon
        #CONFIG -= x86 # phonon
        #message(Building for Mac OS X 64bit/Snow Leopard 10.6 and later)
      #          debug {
                        #QMAKE_CXXFLAGS += -finstrument-functions
                        #LIBS += -lSaturn
                        CONFIG += console
      #          }
    #}

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
    QMAKE_POST_LINK += && cp -rf $$BASEDIR/audio $$TARGETDIR/qgroundcontrol.app/Contents/MacOS
    # Copy google earth starter file
    QMAKE_POST_LINK += && cp -f $$BASEDIR/images/earth.html $$TARGETDIR/qgroundcontrol.app/Contents/MacOS
    # Copy CSS stylesheets
    QMAKE_POST_LINK += && cp -f $$BASEDIR/images/style-mission.css $$TARGETDIR/qgroundcontrol.app/Contents/MacOS/style-indoor.css
    QMAKE_POST_LINK += && cp -f $$BASEDIR/images/style-outdoor.css $$TARGETDIR/qgroundcontrol.app/Contents/MacOS
    # Copy model files
    #QMAKE_POST_LINK += && cp -f $$BASEDIR/models/*.dae $$TARGETDIR/qgroundcontrol.app/Contents/MacOs

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
    #QMAKE_CXXFLAGS += -Wl,-E
}

# GNU/Linux
linux-g++ {

    debug {
        DESTDIR = $$TARGETDIR/debug
        CONFIG += debug console
    }

    release {
        DESTDIR = $$TARGETDIR/release
        DEFINES += QT_NO_DEBUG
        CONFIG -= console
    }

    QMAKE_POST_LINK += cp -rf $$BASEDIR/audio $$DESTDIR/.

message("Compiling for linux 32")

    INCLUDEPATH += /usr/include \
                   /usr/local/include \
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
    LIBS += -losg \
            -losgViewer \
            -losgGA \
            -losgDB \
            -losgText \
            -lOpenThreads

    DEFINES += QGC_OSG_ENABLED
    }

    exists(/usr/include/osgEarth):exists(/usr/include/osg) | exists(/usr/local/include/osgEarth):exists(/usr/include/osg) {
    message("Building support for osgEarth")
    DEPENDENCIES_PRESENT += osgearth
    # Include osgEarth libraries
    LIBS += -losgEarth \
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

    QMAKE_POST_LINK += && cp -rf $$BASEDIR/models $$DESTDIR
    QMAKE_POST_LINK += && cp -rf $$BASEDIR/data $$DESTDIR
    QMAKE_POST_LINK += && mkdir -p $$DESTDIR/images
    QMAKE_POST_LINK += && cp -rf $$BASEDIR/images/Vera.ttf $$DESTDIR/images/Vera.ttf

    # osg/osgEarth dynamic casts might fail without this compiler option.
    # see http://osgearth.org/wiki/FAQ for details.
    QMAKE_CXXFLAGS += -Wl,-E
}

linux-g++-64 {

    debug {
        DESTDIR = $$TARGETDIR/debug
        CONFIG += debug console
    }

    release {
        DESTDIR = $$TARGETDIR/release
        DEFINES += QT_NO_DEBUG
        CONFIG -= console
    }

    QMAKE_POST_LINK += cp -rf $$BASEDIR/audio $$DESTDIR/.

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
    LIBS += -losg \
            -losgViewer \
            -losgGA \
            -losgDB \
            -losgText \
            -lOpenThreads

    DEFINES += QGC_OSG_ENABLED
    }

    exists(/usr/include/osgEarth) {
    message("Building support for osgEarth")
    DEPENDENCIES_PRESENT += osgearth
    # Include osgEarth libraries
    LIBS += -losgEarth \
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

    QMAKE_POST_LINK += && cp -rf $$BASEDIR/models $$DESTDIR
    QMAKE_POST_LINK += && cp -rf $$BASEDIR/data $$DESTDIR
    QMAKE_POST_LINK += && mkdir -p $$DESTDIR/images
    QMAKE_POST_LINK += && cp -rf $$BASEDIR/images/Vera.ttf $$DESTDIR/images/Vera.ttf

    # osg/osgEarth dynamic casts might fail without this compiler option.
    # see http://osgearth.org/wiki/FAQ for details.
    QMAKE_CXXFLAGS += -Wl,-E
}

# Windows (32bit)
win32-msvc2008 {

    message(Building for Windows Visual Studio 2008 (32bit))

    CONFIG += qaxcontainer

    # QWebkit is not needed on MS-Windows compilation environment
    CONFIG -= webkit

    release {
        CONFIG -= console
        DEFINES += QT_NO_DEBUG
    }

    debug {
		CONFIG += console
	}

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

    exists($$TARGETDIR/debug) {
        QMAKE_POST_LINK += && copy /Y \"$$BASEDIR_WIN\\lib\\sdl\\win32\\SDL.dll\" \"$$TARGETDIR_WIN\\debug\\SDL.dll\"
        QMAKE_POST_LINK += && xcopy \"$$BASEDIR_WIN\\audio\" \"$$TARGETDIR_WIN\\debug\\audio\\\" /S /E /Y
        QMAKE_POST_LINK += && xcopy \"$$BASEDIR_WIN\\models\" \"$$TARGETDIR_WIN\\debug\\models\\\" /S /E /Y
        QMAKE_POST_LINK += && copy /Y \"$$BASEDIR_WIN\\images\\earth.html\" \"$$TARGETDIR_WIN\\debug\\earth.html\"
    }

    exists($$TARGETDIR/release) {
        QMAKE_POST_LINK += && copy /Y \"$$BASEDIR_WIN\\lib\\sdl\\win32\\SDL.dll\" \"$$TARGETDIR_WIN\\release\\SDL.dll\"
        QMAKE_POST_LINK += && xcopy \"$$BASEDIR_WIN\\audio\" \"$$TARGETDIR_WIN\\release\\audio\\\" /S /E /Y
        QMAKE_POST_LINK += && xcopy \"$$BASEDIR_WIN\\models\" \"$$TARGETDIR_WIN\\release\\models\\\" /S /E /Y
        QMAKE_POST_LINK += && copy /Y \"$$BASEDIR_WIN\\images\\earth.html\" \"$$TARGETDIR_WIN\\release\\earth.html\"
    }

}

# Windows (32bit)
win32-g++ {

    message(Building for Windows Platform (32bit))

    # Special settings for debug
    CONFIG += CONSOLE
    OUTPUT += CONSOLE

    INCLUDEPATH += $$BASEDIR/lib/sdl/include \
                   $$BASEDIR/lib/opal/include #\ #\
                   #"C:\Program Files\Microsoft SDKs\Windows\v7.0\Include"

    LIBS += -L$$BASEDIR/lib/sdl/win32 \
             -lmingw32 -lSDLmain -lSDL -mwindows

    CONFIG += windows



    debug {
        #DESTDIR = $$BUILDDIR/debug
    CONFIG += console
    }

    release {
        CONFIG -= console
        DEFINES += QT_NO_DEBUG
        #DESTDIR = $$BUILDDIR/release
    }

    RC_FILE = $$BASEDIR/qgroundcontrol.rc

    # Copy dependencies

    system(cp): {
    # CP command is available, use it instead of copy / xcopy
    message("Using cp to copy image and audio files to executable")
    debug {
        QMAKE_POST_LINK += && cp $$BASEDIR/lib/sdl/win32/SDL.dll $$TARGETDIR/debug/SDL.dll
        QMAKE_POST_LINK += && cp -r $$BASEDIR/audio $$TARGETDIR/debug/audio
        QMAKE_POST_LINK += && cp -r $$BASEDIR/models $$TARGETDIR/debug/models
    }

    release {
        QMAKE_POST_LINK += && cp $$BASEDIR/lib/sdl/win32/SDL.dll $$TARGETDIR/release/SDL.dll
        QMAKE_POST_LINK += && cp -r $$BASEDIR/audio $$TARGETDIR/release/audio
        QMAKE_POST_LINK += && cp -r $$BASEDIR/models $$TARGETDIR/release/models
    }

    } else {
    # No cp command available, go for copy / xcopy
    # Copy dependencies
    BASEDIR_WIN = $$replace(BASEDIR,"/","\\")
    TARGETDIR_WIN = $$replace(TARGETDIR,"/","\\")

    exists($$TARGETDIR/debug) {
        QMAKE_POST_LINK += && copy /Y \"$$BASEDIR_WIN\\lib\\sdl\\win32\\SDL.dll\" \"$$TARGETDIR_WIN\\debug\\SDL.dll\"
        QMAKE_POST_LINK += && xcopy \"$$BASEDIR_WIN\\audio\" \"$$TARGETDIR_WIN\\debug\\audio\\\" /S /E /Y
        QMAKE_POST_LINK += && xcopy \"$$BASEDIR_WIN\\models\" \"$$TARGETDIR_WIN\\debug\\models\\\" /S /E /Y
        QMAKE_POST_LINK += && copy /Y \"$$BASEDIR_WIN\\images\\earth.html\" \"$$TARGETDIR_WIN\\debug\\earth.html\"
    }

    exists($$TARGETDIR/release) {
        QMAKE_POST_LINK += && copy /Y \"$$BASEDIR_WIN\\lib\\sdl\\win32\\SDL.dll\" \"$$TARGETDIR_WIN\\release\\SDL.dll\"
        QMAKE_POST_LINK += && xcopy \"$$BASEDIR_WIN\\audio\" \"$$TARGETDIR_WIN\\release\\audio\\\" /S /E /Y
        QMAKE_POST_LINK += && xcopy \"$$BASEDIR_WIN\\models\" \"$$TARGETDIR_WIN\\release\\models\\\" /S /E /Y
        QMAKE_POST_LINK += && copy /Y \"$$BASEDIR_WIN\\images\\earth.html\" \"$$TARGETDIR_WIN\\release\\earth.html\"
    }

}

    # osg/osgEarth dynamic casts might fail without this compiler option.
    # see http://osgearth.org/wiki/FAQ for details.
    QMAKE_CXXFLAGS += -Wl,-E
}
# vim:ts=4:sw=4:expandtab
