################################################################################
#
# (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
#
# QGroundControl is licensed according to the terms in the file
# COPYING.md in the root of the source code directory.
#
################################################################################

QMAKE_POST_LINK += echo "Copying files"

#
# Copy the application resources to the associated place alongside the application
#

LinuxBuild {
    DESTDIR_COPY_RESOURCE_LIST = $$DESTDIR
}

MacBuild {
    DESTDIR_COPY_RESOURCE_LIST = $$DESTDIR/$${TARGET}.app/Contents/MacOS
}

# Windows version of QMAKE_COPY_DIR of course doesn't work the same as Mac/Linux. It will only
# copy the contents of the source directory. It doesn't create the top level source directory
# in the target.
WindowsBuild {
    # Make sure to keep both side of this if using the same set of directories
    DESTDIR_COPY_RESOURCE_LIST = $$replace(DESTDIR,"/","\\")
    BASEDIR_COPY_RESOURCE_LIST = $$replace(BASEDIR,"/","\\")
    QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY_DIR \"$$BASEDIR_COPY_RESOURCE_LIST\\resources\\flightgear\" \"$$DESTDIR_COPY_RESOURCE_LIST\\flightgear\"
} else {
    !MobileBuild {
        # Make sure to keep both sides of this if using the same set of directories
        QMAKE_POST_LINK += && $$QMAKE_COPY_DIR $$BASEDIR/resources/flightgear $$DESTDIR_COPY_RESOURCE_LIST
    }
}

#
# Perform platform specific setup
#

MacBuild {
    # Update version info in bundle
    QMAKE_POST_LINK += && /usr/libexec/PlistBuddy -c \"Set :CFBundleShortVersionString $${MAC_VERSION}\" $$DESTDIR/$${TARGET}.app/Contents/Info.plist
    QMAKE_POST_LINK += && /usr/libexec/PlistBuddy -c \"Set :CFBundleVersion $${MAC_BUILD}\" $$DESTDIR/$${TARGET}.app/Contents/Info.plist
}

