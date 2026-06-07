QT.qml.VERSION = 6.10.3
QT.qml.name = QtQml
QT.qml.module = Qt6Qml
QT.qml.libs = $$QT_MODULE_LIB_BASE
QT.qml.ldflags = 
QT.qml.includes = $$QT_MODULE_INCLUDE_BASE $$QT_MODULE_INCLUDE_BASE/QtQml
QT.qml.frameworks = 
QT.qml.bins = $$QT_MODULE_BIN_BASE
QT.qml.plugin_types = qmltooling
QT.qml.depends =  core qmlintegration network
QT.qml.uses = 
QT.qml.module_config = v2
QT.qml.DEFINES = QT_QML_LIB
QT.qml.enabled_features = qml-network qml-ssl qml-debug
QT.qml.disabled_features = 
QT_CONFIG += qml-network qml-ssl qml-debug
QT_MODULES += qml

