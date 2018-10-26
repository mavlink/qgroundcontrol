/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
#include <QDialog>

#include "QGC.h"
#include "MAVLinkProtocol.h"
#include "MainWindow.h"
#include "AudioOutput.h"
#ifndef __mobile__
#include "QGCMAVLinkLogPlayer.h"
#endif
#include "MAVLinkDecoder.h"
#include "QGCApplication.h"
#include "MultiVehicleManager.h"
#include "LogCompressor.h"
#include "UAS.h"
#include "QGCImageProvider.h"
#include "QGCCorePlugin.h"

#ifndef __mobile__
#include "Linecharts.h"
#include "QGCUASFileViewMulti.h"
#include "CustomCommandWidget.h"
#include "QGCDockWidget.h"
#include "HILDockWidget.h"
#include "AppMessages.h"
#endif

#ifndef NO_SERIAL_LINK
#include "SerialLink.h"
#endif

#ifdef UNITTEST_BUILD
#include "QmlControls/QmlTestWidget.h"
#endif

/// The key under which the Main Window settings are saved
const char* MAIN_SETTINGS_GROUP = "QGC_MAINWINDOW";

#ifndef __mobile__
enum DockWidgetTypes {
    MAVLINK_INSPECTOR,
    CUSTOM_COMMAND,
    ONBOARD_FILES,
    HIL_CONFIG,
    ANALYZE
};

static const char *rgDockWidgetNames[] = {
    "MAVLink Inspector",
    "Custom Command",
    "Onboard Files",
    "HIL Config",
    "Analyze"
};

#define ARRAY_SIZE(ARRAY) (sizeof(ARRAY) / sizeof(ARRAY[0]))

static const char* _visibleWidgetsKey = "VisibleWidgets";
#endif

static MainWindow* _instance = NULL;   ///< @brief MainWindow singleton

MainWindow* MainWindow::_create()
{
    new MainWindow();
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
    : _mavlinkDecoder       (NULL)
    , _lowPowerMode         (false)
    , _showStatusBar        (false)
    , _mainQmlWidgetHolder  (NULL)
    , _forceClose           (false)
{
    _instance = this;

    //-- Load fonts
    if(QFontDatabase::addApplicationFont(":/fonts/opensans") < 0) {
        qWarning() << "Could not load /fonts/opensans font";
    }
    if(QFontDatabase::addApplicationFont(":/fonts/opensans-demibold") < 0) {
        qWarning() << "Could not load /fonts/opensans-demibold font";
    }

    // Qt 4/5 on Ubuntu does place the native menubar correctly so on Linux we revert back to in-window menu bar.
#ifdef Q_OS_LINUX
    menuBar()->setNativeMenuBar(false);
#endif
    // Setup user interface
    loadSettings();
    emit initStatusChanged(tr("Setting up user interface"), Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));

    _ui.setupUi(this);
    // Make sure tool bar elements all fit before changing minimum width
    setMinimumWidth(1024);
    setMinimumHeight(620);
    configureWindowName();

    // Setup central widget with a layout to hold the views
    _centralLayout = new QVBoxLayout();
    _centralLayout->setContentsMargins(0, 0, 0, 0);
    centralWidget()->setLayout(_centralLayout);

    _mainQmlWidgetHolder = new QGCQmlWidgetHolder(QString(), NULL, this);
    _centralLayout->addWidget(_mainQmlWidgetHolder);
    _mainQmlWidgetHolder->setVisible(true);

    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    _mainQmlWidgetHolder->setContextPropertyObject("controller", this);
    _mainQmlWidgetHolder->setContextPropertyObject("debugMessageModel", AppMessages::getModel());
    _mainQmlWidgetHolder->setSource(QUrl::fromUserInput("qrc:qml/MainWindowHybrid.qml"));

    // Image provider
    QQuickImageProvider* pImgProvider = dynamic_cast<QQuickImageProvider*>(qgcApp()->toolbox()->imageProvider());
    _mainQmlWidgetHolder->getEngine()->addImageProvider(QStringLiteral("QGCImages"), pImgProvider);

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

    connect(qgcApp()->toolbox()->corePlugin(), &QGCCorePlugin::showAdvancedUIChanged, this, &MainWindow::_showAdvancedUIChanged);
    _showAdvancedUIChanged(qgcApp()->toolbox()->corePlugin()->showAdvancedUI());

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
#endif

    connect(_ui.actionStatusBar,  &QAction::triggered, this, &MainWindow::showStatusBarCallback);

    connect(&windowNameUpdateTimer, &QTimer::timeout, this, &MainWindow::configureWindowName);
    windowNameUpdateTimer.start(15000);
    emit initStatusChanged(tr("Done"), Qt::AlignLeft | Qt::AlignBottom, QColor(62, 93, 141));

    if (!qgcApp()->runningUnitTests()) {
        _ui.actionStatusBar->setChecked(_showStatusBar);
        showStatusBarCallback(_showStatusBar);
#ifdef __mobile__
        menuBar()->hide();
#endif
        show();
    }

#ifndef __mobile__
    _loadVisibleWidgetsSettings();
#endif
    //-- Enable message handler display of messages in main window
    UASMessageHandler* msgHandler = qgcApp()->toolbox()->uasMessageHandler();
    if(msgHandler) {
        msgHandler->showErrorsInToolbar();
    }
}

