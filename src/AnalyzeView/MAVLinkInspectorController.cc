/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MAVLinkInspectorController.h"
#include "MAVLinkChartController.h"
#include "MAVLinkMessage.h"
#include "MAVLinkProtocol.h"
#include "MAVLinkSystem.h"
#include "MultiVehicleManager.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"
#include "Vehicle.h"

#include <QtQml/QQmlEngine>

QGC_LOGGING_CATEGORY(MAVLinkInspectorControllerLog, "qgc.analyzeview.mavlinkinspectorcontroller")

MAVLinkInspectorController::TimeScale_st::TimeScale_st(const QString &label_, uint32_t timeScale_)
    : label(label_)
    , timeScale(timeScale_)
{

}

/*===========================================================================*/

MAVLinkInspectorController::Range_st::Range_st(const QString &label_, qreal range_)
    : label(label_)
    , range(range_)
{

}

/*===========================================================================*/

MAVLinkInspectorController::MAVLinkInspectorController(QObject *parent)
    : QObject(parent)
    , _updateFrequencyTimer(new QTimer(this))
    , _systems(new QmlObjectListModel(this))
    , _charts(new QmlObjectListModel(this))
{
    // qCDebug(MAVLinkInspectorControllerLog) << Q_FUNC_INFO << this;

    MultiVehicleManager *const multiVehicleManager = MultiVehicleManager::instance();
    (void) connect(multiVehicleManager, &MultiVehicleManager::vehicleAdded,   this, &MAVLinkInspectorController::_vehicleAdded);
    (void) connect(multiVehicleManager, &MultiVehicleManager::vehicleRemoved, this, &MAVLinkInspectorController::_vehicleRemoved);
    (void) connect(multiVehicleManager, &MultiVehicleManager::activeVehicleChanged, this, &MAVLinkInspectorController::_setActiveVehicle);

    MAVLinkProtocol *const mavlinkProtocol = MAVLinkProtocol::instance();
    (void) connect(mavlinkProtocol, &MAVLinkProtocol::messageReceived, this, &MAVLinkInspectorController::_receiveMessage);
    (void) connect(_updateFrequencyTimer, &QTimer::timeout, this, &MAVLinkInspectorController::_refreshFrequency);

    _updateFrequencyTimer->setInterval(1000);
    _updateFrequencyTimer->setSingleShot(false);
    _updateFrequencyTimer->start();

    _timeScaleSt.append(new TimeScale_st(tr("5 Sec"),   5 * 1000));
    _timeScaleSt.append(new TimeScale_st(tr("10 Sec"), 10 * 1000));
    _timeScaleSt.append(new TimeScale_st(tr("30 Sec"), 30 * 1000));
    _timeScaleSt.append(new TimeScale_st(tr("60 Sec"), 60 * 1000));
    emit timeScalesChanged();

    _rangeSt.append(new Range_st(tr("Auto"),    0));
    _rangeSt.append(new Range_st(tr("10,000"),  10000));
    _rangeSt.append(new Range_st(tr("1,000"),   1000));
    _rangeSt.append(new Range_st(tr("100"),     100));
    _rangeSt.append(new Range_st(tr("10"),      10));
    _rangeSt.append(new Range_st(tr("1"),       1));
    _rangeSt.append(new Range_st(tr("0.1"),     0.1));
    _rangeSt.append(new Range_st(tr("0.01"),    0.01));
    _rangeSt.append(new Range_st(tr("0.001"),   0.001));
    _rangeSt.append(new Range_st(tr("0.0001"),  0.0001));
    emit rangeListChanged();
}

MAVLinkInspectorController::~MAVLinkInspectorController()
{
    qDeleteAll(_timeScaleSt);
    qDeleteAll(_rangeSt);
    _charts->clearAndDeleteContents();
    _systems->clearAndDeleteContents();

    // qCDebug(MAVLinkInspectorControllerLog) << Q_FUNC_INFO << this;
}

QStringList MAVLinkInspectorController::timeScales()
{
    if (_timeScales.isEmpty()) {
        for (const TimeScale_st *timeScale : _timeScaleSt) {
            _timeScales << timeScale->label;
        }
    }

    return _timeScales;
}

QStringList MAVLinkInspectorController::rangeList()
{
    if (_rangeList.isEmpty()) {
        for (const Range_st *range : _rangeSt) {
            _rangeList << range->label;
        }
    }

    return _rangeList;
}

void MAVLinkInspectorController::_setActiveVehicle(Vehicle *vehicle)
{
    if (vehicle) {
        QGCMAVLinkSystem *const system = _findVehicle(static_cast<uint8_t>(vehicle->id()));
        if (system) {
            _activeSystem = system;
        } else {
            _activeSystem = nullptr;
        }
    } else {
        _activeSystem = nullptr;
    }

    emit activeSystemChanged();
}

QGCMAVLinkSystem *MAVLinkInspectorController::_findVehicle(uint8_t id)
{
    for (int i = 0; i < _systems->count(); i++) {
        QGCMAVLinkSystem *const system = qobject_cast<QGCMAVLinkSystem*>(_systems->get(i));
        if (system && (system->id() == id)) {
            return system;
        }
    }

    return nullptr;
}

