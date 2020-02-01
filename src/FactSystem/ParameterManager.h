/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QMap>
#include <QXmlStreamReader>
#include <QLoggingCategory>
#include <QMutex>
#include <QDir>
#include <QJsonObject>

#include "FactSystem.h"
#include "MAVLinkProtocol.h"
#include "AutoPilotPlugin.h"
#include "QGCMAVLink.h"
#include "Vehicle.h"

Q_DECLARE_LOGGING_CATEGORY(ParameterManagerVerbose1Log)
Q_DECLARE_LOGGING_CATEGORY(ParameterManagerVerbose2Log)
Q_DECLARE_LOGGING_CATEGORY(ParameterManagerDebugCacheFailureLog)

class ParameterManager : public QObject
{
    Q_OBJECT

public:
    /// @param uas Uas which this set of facts is associated with
    ParameterManager    (Vehicle* vehicle);
    ~ParameterManager   ();

    Q_PROPERTY(bool     parametersReady     READ parametersReady    NOTIFY parametersReadyChanged)      ///< true: Parameters are ready for use
    Q_PROPERTY(bool     missingParameters   READ missingParameters  NOTIFY missingParametersChanged)    ///< true: Parameters are missing from firmware response, false: all parameters received from firmware
    Q_PROPERTY(double   loadProgress        READ loadProgress       NOTIFY loadProgressChanged)
    Q_PROPERTY(bool     pendingWrites       READ pendingWrites      NOTIFY pendingWritesChanged)        ///< true: There are still pending write updates against the vehicle

    bool parametersReady    (void) const { return _parametersReady; }
    bool missingParameters  (void) const { return _missingParameters; }
    double loadProgress     (void) const { return _loadProgress; }

    /// @return Directory of parameter caches
    static QDir parameterCacheDir();

    /// @return Location of parameter cache file
    static QString parameterCacheFile(int vehicleId, int componentId);

    QList<int> componentIds(void);

    /// Re-request the full set of parameters from the autopilot
    void refreshAllParameters(uint8_t componentID = MAV_COMP_ID_ALL);

    /// Request a refresh on the specific parameter
    void refreshParameter(int componentId, const QString& paramName);

    /// Request a refresh on all parameters that begin with the specified prefix
    void refreshParametersPrefix(int componentId, const QString& namePrefix);

    void resetAllParametersToDefaults();
    void resetAllToVehicleConfiguration();

    /// Returns true if the specifed parameter exists
    ///     @param componentId: Component id or FactSystem::defaultComponentId
    ///     @param name: Parameter name
    bool parameterExists(int componentId, const QString& paramName);

    /// Returns all parameter names
    QStringList parameterNames(int componentId);

    /// Returns the specified Parameter. Returns a default empty fact is parameter does not exists. Also will pop
    /// a missing parameter error to user if parameter does not exist.
    ///     @param componentId: Component id or FactSystem::defaultComponentId
    ///     @param name: Parameter name
    Fact* getParameter(int componentId, const QString& paramName);

    int  getComponentId(const QString& category);
    QString getComponentCategory(int componentId);
    const QMap<QString, QMap<QString, QStringList> >& getComponentCategoryMap(int componentId);

    /// Returns error messages from loading
    QString readParametersFromStream(QTextStream& stream);

    void writeParametersToStream(QTextStream& stream);

    /// Returns the version number for the parameter set, -1 if not known
    int parameterSetVersion(void) { return _parameterSetMajorVersion; }

    /// Returns the newest available parameter meta data file (from cache or internal) for the specified information.
    ///     @param wantedMajorVersion Major version you are looking for
    ///     @param[out] majorVersion Major version for found meta data
    ///     @param[out] minorVersion Minor version for found meta data
    /// @return Meta data file name of best match, emptyString is none found
    static QString parameterMetaDataFile(Vehicle* vehicle, MAV_AUTOPILOT firmwareType, int wantedMajorVersion, int& majorVersion, int& minorVersion);

    /// If this file is newer than anything in the cache, cache it as the latest version
    static void cacheMetaDataFile(const QString& metaDataFile, MAV_AUTOPILOT firmwareType);

    /// Saves the specified param set to the json object.
    ///     @param componentId Component id which contains params, MAV_COMP_ID_ALL to save all components
    ///     @param paramsToSave List of params names to save, empty to save all for component
    ///     @param saveObject Json object to save to
    void saveToJson(int componentId, const QStringList& paramsToSave, QJsonObject& saveObject);

    /// Load a parameter set from json
    ///     @param json Json object to load from
    ///     @param required true: no parameters in object will generate error
    ///     @param errorString Error string if return is false
    /// @return true: success, false: failure (errorString set)
    bool loadFromJson(const QJsonObject& json, bool required, QString& errorString);

    bool pendingWrites(void);

    Vehicle* vehicle(void) { return _vehicle; }

signals:
    void parametersReadyChanged     (bool parametersReady);
    void missingParametersChanged   (bool missingParameters);
    void loadProgressChanged        (float value);
    void pendingWritesChanged       (bool pendingWrites);

protected:
    Vehicle*            _vehicle;
    MAVLinkProtocol*    _mavlink;

    void _parameterUpdate(int vehicleId, int componentId, QString parameterName, int parameterCount, int parameterId, int mavType, QVariant value);
    void _valueUpdated(const QVariant& value);
    void _waitingParamTimeout(void);
    void _tryCacheLookup(void);
    void _initialRequestTimeout(void);

private:
    static QVariant         _stringToTypedVariant(const QString& string, FactMetaData::ValueType_t type, bool failOk = false);
    static FirmwarePlugin*  _anyVehicleTypeFirmwarePlugin(MAV_AUTOPILOT firmwareType);

