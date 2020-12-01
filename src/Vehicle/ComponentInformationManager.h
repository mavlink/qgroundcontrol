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
class CompInfo;
class CompInfoParam;
class CompInfoVersion;

class RequestMetaDataTypeStateMachine : public StateMachine
{
    Q_OBJECT

public:
    RequestMetaDataTypeStateMachine(ComponentInformationManager* compMgr);

    void        request     (CompInfo* compInfo);
    QString     typeToString(void);
    CompInfo*   compInfo    (void) { return _compInfo; }

    // Overrides from StateMachine
    int             stateCount      (void) const final;
    const StateFn*  rgStates        (void) const final;
    void            statesCompleted (void) const final;

private slots:
    void    _ftpDownloadCompleteMetaDataJson    (const QString& file, const QString& errorMsg);
    void    _ftpDownloadCompleteTranslationJson (const QString& file, const QString& errorMsg);
    void    _httpDownloadCompleteMetaDataJson   (QString remoteFile, QString localFile, QString errorMsg);
    void    _httpDownloadCompleteTranslationJson(QString remoteFile, QString localFile, QString errorMsg);
    QString _downloadCompleteJsonWorker         (const QString& jsonFileName, const QString& inflatedFileName);

private:
    static void _stateRequestCompInfo           (StateMachine* stateMachine);
    static void _stateRequestMetaDataJson       (StateMachine* stateMachine);
    static void _stateRequestTranslationJson    (StateMachine* stateMachine);
    static void _stateRequestComplete           (StateMachine* stateMachine);
    static bool _uriIsMAVLinkFTP                (const QString& uri);


    ComponentInformationManager*    _compMgr                    = nullptr;
    CompInfo*                       _compInfo                   = nullptr;
    QString                         _jsonMetadataFileName;
    QString                         _jsonTranslationFileName;

    static StateFn  _rgStates[];
    static int      _cStates;
};

class ComponentInformationManager : public StateMachine
{
    Q_OBJECT

public:
    ComponentInformationManager(Vehicle* vehicle);

    typedef void (*RequestAllCompleteFn)(void* requestAllCompleteFnData);

    void                requestAllComponentInformation  (RequestAllCompleteFn requestAllCompletFn, void * requestAllCompleteFnData);
    Vehicle*            vehicle                         (void) { return _vehicle; }
    CompInfoParam*      compInfoParam                   (uint8_t compId);
    CompInfoVersion*    compInfoVersion                 (uint8_t compId);

    // Overrides from StateMachine
    int             stateCount  (void) const final;
    const StateFn*  rgStates    (void) const final;

private:
    void _stateRequestCompInfoComplete  (void);
    bool _isCompTypeSupported           (COMP_METADATA_TYPE type);

    static void _stateRequestCompInfoVersion        (StateMachine* stateMachine);
    static void _stateRequestCompInfoParam          (StateMachine* stateMachine);
    static void _stateRequestAllCompInfoComplete    (StateMachine* stateMachine);

    Vehicle*                        _vehicle                    = nullptr;
    RequestMetaDataTypeStateMachine _requestTypeStateMachine;
    RequestAllCompleteFn            _requestAllCompleteFn       = nullptr;
    void*                           _requestAllCompleteFnData   = nullptr;

    QMap<uint8_t /* compId */, QMap<COMP_METADATA_TYPE, CompInfo*>> _compInfoMap;

    static StateFn                  _rgStates[];
    static int                      _cStates;

    friend class RequestMetaDataTypeStateMachine;
};
