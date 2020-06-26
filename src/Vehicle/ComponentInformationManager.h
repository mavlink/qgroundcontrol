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

class RequestMetaDataTypeStateMachine : public StateMachine
{
public:
    RequestMetaDataTypeStateMachine(ComponentInformationManager* compMgr);

    void request(COMP_METADATA_TYPE type);
    ComponentInformationManager* compMgr(void) { return _compMgr; }
    QString typeToString(void);

    // Overrides from StateMachine
    int             stateCount      (void) const final;
    const StateFn*  rgStates        (void) const final;
    void            statesCompleted (void) const final;

private:
    static void _stateRequestCompInfo           (StateMachine* stateMachine);
    static void _stateRequestMetaDataJson       (StateMachine* stateMachine);
    static void _stateRequestTranslationJson    (StateMachine* stateMachine);

    ComponentInformationManager*    _compMgr;
    COMP_METADATA_TYPE              _type = COMP_METADATA_TYPE_VERSION;

    static StateFn                  _rgStates[];
    static int                      _cStates;
};

class ComponentInformationManager : public StateMachine
{
public:
    ComponentInformationManager(Vehicle* vehicle);

    typedef struct {
        uint32_t        metadataUID;
        QString         metadataURI;
        uint32_t        translationUID;
        QString         translationURI;
    } ComponentInformation_t;

    typedef void (*RequestAllCompleteFn)(void* requestAllCompleteFnData);

    void        requestAllComponentInformation  (RequestAllCompleteFn requestAllCompletFn, void * requestAllCompleteFnData);
    Vehicle*    vehicle                         (void) { return _vehicle; }

    // These methods should only be called by RequestMetaDataTypeStateMachine
    void _componentInformationReceived(const mavlink_message_t& message);
    void _stateRequestCompInfoComplete(void);

    // Overrides from StateMachine
    int             stateCount  (void) const final;
    const StateFn*  rgStates    (void) const final;

private:
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
};
