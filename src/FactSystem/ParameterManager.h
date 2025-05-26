/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QDir>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QTimer>

#include "Fact.h"
#include "FactMetaData.h"
#include "MAVLinkLib.h"

Q_DECLARE_LOGGING_CATEGORY(ParameterManagerLog)
Q_DECLARE_LOGGING_CATEGORY(ParameterManagerVerbose1Log)
Q_DECLARE_LOGGING_CATEGORY(ParameterManagerVerbose2Log)
Q_DECLARE_LOGGING_CATEGORY(ParameterManagerDebugCacheFailureLog)

class ParameterEditorController;
class Vehicle;

class ParameterManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool     parametersReady     READ parametersReady    NOTIFY parametersReadyChanged)      ///< true: Parameters are ready for use
    Q_PROPERTY(bool     missingParameters   READ missingParameters  NOTIFY missingParametersChanged)    ///< true: Parameters are missing from firmware response, false: all parameters received from firmware
    Q_PROPERTY(double   loadProgress        READ loadProgress       NOTIFY loadProgressChanged)
    Q_PROPERTY(bool     pendingWrites       READ pendingWrites      NOTIFY pendingWritesChanged)        ///< true: There are still pending write updates against the vehicle
    friend class ParameterEditorController;

public:
    ParameterManager(Vehicle *vehicle);
    ~ParameterManager();

    static void registerQmlTypes();

    bool parametersReady() const { return _parametersReady; }
    bool missingParameters() const { return _missingParameters; }
    double loadProgress() const { return _loadProgress; }

    /// @return Directory of parameter caches
    static QDir parameterCacheDir();

    /// @return Location of parameter cache file
    static QString parameterCacheFile(int vehicleId, int componentId);

    void mavlinkMessageReceived(const mavlink_message_t &message);

    QList<int> componentIds() const;

    /// Re-request the full set of parameters from the autopilot
    void refreshAllParameters(uint8_t componentID = MAV_COMP_ID_ALL);

    /// Request a refresh on the specific parameter
    void refreshParameter(int componentId, const QString &paramName);

    /// Request a refresh on all parameters that begin with the specified prefix
    void refreshParametersPrefix(int componentId, const QString &namePrefix);

    void resetAllParametersToDefaults();
    void resetAllToVehicleConfiguration();

    /// Returns true if the specifed parameter exists
    ///     @param componentId: Component id or ParameterManager::defaultComponentId
    ///     @param name: Parameter name
    bool parameterExists(int componentId, const QString &paramName) const;

    /// Returns all parameter names
    QStringList parameterNames(int componentId) const;

    /// Returns the specified Parameter. Returns a default empty fact is parameter does not exists. Also will pop
    /// a missing parameter error to user if parameter does not exist.
    ///     @param componentId: Component id or ParameterManager::defaultComponentId
    ///     @param name: Parameter name
    Fact *getParameter(int componentId, const QString &paramName);

    /// Returns error messages from loading
    QString readParametersFromStream(QTextStream &stream);

    void writeParametersToStream(QTextStream &stream) const;

    bool pendingWrites() const;

    Vehicle *vehicle();

    static MAV_PARAM_TYPE factTypeToMavType(FactMetaData::ValueType_t factType);
    static FactMetaData::ValueType_t mavTypeToFactType(MAV_PARAM_TYPE mavType);

    static constexpr int defaultComponentId = -1;

signals:
    void parametersReadyChanged(bool parametersReady);
    void missingParametersChanged(bool missingParameters);
    void loadProgressChanged(float value);
    void pendingWritesChanged(bool pendingWrites);
    void factAdded(int componentId, Fact *fact);

private slots:
    void _factRawValueUpdated(const QVariant &rawValue);

