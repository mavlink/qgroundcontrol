/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

/// @file
///     @author Gus Grubba <mavlink@grubba.com>

#include "ScreenTools.h"
#include "MainWindow.h"

ScreenTools::ScreenTools()
    : _dotsPerInch(96.0)
    , _dpiFactor( 72.0 / 96.0)
{
    // Get screen DPI to manage font sizes on different platforms
    QScreen *srn = QGuiApplication::primaryScreen();
    if(srn && srn->logicalDotsPerInch() > 50.0) {
        _dotsPerInch = (double)srn->logicalDotsPerInch(); // Font point sizes are based on Mac 72dpi
        _dpiFactor = 72.0 / _dotsPerInch;
    } else {
        qWarning() << "System not reporting logical DPI, which is used to compute the appropriate font size. The default being used is 96dpi. If the text within buttons and UI elements are too big or too small, that's the reason.";
    }
    connect(MainWindow::instance(), &MainWindow::repaintCanvas, this, &ScreenTools::_updateCanvas);
}

void ScreenTools::_updateCanvas()
{
    emit repaintRequestedChanged();
}
