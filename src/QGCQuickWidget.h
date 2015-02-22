/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

#ifndef QGCQuickWidget_H
#define QGCQuickWidget_H

#include <QQuickWidget>

#include "AutoPilotPlugin.h"

/// @file
///     @brief Subclass of QQuickWidget which injects Facts and the Pallete object into
///             the QML context.
///
///     @author Don Gagne <don@thegagnes.com>

class QGCQuickWidget : public QQuickWidget {
    Q_OBJECT
    
public:
    QGCQuickWidget(QWidget* parent = NULL);
    
    /// Sets the UAS into the widget which in turn will load facts into the context
    void setAutoPilot(AutoPilotPlugin* autoPilot);
    
    /// Sets the QML into the control. Will display errors message box if error occurs loading source.
    ///     @return true: source loaded, false: source not loaded, errors occured
    bool setSource(const QUrl& qmlUrl);
};

#endif