private:
    /// Called whenever a parameter is updated or first seen.
    void _handleParamValue(int componentId, const QString &parameterName, int parameterCount, int parameterIndex, MAV_PARAM_TYPE mavParamType, const QVariant &parameterValue);
     /// Writes the parameter update to mavlink, sets up for write wait
    void _factRawValueUpdateWorker(int componentId, const QString &name, FactMetaData::ValueType_t valueType, const QVariant &rawValue);
    void _waitingParamTimeout();
    void _tryCacheLookup();
    void _initialRequestTimeout();
    /// Translates ParameterManager::defaultComponentId to real component id if needed
    int _actualComponentId(int componentId) const;
    void _readParameterRaw(int componentId, const QString &paramName, int paramIndex) const;
    void _sendParamSetToVehicle(int componentId, const QString &paramName, FactMetaData::ValueType_t valueType, const QVariant &value) const;
    void _writeLocalParamCache(int vehicleId, int componentId);
    void _tryCacheHashLoad(int vehicleId, int componentId, const QVariant &hashValue);
    void _loadMetaData();
    void _clearMetaData();
    /// Remap a parameter from one firmware version to another
    QString _remapParamNameToVersion(const QString &paramName) const;
    /// The offline editing vehicle can have custom loaded params bolted into it.
    void _loadOfflineEditingParams();
    QString _logVehiclePrefix(int componentId) const;
    void _setLoadProgress(double loadProgress);
    /// Requests missing index based parameters from the vehicle.
    ///     @param waitingParamTimeout: true: being called due to timeout, false: being called to re-fill the batch queue
    /// return true: Parameters were requested, false: No more requests needed
    bool _fillIndexBatchQueue(bool waitingParamTimeout);
    void _updateProgressBar();
    void _checkInitialLoadComplete();
    void _ftpDownloadComplete(const QString &fileName, const QString &errorMsg);
    void _ftpDownloadProgress(float progress);
    /// Parse the binary parameter file and inject the parameters in the qgc fact system.
    /// See: https://github.com/ArduPilot/ardupilot/tree/master/libraries/AP_Filesystem
    bool _parseParamFile(const QString &filename);

    static QVariant _stringToTypedVariant(const QString &string, FactMetaData::ValueType_t type, bool failOk = false);

    Vehicle *_vehicle = nullptr;

    QMap<int /* comp id */, QMap<QString /* parameter name */, Fact*>> _mapCompId2FactMap;

    double _loadProgress = 0;                   ///< Parameter load progess, [0.0,1.0]
    bool _parametersReady = false;              ///< true: parameter load complete
    bool _missingParameters = false;            ///< true: parameter missing from initial load
    bool _initialLoadComplete = false;          ///< true: Initial load of all parameters complete, whether successful or not
    bool _waitingForDefaultComponent = false;   ///< true: last chance wait for default component params
    bool _saveRequired = false;                 ///< true: _saveToEEPROM should be called
    bool _metaDataAddedToFacts = false;         ///< true: FactMetaData has been adde to the default component facts
    bool _logReplay = false;                    ///< true: running with log replay link

    typedef QPair<int /* FactMetaData::ValueType_t */, QVariant /* Fact::rawValue */> ParamTypeVal;
    typedef QMap<QString /* parameter name */, ParamTypeVal> CacheMapName2ParamTypeVal;

    QMap<int /* component id */, bool> _debugCacheCRC; ///< true: debug cache crc failure
    QMap<int /* component id */, CacheMapName2ParamTypeVal> _debugCacheMap;
    QMap<int /* component id */, QMap<QString /* param name */, bool /* seen */>> _debugCacheParamSeen;

    // Wait counts from previous parameter update cycle
    int _prevWaitingReadParamIndexCount = 0;
    int _prevWaitingReadParamNameCount = 0;
    int _prevWaitingWriteParamNameCount = 0;

    bool _readParamIndexProgressActive = false;
    bool _readParamNameProgressActive = false;
    bool _writeParamProgressActive = false;

    static constexpr int _maxInitialRequestListRetry = 4;       ///< Maximum retries for request list
    int _initialRequestRetryCount = 0;                          ///< Current retry count for request list
    static constexpr int _maxInitialLoadRetrySingleParam = 5;   ///< Maximum retries for initial index based load of a single param
    static constexpr int _maxReadWriteRetry = 5;                ///< Maximum retries read/write
    bool _disableAllRetries = false;                            ///< true: Don't retry any requests (used for testing)

    bool _indexBatchQueueActive = false;    ///< true: we are actively batching re-requests for missing index base params, false: index based re-request has not yet started
    QList<int> _indexBatchQueue;            ///< The current queue of index re-requests

    QMap<int, int> _paramCountMap;                              ///< Key: Component id, Value: count of parameters in this component
    QMap<int, QMap<int, int>> _waitingReadParamIndexMap;        ///< Key: Component id, Value: Map { Key: parameter index still waiting for, Value: retry count }
    QMap<int, QMap<QString, int>> _waitingReadParamNameMap;     ///< Key: Component id, Value: Map { Key: parameter name still waiting for, Value: retry count }
    QMap<int, QMap<QString, int>> _waitingWriteParamNameMap;    ///< Key: Component id, Value: Map { Key: parameter name still waiting for, Value: retry count }
    QMap<int, QList<int>> _failedReadParamIndexMap;             ///< Key: Component id, Value: failed parameter index

    int _totalParamCount = 0;                   ///< Number of parameters across all components
    int _waitingWriteParamBatchCount = 0;       ///< Number of parameters which are batched up waiting on write responses
    int _waitingReadParamNameBatchCount = 0;    ///< Number of parameters which are batched up waiting on read responses

    QTimer _initialRequestTimeoutTimer;
    QTimer _waitingParamTimeoutTimer;

    Fact _defaultFact;   ///< Used to return default fact, when parameter not found

    bool _tryftp = false;
};
