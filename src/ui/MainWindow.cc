/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009 - 2013 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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
 *   @brief Implementation of class MainWindow
 *   @author Lorenz Meier <mail@qgroundcontrol.org>
 */

#include <QSettings>
#include <QDockWidget>
#include <QNetworkInterface>
#include <QMessageBox>
#include <QDebug>
#include <QTimer>
#include <QHostInfo>
#include <QSplashScreen>
#include <QGCHilLink.h>
#include <QGCHilConfiguration.h>
#include <QGCHilFlightGearConfiguration.h>
#include "dockwidgettitlebareventfilter.h"
#include "QGC.h"
#include "MAVLinkSimulationLink.h"
#include "SerialLink.h"
#include "UDPLink.h"
#include "MAVLinkProtocol.h"
#include "CommConfigurationWindow.h"
#include "QGCWaypointListMulti.h"
#include "MainWindow.h"
#include "JoystickWidget.h"
#include "GAudioOutput.h"
#include "QGCToolWidget.h"
#include "QGCMAVLinkLogPlayer.h"
#include "QGCSettingsWidget.h"
#include "QGCMapTool.h"
#include "MAVLinkDecoder.h"
#include "QGCMAVLinkMessageSender.h"
#include "QGCRGBDView.h"
#include "QGCFirmwareUpdate.h"
#include "QGCStatusBar.h"
#include "UASQuickView.h"
#include "QGCDataPlot2D.h"
#include "Linecharts.h"
#include "UASActionsWidget.h"
#include "QGCTabbedInfoView.h"
#include "UASRawStatusView.h"
#include "PrimaryFlightDisplay.h"

#ifdef QGC_OSG_ENABLED
#include "Q3DWidgetFactory.h"
#endif

// FIXME Move
#include "PxQuadMAV.h"
#include "SlugsMAV.h"

#include "LogCompressor.h"

// Set up some constants
const QString MainWindow::defaultDarkStyle = ":files/styles/style-dark.css";
const QString MainWindow::defaultLightStyle = ":files/styles/style-light.css";

MainWindow* MainWindow::instance(QSplashScreen* screen)
{
    static MainWindow* _instance = 0;
    if (_instance == 0)
    {
        _instance = new MainWindow();
        if (screen)
        {
            connect(_instance, SIGNAL(initStatusChanged(QString,int,QColor)),
                    screen, SLOT(showMessage(QString,int,QColor)));
        }
        _instance->init();
    }
    return _instance;
}

/**
* Create new mainwindow. The constructor instantiates all parts of the user
* interface. It does NOT show the mainwindow. To display it, call the show()
* method.
*
* @see QMainWindow::show()
**/
MainWindow::MainWindow(QWidget *parent):
    QMainWindow(parent),
    currentView(VIEW_FLIGHT),
    currentStyle(QGC_MAINWINDOW_STYLE_DARK),
    aboutToCloseFlag(false),
    changingViewsFlag(false),
    centerStackActionGroup(new QActionGroup(this)),
    darkStyleFileName(defaultDarkStyle),
    lightStyleFileName(defaultLightStyle),
    autoReconnect(false),
    lowPowerMode(false),
    dockWidgetTitleBarEnabled(true),
    isAdvancedMode(false)
{
    this->setAttribute(Qt::WA_DeleteOnClose);
    hide();
}

void MainWindow::init()
{
    emit initStatusChanged(tr("Loading settings"), Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));
    loadSettings();

    emit initStatusChanged(tr("Loading style"), Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));
    qApp->setStyle("plastique");
    if (currentStyle == QGC_MAINWINDOW_STYLE_LIGHT)
    {
        loadStyle(currentStyle, lightStyleFileName);
    }
    else
    {
        loadStyle(currentStyle, darkStyleFileName);
    }

    if (settings.contains("ADVANCED_MODE"))
    {
        isAdvancedMode = settings.value("ADVANCED_MODE").toBool();
    }

    if (!settings.contains("CURRENT_VIEW"))
    {
        // Set this view as default view
        settings.setValue("CURRENT_VIEW", currentView);
    }
    else
    {
        // LOAD THE LAST VIEW
        VIEW_SECTIONS currentViewCandidate = (VIEW_SECTIONS) settings.value("CURRENT_VIEW", currentView).toInt();
        if (currentViewCandidate != VIEW_ENGINEER &&
                currentViewCandidate != VIEW_MISSION &&
                currentViewCandidate != VIEW_FLIGHT &&
                currentViewCandidate != VIEW_FULL)
        {
            currentView = currentViewCandidate;
        }
    }

    settings.sync();
    emit initStatusChanged(tr("Setting up user interface"), Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));

    // Setup user interface
    ui.setupUi(this);
    hide();

    // We only need this menu if we have more than one system
    //    ui.menuConnected_Systems->setEnabled(false);

    // Set dock options
    setDockOptions(AnimatedDocks | AllowTabbedDocks | AllowNestedDocks);

    configureWindowName();

    // Setup corners
    setCorner(Qt::BottomRightCorner, Qt::BottomDockWidgetArea);

    // Setup UI state machines
    centerStackActionGroup->setExclusive(true);

    centerStack = new QStackedWidget(this);
    setCentralWidget(centerStack);

    // Load Toolbar
    toolBar = new QGCToolBar(this);
    this->addToolBar(toolBar);

    // Add actions for average users (displayed next to each other)
    QList<QAction*> actions;
    actions << ui.actionFlightView;
    actions << ui.actionMissionView;
    actions << ui.actionConfiguration_2;
    toolBar->setPerspectiveChangeActions(actions);

    // Add actions for advanced users (displayed in dropdown under "advanced")
    QList<QAction*> advancedActions;
    advancedActions << ui.actionSimulation_View;
    advancedActions << ui.actionEngineersView;

    toolBar->setPerspectiveChangeAdvancedActions(advancedActions);
    customStatusBar = new QGCStatusBar(this);
    setStatusBar(customStatusBar);
    statusBar()->setSizeGripEnabled(true);

    emit initStatusChanged(tr("Building common widgets"), Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));

    buildCommonWidgets();
    connectCommonWidgets();

    emit initStatusChanged(tr("Building common actions"), Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));

    // Create actions
    connectCommonActions();

    // Populate link menu
    emit initStatusChanged(tr("Populating link menu"), Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));
    QList<LinkInterface*> links = LinkManager::instance()->getLinks();
    foreach(LinkInterface* link, links)
    {
        this->addLink(link);
    }

    connect(LinkManager::instance(), SIGNAL(newLink(LinkInterface*)), this, SLOT(addLink(LinkInterface*)));

    // Connect user interface devices
    emit initStatusChanged(tr("Initializing joystick interface"), Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));
    joystickWidget = 0;
    joystick = new JoystickInput();

#ifdef MOUSE_ENABLED_WIN
    emit initStatusChanged(tr("Initializing 3D mouse interface", Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));

    mouseInput = new Mouse3DInput(this);
    mouse = new Mouse6dofInput(mouseInput);
#endif //MOUSE_ENABLED_WIN

#if MOUSE_ENABLED_LINUX
    emit initStatusChanged(tr("Initializing 3D mouse interface"), Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));

    mouse = new Mouse6dofInput(this);
    connect(this, SIGNAL(x11EventOccured(XEvent*)), mouse, SLOT(handleX11Event(XEvent*)));
#endif //MOUSE_ENABLED_LINUX

    // Connect link
    if (autoReconnect)
    {
        SerialLink* link = new SerialLink();
        // Add to registry
        LinkManager::instance()->add(link);
        LinkManager::instance()->addProtocol(link, mavlink);
        link->connect();
    }

    // Set low power mode
    enableLowPowerMode(lowPowerMode);

    // Initialize window state
    windowStateVal = windowState();

    emit initStatusChanged(tr("Restoring last view state"), Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));

    // Restore the window setup
    loadViewState();

    emit initStatusChanged(tr("Restoring last window size"), Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));
    // Restore the window position and size
    if (settings.contains(getWindowGeometryKey()))
    {
        // Restore the window geometry
        restoreGeometry(settings.value(getWindowGeometryKey()).toByteArray());
        show();
    }
    else
    {
        // Adjust the size
        const int screenWidth = QApplication::desktop()->width();
        const int screenHeight = QApplication::desktop()->height();

        if (screenWidth < 1500)
        {
            resize(screenWidth, screenHeight - 80);
            show();
        }
        else
        {
            resize(screenWidth*0.67f, qMin(screenHeight, (int)(screenWidth*0.67f*0.67f)));
            show();
        }

    }

    connect(&windowNameUpdateTimer, SIGNAL(timeout()), this, SLOT(configureWindowName()));
    windowNameUpdateTimer.start(15000);
    emit initStatusChanged(tr("Done"), Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));
    show();
}

MainWindow::~MainWindow()
{
    if (mavlink)
    {
        delete mavlink;
        mavlink = NULL;
    }
    //    if (simulationLink)
    //    {
    //        simulationLink->deleteLater();
    //        simulationLink = NULL;
    //    }
    if (joystick)
    {
        joystick->shutdown();
        joystick->wait(5000);
        delete joystick;
        joystick = NULL;
    }

    // Get and delete all dockwidgets and contained
    // widgets
    QObjectList childList(this->children());

    QObjectList::iterator i;
    QDockWidget* dockWidget;
    for (i = childList.begin(); i != childList.end(); ++i)
    {
        dockWidget = dynamic_cast<QDockWidget*>(*i);
        if (dockWidget)
        {
            // Remove dock widget from main window
            // removeDockWidget(dockWidget);
            // delete dockWidget->widget();
            delete dockWidget;
            dockWidget = NULL;
        }
        else if (dynamic_cast<QWidget*>(*i))
        {
            delete dynamic_cast<QWidget*>(*i);
            *i = NULL;
        }
    }
    // Delete all UAS objects


    if (debugConsole)
    {
        delete debugConsole;
    }
    for (int i=0;i<commsWidgetList.size();i++)
    {
        commsWidgetList[i]->deleteLater();
    }
}

void MainWindow::resizeEvent(QResizeEvent * event)
{
    if (width() > 1200)
    {
        toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    }
    else
    {
        toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    }

    QMainWindow::resizeEvent(event);
}