    int     _actualComponentId(int componentId);
    void    _setupComponentCategoryMap(int componentId);
    void    _setupDefaultComponentCategoryMap(void);
    void    _readParameterRaw(int componentId, const QString& paramName, int paramIndex);
    void    _writeParameterRaw(int componentId, const QString& paramName, const QVariant& value);
    void    _writeLocalParamCache(int vehicleId, int componentId);
    void    _tryCacheHashLoad(int vehicleId, int componentId, QVariant hash_value);
    void    _loadMetaData(void);
    void    _clearMetaData(void);
    void    _addMetaDataToDefaultComponent(void);
    QString _remapParamNameToVersion(const QString& paramName);
    void    _loadOfflineEditingParams(void);
    QString _logVehiclePrefix(int componentId);
    void    _setLoadProgress(double loadProgress);
    bool    _fillIndexBatchQueue(bool waitingParamTimeout);
    void    _updateProgressBar(void);

    MAV_PARAM_TYPE _factTypeToMavType(FactMetaData::ValueType_t factType);
    FactMetaData::ValueType_t _mavTypeToFactType(MAV_PARAM_TYPE mavType);
    void _checkInitialLoadComplete(void);

    /// First mapping is by component id
    /// Second mapping is parameter name, to Fact* in QVariant
    QMap<int, QVariantMap>            _mapParameterName2Variant;

    // List of category map of component parameters
    typedef QMap<QString, QMap<QString, QStringList>>   ComponentCategoryMapType; //<Key: category, Value: Map< Key: group, Value: parameter names list >>
    QMap<int, ComponentCategoryMapType>                 _componentCategoryMaps;
    QHash<QString, int>                                 _componentCategoryHash;

    double      _loadProgress;                  ///< Parameter load progess, [0.0,1.0]
    bool        _parametersReady;               ///< true: parameter load complete
    bool        _missingParameters;             ///< true: parameter missing from initial load
    bool        _initialLoadComplete;           ///< true: Initial load of all parameters complete, whether successful or not
    bool        _waitingForDefaultComponent;    ///< true: last chance wait for default component params
    bool        _saveRequired;                  ///< true: _saveToEEPROM should be called
    bool        _metaDataAddedToFacts;          ///< true: FactMetaData has been adde to the default component facts
    bool        _logReplay;                     ///< true: running with log replay link
    QString     _versionParam;                  ///< Parameter which contains parameter set version
    int         _parameterSetMajorVersion;      ///< Version for parameter set, -1 if not known
    QObject*    _parameterMetaData;             ///< Opaque data from FirmwarePlugin::loadParameterMetaDataCall

    typedef QPair<int /* FactMetaData::ValueType_t */, QVariant /* Fact::rawValue */> ParamTypeVal;
    typedef QMap<QString /* parameter name */, ParamTypeVal> CacheMapName2ParamTypeVal;

    QMap<int /* component id */, bool>                                              _debugCacheCRC; ///< true: debug cache crc failure
    QMap<int /* component id */, CacheMapName2ParamTypeVal>                         _debugCacheMap;
    QMap<int /* component id */, QMap<QString /* param name */, bool /* seen */>>   _debugCacheParamSeen;

    // Wait counts from previous parameter update cycle
    int _prevWaitingReadParamIndexCount;
    int _prevWaitingReadParamNameCount;
    int _prevWaitingWriteParamNameCount;

    bool _readParamIndexProgressActive =    false;
    bool _readParamNameProgressActive =     false;
    bool _writeParamProgressActive =        false;

    static const int    _maxInitialRequestListRetry = 4;        ///< Maximum retries for request list
    int                 _initialRequestRetryCount;              ///< Current retry count for request list
    static const int    _maxInitialLoadRetrySingleParam = 5;    ///< Maximum retries for initial index based load of a single param
    static const int    _maxReadWriteRetry = 5;                 ///< Maximum retries read/write
    bool                _disableAllRetries;                     ///< true: Don't retry any requests (used for testing)

    bool        _indexBatchQueueActive; ///< true: we are actively batching re-requests for missing index base params, false: index based re-request has not yet started
    QList<int>  _indexBatchQueue;       ///< The current queue of index re-requests

    QMap<int, int>                  _paramCountMap;             ///< Key: Component id, Value: count of parameters in this component
    QMap<int, QMap<int, int> >      _waitingReadParamIndexMap;  ///< Key: Component id, Value: Map { Key: parameter index still waiting for, Value: retry count }
    QMap<int, QMap<QString, int> >  _waitingReadParamNameMap;   ///< Key: Component id, Value: Map { Key: parameter name still waiting for, Value: retry count }
    QMap<int, QMap<QString, int> >  _waitingWriteParamNameMap;  ///< Key: Component id, Value: Map { Key: parameter name still waiting for, Value: retry count }
    QMap<int, QList<int> >          _failedReadParamIndexMap;   ///< Key: Component id, Value: failed parameter index

    int _totalParamCount;                       ///< Number of parameters across all components
    int _waitingWriteParamBatchCount = 0;       ///< Number of parameters which are batched up waiting on write responses
    int _waitingReadParamNameBatchCount = 0;    ///< Number of parameters which are batched up waiting on read responses

    QTimer _initialRequestTimeoutTimer;
    QTimer _waitingParamTimeoutTimer;

    QMutex _dataMutex;

    Fact _defaultFact;   ///< Used to return default fact, when parameter not found

    static const char* _cachedMetaDataFilePrefix;
    static const char* _jsonParametersKey;
    static const char* _jsonCompIdKey;
    static const char* _jsonParamNameKey;
    static const char* _jsonParamValueKey;
};
