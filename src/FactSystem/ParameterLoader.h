/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef PARAMETERLOADER_H
#define PARAMETERLOADER_H

#include <QObject>
#include <QMap>
#include <QXmlStreamReader>
#include <QLoggingCategory>
#include <QMutex>
#include <QDir>

#include "FactSystem.h"
#include "MAVLinkProtocol.h"
#include "AutoPilotPlugin.h"
#include "QGCMAVLink.h"
#include "Vehicle.h"

/// @file
///     @author Don Gagne <don@thegagnes.com>

Q_DECLARE_LOGGING_CATEGORY(ParameterLoaderVerboseLog)

/// Connects to Parameter Manager to load/update Facts
class ParameterLoader : public QObject
{
    Q_OBJECT
    
public:
    /// @param uas Uas which this set of facts is associated with
    ParameterLoader(Vehicle* vehicle);
    
    ~ParameterLoader();

    /// @return Directory of parameter caches
    static QDir parameterCacheDir();

    /// @return Location of parameter cache file
    static QString parameterCacheFile(int uasId, int componentId);
    
    /// Returns true if the full set of facts are ready
    bool parametersAreReady(void) { return _parametersReady; }

    /// Re-request the full set of parameters from the autopilot
    void refreshAllParameters(uint8_t componentID = MAV_COMP_ID_ALL);

    /// Request a refresh on the specific parameter
    void refreshParameter(int componentId, const QString& name);
    
    /// Request a refresh on all parameters that begin with the specified prefix
    void refreshParametersPrefix(int componentId, const QString& namePrefix);
    
    /// Returns true if the specifed parameter exists
    bool parameterExists(int			componentId,    ///< fact component, -1=default component
						 const QString& name);          ///< fact name
	
	/// Returns all parameter names
	QStringList parameterNames(int componentId);
    
    /// Returns the specified Fact.
    /// WARNING: Will assert if parameter does not exists. If that possibily exists, check for existence first with
    /// parameterExists.
    Fact* getFact(int               componentId,    ///< fact component, -1=default component
                  const QString&    name);          ///< fact name
    
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

    int defaultComponenentId(void) { return _defaultComponentId; }
    
signals:
    /// Signalled when the full set of facts are ready
    void parametersReady(bool missingParameters);

    /// Signalled to update progress of full parameter list request
    void parameterListProgress(float value);
    
    /// Signalled to ourselves in order to get call on our own thread
    void restartWaitingParamTimer(void);
    
protected:
    Vehicle*            _vehicle;
    MAVLinkProtocol*    _mavlink;
    
    void _parameterUpdate(int uasId, int componentId, QString parameterName, int parameterCount, int parameterId, int mavType, QVariant value);
    void _valueUpdated(const QVariant& value);
    void _restartWaitingParamTimer(void);
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
    void _writeLocalParamCache(int uasId, int componentId);
    void _tryCacheHashLoad(int uasId, int componentId, QVariant hash_value);
    void _addMetaDataToDefaultComponent(void);
    QString _remapParamNameToVersion(const QString& paramName);

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
    
    bool        _parametersReady;               ///< true: full set of parameters correctly loaded
    bool        _initialLoadComplete;           ///< true: Initial load of all parameters complete, whether succesful or not
    bool        _waitingForDefaultComponent;    ///< true: last chance wait for default component params
    bool        _saveRequired;                  ///< true: _saveToEEPROM should be called
    int         _defaultComponentId;
    QString     _defaultComponentIdParam;       ///< Parameter which identifies default component
    QString     _versionParam;                  ///< Parameter which contains parameter set version
    int         _parameterSetMajorVersion;      ///< Version for parameter set, -1 if not known
    QObject*    _parameterMetaData;             ///< Opaque data from FirmwarePlugin::loadParameterMetaDataCall

    static const int _maxInitialLoadRetry = 10;                 ///< Maximum retries for initial index based load
    static const int _maxReadWriteRetry = 5;                    ///< Maximum retries read/write

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
};

#endif