QString MainWindow::getWindowStateKey()
{
    if (UASManager::instance()->getActiveUAS())
    {
        return QString::number(currentView)+"_windowstate_" + UASManager::instance()->getActiveUAS()->getAutopilotTypeName();
    }
    else
        return QString::number(currentView)+"_windowstate";
}

QString MainWindow::getWindowGeometryKey()
{
    //return QString::number(currentView)+"_geometry";
    return "_geometry";
}

void MainWindow::buildCustomWidget()
{
    // Create custom widgets
    QList<QGCToolWidget*> widgets = QGCToolWidget::createWidgetsFromSettings(this);

    if (widgets.size() > 0)
    {
        ui.menuTools->addSeparator();
    }

    for(int i = 0; i < widgets.size(); ++i)
    {
        // Check if this widget already has a parent, do not create it in this case
        QGCToolWidget* tool = widgets.at(i);
        QDockWidget* dock = dynamic_cast<QDockWidget*>(tool->parentWidget());
        if (!dock)
        {
            QSettings settings;
            settings.beginGroup("QGC_MAINWINDOW");

            /*QDockWidget* dock = new QDockWidget(tool->windowTitle(), this);
            dock->setObjectName(tool->objectName()+"_DOCK");
            dock->setWidget(tool);
            connect(tool, SIGNAL(destroyed()), dock, SLOT(deleteLater()));
            QAction* showAction = new QAction(widgets.at(i)->windowTitle(), this);
            showAction->setCheckable(true);
            connect(showAction, SIGNAL(triggered(bool)), dock, SLOT(setVisible(bool)));
            connect(dock, SIGNAL(visibilityChanged(bool)), showAction, SLOT(setChecked(bool)));
            widgets.at(i)->setMainMenuAction(showAction);
            ui.menuTools->addAction(showAction);*/

            // Load dock widget location (default is bottom)
            Qt::DockWidgetArea location = static_cast <Qt::DockWidgetArea>(tool->getDockWidgetArea(currentView));

            //addDockWidget(location, dock);
            //dock->hide();
            int view = settings.value(QString("TOOL_PARENT_") + tool->objectName(),-1).toInt();
            //settings.setValue(QString("TOOL_PARENT_") + "UNNAMED_TOOL_" + QString::number(ui.menuTools->actions().size()),currentView);
            settings.endGroup();

            QDockWidget* dock;

            switch (view)
            {
            case VIEW_ENGINEER:
                dock = createDockWidget(engineeringView,tool,tool->getTitle(),tool->objectName(),(VIEW_SECTIONS)view,location);
                break;
            case VIEW_FLIGHT:
                dock = createDockWidget(pilotView,tool,tool->getTitle(),tool->objectName(),(VIEW_SECTIONS)view,location);
                break;
            case VIEW_SIMULATION:
                dock = createDockWidget(simView,tool,tool->getTitle(),tool->objectName(),(VIEW_SECTIONS)view,location);
                break;
            case VIEW_MISSION:
                dock = createDockWidget(plannerView,tool,tool->getTitle(),tool->objectName(),(VIEW_SECTIONS)view,location);
                break;
            case VIEW_MAVLINK:
                dock = createDockWidget(mavlinkView,tool,tool->getTitle(),tool->objectName(),(VIEW_SECTIONS)view,location);
                break;
            default:
                dock = createDockWidget(centerStack->currentWidget(),tool,tool->getTitle(),tool->objectName(),(VIEW_SECTIONS)view,location);
                break;
            }

            // XXX temporary "fix"
            dock->hide();

            //createDockWidget(0,tool,tool->getTitle(),tool->objectName(),view,location);
        }
    }
}

void MainWindow::buildCommonWidgets()
{
    //TODO:  move protocol outside UI
    mavlink     = new MAVLinkProtocol();
    connect(mavlink, SIGNAL(protocolStatusMessage(QString,QString)), this, SLOT(showCriticalMessage(QString,QString)), Qt::QueuedConnection);
    // Add generic MAVLink decoder
    mavlinkDecoder = new MAVLinkDecoder(mavlink, this);

    // Log player
    logPlayer = new QGCMAVLinkLogPlayer(mavlink, customStatusBar);
    customStatusBar->setLogPlayer(logPlayer);

    // Center widgets
    if (!plannerView)
    {
        plannerView = new SubMainWindow(this);
        plannerView->setCentralWidget(new QGCMapTool(this));
        //mapWidget = new QGCMapTool(this);
        addCentralWidget(plannerView, "Maps");
    }

    //pilotView
    if (!pilotView)
    {
        pilotView = new SubMainWindow(this);
        pilotView->setObjectName("VIEW_FLIGHT");
        pilotView->setCentralWidget(new QGCMapTool(this));
        addCentralWidget(pilotView, "Pilot");
    }
    if (!configView)
    {
        configView = new SubMainWindow(this);
        configView->setObjectName("VIEW_CONFIGURATION");
        configView->setCentralWidget(new QGCVehicleConfig(this));
        addCentralWidget(configView,"Config");
        centralWidgetToDockWidgetsMap[VIEW_CONFIGURATION] = QMap<QString,QWidget*>();

    }
    if (!engineeringView)
    {
        engineeringView = new SubMainWindow(this);
        engineeringView->setObjectName("VIEW_ENGINEER");
        engineeringView->setCentralWidget(new QGCDataPlot2D(this));
        addCentralWidget(engineeringView,tr("Logfile Plot"));
    }
    if (!mavlinkView)
    {
        mavlinkView = new SubMainWindow(this);
        mavlinkView->setObjectName("VIEW_MAVLINK");
        mavlinkView->setCentralWidget(new XMLCommProtocolWidget(this));
        addCentralWidget(mavlinkView,tr("Mavlink Generator"));
    }
    if (!simView)
    {
        simView = new SubMainWindow(this);
        simView->setObjectName("VIEW_SIMULATOR");
        simView->setCentralWidget(new QGCMapTool(this));
        addCentralWidget(simView,tr("Simulation View"));
    }

    // Dock widgets
    QAction* tempAction = ui.menuTools->addAction(tr("Control"));
    tempAction->setCheckable(true);
    connect(tempAction,SIGNAL(triggered(bool)),this, SLOT(showTool(bool)));

    createDockWidget(simView,new UASControlWidget(this),tr("Control"),"UNMANNED_SYSTEM_CONTROL_DOCKWIDGET",VIEW_SIMULATION,Qt::LeftDockWidgetArea);

    createDockWidget(plannerView,new UASListWidget(this),tr("Unmanned Systems"),"UNMANNED_SYSTEM_LIST_DOCKWIDGET",VIEW_MISSION,Qt::LeftDockWidgetArea);
    createDockWidget(plannerView,new QGCWaypointListMulti(this),tr("Mission Plan"),"WAYPOINT_LIST_DOCKWIDGET",VIEW_MISSION,Qt::BottomDockWidgetArea);

    {
        //createDockWidget(plannerView,new QGCWaypointListMulti(this),tr("Mission Plan"),"WAYPOINT_LIST_DOCKWIDGET",VIEW_MISSION,Qt::BottomDockWidgetArea);
        QAction* tempAction = ui.menuTools->addAction(tr("Mission Plan"));
        tempAction->setCheckable(true);
        connect(tempAction,SIGNAL(triggered(bool)),this, SLOT(showTool(bool)));
        menuToDockNameMap[tempAction] = "WAYPOINT_LIST_DOCKWIDGET";
    }

    createDockWidget(simView,new QGCWaypointListMulti(this),tr("Mission Plan"),"WAYPOINT_LIST_DOCKWIDGET",VIEW_SIMULATION,Qt::BottomDockWidgetArea);
    createDockWidget(engineeringView,new QGCMAVLinkInspector(mavlink,this),tr("MAVLink Inspector"),"MAVLINK_INSPECTOR_DOCKWIDGET",VIEW_ENGINEER,Qt::RightDockWidgetArea);

    createDockWidget(engineeringView,new ParameterInterface(this),tr("Onboard Parameters"),"PARAMETER_INTERFACE_DOCKWIDGET",VIEW_ENGINEER,Qt::RightDockWidgetArea);
    createDockWidget(simView,new ParameterInterface(this),tr("Onboard Parameters"),"PARAMETER_INTERFACE_DOCKWIDGET",VIEW_SIMULATION,Qt::RightDockWidgetArea);


    {
        QAction* tempAction = ui.menuTools->addAction(tr("Status Details"));
        menuToDockNameMap[tempAction] = "UAS_STATUS_DETAILS_DOCKWIDGET";
        tempAction->setCheckable(true);
        connect(tempAction,SIGNAL(triggered(bool)),this, SLOT(showTool(bool)));
    }
    {
        if (!debugConsole)
        {
            debugConsole = new DebugConsole();
            debugConsole->setWindowTitle("Communications Console");
            debugConsole->hide();
            QAction* tempAction = ui.menuTools->addAction(tr("Communication Console"));
            //menuToDockNameMap[tempAction] = "COMMUNICATION_DEBUG_CONSOLE_DOCKWIDGET";
            tempAction->setCheckable(true);
            connect(tempAction,SIGNAL(triggered(bool)),debugConsole,SLOT(setShown(bool)));

        }
    }
    createDockWidget(simView,new HSIDisplay(this),tr("Horizontal Situation"),"HORIZONTAL_SITUATION_INDICATOR_DOCKWIDGET",VIEW_SIMULATION,Qt::BottomDockWidgetArea);

    {
        QAction* tempAction = ui.menuTools->addAction(tr("Flight Display"));
        tempAction->setCheckable(true);
        connect(tempAction,SIGNAL(triggered(bool)),this, SLOT(showTool(bool)));
        menuToDockNameMap[tempAction] = "HEAD_DOWN_DISPLAY_1_DOCKWIDGET";
    }

    {
        QAction* tempAction = ui.menuTools->addAction(tr("Actuator Status"));
        tempAction->setCheckable(true);
        connect(tempAction,SIGNAL(triggered(bool)),this, SLOT(showTool(bool)));
        menuToDockNameMap[tempAction] = "HEAD_DOWN_DISPLAY_2_DOCKWIDGET";
    }

    {
        QAction* tempAction = ui.menuTools->addAction(tr("Radio Control"));
        tempAction->setCheckable(true);
        connect(tempAction,SIGNAL(triggered(bool)),this, SLOT(showTool(bool)));
    }

    createDockWidget(engineeringView,new HUD(320,240,this),tr("Video Downlink"),"HEAD_UP_DISPLAY_DOCKWIDGET",VIEW_ENGINEER,Qt::RightDockWidgetArea,this->width()/1.5);
    createDockWidget(simView,new PrimaryFlightDisplay(320,240,this),tr("Primary Flight Display"),"PRIMARY_FLIGHT_DISPLAY_DOCKWIDGET",VIEW_SIMULATION,Qt::RightDockWidgetArea,this->width()/1.5);

    createDockWidget(pilotView,new PrimaryFlightDisplay(320,240,this),tr("Primary Flight Display"),"PRIMARY_FLIGHT_DISPLAY_DOCKWIDGET",VIEW_FLIGHT,Qt::LeftDockWidgetArea,this->width()/1.8);

    QGCTabbedInfoView *infoview = new QGCTabbedInfoView(this);
    infoview->addSource(mavlinkDecoder);
    createDockWidget(pilotView,infoview,tr("Info View"),"UAS_INFO_INFOVIEW_DOCKWIDGET",VIEW_FLIGHT,Qt::LeftDockWidgetArea);


    //createDockWidget(pilotView,new HUD(320,240,this),tr("Head Up Display"),"HEAD_UP_DISPLAY_DOCKWIDGET",VIEW_FLIGHT,Qt::LeftDockWidgetArea,this->width()/1.8);

//    createDockWidget(pilotView,new UASQuickView(this),tr("Quick View"),"UAS_INFO_QUICKVIEW_DOCKWIDGET",VIEW_FLIGHT,Qt::LeftDockWidgetArea);
//    createDockWidget(pilotView,new HSIDisplay(this),tr("Horizontal Situation"),"HORIZONTAL_SITUATION_INDICATOR_DOCKWIDGET",VIEW_FLIGHT,Qt::LeftDockWidgetArea);
//    pilotView->setTabPosition(Qt::LeftDockWidgetArea,QTabWidget::North);
//    pilotView->tabifyDockWidget((QDockWidget*)centralWidgetToDockWidgetsMap[VIEW_FLIGHT]["HORIZONTAL_SITUATION_INDICATOR_DOCKWIDGET"],(QDockWidget*)centralWidgetToDockWidgetsMap[VIEW_FLIGHT]["UAS_INFO_QUICKVIEW_DOCKWIDGET"]);

    //UASRawStatusView *view = new UASRawStatusView();
    //view->setDecoder(mavlinkDecoder);
    //view->show();
    //hddisplay->addSource(mavlinkDecoder);
    //createDockWidget(pilotView,new HSIDisplay(this),tr("Horizontal Situation"),"HORIZONTAL_SITUATION_INDICATOR_DOCKWIDGET",VIEW_FLIGHT,Qt::LeftDockWidgetArea);
    //pilotView->setTabPosition(Qt::LeftDockWidgetArea,QTabWidget::North);
    //pilotView->tabifyDockWidget((QDockWidget*)centralWidgetToDockWidgetsMap[VIEW_FLIGHT]["HORIZONTAL_SITUATION_INDICATOR_DOCKWIDGET"],(QDockWidget*)centralWidgetToDockWidgetsMap[VIEW_FLIGHT]["UAS_INFO_QUICKVIEW_DOCKWIDGET"]);


    //createDockWidget(pilotView,new UASActionsWidget(this),tr("Actions"),"UNMANNED_SYSTEM_ACTION_DOCKWIDGET",VIEW_FLIGHT,Qt::RightDockWidgetArea);

    // Custom widgets, added last to all menus and layouts
    buildCustomWidget();



    /*if (!protocolWidget)
    {
        protocolWidget    = new XMLCommProtocolWidget(this);
        addCentralWidget(protocolWidget, "Mavlink Generator");
    }*/


    //    if (!firmwareUpdateWidget)
    //    {
    //        firmwareUpdateWidget    = new QGCFirmwareUpdate(this);
    //        addCentralWidget(firmwareUpdateWidget, "Firmware Update");
    //    }

    /*if (!hudWidget)
    {
        hudWidget         = new HUD(320, 240, this);
        addCentralWidget(hudWidget, tr("Head Up Display"));
    }*/

    /*if (!configWidget)
    {
        configWidget = new QGCVehicleConfig(this);
        addCentralWidget(configWidget, tr("Vehicle Configuration"));
    }*/


    /*if (!dataplotWidget)
    {
        dataplotWidget    = new QGCDataPlot2D(this);
        addCentralWidget(dataplotWidget, tr("Logfile Plot"));
    }*/

#ifdef QGC_OSG_ENABLED
    if (!_3DWidget)
    {
        _3DWidget         = Q3DWidgetFactory::get("PIXHAWK", this);
        addCentralWidget(_3DWidget, tr("Local 3D"));
    }
#endif

#if (defined _MSC_VER) | (defined Q_OS_MAC)
    if (!gEarthWidget)
    {
        gEarthWidget = new QGCGoogleEarthView(this);
        addCentralWidget(gEarthWidget, tr("Google Earth"));
    }
#endif
}

