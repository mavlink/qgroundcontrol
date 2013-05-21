/*
TODO:
- position setting for ribbons (default is center)
- numer label in roll indicator
- ivert angles for horizon
- battery indicator
- message widget
*/

#include <QtGui>

#include "HUD2.h"
#include "HUD2Math.h"

#include "UASManager.h"
#include "UAS.h"

HUD2::~HUD2()
{
    switch(renderType){
    case RENDER_TYPE_NATIVE:
        delete render_native;
        break;
    case RENDER_TYPE_OPENGL:
        delete render_gl;
        break;
    default:
        break;
    }
    delete layout;
}

HUD2::HUD2(QWidget *parent)
    : QWidget(parent),
      uas(NULL),
      huddrawer(&huddata, this)
{
    // Load settings
    QSettings settings;
    settings.beginGroup("QGC_HUD2");
    renderType = settings.value("RENDER_TYPE", 0).toInt();
    hud2_clamp(renderType, 0, (RENDER_TYPE_ENUM_END - 1));
    fpsLimit = settings.value("FPS_LIMIT", HUD2_FPS_DEFAULT).toInt();
    hud2_clamp(fpsLimit, HUD2_FPS_MIN, HUD2_FPS_MAX);
    antiAliasing = settings.value("ANTIALIASING", true).toBool();
    settings.endGroup();

    setMinimumSize(160, 120);

    layout = new QGridLayout(this);

    switch(renderType){
    case RENDER_TYPE_NATIVE:
        render_native = new HUD2RenderNative(&huddrawer, this);
        layout->addWidget(render_native, 0, 0);
        break;
    case RENDER_TYPE_OPENGL:
        render_gl = new HUD2RenderGL(&huddrawer, this);
        layout->addWidget(render_gl, 0, 0);
        break;
    default:
        break;
    }

    toggleAntialising(antiAliasing);

    setLayout(layout);

    fpsLimiter.setInterval(1000 / fpsLimit);
    connect(&fpsLimiter, SIGNAL(timeout()), this, SLOT(enableRepaint()));
    fpsLimiter.start();

    // Connect with UAS
    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)),
            this,                   SLOT(setActiveUAS(UASInterface*)));
    createActions();
    if (UASManager::instance()->getActiveUAS() != NULL)
        setActiveUAS(UASManager::instance()->getActiveUAS());
}


void HUD2::paint(void){
    switch(renderType){
    case RENDER_TYPE_NATIVE:
        render_native->paint();
        break;
    case RENDER_TYPE_OPENGL:
        render_gl->paint();
        break;
    default:
        break;
    }
}


void HUD2::switchRender(int type)
{
    renderType = type;

    switch(renderType){
    case RENDER_TYPE_NATIVE:
        layout->removeWidget(render_gl);
        delete render_gl;
        render_native = new HUD2RenderNative(&huddrawer, this);
        layout->addWidget(render_native, 0, 0);
        break;
    case RENDER_TYPE_OPENGL:
        layout->removeWidget(render_native);
        delete render_native;
        render_gl = new HUD2RenderGL(&huddrawer, this);
        layout->addWidget(render_gl, 0, 0);
        break;
    default:
        qDebug() << "UNHANDLED RENDER TYPE";
        break;
    }

    // update anti aliasing settings for new render
    toggleAntialising(antiAliasing);

    // Save settings
    QSettings settings;
    settings.setValue("QGC_HUD2/RENDER_TYPE", renderType);
}


void HUD2::updateAttitude(UASInterface* uas, double roll, double pitch,
                          double yaw, quint64 timestamp)
{
    Q_UNUSED(uas);
    Q_UNUSED(timestamp);

    if (!isnan(roll) && !isinf(roll))
        huddata.roll  = roll;
    if (!isnan(pitch) && !isinf(pitch))
        huddata.pitch = pitch;
    if (!isnan(yaw) && !isinf(yaw))
        huddata.yaw   = yaw;

    if (repaintEnabled)
    {
        this->paint();
        repaintEnabled = false;
    }
}

