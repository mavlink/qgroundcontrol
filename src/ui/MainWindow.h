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
#include "QGCGoogleEarthView.h"
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

public slots:
//    /** @brief Store the mainwindow settings */
//    void storeSettings();

    /**
     * @brief Shows a status message on the bottom status bar
     *
     * The status message will be overwritten if a new message is posted to this function
     *
     * @param status message text
     * @param timeout how long the status should be displayed
     */
    void showStatusMessage(const QString& status, int timeout);
    /**
     * @brief Shows a status message on the bottom status bar
     *
     * The status message will be overwritten if a new message is posted to this function.
     * it will be automatically hidden after 5 seconds.
     *
     * @param status message text
     */
    void showStatusMessage(const QString& status);
    void addLink();
    void addLink(LinkInterface* link);
    void configure();
    void UASCreated(UASInterface* uas);
    void startVideoCapture();
    void stopVideoCapture();
    void saveScreen();

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

    void closeEvent(QCloseEvent* event);

    /*
    ==========================================================
                  Potentially Deprecated
    ==========================================================
    */

    void loadWidgets();

    /** @brief Load data view, allowing to plot flight data */
    void loadDataView();
    /** @brief Load data view, allowing to plot flight data */
    void loadDataView(QString fileName);

    /** @brief Load 3D map view */
    void load3DMapView();

    /** @brief Load 3D Google Earth view */
    void loadGoogleEarthView();

    /** @brief Load 3D view */
    void load3DView();

    /**
     * @brief Shows a Docked Widget based on the action sender
     *
     * This slot is written to be used in conjunction with the addToToolsMenu function
     * It shows the QDockedWidget based on the action sender
     *
     */
    void showToolWidget();

    /**
     * @brief Shows a Widget from the center stack based on the action sender
     *
     * This slot is written to be used in conjunction with the addToCentralWidgetsMenu function
     * It shows the Widget based on the action sender
     *
     */
    void showCentralWidget();

    /** @brief Updates a QDockWidget's checked status based on its visibility */
    void updateVisibilitySettings (bool vis);

    /** @brief Updates a QDockWidget's location */
    void updateLocationSettings (Qt::DockWidgetArea location);

protected:

    MainWindow(QWidget *parent = 0);

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


    /** @brief Keeps track of the current view */
    VIEW_SECTIONS currentView;
    bool aboutToCloseFlag;

    QStatusBar* statusBar;
    QStatusBar* createStatusBar();


    void clearView();

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

private:
    Ui::MainWindow ui;

    QString buildMenuKey (SETTINGS_SECTIONS section , TOOLS_WIDGET_NAMES tool, VIEW_SECTIONS view);

};

#endif /* _MAINWINDOW_H_ */
