#-------------------------------------------------
#
# MAVGround - Micro Air Vehicle Groundstation
# 
# Please see our website at <http://pixhawk.ethz.ch>
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

QT       += network opengl svg xml

TEMPLATE = app
TARGET = opengroundcontrol

BASEDIR = .
BUILDDIR = build
LANGUAGE = C++

#CONFIG += static debug
#CONFIG += static release console
CONFIG += static debug_and_release console
QMAKE_CFLAGS += -j8

OBJECTS_DIR = $$BUILDDIR/obj
MOC_DIR = $$BUILDDIR/moc
UI_HEADERS_DIR = src/ui/generated


# Add external libraries
INCLUDEPATH += $$BASEDIR/lib/flite/include \
    $$BASEDIR/lib/flite/lang

#$$BASEDIR/lib/qextserialport/include
#               $$BASEDIR/lib/openjaus/libjaus/include \
#               $$BASEDIR/lib/openjaus/libopenJaus/include

message(Qt version $$[QT_VERSION])



# MAC OS X
macx { 
	message(Building for Mac OS X)

	CONFIG += x86_64
	CONFIG -= x86 static

	QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.5

    DESTDIR = $$BASEDIR/bin/mac
    INCLUDEPATH += -framework SDL \
                $$BASEDIR/MAVLink/src

    LIBS += -framework IOKit \
		-framework SDL \
		-framework CoreFoundation \
		-framework ApplicationServices \
        -lm

    DEFINES += _TTY_POSIX_
    
    ICON = $$BASEDIR/images/icons/macx.icns
}

# GNU/Linux
linux-g++ { 

    message(Building for GNU/Linux)

    QT += phonon
    
    debug {
        DESTDIR = $$BASEDIR
    }

    release {
        DESTDIR = $$BASEDIR
    }
    INCLUDEPATH += /usr/include/SDL

    DEFINES += _TTY_POSIX_

    HARDWARE_PLATFORM = $$system(uname -a)
    contains( HARDWARE_PLATFORM, x86_64 ) {
        # 64-bit Linux
    LIBS += \
        -L$$BASEDIR/lib/flite/linux64 \
        -lm \
        -lflite_cmu_us_awb \
        -lflite_cmu_us_rms \
        -lflite_cmu_us_slt \
        -lflite_usenglish \
        -lflite_cmulex \
        -lflite \
        -lSDL \
        -lSDLmain
    } else {
        # 32-bit Linux
    LIBS += \
        -L$$BASEDIR/lib/flite/linux32 \
        -lm \
        -lflite_cmu_us_awb \
        -lflite_cmu_us_rms \
        -lflite_cmu_us_slt \
        -lflite_usenglish \
        -lflite_cmulex \
        -lflite \
        -lSDL \
        -lSDLmain
    }
}


# Windows (32bit)
win32 { 

    message(Building for Windows Platform (32/64bit))
    
    # Special settings for debug
        #CONFIG += CONSOLE
    LIBS += -L$$BASEDIR\lib\sdl\win32 \
        -lmingw32 -lSDLmain -lSDL -mwindows
    
    INCLUDEPATH += $$BASEDIR/lib/sdl/include/SDL
    
    #LIBS += -L$$BASEDIR\lib\qextserialport\win32 \
    #    -lqextserialport \
    #    -lsetupapi
    #    -L$$BASEDIR/lib/openjaus/libjaus/lib/win32 \
    #    -ljaus \
    #    -L$$BASEDIR/lib/openjaus/libopenJaus/lib/win32 \
    #    -lopenjaus
    
    DEFINES += _TTY_WIN_

    debug {
        DESTDIR = $$BASEDIR/bin
    }

    release {
        DESTDIR = $$BASEDIR/bin
    }
        
}

