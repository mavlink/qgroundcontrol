#pragma once

#include <memory>

#include "MAVLinkEnums.h"
#include "QGCStateMachine.h"
#include "RequestMetaDataTypeStateMachine.h"

class Vehicle;
class ComponentInformationTranslation;
class ComponentInformationCache;
class CompInfo;
class CompInfoParam;
class CompInfoGeneral;
class QGCCachedFileDownload;
class AsyncFunctionState;
class FunctionState;
class QGCFinalState;
class QTemporaryDir;

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

    /// Where the metadata downloads of this vehicle are written: a directory of its own, removed with this manager,
    /// so that the files of two vehicles never share a path. The first call creates it and directs the downloader of
    /// the manager to it. The temp location if it cannot be created
    QString downloadDirectory();

    static constexpr int cachedFileMaxAgeSec = 3 * 24 * 3600; ///< 3 days
    /// Directory of the network disk cache of the files that are downloaded with HTTP
    static QString httpCacheDirectory();

signals:
    void requestAllComplete();

private:
    void _createStates();
    void _wireTransitions();
    void _wireProgressTracking();

    // State action functions
    void _requestCompInfoGeneral(AsyncFunctionState* state);
    void _updateAllUri();
    void _requestCompInfoParam(AsyncFunctionState* state);
    void _requestCompInfoEvents(AsyncFunctionState* state);
    void _requestCompInfoActuators(AsyncFunctionState* state);
    void _signalComplete();

    // Skip predicates
    bool _isCompTypeSupported(COMP_METADATA_TYPE type) const;

    static QString _getFileCacheTag(int compInfoType, uint32_t crc, bool isTranslation);

    /// Declared before the request machine, which goes first: its download closes the file it writes into the
    /// directory, which Windows could not remove otherwise
    std::unique_ptr<QTemporaryDir> _downloadDir;
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
    AsyncFunctionState* _stateRequestParam = nullptr;
    AsyncFunctionState* _stateRequestEvents = nullptr;
    AsyncFunctionState* _stateRequestActuators = nullptr;
    FunctionState*          _stateComplete          = nullptr;
    QGCFinalState*          _stateFinal             = nullptr;

    friend class RequestMetaDataTypeStateMachine;
};
