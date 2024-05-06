/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MAVLinkInspectorController.h"
#include "QGCApplication.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "MAVLinkProtocol.h"
#include "MAVLinkChartController.h"
#include "MAVLinkSystem.h"
#include "MAVLinkMessage.h"
#include "QGCLoggingCategory.h"

#include <QtQml/QQmlEngine>

QGC_LOGGING_CATEGORY(MAVLinkInspectorControllerLog, "qgc.analyzeview.mavlinkinspectorcontroller")

//-----------------------------------------------------------------------------
MAVLinkInspectorController::MAVLinkInspectorController()
{
    MultiVehicleManager* multiVehicleManager = qgcApp()->toolbox()->multiVehicleManager();
    connect(multiVehicleManager, &MultiVehicleManager::vehicleAdded,   this, &MAVLinkInspectorController::_vehicleAdded);
    connect(multiVehicleManager, &MultiVehicleManager::vehicleRemoved, this, &MAVLinkInspectorController::_vehicleRemoved);
    connect(multiVehicleManager, &MultiVehicleManager::activeVehicleChanged, this, &MAVLinkInspectorController::_setActiveVehicle);
    MAVLinkProtocol* mavlinkProtocol = qgcApp()->toolbox()->mavlinkProtocol();
    connect(mavlinkProtocol, &MAVLinkProtocol::messageReceived, this, &MAVLinkInspectorController::_receiveMessage);
    connect(&_updateFrequencyTimer, &QTimer::timeout, this, &MAVLinkInspectorController::_refreshFrequency);
    _updateFrequencyTimer.start(1000);
    _timeScaleSt.append(new TimeScale_st(this, tr("5 Sec"),   5 * 1000));
    _timeScaleSt.append(new TimeScale_st(this, tr("10 Sec"), 10 * 1000));
    _timeScaleSt.append(new TimeScale_st(this, tr("30 Sec"), 30 * 1000));
    _timeScaleSt.append(new TimeScale_st(this, tr("60 Sec"), 60 * 1000));
    emit timeScalesChanged();
    _rangeSt.append(new Range_st(this, tr("Auto"),    0));
    _rangeSt.append(new Range_st(this, tr("10,000"),  10000));
    _rangeSt.append(new Range_st(this, tr("1,000"),   1000));
    _rangeSt.append(new Range_st(this, tr("100"),     100));
    _rangeSt.append(new Range_st(this, tr("10"),      10));
    _rangeSt.append(new Range_st(this, tr("1"),       1));
    _rangeSt.append(new Range_st(this, tr("0.1"),     0.1));
    _rangeSt.append(new Range_st(this, tr("0.01"),    0.01));
    _rangeSt.append(new Range_st(this, tr("0.001"),   0.001));
    _rangeSt.append(new Range_st(this, tr("0.0001"),  0.0001));
    emit rangeListChanged();
}

//-----------------------------------------------------------------------------
MAVLinkInspectorController::~MAVLinkInspectorController()
{
    _charts.clearAndDeleteContents();
    _systems.clearAndDeleteContents();
}

//----------------------------------------------------------------------------------------
QStringList
MAVLinkInspectorController::timeScales()
{
    if(!_timeScales.count()) {
        for(int i = 0; i < _timeScaleSt.count(); i++) {
            _timeScales << _timeScaleSt[i]->label;
        }
    }
    return _timeScales;
}

//----------------------------------------------------------------------------------------
QStringList
MAVLinkInspectorController::rangeList()
{
    if(!_rangeList.count()) {
        for(int i = 0; i < _rangeSt.count(); i++) {
            _rangeList << _rangeSt[i]->label;
        }
    }
    return _rangeList;
}

//----------------------------------------------------------------------------------------
void
MAVLinkInspectorController::_setActiveVehicle(Vehicle* vehicle)
{
    if(vehicle) {
        QGCMAVLinkSystem* v = _findVehicle(static_cast<uint8_t>(vehicle->id()));
        if(v) {
            _activeSystem = v;
        } else {
            _activeSystem = nullptr;
        }
    } else {
        _activeSystem = nullptr;
    }
    emit activeSystemChanged();
}

//-----------------------------------------------------------------------------
QGCMAVLinkSystem*
MAVLinkInspectorController::_findVehicle(uint8_t id)
{
    for(int i = 0; i < _systems.count(); i++) {
        QGCMAVLinkSystem* v = qobject_cast<QGCMAVLinkSystem*>(_systems.get(i));
        if(v) {
            if(v->id() == id) {
                return v;
            }
        }
    }
    return nullptr;
}

