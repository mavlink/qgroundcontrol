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

#ifndef SCREENTOOLS_H
#define SCREENTOOLS_H

#include <QObject>
#include <QCursor>

/// This Qml control is used to return screen parameters
class ScreenTools : public QObject
{
    Q_OBJECT
public:
    ScreenTools();

    Q_PROPERTY(double screenDPI READ screenDPI CONSTANT)
    Q_PROPERTY(double dpiFactor READ dpiFactor CONSTANT)
    Q_PROPERTY(int mouseX READ mouseX)
    Q_PROPERTY(int mouseY READ mouseY)

    double screenDPI(void) { return _dotsPerInch; }
    double dpiFactor(void) { return _dpiFactor; }

    int mouseX(void) { return QCursor::pos().x(); }
    int mouseY(void) { return QCursor::pos().y(); }

private:
    double _dotsPerInch;
    double _dpiFactor;

};

#endif
