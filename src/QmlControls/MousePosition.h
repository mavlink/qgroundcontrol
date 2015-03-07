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
///     @author Don Gagne <don@thegagnes.com>

#ifndef MOUSEPOSITION_H
#define MOUSEPOSITION_H

#include <QObject>
#include <QCursor>

/// This Qml control is used to return global mouse positions. It is needed to fix
/// a problem with hover state of buttons not being updated correctly if the mouse
/// moves out of a QQuickWidget control.
class MousePosition : public QObject
{
    Q_OBJECT
    
public:
    MousePosition(void);
    
    Q_PROPERTY(int mouseX READ mouseX)
    Q_PROPERTY(int mouseY READ mouseY)
    
    int mouseX(void) { return QCursor::pos().x(); }
    int mouseY(void) { return QCursor::pos().y(); }
};

#endif
