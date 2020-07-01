/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCLoggingCategory.h"
#include "QGCMAVLink.h"
#include "StateMachine.h"

Q_DECLARE_LOGGING_CATEGORY(ComponentInformationManagerLog)

class Vehicle;
class ComponentInformationManager;

typedef struct {
    uint32_t        metadataUID;
    QString         metadataURI;
    uint32_t        translationUID;
    QString         translationURI;
} ComponentInformation_t;

class RequestMetaDataTypeStateMachine : public StateMachine
{
    Q_OBJECT

public:
    RequestMetaDataTypeStateMachine(ComponentInformationManager* compMgr);

    void                            request                     (COMP_METADATA_TYPE type);
    QString                         typeToString                (void);
    ComponentInformationManager*    compMgr                     (void) { return _compMgr; }
    void                            handleComponentInformation  (const mavlink_message_t& message);

    // Overrides from StateMachine
    int             stateCount      (void) const final;
    const StateFn*  rgStates        (void) const final;
    void            statesCompleted (void) const final;

private slots:
    void    _downloadCompleteMetaDataJson   (const QString& file, const QString& errorMsg);
    void    _downloadCompleteTranslationJson(const QString& file, const QString& errorMsg);
    QString _downloadCompleteJsonWorker     (const QString& jsonFileName, const QString& inflatedFileName);

private:
    static void _stateRequestCompInfo           (StateMachine* stateMachine);
    static void _stateRequestMetaDataJson       (StateMachine* stateMachine);
    static void _stateRequestTranslationJson    (StateMachine* stateMachine);
    static void _stateRequestComplete           (StateMachine* stateMachine);
    static bool _uriIsFTP                       (const QString& uri);


    ComponentInformationManager*    _compMgr;
    COMP_METADATA_TYPE              _type               = COMP_METADATA_TYPE_VERSION;
    bool                            _compInfoAvailable  = false;
    ComponentInformation_t          _compInfo;
    QString                         _jsonMetadataFileName;
    QString                         _jsonTranslationFileName;

    static StateFn                  _rgStates[];
    static int                      _cStates;
};

class ComponentInformationManager : public StateMachine
{
    Q_OBJECT

public:
    ComponentInformationManager(Vehicle* vehicle);

    typedef void (*RequestAllCompleteFn)(void* requestAllCompleteFnData);

    void        requestAllComponentInformation  (RequestAllCompleteFn requestAllCompletFn, void * requestAllCompleteFnData);
    Vehicle*    vehicle                         (void) { return _vehicle; }

    // Overrides from StateMachine
    int             stateCount  (void) const final;
    const StateFn*  rgStates    (void) const final;

private:
    void _stateRequestCompInfoComplete  (void);
    void _compInfoJsonAvailable         (const QString& metadataJsonFileName, const QString& translationsJsonFileName);
    bool _isCompTypeSupported           (COMP_METADATA_TYPE type);

    static void _stateRequestCompInfoVersion        (StateMachine* stateMachine);
    static void _stateRequestCompInfoParam          (StateMachine* stateMachine);
    static void _stateRequestAllCompInfoComplete    (StateMachine* stateMachine);

    Vehicle*                        _vehicle                    = nullptr;
    RequestMetaDataTypeStateMachine _requestTypeStateMachine;
    bool                            _versionCompInfoAvailable   = false;
    ComponentInformation_t          _versionCompInfo;
    bool                            _paramCompInfoAvailable     = false;
    ComponentInformation_t          _parameterCompInfo;
    QList<COMP_METADATA_TYPE>       _supportedMetaDataTypes;
    RequestAllCompleteFn            _requestAllCompleteFn       = nullptr;
    void*                           _requestAllCompleteFnData   = nullptr;

    static StateFn                  _rgStates[];
    static int                      _cStates;

    static const char*              _jsonVersionKey;
    static const char*              _jsonSupportedCompMetadataTypesKey;

    friend class RequestMetaDataTypeStateMachine;
};
