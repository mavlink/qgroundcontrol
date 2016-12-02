/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
