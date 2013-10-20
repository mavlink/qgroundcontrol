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
#include <QtGui/QMainWindow>
#include <QStatusBar>
#include <QStackedWidget>
#include <QSettings>
#include <qlist.h>

#include "ui_MainWindow.h"
#include "LinkManager.h"
#include "LinkInterface.h"
#include "UASInterface.h"
#include "UASManager.h"
#include "UASControlWidget.h"
#include "UASInfoWidget.h"
#include "WaypointList.h"
#include "CameraView.h"
#include "UASListWidget.h"
#include "MAVLinkProtocol.h"
#include "MAVLinkSimulationLink.h"
#include "ObjectDetectionView.h"
#include "submainwindow.h"
#include "input/JoystickInput.h"
#if (defined MOUSE_ENABLED_WIN) | (defined MOUSE_ENABLED_LINUX)
#include "Mouse6dofInput.h"
#endif // MOUSE_ENABLED_WIN
#include "DebugConsole.h"
#include "ParameterInterface.h"
#include "XMLCommProtocolWidget.h"
#include "HDDisplay.h"
#include "WatchdogControl.h"
#include "HSIDisplay.h"
#include "QGCRemoteControlView.h"
#include "opmapcontrol.h"
#if (defined Q_OS_MAC) | (defined _MSC_VER)
#include "QGCGoogleEarthView.h"
#endif
#include "QGCToolBar.h"
#include "SlugsDataSensorView.h"
#include "LogCompressor.h"

#include "SlugsHilSim.h"

#include "SlugsPadCameraControl.h"
#include "UASControlParameters.h"
#include "QGCMAVLinkInspector.h"
#include "QGCMAVLinkLogPlayer.h"
#include "QGCVehicleConfig.h"
#include "MAVLinkDecoder.h"

class QGCMapTool;
class QGCMAVLinkMessageSender;
class QGCFirmwareUpdate;
class QSplashScreen;
class QGCStatusBar;
class Linecharts;
class QGCDataPlot2D;
class JoystickWidget;
class MenuActionHelper;

/**
 * @brief Main Application Window
 *
 **/
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    enum CUSTOM_MODE {
        CUSTOM_MODE_UNCHANGED = 0,
        CUSTOM_MODE_NONE,
        CUSTOM_MODE_PX4,
        CUSTOM_MODE_APM,
        CUSTOM_MODE_WIFI
    };

    /**
     * A static function for obtaining the sole instance of the MainWindow. The screen
     * argument is only important on the FIRST call to this function. The provided splash
     * screen is updated with some status messages that are emitted during init(). This
     * function cannot be used within the MainWindow constructor!
     */
    static MainWindow* instance(QSplashScreen* screen = 0);
    static MainWindow* instance_mode(QSplashScreen* screen = 0, enum MainWindow::CUSTOM_MODE mode = MainWindow::CUSTOM_MODE_NONE);

    /**
     * Initializes the MainWindow. Some variables are initialized and the widget is hidden.
     * Initialization of the MainWindow class really occurs in init(), which loads the UI
     * and does everything important. The constructor is split in two like this so that
     * the instance() is available for all classes.
     */
    MainWindow(QWidget *parent = NULL);
    ~MainWindow();

    /**
     * This function actually performs the non-trivial initialization of the MainWindow
     * class. This is separate from the constructor because instance() won't work within
     * code executed in the MainWindow constructor.
     */
    void init();

    enum QGC_MAINWINDOW_STYLE
    {
        QGC_MAINWINDOW_STYLE_DARK,
        QGC_MAINWINDOW_STYLE_LIGHT
    };

    // Declare default dark and light stylesheets. These should be file-resource
    // paths.
    static const QString defaultDarkStyle;
    static const QString defaultLightStyle;

    /** @brief Get current visual style */
    QGC_MAINWINDOW_STYLE getStyle() const
    {
        return currentStyle;
    }

    /** @brief Get current light visual stylesheet */
    QString getLightStyleSheet() const
    {
        return lightStyleFileName;
    }

    /** @brief Get current dark visual stylesheet */
    QString getDarkStyleSheet() const
    {
        return darkStyleFileName;
    }
    /** @brief Get auto link reconnect setting */
    bool autoReconnectEnabled() const
    {
        return autoReconnect;
    }

    /** @brief Get title bar mode setting */
    bool dockWidgetTitleBarsEnabled() const;

    /** @brief Get low power mode setting */
    bool lowPowerModeEnabled() const
    {
        return lowPowerMode;
    }

    void setCustomMode(MainWindow::CUSTOM_MODE mode)
    {
        if (mode != CUSTOM_MODE_UNCHANGED)
        {
            customMode = mode;
        }
    }

    MainWindow::CUSTOM_MODE getCustomMode() const
    {
        return customMode;
    }

    QList<QAction*> listLinkMenuActions();

