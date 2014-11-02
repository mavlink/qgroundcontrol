OTHER_FILES += $$PWD/qextserialport.qdocconf

#name of qdoc3 has been changed to qdoc under Qt5
QESP_QDOC = qdoc
lessThan(QT_MAJOR_VERSION, 5):QESP_QDOC = qdoc3

docs_target.target = docs
docs_target.commands = $$QESP_QDOC $$PWD/qextserialport.qdocconf

QMAKE_EXTRA_TARGETS = docs_target
QMAKE_CLEAN += "-r $$PWD/html"

