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
#include "Q3DWidgetFactory.h"

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

    // Load widgets and show application windowa
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
    //FIXME: memory of acceptList will never be freed again
    QStringList* acceptList = new QStringList();
    acceptList->append("roll IMU");
    acceptList->append("pitch IMU");
    acceptList->append("yaw IMU");
    acceptList->append("rollspeed IMU");
    acceptList->append("pitchspeed IMU");
    acceptList->append("yawspeed IMU");

    //FIXME: memory of acceptList2 will never be freed again
    QStringList* acceptList2 = new QStringList();
    acceptList2->append("Battery");
    acceptList2->append("Pressure");

    //TODO:  move protocol outside UI
    mavlink     = new MAVLinkProtocol();

    // Center widgets
    linechartWidget   = new Linecharts(this);
    hudWidget         = new HUD(640, 480, this);
    mapWidget         = new MapWidget(this);
    protocolWidget    = new XMLCommProtocolWidget(this);
    dataplotWidget    = new QGCDataPlot2D(this);
    _3DWidget         = Q3DWidgetFactory::get("PIXHAWK");

    // Dock widgets
    controlDockWidget = new QDockWidget(tr("Control"), this);
    controlDockWidget->setWidget( new UASControlWidget(this) );

    listDockWidget = new QDockWidget(tr("Unmanned Systems"), this);
    listDockWidget->setWidget( new UASListWidget(this) );

    waypointsDockWidget = new QDockWidget(tr("Waypoint List"), this);
    waypointsDockWidget->setWidget( new WaypointList(this, NULL) );

    infoDockWidget = new QDockWidget(tr("Status Details"), this);
    infoDockWidget->setWidget( new UASInfoWidget(this) );

    detectionDockWidget = new QDockWidget(tr("Object Recognition"), this);
    detectionDockWidget->setWidget( new ObjectDetectionView("images/patterns", this) );

    debugConsoleDockWidget = new QDockWidget(tr("Communication Console"), this);
    debugConsoleDockWidget->setWidget( new DebugConsole(this) );

    parametersDockWidget = new QDockWidget(tr("Onboard Parameters"), this);
    parametersDockWidget->setWidget( new ParameterInterface(this) );

    watchdogControlDockWidget = new QDockWidget(tr("Process Control"), this);
    watchdogControlDockWidget->setWidget( new WatchdogControl(this) );

    hsiDockWidget = new QDockWidget(tr("Horizontal Situation Indicator"), this);
    hsiDockWidget->setWidget( new HSIDisplay(this) );

    headDown1DockWidget = new QDockWidget(tr("Primary Flight Display"), this);
    headDown1DockWidget->setWidget( new HDDisplay(acceptList, this) );

    headDown2DockWidget = new QDockWidget(tr("Payload Status"), this);
    headDown2DockWidget->setWidget( new HDDisplay(acceptList2, this) );

    rcViewDockWidget = new QDockWidget(tr("Radio Control"), this);
    rcViewDockWidget->setWidget( new QGCRemoteControlView(this) );

    headUpDockWidget = new QDockWidget(tr("Control Indicator"), this);
    headUpDockWidget->setWidget( new HUD(320, 240, this));

    // Dialogue widgets
    //FIXME: free memory in destructor
    joystick    = new JoystickInput();

}

/**
 * Connect all signals and slots of the main window widgets
 */
