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
#include "UASQuickView.h"
#include "QGCDataPlot2D.h"
#include "Linecharts.h"
#include "QGCTabbedInfoView.h"
#include "UASRawStatusView.h"
#include "PrimaryFlightDisplay.h"
#include "SetupView.h"
#include "SerialSettingsDialog.h"
#include "terminalconsole.h"
#include "QGCUASFileViewMulti.h"
#include "QGCApplication.h"
#include "QGCFileDialog.h"
#include "QGCMessageBox.h"
#include "QGCDockWidget.h"

#ifdef QGC_OSG_ENABLED
#include "Q3DWidgetFactory.h"
#endif

#include "LogCompressor.h"

const char* MainWindow::_uasControlDockWidgetName = "UNMANNED_SYSTEM_CONTROL_DOCKWIDGET";
const char* MainWindow::_uasListDockWidgetName = "UNMANNED_SYSTEM_LIST_DOCKWIDGET";
const char* MainWindow::_waypointsDockWidgetName = "WAYPOINT_LIST_DOCKWIDGET";
const char* MainWindow::_mavlinkDockWidgetName = "MAVLINK_INSPECTOR_DOCKWIDGET";
const char* MainWindow::_parametersDockWidgetName = "PARAMETER_INTERFACE_DOCKWIDGET";
const char* MainWindow::_filesDockWidgetName = "FILE_VIEW_DOCKWIDGET";
const char* MainWindow::_uasStatusDetailsDockWidgetName = "UAS_STATUS_DETAILS_DOCKWIDGET";
const char* MainWindow::_mapViewDockWidgetName = "MAP_VIEW_DOCKWIDGET";
const char* MainWindow::_hsiDockWidgetName = "HORIZONTAL_SITUATION_INDICATOR_DOCKWIDGET";
const char* MainWindow::_hdd1DockWidgetName = "HEAD_DOWN_DISPLAY_1_DOCKWIDGET";
const char* MainWindow::_hdd2DockWidgetName = "HEAD_DOWN_DISPLAY_2_DOCKWIDGET";
const char* MainWindow::_pfdDockWidgetName = "PRIMARY_FLIGHT_DISPLAY_DOCKWIDGET";
const char* MainWindow::_hudDockWidgetName = "HEAD_UP_DISPLAY_DOCKWIDGET";
const char* MainWindow::_uasInfoViewDockWidgetName = "UAS_INFO_INFOVIEW_DOCKWIDGET";
const char* MainWindow::_debugConsoleDockWidgetName = "COMMUNICATION_CONSOLE_DOCKWIDGET";

static MainWindow* _instance = NULL;   ///< @brief MainWindow singleton

