############################### *User Config* ###############################

# Uncomment following line if you want to build a static library
# CONFIG += qesp_static

# Uncomment following line if you want to build framework for mac
# macx:CONFIG += qesp_mac_framework

# Uncomment following line if you want to enable udev for linux
# linux*:CONFIG += qesp_linux_udev

# Note: you can create a ".qmake.cache" file, then copy these lines to it.
# If so, you can avoid to change this project file.
############################### *User Config* ###############################

defineReplace(qextLibraryName) {
   unset(LIBRARY_NAME)
   LIBRARY_NAME = $$1
   macx:qesp_mac_framework {
      QMAKE_FRAMEWORK_BUNDLE_NAME = $$LIBRARY_NAME
      export(QMAKE_FRAMEWORK_BUNDLE_NAME)
   } else {
       greaterThan(QT_MAJOR_VERSION, 4):LIBRARY_NAME ~= s,^Qt,Qt$$QT_MAJOR_VERSION,
   }
   CONFIG(debug, debug|release) {
      !debug_and_release|build_pass {
          mac:LIBRARY_NAME = $${LIBRARY_NAME}_debug
          else:win32:LIBRARY_NAME = $${LIBRARY_NAME}d
      }
   }
   return($$LIBRARY_NAME)
}

TEMPLATE=lib
include(src/qextserialport.pri)

#create_prl is needed, otherwise, MinGW can't found libqextserialport1.a
CONFIG += create_prl

#mac framework is designed for shared library
macx:qesp_mac_framework:qesp_static: CONFIG -= qesp_static
!macx:qesp_mac_framework:CONFIG -= qesp_mac_framework

qesp_static {
    CONFIG += static
} else {
    CONFIG += shared
    macx:!qesp_mac_framework:CONFIG += absolute_library_soname
    DEFINES += QEXTSERIALPORT_BUILD_SHARED
}

#Creare lib bundle for mac
macx:qesp_mac_framework {
    CONFIG += lib_bundle
    FRAMEWORK_HEADERS.files = $$PUBLIC_HEADERS
    FRAMEWORK_HEADERS.path = Headers
    QMAKE_BUNDLE_DATA += FRAMEWORK_HEADERS
}

win32|mac:!wince*:!win32-msvc:!macx-xcode:CONFIG += debug_and_release build_all

#For non-windows system, only depends on QtCore module
unix:QT = core
else:QT = core gui

#generate proper library name
greaterThan(QT_MAJOR_VERSION, 4) {
    QESP_LIB_BASENAME = QtExtSerialPort
} else {
    QESP_LIB_BASENAME = qextserialport
}
TARGET = $$qextLibraryName($$QESP_LIB_BASENAME)
VERSION = 1.2.0

# generate feature file by qmake based on this *.in file.
QMAKE_SUBSTITUTES += extserialport.prf.in
OTHER_FILES += extserialport.prf.in

# for make docs
include(doc/doc.pri)

# for make install
win32:!qesp_static {
    dlltarget.path = $$[QT_INSTALL_BINS]
    INSTALLS += dlltarget
}
!macx|!qesp_mac_framework {
    headers.files = $$PUBLIC_HEADERS
    headers.path = $$[QT_INSTALL_HEADERS]/QtExtSerialPort
    INSTALLS += headers
}
target.path = $$[QT_INSTALL_LIBS]

features.files = extserialport.prf
features.path = $$[QMAKE_MKSPECS]/features
INSTALLS += target features
