#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QFuture>
#include <QtCore/QPointer>

#include "MAVLinkEnums.h"
#include "MAVLinkMessageType.h"
#include "QGCStateMachine.h"
#include "VehicleTypes.h"
class Vehicle;
class ComponentInformationManager;
class CompInfo;
class AsyncFunctionState;
class CompletionWaitState;
class VehicleFileFetch;
class QGCState;

class RequestMetaDataTypeStateMachine : public QGCStateMachine
{
    Q_OBJECT

public:
    /// How long a state waits, in milliseconds, before the sequence goes on without what it asked for
    struct Timeouts
    {
        int compInfoRequest;
        int metaDataDownload;
        int translation;
        int metaDataLoad;
    };

    /// Vehicle gives a message request up by itself, after three command attempts and the wait for the message
    /// at the latest (about 6.7 s). The request timeout is a backstop above that. Reading and parsing the largest
    /// metadata file the readers accept takes several seconds (about 7 s in a debug build); the load timeout is a
    /// backstop above that.
    static constexpr Timeouts defaultTimeouts{8000, 30000, 15000, 30000};

    explicit RequestMetaDataTypeStateMachine(ComponentInformationManager* compMgr, QObject* parent = nullptr,
                                             const Timeouts& timeouts = defaultTimeouts);
    ~RequestMetaDataTypeStateMachine() override;

    void request(CompInfo* compInfo);
    QString typeToString() const;
    CompInfo* compInfo() const { return _compInfo; }
    ComponentInformationManager* compMgr() const { return _compMgr; }

signals:
    void requestComplete();

private:
    void _createStates(const Timeouts& timeouts);
    void _wireTransitions();
    void _wireTimeoutHandling();

    // State entry/action functions
    void _requestCompInfo();
    void _requestCompInfoDeprecated();
    void _requestMetaDataJson();
    void _requestMetaDataJsonFallback();
    void _requestTranslationJson();
    void _requestTranslate();
    void _loadMetaData();
    void _completeRequest();

    // Skip predicates
    bool _shouldSkipCompInfoRequest() const;
    bool _shouldSkipDeprecatedRequest() const;
    bool _shouldSkipFallback() const;
    bool _shouldSkipTranslation() const;

    // Download helpers
    void _requestFile(CompletionWaitState* state, const QString& cacheFileTag, bool crcValid, const QString& uri,
                      QString& outputFileName, bool trackMetadataSource);
    /// Caches the metadata file once it has been read, or removes it from the cache if it came from there and could
    /// not be read
    void _updateFileCache(bool read);
    void _ftpDownloadProgress(CompletionWaitState* state, float progress);

    // What a state that is left gives up, so that nothing of it reaches a later state
    void _cancelMessageRequest();
    void _cancelTransfer();
    void _cancelLoad();

    enum class MetadataSource {
        None,
        Cache,
        FTP,
        HTTP,
    };
    static const char* _metadataSourceToString(MetadataSource source);

    // Message result handlers
    void _handleCompMetadataResult(MAV_RESULT result, const mavlink_message_t& message);
    void _handleCompInfoResult(MAV_RESULT result, VehicleTypes::RequestMessageResultHandlerFailureCode_t failureCode, const mavlink_message_t& message);

    /// A download of this machine that has started and not ended
    enum class Transfer
    {
        None,
        Download,
        Translation,
    };

    ComponentInformationManager* _compMgr = nullptr;
    CompInfo* _compInfo = nullptr;

    // Download state
    QString _jsonMetadataFileName;
    QString _jsonMetadataTranslatedFileName;
    bool _jsonMetadataCrcValid = false;
    /// Tag of _jsonMetadataFileName in the file cache, empty if it is not cached for lack of a CRC
    QString _jsonMetadataCacheTag;
    /// _jsonMetadataFileName is a download that is not in the file cache, removed when the request completes
    bool _jsonMetadataDownloaded = false;
    QString _jsonTranslationFileName;

    Transfer _transferInFlight = Transfer::None;
    QPointer<VehicleFileFetch> _fetch;
    QElapsedTimer _downloadStartTime;
    MetadataSource _metadataSource = MetadataSource::None;
    QString _metadataUri;
    bool _metadataIsFallback = false;

    /// Reads and parses the metadata file on a worker thread while the load state waits for it
    QPointer<QObject> _loadJob;
    /// Ends when the last load has stopped reading its file, also one whose state has given it up
    QFuture<void> _loadEnded;

    // State pointers
    AsyncFunctionState* _stateRequestCompInfo = nullptr;
    AsyncFunctionState* _stateRequestDeprecated = nullptr;
    AsyncFunctionState* _stateRequestMetaDataJson = nullptr;
    AsyncFunctionState* _stateRequestMetaDataJsonFallback = nullptr;
    AsyncFunctionState* _stateRequestTranslationJson = nullptr;
    AsyncFunctionState* _stateRequestTranslate = nullptr;
    AsyncFunctionState* _stateLoadMetaData = nullptr;
    QGCState* _stateComplete = nullptr;
    QGCFinalState* _stateFinal = nullptr;
};