MainWindow* MainWindow::_create(QSplashScreen* splashScreen)
{
    Q_ASSERT(_instance == NULL);
    
    new MainWindow(splashScreen);
    
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
MainWindow::MainWindow(QSplashScreen* splashScreen) :
    centerStackActionGroup(new QActionGroup(this)),
    autoReconnect(false),
    simulationLink(NULL),
    lowPowerMode(false),
    _currentView(VIEW_FLIGHT),
    _currentViewWidget(NULL),
    _splashScreen(splashScreen)
{
    Q_ASSERT(_instance == NULL);
    _instance = this;
    
    if (splashScreen) {
        connect(this, &MainWindow::initStatusChanged, splashScreen, &QSplashScreen::showMessage);
    }
    
    loadSettings();
    
    // Select the proper view. Default to the flight view or load the last one used if it's supported.
    VIEW_SECTIONS currentViewCandidate = (VIEW_SECTIONS) settings.value("CURRENT_VIEW", _currentView).toInt();
    switch (currentViewCandidate) {
        case VIEW_ENGINEER:
        case VIEW_MISSION:
        case VIEW_FLIGHT:
        case VIEW_SIMULATION:
        case VIEW_SETUP:
        case VIEW_TERMINAL:
#ifdef QGC_OSG_ENABLED
        case VIEW_LOCAL3D:
#endif
#ifdef QGC_GOOGLE_EARTH_ENABLED
        case VIEW_GOOGLEEARTH:
#endif
            _currentView = currentViewCandidate;
            break;
            
        default:
            // Leave _currentView to the default
            break;
    }
    
    // Put it back, which will set it to a valid value
    settings.setValue("CURRENT_VIEW", _currentView);

    emit initStatusChanged(tr("Setting up user interface"), Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));

    // Setup user interface
    ui.setupUi(this);
    
    // Setup central widget with a layout to hold the views
    _centralLayout = new QVBoxLayout();
    centralWidget()->setLayout(_centralLayout);

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

    setStatusBar(new QStatusBar(this));
    statusBar()->setSizeGripEnabled(true);

    emit initStatusChanged(tr("Building common widgets."), Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));

    _buildCommonWidgets();

    emit initStatusChanged(tr("Building common actions"), Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));

    // Create actions
    connectCommonActions();

    // Populate link menu
    emit initStatusChanged(tr("Populating link menu"), Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));
    QList<LinkInterface*> links = LinkManager::instance()->getLinks();
    foreach(LinkInterface* link, links)
    {
        _addLinkMenu(link);
    }

    connect(LinkManager::instance(), &LinkManager::newLink, this, &MainWindow::_addLinkMenu);
    
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
        linkMgr->addLink(link);
        linkMgr->connectLink(link);
    }

    // Set low power mode
    enableLowPowerMode(lowPowerMode);

    emit initStatusChanged(tr("Restoring last view state"), Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));

    // Restore the window setup
    _loadCurrentViewState();

    emit initStatusChanged(tr("Restoring last window size"), Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));
    // Restore the window position and size
    if (settings.contains(getWindowGeometryKey()))
    {
        // Restore the window geometry
        restoreGeometry(settings.value(getWindowGeometryKey()).toByteArray());
    }
    else
    {
        // Adjust the size
        const int screenWidth = QApplication::desktop()->width();
        const int screenHeight = QApplication::desktop()->height();

        if (screenWidth < 1500)
        {
            resize(screenWidth, screenHeight - 80);
        }
        else
        {
            resize(screenWidth*0.67f, qMin(screenHeight, (int)(screenWidth*0.67f*0.67f)));
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

	if (!qgcApp()->runningUnitTests()) {
		show();
	}
}

MainWindow::~MainWindow()
{
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

    // Delete all UAS objects
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
        return QString::number(_currentView)+"_windowstate_" + UASManager::instance()->getActiveUAS()->getAutopilotTypeName();
    }
    else
        return QString::number(_currentView)+"_windowstate_";
}

QString MainWindow::getWindowGeometryKey()
{
    return "_geometry";
}

void MainWindow::_buildCustomWidgets(void)
{
    Q_ASSERT(_customWidgets.count() == 0);
    
    // Create custom widgets
    _customWidgets = QGCToolWidget::createWidgetsFromSettings(this);

    if (_customWidgets.size() > 0)
    {
        ui.menuTools->addSeparator();
    }
    
    foreach(QGCToolWidget* tool, _customWidgets) {
        // Check if this widget already has a parent, do not create it in this case
        QDockWidget* dock = dynamic_cast<QDockWidget*>(tool->parentWidget());
        
        if (!dock) {
            _createDockWidget(tool->getTitle(), tool->objectName(), Qt::BottomDockWidgetArea, tool);
        }
    }
}

void MainWindow::_createDockWidget(const QString& title, const QString& name, Qt::DockWidgetArea area, QWidget* innerWidget)
{
    Q_ASSERT(!_mapName2DockWidget.contains(name));
    
    QGCDockWidget* dockWidget = new QGCDockWidget(title, this);
    Q_CHECK_PTR(dockWidget);
    dockWidget->setObjectName(name);
    dockWidget->setVisible (false);
    
    if (innerWidget) {
        // Put inner widget inside QDockWidget
        innerWidget->setParent(dockWidget);
        dockWidget->setWidget(innerWidget);
        innerWidget->setVisible(true);
    }
    
    // Add to menu
    
    QAction* action = new QAction(title, NULL);
    action->setCheckable(true);
    action->setData(name);
    
    connect(action, &QAction::triggered, this, &MainWindow::_showDockWidgetAction);
    
    ui.menuTools->addAction(action);
    
    _mapName2DockWidget[name] = dockWidget;
    _mapDockWidget2Action[dockWidget] = action;
    
    addDockWidget(area, dockWidget);
}

