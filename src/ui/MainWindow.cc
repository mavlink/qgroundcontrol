/*===================================================================
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
#include "QGC.h"
#include "MAVLinkSimulationLink.h"
#include "SerialLink.h"
#include "UDPLink.h"
#include "MAVLinkProtocol.h"
#include "CommConfigurationWindow.h"
#include "WaypointList.h"
#include "MainWindow.h"
#include "JoystickWidget.h"
#include "GAudioOutput.h"

#ifdef QGC_OSG_ENABLED
#include "Q3DWidgetFactory.h"
#endif

// FIXME Move
#include "PxQuadMAV.h"
#include "SlugsMAV.h"


#include "LogCompressor.h"

MainWindow* MainWindow::instance()
{
    static MainWindow* _instance = 0;
    if(_instance == 0)
    {
        _instance = new MainWindow();

        /* Set the application as parent to ensure that this object
                 * will be destroyed when the main application exits */
        //_instance->setParent(qApp);
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
        toolsMenuActions(),
        currentView(VIEW_OPERATOR),
        aboutToCloseFlag(false),
        settings()
{
    // Get current settings
    settings.sync();

    // Check if the settings exist, instantiate defaults if necessary
    QString centralKey = buildMenuKey(SUB_SECTION_CHECKED, CENTRAL_MAP, currentView);
    if (!settings.contains(centralKey))
    {
        settings.setValue(centralKey,true);
    }

    QString listKey = buildMenuKey(SUB_SECTION_CHECKED, MENU_UAS_LIST, currentView);
    if (!settings.contains(listKey))
    {
        settings.setValue(listKey, true);
    }

    settings.sync();

    // Setup user interface
    ui.setupUi(this);

    buildCommonWidgets();

    connectCommonWidgets();

    arrangeCommonCenterStack();

    configureWindowName();

    // Add status bar
    //setStatusBar(createStatusBar());

    // Set the application style (not the same as a style sheet)
    // Set the style to Plastique
    qApp->setStyle("plastique");

    // Set style sheet as last step
    reloadStylesheet();

    // Create actions
    connectCommonActions();

    // Set dock options
    setDockOptions(AnimatedDocks | AllowTabbedDocks | AllowNestedDocks);

    // Load mavlink view as default widget set
    //loadMAVLinkView();

    if (settings.contains("geometry"))
    {
        // Restore the window geometry
        restoreGeometry(settings.value("geometry").toByteArray());
    }
    else
    {
    // Adjust the size
    adjustSize();
    }

    // Populate link menu
    QList<LinkInterface*> links = LinkManager::instance()->getLinks();
    foreach(LinkInterface* link, links)
    {
        this->addLink(link);
            }

    // Enable and update view
    presentView();
}

MainWindow::~MainWindow()
{
    delete statusBar;
    statusBar = NULL;
}

void MainWindow::buildCommonWidgets()
{
    //TODO:  move protocol outside UI
    mavlink     = new MAVLinkProtocol();

    // Dock widgets
    if (!controlDockWidget)
    {
    controlDockWidget = new QDockWidget(tr("Control"), this);
    controlDockWidget->setWidget( new UASControlWidget(this) );
        addToToolsMenu (controlDockWidget, tr("Control"), SLOT(showToolWidget()), MENU_UAS_CONTROL, Qt::LeftDockWidgetArea);
    }

    if (!listDockWidget)
    {
    listDockWidget = new QDockWidget(tr("Unmanned Systems"), this);
    listDockWidget->setWidget( new UASListWidget(this) );
        addToToolsMenu (listDockWidget, tr("Unmanned Systems"), SLOT(showToolWidget()), MENU_UAS_LIST, Qt::RightDockWidgetArea);
    }

    if (!waypointsDockWidget)
    {
    waypointsDockWidget = new QDockWidget(tr("Waypoint List"), this);
    waypointsDockWidget->setWidget( new WaypointList(this, NULL) );
    addToToolsMenu (waypointsDockWidget, tr("Waypoints List"), SLOT(showToolWidget()), MENU_WAYPOINTS, Qt::BottomDockWidgetArea);
    }

    if (!infoDockWidget)
    {
    infoDockWidget = new QDockWidget(tr("Status Details"), this);
    infoDockWidget->setWidget( new UASInfoWidget(this) );
    addToToolsMenu (infoDockWidget, tr("Status Details"), SLOT(showToolWidget()), MENU_STATUS, Qt::RightDockWidgetArea);
    }

    if (!debugConsoleDockWidget)
    {
    debugConsoleDockWidget = new QDockWidget(tr("Communication Console"), this);
    debugConsoleDockWidget->setWidget( new DebugConsole(this) );
    addToToolsMenu (debugConsoleDockWidget, tr("Communication Console"), SLOT(showToolWidget()), MENU_DEBUG_CONSOLE, Qt::BottomDockWidgetArea);
    }

    // Center widgets
    if (!mapWidget)
    {
    mapWidget         = new MapWidget(this);
    addToCentralWidgetsMenu (mapWidget, "Maps", SLOT(showCentralWidget()),CENTRAL_MAP);
    }

    if (!protocolWidget)
    {
    protocolWidget    = new XMLCommProtocolWidget(this);
    addToCentralWidgetsMenu (protocolWidget, "Mavlink Generator", SLOT(showCentralWidget()),CENTRAL_PROTOCOL);
    }
}

