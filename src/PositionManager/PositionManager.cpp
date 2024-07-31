/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "PositionManager.h"
#include "QGCApplication.h"
#include "QGCCorePlugin.h"
#include "SimulatedPosition.h"
#include "QGCDeviceInfo.h"
#include <QGCLoggingCategory.h>

#include <QtCore/QPermissions>
#include <QtPositioning/QGeoPositionInfoSource>
#include <QtPositioning/private/qgeopositioninfosource_p.h>
#include <QtPositioning/QNmeaPositionInfoSource>
#include <QtQml/QtQml>

QGC_LOGGING_CATEGORY(QGCPositionManagerLog, "qgc.positionmanager.positionmanager")

QGCPositionManager::QGCPositionManager(QGCApplication *app, QGCToolbox *toolbox)
    : QGCTool(app, toolbox)
{
    // qCDebug(QGCPositionManagerLog) << Q_FUNC_INFO << this;
}

QGCPositionManager::~QGCPositionManager()
{
    // qCDebug(QGCPositionManagerLog) << Q_FUNC_INFO << this;
}

void QGCPositionManager::_setupPositionSources()
{
    m_defaultSource = _toolbox->corePlugin()->createPositionSource(this);
    if (m_defaultSource) {
        m_usingPluginSource = true;
    } else {
        qCDebug(QGCPositionManagerLog) << Q_FUNC_INFO << QGeoPositionInfoSource::availableSources();

        m_defaultSource = QGeoPositionInfoSource::createDefaultSource(this);
        if (!m_defaultSource) {
            qCWarning(QGCPositionManagerLog) << Q_FUNC_INFO << "No default source available";
            return;
        }
    }

    _setPositionSource(QGCPositionSource::InternalGPS);
}

void QGCPositionManager::_handlePermissionStatus(Qt::PermissionStatus permissionStatus)
{
    if (permissionStatus == Qt::PermissionStatus::Granted) {
        _setupPositionSources();
    } else {
        qCWarning(QGCPositionManagerLog) << Q_FUNC_INFO << "Location Permission Denied";
    }
}

void QGCPositionManager::_checkPermission()
{
    QLocationPermission locationPermission;
    locationPermission.setAccuracy(QLocationPermission::Precise);

    const Qt::PermissionStatus permissionStatus = _app->checkPermission(locationPermission);
    if (permissionStatus == Qt::PermissionStatus::Undetermined) {
        _app->requestPermission(locationPermission, this, [this](const QPermission &permission) {
            _handlePermissionStatus(permission.status());
        });
    } else {
        _handlePermissionStatus(permissionStatus);
    }
}

void QGCPositionManager::setToolbox(QGCToolbox *toolbox)
{
    QGCTool::setToolbox(toolbox);

    (void) qmlRegisterUncreatableType<QGCPositionManager>("QGroundControl.QGCPositionManager", 1, 0, "QGCPositionManager", "Reference only");

    if (_app->runningUnitTests()) {
        m_simulatedSource = new SimulatedPosition(this);
        _setPositionSource(QGCPositionSource::Simulated);
    } else {
        _checkPermission();
    }
}

void QGCPositionManager::setNmeaSourceDevice(QIODevice *device)
{
    if (m_nmeaSource) {
        m_nmeaSource->stopUpdates();
        (void) disconnect(m_nmeaSource);

        if (m_currentSource == m_nmeaSource) {
            m_currentSource = nullptr;
        }

        delete m_nmeaSource;
        m_nmeaSource = nullptr;
    }

    m_nmeaSource = new QNmeaPositionInfoSource(QNmeaPositionInfoSource::RealTimeMode, this);
    m_nmeaSource->setDevice(device);
    m_nmeaSource->setUserEquivalentRangeError(5.1);
    _setPositionSource(QGCPositionManager::NmeaGPS);
}

