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
 *   @brief Implementation of main application window
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QSettings>
#include <QDockWidget>
#include <QNetworkInterface>
#include <QMessageBox>
#include <QDebug>
#include <QTimer>
#include <QHostInfo>

#include "MG.h"
#include "MAVLinkSimulationLink.h"
#include "SerialLink.h"
#include "UDPLink.h"
#include "MAVLinkProtocol.h"
#include "CommConfigurationWindow.h"
#include "WaypointList.h"
#include "MainWindow.h"
#include "JoystickWidget.h"
#include "GAudioOutput.h"


#include "LogCompressor.h"

/**
* Create new mainwindow. The constructor instantiates all parts of the user
* interface. It does NOT show the mainwindow. To display it, call the show()
* method.
*
* @see QMainWindow::show()
**/
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{

    // Quick hack
    //comp = new LogCompressor("/home/pixhawk/Desktop/test.txt");

    mavlink = new MAVLinkProtocol();
    //as4link = new AS4Protocol();

    //    MG::DISPLAY::setPixelSize(0.224f);

    // Setup user interface
    ui.setupUi(this);

    // Initialize views, NOT show them yet, only initialize model and controller
    centerStack = new QStackedWidget(this);
    linechart = new LinechartWidget(this);
    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), linechart, SLOT(setActivePlot(UASInterface*)));
    centerStack->addWidget(linechart);
    control = new UASControlWidget(this);
    //controlDock = new QDockWidget(this);
    //controlDock->setWidget(control);
    list = new UASListWidget(this);
    list->setVisible(false);
    waypoints = new WaypointList(this, NULL);
    waypoints->setVisible(false);
    info = new UASInfoWidget(this);
    info->setVisible(false);
    detection = new ObjectDetectionView("test", this);
    detection->setVisible(false);
    hud = new HUD(640, 480, this);
    hud->setVisible(false);
    debugConsole = new DebugConsole(this);
    debugConsole->setVisible(false);
    map = new MapWidget(this);
    map->setVisible(false);
    protocol = new XMLCommProtocolWidget(this);
    protocol->setVisible(false);
    parameters = new ParameterInterface(this);
    parameters->setVisible(false);

    QStringList* acceptList = new QStringList();
    acceptList->append("roll IMU");
    acceptList->append("pitch IMU");
    acceptList->append("yaw IMU");
    headDown1 = new HDDisplay(acceptList, this);
    headDown1->setVisible(false);

    QStringList* acceptList2 = new QStringList();
    acceptList2->append("Battery");
    acceptList2->append("Pressure");
    headDown2 = new HDDisplay(acceptList2, this);
    headDown2->setVisible(false);
    centerStack->addWidget(map);
    centerStack->addWidget(hud);
    setCentralWidget(centerStack);

    // Get IPs
    QList<QHostAddress> hostAddresses = QNetworkInterface::allAddresses();

    QString windowname = qApp->applicationName() + " " + qApp->applicationVersion();
    /*
    windowname.append(" (" + QHostInfo::localHostName() + ": ");
    bool prevAddr = false;
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
    */

    setWindowTitle(windowname);
#ifndef Q_WS_MAC
    //qApp->setWindowIcon(QIcon(":/core/images/qtcreator_logo_128.png"));
#endif

    // Add status bar
    setStatusBar(createStatusBar());
    // Load widgets
    loadWidgets();
    // Adjust the size
    adjustSize();

    // Create actions
    connectActions();

    // Set the application style (not the same as a style sheet)
    // Set the style to Plastique
    qApp->setStyle("plastique");
    // Set style sheet as last step
    reloadStylesheet();

    joystick = new JoystickInput();

    // HAS TO BE THE LAST ACTION
    // to make sure that all components are initialized when the
    // first messages arrive
    udpLink = new UDPLink(QHostAddress::Any, 14550);
    LinkManager::instance()->addProtocol(udpLink, mavlink);
    CommConfigurationWindow* commWidget = new CommConfigurationWindow(udpLink, mavlink, this);
    ui.menuNetwork->addAction(commWidget->getAction());
    udpLink->connect();

    simulationLink = new MAVLinkSimulationLink(MG::DIR::getSupportFilesDirectory() + "/demo-log.txt");
    //connect(simulationLink, SIGNAL(valueChanged(int,QString,double,quint64)), linechart, SLOT(appendData(int,QString,double,quint64)));
    LinkManager::instance()->addProtocol(simulationLink, mavlink);
    //CommConfigurationWindow* simulationWidget = new CommConfigurationWindow(simulationLink, mavlink, this);
    //ui.menuNetwork->addAction(commWidget->getAction());
    simulationLink->connect();
}

MainWindow::~MainWindow()
{
    delete statusBar;
    statusBar = NULL;
}