public slots:
    /** @brief Shows a status message on the bottom status bar */
    void showStatusMessage(const QString& status, int timeout);
    /** @brief Shows a status message on the bottom status bar */
    void showStatusMessage(const QString& status);
    /** @brief Shows a critical message as popup or as widget */
    void showCriticalMessage(const QString& title, const QString& message);
    /** @brief Shows an info message as popup or as widget */
    void showInfoMessage(const QString& title, const QString& message);

    /** @brief Show the application settings */
    void showSettings();
    /** @brief Add a communication link */
    LinkInterface* addLink();
    void addLink(LinkInterface* link);
    bool configLink(LinkInterface *link);
    void configure();
    /** @brief Simulate a link */
    void simulateLink(bool simulate);
    /** @brief Set the currently controlled UAS */
    void setActiveUAS(UASInterface* uas);

    /** @brief Add a new UAS */
    void UASCreated(UASInterface* uas);
    /** Delete an UAS */
    void UASDeleted(UASInterface* uas);
    /** @brief Update system specs of a UAS */
    void UASSpecsChanged(int uas);
    void startVideoCapture();
    void stopVideoCapture();
    void saveScreen();

    /** @brief Sets advanced mode, allowing for editing of tool widget locations */
    void setAdvancedMode(bool isAdvancedMode);
    /** @brief Load configuration views */
    void loadHardwareConfigView();
    void loadSoftwareConfigView();
    /** @brief Load default view when no MAV is connected */
    void loadUnconnectedView();
    /** @brief Load view for pilot */
    void loadPilotView();
    /** @brief Load view for simulation */
    void loadSimulationView();
    /** @brief Load view for engineer */
    void loadEngineerView();
    /** @brief Load view for operator */
    void loadOperatorView();
    /** @brief Load MAVLink XML generator view */
    void loadMAVLinkView();
    /** @brief Load Terminal Console views */
    void loadTerminalView();

    /** @brief Show the online help for users */
    void showHelp();
    /** @brief Show the authors / credits */
    void showCredits();
    /** @brief Show the project roadmap */
    void showRoadMap();

    /** @breif Enable title bars on dock widgets when no in advanced mode */
    void enableDockWidgetTitleBars(bool enabled);
    /** @brief Automatically reconnect last link */
    void enableAutoReconnect(bool enabled);

    /** @brief Save power by reducing update rates */
    void enableLowPowerMode(bool enabled) { lowPowerMode = enabled; }
    /** @brief Load a specific style.
      * If it's a custom style, load the file indicated by the cssFile path.
      */
    bool loadStyle(QGC_MAINWINDOW_STYLE style, QString cssFile);

    /** @brief Add a custom tool widget */
    void createCustomWidget();

    /** @brief Load a custom tool widget from a file chosen by user (QFileDialog) */
    void loadCustomWidget();

    /** @brief Load a custom tool widget from a file */
    void loadCustomWidget(const QString& fileName, bool singleinstance=false);
    void loadCustomWidget(const QString& fileName, int view);

    /** @brief Load custom widgets from default file */
    void loadCustomWidgetsFromDefaults(const QString& systemType, const QString& autopilotType);

    /** @brief Loads and shows the HIL Configuration Widget for the given UAS*/
    void showHILConfigurationWidget(UASInterface *uas);

    void closeEvent(QCloseEvent* event);

    /** @brief Load data view, allowing to plot flight data */
//    void loadDataView(QString fileName);

    /**
     * @brief Shows a Widget from the center stack based on the action sender
     *
     * This slot is written to be used in conjunction with the addCentralWidget() function
     * It shows the Widget based on the action sender
     *
     */
    void showCentralWidget();

    /** @brief Update the window name */
    void configureWindowName();

    void commsWidgetDestroyed(QObject *obj);

protected slots:
    void showDockWidget(const QString &name, bool show);

