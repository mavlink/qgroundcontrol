#pragma once

#include "QGCStateMachine.h"
#include "MAVLinkLib.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(RequestMetaDataTypeStateMachineLog)

class Vehicle;
class ComponentInformationManager;
class CompInfo;
class AsyncFunctionState;
class SkippableAsyncState;
class ConditionalState;
class FunctionState;

class RequestMetaDataTypeStateMachine : public QGCStateMachine
{
    Q_OBJECT

public:
    explicit RequestMetaDataTypeStateMachine(ComponentInformationManager* compMgr, QObject* parent = nullptr);
    ~RequestMetaDataTypeStateMachine() override;

    void request(CompInfo* compInfo);
    QString typeToString() const;
    CompInfo* compInfo() const { return _compInfo; }
    ComponentInformationManager* compMgr() const { return _compMgr; }

signals:
    void requestComplete();

private:
    void _createStates();
    void _wireTransitions();
    void _wireTimeoutHandling();

    // State entry/action functions
    void _requestCompInfo();
    void _requestCompInfoDeprecated();
    void _requestMetaDataJson();
    void _requestMetaDataJsonFallback();
    void _requestTranslationJson();
    void _requestTranslate();
    void _completeRequest();

    // Skip predicates
    bool _shouldSkipCompInfoRequest() const;
    bool _shouldSkipDeprecatedRequest() const;
    bool _shouldSkipFallback() const;
    bool _shouldSkipTranslation() const;

    // Download helpers
    void _requestFile(const QString& cacheFileTag, bool crcValid, const QString& uri, QString& outputFileName);
    QString _downloadCompleteJsonWorker(const QString& jsonFileName);
    static bool _uriIsMAVLinkFTP(const QString& uri);

    // Message result handlers
    void _handleCompMetadataResult(MAV_RESULT result, const mavlink_message_t& message);
    void _handleCompInfoResult(MAV_RESULT result, Vehicle::RequestMessageResultHandlerFailureCode_t failureCode, const mavlink_message_t& message);

private slots:
    void _ftpDownloadComplete(const QString& file, const QString& errorMsg);
    void _ftpDownloadProgress(float progress);
    void _httpDownloadComplete(QString remoteFile, QString localFile, QString errorMsg);
    void _downloadAndTranslationComplete(QString translatedJsonTempFile, QString errorMsg);

private:
    ComponentInformationManager* _compMgr = nullptr;
    CompInfo* _compInfo = nullptr;

    // Download state
    QString _jsonMetadataFileName;
    QString _jsonMetadataTranslatedFileName;
    bool _jsonMetadataCrcValid = false;
    QString _jsonTranslationFileName;
    bool _jsonTranslationCrcValid = false;

    QString* _currentFileName = nullptr;
    QString _currentCacheFileTag;
    bool _currentFileValidCrc = false;

    QElapsedTimer _downloadStartTime;

    // State pointers
    AsyncFunctionState* _stateRequestCompInfo = nullptr;
    SkippableAsyncState* _stateRequestDeprecated = nullptr;
    AsyncFunctionState* _stateRequestMetaDataJson = nullptr;
    SkippableAsyncState* _stateRequestMetaDataJsonFallback = nullptr;
    AsyncFunctionState* _stateRequestTranslationJson = nullptr;
    SkippableAsyncState* _stateRequestTranslate = nullptr;
    FunctionState* _stateComplete = nullptr;
    QGCFinalState* _stateFinal = nullptr;

    // Track active download state for completion
    AsyncFunctionState* _activeAsyncState = nullptr;
    SkippableAsyncState* _activeSkippableState = nullptr;

    // Timeout values (ms)
    static constexpr int _timeoutCompInfoRequest = 5000;
    static constexpr int _timeoutMetaDataDownload = 30000;
    static constexpr int _timeoutTranslation = 15000;
};
