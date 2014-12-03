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
#include <QDebug>
#include <QTimer>
#include <QHostInfo>
#include <QSplashScreen>
#include <QGCHilLink.h>
#include <QGCHilConfiguration.h>
#include <QGCHilFlightGearConfiguration.h>
#include <QQuickView>
#include <QDesktopWidget>

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
#include "SettingsDialog.h"
#include "QGCMapTool.h"
#include "MAVLinkDecoder.h"
#include "QGCMAVLinkMessageSender.h"
#include "QGCRGBDView.h"
#include "QGCStatusBar.h"
#include "UASQuickView.h"
#include "QGCDataPlot2D.h"
#include "Linecharts.h"
#include "QGCTabbedInfoView.h"
#include "UASRawStatusView.h"
#include "PrimaryFlightDisplay.h"
#include "SetupView.h"
#include "SerialSettingsDialog.h"
#include "terminalconsole.h"
#include "menuactionhelper.h"
#include "QGCUASFileViewMulti.h"
#include "QGCApplication.h"
#include "QGCFileDialog.h"
#include "QGCMessageBox.h"

#ifdef QGC_OSG_ENABLED
#include "Q3DWidgetFactory.h"
#endif

#include "LogCompressor.h"

static MainWindow* _instance = NULL;   ///< @brief MainWindow singleton

// Set up some constants
const QString MainWindow::defaultDarkStyle = ":files/styles/style-dark.css";
const QString MainWindow::defaultLightStyle = ":files/styles/style-light.css";

MainWindow* MainWindow::_create(QSplashScreen* splashScreen, enum MainWindow::CUSTOM_MODE mode)
{
    Q_ASSERT(_instance == NULL);
    
    new MainWindow(splashScreen, mode);
    
    // _instance is set in constructor
    Q_ASSERT(_instance);

    return _instance;
}

MainWindow* MainWindow::instance(void)
{
    return _instance;
}

void MainWindow::deleteInstance(void)
{
    delete this;
}

/// @brief Private constructor for MainWindow. MainWindow singleton is only ever created
///         by MainWindow::_create method. Hence no other code should have access to
///         constructor.
MainWindow::MainWindow(QSplashScreen* splashScreen, enum MainWindow::CUSTOM_MODE mode) :
    currentView(VIEW_FLIGHT),
    currentStyle(QGC_MAINWINDOW_STYLE_DARK),
    mavlink(new MAVLinkProtocol()),
    centerStackActionGroup(new QActionGroup(this)),
    autoReconnect(false),
    simulationLink(NULL),
    lowPowerMode(false),
    customMode(mode),
    menuActionHelper(new MenuActionHelper()),
    _splashScreen(splashScreen)
{
    Q_ASSERT(_instance == NULL);
    _instance = this;
    
    if (splashScreen) {
        connect(this, &MainWindow::initStatusChanged, splashScreen, &QSplashScreen::showMessage);
    }
    
    this->setAttribute(Qt::WA_DeleteOnClose);
    connect(menuActionHelper, SIGNAL(needToShowDockWidget(QString,bool)),SLOT(showDockWidget(QString,bool)));
    
    connect(mavlink, SIGNAL(protocolStatusMessage(const QString&, const QString&)), this, SLOT(showCriticalMessage(const QString&, const QString&)));
    connect(mavlink, SIGNAL(saveTempFlightDataLog(QString)), this, SLOT(_saveTempFlightDataLog(QString)));
    
    loadSettings();
    
    emit initStatusChanged(tr("Loading style"), Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));
    qApp->setStyle("plastique");
    loadStyle(currentStyle);

    if (settings.contains("ADVANCED_MODE"))
    {
        menuActionHelper->setAdvancedMode(settings.value("ADVANCED_MODE").toBool());
    }

    // Select the proper view. Default to the flight view or load the last one used if it's supported.
    if (!settings.contains("CURRENT_VIEW"))
    {
        settings.setValue("CURRENT_VIEW", currentView);
    }
    else
    {
        VIEW_SECTIONS currentViewCandidate = (VIEW_SECTIONS) settings.value("CURRENT_VIEW", currentView).toInt();
        switch (currentViewCandidate) {
            case VIEW_ENGINEER:
            case VIEW_MISSION:
            case VIEW_FLIGHT:
            case VIEW_SIMULATION:
            case VIEW_SETUP:
            case VIEW_TERMINAL:

// And only re-load views if they're supported with the current QGC build
#ifdef QGC_OSG_ENABLED
            case VIEW_LOCAL3D:
#endif
#ifdef QGC_GOOGLE_EARTH_ENABLED
            case VIEW_GOOGLEEARTH:
#endif

                currentView = currentViewCandidate;
            default:
                // If an invalid view candidate was found in the settings file, just use the default view and re-save.
                settings.setValue("CURRENT_VIEW", currentView);
                break;
        }
    }

    emit initStatusChanged(tr("Setting up user interface"), Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));

    // Setup user interface
    ui.setupUi(this);
    menuActionHelper->setMenu(ui.menuTools);

    // Set dock options
    setDockOptions(AnimatedDocks | AllowTabbedDocks | AllowNestedDocks);

    configureWindowName();

    // Setup corners
    setCorner(Qt::BottomRightCorner, Qt::BottomDockWidgetArea);

    // Qt 4 on Ubuntu does place the native menubar correctly so on Linux we revert back to in-window menu bar.
    // TODO: Check that this is still necessary on Qt5 on Ubuntu
