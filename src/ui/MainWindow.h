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
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
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
    /** @brief Load 3D view */
    void load3DView();
    /** @brief Load 3D Google Earth view */
    void loadGoogleEarthView();
    /** @brief Load 3D map view */
    void load3DMapView();
    /** @brief Load view with all widgets */
    void loadAllView();
    /** @brief Load MAVLink XML generator view */
    void loadMAVLinkView();
    /** @brief Load data view, allowing to plot flight data */
    void loadDataView();
    /** @brief Load data view, allowing to plot flight data */
    void loadDataView(QString fileName);
    /** @brief Show the online help for users */
    void showHelp();
    /** @brief Show the authors / credits */
    void showCredits();
    /** @brief Show the project roadmap */
    void showRoadMap();

    // Fixme find a nicer solution that scales to more AP types
    void loadSlugsView();

    void loadPixhawkEngineerView();
    void loadSlugsEngineerView();

    /** @brief Reload the CSS style sheet */
    void reloadStylesheet();


    void showToolWidget();
    void updateVisibilitySettings (bool vis);
    void updateLocationSettings (Qt::DockWidgetArea location);
protected:

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
      CENTRAL_LINECHART = 255, // Separation from dockwidgets and central widgets
      CENTRAL_PROTOCOL,
      CENTRAL_MAP,
      CENTRAL_3D_LOCAL,
      CENTRAL_3D_MAP,
      CENTRAL_GOOGLE_EARTH,
      CENTRAL_HUD,
      CENTRAL_DATA_PLOT,
    }TOOLS_WIDGET_NAMES;

    typedef enum _SETTINGS_SECTIONS {
      SECTION_MENU,
      VIEW_ENGINEER,
      VIEW_OPERATOR,
      VIEW_CALIBRATION,
      VIEW_MAVLINK,
      SUB_SECTION_CHECKED,
      SUB_SECTION_LOCATION,
    } SETTINGS_SECTIONS;


    QHash<int, QAction*> toolsMenuActions; // Holds ptr to the Menu Actions
    QHash<int, QWidget*> dockWidgets;  // Holds ptr to the Actual Dock widget
    QHash<int, Qt::DockWidgetArea> dockWidgetLocations; // Holds the location


    void addToToolsMenu (QWidget* widget, const QString title, const char * slotName, TOOLS_WIDGET_NAMES tool, Qt::DockWidgetArea location);
    void showTheWidget (TOOLS_WIDGET_NAMES widget);

    int currentView;
    int aboutToQuit;
    //QHash<int, QString> settingsSections;

    QStatusBar* statusBar;
    QStatusBar* createStatusBar();
    void loadWidgets();

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
    QPointer<QGCGoogleEarthView> gEarthWidget;
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

    QString buildMenuKey (SETTINGS_SECTIONS section , TOOLS_WIDGET_NAMES tool);

};

#endif /* _MAINWINDOW_H_ */