void MainWindow::_buildCommonWidgets(void)
{
    // Add generic MAVLink decoder
    mavlinkDecoder = new MAVLinkDecoder(MAVLinkProtocol::instance(), this);
    connect(mavlinkDecoder, SIGNAL(valueChanged(int,QString,QString,QVariant,quint64)),
                      this, SIGNAL(valueChanged(int,QString,QString,QVariant,quint64)));

    // Log player
    logPlayer = new QGCMAVLinkLogPlayer(MAVLinkProtocol::instance(), statusBar());
    statusBar()->addPermanentWidget(logPlayer);

    // In order for Qt to save and restore state of widgets all widgets must be created ahead of time. We only create the QDockWidget
    // holders. We do not create the actual inner widget until it is needed. This saves memory and cpu from running widgets that are
    // never shown.
    
    struct DockWidgetInfo {
        const char* name;
        const char* title;
        Qt::DockWidgetArea area;
    };
    
    static const struct DockWidgetInfo rgDockWidgetInfo[] = {
        { _uasControlDockWidgetName,        "Control",                  Qt::LeftDockWidgetArea },
        { _uasListDockWidgetName,           "Unmanned Systems",         Qt::RightDockWidgetArea },
        { _waypointsDockWidgetName,         "Mission Plan",             Qt::BottomDockWidgetArea },
        { _mavlinkDockWidgetName,           "MAVLink Inspector",        Qt::RightDockWidgetArea },
        { _parametersDockWidgetName,        "Onboard Parameters",       Qt::RightDockWidgetArea },
        { _filesDockWidgetName,             "Onboard Files",            Qt::RightDockWidgetArea },
        { _uasStatusDetailsDockWidgetName,  "Status Details",           Qt::RightDockWidgetArea },
        { _mapViewDockWidgetName,           "Map view",                 Qt::RightDockWidgetArea },
        { _hsiDockWidgetName,               "Horizontal Situation",     Qt::BottomDockWidgetArea },
        { _hdd1DockWidgetName,              "Flight Display",           Qt::RightDockWidgetArea },
        { _hdd2DockWidgetName,              "Actuator Status",          Qt::RightDockWidgetArea },
        { _pfdDockWidgetName,               "Primary Flight Display",   Qt::RightDockWidgetArea },
        { _hudDockWidgetName,               "Video Downlink",           Qt::RightDockWidgetArea },
        { _uasInfoViewDockWidgetName,       "Info View",                Qt::LeftDockWidgetArea },
        { _debugConsoleDockWidgetName,      "Communications Console",   Qt::LeftDockWidgetArea }
    };
    static const size_t cDockWidgetInfo = sizeof(rgDockWidgetInfo) / sizeof(rgDockWidgetInfo[0]);
    
    for (size_t i=0; i<cDockWidgetInfo; i++) {
        const struct DockWidgetInfo* pDockInfo = &rgDockWidgetInfo[i];
        
        _createDockWidget(pDockInfo->title, pDockInfo->name, pDockInfo->area, NULL /* no inner widget yet */);
    }

    _buildCustomWidgets();
}

void MainWindow::_buildPlannerView(void)
{
    if (!_plannerView) {
        _plannerView = new QGCMapTool(this);
        _plannerView->setVisible(false);
    }
}

void MainWindow::_buildPilotView(void)
{
    if (!_pilotView) {
        _pilotView = new PrimaryFlightDisplay(this);
        _pilotView->setVisible(false);
    }
}

void MainWindow::_buildSetupView(void)
{
    if (!_setupView) {
        _setupView = new SetupView(this);
        _setupView->setVisible(false);
    }
}

void MainWindow::_buildEngineeringView(void)
{
    if (!_engineeringView) {
        _engineeringView = new QGCDataPlot2D(this);
        _engineeringView->setVisible(false);
    }
}

void MainWindow::_buildSimView(void)
{
    if (!_simView) {
        _simView = new QGCMapTool(this);
        _simView->setVisible(false);
    }
}

