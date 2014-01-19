TEMPLATE = lib
CONFIG += static
TARGET = qwt
VERSION = 5.1

# Set up QT for compilation
QT += svg

# Make sure Qwt knows that it's a shared lib.
DEFINES += QT_DLL QWT_DLL QWT_MAKEDLL

# And include all source/header files
include( qwt.pri )
