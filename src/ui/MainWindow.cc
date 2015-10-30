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
#include <QQuickView>
#include <QDesktopWidget>
#include <QScreen>
#include <QDesktopServices>
#include <QDockWidget>
#include <QMenuBar>

#include "QGC.h"
#include "MAVLinkProtocol.h"
#include "MainWindow.h"
#include "GAudioOutput.h"
#include "QGCMAVLinkLogPlayer.h"
#include "SettingsDialog.h"
#include "MAVLinkDecoder.h"
#include "QGCApplication.h"
#include "QGCFileDialog.h"
#include "QGCMessageBox.h"
#include "MultiVehicleManager.h"
#include "HomePositionManager.h"
#include "LogCompressor.h"
#include "UAS.h"

#ifndef __mobile__
#include "QGCDataPlot2D.h"
#include "Linecharts.h"
#include "QGCUASFileViewMulti.h"
#include "UASQuickView.h"
#include "QGCTabbedInfoView.h"
#include "CustomCommandWidget.h"
#include "QGCDockWidget.h"
#include "UASInfoWidget.h"
#include "HILDockWidget.h"
#endif

#ifndef __ios__
#include "SerialLink.h"
#endif

#ifdef UNITTEST_BUILD
#include "QmlControls/QmlTestWidget.h"
#endif

#ifdef QGC_OSG_ENABLED
#include "Q3DWidgetFactory.h"
#endif


/// The key under which the Main Window settings are saved
const char* MAIN_SETTINGS_GROUP = "QGC_MAINWINDOW";

#ifndef __mobile__
enum DockWidgetTypes { 
    MAVLINK_INSPECTOR, 
    CUSTOM_COMMAND, 
    ONBOARD_FILES,
    STATUS_DETAILS,
    INFO_VIEW,
    HIL_CONFIG,
    ANALYZE
};

static const char *rgDockWidgetNames[] = {
    "MAVLink Inspector",
    "Custom Command",
    "Onboard Files",
    "Status Details",
    "Info View",
    "HIL Config",
    "Analyze"
};

#define ARRAY_SIZE(ARRAY) (sizeof(ARRAY) / sizeof(ARRAY[0]))

static const char* _visibleWidgetsKey = "VisibleWidgets";
#endif

static MainWindow* _instance = NULL;   ///< @brief MainWindow singleton

MainWindow* MainWindow::_create()
{
    Q_ASSERT(_instance == NULL);
    new MainWindow();
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
MainWindow::MainWindow()
    : _autoReconnect(false)
    , _lowPowerMode(false)
    , _showStatusBar(false)
    , _mainQmlWidgetHolder(NULL)
{
    Q_ASSERT(_instance == NULL);
    _instance = this;

    // Qt 4/5 on Ubuntu does place the native menubar correctly so on Linux we revert back to in-window menu bar.
#ifdef Q_OS_LINUX
    menuBar()->setNativeMenuBar(false);
#endif
    // Setup user interface
    loadSettings();
    emit initStatusChanged(tr("Setting up user interface"), Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));

    _ui.setupUi(this);
    // Make sure tool bar elements all fit before changing minimum width
    setMinimumWidth(1008);
    configureWindowName();

    // Setup central widget with a layout to hold the views
    _centralLayout = new QVBoxLayout();
    _centralLayout->setContentsMargins(0, 0, 0, 0);
    centralWidget()->setLayout(_centralLayout);

    _mainQmlWidgetHolder = new QGCQmlWidgetHolder(QString(), NULL, this);
    _centralLayout->addWidget(_mainQmlWidgetHolder);
    _mainQmlWidgetHolder->setVisible(true);

    _mainQmlWidgetHolder->setContextPropertyObject("controller", this);
    _mainQmlWidgetHolder->setSource(QUrl::fromUserInput("qrc:qml/MainWindow.qml"));

    // Set dock options
    setDockOptions(0);
    // Setup corners
    setCorner(Qt::BottomRightCorner, Qt::BottomDockWidgetArea);

    // On Mobile devices, we don't want any main menus at all.
#ifdef __mobile__
    menuBar()->setNativeMenuBar(false);
#endif

#ifdef UNITTEST_BUILD
    QAction* qmlTestAction = new QAction("Test QML palette and controls", NULL);
    connect(qmlTestAction, &QAction::triggered, this, &MainWindow::_showQmlTestWidget);
    _ui.menuWidgets->addAction(qmlTestAction);
#endif

    // Status Bar
    setStatusBar(new QStatusBar(this));
    statusBar()->setSizeGripEnabled(true);

#ifndef __mobile__
    emit initStatusChanged(tr("Building common widgets."), Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));
    _buildCommonWidgets();
    emit initStatusChanged(tr("Building common actions"), Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));