void MainWindow::buildPxWidgets()
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

    if (!linechartWidget)
    {
    // Center widgets
    linechartWidget   = new Linecharts(this);
    addToCentralWidgetsMenu(linechartWidget, "Line Plots", SLOT(showCentralWidget()), CENTRAL_LINECHART);
    }


    if (!hudWidget)
    {
    hudWidget         = new HUD(320, 240, this);
    addToCentralWidgetsMenu(hudWidget, "HUD", SLOT(showCentralWidget()), CENTRAL_HUD);
    }

    if (!dataplotWidget)
    {
    dataplotWidget    = new QGCDataPlot2D(this);
    addToCentralWidgetsMenu(dataplotWidget, "Data Plots", SLOT(showCentralWidget()), CENTRAL_DATA_PLOT);
    }

#ifdef QGC_OSG_ENABLED
    if (!_3DWidget)
    {
    _3DWidget         = Q3DWidgetFactory::get("PIXHAWK");
    addToCentralWidgetsMenu(_3DWidget, "Local 3D", SLOT(showCentralWidget()), CENTRAL_3D_LOCAL);
    }
#endif

#ifdef QGC_OSGEARTH_ENABLED
    if (!_3DMapWidget)
    {
    _3DMapWidget = Q3DWidgetFactory::get("MAP3D");
    addToCentralWidgetsMenu(_3DMapWidget, "OSG Earth 3D", SLOT(showCentralWidget()), CENTRAL_OSGEARTH);
    }
#endif

#if (defined _MSC_VER) | (defined Q_OS_MAC)
    if (!gEarthWidget)
    {
    gEarthWidget = new QGCGoogleEarthView(this);
    addToCentralWidgetsMenu(gEarthWidget, "Google Earth", SLOT(showCentralWidget()), CENTRAL_GOOGLE_EARTH);
    }

#endif

    // Dock widgets

    if (!detectionDockWidget)
    {
    detectionDockWidget = new QDockWidget(tr("Object Recognition"), this);
    detectionDockWidget->setWidget( new ObjectDetectionView("images/patterns", this) );
    addToToolsMenu (detectionDockWidget, tr("Object Recognition"), SLOT(showToolWidget()), MENU_DETECTION, Qt::RightDockWidgetArea);
    }

    if (!parametersDockWidget)
    {
    parametersDockWidget = new QDockWidget(tr("Onboard Parameters"), this);
    parametersDockWidget->setWidget( new ParameterInterface(this) );
    addToToolsMenu (parametersDockWidget, tr("Onboard Parameters"), SLOT(showToolWidget()), MENU_PARAMETERS, Qt::RightDockWidgetArea);
    }

    if (!watchdogControlDockWidget)
    {
    watchdogControlDockWidget = new QDockWidget(tr("Process Control"), this);
    watchdogControlDockWidget->setWidget( new WatchdogControl(this) );
    addToToolsMenu (watchdogControlDockWidget, tr("Process Control"), SLOT(showToolWidget()), MENU_WATCHDOG, Qt::BottomDockWidgetArea);
    }

    if (!hsiDockWidget)
    {
    hsiDockWidget = new QDockWidget(tr("Horizontal Situation Indicator"), this);
    hsiDockWidget->setWidget( new HSIDisplay(this) );
    addToToolsMenu (hsiDockWidget, tr("HSI"), SLOT(showToolWidget()), MENU_HSI, Qt::BottomDockWidgetArea);
    }

    if (!headDown1DockWidget)
    {
    headDown1DockWidget = new QDockWidget(tr("System Stats"), this);
    headDown1DockWidget->setWidget( new HDDisplay(acceptList, this) );
    addToToolsMenu (headDown1DockWidget, tr("Flight Display"), SLOT(showToolWidget()), MENU_HDD_1, Qt::RightDockWidgetArea);
    }

    if (!headDown2DockWidget)
    {
    headDown2DockWidget = new QDockWidget(tr("Payload Status"), this);
    headDown2DockWidget->setWidget( new HDDisplay(acceptList2, this) );
    addToToolsMenu (headDown2DockWidget, tr("Payload Status"), SLOT(showToolWidget()), MENU_HDD_2, Qt::RightDockWidgetArea);
    }

    if (!rcViewDockWidget)
    {
    rcViewDockWidget = new QDockWidget(tr("Radio Control"), this);
    rcViewDockWidget->setWidget( new QGCRemoteControlView(this) );
    addToToolsMenu (rcViewDockWidget, tr("Radio Control"), SLOT(showToolWidget()), MENU_RC_VIEW, Qt::BottomDockWidgetArea);
    }

    if (!headUpDockWidget)
    {
    headUpDockWidget = new QDockWidget(tr("HUD"), this);
    headUpDockWidget->setWidget( new HUD(320, 240, this));
    addToToolsMenu (headUpDockWidget, tr("Control Indicator"), SLOT(showToolWidget()), MENU_HUD, Qt::LeftDockWidgetArea);
    }

    // Dialogue widgets
    //FIXME: free memory in destructor
    if (!joystick)
    {
    joystick    = new JoystickInput();
    }

}

