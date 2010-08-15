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

// FIXME Move
#include "PxQuadMAV.h"
#include "SlugsMAV.h"


#include "LogCompressor.h"

/**
* Create new mainwindow. The constructor instantiates all parts of the user
* interface. It does NOT show the mainwindow. To display it, call the show()
* method.
*
* @see QMainWindow::show()
**/
MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  settings()
{
    this->hide();
    this->setVisible(false);

    // Setup user interface
    ui.setupUi(this);

    buildWidgets();

    connectWidgets();

    arrangeCenterStack();

    configureWindowName();

    // Add status bar
    setStatusBar(createStatusBar());

    // Set the application style (not the same as a style sheet)
    // Set the style to Plastique
    qApp->setStyle("plastique");

    // Set style sheet as last step
    reloadStylesheet();


    // Create actions
    connectActions();

    // Load widgets and show application window
    loadWidgets();

    // Adjust the size
    adjustSize();
}

MainWindow::~MainWindow()
{
    delete statusBar;
    statusBar = NULL;
}


void MainWindow::buildWidgets()
{
  QStringList* acceptList = new QStringList();
  acceptList->append("roll IMU");
  acceptList->append("pitch IMU");
  acceptList->append("yaw IMU");
  acceptList->append("rollspeed IMU");
  acceptList->append("pitchspeed IMU");
  acceptList->append("yawspeed IMU");

  QStringList* acceptList2 = new QStringList();
  acceptList2->append("Battery");
  acceptList2->append("Pressure");


  mavlink     = new MAVLinkProtocol();
  linechart   = new Linecharts(this);
  control     = new UASControlWidget(this);
  list        = new UASListWidget(this);
  waypoints   = new WaypointList(this, NULL);
  info        = new UASInfoWidget(this);
  detection   = new ObjectDetectionView("patterns", this);
  hud         = new HUD(640, 480, this);
  debugConsole= new DebugConsole(this);
  map         = new MapWidget(this);
  protocol    = new XMLCommProtocolWidget(this);
  parameters  = new ParameterInterface(this);
  watchdogControl = new WatchdogControl(this);
  hsi         = new HSIDisplay(this);
  headDown1   = new HDDisplay(acceptList, this);
  headDown2   = new HDDisplay(acceptList2, this);
  joystick    = new JoystickInput();
  dataplot    = new QGCDataPlot2D();
}

void MainWindow::connectWidgets()
{
  connect(UASManager::instance(), SIGNAL(UASCreated(UASInterface*)), linechart, SLOT(addSystem(UASInterface*)));
  connect(UASManager::instance(), SIGNAL(activeUASSet(int)), linechart, SLOT(selectSystem(int)));
  connect(mavlink, SIGNAL(receiveLossChanged(int, float)), info, SLOT(updateSendLoss(int, float)));
}