#ifdef Q_OS_LINUX
    menuBar()->setNativeMenuBar(false);
#endif

    // Setup UI state machines
    centerStackActionGroup->setExclusive(true);

    centerStack = new QStackedWidget(this);
    setCentralWidget(centerStack);


    // Load Toolbar
    toolBar = new QGCToolBar(this);
    this->addToolBar(toolBar);

    // Add the perspectives to the toolbar
    QList<QAction*> actions;
    actions << ui.actionSetup;
    actions << ui.actionMissionView;
    actions << ui.actionFlightView;
    actions << ui.actionEngineersView;
    toolBar->setPerspectiveChangeActions(actions);

    // Add actions for advanced users (displayed in dropdown under "advanced")
    QList<QAction*> advancedActions;
    advancedActions << ui.actionGoogleEarthView;
    advancedActions << ui.actionLocal3DView;
    advancedActions << ui.actionTerminalView;
    advancedActions << ui.actionSimulationView;
    toolBar->setPerspectiveChangeAdvancedActions(advancedActions);

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

    // Connect user interface devices
    emit initStatusChanged(tr("Initializing joystick interface"), Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));
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
        LinkManager* linkMgr = LinkManager::instance();
        Q_ASSERT(linkMgr);
        
        SerialLink* link = new SerialLink();
        
        // Add to registry
        linkMgr->add(link);
        linkMgr->addProtocol(link, mavlink);
        linkMgr->connectLink(link);
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

    // Make sure the proper fullscreen/normal menu item is checked properly.
    if (isFullScreen())
    {
        ui.actionFullscreen->setChecked(true);
        ui.actionNormal->setChecked(false);
    }
    else
    {
        ui.actionFullscreen->setChecked(false);
        ui.actionNormal->setChecked(true);
    }

    // And that they will stay checked properly after user input
    QObject::connect(ui.actionFullscreen, SIGNAL(triggered()), this, SLOT(fullScreenActionItemCallback()));
    QObject::connect(ui.actionNormal, SIGNAL(triggered()), this,SLOT(normalActionItemCallback()));


    // Set OS dependent keyboard shortcuts for the main window, non OS dependent shortcuts are set in MainWindow.ui
#ifdef Q_OS_MACX
    ui.actionSetup->setShortcut(QApplication::translate("MainWindow", "Meta+1", 0));
    ui.actionMissionView->setShortcut(QApplication::translate("MainWindow", "Meta+2", 0));
    ui.actionFlightView->setShortcut(QApplication::translate("MainWindow", "Meta+3", 0));
    ui.actionEngineersView->setShortcut(QApplication::translate("MainWindow", "Meta+4", 0));
    ui.actionGoogleEarthView->setShortcut(QApplication::translate("MainWindow", "Meta+5", 0));
    ui.actionLocal3DView->setShortcut(QApplication::translate("MainWindow", "Meta+6", 0));
    ui.actionTerminalView->setShortcut(QApplication::translate("MainWindow", "Meta+7", 0));
    ui.actionSimulationView->setShortcut(QApplication::translate("MainWindow", "Meta+8", 0));
    ui.actionFullscreen->setShortcut(QApplication::translate("MainWindow", "Meta+Return", 0));
#else
    ui.actionSetup->setShortcut(QApplication::translate("MainWindow", "Ctrl+1", 0));
    ui.actionMissionView->setShortcut(QApplication::translate("MainWindow", "Ctrl+2", 0));
    ui.actionFlightView->setShortcut(QApplication::translate("MainWindow", "Ctrl+3", 0));
    ui.actionEngineersView->setShortcut(QApplication::translate("MainWindow", "Ctrl+4", 0));
    ui.actionGoogleEarthView->setShortcut(QApplication::translate("MainWindow", "Ctrl+5", 0));
    ui.actionLocal3DView->setShortcut(QApplication::translate("MainWindow", "Ctrl+6", 0));
    ui.actionTerminalView->setShortcut(QApplication::translate("MainWindow", "Ctrl+7", 0));
    ui.actionSimulationView->setShortcut(QApplication::translate("MainWindow", "Ctrl+8", 0));
    ui.actionFullscreen->setShortcut(QApplication::translate("MainWindow", "Ctrl+Return", 0));
