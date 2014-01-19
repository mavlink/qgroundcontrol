TEMPLATE = lib
CONFIG += static

QT += network
QT += sql

# Configure OPMapControl for use as an external library
# (needed for the utils dependency as well)
DEFINES += EXTERNAL_USE

# Required for finding the utils headers
INCLUDEPATH +=../../../../libs/

# Configure output directories
DESTDIR = ../build
UI_DIR = uics
MOC_DIR = mocs
OBJECTS_DIR = objs
