/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "MAVLinkLib.h"
#include "StateMachine.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(RequestMetaDataTypeStateMachineLog)
Q_DECLARE_LOGGING_CATEGORY(ComponentInformationManagerLog)

class Vehicle;
class ComponentInformationManager;
class ComponentInformationTranslation;
class ComponentInformationCache;
class CompInfo;
class CompInfoParam;
class CompInfoGeneral;
class QGCCachedFileDownload;

class RequestMetaDataTypeStateMachine : public StateMachine
{
    Q_OBJECT

public:
    RequestMetaDataTypeStateMachine(ComponentInformationManager *compMgr, QObject *parent = nullptr);
    ~RequestMetaDataTypeStateMachine();

    void        request     (CompInfo* compInfo);
    QString     typeToString(void);
    CompInfo*   compInfo    (void) { return _compInfo; }

    // Overrides from StateMachine
    int             stateCount      (void) const final;
    const StateFn*  rgStates        (void) const final;
    void            statesCompleted (void) const final;

private slots:
    void    _ftpDownloadComplete                (const QString& file, const QString& errorMsg);
    void    _ftpDownloadProgress                (float progress);
    void    _httpDownloadComplete               (QString remoteFile, QString localFile, QString errorMsg);
    QString _downloadCompleteJsonWorker         (const QString& jsonFileName);
    void _downloadAndTranslationComplete(QString translatedJsonTempFile, QString errorMsg);

private:
    static void _stateRequestCompInfo           (StateMachine* stateMachine);
    static void _stateRequestCompInfoDeprecated (StateMachine* stateMachine);
    static void _stateRequestMetaDataJson       (StateMachine* stateMachine);
    static void _stateRequestMetaDataJsonFallback(StateMachine* stateMachine);
    static void _stateRequestTranslationJson    (StateMachine* stateMachine);
    static void _stateRequestTranslate          (StateMachine* stateMachine);
    static void _stateRequestComplete           (StateMachine* stateMachine);
    static bool _uriIsMAVLinkFTP                (const QString& uri);

    void _requestFile(const QString& cacheFileTag, bool crcValid, const QString& uri, QString& outputFileName);

    ComponentInformationManager*    _compMgr                    = nullptr;
    CompInfo*                       _compInfo                   = nullptr;
    QString                         _jsonMetadataFileName;
    QString                         _jsonMetadataTranslatedFileName;
    bool                            _jsonMetadataCrcValid       = false;
    QString                         _jsonTranslationFileName;
    bool                            _jsonTranslationCrcValid    = false;

    QString*                        _currentFileName            = nullptr;
    QString                         _currentCacheFileTag;
    bool                            _currentFileValidCrc        = false;

    QElapsedTimer                   _downloadStartTime;

    static constexpr const StateFn _rgStates[]= {
        _stateRequestCompInfo,
        _stateRequestCompInfoDeprecated,
        _stateRequestMetaDataJson,
        _stateRequestMetaDataJsonFallback,
        _stateRequestTranslationJson,
        _stateRequestTranslate,
        _stateRequestComplete,
    };

    static constexpr int _cStates = sizeof(_rgStates) / sizeof(_rgStates[0]);
};

class ComponentInformationManager : public StateMachine
{
    Q_OBJECT

public:
    ComponentInformationManager(Vehicle *vehicle, QObject *parent = nullptr);
    ~ComponentInformationManager();

    typedef void (*RequestAllCompleteFn)(void *requestAllCompleteFnData);

    void requestAllComponentInformation(RequestAllCompleteFn requestAllCompletFn, void * requestAllCompleteFnData);
    Vehicle *vehicle() { return _vehicle; }
    CompInfoParam *compInfoParam(uint8_t compId);
    CompInfoGeneral *compInfoGeneral(uint8_t compId);

    // Overrides from StateMachine
    int stateCount() const final;
    const StateFn *rgStates() const final;

    ComponentInformationCache &fileCache() { return _fileCache; }
    ComponentInformationTranslation *translation() { return _translation; }

    float progress() const;

    void advance() override;

    static constexpr int cachedFileMaxAgeSec = 3 * 24 * 3600; ///< 3 days

signals:
    void progressUpdate(float progress);

private:
    void _stateRequestCompInfoComplete  (void);
    bool _isCompTypeSupported           (COMP_METADATA_TYPE type);
    void _updateAllUri                  ();

    static QString _getFileCacheTag(int compInfoType, uint32_t crc, bool isTranslation);

    static void _stateRequestCompInfoGeneral        (StateMachine* stateMachine);
    static void _stateRequestCompInfoGeneralComplete(StateMachine* stateMachine);
    static void _stateRequestCompInfoParam          (StateMachine* stateMachine);
    static void _stateRequestCompInfoEvents         (StateMachine* stateMachine);
    static void _stateRequestCompInfoActuators      (StateMachine* stateMachine);
    static void _stateRequestAllCompInfoComplete    (StateMachine* stateMachine);

    Vehicle*                        _vehicle                    = nullptr;
    RequestMetaDataTypeStateMachine _requestTypeStateMachine;
    RequestAllCompleteFn            _requestAllCompleteFn       = nullptr;
    void*                           _requestAllCompleteFnData   = nullptr;
    QGCCachedFileDownload*          _cachedFileDownload         = nullptr;
    ComponentInformationCache&      _fileCache;
    ComponentInformationTranslation* _translation               = nullptr;

    QMap<uint8_t /* compId */, QMap<COMP_METADATA_TYPE, CompInfo*>> _compInfoMap;

    static constexpr const StateFn _rgStates[]= {
        _stateRequestCompInfoGeneral,
        _stateRequestCompInfoGeneralComplete,
        _stateRequestCompInfoParam,
        _stateRequestCompInfoEvents,
        _stateRequestCompInfoActuators,
        _stateRequestAllCompInfoComplete
    };

    static constexpr int _cStates = sizeof(_rgStates) / sizeof(_rgStates[0]);

    friend class RequestMetaDataTypeStateMachine;
};