#endif

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
    delete menuActionHelper;
    for (int i=0;i<commsWidgetList.size();i++)
    {
        commsWidgetList[i]->deleteLater();
    }
    
    _instance = NULL;
}

void MainWindow::resizeEvent(QResizeEvent * event)
{
    QMainWindow::resizeEvent(event);
}

QString MainWindow::getWindowStateKey()
{
    if (UASManager::instance()->getActiveUAS())
    {
        return QString::number(currentView)+"_windowstate_" + QString::number(getCustomMode()) + "_" + UASManager::instance()->getActiveUAS()->getAutopilotTypeName();
    }
    else
        return QString::number(currentView)+"_windowstate_" + QString::number(getCustomMode());
}

QString MainWindow::getWindowGeometryKey()
{
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

            int view = settings.value(QString("TOOL_PARENT_") + tool->objectName(),-1).toInt();
            settings.endGroup();

            QDockWidget* dock;

            switch (view)
            {
            case VIEW_ENGINEER:
                dock = createDockWidget(engineeringView,tool,tool->getTitle(),tool->objectName(),(VIEW_SECTIONS)view,location);
                break;
            default: // Flight view is the default.
            case VIEW_FLIGHT:
                dock = createDockWidget(pilotView,tool,tool->getTitle(),tool->objectName(),(VIEW_SECTIONS)view,location);
                break;
            case VIEW_SIMULATION:
                dock = createDockWidget(simView,tool,tool->getTitle(),tool->objectName(),(VIEW_SECTIONS)view,location);
                break;
            case VIEW_MISSION:
                dock = createDockWidget(plannerView,tool,tool->getTitle(),tool->objectName(),(VIEW_SECTIONS)view,location);
                break;
            case VIEW_GOOGLEEARTH:
                dock = createDockWidget(googleEarthView,tool,tool->getTitle(),tool->objectName(),(VIEW_SECTIONS)view,location);
                break;
            case VIEW_LOCAL3D:
                dock = createDockWidget(local3DView,tool,tool->getTitle(),tool->objectName(),(VIEW_SECTIONS)view,location);
                break;
            }

            // XXX temporary "fix"
            dock->hide();
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

    // Initialize all of the views, if they haven't been already, and add their central widgets
    if (!plannerView)
    {
        plannerView = new SubMainWindow(this);
        plannerView->setObjectName("VIEW_MISSION");
        plannerView->setCentralWidget(new QGCMapTool(this));
        addToCentralStackedWidget(plannerView, VIEW_MISSION, "Maps");
    }
    if (!pilotView)
    {
        pilotView = new SubMainWindow(this);
        pilotView->setObjectName("VIEW_FLIGHT");
        pilotView->setCentralWidget(new PrimaryFlightDisplay(this));
        addToCentralStackedWidget(pilotView, VIEW_FLIGHT, "Pilot");
    }
    if (!terminalView)
    {
        terminalView = new SubMainWindow(this);
        terminalView->setObjectName("VIEW_TERMINAL");
        TerminalConsole *terminalConsole = new TerminalConsole(this);
        terminalView->setCentralWidget(terminalConsole);
        addToCentralStackedWidget(terminalView, VIEW_TERMINAL, tr("Terminal View"));
    }
    if (!setupView)
    {
        setupView = new SubMainWindow(this);
        setupView->setObjectName("VIEW_SETUP");
        setupView->setCentralWidget(new SetupView(this));
        addToCentralStackedWidget(setupView, VIEW_SETUP, "Setup");
    }
    if (!engineeringView)
    {
        engineeringView = new SubMainWindow(this);
        engineeringView->setObjectName("VIEW_ENGINEER");
        engineeringView->setCentralWidget(new QGCDataPlot2D(this));
        addToCentralStackedWidget(engineeringView, VIEW_ENGINEER, tr("Logfile Plot"));
    }
#ifdef QGC_GOOGLE_EARTH_ENABLED
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

    // Add dock widgets for the planner view
    createDockWidget(plannerView, new UASListWidget(this), tr("Unmanned Systems"), "UNMANNED_SYSTEM_LIST_DOCKWIDGET", VIEW_MISSION, Qt::LeftDockWidgetArea);
    createDockWidget(plannerView, new QGCWaypointListMulti(this), tr("Mission Plan"), "WAYPOINT_LIST_DOCKWIDGET", VIEW_MISSION, Qt::BottomDockWidgetArea);

    // Add dock widgets for the pilot view
    createDockWidget(pilotView, new DebugConsole(this), tr("Communications Console"), "COMMUNICATION_CONSOLE_DOCKWIDGET", VIEW_FLIGHT, Qt::LeftDockWidgetArea);
    QGCTabbedInfoView *infoview = new QGCTabbedInfoView(this);
    infoview->addSource(mavlinkDecoder);
    createDockWidget(pilotView, infoview, tr("Info View"), "UAS_INFO_INFOVIEW_DOCKWIDGET", VIEW_FLIGHT, Qt::LeftDockWidgetArea);

    // Add dock widgets for the simulation view
    createDockWidget(simView,new UASControlWidget(this),tr("Control"),"UNMANNED_SYSTEM_CONTROL_DOCKWIDGET",VIEW_SIMULATION,Qt::LeftDockWidgetArea);
    createDockWidget(simView,new QGCWaypointListMulti(this),tr("Mission Plan"),"WAYPOINT_LIST_DOCKWIDGET",VIEW_SIMULATION,Qt::BottomDockWidgetArea);
    createDockWidget(simView, new ParameterInterface(this), tr("Onboard Parameters"), "PARAMETER_INTERFACE_DOCKWIDGET", VIEW_SIMULATION, Qt::RightDockWidgetArea);
    createDockWidget(simView, new PrimaryFlightDisplay(this), tr("Primary Flight Display"), "PRIMARY_FLIGHT_DISPLAY_DOCKWIDGET", VIEW_SIMULATION, Qt::RightDockWidgetArea);

    // Add dock widgets for the engineering view
    createDockWidget(engineeringView, new QGCMAVLinkInspector(mavlink, this), tr("MAVLink Inspector"), "MAVLINK_INSPECTOR_DOCKWIDGET", VIEW_ENGINEER, Qt::RightDockWidgetArea);
    createDockWidget(engineeringView, new ParameterInterface(this), tr("Onboard Parameters"), "PARAMETER_INTERFACE_DOCKWIDGET", VIEW_ENGINEER, Qt::RightDockWidgetArea);
    createDockWidget(engineeringView, new QGCUASFileViewMulti(this), tr("Onboard Files"), "FILE_VIEW_DOCKWIDGET", VIEW_ENGINEER, Qt::RightDockWidgetArea);
    createDockWidget(engineeringView, new HUD(320, 240, this), tr("Video Downlink"), "HEAD_UP_DISPLAY_DOCKWIDGET", VIEW_ENGINEER, Qt::RightDockWidgetArea);

    // Add some extra widgets to the Tool Widgets menu
    menuActionHelper->createToolAction(tr("Map View"), "MAP_VIEW_DOCKWIDGET");
    menuActionHelper->createToolAction(tr("Status Details"), "UAS_STATUS_DETAILS_DOCKWIDGET");
    menuActionHelper->createToolAction(tr("Flight Display"), "HEAD_DOWN_DISPLAY_1_DOCKWIDGET");
    menuActionHelper->createToolAction(tr("Actuator Status"), "HEAD_DOWN_DISPLAY_2_DOCKWIDGET");

    // Add any custom widgets last to all menus and layouts
    buildCustomWidget();
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

void MainWindow::fullScreenActionItemCallback()
{
    ui.actionNormal->setChecked(false);
}

void MainWindow::normalActionItemCallback()
{
    ui.actionFullscreen->setChecked(false);
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
    else if (name == "FILE_VIEW_DOCKWIDGET")
    {
        createDockWidget(centerStack->currentWidget(),new QGCUASFileViewMulti(this),tr("Onboard Files"),"FILE_VIEW_DOCKWIDGET",currentView,Qt::RightDockWidgetArea);
    }
    else if (name == "UAS_STATUS_DETAILS_DOCKWIDGET")
    {
        createDockWidget(centerStack->currentWidget(),new UASInfoWidget(this),tr("Status Details"),"UAS_STATUS_DETAILS_DOCKWIDGET",currentView,Qt::RightDockWidgetArea);
    }
    else if (name == "MAP_VIEW_DOCKWIDGET")
    {
        createDockWidget(centerStack->currentWidget(),new QGCMapTool(this),tr("Map view"),"MAP_VIEW_DOCKWIDGET",currentView,Qt::RightDockWidgetArea);
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
    else if (name == "PRIMARY_FLIGHT_DISPLAY_DOCKWIDGET")
    {
        createDockWidget(centerStack->currentWidget(),new PrimaryFlightDisplay(this),tr("Primary Flight Display"),"PRIMARY_FLIGHT_DISPLAY_DOCKWIDGET",currentView,Qt::RightDockWidgetArea);
    }
    else if (name == "HEAD_UP_DISPLAY_DOCKWIDGET")
    {
        createDockWidget(centerStack->currentWidget(),new HUD(320,240,this),tr("Video Downlink"),"HEAD_UP_DISPLAY_DOCKWIDGET",currentView,Qt::RightDockWidgetArea);
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
    QWidget* widget = act->data().value<QWidget *>();
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
    storeViewState();
    storeSettings();
    mavlink->storeSettings();
    UASManager::instance()->storeSettings();
    // FIXME: If connected links, should prompt before close
    LinkManager::instance()->disconnectAll();
    event->accept();
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
    QString fileName = QGCFileDialog::getOpenFileName(this, tr("Specify Widget File Name"), QStandardPaths::writableLocation(QStandardPaths::DesktopLocation), tr("QGroundControl Widget (*%1);;").arg(widgetFileExtension));
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
        default: // Flight view is the default.
        case VIEW_FLIGHT:
            createDockWidget(pilotView,tool,tool->getTitle(),tool->objectName()+"DOCK",(VIEW_SECTIONS)view,Qt::LeftDockWidgetArea);
            break;
        case VIEW_SIMULATION:
            createDockWidget(simView,tool,tool->getTitle(),tool->objectName()+"DOCK",(VIEW_SECTIONS)view,Qt::LeftDockWidgetArea);
            break;
        case VIEW_MISSION:
            createDockWidget(plannerView,tool,tool->getTitle(),tool->objectName()+"DOCK",(VIEW_SECTIONS)view,Qt::LeftDockWidgetArea);
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

        int view = settings.value(QString("TOOL_PARENT_") + tool->objectName(),-1).toInt();
        switch (view)
        {
        case VIEW_ENGINEER:
            createDockWidget(engineeringView,tool,tool->getTitle(),tool->objectName()+"DOCK",(VIEW_SECTIONS)view,Qt::LeftDockWidgetArea);
            break;
        default: // Flight view is the default.
        case VIEW_FLIGHT:
            createDockWidget(pilotView,tool,tool->getTitle(),tool->objectName()+"DOCK",(VIEW_SECTIONS)view,Qt::LeftDockWidgetArea);
            break;
        case VIEW_SIMULATION:
            createDockWidget(simView,tool,tool->getTitle(),tool->objectName()+"DOCK",(VIEW_SECTIONS)view,Qt::LeftDockWidgetArea);
            break;
        case VIEW_MISSION:
            createDockWidget(plannerView,tool,tool->getTitle(),tool->objectName()+"DOCK",(VIEW_SECTIONS)view,Qt::LeftDockWidgetArea);
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

    customMode = static_cast<enum MainWindow::CUSTOM_MODE>(settings.value("QGC_CUSTOM_MODE", (unsigned int)MainWindow::CUSTOM_MODE_NONE).toInt());

    settings.beginGroup("QGC_MAINWINDOW");
    autoReconnect = settings.value("AUTO_RECONNECT", autoReconnect).toBool();
    currentStyle = (QGC_MAINWINDOW_STYLE)settings.value("CURRENT_STYLE", currentStyle).toInt();
    lowPowerMode = settings.value("LOW_POWER_MODE", lowPowerMode).toBool();
    bool dockWidgetTitleBarEnabled = settings.value("DOCK_WIDGET_TITLEBARS",menuActionHelper->dockWidgetTitleBarsEnabled()).toBool();
    settings.endGroup();

    enableDockWidgetTitleBars(dockWidgetTitleBarEnabled);
}

void MainWindow::storeSettings()
{
    QSettings settings;

    settings.setValue("QGC_CUSTOM_MODE", (int)customMode);

    settings.beginGroup("QGC_MAINWINDOW");
    settings.setValue("AUTO_RECONNECT", autoReconnect);
    settings.setValue("CURRENT_STYLE", currentStyle);
    settings.setValue("LOW_POWER_MODE", lowPowerMode);
    settings.endGroup();

    settings.setValue(getWindowGeometryKey(), saveGeometry());

    // Save the last current view in any case
    settings.setValue("CURRENT_VIEW", currentView);

    // Save the current window state, but only if a system is connected (else no real number of widgets would be present))
    if (UASManager::instance()->getUASList().length() > 0) settings.setValue(getWindowStateKey(), saveState());

    // Save the current UAS view if a UAS is connected
    if (UASManager::instance()->getUASList().length() > 0) settings.setValue("CURRENT_VIEW_WITH_UAS_CONNECTED", currentView);

    // And save any custom weidgets
    QGCToolWidget::storeWidgetsToSettings(settings);
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
}

void MainWindow::startVideoCapture()
{
    QString format = "bmp";
    QString initialPath = QDir::currentPath() + tr("/untitled.") + format;

    QString screenFileName = QGCFileDialog::getSaveFileName(this, tr("Save As"),
                                                          initialPath,
                                                          tr("%1 Files (*.%2);;All Files (*)")
                                                          .arg(format.toUpper())
                                                          .arg(format));
    delete videoTimer;
    videoTimer = new QTimer(this);
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
        window.save(screenFileName, format.toLatin1());
    }
}
void MainWindow::enableDockWidgetTitleBars(bool enabled)
{
    menuActionHelper->setDockWidgetTitleBarsEnabled(enabled);
    QSettings settings;
    settings.beginGroup("QGC_MAINWINDOW");
    settings.setValue("DOCK_WIDGET_TITLEBARS",enabled);
    settings.endGroup();
}

void MainWindow::enableAutoReconnect(bool enabled)
{
    autoReconnect = enabled;
}

bool MainWindow::loadStyle(QGC_MAINWINDOW_STYLE style)
{
    //qDebug() << "LOAD STYLE" << style;
    bool success = true;
    QString styles;
    
    // Signal to the user that the app will pause to apply a new stylesheet
    qApp->setOverrideCursor(Qt::WaitCursor);
    
    // Store the new style classification.
    currentStyle = style;
    
    // The dark style sheet is the master. Any other selected style sheet just overrides
    // the colors of the master sheet.
    QFile masterStyleSheet(defaultDarkStyle);
    if (masterStyleSheet.open(QIODevice::ReadOnly | QIODevice::Text)) {
        styles = masterStyleSheet.readAll();
    } else {
        qDebug() << "Unable to load master dark style sheet";
        success = false;
    }

    if (success && style == QGC_MAINWINDOW_STYLE_LIGHT) {
        qDebug() << "LOADING LIGHT";
        // Load the slave light stylesheet.
        QFile styleSheet(defaultLightStyle);
        if (styleSheet.open(QIODevice::ReadOnly | QIODevice::Text)) {
            styles += styleSheet.readAll();
        } else {
            qDebug() << "Unable to load slave light sheet:";
            success = false;
        }
    }

    if (!styles.isEmpty()) {
        qApp->setStyleSheet(styles);
        emit styleChanged(style);
    }
    
    // Finally restore the cursor before returning.
    qApp->restoreOverrideCursor();
    
    return success;
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
    qDebug() << "Critical" << title << message;
    QGCMessageBox::critical(title, message);
}

void MainWindow::showInfoMessage(const QString& title, const QString& message)
{
    qDebug() << "Information" << title << message;
    QGCMessageBox::information(title, message);
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
    perspectives->addAction(ui.actionFlightView);
    perspectives->addAction(ui.actionSimulationView);
    perspectives->addAction(ui.actionMissionView);
    perspectives->addAction(ui.actionSetup);
    perspectives->addAction(ui.actionTerminalView);
    perspectives->addAction(ui.actionGoogleEarthView);
    perspectives->addAction(ui.actionLocal3DView);
    perspectives->setExclusive(true);

    /* Hide the actions that are not relevant */
#ifndef QGC_GOOGLE_EARTH_ENABLED
    ui.actionGoogleEarthView->setVisible(false);
#endif
#ifndef QGC_OSG_ENABLED
    ui.actionLocal3DView->setVisible(false);
#endif

    // Mark the right one as selected
    if (currentView == VIEW_ENGINEER)
    {
        ui.actionEngineersView->setChecked(true);
        ui.actionEngineersView->activate(QAction::Trigger);
    }
    if (currentView == VIEW_FLIGHT)
    {
        ui.actionFlightView->setChecked(true);
        ui.actionFlightView->activate(QAction::Trigger);
    }
    if (currentView == VIEW_SIMULATION)
    {
        ui.actionSimulationView->setChecked(true);
        ui.actionSimulationView->activate(QAction::Trigger);
    }
    if (currentView == VIEW_MISSION)
    {
        ui.actionMissionView->setChecked(true);
        ui.actionMissionView->activate(QAction::Trigger);
    }
    if (currentView == VIEW_SETUP)
    {
        ui.actionSetup->setChecked(true);
        ui.actionSetup->activate(QAction::Trigger);
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

    // Views actions
    connect(ui.actionFlightView, SIGNAL(triggered()), this, SLOT(loadPilotView()));
    connect(ui.actionSimulationView, SIGNAL(triggered()), this, SLOT(loadSimulationView()));
    connect(ui.actionEngineersView, SIGNAL(triggered()), this, SLOT(loadEngineerView()));
    connect(ui.actionMissionView, SIGNAL(triggered()), this, SLOT(loadOperatorView()));
    connect(ui.actionSetup,SIGNAL(triggered()),this,SLOT(loadSetupView()));
    connect(ui.actionGoogleEarthView, SIGNAL(triggered()), this, SLOT(loadGoogleEarthView()));
    connect(ui.actionLocal3DView, SIGNAL(triggered()), this, SLOT(loadLocal3DView()));
    connect(ui.actionTerminalView,SIGNAL(triggered()),this,SLOT(loadTerminalView()));

    // Help Actions
    connect(ui.actionOnline_Documentation, SIGNAL(triggered()), this, SLOT(showHelp()));
    connect(ui.actionDeveloper_Credits, SIGNAL(triggered()), this, SLOT(showCredits()));
    connect(ui.actionProject_Roadmap, SIGNAL(triggered()), this, SLOT(showRoadMap()));

    // Custom widget actions
    connect(ui.actionNewCustomWidget, SIGNAL(triggered()), this, SLOT(createCustomWidget()));
    connect(ui.actionLoadCustomWidgetFile, SIGNAL(triggered()), this, SLOT(loadCustomWidget()));

    // Audio output
    ui.actionMuteAudioOutput->setChecked(GAudioOutput::instance()->isMuted());
    connect(GAudioOutput::instance(), SIGNAL(mutedChanged(bool)), ui.actionMuteAudioOutput, SLOT(setChecked(bool)));
    connect(ui.actionMuteAudioOutput, SIGNAL(triggered(bool)), GAudioOutput::instance(), SLOT(mute(bool)));

    // Application Settings
    connect(ui.actionSettings, SIGNAL(triggered()), this, SLOT(showSettings()));

    connect(ui.actionSimulate, SIGNAL(triggered(bool)), this, SLOT(simulateLink(bool)));
}

void MainWindow::_openUrl(const QString& url, const QString& errorMessage)
{
    if(!QDesktopServices::openUrl(QUrl(url))) {
        QMessageBox::critical(this,
                              tr("Could not open information in browser"),
                              errorMessage);
    }
}

void MainWindow::showHelp()
{
    _openUrl("http://qgroundcontrol.org/users/start",
             tr("To get to the online help, please open http://qgroundcontrol.org/user_guide in a browser."));
}

void MainWindow::showCredits()
{
    _openUrl("http://qgroundcontrol.org/credits",
             tr("To get to the credits, please open http://qgroundcontrol.org/credits in a browser."));
}

void MainWindow::showRoadMap()
{
    _openUrl("http://qgroundcontrol.org/dev/roadmap",
             tr("To get to the online help, please open http://qgroundcontrol.org/roadmap in a browser."));
}

void MainWindow::showSettings()
{
    SettingsDialog settings(joystick, this);
    settings.exec();
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
        {
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
        {
            found = true;
            action->trigger(); // Show the Link Config Dialog
        }
    }

    return found;
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
        {
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
    if (simulate) {
        if (!simulationLink) {
            simulationLink = new MAVLinkSimulationLink(":/demo-log.txt");
            Q_CHECK_PTR(simulationLink);
        }
        LinkManager::instance()->connectLink(simulationLink);
    } else {
        Q_ASSERT(simulationLink);
        LinkManager::instance()->disconnectLink(simulationLink);
    }
}

void MainWindow::commsWidgetDestroyed(QObject *obj)
{
    // Do not dynamic cast or de-reference QObject, since object is either in destructor or may have already
    // been destroyed.

    if (commsWidgetList.contains(obj))
    {
        commsWidgetList.removeOne(obj);
    }
}

void MainWindow::setActiveUAS(UASInterface* uas)
{
    Q_UNUSED(uas);
    if (settings.contains(getWindowStateKey()))
    {
        SubMainWindow *win = qobject_cast<SubMainWindow*>(centerStack->currentWidget());
        win->restoreState(settings.value(getWindowStateKey()).toByteArray());
    }

}

void MainWindow::UASSpecsChanged(int uas)
{
    Q_UNUSED(uas);
    // TODO: Update UAS properties if its specs change
}

void MainWindow::UASCreated(UASInterface* uas)
{
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


    connect(uas, SIGNAL(systemSpecsChanged(int)), this, SLOT(UASSpecsChanged(int)));
    connect(uas, SIGNAL(valueChanged(int,QString,QString,QVariant,quint64)), this, SIGNAL(valueChanged(int,QString,QString,QVariant,quint64)));
    connect(uas, SIGNAL(misconfigurationDetected(UASInterface*)), this, SLOT(handleMisconfiguration(UASInterface*)));

    // HIL
    showHILConfigurationWidget(uas);

    if (!linechartWidget)
    {
        linechartWidget = new Linecharts(this);
    }

    linechartWidget->addSource(mavlinkDecoder);
    if (engineeringView->centralWidget() != linechartWidget)
    {
        engineeringView->setCentralWidget(linechartWidget);
        linechartWidget->show();
    }

    // Load default custom widgets for this autopilot type
    loadCustomWidgetsFromDefaults(uas->getSystemTypeName(), uas->getAutopilotTypeName());

    // Reload view state in case new widgets were added
    loadViewState();
}

void MainWindow::UASDeleted(UASInterface* uas)
{
    Q_UNUSED(uas);
    // TODO: Update the UI when a UAS is deleted
}

/**
 * Stores the current view state
 */
void MainWindow::storeViewState()
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
    settings.setValue(getWindowStateKey(), win->saveState());
    settings.setValue(getWindowStateKey()+"CENTER_WIDGET", centerStack->currentIndex());
    // Although we want save the state of the window, we do not want to change the top-leve state (minimized, maximized, etc)
    // therefore this state is stored here and restored after applying the rest of the settings in the new
    // perspective.
    windowStateVal = this->windowState();
    settings.setValue(getWindowGeometryKey(), saveGeometry());
}

void MainWindow::loadViewState()
{
    // Restore center stack state
    int index = settings.value(getWindowStateKey()+"CENTER_WIDGET", -1).toInt();

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
        case VIEW_SETUP:
            centerStack->setCurrentWidget(setupView);
            break;
        case VIEW_ENGINEER:
            centerStack->setCurrentWidget(engineeringView);
            break;
        default: // Default to the flight view
        case VIEW_FLIGHT:
            centerStack->setCurrentWidget(pilotView);
            break;
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
                //qDebug() << "Loading widget:" << widgetname;
                loadDockWidget(widgetname);
            }
        }
    }
    if (settings.contains(getWindowStateKey()))
    {
        SubMainWindow *win = qobject_cast<SubMainWindow*>(centerStack->currentWidget());
        win->restoreState(settings.value(getWindowStateKey()).toByteArray());
    }
}
void MainWindow::setAdvancedMode(bool isAdvancedMode)
{
    menuActionHelper->setAdvancedMode(isAdvancedMode);
    ui.actionAdvanced_Mode->setChecked(isAdvancedMode);
    settings.setValue("ADVANCED_MODE",isAdvancedMode);
}

void MainWindow::handleMisconfiguration(UASInterface* uas)
{
    static QTime lastTime;
    
    // We have to debounce this signal
    if (!lastTime.isValid()) {
        lastTime.start();
    } else {
        if (lastTime.elapsed() < 10000) {
            lastTime.start();
            return;
        }
    }
    
    // Ask user if he wants to handle this now
    QMessageBox::StandardButton button = QGCMessageBox::question(tr("Missing or Invalid Onboard Configuration"),
                                                                    tr("The onboard system configuration is missing or incomplete. Do you want to resolve this now?"),
                                                                    QMessageBox::Ok | QMessageBox::Cancel,
                                                                    QMessageBox::Ok);
    if (button == QMessageBox::Ok) {
        // He wants to handle it, make sure this system is selected
        UASManager::instance()->setActiveUAS(uas);

        // Flick to config view
        loadSetupView();
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
void MainWindow::loadSetupView()
{
    if (currentView != VIEW_SETUP)
    {
        storeViewState();
        currentView = VIEW_SETUP;
        ui.actionSetup->setChecked(true);
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
        ui.actionSimulationView->setChecked(true);
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

void MainWindow::_saveTempFlightDataLog(QString tempLogfile)
{
    if (qgcApp()->promptFlightDataSave()) {
        QString saveFilename = QGCFileDialog::getSaveFileName(this,
                                                            tr("Select file to save Flight Data Log"),
                                                            qgcApp()->mavlinkLogFilesLocation(),
                                                            tr("Flight Data Log (*.mavlink)"));
        if (!saveFilename.isEmpty()) {
            QFile::copy(tempLogfile, saveFilename);
        }
    }
    QFile::remove(tempLogfile);
}

/// @brief Hides the spash screen if it is currently being shown
void MainWindow::hideSplashScreen(void)
{
    if (_splashScreen) {
        _splashScreen->hide();
        _splashScreen = NULL;
    }
}


#ifdef QGC_MOUSE_ENABLED_LINUX
bool MainWindow::x11Event(XEvent *event)
{
    emit x11EventOccured(event);
    return false;
}
#endif // QGC_MOUSE_ENABLED_LINUX
