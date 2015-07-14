# -------------------------------------------------
# QGroundControl - Micro Air Vehicle Groundstation
# Please see our website at <http://qgroundcontrol.org>
# Maintainer:
# Lorenz Meier <lm@inf.ethz.ch>
# (c) 2009-2011 QGroundControl Developers
# This file is part of the open groundstation project
# QGroundControl is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# QGroundControl is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with QGroundControl. If not, see <http://www.gnu.org/licenses/>.
# -------------------------------------------------

QMAKE_POST_LINK += echo "Copying files"

AndroidBuild {
    INSTALLS += $$DESTDIR
}

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
    QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY_DIR \"$$BASEDIR_COPY_RESOURCE_LIST\\flightgear\" \"$$DESTDIR_COPY_RESOURCE_LIST\\flightgear\"
} else {
    !MobileBuild {
        # Make sure to keep both sides of this if using the same set of directories
        QMAKE_POST_LINK += && $$QMAKE_COPY_DIR $$BASEDIR/flightgear $$DESTDIR_COPY_RESOURCE_LIST
    }
}

#
# Perform platform specific setup
#

MacBuild {

	# Copy non-standard libraries and frameworks into app package
    QMAKE_POST_LINK += && $$QMAKE_COPY_DIR $$BASEDIR/libs/lib/mac64/lib $$DESTDIR/$${TARGET}.app/Contents/libs
    QMAKE_POST_LINK += && rsync -a --delete $$BASEDIR/libs/lib/Frameworks $$DESTDIR/$${TARGET}.app/Contents/

	# Fix library paths inside executable

    INSTALL_NAME_TARGET = $$DESTDIR/$${TARGET}.app/Contents/MacOS/$${TARGET}
    INSTALL_NAME_LIB_LIST = \
        libOpenThreads.dylib \
        libosg.dylib \
        libosgViewer.dylib \
        libosgGA.dylib \
        libosgDB.dylib \
        libosgText.dylib \
        libosgWidget.dylib
    for(INSTALL_NAME_LIB, INSTALL_NAME_LIB_LIST) {
        QMAKE_POST_LINK += && install_name_tool -change $$INSTALL_NAME_LIB "@executable_path/../libs/$${INSTALL_NAME_LIB}" $$INSTALL_NAME_TARGET
    }

	# Fix library paths within libraries (inter-library dependencies)

	# OSG GA LIBRARY
    INSTALL_NAME_TARGET = $$DESTDIR/$${TARGET}.app/Contents/libs/libosgGA.dylib
    INSTALL_NAME_LIB_LIST = \
        libOpenThreads.dylib \
        libosg.dylib \
        libosgGA.dylib \
        libosgDB.dylib \
        libosgUtil.dylib
    for(INSTALL_NAME_LIB, INSTALL_NAME_LIB_LIST) {
        QMAKE_POST_LINK += && install_name_tool -change $$INSTALL_NAME_LIB "@executable_path/../libs/$${INSTALL_NAME_LIB}" $$INSTALL_NAME_TARGET
    }

	# OSG DB LIBRARY
    INSTALL_NAME_TARGET = $$DESTDIR/$${TARGET}.app/Contents/libs/libosgDB.dylib
    INSTALL_NAME_LIB_LIST = \
        libOpenThreads.dylib \
        libosg.dylib \
        libosgDB.dylib \
        libosgUtil.dylib
    for(INSTALL_NAME_LIB, INSTALL_NAME_LIB_LIST) {
        QMAKE_POST_LINK += && install_name_tool -change $$INSTALL_NAME_LIB "@executable_path/../libs/$${INSTALL_NAME_LIB}" $$INSTALL_NAME_TARGET
    }

	# OSG TEXT LIBRARY
    INSTALL_NAME_TARGET = $$DESTDIR/$${TARGET}.app/Contents/libs/libosgText.dylib
    INSTALL_NAME_LIB_LIST = \
        libOpenThreads.dylib \
        libosg.dylib \
        libosgDB.dylib \
        libosgUtil.dylib \
        libosgText.dylib
    for(INSTALL_NAME_LIB, INSTALL_NAME_LIB_LIST) {
        QMAKE_POST_LINK += && install_name_tool -change $$INSTALL_NAME_LIB "@executable_path/../libs/$${INSTALL_NAME_LIB}" $$INSTALL_NAME_TARGET
    }

	# OSG UTIL LIBRARY
    INSTALL_NAME_TARGET = $$DESTDIR/$${TARGET}.app/Contents/libs/libosgUtil.dylib
    INSTALL_NAME_LIB_LIST = \
        libOpenThreads.dylib \
        libosg.dylib
    for(INSTALL_NAME_LIB, INSTALL_NAME_LIB_LIST) {
        QMAKE_POST_LINK += && install_name_tool -change $$INSTALL_NAME_LIB "@executable_path/../libs/$${INSTALL_NAME_LIB}" $$INSTALL_NAME_TARGET
    }


	# OSG VIEWER LIBRARY
    INSTALL_NAME_TARGET = $$DESTDIR/$${TARGET}.app/Contents/libs/libosgViewer.dylib
    INSTALL_NAME_LIB_LIST = \
        libOpenThreads.dylib \
        libosg.dylib \
        libosgGA.dylib \
        libosgDB.dylib \
        libosgUtil.dylib \
        libosgText.dylib
    for(INSTALL_NAME_LIB, INSTALL_NAME_LIB_LIST) {
        QMAKE_POST_LINK += && install_name_tool -change $$INSTALL_NAME_LIB "@executable_path/../libs/$${INSTALL_NAME_LIB}" $$INSTALL_NAME_TARGET
    }

	# OSG WIDGET LIBRARY
    INSTALL_NAME_TARGET = $$DESTDIR/$${TARGET}.app/Contents/libs/libosgWidget.dylib
    INSTALL_NAME_LIB_LIST = \
        libOpenThreads.dylib \
        libosg.dylib \
        libosgGA.dylib \
        libosgDB.dylib \
        libosgUtil.dylib \
        libosgText.dylib \
        libosgViewer.dylib
    for(INSTALL_NAME_LIB, INSTALL_NAME_LIB_LIST) {
        QMAKE_POST_LINK += && install_name_tool -change $$INSTALL_NAME_LIB "@executable_path/../libs/$${INSTALL_NAME_LIB}" $$INSTALL_NAME_TARGET
    }

	# CORE OSG LIBRARY
    QMAKE_POST_LINK += && install_name_tool -change libOpenThreads.dylib "@executable_path/../libs/libOpenThreads.dylib" $$DESTDIR/$${TARGET}.app/Contents/libs/libosg.dylib

    # SDL Framework
    QMAKE_POST_LINK += && install_name_tool -change "@rpath/SDL.framework/Versions/A/SDL" "@executable_path/../Frameworks/SDL.framework/Versions/A/SDL" $$DESTDIR/$${TARGET}.app/Contents/MacOS/$${TARGET}

}

