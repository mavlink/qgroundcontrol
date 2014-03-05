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
#include <QDeclarativeView>
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
#include "QGCStatusBar.h"
#include "UASQuickView.h"
#include "QGCDataPlot2D.h"
#include "Linecharts.h"
#include "UASActionsWidget.h"
#include "QGCTabbedInfoView.h"
#include "UASRawStatusView.h"
#include "PrimaryFlightDisplay.h"
#include <apmtoolbar.h>
#include <ApmHardwareConfig.h>
#include <ApmSoftwareConfig.h>
#include <QGCConfigView.h>
#include "SerialSettingsDialog.h"
#include "terminalconsole.h"
#include "menuactionhelper.h"

// Add support for the MAVLink generator UI if it's been requested.
#ifdef QGC_MAVGEN_ENABLED
#include "XMLCommProtocolWidget.h"
#endif

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

MainWindow* MainWindow::instance_mode(QSplashScreen* screen, enum MainWindow::CUSTOM_MODE mode)
{
    static MainWindow* _instance = 0;
    if (_instance == 0)
    {
        _instance = new MainWindow();
        _instance->setCustomMode(mode);
        if (screen)
        {
            connect(_instance, SIGNAL(initStatusChanged(QString,int,QColor)),
                    screen, SLOT(showMessage(QString,int,QColor)));
        }
        _instance->init();
    }
    return _instance;
}

MainWindow* MainWindow::instance(QSplashScreen* screen)
{
    return instance_mode(screen, CUSTOM_MODE_UNCHANGED);
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
    mavlink(new MAVLinkProtocol()),
    centerStackActionGroup(new QActionGroup(this)),
    darkStyleFileName(defaultDarkStyle),
    lightStyleFileName(defaultLightStyle),
    autoReconnect(false),
    simulationLink(NULL),
    lowPowerMode(false),
    customMode(CUSTOM_MODE_NONE),
    menuActionHelper(new MenuActionHelper())
{
    this->setAttribute(Qt::WA_DeleteOnClose);
    connect(menuActionHelper, SIGNAL(needToShowDockWidget(QString,bool)),SLOT(showDockWidget(QString,bool)));
    //TODO:  move protocol outside UI
    connect(mavlink, SIGNAL(protocolStatusMessage(QString,QString)), this, SLOT(showCriticalMessage(QString,QString)), Qt::QueuedConnection);
    loadSettings();
}

void MainWindow::init()
{

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
        menuActionHelper->setAdvancedMode(settings.value("ADVANCED_MODE").toBool());
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
    menuActionHelper->setMenu(ui.menuTools);
    
    // Qt 4 on Ubuntu does place the native menubar correctly so on Linux we revert back to in-window menu bar.
#ifdef Q_OS_LINUX
    menuBar()->setNativeMenuBar(false);
#endif

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
    if (!(getCustomMode() == CUSTOM_MODE_APM)) {
        toolBar = new QGCToolBar(this);
        this->addToolBar(toolBar);

        ui.actionHardwareConfig->setText(tr("Config"));

        // Add actions for average users (displayed next to each other)
        QList<QAction*> actions;
        actions << ui.actionFlightView;
        actions << ui.actionMissionView;
        actions << ui.actionHardwareConfig;
        toolBar->setPerspectiveChangeActions(actions);

        // Add actions for advanced users (displayed in dropdown under "advanced")
        QList<QAction*> advancedActions;
        advancedActions << ui.actionSimulation_View;
        advancedActions << ui.actionEngineersView;

        toolBar->setPerspectiveChangeAdvancedActions(advancedActions);
    }

    customStatusBar = new QGCStatusBar(this);
    setStatusBar(customStatusBar);
    statusBar()->setSizeGripEnabled(true);

    emit initStatusChanged(tr("Building common widgets."), Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));

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

    if (getCustomMode() == CUSTOM_MODE_APM) {
        // Add the APM 'toolbar'

        APMToolBar *apmToolBar = new APMToolBar(this);
        apmToolBar->setFlightViewAction(ui.actionFlightView);
        apmToolBar->setFlightPlanViewAction(ui.actionMissionView);
        apmToolBar->setHardwareViewAction(ui.actionHardwareConfig);
        apmToolBar->setSoftwareViewAction(ui.actionSoftwareConfig);
        apmToolBar->setSimulationViewAction(ui.actionSimulation_View);
        apmToolBar->setTerminalViewAction(ui.actionTerminalView);

        QDockWidget *widget = new QDockWidget(tr("APM Tool Bar"),this);
        widget->setWidget(apmToolBar);
        widget->setMinimumHeight(72);
        widget->setMaximumHeight(72);
        widget->setMinimumWidth(1024);
        widget->setFeatures(QDockWidget::NoDockWidgetFeatures);
        widget->setTitleBarWidget(new QWidget(this)); // Disables the title bar
    //    /*widget*/->setStyleSheet("QDockWidget { border: 0px solid #FFFFFF; border-radius: 0px; border-bottom: 0px;}");
        this->addDockWidget(Qt::TopDockWidgetArea, widget);
    }

    // Connect user interface devices
    emit initStatusChanged(tr("Initializing joystick interface"), Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));
    joystickWidget = 0;
    joystick = new JoystickInput();