signals:
    void styleChanged(MainWindow::QGC_MAINWINDOW_STYLE newTheme);
    void initStatusChanged(const QString& message, int alignment, const QColor &color);
#ifdef MOUSE_ENABLED_LINUX
    /** @brief Forward X11Event to catch 3DMouse inputs */
    void x11EventOccured(XEvent *event);
#endif //MOUSE_ENABLED_LINUX

public:
    QGCMAVLinkLogPlayer* getLogPlayer()
    {
        return logPlayer;
    }

    MAVLinkProtocol* getMAVLink()
    {
        return mavlink;
    }

protected:

    typedef enum _VIEW_SECTIONS
    {
        VIEW_ENGINEER,
        VIEW_MISSION,
        VIEW_FLIGHT,
        VIEW_SIMULATION,
        VIEW_MAVLINK,
        VIEW_FIRMWAREUPDATE,
        VIEW_HARDWARE_CONFIG,
        VIEW_SOFTWARE_CONFIG,
        VIEW_TERMINAL,
        VIEW_3DWIDGET,
        VIEW_GOOGLEEARTH,
        VIEW_UNCONNECTED,    ///< View in unconnected mode, when no UAS is available
        VIEW_FULL            ///< All widgets shown at once
    } VIEW_SECTIONS;

    /**
     * @brief Adds an already instantiated QDockedWidget to the Tools Menu
     *
     * This function does all the hosekeeping to have a QDockedWidget added to the
     * tools menu and connects the QMenuAction to a slot that shows the widget and
     * checks/unchecks the tools menu item
     *
     * @param widget    The QDockWidget being added
     * @param title     The entry that will appear in the Menu and in the QDockedWidget title bar
     * @param location  The default location for the QDockedWidget in case there is no previous key in the settings
     */
    void addTool(SubMainWindow *parent,VIEW_SECTIONS view,QDockWidget* widget, const QString& title, Qt::DockWidgetArea area);
    void loadDockWidget(const QString &name);

    QDockWidget* createDockWidget(QWidget *subMainWindowParent,QWidget *child,const QString& title,const QString& objectname,VIEW_SECTIONS view,Qt::DockWidgetArea area,const QSize& minSize = QSize());
    /**
     * @brief Adds an already instantiated QWidget to the center stack
     *
     * This function does all the hosekeeping to have a QWidget added to the tools menu
     * tools menu and connects the QMenuAction to a slot that shows the widget and
     * checks/unchecks the tools menu item. This is used for all the central widgets (those in
     * the center stack.
     *
     * @param widget        The QWidget being added
     * @param title         The entry that will appear in the Menu
     */
    void addToCentralStackedWidget(QWidget* widget, VIEW_SECTIONS viewSection, const QString& title);

    /** @brief Catch window resize events */
    void resizeEvent(QResizeEvent * event);

    /** @brief Keeps track of the current view */
    VIEW_SECTIONS currentView;
    QGC_MAINWINDOW_STYLE currentStyle;
    bool aboutToCloseFlag;
    bool changingViewsFlag;

    void storeViewState();
    void loadViewState();

    void buildCustomWidget();
    void buildCommonWidgets();
    void connectCommonWidgets();
    void connectCommonActions();
    void connectSenseSoarActions();

    void loadSettings();
    void storeSettings();

    // TODO Should be moved elsewhere, as the protocol does not belong to the UI
    QPointer<MAVLinkProtocol> mavlink;

    LinkInterface* udpLink;

    QSettings settings;
    QStackedWidget *centerStack;
    QActionGroup* centerStackActionGroup;

    // Center widgets
    QPointer<SubMainWindow> plannerView;
    QPointer<SubMainWindow> pilotView;
    QPointer<SubMainWindow> configView;
    QPointer<SubMainWindow> softwareConfigView;
    QPointer<SubMainWindow> mavlinkView;
    QPointer<SubMainWindow> engineeringView;
    QPointer<SubMainWindow> simView;
    QPointer<SubMainWindow> terminalView;

    // Center widgets
    QPointer<Linecharts> linechartWidget;
    //QPointer<HUD> hudWidget;
    //QPointer<QGCVehicleConfig> configWidget;
    //QPointer<QGCMapTool> mapWidget;
    //QPointer<XMLCommProtocolWidget> protocolWidget;
    //QPointer<QGCDataPlot2D> dataplotWidget;