void MainWindow::buildSlugsWidgets()
{
    if (!linechartWidget)
    {
    // Center widgets
        linechartWidget   = new Linecharts(this);
    }

    if (!headUpDockWidget)
    {
        // Dock widgets
        headUpDockWidget = new QDockWidget(tr("Control Indicator"), this);
        headUpDockWidget->setWidget( new HUD(320, 240, this));
        addToToolsMenu (headUpDockWidget, tr("HUD"), SLOT(showToolWidget()), MENU_HUD, Qt::LeftDockWidgetArea);
    }

    if (!rcViewDockWidget)
    {
        rcViewDockWidget = new QDockWidget(tr("Radio Control"), this);
        rcViewDockWidget->setWidget( new QGCRemoteControlView(this) );
        addToToolsMenu (rcViewDockWidget, tr("Radio Control"), SLOT(showToolWidget()), MENU_RC_VIEW, Qt::BottomDockWidgetArea);
    }

    if (!slugsDataWidget)
    {
        // Dialog widgets
        slugsDataWidget = new QDockWidget(tr("Slugs Data"), this);
        slugsDataWidget->setWidget( new SlugsDataSensorView(this));
        addToToolsMenu (slugsDataWidget, tr("Telemetry Data"), SLOT(showToolWidget()), MENU_SLUGS_DATA, Qt::RightDockWidgetArea);
    }

    if (!slugsPIDControlWidget)
    {
    slugsPIDControlWidget = new QDockWidget(tr("Slugs PID Control"), this);
    slugsPIDControlWidget->setWidget(new SlugsPIDControl(this));
    addToToolsMenu (slugsPIDControlWidget, tr("PID Configuration"), SLOT(showToolWidget()), MENU_SLUGS_PID, Qt::LeftDockWidgetArea);
    }

    if (!slugsHilSimWidget)
    {
    slugsHilSimWidget = new QDockWidget(tr("Slugs Hil Sim"), this);
    slugsHilSimWidget->setWidget( new SlugsHilSim(this));
    addToToolsMenu (slugsHilSimWidget, tr("HIL Sim Configuration"), SLOT(showToolWidget()), MENU_SLUGS_HIL, Qt::LeftDockWidgetArea);
    }

    if (!slugsCamControlWidget)
    {
    slugsCamControlWidget = new QDockWidget(tr("Slugs Video Camera Control"), this);
    slugsCamControlWidget->setWidget(new SlugsVideoCamControl(this));
    addToToolsMenu (slugsCamControlWidget, tr("Camera Control"), SLOT(showToolWidget()), MENU_SLUGS_CAMERA, Qt::BottomDockWidgetArea);
    }
}


void MainWindow::addToCentralWidgetsMenu ( QWidget* widget,
                                           const QString title,
                                           const char * slotName,
                                           TOOLS_WIDGET_NAMES centralWidget)
{
  QAction* tempAction;


  // Add the separator that will separate tools from central Widgets
    if (!toolsMenuActions[CENTRAL_SEPARATOR])
    {
    tempAction = ui.menuTools->addSeparator();
    toolsMenuActions[CENTRAL_SEPARATOR] = tempAction;
    tempAction->setData(CENTRAL_SEPARATOR);
  }

  tempAction = ui.menuTools->addAction(title);

  tempAction->setCheckable(true);
  tempAction->setData(centralWidget);

  // populate the Hashes
  toolsMenuActions[centralWidget] = tempAction;
  dockWidgets[centralWidget] = widget;

  QString chKey = buildMenuKey(SUB_SECTION_CHECKED, centralWidget, currentView);

    if (!settings.contains(chKey))
    {
    settings.setValue(chKey,false);
    tempAction->setChecked(false);
  }
    else
    {
        tempAction->setChecked(settings.value(chKey).toBool());
    }

  // connect the action
  connect(tempAction,SIGNAL(triggered()),this, slotName);
}