void MAVLinkInspectorController::_refreshFrequency()
{
    for (int i = 0; i < _systems->count(); i++) {
        QGCMAVLinkSystem *const system = qobject_cast<QGCMAVLinkSystem*>(_systems->get(i));
        if (!system) {
            continue;
        }

        for (int i = 0; i < system->messages()->count(); i++) {
            QGCMAVLinkMessage *const msg = qobject_cast<QGCMAVLinkMessage*>(system->messages()->get(i));
            if (msg) {
                msg->updateFreq();
            }
        }
    }
}

void MAVLinkInspectorController::_vehicleAdded(Vehicle *vehicle)
{
    QGCMAVLinkSystem *sys = _findVehicle(static_cast<uint8_t>(vehicle->id()));

    if (sys) {
        sys->messages()->clearAndDeleteContents();
    } else {
        sys = new QGCMAVLinkSystem(static_cast<uint8_t>(vehicle->id()), this);
        _systems->append(sys);
        _systemNames.append(tr("System %1").arg(vehicle->id()));

        (void) connect(vehicle, &Vehicle::mavlinkMsgIntervalsChanged, sys, [sys](uint8_t compid, uint16_t msgId, int32_t rate) {
            for (int i = 0; i < sys->messages()->count(); i++) {
                QGCMAVLinkMessage *const msg = qobject_cast<QGCMAVLinkMessage*>(sys->messages()->get(i));
                if ((msg->compId() == compid) && (msg->id() == msgId)) {
                    msg->setTargetRateHz(rate);
                    break;
                }
            }
        });
    }

    emit systemsChanged();
}

void MAVLinkInspectorController::_vehicleRemoved(const Vehicle *vehicle)
{
    QGCMAVLinkSystem *const system = _findVehicle(static_cast<uint8_t>(vehicle->id()));
    if (!system) {
        return;
    }

    system->deleteLater();
    (void) _systems->removeOne(system);

    const QString systemName = tr("System %1").arg(vehicle->id());
    (void) _systemNames.removeOne(systemName);

    emit systemsChanged();
}

void MAVLinkInspectorController::_receiveMessage(LinkInterface *link, const mavlink_message_t &message)
{
    Q_UNUSED(link);

    QGCMAVLinkMessage *msg = nullptr;
    QGCMAVLinkSystem *system = _findVehicle(message.sysid);

    if (!system) {
        system = new QGCMAVLinkSystem(message.sysid, this);
        _systems->append(system);
        _systemNames.append(tr("System %1").arg(message.sysid));
        emit systemsChanged();

        if (!_activeSystem) {
            _activeSystem = system;
            emit activeSystemChanged();
        }
    } else {
        msg = system->findMessage(message.msgid, message.compid);
    }

    if (!msg) {
        msg = new QGCMAVLinkMessage(message, this);
        system->append(msg);
    } else {
        msg->update(message);
    }
}

MAVLinkChartController *MAVLinkInspectorController::createChart()
{
    MAVLinkChartController *const pChart = new MAVLinkChartController(this, _charts->count());
    QQmlEngine::setObjectOwnership(pChart, QQmlEngine::CppOwnership);

    _charts->append(pChart);
    emit chartsChanged();

    return pChart;
}

void MAVLinkInspectorController::deleteChart(MAVLinkChartController *chart)
{
    if (!chart) {
        return;
    }

    bool found = false;
    for (int i = 0; i < _charts->count(); i++) {
        MAVLinkChartController *const controller = qobject_cast<MAVLinkChartController*>(_charts->get(i));
        if (controller && (controller == chart)) {
            found = true;
            _charts->removeOne(controller);
            delete controller;
            break;
        }
    }

    if (found) {
        emit chartsChanged();
    }
}

void MAVLinkInspectorController::setActiveSystem(int systemId)
{
    QGCMAVLinkSystem *const system = _findVehicle(systemId);
    if (system != _activeSystem) {
        _activeSystem = system;
        emit activeSystemChanged();
    }
}

void MAVLinkInspectorController::setMessageInterval(int32_t rate) const
{
    if (!_activeSystem) {
        return;
    }

    MultiVehicleManager *const multiVehicleManager = MultiVehicleManager::instance();
    if (!multiVehicleManager) {
        return;
    }

    const uint8_t sysId = _selectedSystemID();
    if (sysId == 0) {
        return;
    }

    Vehicle *const vehicle = multiVehicleManager->getVehicleById(sysId);
    if (!vehicle) {
        return;
    }

    const QGCMAVLinkMessage *const msg = _activeSystem->selectedMsg();
    if (!msg) {
        return;
    }

    const uint8_t compId = _selectedComponentID();
    if (compId == 0) {
        return;
    }

    // TODO: Make QGCMAVLinkMessage a part of comm and use signals/slots for msg rate changes
    vehicle->setMessageRate(compId, msg->id(), rate);
}

uint8_t MAVLinkInspectorController::_selectedSystemID() const
{
    return (_activeSystem ? _activeSystem->id() : 0);
}

uint8_t MAVLinkInspectorController::_selectedComponentID() const
{
    const QGCMAVLinkMessage *const msg = _activeSystem ? _activeSystem->selectedMsg() : nullptr;
    return (msg ? msg->compId() : 0);
}
