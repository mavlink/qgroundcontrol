macx {
    CONFIG(debug, debug|release):LIBS *= -lUtils_debug
    else:LIBS *= -lUtils
} else:win32 {
    CONFIG(debug, debug|release):LIBS *= -lUtilsd
    else:LIBS *= -lUtils
} else {
    LIBS += -l$$qtLibraryTarget(Utils)
}
