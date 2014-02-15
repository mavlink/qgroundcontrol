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

QMAKE_POST_LINK += $$quote(echo "Copying files")

#
# Copy the application resources to the associated place alongside the application
#

COPY_RESOURCE_LIST = \
    $$BASEDIR/files \
    $$BASEDIR/qml \
    $$BASEDIR/data
    
WindowsBuild {
	DESTDIR_COPY_RESOURCE_LIST = $$replace(DESTDIR,"/","\\")
    COPY_RESOURCE_LIST = $$replace(COPY_RESOURCE_LIST, "/","\\")
    CONCATCMD = $$escape_expand(\\n)
}

LinuxBuild {
    DESTDIR_COPY_RESOURCE_LIST = $$DESTDIR
    CONCATCMD = &&
}

MacBuild {
    DESTDIR_COPY_RESOURCE_LIST = $$DESTDIR/$${TARGET}.app/Contents/MacOS
    CONCATCMD = &&
}
    
for(COPY_DIR, COPY_RESOURCE_LIST):QMAKE_POST_LINK += $$CONCATCMD $$QMAKE_COPY_DIR $${COPY_DIR} $$DESTDIR_COPY_RESOURCE_LIST

#
# Perform platform specific setup
#

MacBuild {
	# Copy non-standard libraries and frameworks into app package
    QMAKE_POST_LINK += && $$QMAKE_COPY_DIR $$BASEDIR/libs/lib/mac64/lib $$DESTDIR/$${TARGET}.app/Contents/libs
    QMAKE_POST_LINK += && $$QMAKE_COPY_DIR -L $$BASEDIR/libs/lib/Frameworks $$DESTDIR/$${TARGET}.app/Contents/Frameworks

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
	# Copy dependencies
	BASEDIR_WIN = $$replace(BASEDIR,"/","\\")
	DESTDIR_WIN = $$replace(DESTDIR,"/","\\")

    QMAKE_POST_LINK += $$escape_expand(\\n) $$quote($$QMAKE_COPY_DIR "$$(QTDIR)\\plugins" "$$DESTDIR_WIN")

    COPY_FILE_DESTDIR = $$DESTDIR_WIN
	DebugBuild: DLL_QT_DEBUGCHAR = "d"
    ReleaseBuild: DLL_QT_DEBUGCHAR = ""
    COPY_FILE_LIST = \
        $$BASEDIR_WIN\\libs\\lib\\sdl\\win32\\SDL.dll \
        $$BASEDIR_WIN\\libs\\thirdParty\\libxbee\\lib\\libxbee.dll \
        $$(QTDIR)\\bin\\phonon$${DLL_QT_DEBUGCHAR}4.dll \
        $$(QTDIR)\\bin\\QtCore$${DLL_QT_DEBUGCHAR}4.dll \
        $$(QTDIR)\\bin\\QtGui$${DLL_QT_DEBUGCHAR}4.dll \
        $$(QTDIR)\\bin\\QtMultimedia$${DLL_QT_DEBUGCHAR}4.dll \
        $$(QTDIR)\\bin\\QtNetwork$${DLL_QT_DEBUGCHAR}4.dll \
        $$(QTDIR)\\bin\\QtOpenGL$${DLL_QT_DEBUGCHAR}4.dll \
        $$(QTDIR)\\bin\\QtSql$${DLL_QT_DEBUGCHAR}4.dll \
        $$(QTDIR)\\bin\\QtSvg$${DLL_QT_DEBUGCHAR}4.dll \
        $$(QTDIR)\\bin\\QtTest$${DLL_QT_DEBUGCHAR}4.dll \
        $$(QTDIR)\\bin\\QtWebKit$${DLL_QT_DEBUGCHAR}4.dll \
        $$(QTDIR)\\bin\\QtXml$${DLL_QT_DEBUGCHAR}4.dll \
        $$(QTDIR)\\bin\\QtXmlPatterns$${DLL_QT_DEBUGCHAR}4.dll \
        $$(QTDIR)\\bin\\QtDeclarative$${DLL_QT_DEBUGCHAR}4.dll \
        $$(QTDIR)\\bin\\QtScript$${DLL_QT_DEBUGCHAR}4.dll
    for(COPY_FILE, COPY_FILE_LIST) {
        QMAKE_POST_LINK += $$escape_expand(\\n) $$quote($$QMAKE_COPY "$$COPY_FILE" "$$COPY_FILE_DESTDIR")
    }

	ReleaseBuild {
		QMAKE_POST_LINK += $$escape_expand(\\n) $$quote(del /F "$$DESTDIR_WIN\\$${TARGET}.exp")

        # This next xcopy does not always work because the files are not there by default, so disabling for now while 
        # we figure out the right way to do it

		# Copy Visual Studio DLLs
		# Note that this is only done for release because the debugging versions of these DLLs cannot be redistributed.
		# I'm not certain of the path for VS2008, so this only works for VS2010.
		win32-msvc2010_NotWorkingButKeepingIn {
			QMAKE_POST_LINK += $$escape_expand(\\n) $$quote(xcopy /D /Y "\"C:\\Program Files \(x86\)\\Microsoft Visual Studio 10.0\\VC\\redist\\x86\\Microsoft.VC100.CRT\\*.dll\""  "$$DESTDIR_WIN\\")
		}
	}
}
