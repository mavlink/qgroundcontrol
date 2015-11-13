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

#ifndef QGCQmlWidgetHolder_h
#define QGCQmlWidgetHolder_h

/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "QGCDockWidget.h"
#include "AutoPilotPlugin.h"

#include "ui_QGCQmlWidgetHolder.h"

namespace Ui {
class QGCQmlWidgetHolder;
}

/// This is used to create widgets which are implemented in QML.

class QGCQmlWidgetHolder : public QGCDockWidget
{
    Q_OBJECT

public:
    // This has a title and action since the base class is QGCDockWidget. In order to use this
    // control as a normal QWidget, not a doc widget just pass in:
    //      title = QString()
    //      action = NULL
    explicit QGCQmlWidgetHolder(const QString& title, QAction* action, QWidget *parent = 0);
    ~QGCQmlWidgetHolder();

    /// Sets the UAS into the widget which in turn will load facts into the context
    void setAutoPilot(AutoPilotPlugin* autoPilot);

    /// Get Root Context
    QQmlContext* getRootContext(void);

    /// Get Root Object
    QQuickItem* getRootObject(void);

    /// Get QML Engine
    QQmlEngine*	getEngine();

    /// Sets the QML into the control. Will display errors message box if error occurs loading source.
    /// @return true: source loaded, false: source not loaded, errors occured
    bool setSource(const QUrl& qmlUrl);

    void setContextPropertyObject(const QString& name, QObject* object);

    /// Sets the resize mode for the QQuickWidget container
    void setResizeMode(QQuickWidget::ResizeMode resizeMode);

private:
    Ui::QGCQmlWidgetHolder _ui;
};

#endif
