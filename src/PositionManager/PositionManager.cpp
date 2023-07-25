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

#if !defined(NO_SERIAL_LINK) && !defined(__android__)
#include <QSerialPortInfo>
#endif

#include <QtPositioning/private/qgeopositioninfosource_p.h>

QGCPositionManager::QGCPositionManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool           (app, toolbox)
{

}

QGCPositionManager::~QGCPositionManager()
{
    delete(_simulatedSource);
    delete(_nmeaSource);
}

void QGCPositionManager::setToolbox(QGCToolbox *toolbox)
{
   QGCTool::setToolbox(toolbox);
   //-- First see if plugin provides a position source
   _defaultSource = toolbox->corePlugin()->createPositionSource(this);
   if (_defaultSource) {
       _usingPluginSource = true;
   }

   if (qgcApp()->runningUnitTests()) {
       // Units test on travis fail due to lack of position source
       return;
   }

   if (!_defaultSource) {
       //-- Otherwise, create a default one
#if 0
       // Calling this can end up falling through a path which tries to instantiate a serialnmea source.
       // The Qt code for this will pop a qWarning if there are no serial ports available. This in turn
       // causes you to be unable to run with QT_FATAL_WARNINGS=1 to debug stuff.
       _defaultSource = QGeoPositionInfoSource::createDefaultSource(this);
#else
       // So instead we create our own version of QGeoPositionInfoSource::createDefaultSource which isn't as stupid.
       QList<QJsonObject> plugins = QGeoPositionInfoSourcePrivate::pluginsSorted();
       foreach (const QJsonObject &obj, plugins) {
           if (obj.value("Position").isBool() && obj.value("Position").toBool()) {
               QString pluginName = obj.value("Keys").toArray()[0].toString();
               if (pluginName == "serialnmea") {
#if !defined(NO_SERIAL_LINK) && !defined(__android__)
                   if (QSerialPortInfo::availablePorts().isEmpty()) {
                       // This prevents the qWarning from popping
                       continue;
                   }
#else
                   continue;
#endif
               }
               _defaultSource = QGeoPositionInfoSource::createSource(pluginName, this);
               if (_defaultSource) {
                   break;
               }
           }
       }
#endif
   }
   _simulatedSource = new SimulatedPosition();

#if 1
   setPositionSource(QGCPositionSource::InternalGPS);
#else
   setPositionSource(QGCPositionManager::Simulated);
#endif
}

void QGCPositionManager::setNmeaSourceDevice(QIODevice* device)
{
    // stop and release _nmeaSource
    if (_nmeaSource) {
        _nmeaSource->stopUpdates();
        disconnect(_nmeaSource);

        // if _currentSource is pointing there, point to null
        if (_currentSource == _nmeaSource){
            _currentSource = nullptr;
        }

        delete _nmeaSource;
        _nmeaSource = nullptr;

    }
    _nmeaSource = new QNmeaPositionInfoSource(QNmeaPositionInfoSource::RealTimeMode, this);
    _nmeaSource->setDevice(device);
    // set equivalent range error to enable position accuracy reporting
    _nmeaSource->setUserEquivalentRangeError(5.1);
    setPositionSource(QGCPositionManager::NmeaGPS);
}

void QGCPositionManager::_positionUpdated(const QGeoPositionInfo &update)
{
    _geoPositionInfo = update;

    QGeoCoordinate newGCSPosition = QGeoCoordinate();
    qreal newGCSHeading = update.attribute(QGeoPositionInfo::Direction);

    if (update.isValid() && update.hasAttribute(QGeoPositionInfo::HorizontalAccuracy)) {
        // Note that gcsPosition filters out possible crap values
        if (qAbs(update.coordinate().latitude()) > 0.001 &&
            qAbs(update.coordinate().longitude()) > 0.001 ) {
            _gcsPositionHorizontalAccuracy = update.attribute(QGeoPositionInfo::HorizontalAccuracy);
            if (_gcsPositionHorizontalAccuracy <= MinHorizonalAccuracyMeters) {
                newGCSPosition = update.coordinate();
            }
            emit gcsPositionHorizontalAccuracyChanged();
        }
    }
    if (newGCSPosition != _gcsPosition) {
        _gcsPosition = newGCSPosition;
        emit gcsPositionChanged(_gcsPosition);
    }

    // At this point only plugins support gcs heading. The reason is that the quality of heading information from a local
    // position device (not a compass) is unknown. In many cases it can only be trusted if the GCS location is moving above
    // a certain rate of speed. When it is not, or the gcs is standing still the heading is just random. We don't want these
    // random heading to be shown on the fly view. So until we can get a true compass based heading or some smarted heading quality
    // information which takes into account the speed of movement we normally don't set a heading. We do use the heading though
    // if the plugin overrides the position source. In that case we assume that it hopefully know what it is doing.
    if (_usingPluginSource && newGCSHeading != _gcsHeading) {
        _gcsHeading = newGCSHeading;
        emit gcsHeadingChanged(_gcsHeading);
    }

    emit positionInfoUpdated(update);
}

int QGCPositionManager::updateInterval() const
{
    return _updateInterval;
}

void QGCPositionManager::setPositionSource(QGCPositionManager::QGCPositionSource source)
{
    if (_currentSource != nullptr) {
        _currentSource->stopUpdates();
        disconnect(_currentSource);
    }

    if (qgcApp()->runningUnitTests()) {
        // Units test on travis fail due to lack of position source
        return;
    }

    switch(source) {
    case QGCPositionManager::Log:
        break;
    case QGCPositionManager::Simulated:
        _currentSource = _simulatedSource;
        break;
    case QGCPositionManager::NmeaGPS:
        _currentSource = _nmeaSource;
        break;
    case QGCPositionManager::InternalGPS:
    default:        
        _currentSource = _defaultSource;
        break;
    }

    if (_currentSource != nullptr) {
        _updateInterval = _currentSource->minimumUpdateInterval();
        _currentSource->setPreferredPositioningMethods(QGeoPositionInfoSource::SatellitePositioningMethods);
        _currentSource->setUpdateInterval(_updateInterval);
        connect(_currentSource, &QGeoPositionInfoSource::positionUpdated,       this, &QGCPositionManager::_positionUpdated);
        connect(_currentSource, SIGNAL(error(QGeoPositionInfoSource::Error)),   this, SLOT(_error(QGeoPositionInfoSource::Error)));
        _currentSource->startUpdates();
    }
}

void QGCPositionManager::_error(QGeoPositionInfoSource::Error positioningError)
{
    qWarning() << "QGCPositionManager error" << positioningError;
}