WindowsBuild {
    BASEDIR_WIN = $$replace(BASEDIR, "/", "\\")
    DESTDIR_WIN = $$replace(DESTDIR, "/", "\\")
    D_DIR = $$[QT_INSTALL_LIBEXECS]
    DLL_DIR = $$replace(D_DIR, "/", "\\")

    # Copy dependencies
    DebugBuild: DLL_QT_DEBUGCHAR = "d"
    ReleaseBuild: DLL_QT_DEBUGCHAR = ""
    COPY_FILE_LIST = \
        $$BASEDIR\\libs\\lib\\sdl\\win32\\SDL.dll \
        $$BASEDIR\\libs\\thirdParty\\libxbee\\lib\\libxbee.dll \
        $$DLL_DIR\\icu*.dll \
        $$DLL_DIR\\Qt5Core$${DLL_QT_DEBUGCHAR}.dll \
        $$DLL_DIR\\Qt5Gui$${DLL_QT_DEBUGCHAR}.dll \
        $$DLL_DIR\\Qt5Location$${DLL_QT_DEBUGCHAR}.dll \
        $$DLL_DIR\\Qt5Multimedia$${DLL_QT_DEBUGCHAR}.dll \
        $$DLL_DIR\\Qt5MultimediaWidgets$${DLL_QT_DEBUGCHAR}.dll \
        $$DLL_DIR\\Qt5Network$${DLL_QT_DEBUGCHAR}.dll \
        $$DLL_DIR\\Qt5OpenGL$${DLL_QT_DEBUGCHAR}.dll \
        $$DLL_DIR\\Qt5Positioning$${DLL_QT_DEBUGCHAR}.dll \
        $$DLL_DIR\\Qt5PrintSupport$${DLL_QT_DEBUGCHAR}.dll \
        $$DLL_DIR\\Qt5Qml$${DLL_QT_DEBUGCHAR}.dll \
        $$DLL_DIR\\Qt5Quick$${DLL_QT_DEBUGCHAR}.dll \
        $$DLL_DIR\\Qt5QuickWidgets$${DLL_QT_DEBUGCHAR}.dll \
        $$DLL_DIR\\Qt5Sensors$${DLL_QT_DEBUGCHAR}.dll \
        $$DLL_DIR\\Qt5SerialPort$${DLL_QT_DEBUGCHAR}.dll \
        $$DLL_DIR\\Qt5Sql$${DLL_QT_DEBUGCHAR}.dll \
        $$DLL_DIR\\Qt5Svg$${DLL_QT_DEBUGCHAR}.dll \
        $$DLL_DIR\\Qt5Test$${DLL_QT_DEBUGCHAR}.dll \
        $$DLL_DIR\\Qt5WebKit$${DLL_QT_DEBUGCHAR}.dll \
        $$DLL_DIR\\Qt5WebKitWidgets$${DLL_QT_DEBUGCHAR}.dll \
        $$DLL_DIR\\Qt5Widgets$${DLL_QT_DEBUGCHAR}.dll \
        $$DLL_DIR\\Qt5Xml$${DLL_QT_DEBUGCHAR}.dll
    for(COPY_FILE, COPY_FILE_LIST) {
		QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY \"$$COPY_FILE\" \"$$DESTDIR_WIN\"
    }

    # Copy platform plugins
    P_DIR = $$[QT_INSTALL_PLUGINS]
    PLUGINS_DIR_WIN = $$replace(P_DIR, "/", "\\")
    QMAKE_POST_LINK += $$escape_expand(\\n) mkdir "$$DESTDIR_WIN\\platforms"
    QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY     \"$$PLUGINS_DIR_WIN\\platforms\\qwindows$${DLL_QT_DEBUGCHAR}.dll\" \"$$DESTDIR_WIN\\platforms\\qwindows$${DLL_QT_DEBUGCHAR}.dll\"
    QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY_DIR \"$$PLUGINS_DIR_WIN\\imageformats\" \"$$DESTDIR_WIN\\imageformats\"
    QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY_DIR \"$$PLUGINS_DIR_WIN\\sqldrivers\" \"$$DESTDIR_WIN\\sqldrivers\"
    QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY_DIR \"$$PLUGINS_DIR_WIN\\bearer\" \"$$DESTDIR_WIN\\bearer\"
    QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY_DIR \"$$PLUGINS_DIR_WIN\\iconengines\" \"$$DESTDIR_WIN\\iconengines\"
    QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY_DIR \"$$PLUGINS_DIR_WIN\\printsupport\" \"$$DESTDIR_WIN\\printsupport\"
    QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY_DIR \"$$PLUGINS_DIR_WIN\\qmltooling\" \"$$DESTDIR_WIN\\qmltooling\"
    QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY_DIR \"$$PLUGINS_DIR_WIN\\geoservices\" \"$$DESTDIR_WIN\\geoservices\"

    # Copy Qml libraries
    Q_DIR = $$[QT_INSTALL_QML]
    QML_DIR_WIN = $$replace(Q_DIR, "/", "\\")
    QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY_DIR \"$$QML_DIR_WIN\\QtGraphicalEffects\" \"$$DESTDIR_WIN\\QtGraphicalEffects\"
    QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY_DIR \"$$QML_DIR_WIN\\QtQuick\" \"$$DESTDIR_WIN\\QtQuick\"
    QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY_DIR \"$$QML_DIR_WIN\\QtQuick.2\" \"$$DESTDIR_WIN\\QtQuick.2\"
    QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY_DIR \"$$QML_DIR_WIN\\QtLocation\" \"$$DESTDIR_WIN\\QtLocation\"
    QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY_DIR \"$$QML_DIR_WIN\\QtPositioning\" \"$$DESTDIR_WIN\\QtPositioning\"

        ReleaseBuild {
		# Copy Visual Studio DLLs
		# Note that this is only done for release because the debugging versions of these DLLs cannot be redistributed.
		win32-msvc2010 {
			QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY \"C:\\Windows\\System32\\msvcp100.dll\"  \"$$DESTDIR_WIN\"
			QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY \"C:\\Windows\\System32\\msvcr100.dll\"  \"$$DESTDIR_WIN\"
		}
		else:win32-msvc2012 {
			QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY \"C:\\Windows\\System32\\msvcp110.dll\"  \"$$DESTDIR_WIN\"
			QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY \"C:\\Windows\\System32\\msvcr110.dll\"  \"$$DESTDIR_WIN\"
		}
		else:win32-msvc2013 {
			QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY \"C:\\Windows\\System32\\msvcp120.dll\"  \"$$DESTDIR_WIN\"
			QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY \"C:\\Windows\\System32\\msvcr120.dll\"  \"$$DESTDIR_WIN\"
		}
		else {
			error("Visual studio version not supported, installation cannot be completed.")
		}
	}
}
