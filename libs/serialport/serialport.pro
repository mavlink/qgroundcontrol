QT = core

QMAKE_DOCS = $$PWD/doc/qtserialport.qdocconf
include($$PWD/serialport-lib.pri)

greaterThan(QT_MAJOR_VERSION, 4) {
    load(qt_build_config)
    QT += core-private
    TARGET = QtSerialPort
    load(qt_module)
} else {
    TEMPLATE = lib
    TARGET = $$qtLibraryTarget(QtSerialPort$$QT_LIBINFIX)
    include($$PWD/qt4support/install-helper.pri)
    CONFIG += module create_prl
    mac:QMAKE_FRAMEWORK_BUNDLE_NAME = $$TARGET
}
