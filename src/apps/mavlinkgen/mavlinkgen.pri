# MAVLink code generator
# generates code in several languages for MAVLink encoding/decoding

INCLUDEPATH += .\
    ui \
    generator

FORMS += ui/XMLCommProtocolWidget.ui

HEADERS += MAVLinkGen.h \
    ui/XMLCommProtocolWidget.h \
    generator/MAVLinkXMLParser.h \
    ui/DomItem.h \
    ui/DomModel.h \
    ui/QGCMAVLinkTextEdit.h
SOURCES += main.cc \
    MAVLinkGen.cc \
    ui/XMLCommProtocolWidget.cc \
    ui/DomItem.cc \
    ui/DomModel.cc \
    generator/MAVLinkXMLParser.cc \
    ui/QGCMAVLinkTextEdit.cc
    
RESOURCES = mavlinkgen.qrc