MacBuild {
    # Copy non-standard frameworks into app package
    QMAKE_POST_LINK += && rsync -a --delete $$BASEDIR/libs/Frameworks $$DESTDIR/$${TARGET}.app/Contents/
    # SDL2 Framework
    QMAKE_POST_LINK += && install_name_tool -change "@rpath/SDL2.framework/Versions/A/SDL2" "@executable_path/../Frameworks/SDL2.framework/Versions/A/SDL2" $$DESTDIR/$${TARGET}.app/Contents/MacOS/$${TARGET}
    # AirMap
    contains (DEFINES, QGC_AIRMAP_ENABLED) {
        QMAKE_POST_LINK += && rsync -a $$BASEDIR/libs/airmapd/macOS/$$AIRMAP_QT_PATH/* $$DESTDIR/$${TARGET}.app/Contents/Frameworks/
        QMAKE_POST_LINK += && install_name_tool -change "@rpath/libairmap-qt.0.0.1.dylib" "@executable_path/../Frameworks/libairmap-qt.0.0.1.dylib" $$DESTDIR/$${TARGET}.app/Contents/MacOS/$${TARGET}
    }
}

WindowsBuild {
    BASEDIR_WIN = $$replace(BASEDIR, "/", "\\")
    DESTDIR_WIN = $$replace(DESTDIR, "/", "\\")
    QT_BIN_DIR  = $$dirname(QMAKE_QMAKE)

    # Copy dependencies
    DebugBuild: DLL_QT_DEBUGCHAR = "d"
    ReleaseBuild: DLL_QT_DEBUGCHAR = ""
    COPY_FILE_LIST = \
        $$BASEDIR\\libs\\sdl2\\msvc\\lib\\x64\\SDL2.dll \
        $$BASEDIR\\deploy\\libcrypto-1_1-x64.dll \
        $$BASEDIR_WIN\\deploy\\libssl-1_1-x64.dll

    for(COPY_FILE, COPY_FILE_LIST) {
        QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY \"$$COPY_FILE\" \"$$DESTDIR_WIN\"
    }

    ReleaseBuild {
        # Copy Visual Studio DLLs
        # Note that this is only done for release because the debugging versions of these DLLs cannot be redistributed.
        QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY \"$$BASEDIR\\deploy\\msvcp140.dll\"  \"$$DESTDIR_WIN\"
        QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY \"$$BASEDIR\\deploy\\msvcp140_1.dll\"  \"$$DESTDIR_WIN\"
        QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY \"$$BASEDIR\\deploy\\msvcp140_2.dll\"  \"$$DESTDIR_WIN\"
        QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY \"$$BASEDIR\\deploy\\vcruntime140.dll\"  \"$$DESTDIR_WIN\"
    }

    DEPLOY_TARGET = $$shell_quote($$shell_path($$DESTDIR_WIN\\$${TARGET}.exe))
    QMAKE_POST_LINK += $$escape_expand(\\n) $$QT_BIN_DIR\\windeployqt --qmldir=$${BASEDIR_WIN}\\src $${DEPLOY_TARGET}
}

LinuxBuild {
    QMAKE_POST_LINK += && mkdir -p $$DESTDIR/Qt/libs && mkdir -p $$DESTDIR/Qt/plugins

    # QT_INSTALL_LIBS
    QT_LIB_LIST = \
        libQt5Charts.so.5 \
        libQt5Core.so.5 \
        libQt5DBus.so.5 \
        libQt5Gui.so.5 \
        libQt5Location.so.5 \
        libQt5Multimedia.so.5 \
        libQt5MultimediaQuick.so.5 \
        libQt5Network.so.5 \
        libQt5OpenGL.so.5 \
        libQt5Positioning.so.5 \
        libQt5PositioningQuick.so.5 \
        libQt5PrintSupport.so.5 \
        libQt5Qml.so.5 \
        libQt5Quick.so.5 \
        libQt5QuickControls2.so.5 \
        libQt5QuickTemplates2.so.5 \
        libQt5QuickWidgets.so.5 \
        libQt5SerialPort.so.5 \
        libQt5Sql.so.5 \
        libQt5Svg.so.5 \
        libQt5Test.so.5 \
        libQt5Widgets.so.5 \
        libQt5X11Extras.so.5 \
        libQt5XcbQpa.so.5 \
        libQt5Xml.so.5 \
        libicui18n.so* \
        libQt5TextToSpeech.so.5

    !contains(DEFINES, __rasp_pi2__) {
        # Some Qt distributions link with *.so.56
        QT_LIB_LIST += \
            libicudata.so.56 \
            libicui18n.so.56 \
            libicuuc.so.56
    }

    for(QT_LIB, QT_LIB_LIST) {
        QMAKE_POST_LINK += && $$QMAKE_COPY --dereference $$[QT_INSTALL_LIBS]/$$QT_LIB $$DESTDIR/Qt/libs/
    }

    # QT_INSTALL_PLUGINS
    QT_PLUGIN_LIST = \
        bearer \
        geoservices \
        iconengines \
        imageformats \
        platforminputcontexts \
        platforms \
        position \
        sqldrivers \
        texttospeech

    !contains(DEFINES, __rasp_pi2__) {
        QT_PLUGIN_LIST += xcbglintegrations
    }

    for(QT_PLUGIN, QT_PLUGIN_LIST) {
        QMAKE_POST_LINK += && $$QMAKE_COPY --dereference --recursive $$[QT_INSTALL_PLUGINS]/$$QT_PLUGIN $$DESTDIR/Qt/plugins/
    }

    # QT_INSTALL_QML
    QMAKE_POST_LINK += && $$QMAKE_COPY --dereference --recursive $$[QT_INSTALL_QML] $$DESTDIR/Qt/

    # Airmap
    contains (DEFINES, QGC_AIRMAP_ENABLED) {
        QMAKE_POST_LINK += && $$QMAKE_COPY $$PWD/libs/airmapd/linux/Qt.5.11.0/libairmap-qt.so.0.0.1 $$DESTDIR/Qt/libs/
    }

    # QGroundControl start script
    QMAKE_POST_LINK += && $$QMAKE_COPY $$BASEDIR/deploy/qgroundcontrol-start.sh $$DESTDIR
    QMAKE_POST_LINK += && $$QMAKE_COPY $$BASEDIR/deploy/qgroundcontrol.desktop $$DESTDIR
    QMAKE_POST_LINK += && $$QMAKE_COPY $$BASEDIR/resources/icons/qgroundcontrol.png $$DESTDIR
}