#endif

    // Create actions
    connectCommonActions();
    // Connect user interface devices
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

    // These also cause the screen to redraw so we need to update any OpenGL canvases in QML controls
    connect(qgcApp()->toolbox()->linkManager(), &LinkManager::linkConnected,    this, &MainWindow::_linkStateChange);
    connect(qgcApp()->toolbox()->linkManager(), &LinkManager::linkDisconnected, this, &MainWindow::_linkStateChange);

    // Connect link
    if (_autoReconnect)
    {
        restoreLastUsedConnection();
    }

    // Set low power mode
    enableLowPowerMode(_lowPowerMode);
    emit initStatusChanged(tr("Restoring last view state"), Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));

#ifndef __mobile__

    // Restore the window position and size
    emit initStatusChanged(tr("Restoring last window size"), Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));
    if (settings.contains(_getWindowGeometryKey()))
    {
        restoreGeometry(settings.value(_getWindowGeometryKey()).toByteArray());
    }
    else
    {
        // Adjust the size
        QScreen* scr = QApplication::primaryScreen();
        QSize scrSize = scr->availableSize();
        if (scrSize.width() <= 1280)
        {
            resize(scrSize.width(), scrSize.height());
        }
        else
        {
            int w = scrSize.width()  > 1600 ? 1600 : scrSize.width();
            int h = scrSize.height() >  800 ?  800 : scrSize.height();
            resize(w, h);
            move((scrSize.width() - w) / 2, (scrSize.height() - h) / 2);
        }
    }

    // Make sure the proper fullscreen/normal menu item is checked properly.
    _ui.actionFullscreen->setChecked(isFullScreen());
    _ui.actionNormal->setChecked(!isFullScreen());

    // And that they will stay checked properly after user input
    connect(_ui.actionFullscreen, &QAction::triggered, this, &MainWindow::fullScreenActionItemCallback);
    connect(_ui.actionNormal,     &QAction::triggered, this, &MainWindow::normalActionItemCallback);
#endif

    connect(_ui.actionStatusBar,  &QAction::triggered, this, &MainWindow::showStatusBarCallback);

    // Set OS dependent keyboard shortcuts for the main window, non OS dependent shortcuts are set in MainWindow.ui
#ifdef Q_OS_MACX
    _ui.actionSetup->setShortcut(QApplication::translate("MainWindow", "Meta+1", 0));
    _ui.actionPlan->setShortcut(QApplication::translate("MainWindow", "Meta+2", 0));
    _ui.actionFlight->setShortcut(QApplication::translate("MainWindow", "Meta+3", 0));
    _ui.actionFullscreen->setShortcut(QApplication::translate("MainWindow", "Meta+Return", 0));
#else
    _ui.actionSetup->setShortcut(QApplication::translate("MainWindow", "Ctrl+1", 0));
    _ui.actionPlan->setShortcut(QApplication::translate("MainWindow", "Ctrl+2", 0));
    _ui.actionFlight->setShortcut(QApplication::translate("MainWindow", "Ctrl+3", 0));
    _ui.actionFullscreen->setShortcut(QApplication::translate("MainWindow", "Ctrl+Return", 0));
#endif

    _ui.actionFlight->setChecked(true);

    connect(&windowNameUpdateTimer, SIGNAL(timeout()), this, SLOT(configureWindowName()));
    windowNameUpdateTimer.start(15000);
    emit initStatusChanged(tr("Done"), Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));

    if (!qgcApp()->runningUnitTests()) {
        _ui.actionStatusBar->setChecked(_showStatusBar);
        showStatusBarCallback(_showStatusBar);
#ifdef __mobile__
        menuBar()->hide();
#endif
        show();
#ifdef Q_OS_MAC
        // TODO HACK
        // This is a really ugly hack. For whatever reason, by having a QQuickWidget inside a
        // QDockWidget (MainToolBar above), the main menu is not shown when the app first
        // starts. I looked everywhere and I could not find a solution. What I did notice was
        // that if any other window gets focus, the menu comes up when you come back to QGC.
        // That is, if you were to click on another window and then back to QGC, the menus
        // would appear. This hack below creates a 0x0 dialog and immediately closes it.
        // That works around the issue and it will do until I find the root of the problem.
        QDialog qd(this);
        qd.show();
        qd.raise();
        qd.activateWindow();
        qd.close();
#endif
    }

