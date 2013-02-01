/*
 * Convetions:
 *
 * - all outer dimensions specified in percents of widget sizes (height,
 *   width or diagonal). Type is qreal.
 */

/*
TODO:
- color chosers for hud elements
- tiny pitch lines on horizon
- convert all sizes to percents
- code for detection of invisible elements in yaw and pitch idicators
- battery indicator
- altimeter
- climb meter
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
      uas(NULL)
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
        render_native = new HUD2RenderNative(huddata, this);
        layout->addWidget(render_native, 0, 0);
        break;
    case RENDER_TYPE_OPENGL:
        render_gl = new HUD2RenderGL(huddata, this);
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
        render_native = new HUD2RenderNative(huddata, this);
        layout->addWidget(render_native, 0, 0);
        break;
    case RENDER_TYPE_OPENGL:
        layout->removeWidget(render_native);
        delete render_native;
        render_gl = new HUD2RenderGL(huddata, this);
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


void HUD2::updateAttitude(UASInterface* uas, double roll, double pitch, double yaw, quint64 timestamp)
{
    Q_UNUSED(uas);
    Q_UNUSED(timestamp);
    if (!isnan(roll) && !isinf(roll) && !isnan(pitch) && !isinf(pitch) && !isnan(yaw) && !isinf(yaw))
    {
        huddata.roll  = roll;
        huddata.pitch = pitch;
        huddata.yaw   = yaw;
        if (repaintEnabled){
            this->paint();
            repaintEnabled = false;
        }
    }
}


void HUD2::updateGlobalPosition(UASInterface* uas, double lat, double lon, double altitude, quint64 timestamp)
{
    Q_UNUSED(uas);
    Q_UNUSED(timestamp);
    huddata.lat = lat;
    huddata.lon = lon;
    huddata.alt = altitude;
}


/**
 * @param uas the UAS/MAV to monitor/display with the HUD
 */
void HUD2::setActiveUAS(UASInterface* uas)
{
    if (this->uas != NULL) {
        // Disconnect any previously connected active MAV
        disconnect(this->uas, SIGNAL(attitudeChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateAttitude(UASInterface*, double, double, double, quint64)));
        disconnect(this->uas, SIGNAL(attitudeChanged(UASInterface*,int,double,double,double,quint64)), this, SLOT(updateAttitude(UASInterface*,int,double, double, double, quint64)));
        disconnect(this->uas, SIGNAL(batteryChanged(UASInterface*, double, double, int)), this, SLOT(updateBattery(UASInterface*, double, double, int)));
        disconnect(this->uas, SIGNAL(statusChanged(UASInterface*,QString,QString)), this, SLOT(updateState(UASInterface*,QString)));
        disconnect(this->uas, SIGNAL(modeChanged(int,QString,QString)), this, SLOT(updateMode(int,QString,QString)));
        disconnect(this->uas, SIGNAL(heartbeat(UASInterface*)), this, SLOT(receiveHeartbeat(UASInterface*)));

        disconnect(this->uas, SIGNAL(localPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateLocalPosition(UASInterface*,double,double,double,quint64)));
        disconnect(this->uas, SIGNAL(globalPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateGlobalPosition(UASInterface*,double,double,double,quint64)));
        disconnect(this->uas, SIGNAL(speedChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateSpeed(UASInterface*,double,double,double,quint64)));
        disconnect(this->uas, SIGNAL(waypointSelected(int,int)), this, SLOT(selectWaypoint(int, int)));
    }

    if (uas) {
        // Now connect the new UAS
        // Setup communication
        connect(uas, SIGNAL(attitudeChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateAttitude(UASInterface*, double, double, double, quint64)));
        connect(uas, SIGNAL(attitudeChanged(UASInterface*,int,double,double,double,quint64)), this, SLOT(updateAttitude(UASInterface*,int,double, double, double, quint64)));
        connect(uas, SIGNAL(batteryChanged(UASInterface*, double, double, int)), this, SLOT(updateBattery(UASInterface*, double, double, int)));
        connect(uas, SIGNAL(statusChanged(UASInterface*,QString,QString)), this, SLOT(updateState(UASInterface*,QString)));
        connect(uas, SIGNAL(modeChanged(int,QString,QString)), this, SLOT(updateMode(int,QString,QString)));
        connect(uas, SIGNAL(heartbeat(UASInterface*)), this, SLOT(receiveHeartbeat(UASInterface*)));

        connect(uas, SIGNAL(localPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateLocalPosition(UASInterface*,double,double,double,quint64)));
        connect(uas, SIGNAL(globalPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateGlobalPosition(UASInterface*,double,double,double,quint64)));
        connect(uas, SIGNAL(speedChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateSpeed(UASInterface*,double,double,double,quint64)));
        connect(uas, SIGNAL(waypointSelected(int,int)), this, SLOT(selectWaypoint(int, int)));

        // Set new UAS
        this->uas = uas;
    }
}

void HUD2::createActions()
{
//    enableHUDAction = new QAction(tr("Enable HUD"), this);
//    enableHUDAction->setStatusTip(tr("Show the HUD instruments in this window"));
//    enableHUDAction->setCheckable(true);
//    enableHUDAction->setChecked(hudInstrumentsEnabled);
//    connect(enableHUDAction, SIGNAL(triggered(bool)), this, SLOT(enableHUDInstruments(bool)));
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

    settings_dialog = new HUD2Dialog(this);
    settings_dialog->exec();
    delete settings_dialog;
    //menu.addAction(enableHUDAction);
}

void HUD2::setFpsLimit(int limit){
    fpsLimit = limit;
    fpsLimiter.setInterval(1000 / fpsLimit);

    // Save settings
    QSettings settings;
    hud2_clamp(fpsLimit, HUD2_FPS_MIN, HUD2_FPS_MAX);
    settings.setValue("QGC_HUD2/FPS_LIMIT", fpsLimit);
}
