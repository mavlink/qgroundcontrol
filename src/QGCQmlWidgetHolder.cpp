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

QGCQmlWidgetHolder::QGCQmlWidgetHolder(const QString& title, QAction* action, QWidget *parent) :
    QGCDockWidget(title, action, parent)
{
    _ui.setupUi(this);

    layout()->setContentsMargins(0,0,0,0);

    if (action) {
        setWindowTitle(title);
    }
    setResizeMode(QQuickWidget::SizeRootObjectToView);
}

QGCQmlWidgetHolder::~QGCQmlWidgetHolder()
{

}

void QGCQmlWidgetHolder::setAutoPilot(AutoPilotPlugin* autoPilot)
{
    setContextPropertyObject("autopilot", autoPilot);
}

bool QGCQmlWidgetHolder::setSource(const QUrl& qmlUrl)
{
    return _ui.qmlWidget->setSource(qmlUrl);
}

void QGCQmlWidgetHolder::setContextPropertyObject(const QString& name, QObject* object)
{
    _ui.qmlWidget->rootContext()->setContextProperty(name, object);
}

QQmlContext* QGCQmlWidgetHolder::getRootContext(void)
{
    return _ui.qmlWidget->rootContext();
}

QQuickItem* QGCQmlWidgetHolder::getRootObject(void)
{
    return _ui.qmlWidget->rootObject();
}

QQmlEngine*	QGCQmlWidgetHolder::getEngine()
{
    return _ui.qmlWidget->engine();
}


void QGCQmlWidgetHolder::setResizeMode(QQuickWidget::ResizeMode resizeMode)
{
    _ui.qmlWidget->setResizeMode(resizeMode);
}