#ifndef __mobile__
    _loadVisibleWidgetsSettings();
#endif
}

MainWindow::~MainWindow()
{
    _instance = NULL;
}

QString MainWindow::_getWindowGeometryKey()
{
    return "_geometry";
}

#ifndef __mobile__
void MainWindow::_buildCommonWidgets(void)
{
    // Add generic MAVLink decoder
    // TODO: This is never deleted
    mavlinkDecoder = new MAVLinkDecoder(qgcApp()->toolbox()->mavlinkProtocol(), this);
    connect(mavlinkDecoder, SIGNAL(valueChanged(int,QString,QString,QVariant,quint64)),
                      this, SIGNAL(valueChanged(int,QString,QString,QVariant,quint64)));

    // Log player
    // TODO: Make this optional with a preferences setting or under a "View" menu
    logPlayer = new QGCMAVLinkLogPlayer(statusBar());
    statusBar()->addPermanentWidget(logPlayer);

    for (int i = 0, end = ARRAY_SIZE(rgDockWidgetNames); i < end; i++) {

        const char* pDockWidgetName = rgDockWidgetNames[i];

        // Add to menu
        QAction* action = new QAction(tr(pDockWidgetName), NULL);
        action->setCheckable(true);
        action->setData(i);
        connect(action, &QAction::triggered, this, &MainWindow::_showDockWidgetAction);
        _ui.menuWidgets->addAction(action);
        _mapName2Action[pDockWidgetName] = action;
    }
}

/// Shows or hides the specified dock widget, creating if necessary
void MainWindow::_showDockWidget(const QString& name, bool show)
{
    // Create the inner widget if we need to
    if (!_mapName2DockWidget.contains(name)) {
        _createInnerDockWidget(name);
    }

    Q_ASSERT(_mapName2DockWidget.contains(name));
    QGCDockWidget* dockWidget = _mapName2DockWidget[name];
    Q_ASSERT(dockWidget);

    dockWidget->setVisible(show);

    Q_ASSERT(_mapName2Action.contains(name));
    _mapName2Action[name]->setChecked(show);
}

/// Creates the specified inner dock widget and adds to the QDockWidget
void MainWindow::_createInnerDockWidget(const QString& widgetName)
{
    QGCDockWidget* widget = NULL;
    QAction *action = _mapName2Action[widgetName];
    
    switch(action->data().toInt()) {
        case MAVLINK_INSPECTOR:
            widget = new QGCMAVLinkInspector(widgetName, action, qgcApp()->toolbox()->mavlinkProtocol(),this);
            break;
        case CUSTOM_COMMAND:
            widget = new CustomCommandWidget(widgetName, action, this);
            break;
        case ONBOARD_FILES:
            widget = new QGCUASFileViewMulti(widgetName, action, this);
            break;
        case STATUS_DETAILS:
            widget = new UASInfoWidget(widgetName, action, this);
            break;
        case HIL_CONFIG:
            widget = new HILDockWidget(widgetName, action, this);
            break;
        case ANALYZE:
            widget = new Linecharts(widgetName, action, mavlinkDecoder, this);
            break;
        case INFO_VIEW:
            widget= new QGCTabbedInfoView(widgetName, action, this);
            break;
    }

    if(action->data().toInt() == INFO_VIEW) {
        qobject_cast<QGCTabbedInfoView*>(widget)->addSource(mavlinkDecoder);
    }
    _mapName2DockWidget[widgetName] = widget;
}

void MainWindow::_hideAllDockWidgets(void)
{
    foreach(QGCDockWidget* dockWidget, _mapName2DockWidget) {
        dockWidget->setVisible(false);
    }
}

void MainWindow::_showDockWidgetAction(bool show)
{
    QAction* action = qobject_cast<QAction*>(QObject::sender());
    Q_ASSERT(action);
    _showDockWidget(rgDockWidgetNames[action->data().toInt()], show);
}
#endif

void MainWindow::fullScreenActionItemCallback(bool)
{
    if (!_ui.actionFullscreen->isChecked())
        _ui.actionFullscreen->setChecked(true);
    _ui.actionNormal->setChecked(false);
}

void MainWindow::normalActionItemCallback(bool)
{
    if (!_ui.actionNormal->isChecked())
        _ui.actionNormal->setChecked(true);
    _ui.actionFullscreen->setChecked(false);
}

