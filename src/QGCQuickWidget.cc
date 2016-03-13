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

#include "QGCQuickWidget.h"
#include "AutoPilotPluginManager.h"
#include "MultiVehicleManager.h"
#include "JoystickManager.h"
#include "QGCApplication.h"

#include <QQmlContext>
#include <QQmlEngine>

/// @file
///     @brief Subclass of QQuickWidget which injects Facts and the Pallete object into
///             the QML context.
///
///     @author Don Gagne <don@thegagnes.com>

QGCQuickWidget::QGCQuickWidget(QWidget* parent) :
    QQuickWidget(parent)
{
    setAttribute(Qt::WA_AcceptTouchEvents);
    rootContext()->engine()->addImportPath("qrc:/qml");
    rootContext()->setContextProperty("joystickManager", qgcApp()->toolbox()->joystickManager());
}

void QGCQuickWidget::setAutoPilot(AutoPilotPlugin* autoPilot)
{
    rootContext()->setContextProperty("autopilot", autoPilot);
}

bool QGCQuickWidget::setSource(const QUrl& qmlUrl)
{
    QQuickWidget::setSource(qmlUrl);
    if (status() != Ready) {
        QString errorList;
        
        foreach (QQmlError error, errors()) {
            errorList += error.toString();
            errorList += "\n";
        }
        qgcApp()->showMessage(QString("Source not ready: Status(%1)\nErrors:\n%2").arg(status()).arg(errorList));
        return false;
    }
    
    return true;
}
