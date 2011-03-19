include (../common.pri)
HEADERS += core.h \
    mousewheelzoomtype.h \
    rectangle.h \
    tile.h \
    tilematrix.h \
    loadtask.h \
    copyrightstrings.h \
    pureprojection.h \
    pointlatlng.h \
    rectlatlng.h \
    sizelatlng.h \
    debugheader.h
SOURCES += core.cpp \
    rectangle.cpp \
    tile.cpp \
    tilematrix.cpp \
    pureprojection.cpp \
    rectlatlng.cpp \
    sizelatlng.cpp \
    pointlatlng.cpp \
    loadtask.cpp \
    mousewheelzoomtype.cpp
HEADERS += ./projections/lks94projection.h \
    ./projections/mercatorprojection.h \
    ./projections/mercatorprojectionyandex.h \
    ./projections/platecarreeprojection.h \
    ./projections/platecarreeprojectionpergo.h
SOURCES += ./projections/lks94projection.cpp \
    ./projections/mercatorprojection.cpp \
    ./projections/mercatorprojectionyandex.cpp \
    ./projections/platecarreeprojection.cpp \
    ./projections/platecarreeprojectionpergo.cpp
LIBS += -L../build \
    -lcore