void QGCPositionManager::_positionUpdated(const QGeoPositionInfo &update)
{
    m_geoPositionInfo = update;

    QGeoCoordinate newGCSPosition(m_gcsPosition);

    if (update.hasAttribute(QGeoPositionInfo::HorizontalAccuracy)) {
        if ((qAbs(update.coordinate().latitude()) > 0.001) && (qAbs(update.coordinate().longitude()) > 0.001)) {
            m_gcsPositionHorizontalAccuracy = update.attribute(QGeoPositionInfo::HorizontalAccuracy);
            if (m_gcsPositionHorizontalAccuracy <= s_minHorizonalAccuracyMeters) {
                newGCSPosition.setLatitude(update.coordinate().latitude());
                newGCSPosition.setLongitude(update.coordinate().longitude());
            }
            emit gcsPositionHorizontalAccuracyChanged(m_gcsPositionHorizontalAccuracy);
        }
    }

    if (update.hasAttribute(QGeoPositionInfo::VerticalAccuracy)) {
        m_gcsPositionVerticalAccuracy = update.attribute(QGeoPositionInfo::VerticalAccuracy);
        if (m_gcsPositionVerticalAccuracy <= s_minVerticalAccuracyMeters) {
            newGCSPosition.setAltitude(update.coordinate().altitude());
        }
    }

    m_gcsPositionAccuracy = sqrt(pow(m_gcsPositionHorizontalAccuracy, 2) + pow(m_gcsPositionVerticalAccuracy, 2));

    _setGCSPosition(newGCSPosition);

    if (update.hasAttribute(QGeoPositionInfo::DirectionAccuracy)) {
        m_gcsDirectionAccuracy = update.attribute(QGeoPositionInfo::DirectionAccuracy);
        if (m_gcsDirectionAccuracy <= s_minDirectionAccuracyDegrees) {
            _setGCSHeading(update.attribute(QGeoPositionInfo::Direction));
        }
    } else if (m_usingPluginSource) {
        _setGCSHeading(update.attribute(QGeoPositionInfo::Direction));
    }

    emit positionInfoUpdated(update);
}

void QGCPositionManager::_setGCSHeading(qreal newGCSHeading)
{
    if (newGCSHeading != m_gcsHeading) {
        m_gcsHeading = newGCSHeading;
        emit gcsHeadingChanged(m_gcsHeading);
    }
}

void QGCPositionManager::_setGCSPosition(const QGeoCoordinate& newGCSPosition)
{
    if (newGCSPosition != m_gcsPosition) {
        m_gcsPosition = newGCSPosition;
        emit gcsPositionChanged(m_gcsPosition);
    }
}

void QGCPositionManager::_setPositionSource(QGCPositionSource source)
{
    if (m_currentSource != nullptr) {
        m_currentSource->stopUpdates();
        (void) disconnect(m_currentSource);

        m_geoPositionInfo = QGeoPositionInfo();
        m_gcsPosition = QGeoCoordinate();
        m_gcsHeading = qQNaN();
        m_gcsPositionHorizontalAccuracy = std::numeric_limits<qreal>::infinity();

        emit gcsPositionChanged(m_gcsPosition);
        emit gcsHeadingChanged(m_gcsHeading);
        emit positionInfoUpdated(m_geoPositionInfo);
        emit gcsPositionHorizontalAccuracyChanged(m_gcsPositionHorizontalAccuracy);
    }

    switch (source) {
    case QGCPositionManager::Log:
        break;
    case QGCPositionManager::Simulated:
        m_currentSource = m_simulatedSource;
        break;
    case QGCPositionManager::NmeaGPS:
        m_currentSource = m_nmeaSource;
        break;
    case QGCPositionManager::InternalGPS:
        m_currentSource = m_defaultSource;
        break;
    case QGCPositionManager::ExternalGPS:
        break;
    default:
        m_currentSource = m_defaultSource;
        break;
    }

    if (m_currentSource != nullptr) {
        m_currentSource->setPreferredPositioningMethods(QGeoPositionInfoSource::SatellitePositioningMethods);
        m_updateInterval = m_currentSource->minimumUpdateInterval();
        #if !defined(Q_OS_DARWIN) && !defined(Q_OS_IOS)
            m_currentSource->setUpdateInterval(m_updateInterval);
        #endif
        (void) connect(m_currentSource, &QGeoPositionInfoSource::positionUpdated, this, &QGCPositionManager::_positionUpdated);
        (void) connect(m_currentSource, &QGeoPositionInfoSource::errorOccurred, this, [](QGeoPositionInfoSource::Error positioningError) {
            qCWarning(QGCPositionManagerLog) << Q_FUNC_INFO << positioningError;
        });

        // (void) connect(QGCCompass::instance(), &QGCCompass::positionUpdated, this, &QGCPositionManager::_positionUpdated);

        m_currentSource->startUpdates();
    }
}