void MainWindow::showCentralWidget()
{
  QAction* senderAction = qobject_cast<QAction *>(sender());
  int tool = senderAction->data().toInt();
  QString chKey;

  // check the current action

    if (senderAction && dockWidgets[tool])
    {

    // uncheck all central widget actions
    QHashIterator<int, QAction*> i(toolsMenuActions);
     while (i.hasNext()) {
       i.next();
       qDebug() << "shCW" << i.key() << "read";
       if (i.value() && i.value()->data().toInt() > 255){
           i.value()->setChecked(false);

           // update the settings
           chKey = buildMenuKey (SUB_SECTION_CHECKED,static_cast<TOOLS_WIDGET_NAMES>(i.value()->data().toInt()), currentView);
           settings.setValue(chKey,false);
         }
     }

    // check the current action
     qDebug() << senderAction->text();
    senderAction->setChecked(true);

    // update the central widget
    centerStack->setCurrentWidget(dockWidgets[tool]);

    // store the selected central widget
    chKey = buildMenuKey (SUB_SECTION_CHECKED,static_cast<TOOLS_WIDGET_NAMES>(tool), currentView);
    settings.setValue(chKey,true);
        settings.sync();

   presentView();
  }
}

/**
 * Adds a widget to the tools menu and sets it visible if it was
 * enabled last time.
 */
void MainWindow::addToToolsMenu ( QWidget* widget,
                                 const QString title,
                                 const char * slotName,
                                 TOOLS_WIDGET_NAMES tool,
                                  Qt::DockWidgetArea location)
{
  QAction* tempAction;
  QString posKey, chKey;


    if (toolsMenuActions[CENTRAL_SEPARATOR])
    {
    tempAction = new QAction(title, this);
    ui.menuTools->insertAction(toolsMenuActions[CENTRAL_SEPARATOR],
                               tempAction);
    }
    else
    {
    tempAction = ui.menuTools->addAction(title);
  }

  tempAction->setCheckable(true);
  tempAction->setData(tool);

  // populate the Hashes
  toolsMenuActions[tool] = tempAction;
  dockWidgets[tool] = widget;
  qDebug() << widget;

  posKey = buildMenuKey (SUB_SECTION_LOCATION,tool, currentView);

    if (!settings.contains(posKey))
    {
    settings.setValue(posKey,location);
    dockWidgetLocations[tool] = location;
  }
    else
    {
        dockWidgetLocations[tool] = static_cast <Qt::DockWidgetArea> (settings.value(posKey, Qt::RightDockWidgetArea).toInt());
    }

  chKey = buildMenuKey(SUB_SECTION_CHECKED,tool, currentView);

    if (!settings.contains(chKey))
    {
    settings.setValue(chKey,false);
    tempAction->setChecked(false);
        widget->setVisible(false);
    }
    else
    {
    tempAction->setChecked(settings.value(chKey).toBool());
        widget->setVisible(settings.value(chKey, false).toBool());
  }

  // connect the action
  connect(tempAction,SIGNAL(triggered()),this, slotName);

  connect(qobject_cast <QDockWidget *>(dockWidgets[tool]),
          SIGNAL(visibilityChanged(bool)), this, SLOT(updateVisibilitySettings(bool)));

  connect(qobject_cast <QDockWidget *>(dockWidgets[tool]),
          SIGNAL(dockLocationChanged(Qt::DockWidgetArea)), this, SLOT(updateLocationSettings(Qt::DockWidgetArea)));

}

void MainWindow::showToolWidget()
{
  QAction* temp = qobject_cast<QAction *>(sender());
  int tool = temp->data().toInt();


    if (temp && dockWidgets[tool])
    {
        if (temp->isChecked())
        {
      addDockWidget(dockWidgetLocations[tool], qobject_cast<QDockWidget *> (dockWidgets[tool]));
      qobject_cast<QDockWidget *>(dockWidgets[tool])->show();
        }
        else
        {
      removeDockWidget(qobject_cast<QDockWidget *>(dockWidgets[tool]));
    }
    QString chKey = buildMenuKey (SUB_SECTION_CHECKED,static_cast<TOOLS_WIDGET_NAMES>(tool), currentView);
    settings.setValue(chKey,temp->isChecked());
        settings.sync();
  }
}