MainWindow::~MainWindow()
{
    if (_mavlinkDecoder) {
        // Enforce thread-safe shutdown of the mavlink decoder
        _mavlinkDecoder->finish();
        _mavlinkDecoder->wait(1000);
        _mavlinkDecoder->deleteLater();
        _mavlinkDecoder = NULL;
    }

    // This needs to happen before we get into the QWidget dtor
    // otherwise  the QML engine reads freed data and tries to
    // destroy MainWindow a second time.
    delete _mainQmlWidgetHolder;
    _instance = NULL;
}

QString MainWindow::_getWindowGeometryKey()
{
    return "_geometry";
}

#ifndef __mobile__
MAVLinkDecoder* MainWindow::_mavLinkDecoderInstance(void)
{
    if (!_mavlinkDecoder) {
        _mavlinkDecoder = new MAVLinkDecoder(qgcApp()->toolbox()->mavlinkProtocol());
        connect(_mavlinkDecoder, &MAVLinkDecoder::valueChanged, this, &MainWindow::valueChanged);
    }

    return _mavlinkDecoder;
}

void MainWindow::_buildCommonWidgets(void)
{
    // Log player
    // TODO: Make this optional with a preferences setting or under a "View" menu
    logPlayer = new QGCMAVLinkLogPlayer(statusBar());
    statusBar()->addPermanentWidget(logPlayer);

    // Populate widget menu
    for (int i = 0, end = ARRAY_SIZE(rgDockWidgetNames); i < end; i++) {

        const char* pDockWidgetName = rgDockWidgetNames[i];

        // Add to menu
        QAction* action = new QAction(pDockWidgetName, this);
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
        if(!_createInnerDockWidget(name)) {
            qWarning() << "Trying to load non existent widget:" << name;
            return;
        }
    }
    Q_ASSERT(_mapName2DockWidget.contains(name));
    QGCDockWidget* dockWidget = _mapName2DockWidget[name];
    Q_ASSERT(dockWidget);
    dockWidget->setVisible(show);
    Q_ASSERT(_mapName2Action.contains(name));
    _mapName2Action[name]->setChecked(show);
}

/// Creates the specified inner dock widget and adds to the QDockWidget
bool MainWindow::_createInnerDockWidget(const QString& widgetName)
{
    QGCDockWidget* widget = NULL;
    QAction *action = _mapName2Action[widgetName];
    if(action) {
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
            case HIL_CONFIG:
                widget = new HILDockWidget(widgetName, action, this);
                break;
            case ANALYZE:
                widget = new Linecharts(widgetName, action, _mavLinkDecoderInstance(), this);
                break;
        }
        if(widget) {
            _mapName2DockWidget[widgetName] = widget;
        }
    }
    return widget != NULL;
}