#ifdef QGC_MOUSE_ENABLED_WIN
    emit initStatusChanged(tr("Initializing 3D mouse interface"), Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));

    mouseInput = new Mouse3DInput(this);
    mouse = new Mouse6dofInput(mouseInput);
#endif //QGC_MOUSE_ENABLED_WIN

#if QGC_MOUSE_ENABLED_LINUX
    emit initStatusChanged(tr("Initializing 3D mouse interface"), Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));

    mouse = new Mouse6dofInput(this);
    connect(this, SIGNAL(x11EventOccured(XEvent*)), mouse, SLOT(handleX11Event(XEvent*)));
#endif //QGC_MOUSE_ENABLED_LINUX

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
    if (simulationLink)
    {
        delete simulationLink;
        simulationLink = NULL;
    }
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
    delete debugConsole;
    delete menuActionHelper;
    for (int i=0;i<commsWidgetList.size();i++)
    {
        commsWidgetList[i]->deleteLater();
    }
}

void MainWindow::resizeEvent(QResizeEvent * event)
{
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

            // Load dock widget location (default is bottom)
            Qt::DockWidgetArea location = tool->getDockWidgetArea(currentView);

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
            case VIEW_GOOGLEEARTH:
                dock = createDockWidget(googleEarthView,tool,tool->getTitle(),tool->objectName(),(VIEW_SECTIONS)view,location);
                break;
            case VIEW_LOCAL3D:
                dock = createDockWidget(local3DView,tool,tool->getTitle(),tool->objectName(),(VIEW_SECTIONS)view,location);
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
    // Add generic MAVLink decoder
    mavlinkDecoder = new MAVLinkDecoder(mavlink, this);
    connect(mavlinkDecoder, SIGNAL(valueChanged(int,QString,QString,QVariant,quint64)),
                      this, SIGNAL(valueChanged(int,QString,QString,QVariant,quint64)));

    // Log player
    logPlayer = new QGCMAVLinkLogPlayer(mavlink, customStatusBar);
    customStatusBar->setLogPlayer(logPlayer);

    // Center widgets
    if (!plannerView)
    {
        plannerView = new SubMainWindow(this);
        plannerView->setObjectName("VIEW_MISSION");
        plannerView->setCentralWidget(new QGCMapTool(this));
        addToCentralStackedWidget(plannerView, VIEW_MISSION, "Maps");
    }

    //pilotView (aka Flight or Mission View)
    if (!pilotView)
    {
        pilotView = new SubMainWindow(this);
        pilotView->setObjectName("VIEW_FLIGHT");
        pilotView->setCentralWidget(new QGCMapTool(this));
        addToCentralStackedWidget(pilotView, VIEW_FLIGHT, "Pilot");
    }

    if (getCustomMode() == CUSTOM_MODE_APM) {
        if (!configView)
        {
            configView = new SubMainWindow(this);
            configView->setObjectName("VIEW_HARDWARE_CONFIG");
            configView->setCentralWidget(new ApmHardwareConfig(this));
            addToCentralStackedWidget(configView, VIEW_HARDWARE_CONFIG, "Hardware");

        }
        if (!softwareConfigView)
        {
            softwareConfigView = new SubMainWindow(this);
            softwareConfigView->setObjectName("VIEW_SOFTWARE_CONFIG");
            softwareConfigView->setCentralWidget(new ApmSoftwareConfig(this));
            addToCentralStackedWidget(softwareConfigView, VIEW_SOFTWARE_CONFIG, "Software");
        }
        if (!terminalView)
        {
            terminalView = new SubMainWindow(this);
            terminalView->setObjectName("VIEW_TERMINAL");
            TerminalConsole *terminalConsole = new TerminalConsole(this);
            terminalView->setCentralWidget(terminalConsole);
            addToCentralStackedWidget(terminalView, VIEW_TERMINAL, tr("Terminal View"));
        }
    } else {
        if (!configView)
        {
            configView = new SubMainWindow(this);
            configView->setObjectName("VIEW_HARDWARE_CONFIG");
            configView->setCentralWidget(new QGCConfigView(this));
            addToCentralStackedWidget(configView, VIEW_HARDWARE_CONFIG, "Config");
        }
    }

    if (!engineeringView)
    {
        engineeringView = new SubMainWindow(this);
        engineeringView->setObjectName("VIEW_ENGINEER");
        engineeringView->setCentralWidget(new QGCDataPlot2D(this));
        addToCentralStackedWidget(engineeringView, VIEW_ENGINEER, tr("Logfile Plot"));
    }

// Add the MAVLink generator UI if it's been requested.
#ifdef QGC_MAVGEN_ENABLED
    if (!mavlinkView)
    {
        mavlinkView = new SubMainWindow(this);
        mavlinkView->setObjectName("VIEW_MAVLINK");
        mavlinkView->setCentralWidget(new XMLCommProtocolWidget(this));
        addToCentralStackedWidget(mavlinkView, VIEW_MAVLINK, tr("Mavlink Generator"));
    }
#endif

#if QGC_GOOGLE_EARTH_ENABLED
    if (!googleEarthView)
    {
        googleEarthView = new SubMainWindow(this);
        googleEarthView->setObjectName("VIEW_GOOGLEEARTH");
        googleEarthView->setCentralWidget(new QGCGoogleEarthView(this));
        addToCentralStackedWidget(googleEarthView, VIEW_GOOGLEEARTH, tr("Google Earth View"));
    }
#endif

#ifdef QGC_OSG_ENABLED
    if (!local3DView)
    {
        q3DWidget = Q3DWidgetFactory::get("PIXHAWK", this);
        q3DWidget->setObjectName("VIEW_3DWIDGET");

        local3DView = new SubMainWindow(this);
        local3DView->setObjectName("VIEW_LOCAL3D");
        local3DView->setCentralWidget(q3DWidget);
        addToCentralStackedWidget(local3DView, VIEW_LOCAL3D, tr("Local 3D View"));
    }
#endif

    if (!simView)
    {
        simView = new SubMainWindow(this);
        simView->setObjectName("VIEW_SIMULATOR");
        simView->setCentralWidget(new QGCMapTool(this));
        addToCentralStackedWidget(simView, VIEW_SIMULATION, tr("Simulation View"));
    }

    // Dock widgets
    createDockWidget(simView,new UASControlWidget(this),tr("Control"),"UNMANNED_SYSTEM_CONTROL_DOCKWIDGET",VIEW_SIMULATION,Qt::LeftDockWidgetArea);

    createDockWidget(plannerView,new UASListWidget(this),tr("Unmanned Systems"),"UNMANNED_SYSTEM_LIST_DOCKWIDGET",VIEW_MISSION,Qt::LeftDockWidgetArea);
    createDockWidget(plannerView,new QGCWaypointListMulti(this),tr("Mission Plan"),"WAYPOINT_LIST_DOCKWIDGET",VIEW_MISSION,Qt::BottomDockWidgetArea);

    createDockWidget(simView,new QGCWaypointListMulti(this),tr("Mission Plan"),"WAYPOINT_LIST_DOCKWIDGET",VIEW_SIMULATION,Qt::BottomDockWidgetArea);
    createDockWidget(engineeringView,new QGCMAVLinkInspector(mavlink,this),tr("MAVLink Inspector"),"MAVLINK_INSPECTOR_DOCKWIDGET",VIEW_ENGINEER,Qt::RightDockWidgetArea);

    createDockWidget(engineeringView,new ParameterInterface(this),tr("Onboard Parameters"),"PARAMETER_INTERFACE_DOCKWIDGET",VIEW_ENGINEER,Qt::RightDockWidgetArea);
    createDockWidget(simView,new ParameterInterface(this),tr("Onboard Parameters"),"PARAMETER_INTERFACE_DOCKWIDGET",VIEW_SIMULATION,Qt::RightDockWidgetArea);

    menuActionHelper->createToolAction(tr("Status Details"), "UAS_STATUS_DETAILS_DOCKWIDGET");

    {
        if (!debugConsole)
        {
            debugConsole = new DebugConsole();
            debugConsole->setWindowTitle("Communications Console");
            debugConsole->hide();
            QAction* tempAction = ui.menuTools->addAction(tr("Communication Console"));
            tempAction->setCheckable(true);
            connect(tempAction,SIGNAL(triggered(bool)),debugConsole,SLOT(setShown(bool)));
        }
    }
    createDockWidget(simView,new HSIDisplay(this),tr("Horizontal Situation"),"HORIZONTAL_SITUATION_INDICATOR_DOCKWIDGET",VIEW_SIMULATION,Qt::BottomDockWidgetArea);

    menuActionHelper->createToolAction(tr("Flight Display"), "HEAD_DOWN_DISPLAY_1_DOCKWIDGET");
    menuActionHelper->createToolAction(tr("Actuator Status"), "HEAD_DOWN_DISPLAY_2_DOCKWIDGET");
    menuActionHelper->createToolAction(tr("Radio Control"));

    createDockWidget(engineeringView,new HUD(320,240,this),tr("Video Downlink"),"HEAD_UP_DISPLAY_DOCKWIDGET",VIEW_ENGINEER,Qt::RightDockWidgetArea,QSize(this->width()/1.5,0));

    createDockWidget(simView,new PrimaryFlightDisplay(320,240,this),tr("Primary Flight Display"),"PRIMARY_FLIGHT_DISPLAY_DOCKWIDGET",VIEW_SIMULATION,Qt::RightDockWidgetArea,QSize(this->width()/1.5,0));
    createDockWidget(pilotView,new PrimaryFlightDisplay(320,240,this),tr("Primary Flight Display"),"PRIMARY_FLIGHT_DISPLAY_DOCKWIDGET",VIEW_FLIGHT,Qt::LeftDockWidgetArea,QSize(this->width()/1.8,0));

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
}

