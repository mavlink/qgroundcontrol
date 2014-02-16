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
        QMAKE_POST_LINK += && rm -f -R $${INSTALLDIR}
        QMAKE_POST_LINK += && mkdir $${INSTALLDIR}
        QMAKE_POST_LINK += && hdiutil create -format UDBZ -quiet -srcfolder $${DESTDIR}/qgroundcontrol.app $${INSTALLDIR}/qgroundcontrol.dmg
    }
    
    WindowsBuild {
    	INSTALLDIR_WIN = $$replace(INSTALLDIR,"/","\\")
        QMAKE_POST_LINK += $$escape_expand(\\n) $$quote(rd /S /Q "$${INSTALLDIR_WIN}")
        QMAKE_POST_LINK += $$escape_expand(\\n) $$quote(md "$${INSTALLDIR_WIN}")
        QMAKE_POST_LINK += $$escape_expand(\\n) $$quote("\"C:\\Program Files \(x86\)\\NSIS\\makensis.exe\"" /NOCD "\"/XOutFile $$INSTALLDIR_WIN\\qgroundcontrol-installer-win32.exe\"" "$$BASEDIR_WIN\\deploy\\qgroundcontrol_installer.nsi")
    }
}
