/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "QGCQuickWidget.h"
#include "MultiVehicleManager.h"
#include "JoystickManager.h"
#include "QGCApplication.h"

#include <QQmlContext>
#include <QQmlEngine>

/// @file
///     @brief Subclass of QQuickWidget which injects Facts and the Palette object into
///             the QML context.
///
///     @author Don Gagne <don@thegagnes.com>

QGCQuickWidget::QGCQuickWidget(QWidget* parent) :
    QQuickWidget(parent)
{
#ifndef Q_OS_MAC
    // The following causes the Map control to hang after doing a pinch gesture on mac trackpads.
    // By not turning this on for macos we lose pinch gesture, but two finger scroll still zooms the map.
    // So it's a decent workaround. Qt bug reported: 53634
    setAttribute(Qt::WA_AcceptTouchEvents);
#endif
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
        qgcApp()->showMessage(tr("Source not ready: Status(%1)\nErrors:\n%2").arg(status()).arg(errorList));
        return false;
    }
    
    return true;
}
