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

// Pixel size, instead of a physical thing is actually a philosophical question when
// it comes to Qt.
// The values below came from actually measuring the elements on the screen on these
// devices. I have yet to find a constant from Qt so these things can be properly
// computed at runtime.

#if defined(Q_OS_OSX)
double ScreenTools::_pixelFactor    = 1.0;
#elif defined(__ios__)
double ScreenTools::_pixelFactor    = 0.75;
#elif defined(Q_OS_WIN)
double ScreenTools::_pixelFactor    = 0.86;
#elif defined(__android__)
double ScreenTools::_pixelFactor    = 2.5;
#elif defined(Q_OS_LINUX)
double ScreenTools::_pixelFactor    = 1.0;
#endif

#if defined(__android__)
#define FONT_FACTOR 2
#else
#define FONT_FACTOR 1
#endif

int ScreenTools::_font8  = 8  * FONT_FACTOR;
int ScreenTools::_font9  = 9  * FONT_FACTOR;
int ScreenTools::_font10 = 10 * FONT_FACTOR;
int ScreenTools::_font11 = 11 * FONT_FACTOR;
int ScreenTools::_font12 = 12 * FONT_FACTOR;
int ScreenTools::_font13 = 13 * FONT_FACTOR;
int ScreenTools::_font14 = 14 * FONT_FACTOR;
int ScreenTools::_font15 = 15 * FONT_FACTOR;
int ScreenTools::_font16 = 16 * FONT_FACTOR;
int ScreenTools::_font17 = 17 * FONT_FACTOR;
int ScreenTools::_font18 = 18 * FONT_FACTOR;
int ScreenTools::_font19 = 19 * FONT_FACTOR;
int ScreenTools::_font20 = 20 * FONT_FACTOR;
int ScreenTools::_font21 = 21 * FONT_FACTOR;
int ScreenTools::_font22 = 22 * FONT_FACTOR;

ScreenTools::ScreenTools()
{
    MainWindow* mainWindow = MainWindow::instance();
    // Unit tests can run Qml without MainWindow
    if (mainWindow) {
        connect(mainWindow, &MainWindow::repaintCanvas, this, &ScreenTools::_updateCanvas);
    }
}

qreal ScreenTools::adjustPixelSize(qreal pixelSize)
{
    return adjustPixelSize_s(pixelSize);
}

qreal ScreenTools::adjustPixelSize_s(qreal pixelSize)
{
    return pixelSize * _pixelFactor;
}

void ScreenTools::_updateCanvas()
{
    emit repaintRequestedChanged();
}
