DEPENDPATH += src/libs/nmea src/libs/nmea/include
INCLUDEPATH += src/libs/nmea/include

# Input
HEADERS += include/nmea/config.h \
           include/nmea/context.h \
           include/nmea/gmath.h \
           include/nmea/info.h \
           include/nmea/nmea.h \
           include/nmea/parse.h \
           include/nmea/parser.h \
           include/nmea/sentence.h \
           include/nmea/time.h \
           include/nmea/tok.h \
           include/nmea/units.h
SOURCES += context.c \
           gmath.c \
           info.c \
           parse.c \
           parser.c \
           sentence.c \
           time.c \
           tok.c
