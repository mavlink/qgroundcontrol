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
#include "QGCMAVLinkLogPlayer.h"
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

public slots:
    /** @brief Show the application settings */
    void showSettings();

    void manageLinks();

    /** @brief Save power by reducing update rates */
    void enableLowPowerMode(bool enabled) { _lowPowerMode = enabled; }

    void closeEvent(QCloseEvent* event);

    /** @brief Update the window name */
    void configureWindowName();

protected slots:
    /**
     * @brief Unchecks the normalActionItem.
     * Used as a triggered() callback by the fullScreenAction to make sure only one of it or the
     * normalAction are checked at a time, as they're mutually exclusive.
     */
    void fullScreenActionItemCallback(bool);
    /**
     * @brief Unchecks the fullScreenActionItem.
     * Used as a triggered() callback by the normalAction to make sure only one of it or the
     * fullScreenAction are checked at a time, as they're mutually exclusive.
     */
    void normalActionItemCallback(bool);
    /**
     * @brief Enable/Disable Status Bar
     */
    void showStatusBarCallback(bool checked);

    /**
     * @brief Disable the other QActions that trigger view mode changes
     *
     * When a user hits Ctrl+1, Ctrl+2, Ctrl+3  - only one view is set to active
     * (and in the QML file for the MainWindow the others are set to have
     * visibility = false), but on the Menu all of them would be selected making
     * this incoherent.
     */
    void handleActiveViewActionState(bool triggered);
signals:
    // Signals the Qml to show the specified view
    void showFlyView(void);
    void showPlanView(void);
    void showSetupView(void);

    void showToolbarMessage(const QString& message);

    // These are used for unit testing
    void showSetupFirmware(void);
    void showSetupParameters(void);
    void showSetupSummary(void);
    void showSetupVehicleComponent(VehicleComponent* vehicleComponent);

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
    QGCMAVLinkLogPlayer* getLogPlayer()
    {
        return logPlayer;
    }

protected:
    void connectCommonActions();

    void loadSettings();
    void storeSettings();

    QSettings settings;

    QPointer<MAVLinkDecoder> mavlinkDecoder;
    QGCMAVLinkLogPlayer* logPlayer;

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

    // Center widgets
    QPointer<QWidget> _planView;
    QPointer<QWidget> _flightView;
    QPointer<QWidget> _setupView;
    QPointer<QWidget> _missionEditorView;

#ifndef __mobile__
    QMap<QString, QGCDockWidget*>   _mapName2DockWidget;
    QMap<QString, QAction*>         _mapName2Action;
#endif

    void _storeCurrentViewState(void);
    void _loadCurrentViewState(void);

#ifndef __mobile__
    void _createInnerDockWidget(const QString& widgetName);
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

    QString _getWindowGeometryKey();
};

#endif /* _MAINWINDOW_H_ */