#ifdef QGC_OSG_ENABLED
    QPointer<QWidget> q3DWidget;
#endif
#if (defined _MSC_VER) || (defined Q_OS_MAC)
    QPointer<QGCGoogleEarthView> earthWidget;
#endif
    QPointer<QGCFirmwareUpdate> firmwareUpdateWidget;

    // Dock widgets
    QPointer<QDockWidget> controlDockWidget;
    QPointer<QDockWidget> controlParameterWidget;
    QPointer<QDockWidget> infoDockWidget;
    QPointer<QDockWidget> cameraDockWidget;
    QPointer<QDockWidget> listDockWidget;
    QPointer<QDockWidget> waypointsDockWidget;
    QPointer<QDockWidget> detectionDockWidget;
    QPointer<QDockWidget> debugConsoleDockWidget;
    QPointer<QDockWidget> parametersDockWidget;
    QPointer<QDockWidget> headDown1DockWidget;
    QPointer<QDockWidget> headDown2DockWidget;
    QPointer<QDockWidget> watchdogControlDockWidget;

    QPointer<QDockWidget> headUpDockWidget;
    QPointer<QDockWidget> video1DockWidget;
    QPointer<QDockWidget> video2DockWidget;
    QPointer<QDockWidget> rgbd1DockWidget;
    QPointer<QDockWidget> rgbd2DockWidget;
    QPointer<QDockWidget> logPlayerDockWidget;

    QPointer<QDockWidget> hsiDockWidget;
    QPointer<QDockWidget> rcViewDockWidget;
    QPointer<QDockWidget> hudDockWidget;
    QPointer<QDockWidget> slugsDataWidget;
    QPointer<QDockWidget> slugsHilSimWidget;
    QPointer<QDockWidget> slugsCamControlWidget;

    QPointer<QGCToolBar> toolBar;
    QPointer<QGCStatusBar> customStatusBar;

    QPointer<DebugConsole> debugConsole;

    QPointer<QDockWidget> mavlinkInspectorWidget;
    QPointer<MAVLinkDecoder> mavlinkDecoder;
    QPointer<QDockWidget> mavlinkSenderWidget;
    QGCMAVLinkLogPlayer* logPlayer;
    QMap<int, QDockWidget*> hilDocks;

    // Popup widgets
    JoystickWidget* joystickWidget;

    JoystickInput* joystick;

#ifdef MOUSE_ENABLED_WIN
    /** @brief 3d Mouse support (WIN only) */
    Mouse3DInput* mouseInput;               ///< 3dConnexion 3dMouse SDK
    Mouse6dofInput* mouse;                  ///< Implementation for 3dMouse input
#endif // MOUSE_ENABLED_WIN

#ifdef MOUSE_ENABLED_LINUX
    /** @brief Reimplementation of X11Event to handle 3dMouse Events (magellan) */
    bool x11Event(XEvent *event);
    Mouse6dofInput* mouse;                  ///< Implementation for 3dMouse input
#endif // MOUSE_ENABLED_LINUX

    /** User interface actions **/
    QAction* connectUASAct;
    QAction* disconnectUASAct;
    QAction* startUASAct;
    QAction* returnUASAct;
    QAction* stopUASAct;
    QAction* killUASAct;
    QAction* simulateUASAct;


    LogCompressor* comp;
    QString screenFileName;
    QTimer* videoTimer;
    QString darkStyleFileName;
    QString lightStyleFileName;
    bool autoReconnect;
    MAVLinkSimulationLink* simulationLink;
    Qt::WindowStates windowStateVal;
    bool lowPowerMode; ///< If enabled, QGC reduces the update rates of all widgets
    QGCFlightGearLink* fgLink;
    QTimer windowNameUpdateTimer;
    CUSTOM_MODE customMode;

private:
    QList<QObject*> commsWidgetList;
    QMap<QString,QString> customWidgetNameToFilenameMap;
    MenuActionHelper *menuActionHelper;
    Ui::MainWindow ui;

    /** @brief Set the appropriate titlebar for a given dock widget.
      * Relies on the isAdvancedMode and dockWidgetTitleBarEnabled member variables.
      */
    void setDockWidgetTitleBar(QDockWidget* widget);

    QString getWindowStateKey();
    QString getWindowGeometryKey();

    friend class MenuActionHelper; //For VIEW_SECTIONS
};

#endif /* _MAINWINDOW_H_ */
