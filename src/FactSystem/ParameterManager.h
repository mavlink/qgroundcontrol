/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef ParameterManager_H
#define ParameterManager_H

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

/// @file
///     @author Don Gagne <don@thegagnes.com>

Q_DECLARE_LOGGING_CATEGORY(ParameterManagerVerbose1Log)
Q_DECLARE_LOGGING_CATEGORY(ParameterManagerVerbose2Log)

/// Connects to Parameter Manager to load/update Facts
class ParameterManager : public QObject
{
    Q_OBJECT
    
public:
    /// @param uas Uas which this set of facts is associated with
    ParameterManager(Vehicle* vehicle);
    ~ParameterManager();

    /// true: Parameters are ready for use
    Q_PROPERTY(bool parametersReady READ parametersReady NOTIFY parametersReadyChanged)
    bool parametersReady(void) { return _parametersReady; }

    /// true: Parameters are missing from firmware response, false: all parameters received from firmware
    Q_PROPERTY(bool missingParameters READ missingParameters NOTIFY missingParametersChanged)
    bool missingParameters(void) { return _missingParameters; }

    /// @return Directory of parameter caches
    static QDir parameterCacheDir();

    /// @return Location of parameter cache file
    static QString parameterCacheFile(int vehicleId, int componentId);
    

    /// Re-request the full set of parameters from the autopilot
    void refreshAllParameters(uint8_t componentID = MAV_COMP_ID_ALL);

    /// Request a refresh on the specific parameter
    void refreshParameter(int componentId, const QString& name);
    
    /// Request a refresh on all parameters that begin with the specified prefix
    void refreshParametersPrefix(int componentId, const QString& namePrefix);
    
    void resetAllParametersToDefaults(void);

    /// Returns true if the specifed parameter exists
    ///     @param componentId Component id or FactSystem::defaultComponentId
    ///     @param name Parameter name
    bool parameterExists(int componentId, const QString& name);

	/// Returns all parameter names
	QStringList parameterNames(int componentId);
    
    /// Returns the specified Parameter. Returns a default empty fact is parameter does not exists. Also will pop
    /// a missing parameter error to user if parameter does not exist.
    ///     @param componentId Component id or FactSystem::defaultComponentId
    ///     @param name Parameter name
    Fact* getParameter(int componentId, const QString& name);
    
    const QMap<int, QMap<QString, QStringList> >& getGroupMap(void);
    
    /// Returns error messages from loading
    QString readParametersFromStream(QTextStream& stream);
    
    void writeParametersToStream(QTextStream &stream);

    /// Returns the version number for the parameter set, -1 if not known
    int parameterSetVersion(void) { return _parameterSetMajorVersion; }

    /// Returns the newest available parameter meta data file (from cache or internal) for the specified information.
    ///     @param wantedMajorVersion Major version you are looking for
    ///     @param[out] majorVersion Major version for found meta data
    ///     @param[out] minorVersion Minor version for found meta data
    /// @return Meta data file name of best match, emptyString is none found
    static QString parameterMetaDataFile(MAV_AUTOPILOT firmwareType, int wantedMajorVersion, int& majorVersion, int& minorVersion);

    /// If this file is newer than anything in the cache, cache it as the latest version
    static void cacheMetaDataFile(const QString& metaDataFile, MAV_AUTOPILOT firmwareType);

    int defaultComponentId(void) { return _defaultComponentId; }

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

    Vehicle* vehicle(void) { return _vehicle; }

signals:
    void parametersReadyChanged(bool parametersReady);
    void missingParametersChanged(bool missingParameters);

    /// Signalled to update progress of full parameter list request
    void parameterListProgress(float value);
    
protected:
    Vehicle*            _vehicle;
    MAVLinkProtocol*    _mavlink;
    
