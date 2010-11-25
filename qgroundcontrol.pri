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

# MAC OS X
macx { 

    HARDWARE_PLATFORM = $$system(uname -a)
    contains( HARDWARE_PLATFORM, 9.6.0 ) || contains( HARDWARE_PLATFORM, 9.7.0 ) || contains( HARDWARE_PLATFORM, 9.8.0 ) || || contains( HARDWARE_PLATFORM, 9.9.0 ) {
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

    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.6

    DESTDIR = $$BASEDIR/bin/mac
    INCLUDEPATH += -framework SDL \
        $$BASEDIR/../mavlink/contrib/slugs/include \
        $$BASEDIR/../mavlink/include

    LIBS += -framework IOKit \
        -framework SDL \
        -framework CoreFoundation \
        -framework ApplicationServices \
 #       -framework GLUT \
        -lm
    
    ICON = $$BASEDIR/images/icons/macx.icns

    # Copy audio files if needed
    QMAKE_PRE_LINK += cp -rf $$BASEDIR/audio $$DESTDIR/qgroundcontrol.app/Contents/MacOs/.

    exists(/usr/include/osg) {
    message("Building support for OpenSceneGraph")
    DEPENDENCIES_PRESENT += osg
    # Include OpenSceneGraph libraries
    LIBS += -losg
    }

    exists(/usr/include/osgEarth) {
    message("Building support for osgEarth")
    DEPENDENCIES_PRESENT += osgearth
    # Include osgEarth libraries
    LIBS += -losgViewer \
            -losgEarth
    DEFINES += QGC_OSGEARTH_ENABLED
    }

    exists(/usr/local/include/libfreenect) {
    message("Building support for libfreenect")
    DEPENDENCIES_PRESENT += libfreenect
    # Include libfreenect libraries
    LIBS += -lfreenect
    DEFINES += QGC_LIBFREENECT_ENABLED
    }
}

# GNU/Linux
linux-g++ {

    CONFIG += debug
    
    debug {
        DESTDIR = $$BUILDDIR/debug
    }

    release {
        DESTDIR = $$BUILDDIR/release
    }

    QMAKE_PRE_LINK += cp -rf $$BASEDIR/audio $$DESTDIR/.

    INCLUDEPATH += /usr/include \
                   /usr/include/qt4/phonon
              # $$BASEDIR/lib/flite/include \
              # $$BASEDIR/lib/flite/lang


    HARDWARE_PLATFORM = $$system(uname -a)
    contains( HARDWARE_PLATFORM, x86_64 ) {
        # 64-bit Linux
        #LIBS += \
            #-L$$BASEDIR/lib/flite/linux64
        message(Building for GNU/Linux 64bit/x64)
    } else {
        # 32-bit Linux
        #LIBS += \
           #-L$$BASEDIR/lib/flite/linux32
        message(Building for GNU/Linux 32bit/i386)
    }
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
            -losgEarth
    DEFINES += QGC_OSGEARTH_ENABLED
    }

    exists(/usr/local/include/libfreenect) {
    message("Building suplocport for libfreenect")
    DEPENDENCIES_PRESENT += libfreenect
    INCLUDEPATH += /usr/include/libusb-1.0
    # Include libfreenect libraries
    LIBS += -lfreenect
    DEFINES += QGC_LIBFREENECT_ENABLED
    }

    QMAKE_CXXFLAGS += -Wl,-E

        #-lflite_cmu_us_rms \
        #-lflite_cmu_us_slt \
}

