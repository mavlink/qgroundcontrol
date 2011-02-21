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

#include "ui_MainWindow.h"
#include "LinkManager.h"
#include "LinkInterface.h"
#include "UASInterface.h"
#include "UASManager.h"
#include "UASControlWidget.h"
#include "Linecharts.h"
#include "UASInfoWidget.h"
#include "WaypointList.h"
#include "CameraView.h"
#include "UASListWidget.h"
#include "MAVLinkProtocol.h"
#include "MAVLinkSimulationLink.h"
#include "AS4Protocol.h"
#include "ObjectDetectionView.h"
#include "HUD.h"
#include "JoystickWidget.h"
#include "input/JoystickInput.h"
#include "DebugConsole.h"
#include "MapWidget.h"
#include "ParameterInterface.h"
#include "XMLCommProtocolWidget.h"
#include "HDDisplay.h"
#include "WatchdogControl.h"
#include "HSIDisplay.h"
#include "QGCDataPlot2D.h"
#include "QGCRemoteControlView.h"
#if (defined Q_OS_MAC) | (defined _MSC_VER)
#include "QGCGoogleEarthView.h"
#endif
//#include "QMap3DWidget.h"
#include "SlugsDataSensorView.h"
#include "LogCompressor.h"

#include "SlugsPIDControl.h"

#include "SlugsHilSim.h"

#include "SlugsVideoCamControl.h"


/**
 * @brief Main Application Window
 *
 **/
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    static MainWindow* instance();
    ~MainWindow();

    enum QGC_MAINWINDOW_STYLE
    {
        QGC_MAINWINDOW_STYLE_NATIVE,
        QGC_MAINWINDOW_STYLE_INDOOR,
        QGC_MAINWINDOW_STYLE_OUTDOOR
    };

    /** @brief Get current visual style */
    int getStyle() { return currentStyle; }
    /** @brief Get auto link reconnect setting */
    bool autoReconnectEnabled() { return autoReconnect; }

public slots:
//    /** @brief Store the mainwindow settings */
//    void storeSettings();

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
    void addLink();
    void addLink(LinkInterface* link);
    void configure();
    /** @brief Set the currently controlled UAS */
    void setActiveUAS(UASInterface* uas);

    /** @brief Add a new UAS */
    void UASCreated(UASInterface* uas);
    /** @brief Update system specs of a UAS */
    void UASSpecsChanged(int uas);
    void startVideoCapture();
    void stopVideoCapture();
    void saveScreen();

    /** @brief Load default view when no MAV is connected */
    void loadUnconnectedView();
    /** @brief Load view for pilot */
    void loadPilotView();
    /** @brief Load view for engineer */
    void loadEngineerView();
    /** @brief Load view for operator */
    void loadOperatorView();
    /** @brief Load MAVLink XML generator view */
    void loadMAVLinkView();

    /** @brief Show the online help for users */
    void showHelp();
    /** @brief Show the authors / credits */
    void showCredits();
    /** @brief Show the project roadmap */
    void showRoadMap();

    /** @brief Shows the widgets based on configuration and current view and autopilot */
    void presentView();

    /** @brief Reload the CSS style sheet */
    void reloadStylesheet();
    /** @brief Let the user select the CSS style sheet */
    void selectStylesheet();
    /** @brief Automatically reconnect last link */
    void enableAutoReconnect(bool enabled);
    /** @brief Switch to native application style */
    void loadNativeStyle();
    /** @brief Switch to indoor mission style */
    void loadIndoorStyle();
    /** @brief Switch to outdoor mission style */
    void loadOutdoorStyle();
    /** @brief Load a specific style */
    void loadStyle(QGC_MAINWINDOW_STYLE style);

    /** @brief Add a custom tool widget */
    void createCustomWidget();

    void closeEvent(QCloseEvent* event);

    /** @brief Load data view, allowing to plot flight data */
    void loadDataView(QString fileName);

    /**
     * @brief Shows a Docked Widget based on the action sender
     *
     * This slot is written to be used in conjunction with the addToToolsMenu function
     * It shows the QDockedWidget based on the action sender
     *
     */
    void showToolWidget(bool visible);

    /**
     * @brief Shows a Widget from the center stack based on the action sender
     *
     * This slot is written to be used in conjunction with the addToCentralWidgetsMenu function
     * It shows the Widget based on the action sender
     *
     */
    void showCentralWidget();

    /** @brief Change actively a QDockWidgets visibility by an action */
    void showDockWidget(bool vis);
    /** @brief Updates a QDockWidget's checked status based on its visibility */
    void updateVisibilitySettings(bool vis);

    /** @brief Updates a QDockWidget's location */
    void updateLocationSettings (Qt::DockWidgetArea location);