void MainWindow::showTheWidget (TOOLS_WIDGET_NAMES widget, VIEW_SECTIONS view)
{
  bool tempVisible;
  Qt::DockWidgetArea tempLocation;
  QDockWidget* tempWidget = static_cast <QDockWidget *>(dockWidgets[widget]);

    tempVisible =  settings.value(buildMenuKey (SUB_SECTION_CHECKED,widget,view), false).toBool();

    // Some widgets are per default visible. Overwrite the settings value if not present.
    if (widget == MainWindow::MENU_UAS_LIST)
    {
        if (!settings.contains(buildMenuKey (SUB_SECTION_CHECKED,widget,view)))
        {
            tempVisible = true;
        }
    }

    if (tempWidget)
    {
    toolsMenuActions[widget]->setChecked(tempVisible);
  }


  //qDebug() <<  buildMenuKey (SUB_SECTION_CHECKED,widget,view) << tempVisible;

    tempLocation = static_cast <Qt::DockWidgetArea>(settings.value(buildMenuKey (SUB_SECTION_LOCATION,widget, view), QVariant(Qt::RightDockWidgetArea)).toInt());

    //    if (widget == MainWindow::MENU_UAS_LIST)
    //    {
    //        if (!settings.contains(buildMenuKey (SUB_SECTION_LOCATION,widget, view)))
    //        {
    //            tempLocation = Qt::RightDockWidgetArea;
    //        }
    //    }

    if ((tempWidget != NULL) && tempVisible)
    {
    addDockWidget(tempLocation, tempWidget);
    tempWidget->show();
  }

}

QString MainWindow::buildMenuKey(SETTINGS_SECTIONS section, TOOLS_WIDGET_NAMES tool, VIEW_SECTIONS view)
{
  // Key is built as follows: autopilot_type/section_menu/view/tool/section
  int apType;

  apType = (UASManager::instance() && UASManager::instance()->silentGetActiveUAS())?
           UASManager::instance()->getActiveUAS()->getAutopilotType():
           -1;

  return (QString::number(apType) + "/" +
          QString::number(SECTION_MENU) + "/" +
          QString::number(view) + "/" +
          QString::number(tool) + "/" +
          QString::number(section) + "/" );
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    settings.setValue("geometry", saveGeometry());
    //settings.setValue("windowState", saveState());
    aboutToCloseFlag = true;
    settings.setValue("VIEW_ON_APPLICATION_CLOSE", currentView);
    settings.sync();
    QMainWindow::closeEvent(event);
}

/**
 * Stores the visibility setting of each widget. This method
 * will only change the settings if the application is not
 * about to close.
 */
void MainWindow::updateVisibilitySettings (bool vis)
{
    if (!aboutToCloseFlag)
    {

  QDockWidget* temp = qobject_cast<QDockWidget *>(sender());

        if (temp)
        {
  QHashIterator<int, QWidget*> i(dockWidgets);
            while (i.hasNext())
            {
      i.next();
                if ((static_cast <QDockWidget *>(dockWidgets[i.key()])) == temp)
                {
                    QString chKey = buildMenuKey (SUB_SECTION_CHECKED,static_cast<TOOLS_WIDGET_NAMES>(i.key()), currentView);
        settings.setValue(chKey,vis);
                    settings.sync();
        toolsMenuActions[i.key()]->setChecked(vis);
        break;
      }
  }
        }
    }
}

void MainWindow::updateLocationSettings (Qt::DockWidgetArea location)
{
  QDockWidget* temp = qobject_cast<QDockWidget *>(sender());

  QHashIterator<int, QWidget*> i(dockWidgets);
    while (i.hasNext())
    {
      i.next();
        if ((static_cast <QDockWidget *>(dockWidgets[i.key()])) == temp)
        {
        QString posKey = buildMenuKey (SUB_SECTION_LOCATION,static_cast<TOOLS_WIDGET_NAMES>(i.key()), currentView);
        settings.setValue(posKey,location);
        break;
      }
  }
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

    if (mapWidget && waypointsDockWidget->widget())
    {
        // clear path create on the map
        connect(waypointsDockWidget->widget(), SIGNAL(clearPathclicked()), mapWidget, SLOT(clearPath()));
        // add Waypoint widget in the WaypointList widget when mouse clicked
        connect(mapWidget, SIGNAL(captureMapCoordinateClick(QPointF)), waypointsDockWidget->widget(), SLOT(addWaypointMouse(QPointF)));

        // it notifies that a waypoint global goes to do create and a map graphic too
        connect(waypointsDockWidget->widget(), SIGNAL(createWaypointAtMap(QPointF)), mapWidget, SLOT(createWaypointGraphAtMap(QPointF)));
    }

}

void MainWindow::connectPxWidgets()
{
    // No special connections necessary at this point
}

void MainWindow::connectSlugsWidgets()
{
    if (slugsHilSimWidget && slugsHilSimWidget->widget()){
      connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)),
              slugsHilSimWidget->widget(), SLOT(activeUasSet(UASInterface*)));
    }

    if (slugsDataWidget && slugsDataWidget->widget()){
      connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)),
              slugsDataWidget->widget(), SLOT(setActiveUAS(UASInterface*)));
    }


}