void MainWindow::showStatusBarCallback(bool checked)
{
    _showStatusBar = checked;
    checked ? statusBar()->show() : statusBar()->hide();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Disallow window close if there are active connections
    if (qgcApp()->toolbox()->linkManager()->anyConnectedLinks()) {
        QGCMessageBox::StandardButton button =
            QGCMessageBox::warning(
                tr("QGroundControl close"),
                tr("There are still active connections to vehicles. Do you want to disconnect these before closing?"),
                QMessageBox::Yes | QMessageBox::Cancel,
                QMessageBox::Cancel);
        if (button == QMessageBox::Yes) {
            qgcApp()->toolbox()->linkManager()->disconnectAll();
            // The above disconnect causes a flurry of activity as the vehicle components are removed. This in turn
            // causes the Windows Version of Qt to crash if you allow the close event to be accepted. In order to prevent
            // the crash, we ignore the close event and setup a delayed timer to close the window after things settle down.
            QTimer::singleShot(1500, this, &MainWindow::_closeWindow);
        }

        event->ignore();
        return;
    }

    // This will process any remaining flight log save dialogs
    qgcApp()->processEvents(QEventLoop::ExcludeUserInputEvents);

    // Should not be any active connections
    Q_ASSERT(!qgcApp()->toolbox()->linkManager()->anyConnectedLinks());

    // We have to pull out the QmlWidget from the main window and delete it here, before
    // the MainWindow ends up getting deleted. Otherwise the Qml has a reference to MainWindow
    // inside it which in turn causes a shutdown crash.
    _centralLayout->removeWidget(_mainQmlWidgetHolder);
    delete _mainQmlWidgetHolder;
    _mainQmlWidgetHolder = NULL;

    _storeCurrentViewState();
    storeSettings();

    event->accept();

    _instance = NULL;
    emit mainWindowClosed();
}

void MainWindow::loadSettings()
{
    // Why the screaming?
    QSettings settings;
    settings.beginGroup(MAIN_SETTINGS_GROUP);
    _autoReconnect  = settings.value("AUTO_RECONNECT",      _autoReconnect).toBool();
    _lowPowerMode   = settings.value("LOW_POWER_MODE",      _lowPowerMode).toBool();
    _showStatusBar  = settings.value("SHOW_STATUSBAR",      _showStatusBar).toBool();
    settings.endGroup();
}

void MainWindow::storeSettings()
{
    QSettings settings;
    settings.beginGroup(MAIN_SETTINGS_GROUP);
    settings.setValue("AUTO_RECONNECT",     _autoReconnect);
    settings.setValue("LOW_POWER_MODE",     _lowPowerMode);
    settings.setValue("SHOW_STATUSBAR",     _showStatusBar);
    settings.endGroup();
    settings.setValue(_getWindowGeometryKey(), saveGeometry());

#ifndef __mobile__
    _storeVisibleWidgetsSettings();
#endif
}

void MainWindow::configureWindowName()
{
    QList<QHostAddress> hostAddresses = QNetworkInterface::allAddresses();
    QString windowname = qApp->applicationName() + " " + qApp->applicationVersion();

    // XXX we do have UDP MAVLink heartbeat broadcast now in SITL and will have it on the
    // WIFI radio, so people should not be in need any more of knowing their IP.
    // this can go once we are certain its not needed any more.
    #if 0
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
    #endif
    setWindowTitle(windowname);
}

void MainWindow::enableAutoReconnect(bool enabled)
{
    _autoReconnect = enabled;
}

/**
* @brief Create all actions associated to the main window
*
**/
void MainWindow::connectCommonActions()
{
    // Connect actions from ui
    connect(_ui.actionAdd_Link, SIGNAL(triggered()), this, SLOT(manageLinks()));

    // Audio output
    _ui.actionMuteAudioOutput->setChecked(qgcApp()->toolbox()->audioOutput()->isMuted());
    connect(qgcApp()->toolbox()->audioOutput(), SIGNAL(mutedChanged(bool)), _ui.actionMuteAudioOutput, SLOT(setChecked(bool)));
    connect(_ui.actionMuteAudioOutput, SIGNAL(triggered(bool)), qgcApp()->toolbox()->audioOutput(), SLOT(mute(bool)));

    // Application Settings
    connect(_ui.actionSettings, SIGNAL(triggered()), this, SLOT(showSettings()));

    // Views actions
    connect(_ui.actionFlight,   &QAction::triggered,    this, &MainWindow::showFlyView);
    connect(_ui.actionPlan,     &QAction::triggered,    this, &MainWindow::showPlanView);
    connect(_ui.actionSetup,    &QAction::triggered,    this, &MainWindow::showSetupView);
    connect(_ui.actionFlight,   &QAction::triggered,    this, &MainWindow::handleActiveViewActionState);
    connect(_ui.actionPlan,     &QAction::triggered,    this, &MainWindow::handleActiveViewActionState);
    connect(_ui.actionSetup,    &QAction::triggered,    this, &MainWindow::handleActiveViewActionState);

    // Connect internal actions
    connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::vehicleAdded, this, &MainWindow::_vehicleAdded);
}

