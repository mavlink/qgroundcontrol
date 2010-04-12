#-------------------------------------------------
#
# MAVGround - Micro Air Vehicle Groundstation
# 
# Please see our website at <http://qgroundcontrol.org>
#
# Original Author:
# Lorenz Meier <mavteam@student.ethz.ch>
#
# Contributing Authors (in alphabetical order):
# 
# (c) 2009 PIXHAWK Team
#
# This file is part of the mav groundstation project
# MAVGround is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# MAVGround is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with MAVGround. If not, see <http://www.gnu.org/licenses/>.
#
#-------------------------------------------------

QT       += network opengl svg xml phonon

TEMPLATE = app
TARGET = qgroundcontrol

BASEDIR = .
BUILDDIR = build
LANGUAGE = C++

CONFIG += debug_and_release console

OBJECTS_DIR = $$BUILDDIR/obj
MOC_DIR = $$BUILDDIR/moc
UI_HEADERS_DIR = src/ui/generated

#$$BASEDIR/lib/qextserialport/include
#               $$BASEDIR/lib/openjaus/libjaus/include \
#               $$BASEDIR/lib/openjaus/libopenJaus/include

message(Qt version $$[QT_VERSION])



# MAC OS X
macx { 

    HARDWARE_PLATFORM = $$system(uname -a)
    contains( HARDWARE_PLATFORM, 9.8.0 ) {
        # x86 Mac OS X Leopard 10.5 and earlier
        CONFIG += x86 cocoa phonon
        message(Building for Mac OS X 32bit/Leopard 10.5 and earlier)

		# Enable function-profiling with the OS X saturn tool
		debug {
			QMAKE_CXXFLAGS += -finstrument-functions
			LIBS += -lSaturn
		}
    } else {
        # x64 Mac OS X Snow Leopard 10.6 and later
        CONFIG += x86_64 cocoa
        CONFIG -= x86 phonon
        message(Building for Mac OS X 64bit/Snow Leopard 10.6 and later)
    }

    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.5

    DESTDIR = $$BASEDIR/bin/mac
    INCLUDEPATH += -framework SDL \
        $$BASEDIR/../mavlink/src

    LIBS += -framework IOKit \
        -framework SDL \
        -framework CoreFoundation \
        -framework ApplicationServices \
        -lm
    
    ICON = $$BASEDIR/images/icons/macx.icns
}

# GNU/Linux
linux-g++ { 
    
    debug {
        DESTDIR = $$BASEDIR
    }

    release {
        DESTDIR = $$BASEDIR
    }
    INCLUDEPATH += /usr/include \
               $$BASEDIR/lib/flite/include \
               $$BASEDIR/lib/flite/lang


    HARDWARE_PLATFORM = $$system(uname -a)
    contains( HARDWARE_PLATFORM, x86_64 ) {
        # 64-bit Linux
        LIBS += \
            -L$$BASEDIR/lib/flite/linux64
        message(Building for GNU/Linux 64bit/x64)
    } else {
        # 32-bit Linux
        LIBS += \
           -L$$BASEDIR/lib/flite/linux32
        message(Building for GNU/Linux 32bit/i386)
    }
    LIBS += -lm \
        -lflite_cmu_us_rms \
        -lflite_cmu_us_slt \
        -lflite_usenglish \
        -lflite_cmulex \
        -lflite \
        -lSDL \
        -lSDLmain
}


# Windows (32bit/64bit)
win32 { 

    message(Building for Windows Platform (32/64bit))
    
    # Special settings for debug
    #CONFIG += CONSOLE
    LIBS += -L$$BASEDIR\lib\sdl\win32 \
        -lmingw32 -lSDLmain -lSDL -mwindows
    
    INCLUDEPATH += $$BASEDIR/lib/sdl/include \
                   C:\Program Files\Microsoft SDKs\Windows\v7.0\Include

    debug {
        DESTDIR = $$BASEDIR/bin
    }

    release {
        DESTDIR = $$BASEDIR/bin
    }
        
}