void MainWindow::addTool(SubMainWindow *parent,VIEW_SECTIONS view,QDockWidget* widget, const QString& title, Qt::DockWidgetArea area)
{
    QList<QAction*> actionlist = ui.menuTools->actions();
    bool found = false;
    QAction *targetAction;
    for (int i=0;i<actionlist.size();i++)
    {
        if (actionlist[i]->text() == title)
        {
            found = true;
            targetAction = actionlist[i];
        }
    }
    if (!found)
    {
        QAction* tempAction = ui.menuTools->addAction(title);
        tempAction->setCheckable(true);
        menuToDockNameMap[tempAction] = widget->objectName();
        if (!centralWidgetToDockWidgetsMap.contains(view))
        {
            centralWidgetToDockWidgetsMap[view] = QMap<QString,QWidget*>();
        }
        centralWidgetToDockWidgetsMap[view][widget->objectName()]= widget;
        connect(tempAction,SIGNAL(triggered(bool)),this, SLOT(showTool(bool)));
        connect(widget, SIGNAL(visibilityChanged(bool)), tempAction, SLOT(setChecked(bool)));
        tempAction->setChecked(widget->isVisible());
    }
    else
    {
        if (!menuToDockNameMap.contains(targetAction))
        {
            menuToDockNameMap[targetAction] = widget->objectName();
            //menuToDockNameMap[targetAction] = title;
        }
        if (!centralWidgetToDockWidgetsMap.contains(view))
        {
            centralWidgetToDockWidgetsMap[view] = QMap<QString,QWidget*>();
        }
        centralWidgetToDockWidgetsMap[view][widget->objectName()]= widget;
        connect(widget, SIGNAL(visibilityChanged(bool)), targetAction, SLOT(setChecked(bool)));
    }
    parent->addDockWidget(area,widget);
}