void MainWindow::addTool(SubMainWindow *parent,VIEW_SECTIONS view,QDockWidget* widget, const QString& title, Qt::DockWidgetArea area)
{
    menuActionHelper->createToolActionForCustomDockWidget(title, widget->objectName(), widget, view);
    parent->addDockWidget(area,widget);
}

QDockWidget* MainWindow::createDockWidget(QWidget *subMainWindowParent,QWidget *child,const QString& title,const QString& objectName,VIEW_SECTIONS view,Qt::DockWidgetArea area,const QSize& minSize)
{
    SubMainWindow *parent = qobject_cast<SubMainWindow*>(subMainWindowParent);
    Q_ASSERT(parent);
    QDockWidget* dockWidget = menuActionHelper->createDockWidget(title, objectName);
    child->setObjectName(objectName);
    dockWidget->setWidget(child); //Set child objectName before setting dockwidget, since the dock widget might react to object name changes
    connect(child, SIGNAL(destroyed()), dockWidget, SLOT(deleteLater()));  //Our dockwidget only has only child widget, so kill the dock widget if the child is deleted

    if (minSize.height() >= 0)
        dockWidget->setMinimumHeight(minSize.height());
    if (minSize.width() >= 0)
        dockWidget->setMinimumWidth(minSize.width());
    addTool(parent,view,dockWidget,title,area);
    return dockWidget;
}

