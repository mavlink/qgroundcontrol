/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCStateMachine.h"

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(ParamHashCheckStateMachineLog)

/// State machine that handles PX4 parameter cache validation using the _HASH_CHECK handshake.
class ParamHashCheckStateMachine : public QGCStateMachine
{
    Q_OBJECT

public:
    ParamHashCheckStateMachine(Vehicle* vehicle, QState *parentState);

signals:
    void cacheLoadFailed();
    void cacheHashMismatch();

private:
    void _setupStateGraph();
    void _loadCache();
    void _computeCacheHash();
    void _injectCachedParameters();
    void _sendHashAckToVehicle();
    void _startLoadProgressAnimation();
    void _handleCacheMismatchDebug();

    using ParamTypeVal = QPair<int, QVariant>;
    using CacheMapName2ParamTypeVal = QMap<QString, ParamTypeVal>;

    bool _loadSucceeded = false;
    bool _hashMatches = false;
    uint32_t _computedHash = 0;
    QString _cacheFilePath;
    int _componentId = MAV_COMP_ID_AUTOPILOT1;

    uint32_t _hashFromVehicle = 0;

    CacheMapName2ParamTypeVal _cacheMap;
    QMap<int, bool> _debugCacheCRC;                         ///< Key: component id, Value: true - debug cache crc failure
    QMap<int, CacheMapName2ParamTypeVal> _debugCacheMap;    ///< Key: component id
    QMap<int, QMap<QString, bool>> _debugCacheParamSeen;    ///< Key : component id, <Key: param name, Value: true - param seen>
};
