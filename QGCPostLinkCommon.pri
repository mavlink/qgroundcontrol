################################################################################
#
# (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
#
# QGroundControl is licensed according to the terms in the file
# COPYING.md in the root of the source code directory.
#
################################################################################

# These are the Post Link steps which are common to all builds

QMAKE_POST_LINK += echo "Post Link Common"

#
# Perform platform specific setup
#

MacBuild {

    # Qt is screwed up if you use qmake to create an XCode Project which has a DESTDIR set on it.
    # This is because XCode builds create the .app in BUILT_PRODUCTS_DIR. If you use a DESTDIR then
    # Qt adds a Copy Phase to the build which copies the .app from the BUILT_PRODUCTS_DIR to DESTDIR.
    # This causes all sort of problem which are too long to list here. In order to work around this
    # We have to duplicate the post link commands here to work from two different locations. And to deal
    # with the differences between post list command running in a shell script (XCode) versus a makefile (Qt Creator)
    macx-xcode {
        # SDL2 Framework
        QMAKE_POST_LINK += && rsync -a --delete $$SOURCE_DIR/libs/Frameworks/SDL2.Framework $BUILT_PRODUCTS_DIR/$${TARGET}.app/Contents/Frameworks
        QMAKE_POST_LINK += && install_name_tool -change "@rpath/SDL2.framework/Versions/A/SDL2" "@executable_path/../Frameworks/SDL2.framework/Versions/A/SDL2" $BUILT_PRODUCTS_DIR/$${TARGET}.app/Contents/MacOS/$${TARGET}
        # AirMap
        contains (DEFINES, QGC_AIRMAP_ENABLED) {
            QMAKE_POST_LINK += && rsync -a $$SOURCE_DIR/libs/airmapd/macOS/$$AIRMAP_QT_PATH/* $BUILT_PRODUCTS_DIR/$${TARGET}.app/Contents/Frameworks/
            QMAKE_POST_LINK += && install_name_tool -change "@rpath/libairmap-qt.0.0.1.dylib" "@executable_path/../Frameworks/libairmap-qt.0.0.1.dylib" $BUILT_PRODUCTS_DIR/$${TARGET}.app/Contents/MacOS/$${TARGET}
        }
    } else {
        # SDL2 Framework
        QMAKE_POST_LINK += && rsync -a --delete $$SOURCE_DIR/libs/Frameworks/SDL2.Framework $${TARGET}.app/Contents/Frameworks
        QMAKE_POST_LINK += && install_name_tool -change "@rpath/SDL2.framework/Versions/A/SDL2" "@executable_path/../Frameworks/SDL2.framework/Versions/A/SDL2" $${TARGET}.app/Contents/MacOS/$${TARGET}
        # AirMap
        contains (DEFINES, QGC_AIRMAP_ENABLED) {
            QMAKE_POST_LINK += && rsync -a $$SOURCE_DIR/libs/airmap-platform-sdk/macOS/$$AIRMAP_QT_PATH/* $${TARGET}.app/Contents/Frameworks/
            QMAKE_POST_LINK += && install_name_tool -change "@rpath/libairmap-qt.0.0.1.dylib" "@executable_path/../Frameworks/libairmap-qt.0.0.1.dylib" $${TARGET}.app/Contents/MacOS/$${TARGET}
        }
    }
}

WindowsBuild {
    #BASEDIR_WIN = $$replace(SOURCE_DIR, "/", "\\")
    QT_BIN_DIR  = $$dirname(QMAKE_QMAKE)

    # Copy dependencies
    DebugBuild: DLL_QT_DEBUGCHAR = "d"
    ReleaseBuild: DLL_QT_DEBUGCHAR = ""
    COPY_FILE_LIST = \
        $$SOURCE_DIR\\libs\\sdl2\\msvc\\lib\\x64\\SDL2.dll \
        $$SOURCE_DIR\\libs\\OpenSSL\\windows\\libcrypto-1_1-x64.dll \
        $$SOURCE_DIR\\libs\\OpenSSL\\windows\\libssl-1_1-x64.dll

    for(COPY_FILE, COPY_FILE_LIST) {
        QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY \"$$COPY_FILE\" \"$$DESTDIR\"
    }

    ReleaseBuild {
        # Copy Visual Studio DLLs
        # Note that this is only done for release because the debugging versions of these DLLs cannot be redistributed.
        QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY \"$$SOURCE_DIR\\libs\\Microsoft\\windows\\msvcp140.dll\"  \"$$DESTDIR\"
        QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY \"$$SOURCE_DIR\\libs\\Microsoft\\windows\\vcruntime140.dll\"  \"$$DESTDIR\"
    }

    DEPLOY_TARGET = $$shell_quote($$shell_path($$DESTDIR\\$${TARGET}.exe))
    QMAKE_POST_LINK += $$escape_expand(\\n) $$QT_BIN_DIR\\windeployqt --qmldir=$${SOURCE_DIR}\\src $${DEPLOY_TARGET}
}

LinuxBuild {
    QMAKE_POST_LINK += && mkdir -p $$DESTDIR/Qt/libs && mkdir -p $$DESTDIR/Qt/plugins
    QMAKE_RPATHDIR += $ORIGIN/Qt/libs

    # QT_INSTALL_LIBS
    QT_LIB_LIST += \
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
        libQt5QmlModels.so.5 \
        libQt5QmlWorkerScript.so.5 \
        libQt5Quick.so.5 \
        libQt5QuickControls2.so.5 \
        libQt5QuickShapes.so.5 \
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

    # Not all Qt libs are built in all systems. CI doesn't build Wayland, for example.
    QT_LIB_OPTIONALS = \
        libQt5WaylandClient.so.5 \
        libQt5WaylandCompositor.so.5
    for(QT_LIB, QT_LIB_OPTIONALS) {
        exists("$$[QT_INSTALL_LIBS]/$$QT_LIB") {
            QT_LIB_LIST += $$QT_LIB
        }
    }

    exists($$[QT_INSTALL_LIBS]/libicudata.so.56) {
        # Some Qt distributions link with *.so.56
        QT_LIB_LIST += \
            libicudata.so.56 \
            libicui18n.so.56 \
            libicuuc.so.56
    } else {
        QT_LIB_LIST += \
            libicudata.so \
            libicui18n.so \
            libicuuc.so
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
        QMAKE_POST_LINK += && $$QMAKE_COPY $$OUT_PWD/libs/airmap-platform-sdk/linux/$$AIRMAP_QT_PATH/libairmap-cpp.so.2.0.0 $$DESTDIR/Qt/libs/
    }

    # QGroundControl start script
    contains (CONFIG, QGC_DISABLE_CUSTOM_BUILD) | !exists($$PWD/custom/custom.pri) {
        QMAKE_POST_LINK += && $$QMAKE_COPY $$SOURCE_DIR/deploy/qgroundcontrol-start.sh $$DESTDIR
        QMAKE_POST_LINK += && $$QMAKE_COPY $$SOURCE_DIR/deploy/qgroundcontrol.desktop $$DESTDIR
        QMAKE_POST_LINK += && $$QMAKE_COPY $$SOURCE_DIR/resources/icons/qgroundcontrol.png $$DESTDIR
    } else {
        include($$PWD/custom/custom_deploy.pri)
    }

    QMAKE_POST_LINK += && SEARCHDIR="$$DESTDIR/Qt" RPATHDIR="$$DESTDIR/Qt/libs" "$$PWD/deploy/linux-fixup-rpaths.bash"

    # https://doc.qt.io/qt-5/qt-conf.html
    QMAKE_POST_LINK += && $$QMAKE_COPY "$$SOURCE_DIR/deploy/qt.conf" "$$DESTDIR"
}
