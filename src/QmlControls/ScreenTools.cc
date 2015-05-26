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

#include <QFont>
#include <QFontMetrics>

const double ScreenTools::_defaultFontPointSize = 13;
const double ScreenTools::_mediumFontPointSize = 16;
const double ScreenTools::_largeFontPointSize = 20;

ScreenTools::ScreenTools()
{
    MainWindow* mainWindow = MainWindow::instance();
    
    // Unit tests can run Qml without MainWindow
    if (mainWindow) {
        connect(mainWindow, &MainWindow::repaintCanvas,     this, &ScreenTools::_updateCanvas);
        connect(mainWindow, &MainWindow::pixelSizeChanged,  this, &ScreenTools::_updatePixelSize);
        connect(mainWindow, &MainWindow::fontSizeChanged,   this, &ScreenTools::_updateFontSize);
    }
}

qreal ScreenTools::adjustFontPointSize(qreal pointSize)
{
    return adjustFontPointSize_s(pointSize);
}

qreal ScreenTools::adjustFontPointSize_s(qreal pointSize)
{
    return pointSize * MainWindow::fontPointFactor();
}

qreal ScreenTools::adjustPixelSize(qreal pixelSize)
{
    return adjustPixelSize_s(pixelSize);
}

qreal ScreenTools::adjustPixelSize_s(qreal pixelSize)
{
    return pixelSize * MainWindow::pixelSizeFactor();
}

void ScreenTools::increasePixelSize()
{
    MainWindow::instance()->setPixelSizeFactor(MainWindow::pixelSizeFactor() + 0.025);
}

void ScreenTools::decreasePixelSize()
{
    MainWindow::instance()->setPixelSizeFactor(MainWindow::pixelSizeFactor() - 0.025);
}

void ScreenTools::increaseFontSize()
{
    MainWindow::instance()->setFontSizeFactor(MainWindow::fontPointFactor() + 0.025);
}

void ScreenTools::decreaseFontSize()
{
    MainWindow::instance()->setFontSizeFactor(MainWindow::fontPointFactor() - 0.025);
}

void ScreenTools::_updateCanvas()
{
    emit repaintRequestedChanged();
}

void ScreenTools::_updatePixelSize()
{
    emit pixelSizeFactorChanged();
}

void ScreenTools::_updateFontSize()
{
    emit fontPointFactorChanged();
    emit fontSizesChanged();
}

double ScreenTools::fontPointFactor()
{
    return MainWindow::fontPointFactor();
}

double ScreenTools::pixelSizeFactor()
{
    return MainWindow::pixelSizeFactor();
}

double ScreenTools::defaultFontPointSize(void)
{
    return _defaultFontPointSize * MainWindow::fontPointFactor();
}

double ScreenTools::mediumFontPointSize(void)
{
    return _mediumFontPointSize * MainWindow::fontPointFactor();
}

double ScreenTools::largeFontPointSize(void)
{
    return _largeFontPointSize * MainWindow::fontPointFactor();
}