void MainWindow::_hideAllDockWidgets(void)
{
    for(QGCDockWidget* dockWidget: _mapName2DockWidget) {
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

void MainWindow::showStatusBarCallback(bool checked)
{
    _showStatusBar = checked;
    checked ? statusBar()->show() : statusBar()->hide();
}

void MainWindow::_reallyClose(void)
{
    _forceClose = true;
    close();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!_forceClose) {
        // Attempt close from within the root Qml item
        qgcApp()->qmlAttemptWindowClose();
        event->ignore();
        return;
    }

    // Should not be any active connections
    if (qgcApp()->toolbox()->multiVehicleManager()->activeVehicle()) {
        qWarning() << "All links should be disconnected by now";
    }

    _storeCurrentViewState();
    storeSettings();

    emit mainWindowClosed();
}

void MainWindow::loadSettings()
{
    // Why the screaming?
    QSettings settings;
    settings.beginGroup(MAIN_SETTINGS_GROUP);
    _lowPowerMode   = settings.value("LOW_POWER_MODE",      _lowPowerMode).toBool();
    _showStatusBar  = settings.value("SHOW_STATUSBAR",      _showStatusBar).toBool();
    settings.endGroup();
}

void MainWindow::storeSettings()
{
    QSettings settings;
    settings.beginGroup(MAIN_SETTINGS_GROUP);
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
    setWindowTitle(qApp->applicationName() + " " + qApp->applicationVersion());
}

/**
* @brief Create all actions associated to the main window
*
**/
void MainWindow::connectCommonActions()
{
    // Connect internal actions
    connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::vehicleAdded, this, &MainWindow::_vehicleAdded);
    connect(this, &MainWindow::reallyClose, this, &MainWindow::_reallyClose, Qt::QueuedConnection); // Queued to allow closeEvent to fully unwind before _reallyClose is called
}

void MainWindow::_openUrl(const QString& url, const QString& errorMessage)
{
    if(!QDesktopServices::openUrl(QUrl(url))) {
        qgcApp()->showMessage(QString("Could not open information in browser: %1").arg(errorMessage));
    }
}

void MainWindow::_vehicleAdded(Vehicle* vehicle)
{
    connect(vehicle->uas(), &UAS::valueChanged, this, &MainWindow::valueChanged);
}

/// Stores the state of the toolbar, status bar and widgets associated with the current view
void MainWindow::_storeCurrentViewState(void)
{
#ifndef __mobile__
    for(QGCDockWidget* dockWidget: _mapName2DockWidget) {
        dockWidget->saveSettings();
    }
#endif

    settings.setValue(_getWindowGeometryKey(), saveGeometry());
}

/// @brief Saves the last used connection
void MainWindow::saveLastUsedConnection(const QString connection)
{
    QSettings settings;
    QString key(MAIN_SETTINGS_GROUP);
    key += "/LAST_CONNECTION";
    settings.setValue(key, connection);
}

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

        for (const QString &name: nameList) {
            _showDockWidget(name, true);
        }
    }
}

void MainWindow::_storeVisibleWidgetsSettings(void)
{
    QString widgetNames;
    bool firstWidget = true;

    for (const QString &name: _mapName2DockWidget.keys()) {
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

QObject* MainWindow::rootQmlObject(void)
{
    return _mainQmlWidgetHolder->getRootObject();
}

void MainWindow::_showAdvancedUIChanged(bool advanced)
{
    if (advanced) {
        menuBar()->addMenu(_ui.menuFile);
        menuBar()->addMenu(_ui.menuWidgets);
    } else {
        menuBar()->clear();
    }
}