linux-g++-64 {
    CONFIG += debug

    debug {
        DESTDIR = $$BUILDDIR/debug
    }

    release {
        DESTDIR = $$BUILDDIR/release
    }

    QMAKE_PRE_LINK += cp -rf $$BASEDIR/audio $$DESTDIR/.
    INCLUDEPATH += /usr/include \
                   /usr/include/qt4/phonon
              # $$BASEDIR/lib/flite/include \
              # $$BASEDIR/lib/flite/lang


    HARDWARE_PLATFORM = $$system(uname -a)
    contains( HARDWARE_PLATFORM, x86_64 ) {
        # 64-bit Linux
        #LIBS += \
            #-L$$BASEDIR/lib/flite/linux64
        message(Building for GNU/Linux 64bit/x64)
    } else {
        # 32-bit Linux
        #LIBS += \
           #-L$$BASEDIR/lib/flite/linux32
        message(Building for GNU/Linux 32bit/i386)
    }
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
            -losgEarth
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

}

# Windows (32bit)
win32-msvc2008 {

    message(Building for Windows Visual Studio 2008 (32bit))

    # Special settings for debug
    #CONFIG += CONSOLE

    INCLUDEPATH += $$BASEDIR/lib/sdl/msvc/include \
                   $$BASEDIR/lib/opal/include \
                   $$BASEDIR/lib/msinttypes
                   #"C:\Program Files\Microsoft SDKs\Windows\v7.0\Include"

    LIBS += -L$$BASEDIR/lib/sdl/msvc/lib \
             -lSDLmain -lSDL

    RC_FILE = $$BASEDIR/qgroundcontrol.rc

    # Copy dependencies
    QMAKE_PRE_LINK += cp -f $$BASEDIR/lib/sdl/win32/SDL.dll $$TARGETDIR/debug/. &&
    QMAKE_PRE_LINK += cp -f $$BASEDIR/lib/sdl/win32/SDL.dll $$TARGETDIR/release/. &&
    QMAKE_PRE_LINK += cp -rf $$BASEDIR/audio $$TARGETDIR/debug/. &&
    QMAKE_PRE_LINK += cp -rf $$BASEDIR/audio $$TARGETDIR/release/.
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
        DESTDIR = $$BUILDDIR/debug
    }

    release {
        DESTDIR = $$BUILDDIR/release
    }
        
    RC_FILE = $$BASEDIR/qgroundcontrol.rc

    # Copy dependencies
    QMAKE_PRE_LINK += cp -f $$BASEDIR/lib/sdl/win32/SDL.dll $$BUILDDIR/debug/. &&
    QMAKE_PRE_LINK += cp -f $$BASEDIR/lib/sdl/win32/SDL.dll $$BUILDDIR/release/. &&
    QMAKE_PRE_LINK += cp -rf $$BASEDIR/audio $$BUILDDIR/debug/. &&
    QMAKE_PRE_LINK += cp -rf $$BASEDIR/audio $$BUILDDIR/release/.
}

# Windows (64bit)
win64-g++ {

    message(Building for Windows Platform (64bit))

    # Special settings for debug
    #CONFIG += CONSOLE

    INCLUDEPATH += $$BASEDIR\lib\sdl\include \
                   $$BASEDIR\lib\opal\include #\ #\
                   #"C:\Program Files\Microsoft SDKs\Windows\v7.0\Include"

    LIBS += -L$$BASEDIR\lib\sdl\win32 \
             -lmingw32 -lSDLmain -lSDL -mwindows



    debug {
        DESTDIR = $$BASEDIR/bin
    }

    release {
        DESTDIR = $$BASEDIR/bin
    }

    RC_FILE = $$BASEDIR/qgroundcontrol.rc

    # Copy dependencies
    QMAKE_PRE_LINK += cp -f $$BASEDIR/lib/sdl/win32/SDL.dll $$BUILDDIR/debug/. &&
    QMAKE_PRE_LINK += cp -f $$BASEDIR/lib/sdl/win32/SDL.dll $$BUILDDIR/release/. &&
    QMAKE_PRE_LINK += cp -rf $$BASEDIR/audio $$BUILDDIR/debug/. &&
    QMAKE_PRE_LINK += cp -rf $$BASEDIR/audio $$BUILDDIR/release/.
}