void MainWindow::connectWidgets()
{
    if (linechartWidget)
    {
        connect(UASManager::instance(), SIGNAL(UASCreated(UASInterface*)),
                linechartWidget, SLOT(addSystem(UASInterface*)));
        connect(UASManager::instance(), SIGNAL(activeUASSet(int)),
                linechartWidget, SLOT(selectSystem(int)));
        connect(linechartWidget, SIGNAL(logfileWritten(QString)),
                this, SLOT(loadDataView(QString)));
    }
    if (infoDockWidget && infoDockWidget->widget())
    {
        connect(mavlink, SIGNAL(receiveLossChanged(int, float)),
                infoDockWidget->widget(), SLOT(updateSendLoss(int, float)));
    }
    if (mapWidget && waypointsDockWidget->widget())
    {
        // clear path create on the map
        connect(waypointsDockWidget->widget(), SIGNAL(clearPathclicked()), mapWidget, SLOT(clearPath()));
        // add Waypoint widget in the WaypointList widget when mouse clicked
        connect(mapWidget, SIGNAL(captureMapCoordinateClick(QPointF)), waypointsDockWidget->widget(), SLOT(addWaypointMouse(QPointF)));
        // it notifies that a waypoint global goes to do create
        connect(mapWidget, SIGNAL(createGlobalWP(bool, QPointF)), waypointsDockWidget->widget(), SLOT(setIsWPGlobal(bool, QPointF)));
        connect(mapWidget, SIGNAL(sendGeometryEndDrag(QPointF,int)), waypointsDockWidget->widget(), SLOT(waypointGlobalChanged(QPointF,int)) );

        // it notifies that a waypoint global goes to do create and a map graphic too
        connect(waypointsDockWidget->widget(), SIGNAL(createWaypointAtMap(QPointF)), mapWidget, SLOT(createWaypointGraphAtMap(QPointF)));
        // it notifies that a waypoint global change it´s position by spinBox on Widget WaypointView
        connect(waypointsDockWidget->widget(), SIGNAL(changePositionWPGlobalBySpinBox(int,float,float)), mapWidget, SLOT(changeGlobalWaypointPositionBySpinBox(int,float,float)));
    }
}