QDockWidget* MainWindow::createDockWidget(QWidget *parent,QWidget *child,QString title,QString objectname,VIEW_SECTIONS view,Qt::DockWidgetArea area,int minwidth,int minheight)
{
    //if (child->objectName() == "")
    //{
    child->setObjectName(objectname);
    //}
    QDockWidget *widget = new QDockWidget(title,this);
    if (!isAdvancedMode)
    {
        if (dockWidgetTitleBarEnabled)
        {
            dockToTitleBarMap[widget] = widget->titleBarWidget();
            QLabel *label = new QLabel(this);
            label->setText(title);
            widget->setTitleBarWidget(label);
            label->installEventFilter(new DockWidgetTitleBarEventFilter());
        }
        else
        {
            dockToTitleBarMap[widget] = widget->titleBarWidget();
            widget->setTitleBarWidget(new QWidget(this));
        }
    }
    else
    {
        QLabel *label = new QLabel(this);
        label->setText(title);
        dockToTitleBarMap[widget] = label;
        label->installEventFilter(new DockWidgetTitleBarEventFilter());
        label->hide();
    }
    widget->setObjectName(child->objectName());
    widget->setWidget(child);
    if (minheight != 0 || minwidth != 0)
    {
        widget->setMinimumHeight(minheight);
        widget->setMinimumWidth(minwidth);
    }
    addTool(qobject_cast<SubMainWindow*>(parent),view,widget,title,area);

    return widget;
}
void MainWindow::loadDockWidget(QString name)
{
    if (centralWidgetToDockWidgetsMap[currentView].contains(name))
    {
        return;
    }
    if (name.startsWith("HIL_CONFIG"))
    {
        //It's a HIL widget.
        showHILConfigurationWidget(UASManager::instance()->getActiveUAS());
    }
    else if (name == "UNMANNED_SYSTEM_CONTROL_DOCKWIDGET")
    {
        createDockWidget(centerStack->currentWidget(),new UASControlWidget(this),tr("Control"),"UNMANNED_SYSTEM_CONTROL_DOCKWIDGET",currentView,Qt::LeftDockWidgetArea);
    }
    else if (name == "UNMANNED_SYSTEM_LIST_DOCKWIDGET")
    {
        createDockWidget(centerStack->currentWidget(),new UASListWidget(this),tr("Unmanned Systems"),"UNMANNED_SYSTEM_LIST_DOCKWIDGET",currentView,Qt::RightDockWidgetArea);
    }
    else if (name == "WAYPOINT_LIST_DOCKWIDGET")
    {
        createDockWidget(centerStack->currentWidget(),new QGCWaypointListMulti(this),tr("Mission Plan"),"WAYPOINT_LIST_DOCKWIDGET",currentView,Qt::BottomDockWidgetArea);
    }
    else if (name == "MAVLINK_INSPECTOR_DOCKWIDGET")
    {
        createDockWidget(centerStack->currentWidget(),new QGCMAVLinkInspector(mavlink,this),tr("MAVLink Inspector"),"MAVLINK_INSPECTOR_DOCKWIDGET",currentView,Qt::RightDockWidgetArea);
    }
    else if (name == "PARAMETER_INTERFACE_DOCKWIDGET")
    {
        createDockWidget(centerStack->currentWidget(),new ParameterInterface(this),tr("Onboard Parameters"),"PARAMETER_INTERFACE_DOCKWIDGET",currentView,Qt::RightDockWidgetArea);
    }
    else if (name == "UAS_STATUS_DETAILS_DOCKWIDGET")
    {
        createDockWidget(centerStack->currentWidget(),new UASInfoWidget(this),tr("Status Details"),"UAS_STATUS_DETAILS_DOCKWIDGET",currentView,Qt::RightDockWidgetArea);
    }
    else if (name == "COMMUNICATION_DEBUG_CONSOLE_DOCKWIDGET")
    {
        //This is now a permanently detached window.
        //centralWidgetToDockWidgetsMap[currentView][name] = console;
        //createDockWidget(centerStack->currentWidget(),new DebugConsole(this),tr("Communication Console"),"COMMUNICATION_DEBUG_CONSOLE_DOCKWIDGET",currentView,Qt::BottomDockWidgetArea);
    }
    else if (name == "HORIZONTAL_SITUATION_INDICATOR_DOCKWIDGET")
    {
        createDockWidget(centerStack->currentWidget(),new HSIDisplay(this),tr("Horizontal Situation"),"HORIZONTAL_SITUATION_INDICATOR_DOCKWIDGET",currentView,Qt::BottomDockWidgetArea);
    }
    else if (name == "HEAD_DOWN_DISPLAY_1_DOCKWIDGET")
    {
        //FIXME: memory of acceptList will never be freed again
        QStringList* acceptList = new QStringList();
        acceptList->append("-3.3,ATTITUDE.roll,rad,+3.3,s");
        acceptList->append("-3.3,ATTITUDE.pitch,deg,+3.3,s");
        acceptList->append("-3.3,ATTITUDE.yaw,deg,+3.3,s");
        HDDisplay *hddisplay = new HDDisplay(acceptList,"Flight Display",this);
        hddisplay->addSource(mavlinkDecoder);
        createDockWidget(centerStack->currentWidget(),hddisplay,tr("Flight Display"),"HEAD_DOWN_DISPLAY_1_DOCKWIDGET",currentView,Qt::RightDockWidgetArea);
    }
    else if (name == "HEAD_DOWN_DISPLAY_2_DOCKWIDGET")
    {
        //FIXME: memory of acceptList2 will never be freed again
        QStringList* acceptList2 = new QStringList();
        acceptList2->append("0,RAW_PRESSURE.pres_abs,hPa,65500");
        HDDisplay *hddisplay = new HDDisplay(acceptList2,"Actuator Status",this);
        hddisplay->addSource(mavlinkDecoder);
        createDockWidget(centerStack->currentWidget(),hddisplay,tr("Actuator Status"),"HEAD_DOWN_DISPLAY_2_DOCKWIDGET",currentView,Qt::RightDockWidgetArea);
    }
    else if (name == "Radio Control")
    {
        qDebug() << "Error loading window:" << name << "Unknown window type";
        //createDockWidget(centerStack->currentWidget(),hddisplay,tr("Actuator Status"),"HEADS_DOWN_DISPLAY_2_DOCKWIDGET",currentView,Qt::RightDockWidgetArea);
    }
    else if (name == "PRIMARY_FLIGHT_DISPLAY_DOCKWIDGET")
    {
        // createDockWidget(centerStack->currentWidget(),new HUD(320,240,this),tr("Head Up Display"),"PRIMARY_FLIGHT_DISPLAY_DOCKWIDGET",currentView,Qt::RightDockWidgetArea);
        createDockWidget(centerStack->currentWidget(),new PrimaryFlightDisplay(320,240,this),tr("Primary Flight Display"),"HEAD_UP_DISPLAY_DOCKWIDGET",currentView,Qt::RightDockWidgetArea);
    }
    else if (name == "UAS_INFO_QUICKVIEW_DOCKWIDGET")
    {
        createDockWidget(centerStack->currentWidget(),new UASQuickView(this),tr("Quick View"),"UAS_INFO_QUICKVIEW_DOCKWIDGET",currentView,Qt::LeftDockWidgetArea);
    }
    else
    {
        if (customWidgetNameToFilenameMap.contains(name))
        {
            loadCustomWidget(customWidgetNameToFilenameMap[name],currentView);
            //customWidgetNameToFilenameMap.remove(name);
        }
        else
        {
            qDebug() << "Error loading window:" << name;
        }
    }
}

void MainWindow::showTool(bool show)
{
    //Called when a menu item is clicked on, regardless of view.

    QAction* act = qobject_cast<QAction *>(sender());
    if (menuToDockNameMap.contains(act))
    {
        QString name = menuToDockNameMap[act];
        if (centralWidgetToDockWidgetsMap.contains(currentView))
        {
            if (centralWidgetToDockWidgetsMap[currentView].contains(name))
            {
                if (show)
                {
                    centralWidgetToDockWidgetsMap[currentView][name]->show();
                }
                else
                {
                    centralWidgetToDockWidgetsMap[currentView][name]->hide();
                }
            }
            else if (show)
            {
                loadDockWidget(name);
            }
        }
    }
    //QWidget* widget = qVariantValue<QWidget *>(act->data());
    //widget->setVisible(show);
}
/*void addToolByName(QString name,SubMainWindow parent,const QString& title, Qt::DockWidgetArea area)
{
    if (name == "Control")
    {
        QDockWidget *widget = new QDockWidget(tr("Control"),this);
        dockToTitleBarMap[widget] = widget->titleBarWidget();
        widget->setObjectName("UNMANNED_SYSTEM_CONTROL_DOCKWIDGET");
        widget->setWidget(new UASControlWidget(this));
        addTool(parent,VIEW_SIMULATION,widget,tr("Control"),area);
    }
}*/
void MainWindow::addCentralWidget(QWidget* widget, const QString& title)
{
    Q_UNUSED(title);
    // Check if this widget already has been added
    if (centerStack->indexOf(widget) == -1)
    {
        centerStack->addWidget(widget);

        //        QAction* tempAction = ui.menuMain->addAction(title);

        //        tempAction->setCheckable(true);
        //        QVariant var;
        //        var.setValue((QWidget*)widget);
        //        tempAction->setData(var);
        //        centerStackActionGroup->addAction(tempAction);
        //        connect(tempAction,SIGNAL(triggered()),this, SLOT(showCentralWidget()));
        //connect(widget, SIGNAL(visibilityChanged(bool)), tempAction, SLOT(setChecked(bool)));
        //        tempAction->setChecked(widget->isVisible());
    }
}


void MainWindow::showCentralWidget()
{
    QAction* act = qobject_cast<QAction *>(sender());
    QWidget* widget = qVariantValue<QWidget *>(act->data());
    centerStack->setCurrentWidget(widget);
}