void MainWindow::_buildTerminalView(void)
{
    if (!_terminalView) {
        _terminalView = new TerminalConsole(this);
        _terminalView->setVisible(false);
    }
}

void MainWindow::_buildGoogleEarthView(void)
{
#ifdef QGC_GOOGLE_EARTH_ENABLED
    if (!_googleEarthView) {
        _googleEarthView = new QGCGoogleEarthView(this);
        _googleEarthView->setVisible(false);
    }
#endif
}

void MainWindow::_buildLocal3DView(void)
{
#ifdef QGC_OSG_ENABLED
    if (!_local3DView) {
        _local3DView = Q3DWidgetFactory::get("PIXHAWK", this);
        _local3DView->setVisible(false);
    }
#endif
}

/// Shows or hides the specified dock widget, creating if necessary
void MainWindow::_showDockWidget(const QString& name, bool show)
{
    if (!_mapName2DockWidget.contains(name)) {
        qWarning() << "Attempt to show unknown dock widget" << name;
        return;
    }
    
    // Create the inner widget if we need to
    if (!_mapName2DockWidget[name]->widget()) {
        _createInnerDockWidget(name);
    }

    Q_ASSERT(_mapName2DockWidget.contains(name));
    QDockWidget* dockWidget = _mapName2DockWidget[name];
    Q_ASSERT(dockWidget);
    
    dockWidget->setVisible(show);
    
    Q_ASSERT(_mapDockWidget2Action.contains(dockWidget));
    _mapDockWidget2Action[dockWidget]->setChecked(show);
}

/// Creates the specified inner dock widget and adds to the QDockWidget
void MainWindow::_createInnerDockWidget(const QString& widgetName)
{
    Q_ASSERT(_mapName2DockWidget.contains(widgetName)); // QDockWidget should already exist
    Q_ASSERT(!_mapName2DockWidget[widgetName]->widget());     // Inner widget should not
    
    QWidget* widget = NULL;
    
    if (widgetName == _uasControlDockWidgetName) {
        widget = new UASControlWidget(this);
    } else if (widgetName == _uasListDockWidgetName) {
        widget = new UASListWidget(this);
    } else if (widgetName == _waypointsDockWidgetName) {
        widget = new QGCWaypointListMulti(this);
    } else if (widgetName == _mavlinkDockWidgetName) {
        widget = new QGCMAVLinkInspector(MAVLinkProtocol::instance(),this);
    } else if (widgetName == _parametersDockWidgetName) {
        widget = new ParameterInterface(this);
    } else if (widgetName == _filesDockWidgetName) {
        widget = new QGCUASFileViewMulti(this);
    } else if (widgetName == _uasStatusDetailsDockWidgetName) {
        widget = new UASInfoWidget(this);
    } else if (widgetName == _mapViewDockWidgetName) {
        widget = new QGCMapTool(this);
    } else if (widgetName == _hsiDockWidgetName) {
        widget = new HSIDisplay(this);
    } else if (widgetName == _hdd1DockWidgetName) {
        QStringList acceptList;
        acceptList.append("-3.3,ATTITUDE.roll,rad,+3.3,s");
        acceptList.append("-3.3,ATTITUDE.pitch,deg,+3.3,s");
        acceptList.append("-3.3,ATTITUDE.yaw,deg,+3.3,s");
        HDDisplay *hddisplay = new HDDisplay(acceptList,"Flight Display",this);
        hddisplay->addSource(mavlinkDecoder);
        
        widget = hddisplay;
    } else if (widgetName == _hdd2DockWidgetName) {
        QStringList acceptList;
        acceptList.append("0,RAW_PRESSURE.pres_abs,hPa,65500");
        HDDisplay *hddisplay = new HDDisplay(acceptList,"Actuator Status",this);
        hddisplay->addSource(mavlinkDecoder);
        
        widget = hddisplay;
    } else if (widgetName == _pfdDockWidgetName) {
        widget = new PrimaryFlightDisplay(this);
    } else if (widgetName == _hudDockWidgetName) {
        widget = new HUD(320,240,this);
    } else if (widgetName == _uasInfoViewDockWidgetName) {
        widget = new QGCTabbedInfoView(this);
    } else if (widgetName == _debugConsoleDockWidgetName) {
        widget = new DebugConsole(this);
    } else {
        qWarning() << "Attempt to create unknown Inner Dock Widget" << widgetName;
    }
    
    if (widget) {
        QDockWidget* dockWidget = _mapName2DockWidget[widgetName];
        Q_CHECK_PTR(dockWidget);
        
        widget->setParent(dockWidget);
        dockWidget->setWidget(widget);
    }
}