void HUD2::updateGlobalPosition(UASInterface* uas, double lat, double lon,
                                double altitude, quint64 timestamp)
{
    Q_UNUSED(uas);
    Q_UNUSED(timestamp);
    Q_UNUSED(lat);
    Q_UNUSED(lon);
    huddata.alt_gnss = altitude;
}

void HUD2::updateBattery(UASInterface* uas, double voltage, double percent, int seconds){
    Q_UNUSED(uas);
    huddata.batt_voltage = voltage;
    huddata.batt_charge = percent;
    huddata.batt_time = seconds;
}

void HUD2::updateThrust(UASInterface* uas, double thrust){
    Q_UNUSED(uas);
    huddata.thrust = thrust;
}

void HUD2::updateAltitude(int uasid, double alt){
    Q_UNUSED(uasid);
    huddata.alt_baro = alt;
}

void HUD2::updateSpeed(UASInterface *uas, double airspeed, double groundspeed,
                       double climb, quint64 time){
    Q_UNUSED(uas);
    Q_UNUSED(time);
    huddata.airspeed = airspeed;
    huddata.groundspeed = groundspeed;
    huddata.climb = climb;
}

void HUD2::updateTextMessage(int uasid, int componentid, int severity, QString text){
    this->huddrawer.updateTextMessage(uasid, componentid, severity, text);
}

/**
 * @param uas the UAS/MAV to monitor/display with the HUD
 */
void HUD2::setActiveUAS(UASInterface* uas)
{
    if (this->uas != NULL) {
        // Disconnect any previously connected active MAV
        disconnect(this->uas, SIGNAL(attitudeChanged(UASInterface*,double,double,double,quint64)),
                   this, SLOT(updateAttitude(UASInterface*, double, double, double, quint64)));
        disconnect(this->uas, SIGNAL(attitudeChanged(UASInterface*,int,double,double,double,quint64)),
                   this, SLOT(updateAttitude(UASInterface*,int,double, double, double, quint64)));
        disconnect(this->uas, SIGNAL(batteryChanged(UASInterface*, double, double, int)),
                   this, SLOT(updateBattery(UASInterface*, double, double, int)));
        disconnect(this->uas, SIGNAL(statusChanged(UASInterface*,QString,QString)),
                   this, SLOT(updateState(UASInterface*,QString)));
        disconnect(this->uas, SIGNAL(modeChanged(int,QString,QString)),
                   this, SLOT(updateMode(int,QString,QString)));
        disconnect(this->uas, SIGNAL(heartbeat(UASInterface*)),
                   this, SLOT(receiveHeartbeat(UASInterface*)));
        disconnect(this->uas, SIGNAL(altitudeChanged(int,double)),
                   this, SLOT(updateAltitude(int,double)));
        disconnect(this->uas, SIGNAL(speedChanged(UASInterface*,double,double,double,quint64)),
                   this, SLOT(updateSpeed(UASInterface*,double,double,double,quint64)));
        disconnect(this->uas, SIGNAL(textMessageReceived(int,int,int,QString)),
                   this, SLOT(updateTextMessage(int,int,int,QString)));

        disconnect(this->uas, SIGNAL(localPositionChanged(UASInterface*,double,double,double,quint64)),
                   this, SLOT(updateLocalPosition(UASInterface*,double,double,double,quint64)));
        disconnect(this->uas, SIGNAL(globalPositionChanged(UASInterface*,double,double,double,quint64)),
                   this, SLOT(updateGlobalPosition(UASInterface*,double,double,double,quint64)));
        disconnect(this->uas, SIGNAL(waypointSelected(int,int)),
                   this, SLOT(selectWaypoint(int, int)));
    }

    if (uas) {
        // Now connect the new UAS
        // Setup communication
        connect(uas, SIGNAL(attitudeChanged(UASInterface*,double,double,double,quint64)),
                this, SLOT(updateAttitude(UASInterface*, double, double, double, quint64)));
        connect(uas, SIGNAL(attitudeChanged(UASInterface*,int,double,double,double,quint64)),
                this, SLOT(updateAttitude(UASInterface*,int,double, double, double, quint64)));
        connect(uas, SIGNAL(batteryChanged(UASInterface*, double, double, int)),
                this, SLOT(updateBattery(UASInterface*, double, double, int)));
        connect(uas, SIGNAL(statusChanged(UASInterface*,QString,QString)),
                this, SLOT(updateState(UASInterface*,QString)));
        connect(uas, SIGNAL(modeChanged(int,QString,QString)),
                this, SLOT(updateMode(int,QString,QString)));
        connect(uas, SIGNAL(heartbeat(UASInterface*)),
                this, SLOT(receiveHeartbeat(UASInterface*)));
        connect(uas, SIGNAL(altitudeChanged(int,double)),
                this, SLOT(updateAltitude(int,double)));
        connect(uas, SIGNAL(speedChanged(UASInterface*,double,double,double,quint64)),
                this, SLOT(updateSpeed(UASInterface*,double,double,double,quint64)));
        connect(uas, SIGNAL(textMessageReceived(int,int,int,QString)),
                this, SLOT(updateTextMessage(int,int,int,QString)));

        connect(uas, SIGNAL(localPositionChanged(UASInterface*,double,double,double,quint64)),
                this, SLOT(updateLocalPosition(UASInterface*,double,double,double,quint64)));
        connect(uas, SIGNAL(globalPositionChanged(UASInterface*,double,double,double,quint64)),
                this, SLOT(updateGlobalPosition(UASInterface*,double,double,double,quint64)));
        connect(uas, SIGNAL(waypointSelected(int,int)),
                this, SLOT(selectWaypoint(int, int)));

        // Set new UAS
        this->uas = uas;
    }
}

