#pragma once

#include "QGCStateMachine.h"

#include <QMap>
#include <QPair>
#include <QString>
#include <QVariant>

class ParameterManager;

/// State machine that handles PX4 parameter cache validation using the _HASH_CHECK handshake.
class ParamHashCheckStateMachine : public QGCStateMachine
{
    Q_OBJECT

public:
    ParamHashCheckStateMachine(Vehicle* vehicle, uint8_t componentId, const QVariant& hashValue);

signals:
    void cacheLoadFailed();
    void cacheHashMismatch();

private:
    using ParamTypeVal = QPair<int, QVariant>;
    using CacheMapName2ParamTypeVal = QMap<QString, ParamTypeVal>;

    void _setupStateGraph();
    void _loadCache();
    void _computeCacheHash();
    void _injectCachedParameters();
    void _sendHashAckToVehicle();
    void _startLoadProgressAnimation();
    void _handleCacheMismatchDebug();

    ParameterManager* _parameterManager = nullptr;
    int _vehicleId = 0;
    int _componentId = 0;
    QVariant _hashValue;
    CacheMapName2ParamTypeVal _cacheMap;
    bool _loadSucceeded = false;
    bool _hashMatches = false;
    uint32_t _computedHash = 0;
    QString _cacheFilePath;
};