void MainWindow::handleActiveViewActionState(bool triggered)
{
    Q_UNUSED(triggered);
    QAction *triggeredAction = qobject_cast<QAction*>(sender());
    _ui.actionFlight->setChecked(triggeredAction == _ui.actionFlight);
    _ui.actionPlan->setChecked(triggeredAction == _ui.actionPlan);
    _ui.actionSetup->setChecked(triggeredAction == _ui.actionSetup);
}

void MainWindow::_openUrl(const QString& url, const QString& errorMessage)
{
    if(!QDesktopServices::openUrl(QUrl(url))) {
        QMessageBox::critical(
            this,
            tr("Could not open information in browser"),
            errorMessage);
    }
}

void MainWindow::showSettings()
{
    SettingsDialog settings(qgcApp()->toolbox()->audioOutput(), qgcApp()->toolbox()->flightMapSettings(), this);
    settings.exec();
}

void MainWindow::_vehicleAdded(Vehicle* vehicle)
{
    connect(vehicle->uas(), SIGNAL(valueChanged(int,QString,QString,QVariant,quint64)), this, SIGNAL(valueChanged(int,QString,QString,QVariant,quint64)));
}

/// Stores the state of the toolbar, status bar and widgets associated with the current view
void MainWindow::_storeCurrentViewState(void)
{
#ifndef __mobile__
    foreach(QGCDockWidget* dockWidget, _mapName2DockWidget) {
        dockWidget->saveSettings();
    }
#endif

    settings.setValue(_getWindowGeometryKey(), saveGeometry());
}

void MainWindow::manageLinks()
{
    SettingsDialog settings(qgcApp()->toolbox()->audioOutput(), qgcApp()->toolbox()->flightMapSettings(), this, SettingsDialog::ShowCommLinks);
    settings.exec();
}

/// @brief Saves the last used connection
void MainWindow::saveLastUsedConnection(const QString connection)
{
    QSettings settings;
    QString key(MAIN_SETTINGS_GROUP);
    key += "/LAST_CONNECTION";
    settings.setValue(key, connection);
}

/// @brief Restore (and connects) the last used connection (if any)
void MainWindow::restoreLastUsedConnection()
{
    // TODO This should check and see of the port/whatever is present
    // first. That is, if the last connection was to a PX4 on some serial
    // port, it should check and see if the port is present before making
    // the connection.
    QSettings settings;
    QString key(MAIN_SETTINGS_GROUP);
    key += "/LAST_CONNECTION";
    if(settings.contains(key)) {
        QString connection = settings.value(key).toString();
        // Create a link for it
        qgcApp()->toolbox()->linkManager()->createConnectedLink(connection);
    }
}

void MainWindow::_linkStateChange(LinkInterface*)
{
    emit repaintCanvas();
}

#ifdef QGC_MOUSE_ENABLED_LINUX
bool MainWindow::x11Event(XEvent *event)
{
    emit x11EventOccured(event);
    return false;
}
#endif // QGC_MOUSE_ENABLED_LINUX

#ifdef UNITTEST_BUILD
void MainWindow::_showQmlTestWidget(void)
{
    new QmlTestWidget();
}
#endif

#ifndef __mobile__
void MainWindow::_loadVisibleWidgetsSettings(void)
{
    QSettings settings;

    QString widgets = settings.value(_visibleWidgetsKey).toString();

    if (!widgets.isEmpty()) {
        QStringList nameList = widgets.split(",");

        foreach (const QString &name, nameList) {
            _showDockWidget(name, true);
        }
    }
}

void MainWindow::_storeVisibleWidgetsSettings(void)
{
    QString widgetNames;
    bool firstWidget = true;

    foreach (const QString &name, _mapName2DockWidget.keys()) {
        if (_mapName2DockWidget[name]->isVisible()) {
            if (!firstWidget) {
                widgetNames += ",";
            } else {
                firstWidget = false;
            }

            widgetNames += name;
        }
    }

    QSettings settings;

    settings.setValue(_visibleWidgetsKey, widgetNames);
}
#endif
