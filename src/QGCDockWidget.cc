/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "QGCDockWidget.h"

#include <QCloseEvent>
#include <QSettings>

const char*  QGCDockWidget::_settingsGroup = "DockWidgets";

QGCDockWidget::QGCDockWidget(const QString& title, QAction* action, QWidget* parent)
    : QWidget(parent)
    , _title(title)
	, _action(action)
{
    if (action) {
        setWindowTitle(title);
        setWindowFlags(Qt::Tool);
        loadSettings();
    }
}

// Instead of destroying the widget just hide it
void QGCDockWidget::closeEvent(QCloseEvent* event)
{
    if (_action) {
        saveSettings();
        event->ignore();
        _action->trigger();
    } else {
        QWidget::closeEvent(event);
    }
}

void QGCDockWidget::loadSettings(void)
{
    // TODO: This is crashing for some reason. Disabled until sorted out.
    if (0 /*_action*/) {
        QSettings settings;
        settings.beginGroup(_settingsGroup);
        if (settings.contains(_title)) {
            restoreGeometry(settings.value(_title).toByteArray());
        }
        settings.endGroup();
    }
}

void QGCDockWidget::saveSettings(void)
{
    // TODO: This is crashing for some reason. Disabled until sorted out.
    if (0 /*_action*/) {
        QSettings settings;
        settings.beginGroup(_settingsGroup);
        settings.setValue(_title, saveGeometry());
        settings.endGroup();
    }
}
