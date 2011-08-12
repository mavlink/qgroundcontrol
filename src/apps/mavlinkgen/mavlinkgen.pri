# Third-party includes.
# if you include this file with the commands below into
# your Qt project, you can enable your application
# to generate MAVLink code easily.

###### EXAMPLE BEGIN

## Include MAVLink generator
#DEPENDPATH += \
#    src/apps/mavlinkgen
#
#INCLUDEPATH += \
#    src/apps/mavlinkgen
#    src/apps/mavlinkgen/ui \
#    src/apps/mavlinkgen/generator
#
#include(src/apps/mavlinkgen/mavlinkgen.pri)

###### EXAMPLE END



INCLUDEPATH += .\
    ui \
    generator

FORMS += ui/XMLCommProtocolWidget.ui

HEADERS += \
    ui/XMLCommProtocolWidget.h \
    generator/MAVLinkXMLParser.h \
    ui/DomItem.h \
    ui/DomModel.h \
    ui/QGCMAVLinkTextEdit.h
SOURCES += \
    ui/XMLCommProtocolWidget.cc \
    ui/DomItem.cc \
    ui/DomModel.cc \
    generator/MAVLinkXMLParser.cc \
    ui/QGCMAVLinkTextEdit.cc

RESOURCES += mavlinkgen.qrc
