/*=====================================================================

PIXHAWK Micro Air Vehicle Flying Robotics Toolkit

(c) 2009, 2010 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>

This file is part of the PIXHAWK project

    PIXHAWK is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PIXHAWK is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with PIXHAWK. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Main application window
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
#include "PFD.h"
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

#include "LogCompressor.h"


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
    /** @brief Load view for general settings */
    void loadSettingsView();
    /** @brief Load view with all widgets */
    void loadAllView();
    /** @brief Load MAVLink XML generator view */
    void loadMAVLinkView();
    /** @brief Load data view, allowing to plot flight data */
    void loadDataView();

    /** @brief Show the online help for users */
    void showHelp();
    /** @brief Show the authors / credits */
    void showCredits();
    /** @brief Show the project roadmap */
    void showRoadMap();

    // Fixme find a nicer solution that scales to more AP types
    void loadSlugsView();
    void loadPixhawkView();

    /** @brief Reload the CSS style sheet */
    void reloadStylesheet();
protected:
    QStatusBar* statusBar;
    QStatusBar* createStatusBar();
    void loadWidgets();
    void connectActions();
    void clearView();
    void buildWidgets();
    void connectWidgets();
    void arrangeCenterStack();
    void configureWindowName();

    // TODO Should be moved elsewhere, as the protocol does not belong to the UI
    MAVLinkProtocol* mavlink;
    AS4Protocol* as4link;

    MAVLinkSimulationLink* simulationLink;
    LinkInterface* udpLink;

    QSettings settings;
    UASControlWidget* control;
    Linecharts* linechart;
    UASInfoWidget* info;
    CameraView* camera;
    UASListWidget* list;
    WaypointList* waypoints;
    ObjectDetectionView* detection;
    HUD* hud;
    DebugConsole* debugConsole;
    MapWidget* map;
    ParameterInterface* parameters;
    XMLCommProtocolWidget* protocol;
    HDDisplay* headDown1;
    HDDisplay* headDown2;
    WatchdogControl* watchdogControl;
    HSIDisplay* hsi;
    QGCDataPlot2D* dataplot;

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

    QDockWidget* controlDock;
    QStackedWidget* centerStack;

    LogCompressor* comp;
    QString screenFileName;
    QTimer* videoTimer;

private:
    Ui::MainWindow ui;

};

#endif /* _MAINWINDOW_H_ */
