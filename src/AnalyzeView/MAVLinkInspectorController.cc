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
#include "QGCGeo.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"
#include "Vehicle.h"

#include <QtQml/QQmlEngine>
#include <QtPositioning/QGeoCoordinate>
#include <QtCore/QVariantMap>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

QGC_LOGGING_CATEGORY(MAVLinkInspectorControllerLog, "AnalyzeView.MAVLinkInspectorController")

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
    QGCMAVLinkSystem *previousSystem = _activeSystem;
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

    if (_activeSystem != previousSystem) {
        emit activeSystemChanged();
        _clearXYData();
    }
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
    _clearXYData();
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

    _processXYData(message);
}

void MAVLinkInspectorController::setActiveSystem(int systemId)
{
    QGCMAVLinkSystem *const system = _findVehicle(systemId);
    if (system != _activeSystem) {
        _activeSystem = system;
        emit activeSystemChanged();
        _clearXYData();
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

void MAVLinkInspectorController::setGpsXEnabled(bool enabled)
{
    if (_gpsXEnabled == enabled) {
        return;
    }

    const bool wasActive = gpsXYEnabled();
    _gpsXEnabled = enabled;
    emit gpsXEnabledChanged();
    _postGpsEnablementChange(wasActive);
}

void MAVLinkInspectorController::setGpsYEnabled(bool enabled)
{
    if (_gpsYEnabled == enabled) {
        return;
    }

    const bool wasActive = gpsXYEnabled();
    _gpsYEnabled = enabled;
    emit gpsYEnabledChanged();
    _postGpsEnablementChange(wasActive);
}

void MAVLinkInspectorController::setOdomXEnabled(bool enabled)
{
    if (_odomXEnabled == enabled) {
        return;
    }

    const bool wasActive = odomXYEnabled();
    _odomXEnabled = enabled;
    emit odomXEnabledChanged();
    _postOdomEnablementChange(wasActive);
}

void MAVLinkInspectorController::setOdomYEnabled(bool enabled)
{
    if (_odomYEnabled == enabled) {
        return;
    }

    const bool wasActive = odomXYEnabled();
    _odomYEnabled = enabled;
    emit odomYEnabledChanged();
    _postOdomEnablementChange(wasActive);
}

void MAVLinkInspectorController::_processXYData(const mavlink_message_t &message)
{
    if (!_activeSystem || (_activeSystem->id() != message.sysid)) {
        return;
    }

    switch (message.msgid) {
    case MAVLINK_MSG_ID_GPS_RAW_INT:
        _handleGpsRawInt(message);
        break;
    case MAVLINK_MSG_ID_ODOMETRY:
        _handleOdometry(message);
        break;
    default:
        break;
    }
}

void MAVLinkInspectorController::_handleGpsRawInt(const mavlink_message_t &message)
{
    if (!gpsXYEnabled()) {
        return;
    }

    MultiVehicleManager *const multiVehicleManager = MultiVehicleManager::instance();
    if (!multiVehicleManager) {
        return;
    }

    Vehicle *const vehicle = multiVehicleManager->getVehicleById(message.sysid);
    if (!vehicle) {
        return;
    }

    const QGeoCoordinate home = vehicle->homePosition();
    if (!home.isValid()) {
        return;
    }

    mavlink_gps_raw_int_t gps{};
    mavlink_msg_gps_raw_int_decode(&message, &gps);

    if ((gps.lat == 0 && gps.lon == 0) || gps.lat == INT32_MAX || gps.lon == INT32_MAX) {
        return;
    }

    const double latitude = static_cast<double>(gps.lat) / 1e7;
    const double longitude = static_cast<double>(gps.lon) / 1e7;
    QGeoCoordinate coordinate(latitude, longitude);
    if (!coordinate.isValid()) {
        return;
    }

    double north = std::numeric_limits<double>::quiet_NaN();
    double east = std::numeric_limits<double>::quiet_NaN();
    double down = 0.0;
    QGCGeo::convertGeoToNed(coordinate, home, north, east, down);

    if (std::isnan(north) || std::isnan(east)) {
        return;
    }

    _appendGpsPoint(north, east);
}

void MAVLinkInspectorController::_handleOdometry(const mavlink_message_t &message)
{
    if (!odomXYEnabled()) {
        return;
    }

    mavlink_odometry_t odom{};
    mavlink_msg_odometry_decode(&message, &odom);

    if (std::isnan(odom.x) || std::isnan(odom.y)) {
        return;
    }

    _appendOdomPoint(static_cast<double>(odom.x), static_cast<double>(odom.y));
}

void MAVLinkInspectorController::_appendGpsPoint(double x, double y)
{
    const QPointF point(static_cast<qreal>(x), static_cast<qreal>(y));
    _gpsPoints.append(point);
    QVariantMap variantPoint;
    variantPoint.insert(QStringLiteral("x"), point.x());
    variantPoint.insert(QStringLiteral("y"), point.y());
    _gpsXYPoints.append(variantPoint);
    if (_gpsPoints.size() > _maxXYPointCount) {
        _gpsPoints.removeFirst();
        _gpsXYPoints.removeFirst();
    }
    
    qCDebug(MAVLinkInspectorControllerLog) << "GPS point added:" << point.x() << point.y() << "Total points:" << _gpsPoints.size();
    
    emit gpsXYPointsChanged();
    _updateXYBounds();
}

void MAVLinkInspectorController::_appendOdomPoint(double x, double y)
{
    const QPointF point(static_cast<qreal>(x), static_cast<qreal>(y));
    _odomPoints.append(point);
    QVariantMap variantPoint;
    variantPoint.insert(QStringLiteral("x"), point.x());
    variantPoint.insert(QStringLiteral("y"), point.y());
    _odomXYPoints.append(variantPoint);
    if (_odomPoints.size() > _maxXYPointCount) {
        _odomPoints.removeFirst();
        _odomXYPoints.removeFirst();
    }
    
    qCDebug(MAVLinkInspectorControllerLog) << "Odom point added:" << point.x() << point.y() << "Total points:" << _odomPoints.size();
    
    emit odomXYPointsChanged();
    _updateXYBounds();
}

void MAVLinkInspectorController::_postGpsEnablementChange(bool wasActive)
{
    const bool isActive = gpsXYEnabled();
    if (!isActive && !_gpsPoints.isEmpty()) {
        _gpsPoints.clear();
        _gpsXYPoints.clear();
        emit gpsXYPointsChanged();
        _updateXYBounds();
    }

    if (wasActive != isActive) {
        emit gpsXYEnabledChanged();
        _updateXYPlotVisible();
    }
}

void MAVLinkInspectorController::_postOdomEnablementChange(bool wasActive)
{
    const bool isActive = odomXYEnabled();
    if (!isActive && !_odomPoints.isEmpty()) {
        _odomPoints.clear();
        _odomXYPoints.clear();
        emit odomXYPointsChanged();
        _updateXYBounds();
    }

    if (wasActive != isActive) {
        emit odomXYEnabledChanged();
        _updateXYPlotVisible();
    }
}

void MAVLinkInspectorController::_updateXYPlotVisible()
{
    const bool visible = gpsXYEnabled() || odomXYEnabled();
    if (visible != _xyPlotVisible) {
        _xyPlotVisible = visible;
        emit xyPlotVisibleChanged();
    }
}

void MAVLinkInspectorController::_clearXYData()
{
    bool changed = false;
    if (!_gpsPoints.isEmpty()) {
        _gpsPoints.clear();
        _gpsXYPoints.clear();
        emit gpsXYPointsChanged();
        changed = true;
    }
    if (!_odomPoints.isEmpty()) {
        _odomPoints.clear();
        _odomXYPoints.clear();
        emit odomXYPointsChanged();
        changed = true;
    }
    if (changed) {
        _updateXYBounds();
    }
}

void MAVLinkInspectorController::_updateXYBounds()
{
    if (_gpsPoints.isEmpty() && _odomPoints.isEmpty()) {
        const qreal defaultMin = -1.0;
        const qreal defaultMax = 1.0;
        bool changed = false;
        if (!qFuzzyCompare(_xyMinX, defaultMin) || !qFuzzyCompare(_xyMaxX, defaultMax) ||
            !qFuzzyCompare(_xyMinY, defaultMin) || !qFuzzyCompare(_xyMaxY, defaultMax)) {
            _xyMinX = defaultMin;
            _xyMaxX = defaultMax;
            _xyMinY = defaultMin;
            _xyMaxY = defaultMax;
            changed = true;
        }
        if (changed) {
            emit xyRangeChanged();
        }
        return;
    }

    qreal minX = std::numeric_limits<qreal>::max();
    qreal maxX = std::numeric_limits<qreal>::lowest();
    qreal minY = std::numeric_limits<qreal>::max();
    qreal maxY = std::numeric_limits<qreal>::lowest();

    auto accumulate = [&minX, &maxX, &minY, &maxY](const QList<QPointF> &points) {
        for (const QPointF &point : points) {
            minX = std::min(minX, point.x());
            maxX = std::max(maxX, point.x());
            minY = std::min(minY, point.y());
            maxY = std::max(maxY, point.y());
        }
    };

    if (!_gpsPoints.isEmpty()) {
        accumulate(_gpsPoints);
    }
    if (!_odomPoints.isEmpty()) {
        accumulate(_odomPoints);
    }

    if (minX > maxX || minY > maxY) {
        return;
    }

    auto expandRange = [](qreal &minValue, qreal &maxValue) {
        const qreal span = maxValue - minValue;
        if (span < 1e-6) {
            minValue -= 1.0;
            maxValue += 1.0;
        } else {
            const qreal padding = span < 1.0 ? 0.5 : span * 0.1;
            minValue -= padding;
            maxValue += padding;
        }
    };

    expandRange(minX, maxX);
    expandRange(minY, maxY);

    bool changed = false;
    if (!qFuzzyCompare(_xyMinX, minX)) {
        _xyMinX = minX;
        changed = true;
    }
    if (!qFuzzyCompare(_xyMaxX, maxX)) {
        _xyMaxX = maxX;
        changed = true;
    }
    if (!qFuzzyCompare(_xyMinY, minY)) {
        _xyMinY = minY;
        changed = true;
    }
    if (!qFuzzyCompare(_xyMaxY, maxY)) {
        _xyMaxY = maxY;
        changed = true;
    }

    if (changed) {
        emit xyRangeChanged();
    }
}