void MainWindow::showHILConfigurationWidget(UASInterface* uas)
{
    // Add simulation configuration widget
    UAS* mav = dynamic_cast<UAS*>(uas);

    if (mav && !hilDocks.contains(mav->getUASID()))
    {
        //QGCToolWidget* tool = new QGCToolWidget("Unnamed Tool " + QString::number(ui.menuTools->actions().size()));
        //createDockWidget(centerStack->currentWidget(),tool,"Unnamed Tool " + QString::number(ui.menuTools->actions().size()),"UNNAMED_TOOL_" + QString::number(ui.menuTools->actions().size())+"DOCK",currentView,Qt::BottomDockWidgetArea);

        QGCHilConfiguration* hconf = new QGCHilConfiguration(mav, this);
       QString hilDockName = tr("HIL Config %1").arg(uas->getUASName());
        QDockWidget* hilDock = createDockWidget(simView, hconf,hilDockName, hilDockName.toUpper().replace(" ", "_"),VIEW_SIMULATION,Qt::LeftDockWidgetArea);
        hilDocks.insert(mav->getUASID(), hilDock);

        //        if (currentView != VIEW_SIMULATION)
        //            hilDock->hide();
        //        else
        //            hilDock->show();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (isVisible()) storeViewState();
    aboutToCloseFlag = true;
    storeSettings();
    mavlink->storeSettings();
    UASManager::instance()->storeSettings();
    QMainWindow::closeEvent(event);
}

/**
 * Connect the signals and slots of the common window widgets
 */
void MainWindow::connectCommonWidgets()
{
    if (infoDockWidget && infoDockWidget->widget())
    {
        connect(mavlink, SIGNAL(receiveLossChanged(int, float)),
                infoDockWidget->widget(), SLOT(updateSendLoss(int, float)));
    }
}

void MainWindow::createCustomWidget()
{
    //void MainWindow::createDockWidget(QWidget *parent,QWidget *child,QString title,QString objectname,VIEW_SECTIONS view,Qt::DockWidgetArea area,int minwidth,int minheight)
    //QDockWidget* dock = new QDockWidget("Unnamed Tool", this);

    if (QGCToolWidget::instances()->size() < 2)
    {
        // This is the first widget
        ui.menuTools->addSeparator();
    }
    QGCToolWidget* tool = new QGCToolWidget("Unnamed Tool " + QString::number(ui.menuTools->actions().size()));
    createDockWidget(centerStack->currentWidget(),tool,"Unnamed Tool " + QString::number(ui.menuTools->actions().size()),"UNNAMED_TOOL_" + QString::number(ui.menuTools->actions().size())+"DOCK",currentView,Qt::BottomDockWidgetArea);
    //tool->setObjectName("UNNAMED_TOOL_" + QString::number(ui.menuTools->actions().size()));
    QSettings settings;
    settings.beginGroup("QGC_MAINWINDOW");
    settings.setValue(QString("TOOL_PARENT_") + tool->objectName(),currentView);
    settings.endGroup();



    //connect(tool, SIGNAL(destroyed()), dock, SLOT(deleteLater()));
    //dock->setWidget(tool);

    //QAction* showAction = new QAction(tool->getTitle(), this);
    //showAction->setCheckable(true);
    //connect(dock, SIGNAL(visibilityChanged(bool)), showAction, SLOT(setChecked(bool)));
    //connect(showAction, SIGNAL(triggered(bool)), dock, SLOT(setVisible(bool)));
    //tool->setMainMenuAction(showAction);
    //ui.menuTools->addAction(showAction);
    //this->addDockWidget(Qt::BottomDockWidgetArea, dock);
    //dock->setVisible(true);
}

void MainWindow::loadCustomWidget()
{
    QString widgetFileExtension(".qgw");
    QString fileName = QFileDialog::getOpenFileName(this, tr("Specify Widget File Name"), QDesktopServices::storageLocation(QDesktopServices::DesktopLocation), tr("QGroundControl Widget (*%1);;").arg(widgetFileExtension));
    if (fileName != "") loadCustomWidget(fileName);
}
void MainWindow::loadCustomWidget(const QString& fileName, int view)
{
    QGCToolWidget* tool = new QGCToolWidget("", this);
    if (tool->loadSettings(fileName, true))
    {
        qDebug() << "Loading custom tool:" << tool->getTitle() << tool->objectName();
        switch ((VIEW_SECTIONS)view)
        {
        case VIEW_ENGINEER:
            createDockWidget(engineeringView,tool,tool->getTitle(),tool->objectName()+"DOCK",(VIEW_SECTIONS)view,Qt::LeftDockWidgetArea);
            break;
        case VIEW_FLIGHT:
            createDockWidget(pilotView,tool,tool->getTitle(),tool->objectName()+"DOCK",(VIEW_SECTIONS)view,Qt::LeftDockWidgetArea);
            break;
        case VIEW_SIMULATION:
            createDockWidget(simView,tool,tool->getTitle(),tool->objectName()+"DOCK",(VIEW_SECTIONS)view,Qt::LeftDockWidgetArea);
            break;
        case VIEW_MISSION:
            createDockWidget(plannerView,tool,tool->getTitle(),tool->objectName()+"DOCK",(VIEW_SECTIONS)view,Qt::LeftDockWidgetArea);
            break;
        default:
        {
            //Delete tool, create menu item to tie it to.
            customWidgetNameToFilenameMap[tool->objectName()+"DOCK"] = fileName;
            QAction* tempAction = ui.menuTools->addAction(tool->getTitle());
            menuToDockNameMap[tempAction] = tool->objectName()+"DOCK";
            tempAction->setCheckable(true);
            connect(tempAction,SIGNAL(triggered(bool)),this, SLOT(showTool(bool)));
            tool->deleteLater();
            //createDockWidget(centerStack->currentWidget(),tool,tool->getTitle(),tool->objectName()+"DOCK",(VIEW_SECTIONS)view,Qt::LeftDockWidgetArea);
        }
            break;
        }
    }
    else
    {
        return;
    }
}

void MainWindow::loadCustomWidget(const QString& fileName, bool singleinstance)
{
    QGCToolWidget* tool = new QGCToolWidget("", this);
    if (tool->loadSettings(fileName, true) || !singleinstance)
    {
        qDebug() << "Loading custom tool:" << tool->getTitle() << tool->objectName();
        QSettings settings;
        settings.beginGroup("QGC_MAINWINDOW");
        //settings.setValue(QString("TOOL_PARENT_") + "UNNAMED_TOOL_" + QString::number(ui.menuTools->actions().size()),currentView);

        int view = settings.value(QString("TOOL_PARENT_") + tool->objectName(),-1).toInt();
        switch (view)
        {
        case VIEW_ENGINEER:
            createDockWidget(engineeringView,tool,tool->getTitle(),tool->objectName()+"DOCK",(VIEW_SECTIONS)view,Qt::LeftDockWidgetArea);
            break;
        case VIEW_FLIGHT:
            createDockWidget(pilotView,tool,tool->getTitle(),tool->objectName()+"DOCK",(VIEW_SECTIONS)view,Qt::LeftDockWidgetArea);
            break;
        case VIEW_SIMULATION:
            createDockWidget(simView,tool,tool->getTitle(),tool->objectName()+"DOCK",(VIEW_SECTIONS)view,Qt::LeftDockWidgetArea);
            break;
        case VIEW_MISSION:
            createDockWidget(plannerView,tool,tool->getTitle(),tool->objectName()+"DOCK",(VIEW_SECTIONS)view,Qt::LeftDockWidgetArea);
            break;
        default:
        {
            //Delete tool, create menu item to tie it to.
            customWidgetNameToFilenameMap[tool->objectName()+"DOCK"] = fileName;
            QAction* tempAction = ui.menuTools->addAction(tool->getTitle());
            menuToDockNameMap[tempAction] = tool->objectName()+"DOCK";
            tempAction->setCheckable(true);
            connect(tempAction,SIGNAL(triggered(bool)),this, SLOT(showTool(bool)));
            tool->deleteLater();
            //createDockWidget(centerStack->currentWidget(),tool,tool->getTitle(),tool->objectName()+"DOCK",(VIEW_SECTIONS)view,Qt::LeftDockWidgetArea);
        }
            break;
        }


        settings.endGroup();
        // Add widget to UI
        /*QDockWidget* dock = new QDockWidget(tool->getTitle(), this);
        connect(tool, SIGNAL(destroyed()), dock, SLOT(deleteLater()));
        dock->setWidget(tool);
        tool->setParent(dock);

        QAction* showAction = new QAction(tool->getTitle(), this);
        showAction->setCheckable(true);
        connect(dock, SIGNAL(visibilityChanged(bool)), showAction, SLOT(setChecked(bool)));
        connect(showAction, SIGNAL(triggered(bool)), dock, SLOT(setVisible(bool)));
        tool->setMainMenuAction(showAction);
        ui.menuTools->addAction(showAction);
        this->addDockWidget(Qt::BottomDockWidgetArea, dock);
        dock->hide();*/
    }
    else
    {
        return;
    }
}

void MainWindow::loadCustomWidgetsFromDefaults(const QString& systemType, const QString& autopilotType)
{
    QString defaultsDir = qApp->applicationDirPath() + "/files/" + autopilotType.toLower() + "/widgets/";
    QString platformDir = qApp->applicationDirPath() + "/files/" + autopilotType.toLower() + "/" + systemType.toLower() + "/widgets/";

    QDir widgets(defaultsDir);
    QStringList files = widgets.entryList();
    QDir platformWidgets(platformDir);
    files.append(platformWidgets.entryList());

    if (files.count() == 0)
    {
        qDebug() << "No default custom widgets for system " << systemType << "autopilot" << autopilotType << " found";
        qDebug() << "Tried with path: " << defaultsDir;
        showStatusMessage(tr("Did not find any custom widgets in %1").arg(defaultsDir));
    }

    // Load all custom widgets found in the AP folder
    for(int i = 0; i < files.count(); ++i)
    {
        QString file = files[i];
        if (file.endsWith(".qgw"))
        {
            // Will only be loaded if not already a custom widget with
            // the same name is present
            loadCustomWidget(defaultsDir+"/"+file, true);
            showStatusMessage(tr("Loaded custom widget %1").arg(defaultsDir+"/"+file));
        }
    }
}

void MainWindow::loadSettings()
{
    QSettings settings;
    settings.beginGroup("QGC_MAINWINDOW");
    autoReconnect = settings.value("AUTO_RECONNECT", autoReconnect).toBool();
    currentStyle = (QGC_MAINWINDOW_STYLE)settings.value("CURRENT_STYLE", currentStyle).toInt();
    darkStyleFileName = settings.value("DARK_STYLE_FILENAME", darkStyleFileName).toString();
    lightStyleFileName = settings.value("LIGHT_STYLE_FILENAME", lightStyleFileName).toString();
    lowPowerMode = settings.value("LOW_POWER_MODE", lowPowerMode).toBool();
    dockWidgetTitleBarEnabled = settings.value("DOCK_WIDGET_TITLEBARS",dockWidgetTitleBarEnabled).toBool();
    settings.endGroup();
    enableDockWidgetTitleBars(dockWidgetTitleBarEnabled);
}

void MainWindow::storeSettings()
{
    QSettings settings;
    settings.beginGroup("QGC_MAINWINDOW");
    settings.setValue("AUTO_RECONNECT", autoReconnect);
    settings.setValue("CURRENT_STYLE", currentStyle);
    settings.setValue("DARK_STYLE_FILENAME", darkStyleFileName);
    settings.setValue("LIGHT_STYLE_FILENAME", lightStyleFileName);
    settings.endGroup();
    if (!aboutToCloseFlag && isVisible())
    {
        settings.setValue(getWindowGeometryKey(), saveGeometry());
        // Save the last current view in any case
        settings.setValue("CURRENT_VIEW", currentView);
        // Save the current window state, but only if a system is connected (else no real number of widgets would be present))
        if (UASManager::instance()->getUASList().length() > 0) settings.setValue(getWindowStateKey(), saveState(QGC::applicationVersion()));
        // Save the current view only if a UAS is connected
        if (UASManager::instance()->getUASList().length() > 0) settings.setValue("CURRENT_VIEW_WITH_UAS_CONNECTED", currentView);
        // Save the current power mode
    }
    settings.setValue("LOW_POWER_MODE", lowPowerMode);
    settings.sync();
}

void MainWindow::configureWindowName()
{
    QList<QHostAddress> hostAddresses = QNetworkInterface::allAddresses();
    QString windowname = qApp->applicationName() + " " + qApp->applicationVersion();
    bool prevAddr = false;

    windowname.append(" (" + QHostInfo::localHostName() + ": ");

    for (int i = 0; i < hostAddresses.size(); i++)
    {
        // Exclude loopback IPv4 and all IPv6 addresses
        if (hostAddresses.at(i) != QHostAddress("127.0.0.1") && !hostAddresses.at(i).toString().contains(":"))
        {
            if(prevAddr) windowname.append("/");
            windowname.append(hostAddresses.at(i).toString());
            prevAddr = true;
        }
    }

    windowname.append(")");

    setWindowTitle(windowname);

#ifndef Q_WS_MAC
    //qApp->setWindowIcon(QIcon(":/core/images/qtcreator_logo_128.png"));
#endif
}

void MainWindow::startVideoCapture()
{
    QString format = "bmp";
    QString initialPath = QDir::currentPath() + tr("/untitled.") + format;

    QString screenFileName = QFileDialog::getSaveFileName(this, tr("Save As"),
                                                          initialPath,
                                                          tr("%1 Files (*.%2);;All Files (*)")
                                                          .arg(format.toUpper())
                                                          .arg(format));
    delete videoTimer;
    videoTimer = new QTimer(this);
    //videoTimer->setInterval(40);
    //connect(videoTimer, SIGNAL(timeout()), this, SLOT(saveScreen()));
    //videoTimer->stop();
}

void MainWindow::stopVideoCapture()
{
    videoTimer->stop();

    // TODO Convert raw images to PNG
}

void MainWindow::saveScreen()
{
    QPixmap window = QPixmap::grabWindow(this->winId());
    QString format = "bmp";

    if (!screenFileName.isEmpty())
    {
        window.save(screenFileName, format.toAscii());
    }
}
void MainWindow::enableDockWidgetTitleBars(bool enabled)
{
    dockWidgetTitleBarEnabled = enabled;
    QSettings settings;
    settings.beginGroup("QGC_MAINWINDOW");
    settings.setValue("DOCK_WIDGET_TITLEBARS",dockWidgetTitleBarEnabled);
    settings.endGroup();
    settings.sync();
    if (!isAdvancedMode)
    {
        if (enabled)
        {
            for (QMap<QDockWidget*,QWidget*>::const_iterator i=dockToTitleBarMap.constBegin();i!=dockToTitleBarMap.constEnd();i++)
            {
                QLabel *label = new QLabel(this);
                label->setText(i.key()->windowTitle());
                i.key()->setTitleBarWidget(label);
                //label->setEnabled(false);
                label->installEventFilter(new DockWidgetTitleBarEventFilter());
            }
        }
        else
        {
            for (QMap<QDockWidget*,QWidget*>::const_iterator i=dockToTitleBarMap.constBegin();i!=dockToTitleBarMap.constEnd();i++)
            {
                i.key()->setTitleBarWidget(new QWidget(this));
            }
        }
    }
}

void MainWindow::enableAutoReconnect(bool enabled)
{
    autoReconnect = enabled;
}

bool MainWindow::loadStyle(QGC_MAINWINDOW_STYLE style, QString cssFile)
{
    // Store the new style classification.
    currentStyle = style;

    // Load the new stylesheet.
    QFile styleSheet(cssFile);

    // Attempt to open the stylesheet.
    if (styleSheet.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        // Signal to the user that the app will pause to apply a new stylesheet
        qApp->setOverrideCursor(Qt::WaitCursor);

        qApp->setStyleSheet(styleSheet.readAll());

        // And save the new stylesheet path.
        if (currentStyle == QGC_MAINWINDOW_STYLE_LIGHT)
        {
            lightStyleFileName = cssFile;
        }
        else
        {
            darkStyleFileName = cssFile;
        }

        // And trigger any changes to other UI elements that are watching for
        // theme changes.
        emit styleChanged(style);

        // Finally restore the cursor before returning.
        qApp->restoreOverrideCursor();
        return true;
    }

    // Otherwise alert return a failure code.
    return false;
}

/**
 * The status message will be overwritten if a new message is posted to this function
 *
 * @param status message text
 * @param timeout how long the status should be displayed
 */
void MainWindow::showStatusMessage(const QString& status, int timeout)
{
    statusBar()->showMessage(status, timeout);
}

/**
 * The status message will be overwritten if a new message is posted to this function.
 * it will be automatically hidden after 5 seconds.
 *
 * @param status message text
 */
void MainWindow::showStatusMessage(const QString& status)
{
    statusBar()->showMessage(status, 20000);
}

void MainWindow::showCriticalMessage(const QString& title, const QString& message)
{
    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setText(title);
    msgBox.setInformativeText(message);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();
}

void MainWindow::showInfoMessage(const QString& title, const QString& message)
{
    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setText(title);
    msgBox.setInformativeText(message);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();
}

/**
* @brief Create all actions associated to the main window
*
**/
void MainWindow::connectCommonActions()
{
    // Bind together the perspective actions
    QActionGroup* perspectives = new QActionGroup(ui.menuPerspectives);
    perspectives->addAction(ui.actionEngineersView);
    perspectives->addAction(ui.actionMavlinkView);
    perspectives->addAction(ui.actionFlightView);
    perspectives->addAction(ui.actionSimulation_View);
    perspectives->addAction(ui.actionMissionView);
    perspectives->addAction(ui.actionConfiguration_2);
    perspectives->addAction(ui.actionFirmwareUpdateView);
    perspectives->addAction(ui.actionUnconnectedView);
    perspectives->setExclusive(true);

    // Mark the right one as selected
    if (currentView == VIEW_ENGINEER)
    {
        ui.actionEngineersView->setChecked(true);
        ui.actionEngineersView->activate(QAction::Trigger);
    }
    if (currentView == VIEW_MAVLINK)
    {
        ui.actionMavlinkView->setChecked(true);
        ui.actionMavlinkView->activate(QAction::Trigger);
    }
    if (currentView == VIEW_FLIGHT)
    {
        ui.actionFlightView->setChecked(true);
        ui.actionFlightView->activate(QAction::Trigger);
    }
    if (currentView == VIEW_SIMULATION)
    {
        ui.actionSimulation_View->setChecked(true);
        ui.actionSimulation_View->activate(QAction::Trigger);
    }
    if (currentView == VIEW_MISSION)
    {
        ui.actionMissionView->setChecked(true);
        ui.actionMissionView->activate(QAction::Trigger);
    }
    if (currentView == VIEW_CONFIGURATION)
    {
        ui.actionConfiguration_2->setChecked(true);
        ui.actionConfiguration_2->activate(QAction::Trigger);
    }
    if (currentView == VIEW_FIRMWAREUPDATE)
    {
        ui.actionFirmwareUpdateView->setChecked(true);
        ui.actionFirmwareUpdateView->activate(QAction::Trigger);
    }
    if (currentView == VIEW_UNCONNECTED)
    {
        ui.actionUnconnectedView->setChecked(true);
        ui.actionUnconnectedView->activate(QAction::Trigger);
    }

    // The UAS actions are not enabled without connection to system
    ui.actionLiftoff->setEnabled(false);
    ui.actionLand->setEnabled(false);
    ui.actionEmergency_Kill->setEnabled(false);
    ui.actionEmergency_Land->setEnabled(false);
    ui.actionShutdownMAV->setEnabled(false);

    // Connect actions from ui
    connect(ui.actionAdd_Link, SIGNAL(triggered()), this, SLOT(addLink()));
    connect(ui.actionAdvanced_Mode,SIGNAL(triggered()),this,SLOT(setAdvancedMode()));

    // Connect internal actions
    connect(UASManager::instance(), SIGNAL(UASCreated(UASInterface*)), this, SLOT(UASCreated(UASInterface*)));
    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setActiveUAS(UASInterface*)));

    // Unmanned System controls
    connect(ui.actionLiftoff, SIGNAL(triggered()), UASManager::instance(), SLOT(launchActiveUAS()));
    connect(ui.actionLand, SIGNAL(triggered()), UASManager::instance(), SLOT(returnActiveUAS()));
    connect(ui.actionEmergency_Land, SIGNAL(triggered()), UASManager::instance(), SLOT(stopActiveUAS()));
    connect(ui.actionEmergency_Kill, SIGNAL(triggered()), UASManager::instance(), SLOT(killActiveUAS()));
    connect(ui.actionShutdownMAV, SIGNAL(triggered()), UASManager::instance(), SLOT(shutdownActiveUAS()));
    connect(ui.actionConfiguration, SIGNAL(triggered()), UASManager::instance(), SLOT(configureActiveUAS()));

    // Views actions
    connect(ui.actionFlightView, SIGNAL(triggered()), this, SLOT(loadPilotView()));
    connect(ui.actionSimulation_View, SIGNAL(triggered()), this, SLOT(loadSimulationView()));
    connect(ui.actionEngineersView, SIGNAL(triggered()), this, SLOT(loadEngineerView()));
    connect(ui.actionMissionView, SIGNAL(triggered()), this, SLOT(loadOperatorView()));
    connect(ui.actionUnconnectedView, SIGNAL(triggered()), this, SLOT(loadUnconnectedView()));
    connect(ui.actionConfiguration_2,SIGNAL(triggered()),this,SLOT(loadConfigurationView()));

    connect(ui.actionFirmwareUpdateView, SIGNAL(triggered()), this, SLOT(loadFirmwareUpdateView()));
    connect(ui.actionMavlinkView, SIGNAL(triggered()), this, SLOT(loadMAVLinkView()));

    // Help Actions
    connect(ui.actionOnline_Documentation, SIGNAL(triggered()), this, SLOT(showHelp()));
    connect(ui.actionDeveloper_Credits, SIGNAL(triggered()), this, SLOT(showCredits()));
    connect(ui.actionProject_Roadmap_2, SIGNAL(triggered()), this, SLOT(showRoadMap()));

    // Custom widget actions
    connect(ui.actionNewCustomWidget, SIGNAL(triggered()), this, SLOT(createCustomWidget()));
    connect(ui.actionLoadCustomWidgetFile, SIGNAL(triggered()), this, SLOT(loadCustomWidget()));

    // Audio output
    ui.actionMuteAudioOutput->setChecked(GAudioOutput::instance()->isMuted());
    connect(GAudioOutput::instance(), SIGNAL(mutedChanged(bool)), ui.actionMuteAudioOutput, SLOT(setChecked(bool)));
    connect(ui.actionMuteAudioOutput, SIGNAL(triggered(bool)), GAudioOutput::instance(), SLOT(mute(bool)));

    // User interaction
    // NOTE: Joystick thread is not started and
    // configuration widget is not instantiated
    // unless it is actually used
    // so no ressources spend on this.
    ui.actionJoystickSettings->setVisible(true);

    // Configuration
    // Joystick
    connect(ui.actionJoystickSettings, SIGNAL(triggered()), this, SLOT(configure()));
    // Application Settings
    connect(ui.actionSettings, SIGNAL(triggered()), this, SLOT(showSettings()));
}

