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

#include "ui_MainWindow.h"
#include "LinkManager.h"
#include "LinkInterface.h"
#include "UASInterface.h"
#include "UASManager.h"
#include "UASControlWidget.h"
#include "LinechartWidget.h"
#include "UASInfoWidget.h"
#include "WaypointList.h"
#include "CameraView.h"
#include "UASListWidget.h"
#include "MAVLinkProtocol.h"
#include "AS4Protocol.h"
#include "ObjectDetectionView.h"
#include "HUD.h"
#include "PFD.h"
#include "GaugePanel.h"
#include "JoystickWidget.h"
#include "input/JoystickInput.h"
#include "DebugConsole.h"
#include "MapWidget.h"
#include "ParameterInterface.h"
#include "XMLCommProtocolWidget.h"
#include "HDDisplay.h"

#include "LogCompressor.h"


/**
 * @brief Main Application Window
 *
 **/
class MGMainWindow : public QMainWindow {
    Q_OBJECT

public:
    MGMainWindow(QWidget *parent = 0);
    ~MGMainWindow();

    UASControlWidget* control;
    LinechartWidget* linechart;
    UASInfoWidget* info;
    CameraView* camera;
    UASListWidget* list;
    WaypointList* waypoints;
    ObjectDetectionView* detection;
    HUD* hud;
    PFD* pfd;
    GaugePanel* gaugePanel;

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
    void setLastAction(QString status);
    void setLinkStatus(QString status);
    void addLink();
    void configure();
    void UASCreated(UASInterface* uas);

    /** @brief Load view for pilot */
    void loadPilotView();
    /** @brief Load view for engineer */
    void loadEngineerView();
    /** @brief Load view for operator */
    void loadOperatorView();
    /** @brief Load view for general settings */
    void loadSettingsView();

    void reloadStylesheet();

    void runTests();

protected:
    QStatusBar* statusBar;
    QStatusBar* createStatusBar();
    void loadWidgets();
    void connectActions();
    void clearView();

    // TODO Should be moved elsewhere, as the protocol does not belong to the UI
    MAVLinkProtocol* mavlink;
    AS4Protocol* as4link;

    LinkInterface* simulationLink;
    LinkInterface* udpLink;

    QDockWidget* controlDock;
    QStackedWidget* centerStack;

    DebugConsole* debugConsole;
    MapWidget* map;
    ParameterInterface* parameters;
    XMLCommProtocolWidget* protocol;
    HDDisplay* headDown1;
    HDDisplay* headDown2;


    LogCompressor* comp;

private:
    Ui::MGMainWindow ui;
};

#endif /* _MAINWINDOW_H_ */
