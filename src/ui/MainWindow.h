/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

/**
 * @file
 *   @brief Definition of class MainWindow
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef _MAINWINDOW_H_
#define _MAINWINDOW_H_

#ifdef __mobile__
#error Should not be include in mobile build
#endif

#include <QMainWindow>
#include <QStatusBar>
#include <QStackedWidget>
#include <QSettings>
#include <QList>

#include "LinkManager.h"
#include "LinkInterface.h"
#include "UASInterface.h"
#include "LogCompressor.h"
#include "QGCMAVLinkInspector.h"
#ifndef __mobile__
#include "QGCMAVLinkLogPlayer.h"
#endif
#include "MAVLinkDecoder.h"
#include "Vehicle.h"
#include "QGCDockWidget.h"
#include "QGCQmlWidgetHolder.h"

#include "ui_MainWindow.h"

#if (defined QGC_MOUSE_ENABLED_WIN) | (defined QGC_MOUSE_ENABLED_LINUX)
    #include "Mouse6dofInput.h"
#endif // QGC_MOUSE_ENABLED_WIN

class QGCStatusBar;
class Linecharts;
class QGCDataPlot2D;

/**
 * @brief Main Application Window
 *
 **/
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /// @brief Returns the MainWindow singleton. Will not create the MainWindow if it has not already
    ///         been created.
    static MainWindow* instance(void);

    /// @brief Deletes the MainWindow singleton
    void deleteInstance(void);

    /// @brief Creates the MainWindow singleton. Should only be called once by QGCApplication.
    static MainWindow* _create();

    ~MainWindow();


    /** @brief Get low power mode setting */
    bool lowPowerModeEnabled() const
    {
        return _lowPowerMode;
    }

    /// @brief Saves the last used connection
    void saveLastUsedConnection(const QString connection);

    // Called from MainWindow.qml when the user accepts the window close dialog
    Q_INVOKABLE void reallyClose(void);

    /// @return Root qml object of main window QML
    QObject* rootQmlObject(void);

public slots:
#ifndef __mobile__
    void showSettings();
#endif

    /** @brief Save power by reducing update rates */
    void enableLowPowerMode(bool enabled) { _lowPowerMode = enabled; }

    void closeEvent(QCloseEvent* event);

    /** @brief Update the window name */
    void configureWindowName();

protected slots:
    /**
     * @brief Enable/Disable Status Bar
     */
    void showStatusBarCallback(bool checked);

signals:
    void initStatusChanged(const QString& message, int alignment, const QColor &color);
    /** Emitted when any value changes from any source */
    void valueChanged(const int uasId, const QString& name, const QString& unit, const QVariant& value, const quint64 msec);

    // Used for unit tests to know when the main window closes
    void mainWindowClosed(void);

#ifdef QGC_MOUSE_ENABLED_LINUX
    /** @brief Forward X11Event to catch 3DMouse inputs */
    void x11EventOccured(XEvent *event);
#endif //QGC_MOUSE_ENABLED_LINUX

public:
#ifndef __mobile__
    QGCMAVLinkLogPlayer* getLogPlayer()
    {
        return logPlayer;
    }
#endif

protected:
    void connectCommonActions();

    void loadSettings();
    void storeSettings();

    QSettings settings;

    QPointer<MAVLinkDecoder> mavlinkDecoder;
#ifndef __mobile__
    QGCMAVLinkLogPlayer* logPlayer;
#endif
#ifdef QGC_MOUSE_ENABLED_WIN
    /** @brief 3d Mouse support (WIN only) */
    Mouse3DInput* mouseInput;               ///< 3dConnexion 3dMouse SDK
    Mouse6dofInput* mouse;                  ///< Implementation for 3dMouse input
#endif // QGC_MOUSE_ENABLED_WIN

#ifdef QGC_MOUSE_ENABLED_LINUX
    /** @brief Reimplementation of X11Event to handle 3dMouse Events (magellan) */
    bool x11Event(XEvent *event);
    Mouse6dofInput* mouse;                  ///< Implementation for 3dMouse input
#endif // QGC_MOUSE_ENABLED_LINUX

    /** User interface actions **/
    QAction* connectUASAct;
    QAction* disconnectUASAct;
    QAction* startUASAct;
    QAction* returnUASAct;
    QAction* stopUASAct;
    QAction* killUASAct;


    LogCompressor* comp;
    QTimer* videoTimer;
    QTimer windowNameUpdateTimer;

private slots:
    void _closeWindow(void) { close(); }
    void _vehicleAdded(Vehicle* vehicle);

#ifndef __mobile__
    void _showDockWidgetAction(bool show);
#endif

#ifdef UNITTEST_BUILD
    void _showQmlTestWidget(void);
#endif

private:
    /// Constructor is private since all creation should be through MainWindow::_create
    MainWindow();

    void _openUrl(const QString& url, const QString& errorMessage);

#ifndef __mobile__
    QMap<QString, QGCDockWidget*>   _mapName2DockWidget;
    QMap<QString, QAction*>         _mapName2Action;
#endif

    void _storeCurrentViewState(void);
    void _loadCurrentViewState(void);

#ifndef __mobile__
    bool _createInnerDockWidget(const QString& widgetName);
    void _buildCommonWidgets(void);
    void _hideAllDockWidgets(void);
    void _showDockWidget(const QString &name, bool show);
    void _loadVisibleWidgetsSettings(void);
    void _storeVisibleWidgetsSettings(void);
#endif

    bool                    _lowPowerMode;           ///< If enabled, QGC reduces the update rates of all widgets
    bool                    _showStatusBar;
    QVBoxLayout*            _centralLayout;
    Ui::MainWindow          _ui;

    QGCQmlWidgetHolder*     _mainQmlWidgetHolder;

    bool    _forceClose;

    QString _getWindowGeometryKey();
};

#endif /* _MAINWINDOW_H_ */
