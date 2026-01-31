#pragma once

#include <QtCore/QDir>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtQmlIntegration/QtQmlIntegration>

#include "Fact.h"
#include "FactMetaData.h"
#include "MAVLinkLib.h"

Q_DECLARE_LOGGING_CATEGORY(ParameterManagerLog)
Q_DECLARE_LOGGING_CATEGORY(ParameterManagerVerbose1Log)
Q_DECLARE_LOGGING_CATEGORY(ParameterManagerVerbose2Log)
Q_DECLARE_LOGGING_CATEGORY(ParameterManagerDebugCacheFailureLog)

class ParameterEditorController;
class ParameterLoadStateMachine;
class Vehicle;

class ParameterManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_PROPERTY(bool     parametersReady     READ parametersReady    NOTIFY parametersReadyChanged)      ///< true: Parameters are ready for use
    Q_PROPERTY(bool     missingParameters   READ missingParameters  NOTIFY missingParametersChanged)    ///< true: Parameters are missing from firmware response, false: all parameters received from firmware
    Q_PROPERTY(double   loadProgress        READ loadProgress       NOTIFY loadProgressChanged)
    Q_PROPERTY(bool     pendingWrites       READ pendingWrites      NOTIFY pendingWritesChanged)        ///< true: There are still pending write updates against the vehicle
    friend class ParameterEditorController;
    friend class ParameterLoadStateMachine;

public:
    ParameterManager(Vehicle *vehicle);
    ~ParameterManager();

    bool parametersReady() const;
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

    // These are public for creating unit tests
    static constexpr int kParamSetRetryCount = 2;                   ///< Number of retries for PARAM_SET
    static constexpr int kParamRequestReadRetryCount = 2;           ///< Number of retries for PARAM_REQUEST_READ
    static constexpr int kWaitForParamValueAckMs = 1000;            ///< Time to wait for param value ack after set param
    static constexpr int kInitialRequestTimeoutMsDefault = 5000;    ///< Default timeout for initial param request
    static constexpr int kWaitingParamTimeoutMsDefault = 3000;      ///< Default timeout for waiting param

    /// Set shorter timeouts for unit testing (call before vehicle connection)
    void setTestTimeouts(int initialRequestTimeoutMs, int waitingParamTimeoutMs);

signals:
    void parametersReadyChanged(bool parametersReady);
    void missingParametersChanged(bool missingParameters);
    void loadProgressChanged(float value);
    void pendingWritesChanged(bool pendingWrites);
    void factAdded(int componentId, Fact *fact);

    // These signals are used to verify unit tests
    void _paramSetSuccess(int componentId, const QString &paramName);
    void _paramSetFailure(int componentId, const QString &paramName);
    void _paramRequestReadSuccess(int componentId, const QString &paramName, int paramIndex);
    void _paramRequestReadFailure(int componentId, const QString &paramName, int paramIndex);

private slots:
    void _factRawValueUpdated(const QVariant &rawValue);
    void _onLoadComplete(bool success, bool missingParameters);
    void _onLoadProgressChanged(double progress);

private:
    /// Called whenever a parameter is updated or first seen.
    void _handleParamValue(int componentId, const QString &parameterName, int parameterCount, int parameterIndex, MAV_PARAM_TYPE mavParamType, const QVariant &parameterValue);
    /// Writes the parameter update to mavlink, sets up for write wait
    void _mavlinkParamSet(int componentId, const QString &name, FactMetaData::ValueType_t valueType, const QVariant &rawValue);
    void _tryCacheLookup();
    /// Translates ParameterManager::defaultComponentId to real component id if needed
    int _actualComponentId(int componentId) const;
    void _mavlinkParamRequestRead(int componentId, const QString &paramName, int paramIndex, bool notifyFailure);
    void _writeLocalParamCache(int vehicleId, int componentId);
    void _tryCacheHashLoad(int vehicleId, int componentId, const QVariant &hashValue);
    void _loadMetaData();
    void _clearMetaData();
    /// Remap a parameter from one firmware version to another
    QString _remapParamNameToVersion(const QString &paramName) const;
    bool _fillMavlinkParamUnion(FactMetaData::ValueType_t valueType, const QVariant &rawValue, mavlink_param_union_t &paramUnion) const;
    bool _mavlinkParamUnionToVariant(const mavlink_param_union_t &paramUnion, QVariant &outValue) const;
    /// The offline editing vehicle can have custom loaded params bolted into it.
    void _loadOfflineEditingParams();
    QString _logVehiclePrefix(int componentId) const;
    void _setLoadProgress(double loadProgress);
    /// Parse the binary parameter file and inject the parameters in the qgc fact system.
    /// See: https://github.com/ArduPilot/ardupilot/tree/master/libraries/AP_Filesystem
    bool _parseParamFile(const QString &filename);
    void _incrementPendingWriteCount();
    void _decrementPendingWriteCount();
    QString _vehicleAndComponentString(int componentId) const;

    static QVariant _stringToTypedVariant(const QString &string, FactMetaData::ValueType_t type, bool failOk = false);

    Vehicle *_vehicle = nullptr;
    ParameterLoadStateMachine* _loadStateMachine = nullptr;

    QMap<int /* comp id */, QMap<QString /* parameter name */, Fact*>> _mapCompId2FactMap;

    // QML property state
    double _loadProgress = 0;                   ///< Parameter load progess, [0.0,1.0]
    bool _missingParameters = false;            ///< true: parameter missing from initial load

    // Internal state (not managed by state machine)
    bool _offlineParametersReady = false;       ///< true: offline editing vehicle params loaded (no state machine)
    bool _logReplay = false;                    ///< true: running with log replay link

    typedef QPair<int /* FactMetaData::ValueType_t */, QVariant /* Fact::rawValue */> ParamTypeVal;
    typedef QMap<QString /* parameter name */, ParamTypeVal> CacheMapName2ParamTypeVal;

    // Cache debugging
    QMap<int /* component id */, bool> _debugCacheCRC; ///< true: debug cache crc failure
    QMap<int /* component id */, CacheMapName2ParamTypeVal> _debugCacheMap;
    QMap<int /* component id */, QMap<QString /* param name */, bool /* seen */>> _debugCacheParamSeen;

    // Parameter tracking data structures (shared with state machine)
    QMap<int, int> _paramCountMap;                              ///< Key: Component id, Value: count of parameters in this component
    QMap<int, QMap<int, int>> _waitingReadParamIndexMap;        ///< Key: Component id, Value: Map { Key: parameter index still waiting for, Value: retry count }
    QMap<int, QList<int>> _failedReadParamIndexMap;             ///< Key: Component id, Value: failed parameter index
    int _totalParamCount = 0;                                   ///< Number of parameters across all components
    int _pendingWritesCount = 0;                                ///< Number of parameters with pending writes

    Fact _defaultFact;   ///< Used to return default fact, when parameter not found
};