void MainWindow::_showHILConfigurationWidgets(void)
{
    UASInterface* uas = UASManager::instance()->getActiveUAS();
    
    if (!uas) {
        return;
    }
    
    UAS* mav = dynamic_cast<UAS*>(uas);
    Q_ASSERT(mav);
    
    int uasId = mav->getUASID();

    if (!_mapUasId2HilDockWidget.contains(uasId)) {
        
        // Create QDockWidget
        QGCDockWidget* dockWidget = new QGCDockWidget(tr("HIL Config %1").arg(uasId), this);
        Q_CHECK_PTR(dockWidget);
        dockWidget->setObjectName(tr("HIL_CONFIG_%1").arg(uasId));
        dockWidget->setVisible (false);
        
        // Create inner widget and set it
        QWidget* widget = new QGCHilConfiguration(mav, dockWidget);
        
        widget->setParent(dockWidget);
        dockWidget->setWidget(widget);
        
        _mapUasId2HilDockWidget[uasId] = dockWidget;
        
        addDockWidget(Qt::LeftDockWidgetArea, dockWidget);
    }
    
    if (_currentView == VIEW_SIMULATION) {
        // HIL dock widgets only show up on simulation view
        foreach (QDockWidget* dockWidget, _mapUasId2HilDockWidget) {
            dockWidget->setVisible(true);
        }
    }
}

void MainWindow::fullScreenActionItemCallback()
{
    ui.actionNormal->setChecked(false);
}

void MainWindow::normalActionItemCallback()
{
    ui.actionFullscreen->setChecked(false);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Disallow window close if there are active connections
    
    bool foundConnections = false;
    foreach(LinkInterface* link, LinkManager::instance()->getLinks()) {
        if (link->isConnected()) {
            foundConnections = true;
            break;
        }
    }
    
    if (foundConnections) {
        QGCMessageBox::StandardButton button = QGCMessageBox::warning(tr("QGroundControl close"),
                                                                      tr("There are still active connections to vehicles. Do you want to disconnect these before closing?"),
                                                                      QMessageBox::Yes | QMessageBox::Cancel,
                                                                      QMessageBox::Cancel);
        if (button == QMessageBox::Yes) {
            foreach(LinkInterface* link, LinkManager::instance()->getLinks()) {
                LinkManager::instance()->disconnectLink(link);
            }
        } else {
            event->ignore();
            return;
        }
    }

    // This will process any remaining flight log save dialogs
    qgcApp()->processEvents(QEventLoop::ExcludeUserInputEvents);

    // Should not be any active connections
    foreach(LinkInterface* link, LinkManager::instance()->getLinks()) {
        Q_UNUSED(link);
        Q_ASSERT(!link->isConnected());
    }

    _storeCurrentViewState();
    storeSettings();
    UASManager::instance()->storeSettings();
    event->accept();
}

void MainWindow::_createNewCustomWidget(void)
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
    tool->resize(100, 100);
    _createDockWidget(title, objectName, Qt::BottomDockWidgetArea, tool);
    
    _mapName2DockWidget[objectName]->setVisible(true);
}

void MainWindow::_loadCustomWidgetFromFile(void)
{
    QString fileName = QGCFileDialog::getOpenFileName(
        this, tr("Load Widget File"),
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
        tr("QGroundControl Widget (*.qgw);;All Files (*)"));
    if (!fileName.isEmpty()) {
        QGCToolWidget* tool = new QGCToolWidget("", "", this);
        if (tool->loadSettings(fileName, true)) {
            QString objectName = tool->objectName() + "DOCK";
            _createDockWidget(tool->getTitle(), objectName, Qt::LeftDockWidgetArea, tool);
            _mapName2DockWidget[objectName]->widget()->setVisible(true);
        }
    }
    // TODO Add error dialog if widget could not be loaded
}

