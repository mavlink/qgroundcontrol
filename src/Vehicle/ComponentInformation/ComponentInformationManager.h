#pragma once

#include "MAVLinkLib.h"
#include "QGCStateMachine.h"
#include "RequestMetaDataTypeStateMachine.h"

#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(ComponentInformationManagerLog)

class Vehicle;
class ComponentInformationTranslation;
class ComponentInformationCache;
class CompInfo;
class CompInfoParam;
class CompInfoGeneral;
class QGCCachedFileDownload;
class AsyncFunctionState;
class SkippableAsyncState;
class FunctionState;
class QGCFinalState;

class ComponentInformationManager : public QGCStateMachine
{
    Q_OBJECT

public:
    explicit ComponentInformationManager(Vehicle *vehicle, QObject *parent = nullptr);
    ~ComponentInformationManager() override;

    typedef void (*RequestAllCompleteFn)(void *requestAllCompleteFnData);

    void requestAllComponentInformation(RequestAllCompleteFn requestAllCompletFn, void * requestAllCompleteFnData);
    CompInfoParam *compInfoParam(uint8_t compId);
    CompInfoGeneral *compInfoGeneral(uint8_t compId);

    ComponentInformationCache &fileCache() { return _fileCache; }
    ComponentInformationTranslation *translation() { return _translation; }

    float progress() const;

    static constexpr int cachedFileMaxAgeSec = 3 * 24 * 3600; ///< 3 days

signals:
    void progressUpdate(float progress);

private:
    void _createStates();
    void _wireTransitions();
    void _wireProgressTracking();

    // State action functions
    void _requestCompInfoGeneral(AsyncFunctionState* state);
    void _updateAllUri();
    void _requestCompInfoParam(SkippableAsyncState* state);
    void _requestCompInfoEvents(SkippableAsyncState* state);
    void _requestCompInfoActuators(SkippableAsyncState* state);
    void _signalComplete();

    // Skip predicates
    bool _isCompTypeSupported(COMP_METADATA_TYPE type) const;

    // Progress tracking
    void _updateProgress();

    static QString _getFileCacheTag(int compInfoType, uint32_t crc, bool isTranslation);

    RequestMetaDataTypeStateMachine _requestTypeStateMachine;
    RequestAllCompleteFn            _requestAllCompleteFn       = nullptr;
    void*                           _requestAllCompleteFnData   = nullptr;
    QGCCachedFileDownload*          _cachedFileDownload         = nullptr;
    ComponentInformationCache&      _fileCache;
    ComponentInformationTranslation* _translation               = nullptr;

    QMap<uint8_t /* compId */, QMap<COMP_METADATA_TYPE, CompInfo*>> _compInfoMap;

    // State pointers
    AsyncFunctionState*     _stateRequestGeneral    = nullptr;
    FunctionState*          _stateUpdateUri         = nullptr;
    SkippableAsyncState*    _stateRequestParam      = nullptr;
    SkippableAsyncState*    _stateRequestEvents     = nullptr;
    SkippableAsyncState*    _stateRequestActuators  = nullptr;
    FunctionState*          _stateComplete          = nullptr;
    QGCFinalState*          _stateFinal             = nullptr;

    // Progress tracking
    int _currentStateIndex = 0;
    static constexpr int _stateCount = 6;

    friend class RequestMetaDataTypeStateMachine;
};