void MainWindow::arrangeCommonCenterStack()
{
    centerStack = new QStackedWidget(this);

    if (!centerStack) return;

    if (mapWidget && (centerStack->indexOf(mapWidget) == -1)) centerStack->addWidget(mapWidget);
    if (protocolWidget && (centerStack->indexOf(protocolWidget) == -1)) centerStack->addWidget(protocolWidget);

    setCentralWidget(centerStack);
}

void MainWindow::arrangePxCenterStack()
{

  if (!centerStack) {
    qDebug() << "Center Stack not Created!";
    return;
  }


    if (linechartWidget && (centerStack->indexOf(linechartWidget) == -1)) centerStack->addWidget(linechartWidget);

#ifdef QGC_OSG_ENABLED
    if (_3DWidget && (centerStack->indexOf(_3DWidget) == -1)) centerStack->addWidget(_3DWidget);
#endif
#ifdef QGC_OSGEARTH_ENABLED
    if (_3DMapWidget && (centerStack->indexOf(_3DMapWidget) == -1)) centerStack->addWidget(_3DMapWidget);
#endif
#if (defined _MSC_VER) | (defined Q_OS_MAC)
    if (gEarthWidget && (centerStack->indexOf(gEarthWidget) == -1)) centerStack->addWidget(gEarthWidget);
#endif
    if (hudWidget && (centerStack->indexOf(hudWidget) == -1)) centerStack->addWidget(hudWidget);
    if (dataplotWidget && (centerStack->indexOf(dataplotWidget) == -1)) centerStack->addWidget(dataplotWidget);
}

void MainWindow::arrangeSlugsCenterStack()
{

  if (!centerStack) {
    qDebug() << "Center Stack not Created!";
    return;
  }

    if (linechartWidget && (centerStack->indexOf(linechartWidget) == -1)) centerStack->addWidget(linechartWidget);
    if (hudWidget && (centerStack->indexOf(hudWidget) == -1)) centerStack->addWidget(hudWidget);

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
    Q_UNUSED(status);
    Q_UNUSED(timeout);
    //statusBar->showMessage(status, timeout);
}

void MainWindow::showStatusMessage(const QString& status)
{
    Q_UNUSED(status);
    //statusBar->showMessage(status, 5);
}

/**
* @brief Create all actions associated to the main window
*
**/
void MainWindow::connectCommonActions()
{
    // Connect actions from ui
    connect(ui.actionAdd_Link, SIGNAL(triggered()), this, SLOT(addLink()));

    // Connect internal actions
    connect(UASManager::instance(), SIGNAL(UASCreated(UASInterface*)), this, SLOT(UASCreated(UASInterface*)));

    // Unmanned System controls
    connect(ui.actionLiftoff, SIGNAL(triggered()), UASManager::instance(), SLOT(launchActiveUAS()));
    connect(ui.actionLand, SIGNAL(triggered()), UASManager::instance(), SLOT(returnActiveUAS()));
    connect(ui.actionEmergency_Land, SIGNAL(triggered()), UASManager::instance(), SLOT(stopActiveUAS()));
    connect(ui.actionEmergency_Kill, SIGNAL(triggered()), UASManager::instance(), SLOT(killActiveUAS()));

    connect(ui.actionConfiguration, SIGNAL(triggered()), UASManager::instance(), SLOT(configureActiveUAS()));

    // Views actions
    connect(ui.actionPilotsView, SIGNAL(triggered()), this, SLOT(loadPilotView()));
    connect(ui.actionEngineersView, SIGNAL(triggered()), this, SLOT(loadEngineerView()));
    connect(ui.actionOperatorsView, SIGNAL(triggered()), this, SLOT(loadOperatorView()));

    connect(ui.actionMavlinkView, SIGNAL(triggered()), this, SLOT(loadMAVLinkView()));
    connect(ui.actionReloadStyle, SIGNAL(triggered()), this, SLOT(reloadStylesheet()));

    // Help Actions
    connect(ui.actionOnline_documentation, SIGNAL(triggered()), this, SLOT(showHelp()));
    connect(ui.actionDeveloper_Credits, SIGNAL(triggered()), this, SLOT(showCredits()));
    connect(ui.actionProject_Roadmap, SIGNAL(triggered()), this, SLOT(showRoadMap()));
}

void MainWindow::connectPxActions()
{
  ui.actionJoystickSettings->setVisible(true);

  // Joystick configuration
  connect(ui.actionJoystickSettings, SIGNAL(triggered()), this, SLOT(configure()));

}

