QTSERIALPORT_PROJECT_INCLUDEDIR = $$QTSERIALPORT_BUILD_ROOT/include/QtSerialPort
QTSERIALPORT_PROJECT_INCLUDEDIR ~=s,/,$$QMAKE_DIR_SEP,

system("$$QMAKE_MKDIR $$QTSERIALPORT_PROJECT_INCLUDEDIR")

for(header_file, PUBLIC_HEADERS) {
   header_file ~=s,/,$$QMAKE_DIR_SEP,
   system("$$QMAKE_COPY \"$${header_file}\" \"$$QTSERIALPORT_PROJECT_INCLUDEDIR\"")
}

# This is a quick workaround for generating forward header with Qt4.

unix {
    system("echo \'$${LITERAL_HASH}include \"qserialport.h\"\' > \"$$QTSERIALPORT_PROJECT_INCLUDEDIR/QSerialPort\"")
    system("echo \'$${LITERAL_HASH}include \"qserialportinfo.h\"\' > \"$$QTSERIALPORT_PROJECT_INCLUDEDIR/QSerialPortInfo\"")
} win32 {
    system("echo $${LITERAL_HASH}include \"qserialport.h\" > \"$$QTSERIALPORT_PROJECT_INCLUDEDIR/QSerialPort\"")
    system("echo $${LITERAL_HASH}include \"qserialportinfo.h\" > \"$$QTSERIALPORT_PROJECT_INCLUDEDIR/QSerialPortInfo\"")
}

PUBLIC_HEADERS += \
     $$PUBLIC_HEADERS \
     \"$$QTSERIALPORT_PROJECT_INCLUDEDIR/QSerialPort\" \
     \"$$QTSERIALPORT_PROJECT_INCLUDEDIR/QSerialPortInfo\"

target_headers.files  = $$PUBLIC_HEADERS
target_headers.path   = $$[QT_INSTALL_PREFIX]/include/QtSerialPort
INSTALLS              += target_headers

mkspecs_features.files    = $$QTSERIALPORT_PROJECT_ROOT/src/serialport/qt4support/serialport.prf
mkspecs_features.path     = $$[QT_INSTALL_DATA]/mkspecs/features
INSTALLS                  += mkspecs_features

win32 {
   dlltarget.path = $$[QT_INSTALL_BINS]
   INSTALLS += dlltarget
}

target.path = $$[QT_INSTALL_LIBS]
INSTALLS += target

INCLUDEPATH += $$QTSERIALPORT_BUILD_ROOT/include $$QTSERIALPORT_BUILD_ROOT/include/QtSerialPort
DEFINES += QT_BUILD_SERIALPORT_LIB