//-----------------------------------------------------------------------------
void
MAVLinkInspectorController::_refreshFrequency()
{
    for(int i = 0; i < _systems.count(); i++) {
        QGCMAVLinkSystem* v = qobject_cast<QGCMAVLinkSystem*>(_systems.get(i));
        if(v) {
            for(int i = 0; i < v->messages()->count(); i++) {
                QGCMAVLinkMessage* m = qobject_cast<QGCMAVLinkMessage*>(v->messages()->get(i));
                if(m) {
                    m->updateFreq();
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
void
MAVLinkInspectorController::_vehicleAdded(Vehicle* vehicle)
{
    QGCMAVLinkSystem* sys = _findVehicle(static_cast<uint8_t>(vehicle->id()));
    if(sys)
    {
        sys->messages()->clearAndDeleteContents();
    }
    else
    {
        sys = new QGCMAVLinkSystem(this, static_cast<uint8_t>(vehicle->id()));
        _systems.append(sys);
        _systemNames.append(tr("System %1").arg(vehicle->id()));
        connect(vehicle, &Vehicle::mavlinkMsgIntervalsChanged, sys, [sys](uint8_t compid, uint16_t msgId, int32_t rate)
        {
            for(int i = 0; i < sys->messages()->count(); i++)
            {
                QGCMAVLinkMessage* msg = qobject_cast<QGCMAVLinkMessage*>(sys->messages()->get(i));
                if((msg->compId() == compid) && (msg->id() == msgId))
                {
                    msg->setTargetRateHz(rate);
                    break;
                }
            }
        });
    }
    emit systemsChanged();
}

//-----------------------------------------------------------------------------
void
MAVLinkInspectorController::_vehicleRemoved(Vehicle* vehicle)
{
    QGCMAVLinkSystem* v = _findVehicle(static_cast<uint8_t>(vehicle->id()));
    if(v) {
        v->deleteLater();
        _systems.removeOne(v);
        QString vs = tr("System %1").arg(vehicle->id());
        _systemNames.removeOne(vs);
        emit systemsChanged();
    }
}

//-----------------------------------------------------------------------------
void
MAVLinkInspectorController::_receiveMessage(LinkInterface*, mavlink_message_t message)
{
    QGCMAVLinkMessage* m = nullptr;
    QGCMAVLinkSystem* v = _findVehicle(message.sysid);
    if(!v) {
        v = new QGCMAVLinkSystem(this, message.sysid);
        _systems.append(v);
        _systemNames.append(tr("System %1").arg(message.sysid));
        emit systemsChanged();
        if(!_activeSystem) {
            _activeSystem = v;
            emit activeSystemChanged();
        }
    } else {
        m = v->findMessage(message.msgid, message.compid);
    }
    if(!m) {
        m = new QGCMAVLinkMessage(this, &message);
        v->append(m);
    } else {
        m->update(&message);
    }
}

//-----------------------------------------------------------------------------
MAVLinkChartController*
MAVLinkInspectorController::createChart()
{
    MAVLinkChartController* pChart = new MAVLinkChartController(this, _charts.count());
    QQmlEngine::setObjectOwnership(pChart, QQmlEngine::CppOwnership);
    _charts.append(pChart);
    emit chartsChanged();
    return pChart;
}

//-----------------------------------------------------------------------------
void
MAVLinkInspectorController::deleteChart(MAVLinkChartController* chart)
{
    if(chart) {
        for(int i = 0; i < _charts.count(); i++) {
            MAVLinkChartController* c = qobject_cast<MAVLinkChartController*>(_charts.get(i));
            if(c && c == chart) {
                _charts.removeOne(c);
                delete c;
                break;
            }
        }
        emit chartsChanged();
    }
}

//-----------------------------------------------------------------------------
MAVLinkInspectorController::TimeScale_st::TimeScale_st(QObject* parent, const QString& l, uint32_t t)
    : QObject(parent)
    , label(l)
    , timeScale(t)
{
}

//-----------------------------------------------------------------------------
MAVLinkInspectorController::Range_st::Range_st(QObject* parent, const QString& l, qreal r)
    : QObject(parent)
    , label(l)
    , range(r)
{
}

void MAVLinkInspectorController::setActiveSystem(int systemId)
{
    QGCMAVLinkSystem* v = _findVehicle(systemId);
    if (v != _activeSystem) {
        _activeSystem = v;
        emit activeSystemChanged();
    }
}

void MAVLinkInspectorController::setMessageInterval(int32_t rate)
{
    if(!_activeSystem) return;

    MultiVehicleManager* multiVehicleManager = qgcApp()->toolbox()->multiVehicleManager();
    if(!multiVehicleManager) return;

    const uint8_t sysId = _selectedSystemID();
    if(sysId == 0) return;

    Vehicle* vehicle = multiVehicleManager->getVehicleById(sysId);
    if(!vehicle) return;

    QGCMAVLinkMessage* msg = _activeSystem->selectedMsg();
    if(!msg) return;

    const uint8_t compId = _selectedComponentID();
    if(compId == 0) return;

    // TODO: Make QGCMAVLinkMessage a part of comm and use signals/slots for msg rate changes
    vehicle->setMessageRate(compId, msg->id(), rate);
}

uint8_t MAVLinkInspectorController::_selectedSystemID() const
{
    return _activeSystem ? _activeSystem->id() : 0;
}

uint8_t MAVLinkInspectorController::_selectedComponentID() const
{
    QGCMAVLinkMessage* msg = _activeSystem->selectedMsg();
    return (msg ? msg->compId() : 0);
}