void MainWindow::connectSlugsActions()
{

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

        switch (uas->getAutopilotType())
          {
          case (MAV_AUTOPILOT_SLUGS):
          {
            // Build Slugs Widgets
            buildSlugsWidgets();

            // Connect Slugs Widgets
            connectSlugsWidgets();

            // Arrange Slugs Centerstack
            arrangeSlugsCenterStack();

            // Connect Slugs Actions
            connectSlugsActions();

            // FIXME: This type checking might be redundant
                //            if (slugsDataWidget) {
                //              SlugsDataSensorView* dataWidget = dynamic_cast<SlugsDataSensorView*>(slugsDataWidget->widget());
                //              if (dataWidget) {
                //                SlugsMAV* mav2 = dynamic_cast<SlugsMAV*>(uas);
                //                if (mav2) {
                  (dynamic_cast<SlugsDataSensorView*>(slugsDataWidget->widget()))->addUAS(uas);
                //                  //loadSlugsView();
                //                  loadGlobalOperatorView();
                //                }
                //              }
                //            }
            }
            break;
        default:
        case (MAV_AUTOPILOT_GENERIC):
        case (MAV_AUTOPILOT_ARDUPILOTMEGA):
        case (MAV_AUTOPILOT_PIXHAWK):
            {
                // Build Pixhawk Widgets
                buildPxWidgets();

                // Connect Pixhawk Widgets
                connectPxWidgets();

                // Arrange Pixhawk Centerstack
                arrangePxCenterStack();

                // Connect Pixhawk Actions
                connectPxActions();

        }
            break;

    }

        // Change the view only if this is the first UAS

        // If this is the first connected UAS, it is both created as well as
        // the currently active UAS
        if (UASManager::instance()->getActiveUAS() == uas)
        {
            qDebug() << "UPDATING THE VIEW SINCE THIS IS THE FIRST CONNECTED SYSTEM";

            // Load last view if setting is present
            if (settings.contains("VIEW_ON_APPLICATION_CLOSE"))
    {
                int view = settings.value("VIEW_ON_APPLICATION_CLOSE").toInt();
                currentView = (VIEW_SECTIONS) view;
                presentView();
    }
            else
    {
                loadEngineerView();
    }
    }

    }
}

/**
 * Clears the current view completely
 */
void MainWindow::clearView()
{
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
            //this->removeDockWidget(dockWidget);
            dockWidget->setVisible(false);
            // Deletion of dockWidget would also delete all child
            // widgets of dockWidget
            // Is there a way to unset a widget from QDockWidget?
        }
    }
}

void MainWindow::loadEngineerView()
{
  clearView();

  currentView = VIEW_ENGINEER;

  presentView();
}

void MainWindow::loadOperatorView()
{
    clearView();

    currentView = VIEW_OPERATOR;

    presentView();
}

void MainWindow::loadPilotView()
{
    clearView();

    currentView = VIEW_PILOT;

    presentView();
}

void MainWindow::loadMAVLinkView()
{
    clearView();

    currentView = VIEW_MAVLINK;

    presentView();
}

