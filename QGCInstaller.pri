# -------------------------------------------------
# QGroundControl - Micro Air Vehicle Groundstation
# Please see our website at <http://qgroundcontrol.org>
# Maintainer:
# Lorenz Meier <lm@inf.ethz.ch>
# (c) 2009-2014 QGroundControl Developers
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

installer {
    DEFINES += QGC_INSTALL_RELEASE
    MacBuild {
        VideoEnabled {
            # Install the gstreamer framework
            # This will:
            # Copy from the original distibution into libs/lib/Framworks (if not already there)
            # Prune the framework, removing stuff we don't need
            # Relocate all dylibs so they can work under @executable_path/...
            # Copy the result into the app bundle
            # Make sure qgroundcontrol can find them
            message("Preparing GStreamer Framework")
            QMAKE_POST_LINK += && $$BASEDIR/tools/prepare_gstreamer_framework.sh $$BASEDIR/libs/lib/Frameworks/ $$DESTDIR/$${TARGET}.app $${TARGET}
        } else {
            message("Skipping GStreamer Framework")
        }
        # We cd to release directory so we can run macdeployqt without a path to the
        # qgroundcontrol.app file. If you specify a path to the .app file the symbolic
        # links to plugins will not be created correctly.
        QMAKE_POST_LINK += && cd release
        QMAKE_POST_LINK += && $$dirname(QMAKE_QMAKE)/macdeployqt qgroundcontrol.app -verbose=2 -qmldir=../src
        QMAKE_POST_LINK += && cd ..
        QMAKE_POST_LINK += && hdiutil create -layout SPUD -srcfolder $${DESTDIR}/qgroundcontrol.app -volname QGroundControl $${DESTDIR}/qgroundcontrol.dmg
    }
    WindowsBuild {
		# The pdb moving command are commented out for now since we are including the .pdb in the installer. This makes it much
		# easier to debug user crashes.
		#QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY $${DESTDIR_WIN}\\qgroundcontrol.pdb
		#QMAKE_POST_LINK += $$escape_expand(\\n) del $${DESTDIR_WIN}\\qgroundcontrol.pdb
        QMAKE_POST_LINK += $$escape_expand(\\n) $$quote("\"C:\\Program Files \(x86\)\\NSIS\\makensis.exe\"" /NOCD "\"/XOutFile $${DESTDIR_WIN}\\qgroundcontrol-installer-win32.exe\"" "$$BASEDIR_WIN\\deploy\\qgroundcontrol_installer.nsi")
		#QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY qgroundcontrol.pdb $${DESTDIR_WIN}
		#QMAKE_POST_LINK += $$escape_expand(\\n) del qgroundcontrol.pdb
    }
}