void MainWindow::loadSettings()
{
    QSettings settings;

    settings.beginGroup("QGC_MAINWINDOW");
    autoReconnect = settings.value("AUTO_RECONNECT", autoReconnect).toBool();
    lowPowerMode = settings.value("LOW_POWER_MODE", lowPowerMode).toBool();
    settings.endGroup();
}

void MainWindow::storeSettings()
{
    QSettings settings;

    settings.beginGroup("QGC_MAINWINDOW");
    settings.setValue("AUTO_RECONNECT", autoReconnect);
    settings.setValue("LOW_POWER_MODE", lowPowerMode);
    settings.endGroup();

    settings.setValue(getWindowGeometryKey(), saveGeometry());

    // Save the last current view in any case
    settings.setValue("CURRENT_VIEW", _currentView);

    // Save the current window state, but only if a system is connected (else no real number of widgets would be present))
    if (UASManager::instance()->getUASList().length() > 0) settings.setValue(getWindowStateKey(), saveState());

    // Save the current UAS view if a UAS is connected
    if (UASManager::instance()->getUASList().length() > 0) settings.setValue("CURRENT_VIEW_WITH_UAS_CONNECTED", _currentView);

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
    // TODO: What is this? What kind of "Video" is saved to bmp?
    QString format("bmp");
    QString initialPath = QDir::currentPath() + tr("/untitled.") + format;
    QString screenFileName = QGCFileDialog::getSaveFileName(
        this, tr("Save Video Capture"),
        initialPath,
        tr("%1 Files (*.%2);;All Files (*)")
        .arg(format.toUpper())
        .arg(format),
        format);
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

void MainWindow::enableAutoReconnect(bool enabled)
{
    autoReconnect = enabled;
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
    if (_currentView == VIEW_ENGINEER)
    {
        ui.actionEngineersView->setChecked(true);
        ui.actionEngineersView->activate(QAction::Trigger);
    }
    if (_currentView == VIEW_FLIGHT)
    {
        ui.actionFlightView->setChecked(true);
        ui.actionFlightView->activate(QAction::Trigger);
    }
    if (_currentView == VIEW_SIMULATION)
    {
        ui.actionSimulationView->setChecked(true);
        ui.actionSimulationView->activate(QAction::Trigger);
    }
    if (_currentView == VIEW_MISSION)
    {
        ui.actionMissionView->setChecked(true);
        ui.actionMissionView->activate(QAction::Trigger);
    }
    if (_currentView == VIEW_SETUP)
    {
        ui.actionSetup->setChecked(true);
        ui.actionSetup->activate(QAction::Trigger);
    }
    if (_currentView == VIEW_TERMINAL)
    {
        ui.actionTerminalView->setChecked(true);
        ui.actionTerminalView->activate(QAction::Trigger);
    }
    if (_currentView == VIEW_GOOGLEEARTH)
    {
        ui.actionGoogleEarthView->setChecked(true);
        ui.actionGoogleEarthView->activate(QAction::Trigger);
    }
    if (_currentView == VIEW_LOCAL3D)
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
    connect(ui.actionNewCustomWidget, SIGNAL(triggered()), this, SLOT(_createNewCustomWidget()));
    connect(ui.actionLoadCustomWidgetFile, SIGNAL(triggered()), this, SLOT(_loadCustomWidgetFromFile()));

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

// FIXME: Where is this called from
LinkInterface* MainWindow::addLink()
{
    SerialLink* link = new SerialLink();
    // TODO This should be only done in the dialog itself

    LinkManager::instance()->addLink(link);

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

void MainWindow::_addLinkMenu(LinkInterface *link)
{
    // Go fishing for this link's configuration window
    QList<QAction*> actions = ui.menuNetwork->actions();

    bool alreadyAdded = false;

    const int32_t& linkIndex(LinkManager::instance()->getLinks().indexOf(link));
    const int32_t& linkID(LinkManager::instance()->getLinks()[linkIndex]->getId());

    foreach (QAction* act, actions) {
        if (act->data().toInt() == linkID) {
            alreadyAdded = true;
            break;
        }
    }

    if (!alreadyAdded) {
        CommConfigurationWindow* commWidget = new CommConfigurationWindow(link, this);
        commsWidgetList.append(commWidget);
        connect(commWidget,SIGNAL(destroyed(QObject*)),this,SLOT(commsWidgetDestroyed(QObject*)));
        QAction* action = commWidget->getAction();
        ui.menuNetwork->addAction(action);
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
        restoreState(settings.value(getWindowStateKey()).toByteArray());
    }

}

void MainWindow::UASSpecsChanged(int uas)
{
    Q_UNUSED(uas);
    // TODO: Update UAS properties if its specs change
}

void MainWindow::UASCreated(UASInterface* uas)
{
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
    _showHILConfigurationWidgets();

    if (!linechartWidget)
    {
        linechartWidget = new Linecharts(this);
        linechartWidget->setVisible(false);
    }

    linechartWidget->addSource(mavlinkDecoder);
    if (_engineeringView != linechartWidget)
    {
        _engineeringView = linechartWidget;
    }

    // Reload view state in case new widgets were added
    _loadCurrentViewState();
}

void MainWindow::UASDeleted(UASInterface* uas)
{
    Q_UNUSED(uas);
    // TODO: Update the UI when a UAS is deleted
}

/// Stores the state of the toolbar, status bar and widgets associated with the current view
void MainWindow::_storeCurrentViewState(void)
{
    // HIL dock widgets are dynamic and are not part of the saved state
    _hideAllHilDockWidgets();
    
    // Save list of visible widgets

    bool firstWidget = true;
    QString widgetNames = "";
    foreach(QDockWidget* dockWidget, _mapName2DockWidget) {
        if (dockWidget->isVisible()) {
            if (!firstWidget) {
                widgetNames += ",";
            }
            widgetNames += dockWidget->objectName();
            firstWidget = false;
        }
    }
    
    settings.setValue(getWindowStateKey() + "WIDGETS", widgetNames);
    settings.setValue(getWindowStateKey(), saveState());
    settings.setValue(getWindowGeometryKey(), saveGeometry());
}

/// Restores the state of the toolbar, status bar and widgets associated with the current view
void MainWindow::_loadCurrentViewState(void)
{
    QWidget* centerView = NULL;
    QString defaultWidgets;
    
    switch (_currentView) {
        case VIEW_SETUP:
            _buildSetupView();
            centerView = _setupView;
            break;
            
        case VIEW_ENGINEER:
            _buildEngineeringView();
            centerView = _engineeringView;
            defaultWidgets = "MAVLINK_INSPECTOR_DOCKWIDGET,PARAMETER_INTERFACE_DOCKWIDGET,FILE_VIEW_DOCKWIDGET,HEAD_UP_DISPLAY_DOCKWIDGET";
            break;
            
        case VIEW_FLIGHT:
            _buildPilotView();
            centerView = _pilotView;
            defaultWidgets = "COMMUNICATION_CONSOLE_DOCKWIDGET,UAS_INFO_INFOVIEW_DOCKWIDGET";
            break;
            
        case VIEW_MISSION:
            _buildPlannerView();
            centerView = _plannerView;
            defaultWidgets = "UNMANNED_SYSTEM_LIST_DOCKWIDGET,WAYPOINT_LIST_DOCKWIDGET";
            break;
            
        case VIEW_SIMULATION:
            _buildSimView();
            centerView = _simView;
            defaultWidgets = "UNMANNED_SYSTEM_CONTROL_DOCKWIDGET,WAYPOINT_LIST_DOCKWIDGET,PARAMETER_INTERFACE_DOCKWIDGET,PRIMARY_FLIGHT_DISPLAY_DOCKWIDGET";
            break;
            
        case VIEW_TERMINAL:
            _buildTerminalView();
            centerView = _terminalView;
            break;
            
        case VIEW_GOOGLEEARTH:
            _buildGoogleEarthView();
            centerView = _googleEarthView;
            break;
            
        case VIEW_LOCAL3D:
            _buildLocal3DView();
            centerView = _local3DView;
            break;
    }
    
    // Remove old view
    if (_currentViewWidget) {
        _currentViewWidget->setVisible(false);
        Q_ASSERT(_centralLayout->count() == 1);
        QLayoutItem *child = _centralLayout->takeAt(0);
        Q_ASSERT(child);
        delete child;
    }
    
    // Add the new one
    Q_ASSERT(centerView);
    Q_ASSERT(_centralLayout->count() == 0);
    _currentViewWidget = centerView;
    _centralLayout->addWidget(_currentViewWidget);
    _currentViewWidget->setVisible(true);
    
    // Hide all widgets from previous view
    _hideAllDockWidgets();

    // Restore the widgets for the new view
    QString widgetNames = settings.value(getWindowStateKey() + "WIDGETS", defaultWidgets).toString();
    if (!widgetNames.isEmpty()) {
        QStringList split = widgetNames.split(",");
        foreach (QString widgetName, split) {
            Q_ASSERT(!widgetName.isEmpty());
            _showDockWidget(widgetName, true);
        }
    }

    if (settings.contains(getWindowStateKey())) {
        restoreState(settings.value(getWindowStateKey()).toByteArray());
    }
    
    // HIL dock widget are dynamic and don't take part in the saved window state, so this
    // need to happen after we restore state
    _showHILConfigurationWidgets();
}

void MainWindow::_hideAllHilDockWidgets(void)
{
    foreach(QDockWidget* dockWidget, _mapUasId2HilDockWidget) {
        dockWidget->setVisible(false);
    }
}

void MainWindow::_hideAllDockWidgets(void)
{
    foreach(QDockWidget* dockWidget, _mapName2DockWidget) {
        dockWidget->setVisible(false);
    }
    
    _hideAllHilDockWidgets();
}

void MainWindow::_showDockWidgetAction(bool show)
{
    QAction* action = dynamic_cast<QAction*>(QObject::sender());
    Q_ASSERT(action);
    
    _showDockWidget(action->data().toString(), show);
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
    if (_currentView != VIEW_ENGINEER)
    {
        _storeCurrentViewState();
        _currentView = VIEW_ENGINEER;
        ui.actionEngineersView->setChecked(true);
        _loadCurrentViewState();
    }
}

void MainWindow::loadOperatorView()
{
    if (_currentView != VIEW_MISSION)
    {
        _storeCurrentViewState();
        _currentView = VIEW_MISSION;
        ui.actionMissionView->setChecked(true);
        _loadCurrentViewState();
    }
}
void MainWindow::loadSetupView()
{
    if (_currentView != VIEW_SETUP)
    {
        _storeCurrentViewState();
        _currentView = VIEW_SETUP;
        ui.actionSetup->setChecked(true);
        _loadCurrentViewState();
    }
}

void MainWindow::loadTerminalView()
{
    if (_currentView != VIEW_TERMINAL)
    {
        _storeCurrentViewState();
        _currentView = VIEW_TERMINAL;
        ui.actionTerminalView->setChecked(true);
        _loadCurrentViewState();
    }
}

void MainWindow::loadGoogleEarthView()
{
    if (_currentView != VIEW_GOOGLEEARTH)
    {
        _storeCurrentViewState();
        _currentView = VIEW_GOOGLEEARTH;
        ui.actionGoogleEarthView->setChecked(true);
        _loadCurrentViewState();
    }
}

void MainWindow::loadLocal3DView()
{
    if (_currentView != VIEW_LOCAL3D)
    {
        _storeCurrentViewState();
        _currentView = VIEW_LOCAL3D;
        ui.actionLocal3DView->setChecked(true);
        _loadCurrentViewState();
    }
}

void MainWindow::loadPilotView()
{
    if (_currentView != VIEW_FLIGHT)
    {
        _storeCurrentViewState();
        _currentView = VIEW_FLIGHT;
        ui.actionFlightView->setChecked(true);
        _loadCurrentViewState();
    }
}

void MainWindow::loadSimulationView()
{
    if (_currentView != VIEW_SIMULATION)
    {
        _storeCurrentViewState();
        _currentView = VIEW_SIMULATION;
        ui.actionSimulationView->setChecked(true);
        _loadCurrentViewState();
    }
}

QList<QAction*> MainWindow::listLinkMenuActions()
{
    return ui.menuNetwork->actions();
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