    void _parameterUpdate(int vehicleId, int componentId, QString parameterName, int parameterCount, int parameterId, int mavType, QVariant value);
    void _valueUpdated(const QVariant& value);
    void _waitingParamTimeout(void);
    void _tryCacheLookup(void);
    void _initialRequestTimeout(void);

private:
    static QVariant _stringToTypedVariant(const QString& string, FactMetaData::ValueType_t type, bool failOk = false);
    int _actualComponentId(int componentId);
    void _determineDefaultComponentId(void);
    void _setupGroupMap(void);
    void _readParameterRaw(int componentId, const QString& paramName, int paramIndex);
    void _writeParameterRaw(int componentId, const QString& paramName, const QVariant& value);
    void _writeLocalParamCache(int vehicleId, int componentId);
    void _tryCacheHashLoad(int vehicleId, int componentId, QVariant hash_value);
    void _addMetaDataToDefaultComponent(void);
    QString _remapParamNameToVersion(const QString& paramName);
    void _loadOfflineEditingParams(void);
    QString _logVehiclePrefix(int componentId = -1);

    MAV_PARAM_TYPE _factTypeToMavType(FactMetaData::ValueType_t factType);
    FactMetaData::ValueType_t _mavTypeToFactType(MAV_PARAM_TYPE mavType);
    void _saveToEEPROM(void);
    void _checkInitialLoadComplete(bool failIfNoDefaultComponent);

    /// First mapping is by component id
    /// Second mapping is parameter name, to Fact* in QVariant
    QMap<int, QVariantMap>            _mapParameterName2Variant;

    QMap<int, QMap<int, QString> >    _mapParameterId2Name;
    
    /// First mapping is by component id
    /// Second mapping is group name, to Fact
    QMap<int, QMap<QString, QStringList> > _mapGroup2ParameterName;
    
    bool        _parametersReady;               ///< true: parameter load complete
    bool        _missingParameters;             ///< true: parameter missing from initial load
    bool        _initialLoadComplete;           ///< true: Initial load of all parameters complete, whether successful or not
    bool        _waitingForDefaultComponent;    ///< true: last chance wait for default component params
    bool        _saveRequired;                  ///< true: _saveToEEPROM should be called
    int         _defaultComponentId;
    QString     _defaultComponentIdParam;       ///< Parameter which identifies default component
    QString     _versionParam;                  ///< Parameter which contains parameter set version
    int         _parameterSetMajorVersion;      ///< Version for parameter set, -1 if not known
    QObject*    _parameterMetaData;             ///< Opaque data from FirmwarePlugin::loadParameterMetaDataCall

    // Wait counts from previous parameter update cycle
    int         _prevWaitingReadParamIndexCount;
    int         _prevWaitingReadParamNameCount;
    int         _prevWaitingWriteParamNameCount;


    static const int    _maxInitialRequestListRetry = 4;        ///< Maximum retries for request list
    int                 _initialRequestRetryCount;              ///< Current retry count for request list
    static const int    _maxInitialLoadRetrySingleParam = 5;    ///< Maximum retries for initial index based load of a single param
    static const int    _maxReadWriteRetry = 5;                 ///< Maximum retries read/write
    bool                _disableAllRetries;                     ///< true: Don't retry any requests (used for testing)

    QMap<int, int>                  _paramCountMap;             ///< Key: Component id, Value: count of parameters in this component
    QMap<int, QMap<int, int> >      _waitingReadParamIndexMap;  ///< Key: Component id, Value: Map { Key: parameter index still waiting for, Value: retry count }
    QMap<int, QMap<QString, int> >  _waitingReadParamNameMap;   ///< Key: Component id, Value: Map { Key: parameter name still waiting for, Value: retry count }
    QMap<int, QMap<QString, int> >  _waitingWriteParamNameMap;  ///< Key: Component id, Value: Map { Key: parameter name still waiting for, Value: retry count }
    QMap<int, QList<int> >          _failedReadParamIndexMap;   ///< Key: Component id, Value: failed parameter index

    int _totalParamCount;   ///< Number of parameters across all components
    
    QTimer _initialRequestTimeoutTimer;
    QTimer _waitingParamTimeoutTimer;
    
    QMutex _dataMutex;
    
    static Fact _defaultFact;   ///< Used to return default fact, when parameter not found

    static const char* _cachedMetaDataFilePrefix;
    static const char* _jsonParametersKey;
    static const char* _jsonCompIdKey;
    static const char* _jsonParamNameKey;
    static const char* _jsonParamValueKey;
};

#endif