void MainWindow::showHelp()
{
    if(!QDesktopServices::openUrl(QUrl("http://qgroundcontrol.org/users/start")))
    {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText("Could not open help in browser");
        msgBox.setInformativeText("To get to the online help, please open http://qgroundcontrol.org/user_guide in a browser.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
    }
}

void MainWindow::showCredits()
{
    if(!QDesktopServices::openUrl(QUrl("http://qgroundcontrol.org/credits")))
    {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText("Could not open credits in browser");
        msgBox.setInformativeText("To get to the online help, please open http://qgroundcontrol.org/credits in a browser.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
    }
}

void MainWindow::showRoadMap()
{
    if(!QDesktopServices::openUrl(QUrl("http://qgroundcontrol.org/dev/roadmap")))
    {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText("Could not open roadmap in browser");
        msgBox.setInformativeText("To get to the online help, please open http://qgroundcontrol.org/roadmap in a browser.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
    }
}

void MainWindow::configure()
{
    if (!joystickWidget)
    {
        if (!joystick->isRunning())
        {
            joystick->start();
        }
        joystickWidget = new JoystickWidget(joystick, this);
    }
    joystickWidget->show();
}

void MainWindow::showSettings()
{
    QGCSettingsWidget* settings = new QGCSettingsWidget(this);
    settings->show();
}

void MainWindow::addLink()
{
    SerialLink* link = new SerialLink();
    // TODO This should be only done in the dialog itself

    LinkManager::instance()->add(link);
    LinkManager::instance()->addProtocol(link, mavlink);

    // Go fishing for this link's configuration window
    QList<QAction*> actions = ui.menuNetwork->actions();

    const int32_t& linkIndex(LinkManager::instance()->getLinks().indexOf(link));
    const int32_t& linkID(LinkManager::instance()->getLinks()[linkIndex]->getId());

    foreach (QAction* act, actions)
    {
        if (act->data().toInt() == linkID)
        { // LinkManager::instance()->getLinks().indexOf(link)
            act->trigger();
            break;
        }
    }
}

void MainWindow::addLink(LinkInterface *link)
{
    // IMPORTANT! KEEP THESE TWO LINES
    // THEY MAKE SURE THE LINK IS PROPERLY REGISTERED
    // BEFORE LINKING THE UI AGAINST IT
    // Register (does nothing if already registered)
    LinkManager::instance()->add(link);
    LinkManager::instance()->addProtocol(link, mavlink);

    // Go fishing for this link's configuration window
    QList<QAction*> actions = ui.menuNetwork->actions();

    bool found(false);

    const int32_t& linkIndex(LinkManager::instance()->getLinks().indexOf(link));
    const int32_t& linkID(LinkManager::instance()->getLinks()[linkIndex]->getId());

    foreach (QAction* act, actions)
    {
        if (act->data().toInt() == linkID)
        { // LinkManager::instance()->getLinks().indexOf(link)
            found = true;
        }
    }

    //UDPLink* udp = dynamic_cast<UDPLink*>(link);

    if (!found)
    {  //  || udp
        CommConfigurationWindow* commWidget = new CommConfigurationWindow(link, mavlink, this);
        commsWidgetList.append(commWidget);
        connect(commWidget,SIGNAL(destroyed(QObject*)),this,SLOT(commsWidgetDestroyed(QObject*)));
        QAction* action = commWidget->getAction();
        ui.menuNetwork->addAction(action);

        // Error handling
        connect(link, SIGNAL(communicationError(QString,QString)), this, SLOT(showCriticalMessage(QString,QString)), Qt::QueuedConnection);
        // Special case for simulationlink
        MAVLinkSimulationLink* sim = dynamic_cast<MAVLinkSimulationLink*>(link);
        if (sim)
        {
            connect(ui.actionSimulate, SIGNAL(triggered(bool)), sim, SLOT(connectLink(bool)));
        }
    }
}
void MainWindow::commsWidgetDestroyed(QObject *obj)
{
    if (commsWidgetList.contains(obj))
    {
        commsWidgetList.removeOne(obj);
    }
}

void MainWindow::setActiveUAS(UASInterface* uas)
{
    Q_UNUSED(uas);
    // Enable and rename menu
    //    ui.menuUnmanned_System->setTitle(uas->getUASName());
    //    if (!ui.menuUnmanned_System->isEnabled()) ui.menuUnmanned_System->setEnabled(true);
    if (settings.contains(getWindowStateKey()))
    {
        SubMainWindow *win = qobject_cast<SubMainWindow*>(centerStack->currentWidget());
        //settings.setValue(getWindowStateKey(), win->saveState(QGC::applicationVersion()))
        win->restoreState(settings.value(getWindowStateKey()).toByteArray(), QGC::applicationVersion());
    }

}

void MainWindow::UASSpecsChanged(int uas)
{
    UASInterface* activeUAS = UASManager::instance()->getActiveUAS();
    if (activeUAS)
    {
        if (activeUAS->getUASID() == uas)
        {
            //            ui.menuUnmanned_System->setTitle(activeUAS->getUASName());
        }
    }
    else
    {
        // Last system deleted
        //        ui.menuUnmanned_System->setTitle(tr("No System"));
        //        ui.menuUnmanned_System->setEnabled(false);
    }
}

void MainWindow::UASCreated(UASInterface* uas)
{

    // Check if this is the 2nd system and we need a switch menu
    if (UASManager::instance()->getUASList().count() > 1)
        //        ui.menuConnected_Systems->setEnabled(true);

        // Connect the UAS to the full user interface

        //if (uas != NULL)
        //{
        // The pilot, operator and engineer views were not available on startup, enable them now
        ui.actionFlightView->setEnabled(true);
        ui.actionMissionView->setEnabled(true);
        ui.actionEngineersView->setEnabled(true);
        // The UAS actions are not enabled without connection to system
        ui.actionLiftoff->setEnabled(true);
        ui.actionLand->setEnabled(true);
        ui.actionEmergency_Kill->setEnabled(true);
        ui.actionEmergency_Land->setEnabled(true);
        ui.actionShutdownMAV->setEnabled(true);

        QIcon icon;
        // Set matching icon
        switch (uas->getSystemType())
        {
        case MAV_TYPE_GENERIC:
            icon = QIcon(":files/images/mavs/generic.svg");
            break;
        case MAV_TYPE_FIXED_WING:
            icon = QIcon(":files/images/mavs/fixed-wing.svg");
            break;
        case MAV_TYPE_QUADROTOR:
            icon = QIcon(":files/images/mavs/quadrotor.svg");
            break;
        case MAV_TYPE_COAXIAL:
            icon = QIcon(":files/images/mavs/coaxial.svg");
            break;
        case MAV_TYPE_HELICOPTER:
            icon = QIcon(":files/images/mavs/helicopter.svg");
            break;
        case MAV_TYPE_ANTENNA_TRACKER:
            icon = QIcon(":files/images/mavs/antenna-tracker.svg");
            break;
        case MAV_TYPE_GCS:
            icon = QIcon(":files/images/mavs/groundstation.svg");
            break;
        case MAV_TYPE_AIRSHIP:
            icon = QIcon(":files/images/mavs/airship.svg");
            break;
        case MAV_TYPE_FREE_BALLOON:
            icon = QIcon(":files/images/mavs/free-balloon.svg");
            break;
        case MAV_TYPE_ROCKET:
            icon = QIcon(":files/images/mavs/rocket.svg");
            break;
        case MAV_TYPE_GROUND_ROVER:
            icon = QIcon(":files/images/mavs/ground-rover.svg");
            break;
        case MAV_TYPE_SURFACE_BOAT:
            icon = QIcon(":files/images/mavs/surface-boat.svg");
            break;
        case MAV_TYPE_SUBMARINE:
            icon = QIcon(":files/images/mavs/submarine.svg");
            break;
        case MAV_TYPE_HEXAROTOR:
            icon = QIcon(":files/images/mavs/hexarotor.svg");
            break;
        case MAV_TYPE_OCTOROTOR:
            icon = QIcon(":files/images/mavs/octorotor.svg");
            break;
        case MAV_TYPE_TRICOPTER:
            icon = QIcon(":files/images/mavs/tricopter.svg");
            break;
        case MAV_TYPE_FLAPPING_WING:
            icon = QIcon(":files/images/mavs/flapping-wing.svg");
            break;
        case MAV_TYPE_KITE:
            icon = QIcon(":files/images/mavs/kite.svg");
            break;
        default:
            icon = QIcon(":files/images/mavs/unknown.svg");
            break;
        }

    // XXX The multi-UAS selection menu has been disabled for now,
    // its redundant with right-clicking the UAS in the list.
    // this code piece might be removed later if this is the final
    // conclusion (May 2013)
    //        QAction* uasAction = new QAction(icon, tr("Select %1 for control").arg(uas->getUASName()), ui.menuConnected_Systems);
    //        connect(uasAction, SIGNAL(triggered()), uas, SLOT(setSelected()));
    //        ui.menuConnected_Systems->addAction(uasAction);


    connect(uas, SIGNAL(systemSpecsChanged(int)), this, SLOT(UASSpecsChanged(int)));

    // HIL
    showHILConfigurationWidget(uas);

    if (!linechartWidget)
    {
        linechartWidget = new Linecharts(this);
        //linechartWidget->hide();

    }

    linechartWidget->addSource(mavlinkDecoder);
    if (engineeringView->centralWidget() != linechartWidget)
    {
        engineeringView->setCentralWidget(linechartWidget);
        linechartWidget->show();
    }

    // Load default custom widgets for this autopilot type
    loadCustomWidgetsFromDefaults(uas->getSystemTypeName(), uas->getAutopilotTypeName());


    if (uas->getAutopilotType() == MAV_AUTOPILOT_PIXHAWK)
    {
        // Dock widgets
        if (!detectionDockWidget)
        {
            detectionDockWidget = new QDockWidget(tr("Object Recognition"), this);
            detectionDockWidget->setWidget( new ObjectDetectionView("files/images/patterns", this) );
            detectionDockWidget->setObjectName("OBJECT_DETECTION_DOCK_WIDGET");
            //addTool(detectionDockWidget, tr("Object Recognition"), Qt::RightDockWidgetArea);
        }

        if (!watchdogControlDockWidget)
        {
            watchdogControlDockWidget = new QDockWidget(tr("Process Control"), this);
            watchdogControlDockWidget->setWidget( new WatchdogControl(this) );
            watchdogControlDockWidget->setObjectName("WATCHDOG_CONTROL_DOCKWIDGET");
            //addTool(watchdogControlDockWidget, tr("Process Control"), Qt::BottomDockWidgetArea);
        }
    }

    // Change the view only if this is the first UAS

    // If this is the first connected UAS, it is both created as well as
    // the currently active UAS
    if (UASManager::instance()->getUASList().size() == 1)
    {
        // Load last view if setting is present
        if (settings.contains("CURRENT_VIEW_WITH_UAS_CONNECTED"))
        {
            /*int view = settings.value("CURRENT_VIEW_WITH_UAS_CONNECTED").toInt();
                switch (view)
                {
                case VIEW_ENGINEER:
                    loadEngineerView();
                    break;
                case VIEW_MAVLINK:
                    loadMAVLinkView();
                    break;
                case VIEW_FIRMWAREUPDATE:
                    loadFirmwareUpdateView();
                    break;
                case VIEW_FLIGHT:
                    loadPilotView();
                    break;
                case VIEW_SIMULATION:
                    loadSimulationView();
                    break;
                case VIEW_UNCONNECTED:
                    loadUnconnectedView();
                    break;
                case VIEW_MISSION:
                default:
                    loadOperatorView();
                    break;
                }*/
        }
        else
        {
            // loadOperatorView();
        }
    }

    //}

    //    if (!ui.menuConnected_Systems->isEnabled()) ui.menuConnected_Systems->setEnabled(true);
    //    if (!ui.menuUnmanned_System->isEnabled()) ui.menuUnmanned_System->setEnabled(true);

    // Reload view state in case new widgets were added
    loadViewState();
}

void MainWindow::UASDeleted(UASInterface* uas)
{
    Q_UNUSED(uas);
    if (UASManager::instance()->getUASList().count() == 0)
    {
        // Last system deleted
        //        ui.menuUnmanned_System->setTitle(tr("No System"));
        //        ui.menuUnmanned_System->setEnabled(false);
    }

    //    QAction* act;
    //    QList<QAction*> actions = ui.menuConnected_Systems->actions();

    //    foreach (act, actions)
    //    {
    //        if (act->text().contains(uas->getUASName()))
    //            ui.menuConnected_Systems->removeAction(act);
    //    }
}

/**
 * Stores the current view state
 */
void MainWindow::storeViewState()
{
    if (!aboutToCloseFlag)
    {
        // Save current state
        SubMainWindow *win = qobject_cast<SubMainWindow*>(centerStack->currentWidget());
        QList<QDockWidget*> widgets = win->findChildren<QDockWidget*>();
        QString widgetnames = "";
        for (int i=0;i<widgets.size();i++)
        {
            widgetnames += widgets[i]->objectName() + ",";
        }
        widgetnames = widgetnames.mid(0,widgetnames.length()-1);

        settings.setValue(getWindowStateKey() + "WIDGETS",widgetnames);
        settings.setValue(getWindowStateKey(), win->saveState(QGC::applicationVersion()));
        settings.setValue(getWindowStateKey()+"CENTER_WIDGET", centerStack->currentIndex());
        // Although we want save the state of the window, we do not want to change the top-leve state (minimized, maximized, etc)
        // therefore this state is stored here and restored after applying the rest of the settings in the new
        // perspective.
        windowStateVal = this->windowState();
        settings.setValue(getWindowGeometryKey(), saveGeometry());
    }
}

void MainWindow::loadViewState()
{
    // Restore center stack state
    int index = settings.value(getWindowStateKey()+"CENTER_WIDGET", -1).toInt();
    // The offline plot view is usually the consequence of a logging run, always show the realtime view first
    if (centerStack->indexOf(engineeringView) == index)
    {
        // Rewrite to realtime plot
        //index = centerStack->indexOf(linechartWidget);
    }

    if (index != -1)
    {
        centerStack->setCurrentIndex(index);
    }
    else
    {
        // Hide custom widgets
        if (detectionDockWidget) detectionDockWidget->hide();
        if (watchdogControlDockWidget) watchdogControlDockWidget->hide();

        // Load defaults
        switch (currentView)
        {
        case VIEW_CONFIGURATION:
            centerStack->setCurrentWidget(configView);
            break;
        case VIEW_ENGINEER:
            centerStack->setCurrentWidget(engineeringView);
            break;
        case VIEW_FLIGHT:
            centerStack->setCurrentWidget(pilotView);
            break;
        case VIEW_MAVLINK:
            centerStack->setCurrentWidget(mavlinkView);
            break;
        case VIEW_FIRMWAREUPDATE:
            centerStack->setCurrentWidget(firmwareUpdateWidget);
            break;
        case VIEW_MISSION:
            centerStack->setCurrentWidget(plannerView);
            break;

        case VIEW_SIMULATION:
            centerStack->setCurrentWidget(simView);
            break;

        case VIEW_UNCONNECTED:
        case VIEW_FULL:
        default:
            //centerStack->setCurrentWidget(mapWidget);
            if (controlDockWidget)
            {
                controlDockWidget->hide();
            }
            if (listDockWidget)
            {
                listDockWidget->show();
            }
            break;
        }
    }

    // Restore the widget positions and size
    if (settings.contains(getWindowStateKey() + "WIDGETS"))
    {
        QString widgetstr = settings.value(getWindowStateKey() + "WIDGETS").toString();
        QStringList split = widgetstr.split(",");
        foreach (QString widgetname,split)
        {
            if (widgetname != "")
            {
                qDebug() << "Loading widget:" << widgetname;
                loadDockWidget(widgetname);
            }
        }
    }
    if (settings.contains(getWindowStateKey()))
    {
        SubMainWindow *win = qobject_cast<SubMainWindow*>(centerStack->currentWidget());
        //settings.setValue(getWindowStateKey(), win->saveState(QGC::applicationVersion()))
        win->restoreState(settings.value(getWindowStateKey()).toByteArray(), QGC::applicationVersion());
    }
}
void MainWindow::setAdvancedMode()
{
    if (!isAdvancedMode)
    {
        ui.actionAdvanced_Mode->setChecked(true);
        isAdvancedMode = true;
        settings.setValue("ADVANCED_MODE",true);
        for (QMap<QDockWidget*,QWidget*>::const_iterator i=dockToTitleBarMap.constBegin();i!=dockToTitleBarMap.constEnd();i++)
        {
            //QWidget *widget = i.value();
            QWidget *widget = i.key()->titleBarWidget();
            i.key()->setTitleBarWidget(i.value());
            dockToTitleBarMap[i.key()] = widget;

        }
    }
    else
    {
        ui.actionAdvanced_Mode->setChecked(false);
        isAdvancedMode = false;
        settings.setValue("ADVANCED_MODE",false);
        for (QMap<QDockWidget*,QWidget*>::const_iterator i=dockToTitleBarMap.constBegin();i!=dockToTitleBarMap.constEnd();i++)
        {
            //QWidget *widget = i.value();
            QWidget *widget = i.key()->titleBarWidget();
            i.key()->setTitleBarWidget(i.value());
            dockToTitleBarMap[i.key()] = widget;
        }
    }
}

void MainWindow::loadEngineerView()
{
    if (currentView != VIEW_ENGINEER)
    {
        storeViewState();
        currentView = VIEW_ENGINEER;
        ui.actionEngineersView->setChecked(true);
        loadViewState();
    }
}

void MainWindow::loadOperatorView()
{
    if (currentView != VIEW_MISSION)
    {
        storeViewState();
        currentView = VIEW_MISSION;
        ui.actionMissionView->setChecked(true);
        loadViewState();
    }
}
void MainWindow::loadConfigurationView()
{
    if (currentView != VIEW_CONFIGURATION)
    {
        storeViewState();
        currentView = VIEW_CONFIGURATION;
        ui.actionConfiguration_2->setChecked(true);
        loadViewState();
    }
}

void MainWindow::loadUnconnectedView()
{
    if (currentView != VIEW_UNCONNECTED)
    {
        storeViewState();
        currentView = VIEW_UNCONNECTED;
        ui.actionUnconnectedView->setChecked(true);
        loadViewState();
    }
}

void MainWindow::loadPilotView()
{
    if (currentView != VIEW_FLIGHT)
    {
        storeViewState();
        currentView = VIEW_FLIGHT;
        ui.actionFlightView->setChecked(true);
        loadViewState();
    }
}

void MainWindow::loadSimulationView()
{
    if (currentView != VIEW_SIMULATION)
    {
        storeViewState();
        currentView = VIEW_SIMULATION;
        ui.actionSimulation_View->setChecked(true);
        loadViewState();
    }
}

void MainWindow::loadMAVLinkView()
{
    if (currentView != VIEW_MAVLINK)
    {
        storeViewState();
        currentView = VIEW_MAVLINK;
        ui.actionMavlinkView->setChecked(true);
        loadViewState();
    }
}

void MainWindow::loadFirmwareUpdateView()
{
    if (currentView != VIEW_FIRMWAREUPDATE)
    {
        storeViewState();
        currentView = VIEW_FIRMWAREUPDATE;
        ui.actionFirmwareUpdateView->setChecked(true);
        loadViewState();
    }
}

//void MainWindow::loadDataView(QString fileName)
//{
//    // Plot is now selected, now load data from file
//    if (dataView)
//    {
//        //dataView->setCentralWidget(new QGCDataPlot2D(this));
//        QGCDataPlot2D *plot = qobject_cast<QGCDataPlot2D*>(dataView->centralWidget());
//        if (plot)
//        {
//            plot->loadFile(fileName);
//        }
//    }
//    /*QStackedWidget *centerStack = dynamic_cast<QStackedWidget*>(centralWidget());
//    if (centerStack)
//    {
//        centerStack->setCurrentWidget(dataView);
//        dataplotWidget->loadFile(fileName);
//    }*/
//}


QList<QAction*> MainWindow::listLinkMenuActions(void)
{
    return ui.menuNetwork->actions();
}

#ifdef MOUSE_ENABLED_LINUX
bool MainWindow::x11Event(XEvent *event)
{
    emit x11EventOccured(event);
    //qDebug("XEvent occured...");
    return false;
}
#endif // MOUSE_ENABLED_LINUX