void MainWindow::showDockWidget(const QString& name, bool show)
{
    QDockWidget *dockWidget = menuActionHelper->getDockWidget(currentView, name);
    if(dockWidget)
        dockWidget->setVisible(show);
    else if (show)
        loadDockWidget(name);
}

void MainWindow::loadDockWidget(const QString& name)
{
    if(menuActionHelper->containsDockWidget(currentView, name))
        return;
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
    }
    else if (name == "HORIZONTAL_SITUATION_INDICATOR_DOCKWIDGET")
    {
        createDockWidget(centerStack->currentWidget(),new HSIDisplay(this),tr("Horizontal Situation"),"HORIZONTAL_SITUATION_INDICATOR_DOCKWIDGET",currentView,Qt::BottomDockWidgetArea);
    }
    else if (name == "HEAD_DOWN_DISPLAY_1_DOCKWIDGET")
    {
        QStringList acceptList;
        acceptList.append("-3.3,ATTITUDE.roll,rad,+3.3,s");
        acceptList.append("-3.3,ATTITUDE.pitch,deg,+3.3,s");
        acceptList.append("-3.3,ATTITUDE.yaw,deg,+3.3,s");
        HDDisplay *hddisplay = new HDDisplay(acceptList,"Flight Display",this);
        hddisplay->addSource(mavlinkDecoder);
        createDockWidget(centerStack->currentWidget(),hddisplay,tr("Flight Display"),"HEAD_DOWN_DISPLAY_1_DOCKWIDGET",currentView,Qt::RightDockWidgetArea);
    }
    else if (name == "HEAD_DOWN_DISPLAY_2_DOCKWIDGET")
    {
        QStringList acceptList;
        acceptList.append("0,RAW_PRESSURE.pres_abs,hPa,65500");
        HDDisplay *hddisplay = new HDDisplay(acceptList,"Actuator Status",this);
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
        createDockWidget(centerStack->currentWidget(),new PrimaryFlightDisplay(320,240,this),tr("Primary Flight Display"),"PRIMARY_FLIGHT_DISPLAY_DOCKWIDGET",currentView,Qt::RightDockWidgetArea);
    }
    else if (name == "HEAD_UP_DISPLAY_DOCKWIDGET")
    {
        createDockWidget(centerStack->currentWidget(),new HUD(320,240,this),tr("Head Up Display"),"HEAD_UP_DISPLAY_DOCKWIDGET",currentView,Qt::RightDockWidgetArea);
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

void MainWindow::addToCentralStackedWidget(QWidget* widget, VIEW_SECTIONS viewSection, const QString& title)
{
    Q_UNUSED(viewSection);
    Q_UNUSED(title);
    Q_ASSERT(widget->objectName().length() != 0);

    // Check if this widget already has been added
    if (centerStack->indexOf(widget) == -1)
    {
        centerStack->addWidget(widget);
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
        QGCHilConfiguration* hconf = new QGCHilConfiguration(mav, this);
        QString hilDockName = tr("HIL Config %1").arg(uas->getUASName());
        QString hilDockObjectName = QString("HIL_CONFIG_%1").arg(uas->getUASName().toUpper().replace(' ','_'));
        QDockWidget* hilDock = createDockWidget(simView, hconf,hilDockName, hilDockObjectName,VIEW_SIMULATION,Qt::LeftDockWidgetArea);
        hilDocks.insert(mav->getUASID(), hilDock);
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
    if (QGCToolWidget::instances()->isEmpty())
    {
        // This is the first widget
        ui.menuTools->addSeparator();
    }
    QString objectName;
    int customToolIndex = 0;
    //Find the next unique object name that we can use
    do {
        ++customToolIndex;
        objectName = QString("CUSTOM_TOOL_%1").arg(customToolIndex) + "DOCK";
    } while(QGCToolWidget::instances()->contains(objectName));

    QString title = tr("Custom Tool %1").arg(customToolIndex );

    QGCToolWidget* tool = new QGCToolWidget(objectName, title);
    createDockWidget(centerStack->currentWidget(),tool,title,objectName,currentView,Qt::BottomDockWidgetArea);

    QSettings settings;
    settings.beginGroup("QGC_MAINWINDOW");
    settings.setValue(QString("TOOL_PARENT_") + tool->objectName(),currentView);
    settings.endGroup();
}

void MainWindow::loadCustomWidget()
{
    QString widgetFileExtension(".qgw");
    QString fileName = QFileDialog::getOpenFileName(this, tr("Specify Widget File Name"), QDesktopServices::storageLocation(QDesktopServices::DesktopLocation), tr("QGroundControl Widget (*%1);;").arg(widgetFileExtension));
    if (fileName != "") loadCustomWidget(fileName);
}
void MainWindow::loadCustomWidget(const QString& fileName, int view)
{
    QGCToolWidget* tool = new QGCToolWidget("", "", this);
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
            menuActionHelper->createToolAction(tool->getTitle(), tool->objectName()+"DOCK");
            tool->deleteLater();
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
    QGCToolWidget* tool = new QGCToolWidget("", "", this);
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
            QAction *action = menuActionHelper->createToolAction(tool->getTitle(), tool->objectName()+"DOCK");
            ui.menuTools->addAction(action);
            tool->deleteLater();
            }
            break;
        }


        settings.endGroup();
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
    settings.sync();
    customMode = static_cast<enum MainWindow::CUSTOM_MODE>(settings.value("QGC_CUSTOM_MODE", (unsigned int)MainWindow::CUSTOM_MODE_NONE).toInt());
    settings.beginGroup("QGC_MAINWINDOW");
    autoReconnect = settings.value("AUTO_RECONNECT", autoReconnect).toBool();
    currentStyle = (QGC_MAINWINDOW_STYLE)settings.value("CURRENT_STYLE", currentStyle).toInt();
    darkStyleFileName = settings.value("DARK_STYLE_FILENAME", darkStyleFileName).toString();
    lightStyleFileName = settings.value("LIGHT_STYLE_FILENAME", lightStyleFileName).toString();
    lowPowerMode = settings.value("LOW_POWER_MODE", lowPowerMode).toBool();
    bool dockWidgetTitleBarEnabled = settings.value("DOCK_WIDGET_TITLEBARS",menuActionHelper->dockWidgetTitleBarsEnabled()).toBool();
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
    settings.setValue("QGC_CUSTOM_MODE", (int)customMode);
    QGCToolWidget::storeWidgetsToSettings(settings);
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
    menuActionHelper->setDockWidgetTitleBarsEnabled(enabled);
    QSettings settings;
    settings.beginGroup("QGC_MAINWINDOW");
    settings.setValue("DOCK_WIDGET_TITLEBARS",enabled);
    settings.endGroup();
    settings.sync();
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
    //perspectives->addAction(ui.actionConfiguration_2);
    perspectives->addAction(ui.actionHardwareConfig);
    if (getCustomMode() == CUSTOM_MODE_APM) {
        perspectives->addAction(ui.actionSoftwareConfig);
    }
    perspectives->addAction(ui.actionTerminalView);
    perspectives->addAction(ui.actionUnconnectedView);
    perspectives->addAction(ui.actionGoogleEarthView);
    perspectives->addAction(ui.actionLocal3DView);
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
    if (currentView == VIEW_HARDWARE_CONFIG)
    {
        ui.actionHardwareConfig->setChecked(true);
        ui.actionHardwareConfig->activate(QAction::Trigger);
    }
    if (currentView == VIEW_SOFTWARE_CONFIG)
    {
        ui.actionSoftwareConfig->setChecked(true);
        ui.actionSoftwareConfig->activate(QAction::Trigger);
    }
    if (currentView == VIEW_TERMINAL)
    {
        ui.actionTerminalView->setChecked(true);
        ui.actionTerminalView->activate(QAction::Trigger);
    }
    if (currentView == VIEW_GOOGLEEARTH)
    {
        ui.actionGoogleEarthView->setChecked(true);
        ui.actionGoogleEarthView->activate(QAction::Trigger);
    }
    if (currentView == VIEW_LOCAL3D)
    {
        ui.actionLocal3DView->setChecked(true);
        ui.actionLocal3DView->activate(QAction::Trigger);
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
    ui.actionAdvanced_Mode->setChecked(menuActionHelper->isAdvancedMode());
    connect(ui.actionAdvanced_Mode,SIGNAL(toggled(bool)),this,SLOT(setAdvancedMode(bool)));

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
    connect(ui.actionHardwareConfig,SIGNAL(triggered()),this,SLOT(loadHardwareConfigView()));
    connect(ui.actionGoogleEarthView, SIGNAL(triggered()), this, SLOT(loadGoogleEarthView()));
    connect(ui.actionLocal3DView, SIGNAL(triggered()), this, SLOT(loadLocal3DView()));

    if (getCustomMode() == CUSTOM_MODE_APM) {
        connect(ui.actionSoftwareConfig,SIGNAL(triggered()),this,SLOT(loadSoftwareConfigView()));
        connect(ui.actionTerminalView,SIGNAL(triggered()),this,SLOT(loadTerminalView()));
    }

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

    connect(ui.actionSimulate, SIGNAL(triggered(bool)), this, SLOT(simulateLink(bool)));
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

LinkInterface* MainWindow::addLink()
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

    return link;
}


bool MainWindow::configLink(LinkInterface *link)
{
    // Go searching for this link's configuration window
    QList<QAction*> actions = ui.menuNetwork->actions();

    bool found(false);

    const int32_t& linkIndex(LinkManager::instance()->getLinks().indexOf(link));
    const int32_t& linkID(LinkManager::instance()->getLinks()[linkIndex]->getId());

    foreach (QAction* action, actions)
    {
        if (action->data().toInt() == linkID)
        { // LinkManager::instance()->getLinks().indexOf(link)
            found = true;
            action->trigger(); // Show the Link Config Dialog
        }
    }

    return found;
}

void MainWindow::addLink(LinkInterface *link)
{

    qDebug() << "ADD LINK CALLED FROM SOMEWHERE";

    // IMPORTANT! KEEP THESE TWO LINES
    // THEY MAKE SURE THE LINK IS PROPERLY REGISTERED
    // BEFORE LINKING THE UI AGAINST IT
    // Register (does nothing if already registered)
    LinkManager::instance()->add(link);

    if (mavlink) {
        qDebug() << "MAVLINK OK";
    } else {
        qDebug() << "MAVLINK FAIL";
    }

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

    if (!found)
    {
        CommConfigurationWindow* commWidget = new CommConfigurationWindow(link, mavlink, this);
        commsWidgetList.append(commWidget);
        connect(commWidget,SIGNAL(destroyed(QObject*)),this,SLOT(commsWidgetDestroyed(QObject*)));
        QAction* action = commWidget->getAction();
        ui.menuNetwork->addAction(action);

        // Error handling
        connect(link, SIGNAL(communicationError(QString,QString)), this, SLOT(showCriticalMessage(QString,QString)), Qt::QueuedConnection);
    }
}

void MainWindow::simulateLink(bool simulate) {
    if (!simulationLink)
        simulationLink = new MAVLinkSimulationLink(":/demo-log.txt");
    simulationLink->connectLink(simulate);
}

//void MainWindow::configLink(LinkInterface *link)
//{

//}
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
    connect(uas, SIGNAL(valueChanged(int,QString,QString,QVariant,quint64)), this, SIGNAL(valueChanged(int,QString,QString,QVariant,quint64)));

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
        case VIEW_HARDWARE_CONFIG:
            centerStack->setCurrentWidget(configView);
            break;
        case VIEW_SOFTWARE_CONFIG:
            if (softwareConfigView)
                centerStack->setCurrentWidget(softwareConfigView);
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
//        case VIEW_FIRMWAREUPDATE:
//            centerStack->setCurrentWidget(firmwareUpdateWidget);
//            break;
        case VIEW_MISSION:
            centerStack->setCurrentWidget(plannerView);
            break;

        case VIEW_SIMULATION:
            centerStack->setCurrentWidget(simView);
            break;

        case VIEW_TERMINAL:
            centerStack->setCurrentWidget(terminalView);
            break;
        case VIEW_GOOGLEEARTH:
            centerStack->setCurrentWidget(googleEarthView);
            break;
        case VIEW_LOCAL3D:
            centerStack->setCurrentWidget(local3DView);
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
void MainWindow::setAdvancedMode(bool isAdvancedMode)
{
    menuActionHelper->setAdvancedMode(isAdvancedMode);
    ui.actionAdvanced_Mode->setChecked(isAdvancedMode);
    settings.setValue("ADVANCED_MODE",isAdvancedMode);
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
void MainWindow::loadHardwareConfigView()
{
    if (currentView != VIEW_HARDWARE_CONFIG)
    {
        storeViewState();
        currentView = VIEW_HARDWARE_CONFIG;
        ui.actionHardwareConfig->setChecked(true);
        loadViewState();
    }
}

void MainWindow::loadSoftwareConfigView()
{
    if (currentView != VIEW_SOFTWARE_CONFIG)
    {
        storeViewState();
        currentView = VIEW_SOFTWARE_CONFIG;
        ui.actionSoftwareConfig->setChecked(true);
        loadViewState();
    }
}

void MainWindow::loadTerminalView()
{
    if (currentView != VIEW_TERMINAL)
    {
        storeViewState();
        currentView = VIEW_TERMINAL;
        ui.actionTerminalView->setChecked(true);
        loadViewState();
    }
}

void MainWindow::loadGoogleEarthView()
{
    if (currentView != VIEW_GOOGLEEARTH)
    {
        storeViewState();
        currentView = VIEW_GOOGLEEARTH;
        ui.actionGoogleEarthView->setChecked(true);
        loadViewState();
    }
}

void MainWindow::loadLocal3DView()
{
    if (currentView != VIEW_LOCAL3D)
    {
        storeViewState();
        currentView = VIEW_LOCAL3D;
        ui.actionLocal3DView->setChecked(true);
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

QList<QAction*> MainWindow::listLinkMenuActions()
{
    return ui.menuNetwork->actions();
}

bool MainWindow::dockWidgetTitleBarsEnabled() const
{
    return menuActionHelper->dockWidgetTitleBarsEnabled();
}

#ifdef QGC_MOUSE_ENABLED_LINUX
bool MainWindow::x11Event(XEvent *event)
{
    emit x11EventOccured(event);
    //qDebug("XEvent occured...");
    return false;
}
#endif // QGC_MOUSE_ENABLED_LINUX