void MainWindow::arrangeCenterStack()
{

  centerStack = new QStackedWidget(this);

  centerStack->addWidget(linechart);
  centerStack->addWidget(protocol);
  centerStack->addWidget(map);
  centerStack->addWidget(hud);
  centerStack->addWidget(dataplot);

  setCentralWidget(centerStack);
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

/**
 * Reload the style sheet from disk. The function tries to load "qgroundcontrol.css" from the application
 * directory (which by default does not exist). If it fails, it will load the bundled default CSS
 * from memory.
 * To customize the application, just create a qgroundcontrol.css file in the application directory
 */
void MainWindow::reloadStylesheet()
{
    // Load style sheet
    QFile* styleSheet = new QFile(QCoreApplication::applicationDirPath() + "/qgroundcontrol.css");
    if (!styleSheet->exists())
    {
        styleSheet = new QFile(":/images/style-mission.css");
    }
    if (styleSheet->open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QString style = QString(styleSheet->readAll());
        style.replace("ICONDIR", QCoreApplication::applicationDirPath()+ "/images/");
        qApp->setStyleSheet(style);
    }
    else
    {
        qDebug() << "Style not set:" << styleSheet->fileName() << "opened: " << styleSheet->isOpen();
    }
    delete styleSheet;
}

void MainWindow::showStatusMessage(const QString& status, int timeout)
{
    statusBar->showMessage(status, timeout);
}

void MainWindow::showStatusMessage(const QString& status)
{
    statusBar->showMessage(status, 5);
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
    connect(ui.actionShow_full_view, SIGNAL(triggered()), this, SLOT(loadAllView()));
    connect(ui.actionShow_MAVLink_view, SIGNAL(triggered()), this, SLOT(loadMAVLinkView()));
    connect(ui.actionShow_data_analysis_view, SIGNAL(triggered()), this, SLOT(loadDataView()));
    connect(ui.actionStyleConfig, SIGNAL(triggered()), this, SLOT(reloadStylesheet()));

    connect(ui.actionOnline_documentation, SIGNAL(triggered()), this, SLOT(showHelp()));
    connect(ui.actionCredits_Developers, SIGNAL(triggered()), this, SLOT(showCredits()));
    connect(ui.actionProject_Roadmap, SIGNAL(triggered()), this, SLOT(showRoadMap()));

    // Joystick configuration
    connect(ui.actionJoystickSettings, SIGNAL(triggered()), this, SLOT(configure()));
}

void MainWindow::showHelp()
{
    if(!QDesktopServices::openUrl(QUrl("http://qgroundcontrol.org/user_guide")))
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
    if(!QDesktopServices::openUrl(QUrl("http://qgroundcontrol.org/roadmap")))
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

void MainWindow::addLink(LinkInterface *link)
{
    CommConfigurationWindow* commWidget = new CommConfigurationWindow(link, mavlink, this);
    ui.menuNetwork->addAction(commWidget->getAction());
    LinkManager::instance()->addProtocol(link, mavlink);

    // Special case for simulationlink
    MAVLinkSimulationLink* sim = dynamic_cast<MAVLinkSimulationLink*>(link);
    if (sim)
    {
        //connect(sim, SIGNAL(valueChanged(int,QString,double,quint64)), linechart, SLOT(appendData(int,QString,double,quint64)));
        connect(ui.actionSimulate, SIGNAL(triggered(bool)), sim, SLOT(connectLink(bool)));
    }
}

void MainWindow::UASCreated(UASInterface* uas)
{
    // Connect the UAS to the full user interface
    //ui.menuConnected_Systems->addAction(QIcon(":/images/mavs/generic.svg"), tr("View ") + uas->getUASName(), uas, SLOT(setSelected()));

    // FIXME Should be not inside the mainwindow
    connect(uas, SIGNAL(textMessageReceived(int,int,QString)), debugConsole, SLOT(receiveTextMessage(int,int,QString)));

    // Health / System status indicator
    info->addUAS(uas);

    // UAS List
    list->addUAS(uas);

    // Camera view
    //camera->addUAS(uas);

    // Revalidate UI
    // TODO Stylesheet reloading should in theory not be necessary
    reloadStylesheet();

    // Check which type this UAS is of
    PxQuadMAV* mav = dynamic_cast<PxQuadMAV*>(uas);
    if (mav) loadPixhawkView();
    SlugsMAV* mav2 = dynamic_cast<SlugsMAV*>(uas);
    if (mav2) loadSlugsView();
}

/**
 * Clears the current view completely
 */
void MainWindow::clearView()
{ 
    // Halt HUD
    hud->stop();
    linechart->setActive(false);
    headDown1->stop();
    headDown2->stop();
    hsi->stop();

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

void MainWindow::loadSlugsView()
{
    clearView();
    // Engineer view, used in EMAV2009

    // LINE CHART
    linechart->setActive(true);
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

    // HORIZONTAL SITUATION INDICATOR
    QDockWidget* container6 = new QDockWidget(tr("Horizontal Situation Indicator"), this);
    container6->setWidget(hsi);
    hsi->start();
    addDockWidget(Qt::LeftDockWidgetArea, container6);

    // WAYPOINT LIST
    QDockWidget* container5 = new QDockWidget(tr("Waypoint List"), this);
    container5->setWidget(waypoints);
    addDockWidget(Qt::BottomDockWidgetArea, container5);

    // DEBUG CONSOLE
    QDockWidget* container7 = new QDockWidget(tr("Communication Console"), this);
    container7->setWidget(debugConsole);
    addDockWidget(Qt::BottomDockWidgetArea, container7);

    // ONBOARD PARAMETERS
    QDockWidget* containerParams = new QDockWidget(tr("Onboard Parameters"), this);
    containerParams->setWidget(parameters);
    addDockWidget(Qt::RightDockWidgetArea, containerParams);


    this->show();
}

void MainWindow::loadPixhawkView()
{
    clearView();
    // Engineer view, used in EMAV2009

    // LINE CHART
    linechart->setActive(true);
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

    // ONBOARD PARAMETERS
    QDockWidget* containerParams = new QDockWidget(tr("Onboard Parameters"), this);
    containerParams->setWidget(parameters);
    addDockWidget(Qt::RightDockWidgetArea, containerParams);


    this->show();
}

void MainWindow::loadDataView()
{
    clearView();
    centerStack->setCurrentWidget(dataplot);
}

void MainWindow::loadPilotView()
{
    clearView();

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

    // MAP
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

    // HORIZONTAL SITUATION INDICATOR
    QDockWidget* container7 = new QDockWidget(tr("Horizontal Situation Indicator"), this);
    container7->setWidget(hsi);
    hsi->start();
    addDockWidget(Qt::BottomDockWidgetArea, container7);

    // OBJECT DETECTION
    QDockWidget* container6 = new QDockWidget(tr("Object Recognition"), this);
    container6->setWidget(detection);
    addDockWidget(Qt::RightDockWidgetArea, container6);

    // PROCESS CONTROL
    QDockWidget* pControl = new QDockWidget(tr("Process Control"), this);
    pControl->setWidget(watchdogControl);
    addDockWidget(Qt::RightDockWidgetArea, pControl);
    this->show();
}

void MainWindow::loadSettingsView()
{
    clearView();

    // LINE CHART
    linechart->setActive(true);
    centerStack->setCurrentWidget(linechart);

    /*
    // COMM XML
    QDockWidget* container1 = new QDockWidget(tr("MAVLink XML to C Code Generator"), this);
    container1->setWidget(protocol);
    addDockWidget(Qt::LeftDockWidgetArea, container1);*/

    // ONBOARD PARAMETERS
    QDockWidget* container6 = new QDockWidget(tr("Onboard Parameters"), this);
    container6->setWidget(parameters);
    addDockWidget(Qt::RightDockWidgetArea, container6);
    this->show();
}

void MainWindow::loadEngineerView()
{
    clearView();
    // Engineer view, used in EMAV2009

    // LINE CHART
    linechart->setActive(true);
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

    // ONBOARD PARAMETERS
    QDockWidget* containerParams = new QDockWidget(tr("Onboard Parameters"), this);
    containerParams->setWidget(parameters);
    addDockWidget(Qt::RightDockWidgetArea, containerParams);


    this->show();
}

void MainWindow::loadMAVLinkView()
{
    clearView();
    centerStack->setCurrentWidget(protocol);
    this->show();
}

void MainWindow::loadAllView()
{
    clearView();

    QDockWidget* containerPFD = new QDockWidget(tr("Primary Flight Display"), this);
    containerPFD->setWidget(headDown1);
    addDockWidget(Qt::RightDockWidgetArea, containerPFD);

    QDockWidget* containerPayload = new QDockWidget(tr("Payload Status"), this);
    containerPayload->setWidget(headDown2);
    addDockWidget(Qt::RightDockWidgetArea, containerPayload);

    headDown1->start();
    headDown2->start();

    // UAS CONTROL
    QDockWidget* containerControl = new QDockWidget(tr("Control"), this);
    containerControl->setWidget(control);
    addDockWidget(Qt::LeftDockWidgetArea, containerControl);

    // UAS LIST
    QDockWidget* containerUASList = new QDockWidget(tr("Unmanned Systems"), this);
    containerUASList->setWidget(list);
    addDockWidget(Qt::BottomDockWidgetArea, containerUASList);

    // UAS STATUS
    QDockWidget* containerStatus = new QDockWidget(tr("Status Details"), this);
    containerStatus->setWidget(info);
    addDockWidget(Qt::LeftDockWidgetArea, containerStatus);

    // WAYPOINT LIST
    QDockWidget* containerWaypoints = new QDockWidget(tr("Waypoint List"), this);
    containerWaypoints->setWidget(waypoints);
    addDockWidget(Qt::BottomDockWidgetArea, containerWaypoints);

    // DEBUG CONSOLE
    QDockWidget* containerComm = new QDockWidget(tr("Communication Console"), this);
    containerComm->setWidget(debugConsole);
    addDockWidget(Qt::BottomDockWidgetArea, containerComm);

    // OBJECT DETECTION
    QDockWidget* containerObjRec = new QDockWidget(tr("Object Recognition"), this);
    containerObjRec->setWidget(detection);
    addDockWidget(Qt::RightDockWidgetArea, containerObjRec);

    // LINE CHART
    linechart->setActive(true);
    centerStack->setCurrentWidget(linechart);

    // ONBOARD PARAMETERS
    QDockWidget* containerParams = new QDockWidget(tr("Onboard Parameters"), this);
    containerParams->setWidget(parameters);
    addDockWidget(Qt::RightDockWidgetArea, containerParams);

    this->show();
}

void MainWindow::loadWidgets()
{
    //loadOperatorView();
    loadEngineerView();
    //loadPilotView();
}
