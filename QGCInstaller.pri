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
    MacBuild {
        QMAKE_POST_LINK += && $$dirname(QMAKE_QMAKE)/macdeployqt $${DESTDIR}/qgroundcontrol.app -dmg
    }
    
    WindowsBuild {
		QMAKE_POST_LINK += $$escape_expand(\\n) $$quote(del /F "$$DESTDIR_WIN\\$${TARGET}.exp")
		QMAKE_POST_LINK += $$escape_expand(\\n) $$quote(del /F "$$DESTDIR_WIN\\$${TARGET}.ilk")
		QMAKE_POST_LINK += $$escape_expand(\\n) $$quote(del /F "$$DESTDIR_WIN\\$${TARGET}.lib")
		QMAKE_POST_LINK += $$escape_expand(\\n) $$quote(del /F "$$DESTDIR_WIN\\$${TARGET}.pdb")
        QMAKE_POST_LINK += $$escape_expand(\\n) $$quote("\"C:\\Program Files \(x86\)\\NSIS\\makensis.exe\"" /NOCD "\"/XOutFile $${DESTDIR_WIN}\\qgroundcontrol-installer-win32.exe\"" "$$BASEDIR_WIN\\deploy\\qgroundcontrol_installer.nsi")
    }
}