void MainWindow::arrangeCenterStack()
{

    QStackedWidget *centerStack = new QStackedWidget(this);
    if (!centerStack) return;
    if (linechartWidget) centerStack->addWidget(linechartWidget);
    if (protocolWidget) centerStack->addWidget(protocolWidget);
    if (mapWidget) centerStack->addWidget(mapWidget);
    if (_3DWidget) centerStack->addWidget(_3DWidget);
    if (hudWidget) centerStack->addWidget(hudWidget);
    if (dataplotWidget) centerStack->addWidget(dataplotWidget);

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
    connect(ui.action3DView, SIGNAL(triggered()), this, SLOT(load3DView()));
    connect(ui.actionShow_full_view, SIGNAL(triggered()), this, SLOT(loadAllView()));
    connect(ui.actionShow_MAVLink_view, SIGNAL(triggered()), this, SLOT(loadMAVLinkView()));
    connect(ui.actionShow_data_analysis_view, SIGNAL(triggered()), this, SLOT(loadDataView()));
    connect(ui.actionStyleConfig, SIGNAL(triggered()), this, SLOT(reloadStylesheet()));
    connect(ui.actionGlobalOperatorView, SIGNAL(triggered()), this, SLOT(loadGlobalOperatorView()));
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
    LinkManager::instance()->addProtocol(link, mavlink);
    CommConfigurationWindow* commWidget = new CommConfigurationWindow(link, mavlink, this);
    ui.menuNetwork->addAction(commWidget->getAction());

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

    if (uas != NULL)
    {
        QIcon icon;
        // Set matching icon
        switch (uas->getSystemType())
        {
        case 0:
            icon = QIcon(":/images/mavs/generic.svg");
            break;
        case 1:
            icon = QIcon(":/images/mavs/fixed-wing.svg");
            break;
        case 2:
            icon = QIcon(":/images/mavs/quadrotor.svg");
            break;
        case 3:
            icon = QIcon(":/images/mavs/coaxial.svg");
            break;
        case 4:
            icon = QIcon(":/images/mavs/helicopter.svg");
            break;
        case 5:
            icon = QIcon(":/images/mavs/groundstation.svg");
            break;
        default:
            icon = QIcon(":/images/mavs/unknown.svg");
            break;
        }

        ui.menuConnected_Systems->addAction(icon, tr("Select %1 for control").arg(uas->getUASName()), uas, SLOT(setSelected()));

        // FIXME Should be not inside the mainwindow
        if (debugConsoleDockWidget)
        {
            DebugConsole *debugConsole = dynamic_cast<DebugConsole*>(debugConsoleDockWidget->widget());
            if (debugConsole)
            {
                connect(uas, SIGNAL(textMessageReceived(int,int,int,QString)),
                        debugConsole, SLOT(receiveTextMessage(int,int,int,QString)));
            }
        }

        // Health / System status indicator
        if (infoDockWidget)
        {
            UASInfoWidget *infoWidget = dynamic_cast<UASInfoWidget*>(infoDockWidget->widget());
            if (infoWidget)
            {
                infoWidget->addUAS(uas);
            }
        }

        // UAS List
        if (listDockWidget)
        {
            UASListWidget *listWidget = dynamic_cast<UASListWidget*>(listDockWidget->widget());
            if (listWidget)
            {
                listWidget->addUAS(uas);
            }
        }

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
}

/**
 * Clears the current view completely
 */
void MainWindow::clearView()
{ 
    // Halt HUD
    if (hudWidget) hudWidget->stop();
    // Disable linechart
    if (linechartWidget) linechartWidget->setActive(false);
    // Halt HDDs
    if (headDown1DockWidget)
    {
        HDDisplay* hddWidget = dynamic_cast<HDDisplay*>( headDown1DockWidget->widget() );
        if (hddWidget) hddWidget->stop();
    }
    if (headDown2DockWidget)
    {
        HDDisplay* hddWidget = dynamic_cast<HDDisplay*>( headDown2DockWidget->widget() );
        if (hddWidget) hddWidget->stop();
    }
    // Halt HSI
    if (hsiDockWidget)
    {
        HSIDisplay* hsi = dynamic_cast<HSIDisplay*>( hsiDockWidget->widget() );
        if (hsi) hsi->stop();
    }

    // Remove all dock widgets from main window
    QObjectList childList( this->children() );

    QObjectList::iterator i;
    QDockWidget* dockWidget;
    for (i = childList.begin(); i != childList.end(); ++i)
    {
        dockWidget = dynamic_cast<QDockWidget*>(*i);
        if (dockWidget)
        {
            // Remove dock widget from main window
            this->removeDockWidget(dockWidget);
            // Deletion of dockWidget would also delete all child
            // widgets of dockWidget
            // Is there a way to unset a widget from QDockWidget?
        }
    }
}

void MainWindow::loadSlugsView()
{
    clearView();
    // Engineer view, used in EMAV2009

    // LINE CHART
    if (linechartWidget)
    {
        QStackedWidget *centerStack = dynamic_cast<QStackedWidget*>(centralWidget());
        if (centerStack)
        {
            linechartWidget->setActive(true);
            centerStack->setCurrentWidget(linechartWidget);
        }
    }

    // UAS CONTROL
    if (controlDockWidget)
    {
        addDockWidget(Qt::LeftDockWidgetArea, controlDockWidget);
        controlDockWidget->show();
    }

    // UAS LIST
    if (listDockWidget)
    {
        addDockWidget(Qt::BottomDockWidgetArea, listDockWidget);
        listDockWidget->show();
    }

    // UAS STATUS
    if (infoDockWidget)
    {
        addDockWidget(Qt::LeftDockWidgetArea, infoDockWidget);
        infoDockWidget->show();
    }

    // HORIZONTAL SITUATION INDICATOR
    if (hsiDockWidget)
    {
        HSIDisplay* hsi = dynamic_cast<HSIDisplay*>( hsiDockWidget->widget() );
        if (hsi)
        {
            hsi->start();
            addDockWidget(Qt::LeftDockWidgetArea, hsiDockWidget);
            hsiDockWidget->show();
        }
    }

    // WAYPOINT LIST
    if (waypointsDockWidget)
    {
        addDockWidget(Qt::BottomDockWidgetArea, waypointsDockWidget);
        waypointsDockWidget->show();
    }

    // DEBUG CONSOLE
    if (debugConsoleDockWidget)
    {
        addDockWidget(Qt::BottomDockWidgetArea, debugConsoleDockWidget);
        debugConsoleDockWidget->show();
    }

    // ONBOARD PARAMETERS
    if (parametersDockWidget)
    {
        addDockWidget(Qt::RightDockWidgetArea, parametersDockWidget);
        parametersDockWidget->show();
    }

    this->show();
}

void MainWindow::loadPixhawkView()
{
    clearView();
    // Engineer view, used in EMAV2009

    // 3D map
    if (_3DWidget)
    {
        QStackedWidget *centerStack = dynamic_cast<QStackedWidget*>(centralWidget());
        if (centerStack)
        {
            //map3DWidget->setActive(true);
            centerStack->setCurrentWidget(_3DWidget);
        }
    }

    // UAS CONTROL
    if (controlDockWidget)
    {
        addDockWidget(Qt::LeftDockWidgetArea, controlDockWidget);
        controlDockWidget->show();
    }

    // HORIZONTAL SITUATION INDICATOR
    if (hsiDockWidget)
    {
        HSIDisplay* hsi = dynamic_cast<HSIDisplay*>( hsiDockWidget->widget() );
        if (hsi)
        {
            hsi->start();
            addDockWidget(Qt::LeftDockWidgetArea, hsiDockWidget);
            hsiDockWidget->show();
        }
    }

    // UAS LIST
    if (listDockWidget)
    {
        addDockWidget(Qt::BottomDockWidgetArea, listDockWidget);
        listDockWidget->show();
    }

    // UAS STATUS
    if (infoDockWidget)
    {
        addDockWidget(Qt::LeftDockWidgetArea, infoDockWidget);
        infoDockWidget->show();
    }

    // WAYPOINT LIST
    if (waypointsDockWidget)
    {
        addDockWidget(Qt::BottomDockWidgetArea, waypointsDockWidget);
        waypointsDockWidget->show();
    }

    // DEBUG CONSOLE
    if (debugConsoleDockWidget)
    {
        addDockWidget(Qt::BottomDockWidgetArea, debugConsoleDockWidget);
        debugConsoleDockWidget->show();
    }

    // ONBOARD PARAMETERS
    if (parametersDockWidget)
    {
        addDockWidget(Qt::RightDockWidgetArea, parametersDockWidget);
        parametersDockWidget->show();
    }

    this->show();
}

void MainWindow::loadDataView()
{
    clearView();

    // DATAPLOT
    if (dataplotWidget)
    {
        QStackedWidget *centerStack = dynamic_cast<QStackedWidget*>(centralWidget());
        if (centerStack)
            centerStack->setCurrentWidget(dataplotWidget);
    }
}

void MainWindow::loadDataView(QString fileName)
{
    clearView();

    // DATAPLOT
    if (dataplotWidget)
    {
        QStackedWidget *centerStack = dynamic_cast<QStackedWidget*>(centralWidget());
        if (centerStack)
        {
            centerStack->setCurrentWidget(dataplotWidget);
            dataplotWidget->loadFile(fileName);
        }
    }
}

void MainWindow::loadPilotView()
{
    clearView();

    // HEAD UP DISPLAY
    if (hudWidget)
    {
        QStackedWidget *centerStack = dynamic_cast<QStackedWidget*>(centralWidget());
        if (centerStack)
        {
            centerStack->setCurrentWidget(hudWidget);
            hudWidget->start();
        }
    }

    //connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), pfd, SLOT(setActiveUAS(UASInterface*)));
    if (headDown1DockWidget)
    {
        HDDisplay *hdd = dynamic_cast<HDDisplay*>(headDown1DockWidget->widget());
        if (hdd)
        {
            addDockWidget(Qt::RightDockWidgetArea, headDown1DockWidget);
            headDown1DockWidget->show();
            hdd->start();
        }
        
    }
    if (headDown2DockWidget)
    {
        HDDisplay *hdd = dynamic_cast<HDDisplay*>(headDown2DockWidget->widget());
        if (hdd)
        {
            addDockWidget(Qt::RightDockWidgetArea, headDown2DockWidget);
            headDown2DockWidget->show();
            hdd->start();
        }
    }

    this->show();
}

void MainWindow::loadOperatorView()
{
    clearView();

    // MAP
    if (mapWidget)
    {
        QStackedWidget *centerStack = dynamic_cast<QStackedWidget*>(centralWidget());
        if (centerStack)
        {
            centerStack->setCurrentWidget(mapWidget);
        }
    }

    // UAS CONTROL
    if (controlDockWidget)
    {
        addDockWidget(Qt::LeftDockWidgetArea, controlDockWidget);
        controlDockWidget->show();
    }

    // UAS LIST
    if (listDockWidget)
    {
        addDockWidget(Qt::BottomDockWidgetArea, listDockWidget);
        listDockWidget->show();
    }

    // UAS STATUS
    if (infoDockWidget)
    {
        addDockWidget(Qt::LeftDockWidgetArea, infoDockWidget);
        infoDockWidget->show();
    }

    // WAYPOINT LIST
    if (waypointsDockWidget)
    {
        addDockWidget(Qt::BottomDockWidgetArea, waypointsDockWidget);
        waypointsDockWidget->show();
    }

    // HORIZONTAL SITUATION INDICATOR
    if (hsiDockWidget)
    {
        HSIDisplay* hsi = dynamic_cast<HSIDisplay*>( hsiDockWidget->widget() );
        if (hsi)
        {
            addDockWidget(Qt::BottomDockWidgetArea, hsiDockWidget);
            hsiDockWidget->show();
            hsi->start();
        }
    }

    // OBJECT DETECTION
    if (detectionDockWidget)
    {
        addDockWidget(Qt::RightDockWidgetArea, detectionDockWidget);
        detectionDockWidget->show();
    }

    // PROCESS CONTROL
    if (watchdogControlDockWidget)
    {
        addDockWidget(Qt::RightDockWidgetArea, watchdogControlDockWidget);
        watchdogControlDockWidget->show();
    }

    this->show();
}

void MainWindow::loadGlobalOperatorView()
{
    clearView();

    // MAP
    if (mapWidget)
    {
        QStackedWidget *centerStack = dynamic_cast<QStackedWidget*>(centralWidget());
        if (centerStack)
        {
            centerStack->setCurrentWidget(mapWidget);
        }
    }

    // UAS CONTROL
    if (controlDockWidget)
    {
        addDockWidget(Qt::LeftDockWidgetArea, controlDockWidget);
        controlDockWidget->show();
    }

    // UAS LIST
    if (listDockWidget)
    {
        addDockWidget(Qt::BottomDockWidgetArea, listDockWidget);
        listDockWidget->show();
    }

    // UAS STATUS
    if (infoDockWidget)
    {
        addDockWidget(Qt::LeftDockWidgetArea, infoDockWidget);
        infoDockWidget->show();
    }

    // WAYPOINT LIST
    if (waypointsDockWidget)
    {
        addDockWidget(Qt::BottomDockWidgetArea, waypointsDockWidget);
        waypointsDockWidget->show();
    }

//    // HORIZONTAL SITUATION INDICATOR
//    if (hsiDockWidget)
//    {
//        HSIDisplay* hsi = dynamic_cast<HSIDisplay*>( hsiDockWidget->widget() );
//        if (hsi)
//        {
//            addDockWidget(Qt::BottomDockWidgetArea, hsiDockWidget);
//            hsiDockWidget->show();
//            hsi->start();
//        }
//    }

    // PROCESS CONTROL
    if (watchdogControlDockWidget)
    {
        addDockWidget(Qt::RightDockWidgetArea, watchdogControlDockWidget);
        watchdogControlDockWidget->show();
    }

    // HEAD UP DISPLAY
    if (headUpDockWidget)
    {
        addDockWidget(Qt::RightDockWidgetArea, headUpDockWidget);
        // FIXME Replace with default ->show() call
        HUD* hud = dynamic_cast<HUD*>(headUpDockWidget->widget());

        if (hud)
        {
            headUpDockWidget->show();
            hud->start();
        }
    }

}

void MainWindow::load3DView()
{
            clearView();

            // 3D map
            if (_3DWidget)
            {
                QStackedWidget *centerStack = dynamic_cast<QStackedWidget*>(centralWidget());
                if (centerStack)
                {
                    //map3DWidget->setActive(true);
                    centerStack->setCurrentWidget(_3DWidget);
                }
            }

            // UAS CONTROL
            if (controlDockWidget)
            {
                addDockWidget(Qt::LeftDockWidgetArea, controlDockWidget);
                controlDockWidget->show();
            }

            // UAS LIST
            if (listDockWidget)
            {
                addDockWidget(Qt::BottomDockWidgetArea, listDockWidget);
                listDockWidget->show();
            }

            // WAYPOINT LIST
            if (waypointsDockWidget)
            {
                addDockWidget(Qt::BottomDockWidgetArea, waypointsDockWidget);
                waypointsDockWidget->show();
            }

            // HORIZONTAL SITUATION INDICATOR
            if (hsiDockWidget)
            {
                HSIDisplay* hsi = dynamic_cast<HSIDisplay*>( hsiDockWidget->widget() );
                if (hsi)
                {
                    hsi->start();
                    addDockWidget(Qt::LeftDockWidgetArea, hsiDockWidget);
                    hsiDockWidget->show();
                }
            }

            this->show();
        }

void MainWindow::loadEngineerView()
{
    clearView();
    // Engineer view, used in EMAV2009

    // LINE CHART
    if (linechartWidget)
    {
        QStackedWidget *centerStack = dynamic_cast<QStackedWidget*>(centralWidget());
        if (centerStack)
        {
            linechartWidget->setActive(true);
            centerStack->setCurrentWidget(linechartWidget);
        }
    }

    // UAS CONTROL
    if (controlDockWidget)
    {
        addDockWidget(Qt::LeftDockWidgetArea, controlDockWidget);
        controlDockWidget->show();
    }

    // UAS LIST
    if (listDockWidget)
    {
        addDockWidget(Qt::BottomDockWidgetArea, listDockWidget);
        listDockWidget->show();
    }

    // UAS STATUS
    if (infoDockWidget)
    {
        addDockWidget(Qt::LeftDockWidgetArea, infoDockWidget);
        infoDockWidget->show();
    }

    // WAYPOINT LIST
    if (waypointsDockWidget)
    {
        addDockWidget(Qt::BottomDockWidgetArea, waypointsDockWidget);
        waypointsDockWidget->show();
    }

    // DEBUG CONSOLE
    if (debugConsoleDockWidget)
    {
        addDockWidget(Qt::BottomDockWidgetArea, debugConsoleDockWidget);
        debugConsoleDockWidget->show();
    }

    // ONBOARD PARAMETERS
    if (parametersDockWidget)
    {
        addDockWidget(Qt::RightDockWidgetArea, parametersDockWidget);
        parametersDockWidget->show();
    }

    // RADIO CONTROL VIEW
    if (rcViewDockWidget)
    {
        addDockWidget(Qt::BottomDockWidgetArea, rcViewDockWidget);
        rcViewDockWidget->show();
    }

    this->show();
}

void MainWindow::loadMAVLinkView()
{
    clearView();

    if (protocolWidget)
    {
        QStackedWidget *centerStack = dynamic_cast<QStackedWidget*>(centralWidget());
        if (centerStack)
        {
            centerStack->setCurrentWidget(protocolWidget);
        }
    }

    this->show();
}

void MainWindow::loadAllView()
{
    clearView();

    if (headDown1DockWidget)
    {
        HDDisplay *hdd = dynamic_cast<HDDisplay*>(headDown1DockWidget->widget());
        if (hdd)
        {
            addDockWidget(Qt::RightDockWidgetArea, headDown1DockWidget);
            headDown1DockWidget->show();
            hdd->start();
        }
        
    }
    if (headDown2DockWidget)
    {
        HDDisplay *hdd = dynamic_cast<HDDisplay*>(headDown2DockWidget->widget());
        if (hdd)
        {
            addDockWidget(Qt::RightDockWidgetArea, headDown2DockWidget);
            headDown2DockWidget->show();
            hdd->start();
        }
    }

    // UAS CONTROL
    if (controlDockWidget)
    {
        addDockWidget(Qt::LeftDockWidgetArea, controlDockWidget);
        controlDockWidget->show();
    }

    // UAS LIST
    if (listDockWidget)
    {
        addDockWidget(Qt::BottomDockWidgetArea, listDockWidget);
        listDockWidget->show();
    }

    // UAS STATUS
    if (infoDockWidget)
    {
        addDockWidget(Qt::LeftDockWidgetArea, infoDockWidget);
        infoDockWidget->show();
    }

    // WAYPOINT LIST
    if (waypointsDockWidget)
    {
        addDockWidget(Qt::BottomDockWidgetArea, waypointsDockWidget);
        waypointsDockWidget->show();
    }

    // DEBUG CONSOLE
    if (debugConsoleDockWidget)
    {
        addDockWidget(Qt::BottomDockWidgetArea, debugConsoleDockWidget);
        debugConsoleDockWidget->show();
    }

    // OBJECT DETECTION
    if (detectionDockWidget)
    {
        addDockWidget(Qt::RightDockWidgetArea, detectionDockWidget);
        detectionDockWidget->show();
    }

    // LINE CHART
    if (linechartWidget)
    {
        QStackedWidget *centerStack = dynamic_cast<QStackedWidget*>(centralWidget());
        if (centerStack)
        {
            linechartWidget->setActive(true);
            centerStack->setCurrentWidget(linechartWidget);
        }
    }

    // ONBOARD PARAMETERS
    if (parametersDockWidget)
    {
        addDockWidget(Qt::RightDockWidgetArea, parametersDockWidget);
        parametersDockWidget->show();
    }

    this->show();
}

void MainWindow::loadWidgets()
{
    //loadOperatorView();
    loadEngineerView();
    //loadPilotView();
}
