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

/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "QGCQmlWidgetHolder.h"
#include "QGCMessageBox.h"

QGCQmlWidgetHolder::QGCQmlWidgetHolder(QWidget *parent) :
    QWidget(parent)
{
    _ui.setupUi(this);
    _ui.qmlWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
}

QGCQmlWidgetHolder::~QGCQmlWidgetHolder()
{

}

void QGCQmlWidgetHolder::setAutoPilot(AutoPilotPlugin* autoPilot)
{
    _ui.qmlWidget->rootContext()->setContextProperty("autopilot", autoPilot);
}

bool QGCQmlWidgetHolder::setSource(const QUrl& qmlUrl)
{
    _ui.qmlWidget->setSource(qmlUrl);
    if (_ui.qmlWidget->status() != QQuickWidget::Ready) {
        QString errorList;
        
        foreach (QQmlError error, _ui.qmlWidget->errors()) {
            errorList += error.toString();
            errorList += "\n";
        }
        QGCMessageBox::warning(tr("Qml Error"), tr("Source not ready: %1\nErrors:\n%2").arg(_ui.qmlWidget->status()).arg(errorList));
        return false;
    }
    
    return true;
}