void MainWindow::presentView()
{
  qDebug() << "LC";
  showTheCentralWidget(CENTRAL_LINECHART, currentView);

  // MAP
  qDebug() << "MAP";
  showTheCentralWidget(CENTRAL_MAP, currentView);

  // PROTOCOL
  qDebug() << "CP";
  showTheCentralWidget(CENTRAL_PROTOCOL, currentView);

  // HEAD UP DISPLAY
  showTheCentralWidget(CENTRAL_HUD, currentView);

    // GOOGLE EARTH
    showTheCentralWidget(CENTRAL_GOOGLE_EARTH, currentView);

    // LOCAL 3D VIEW
    showTheCentralWidget(CENTRAL_3D_LOCAL, currentView);

    // GLOBAL 3D VIEW
    showTheCentralWidget(CENTRAL_3D_MAP, currentView);



  // Show docked widgets based on current view and autopilot type

  // UAS CONTROL
  showTheWidget(MENU_UAS_CONTROL, currentView);

  // UAS LIST
  showTheWidget(MENU_UAS_LIST, currentView);

  // WAYPOINT LIST
  showTheWidget(MENU_WAYPOINTS, currentView);

  // UAS STATUS
  showTheWidget(MENU_STATUS, currentView);

  // DETECTION
  showTheWidget(MENU_DETECTION, currentView);

  // DEBUG CONSOLE
  showTheWidget(MENU_DEBUG_CONSOLE, currentView);

  // ONBOARD PARAMETERS
  showTheWidget(MENU_PARAMETERS, currentView);

  // WATCHDOG
  showTheWidget(MENU_WATCHDOG, currentView);

  // HUD
  showTheWidget(MENU_HUD, currentView);
  if (headUpDockWidget)
  {
      HUD* tmpHud = dynamic_cast<HUD*>( headUpDockWidget->widget() );
      if (tmpHud){
        if (settings.value(buildMenuKey (SUB_SECTION_CHECKED,MENU_HUD,currentView)).toBool()){
          addDockWidget(static_cast <Qt::DockWidgetArea>(settings.value(buildMenuKey (SUB_SECTION_LOCATION,MENU_HUD, currentView)).toInt()),
                        headUpDockWidget);
          headUpDockWidget->show();
        } else {
          headUpDockWidget->hide();
        }
      }
  }


  // RC View
  showTheWidget(MENU_RC_VIEW, currentView);

  // SLUGS DATA
  showTheWidget(MENU_SLUGS_DATA, currentView);

  // SLUGS PID
  showTheWidget(MENU_SLUGS_PID, currentView);

  // SLUGS HIL
  showTheWidget(MENU_SLUGS_HIL, currentView);

  // SLUGS CAMERA
  showTheWidget(MENU_SLUGS_CAMERA, currentView);

  // HORIZONTAL SITUATION INDICATOR
  showTheWidget(MENU_HSI, currentView);
    //    if (hsiDockWidget)
    //    {
    //        HSIDisplay* hsi = dynamic_cast<HSIDisplay*>( hsiDockWidget->widget() );
    //        if (hsi){
    //            if (settings.value(buildMenuKey (SUB_SECTION_CHECKED,MENU_HSI,currentView)).toBool()){
    //                addDockWidget(static_cast <Qt::DockWidgetArea>(settings.value(buildMenuKey (SUB_SECTION_LOCATION,MENU_HSI, currentView)).toInt()),
    //                              hsiDockWidget);
    //            }
    //        }
    //    }

  // HEAD DOWN 1
  showTheWidget(MENU_HDD_1, currentView);
    //    if (headDown1DockWidget)
    //    {
    //        HDDisplay *hdd = dynamic_cast<HDDisplay*>(headDown1DockWidget->widget());
    //        if (hdd) {
    //            if (settings.value(buildMenuKey (SUB_SECTION_CHECKED,MENU_HDD_1,currentView)).toBool()) {
    //                addDockWidget(static_cast <Qt::DockWidgetArea>(settings.value(buildMenuKey (SUB_SECTION_LOCATION,MENU_HDD_1, currentView)).toInt()),
    //                              headDown1DockWidget);
    //                headDown1DockWidget->show();
    //                hdd->start();
    //            } else {
    //                headDown1DockWidget->hide();;
    //                hdd->stop();
    //            }
    //        }
    //    }

  // HEAD DOWN 2
  showTheWidget(MENU_HDD_2, currentView);
    //    if (headDown2DockWidget)
    //    {
    //        HDDisplay *hdd = dynamic_cast<HDDisplay*>(headDown2DockWidget->widget());
    //        if (hdd){
    //            if (settings.value(buildMenuKey (SUB_SECTION_CHECKED,MENU_HDD_2,currentView)).toBool()){
    //                addDockWidget(static_cast <Qt::DockWidgetArea>(settings.value(buildMenuKey (SUB_SECTION_LOCATION,MENU_HDD_2, currentView)).toInt()),
    //                              headDown2DockWidget);
    //                headDown2DockWidget->show();
    //                hdd->start();
    //            } else {
    //                headDown2DockWidget->hide();
    //                hdd->stop();
    //            }
    //        }
    //    }

  this->show();

}

void MainWindow::showTheCentralWidget (TOOLS_WIDGET_NAMES centralWidget, VIEW_SECTIONS view)
{
  bool tempVisible;
  QWidget* tempWidget = dockWidgets[centralWidget];

  tempVisible =  settings.value(buildMenuKey (SUB_SECTION_CHECKED,centralWidget,view)).toBool();
  qDebug() << buildMenuKey (SUB_SECTION_CHECKED,centralWidget,view) << tempVisible;
    if (toolsMenuActions[centralWidget])
    {
    toolsMenuActions[centralWidget]->setChecked(tempVisible);
  }

    if (centerStack && tempWidget && tempVisible)
    {
    centerStack->setCurrentWidget(tempWidget);
  }
}

void MainWindow::loadWidgets()
{
    loadOperatorView();
    //loadMAVLinkView();
    //loadPilotView();
}

void MainWindow::loadDataView(QString fileName)
{
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

void MainWindow::load3DMapView()
{
#ifdef QGC_OSGEARTH_ENABLED
            clearView();

            // 3D map
            if (_3DMapWidget)
            {
                QStackedWidget *centerStack = dynamic_cast<QStackedWidget*>(centralWidget());
                if (centerStack)
                {
                    //map3DWidget->setActive(true);
                    centerStack->setCurrentWidget(_3DMapWidget);
                }
            }
#endif
}

void MainWindow::loadGoogleEarthView()
{
#if (defined _MSC_VER) | (defined Q_OS_MAC)
            clearView();

            // 3D map
            if (gEarthWidget)
            {
                QStackedWidget *centerStack = dynamic_cast<QStackedWidget*>(centralWidget());
                if (centerStack)
                {
                    centerStack->setCurrentWidget(gEarthWidget);
                }
            }
#endif

}

void MainWindow::load3DView()
{
#ifdef QGC_OSG_ENABLED
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
#endif
}