void HUD2::createActions()
{
    renderDialogHUDAction = new QAction(tr("Render"), this);
    renderDialogHUDAction->setStatusTip(tr("Render settings"));
    connect(renderDialogHUDAction, SIGNAL(triggered(bool)), this, SLOT(renderDialog()));

    instrumentsDialogHUDAction = new QAction(tr("Instruments"), this);
    instrumentsDialogHUDAction->setStatusTip(tr("HUD instruments settings"));
    connect(instrumentsDialogHUDAction, SIGNAL(triggered(bool)), &huddrawer, SLOT(showDialog()));

    colorDialogHUDAction = new QAction(tr("Colors"), this);
    colorDialogHUDAction->setStatusTip(tr("Color settings"));
    connect(colorDialogHUDAction, SIGNAL(triggered(bool)), &huddrawer, SLOT(showColorDialog()));
}

void HUD2::renderDialog()
{
    HUD2DialogRender *settings_dialog = new HUD2DialogRender(this);
    settings_dialog->exec();
    delete settings_dialog;
}

void HUD2::enableRepaint(void){
    repaintEnabled = true;
}

void HUD2::toggleAntialising(bool aa){
    antiAliasing = aa;

    switch(renderType){
    case RENDER_TYPE_NATIVE:
        render_native->toggleAntialiasing(aa);
        break;
    case RENDER_TYPE_OPENGL:
        render_gl->toggleAntialiasing(aa);
        break;
    default:
        break;
    }

    // Save settings
    QSettings settings;
    settings.setValue("QGC_HUD2/ANTIALIASING", aa);
}

void HUD2::contextMenuEvent (QContextMenuEvent* event)
{
    Q_UNUSED(event);
    QMenu menu(this);

    menu.addAction(renderDialogHUDAction);
    menu.addAction(instrumentsDialogHUDAction);
    menu.addAction(colorDialogHUDAction);
    menu.exec(event->globalPos());
}

void HUD2::setFpsLimit(int limit){
    fpsLimit = limit;
    fpsLimiter.setInterval(1000 / fpsLimit);

    // Save settings
    QSettings settings;
    hud2_clamp(fpsLimit, HUD2_FPS_MIN, HUD2_FPS_MAX);
    settings.setValue("QGC_HUD2/FPS_LIMIT", fpsLimit);
}