protected:

    MainWindow(QWidget *parent = 0);

    /** @brief Set default window settings for the current autopilot type */
    void setDefaultSettingsForAp();

    // These defines are used to save the settings when selecting with
    // which widgets populate the views
    // FIXME: DO NOT PUT CUSTOM VALUES IN THIS ENUM since it is iterated over
    // this will be fixed in a future release.
    typedef enum _TOOLS_WIDGET_NAMES {
      MENU_UAS_CONTROL,
      MENU_UAS_INFO,
      MENU_CAMERA,
      MENU_UAS_LIST,
      MENU_WAYPOINTS,
      MENU_STATUS,
      MENU_DETECTION,
      MENU_DEBUG_CONSOLE,
      MENU_PARAMETERS,
      MENU_HDD_1,
      MENU_HDD_2,
      MENU_WATCHDOG,
      MENU_HUD,
      MENU_HSI,
      MENU_RC_VIEW,
      MENU_SLUGS_DATA,
      MENU_SLUGS_PID,
      MENU_SLUGS_HIL,
      MENU_SLUGS_CAMERA,
      MENU_MAVLINK_LOG_PLAYER,
      MENU_VIDEO_STREAM_1,
      MENU_VIDEO_STREAM_2,
      CENTRAL_SEPARATOR= 255, // do not change
      CENTRAL_LINECHART,
      CENTRAL_PROTOCOL,
      CENTRAL_MAP,
      CENTRAL_3D_LOCAL,
      CENTRAL_3D_MAP,
      CENTRAL_OSGEARTH,
      CENTRAL_GOOGLE_EARTH,
      CENTRAL_HUD,
      CENTRAL_DATA_PLOT,

    }TOOLS_WIDGET_NAMES;

    typedef enum _SETTINGS_SECTIONS
    {
      SECTION_MENU,
      SUB_SECTION_CHECKED,
      SUB_SECTION_LOCATION,
    } SETTINGS_SECTIONS;

    typedef enum _VIEW_SECTIONS
    {
      VIEW_ENGINEER,
      VIEW_OPERATOR,
      VIEW_PILOT,
      VIEW_MAVLINK,
      VIEW_UNCONNECTED,    ///< View in unconnected mode, when no UAS is available
    } VIEW_SECTIONS;


    QHash<int, QAction*> toolsMenuActions; // Holds ptr to the Menu Actions
    QHash<int, QWidget*> dockWidgets;  // Holds ptr to the Actual Dock widget
    QHash<int, Qt::DockWidgetArea> dockWidgetLocations; // Holds the location

    /**
     * @brief Adds an already instantiated QDockedWidget to the Tools Menu
     *
     * This function does all the hosekeeping to have a QDockedWidget added to the
     * tools menu and connects the QMenuAction to a slot that shows the widget and
     * checks/unchecks the tools menu item
     *
     * @param widget    The QDockedWidget being added
     * @param title     The entry that will appear in the Menu and in the QDockedWidget title bar
     * @param slotName  The slot to which the triggered() signal of the menu action will be connected.
     * @param tool      The ENUM defined in MainWindow.h that is associated to the widget
     * @param location  The default location for the QDockedWidget in case there is no previous key in the settings
     */
    void addToToolsMenu (QWidget* widget, const QString title, const char * slotName, TOOLS_WIDGET_NAMES tool, Qt::DockWidgetArea location=Qt::RightDockWidgetArea);

    /**
     * @brief Determines if a QDockWidget needs to be show and if so, shows it
     *
     *  Based on the the autopilot and the current view it queries the settings and shows the
     *  widget if necessary
     *
     * @param widget    The QDockWidget requested to be shown
     * @param view      The view for which the QDockWidget is requested
     */
    void showTheWidget (TOOLS_WIDGET_NAMES widget, VIEW_SECTIONS view = VIEW_MAVLINK);

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
     * @param slotName      The slot to which the triggered() signal of the menu action will be connected.
     * @param centralWidget The ENUM defined in MainWindow.h that is associated to the widget
     */
    void addToCentralWidgetsMenu ( QWidget* widget, const QString title,const char * slotName, TOOLS_WIDGET_NAMES centralWidget);

    /**
     * @brief Determines if a QWidget needs to be show and if so, shows it
     *
     *  Based on the the autopilot and the current view it queries the settings and shows the
     *  widget if necessary
     *
     * @param centralWidget    The QWidget requested to be shown
     * @param view             The view for which the QWidget is requested
     */
    void showTheCentralWidget (TOOLS_WIDGET_NAMES centralWidget, VIEW_SECTIONS view);

    /** @brief Catch window resize events */
    void resizeEvent(QResizeEvent * event);

    /** @brief Keeps track of the current view */
    VIEW_SECTIONS currentView;
    bool aboutToCloseFlag;
    bool changingViewsFlag;

    void clearView();

    void buildCustomWidget();
    void buildCommonWidgets();
    void buildPxWidgets();
    void buildSlugsWidgets();

    void connectCommonWidgets();
    void connectPxWidgets();
    void connectSlugsWidgets();

    void arrangeCommonCenterStack();
    void arrangePxCenterStack();
    void arrangeSlugsCenterStack();

    void connectCommonActions();
    void connectPxActions();
    void connectSlugsActions();


    void configureWindowName();
    void loadSettings();
    void storeSettings();

    // TODO Should be moved elsewhere, as the protocol does not belong to the UI
    MAVLinkProtocol* mavlink;
    AS4Protocol* as4link;

    MAVLinkSimulationLink* simulationLink;
    LinkInterface* udpLink;

    QSettings settings;
    QStackedWidget *centerStack;

    // Center widgets
    QPointer<Linecharts> linechartWidget;

    QPointer<HUD> hudWidget;

    QPointer<MapWidget> mapWidget;
    QPointer<XMLCommProtocolWidget> protocolWidget;
    QPointer<QGCDataPlot2D> dataplotWidget;
    #ifdef QGC_OSG_ENABLED
    QPointer<QWidget> _3DWidget;
    #endif
    #ifdef QGC_OSGEARTH_ENABLED
    QPointer<QWidget> _3DMapWidget;
    #endif
#if (defined _MSC_VER) || (defined Q_OS_MAC)
    QPointer<QGCGoogleEarthView> gEarthWidget;
#endif
    // Dock widgets
    QPointer<QDockWidget> controlDockWidget;
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
    QPointer<QDockWidget> logPlayerDockWidget;

    QPointer<QDockWidget> hsiDockWidget;
    QPointer<QDockWidget> rcViewDockWidget;
    QPointer<QDockWidget> hudDockWidget;
    QPointer<QDockWidget> slugsDataWidget;
    QPointer<QDockWidget> slugsPIDControlWidget;
    QPointer<QDockWidget> slugsHilSimWidget;
    QPointer<QDockWidget> slugsCamControlWidget;


    // Popup widgets
    JoystickWidget* joystickWidget;

    JoystickInput* joystick;

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
    QString styleFileName;
    bool autoReconnect;
    QGC_MAINWINDOW_STYLE currentStyle;

private:
    Ui::MainWindow ui;

    QString buildMenuKey (SETTINGS_SECTIONS section , TOOLS_WIDGET_NAMES tool, VIEW_SECTIONS view);
    QString getWindowStateKey();
    QString getWindowGeometryKey();

};

#endif /* _MAINWINDOW_H_ */