QStatusBar* MainWindow::createStatusBar()
{
    QStatusBar* bar = new QStatusBar();
    /* Add status fields and messages */
    /* Enable resize grip in the bottom right corner */
    bar->setSizeGripEnabled(true);
    return bar;
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
    videoTimer->setInterval(40);
    connect(videoTimer, SIGNAL(timeout()), this, SLOT(saveScreen()));
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

void MainWindow::reloadStylesheet()
{
    // Load style sheet
    //QFile styleSheet(MG::DIR::getSupportFilesDirectory() + "/images/style-mission.css");
    QFile styleSheet(":/images/style-mission.css");
    if (styleSheet.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString style = QString(styleSheet.readAll());
        style.replace("ICONDIR", MG::DIR::getIconDirectory());
        qApp->setStyleSheet(style);
    } else {
        qDebug() << "Style not set:" << styleSheet.fileName() << "opened: " << styleSheet.isOpen();
    }
}

void MainWindow::showStatusMessage(const QString& status, int timeout)
{
    statusBar->showMessage(status, timeout);
}

void MainWindow::setLastAction(QString status)
{
    showStatusMessage(status, 5);
}

void MainWindow::setLinkStatus(QString status)
{
    showStatusMessage(status, 15);
}

/**
* @brief Create all actions associated to the main window
*
**/
void MainWindow::connectActions()
{
    // Connect actions from ui
    connect(ui.actionAdd_Link, SIGNAL(triggered()), this, SLOT(addLink()));

    // Connect internal actions
    connect(UASManager::instance(), SIGNAL(UASCreated(UASInterface*)), this, SLOT(UASCreated(UASInterface*)));

    // Connect user interface controls
    connect(ui.actionLiftoff, SIGNAL(triggered()), UASManager::instance(), SLOT(launchActiveUAS()));
    connect(ui.actionLand, SIGNAL(triggered()), UASManager::instance(), SLOT(returnActiveUAS()));
    connect(ui.actionEmergency_Land, SIGNAL(triggered()), UASManager::instance(), SLOT(stopActiveUAS()));
    connect(ui.actionEmergency_Kill, SIGNAL(triggered()), UASManager::instance(), SLOT(killActiveUAS()));

    connect(ui.actionConfiguration, SIGNAL(triggered()), UASManager::instance(), SLOT(configureActiveUAS()));

    // User interface actions
    connect(ui.actionPilotView, SIGNAL(triggered()), this, SLOT(loadPilotView()));
    connect(ui.actionEngineerView, SIGNAL(triggered()), this, SLOT(loadEngineerView()));
    connect(ui.actionOperatorView, SIGNAL(triggered()), this, SLOT(loadOperatorView()));
    connect(ui.actionSettingsView, SIGNAL(triggered()), this, SLOT(loadSettingsView()));
    connect(ui.actionStyleConfig, SIGNAL(triggered()), this, SLOT(reloadStylesheet()));

    // Joystick configuration
    connect(ui.actionJoystickSettings, SIGNAL(triggered()), this, SLOT(configure()));
}

void MainWindow::configure()
{
    joystickWidget = new JoystickWidget(joystick, this);
}

void MainWindow::addLink()
{
    SerialLink* link = new SerialLink();
    // TODO This should be only done in the dialog itself

    LinkManager::instance()->addProtocol(link, mavlink);

    CommConfigurationWindow* commWidget = new CommConfigurationWindow(link, mavlink, this);

    ui.menuNetwork->addAction(commWidget->getAction());

    commWidget->show();

    // TODO Implement the link removal!
}

void MainWindow::UASCreated(UASInterface* uas)
{
    // Connect the UAS to the full user interface
    ui.menuConnected_Systems->addAction(QIcon(":/images/mavs/generic.svg"), tr("View ") + uas->getUASName(), uas, SLOT(setSelected()));

    // Line chart
    // FIXME DO THIS ONLY FOR THE FIRST CONNECTED SYSTEM
    static bool sysPresent = false;
    if (!sysPresent)
    {
        connect(uas, SIGNAL(valueChanged(int,QString,double,quint64)), linechart, SLOT(appendData(int,QString,double,quint64)), Qt::QueuedConnection);
        sysPresent = true;
    }

    // Health / System status indicator
    info->addUAS(uas);

    // UAS List
    list->addUAS(uas);

    // Camera view
    //camera->addUAS(uas);

    // Revalidate UI
    // TODO Stylesheet reloading should in theory not be necessary
    reloadStylesheet();
}

void MainWindow::clearView()
{ 
    // Halt HUD
    hud->stop();
    headDown1->stop();
    headDown2->stop();

    // Remove all dock widgets
    QList<QObject*> list = this->children();

    QList<QObject*>::iterator i;
    for (i = list.begin(); i != list.end(); ++i)
    {
        QDockWidget* widget = dynamic_cast<QDockWidget*>(*i);
        if (widget)
        {
            // Hide widgets
            QWidget* childWidget = dynamic_cast<QWidget*>(widget->widget());
            if (childWidget) childWidget->setVisible(false);
            // Remove dock widget
            this->removeDockWidget(widget);
            //delete widget;
        }
    }
}

void MainWindow::loadPilotView()
{
    clearView();
    GAudioOutput::instance()->say("Switched to Pilot View");

    // HEAD UP DISPLAY
    centerStack->setCurrentWidget(hud);
    hud->start();

    //connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), pfd, SLOT(setActiveUAS(UASInterface*)));
    QDockWidget* container1 = new QDockWidget(tr("Primary Flight Display"), this);
    container1->setWidget(headDown1);
    addDockWidget(Qt::RightDockWidgetArea, container1);

    QDockWidget* container2 = new QDockWidget(tr("Payload Status"), this);
    container2->setWidget(headDown2);
    addDockWidget(Qt::RightDockWidgetArea, container2);

    headDown1->start();
    headDown2->start();


    this->show();
}

void MainWindow::loadOperatorView()
{
    clearView();

    GAudioOutput::instance()->say("Switched to Operator View");

    // LINE CHART
    centerStack->setCurrentWidget(map);

    // UAS CONTROL
    QDockWidget* container1 = new QDockWidget(tr("Control"), this);
    container1->setWidget(control);
    addDockWidget(Qt::LeftDockWidgetArea, container1);

    // UAS LIST
    QDockWidget* container4 = new QDockWidget(tr("Unmanned Systems"), this);
    container4->setWidget(list);
    addDockWidget(Qt::BottomDockWidgetArea, container4);

    // UAS STATUS
    QDockWidget* container3 = new QDockWidget(tr("Status Details"), this);
    container3->setWidget(info);
    addDockWidget(Qt::LeftDockWidgetArea, container3);

    // WAYPOINT LIST
    QDockWidget* container5 = new QDockWidget(tr("Waypoint List"), this);
    container5->setWidget(waypoints);
    addDockWidget(Qt::BottomDockWidgetArea, container5);

    // DEBUG CONSOLE
    QDockWidget* container7 = new QDockWidget(tr("Communication Console"), this);
    container7->setWidget(debugConsole);
    addDockWidget(Qt::BottomDockWidgetArea, container7);

    // OBJECT DETECTION
    QDockWidget* container6 = new QDockWidget(tr("Object Recognition"), this);
    container6->setWidget(detection);
    addDockWidget(Qt::RightDockWidgetArea, container6);

    this->show();
}

void MainWindow::loadSettingsView()
{
    clearView();

    GAudioOutput::instance()->say("Switched to Settings View");

    // LINE CHART
    centerStack->setCurrentWidget(linechart);

    // COMM XML
    QDockWidget* container1 = new QDockWidget(tr("MAVLink XML to C Code Generator"), this);
    container1->setWidget(protocol);
    addDockWidget(Qt::LeftDockWidgetArea, container1);

    // ONBOARD PARAMETERS
    QDockWidget* container6 = new QDockWidget(tr("Onboard Parameters"), this);
    container6->setWidget(parameters);
    addDockWidget(Qt::RightDockWidgetArea, container6);
}

void MainWindow::loadEngineerView()
{
    clearView();
    // Engineer view, used in EMAV2009

    GAudioOutput::instance()->say("Switched to Engineer View");

    // LINE CHART
    centerStack->setCurrentWidget(linechart);

    // UAS CONTROL
    QDockWidget* container1 = new QDockWidget(tr("Control"), this);
    container1->setWidget(control);
    addDockWidget(Qt::LeftDockWidgetArea, container1);

    // UAS LIST
    QDockWidget* container4 = new QDockWidget(tr("Unmanned Systems"), this);
    container4->setWidget(list);
    addDockWidget(Qt::BottomDockWidgetArea, container4);

    // UAS STATUS
    QDockWidget* container3 = new QDockWidget(tr("Status Details"), this);
    container3->setWidget(info);
    addDockWidget(Qt::LeftDockWidgetArea, container3);

    // WAYPOINT LIST
    QDockWidget* container5 = new QDockWidget(tr("Waypoint List"), this);
    container5->setWidget(waypoints);
    addDockWidget(Qt::BottomDockWidgetArea, container5);

    // DEBUG CONSOLE
    QDockWidget* container7 = new QDockWidget(tr("Communication Console"), this);
    container7->setWidget(debugConsole);
    addDockWidget(Qt::BottomDockWidgetArea, container7);

    this->show();
}

void MainWindow::loadWidgets()
{
    loadOperatorView();
    //loadEngineerView();
    //loadPilotView();
}

/*
void MainWindow::removeCommConfAct(QAction* action)
{
    ui.menuNetwork->removeAction(action);
}*/

//void MainWindow::startUAS()
//{
//    UASManager::instance()->getActiveUAS()->launch();
//}
//
//void MainWindow::returnUAS()
//{
//   UASManager::instance()->getActiveUAS()->home();
//}
//
//void MainWindow::stopUAS()
//{
//    UASManager::instance()->getActiveUAS()->emergencySTOP();
//}
//
//void MainWindow::killUAS()
//{
//    UASManager::instance()->getActiveUAS()->emergencyKILL();
//}

void MainWindow::runTests()
{
    // TODO Remove after debugging: Add fake data
    static double testvalue = 0.0f;
    testvalue += 0.01f;
    linechart->appendData(126, "test data", testvalue, MG::TIME::getGroundTimeNow());
}


