QPORTDIR               = src/lib/qextserialport

INCLUDEPATH             += $$QPORTDIR
HEADERS                 += $$QPORTDIR/qextserialbase.h \
                          $$QPORTDIR/qextserialport.h \
                          $$QPORTDIR/qextserialenumerator.h
SOURCES                 += $$QPORTDIR/qextserialbase.cpp \
                          $$QPORTDIR/qextserialport.cpp \
                          $$QPORTDIR/qextserialenumerator.cpp

unix:HEADERS           += $$QPORTDIR/posix_qextserialport.h
unix:SOURCES           += $$QPORTDIR/posix_qextserialport.cpp
unix:DEFINES           += _TTY_POSIX_


win32:HEADERS          += $$QPORTDIR/win_qextserialport.h
win32:SOURCES          += $$QPORTDIR/win_qextserialport.cpp
win32:DEFINES          += _TTY_WIN_

win32:LIBS             += -lsetupapi

unix:VERSION            = 1.2.0
