#pragma once

#include "GeoTagData.h"
#include "GeoTagImageModel.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QFileInfo>
#include <QtCore/QFileInfoList>
#include <QtCore/QFutureWatcher>
#include <QtCore/QHash>
#include <QtCore/QMap>
#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtPositioning/QGeoCoordinate>
#include <QtQmlIntegration/QtQmlIntegration>

#include <atomic>


/// Result of timestamp-based calibration/matching
struct CalibrationResult {
    QList<int> imageIndices;      ///< Matched image indices (parallel with triggerIndices)
    QList<int> triggerIndices;    ///< Matched trigger indices (parallel with imageIndices)
    QList<int> unmatchedImages;   ///< Image indices with no matching trigger
    int skippedTriggers = 0;      ///< Triggers skipped due to invalid data
};

namespace GeoTagCalibrator {

/// Match image timestamps to trigger timestamps using tolerance-based search
/// @param imageTimestamps List of image capture times (seconds since epoch)
/// @param triggers List of camera trigger events from flight log
/// @param timeOffsetSecs Offset to apply to image timestamps (camera clock drift)
/// @param toleranceSecs Maximum time difference for a valid match
/// @return CalibrationResult with matched and unmatched indices
CalibrationResult calibrate(const QList<qint64> &imageTimestamps,
                            const QList<GeoTagData> &triggers,
                            qint64 timeOffsetSecs,
                            qint64 toleranceSecs);

} // namespace GeoTagCalibrator

/// Controller for GeoTagPage.qml. Supports geotagging images based on logfile camera tags.
/// Uses async signal-based processing with QFutureWatcher for non-blocking operation.
class GeoTagController : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString           logFile         READ logFile        WRITE setLogFile        NOTIFY logFileChanged)
    Q_PROPERTY(QString           imageDirectory  READ imageDirectory WRITE setImageDirectory NOTIFY imageDirectoryChanged)
    Q_PROPERTY(QString           saveDirectory   READ saveDirectory  WRITE setSaveDirectory  NOTIFY saveDirectoryChanged)
    Q_PROPERTY(QString           errorMessage    READ errorMessage                           NOTIFY errorMessageChanged)
    Q_PROPERTY(double            progress        READ progress                               NOTIFY progressChanged)
    Q_PROPERTY(bool              inProgress      READ inProgress                             NOTIFY inProgressChanged)
    Q_PROPERTY(int               taggedCount     READ taggedCount                            NOTIFY taggingCompleteChanged)
    Q_PROPERTY(int               skippedCount    READ skippedCount                           NOTIFY taggingCompleteChanged)
    Q_PROPERTY(int               failedCount     READ failedCount                            NOTIFY taggingCompleteChanged)
    Q_PROPERTY(double            timeOffsetSecs  READ timeOffsetSecs  WRITE setTimeOffsetSecs  NOTIFY timeOffsetSecsChanged)
    Q_PROPERTY(double            toleranceSecs   READ toleranceSecs   WRITE setToleranceSecs   NOTIFY toleranceSecsChanged)
    Q_PROPERTY(bool              previewMode     READ previewMode     WRITE setPreviewMode     NOTIFY previewModeChanged)
    Q_PROPERTY(bool              recursiveScan   READ recursiveScan   WRITE setRecursiveScan   NOTIFY recursiveScanChanged)
    Q_PROPERTY(GeoTagImageModel* imageModel      READ imageModel                             CONSTANT)

public:
    explicit GeoTagController(QObject *parent = nullptr);
    ~GeoTagController() override;

    Q_INVOKABLE void startTagging();
    Q_INVOKABLE void cancelTagging();

    QString logFile() const { return _logFile; }
    QString imageDirectory() const { return _imageDirectory; }
    QString saveDirectory() const { return _saveDirectory; }
    double progress() const { return _progress; }
    bool inProgress() const { return _stage != Stage::Idle; }
    QString errorMessage() const { return _errorMessage; }
    int taggedCount() const { return _taggedCount; }
    int skippedCount() const { return _skippedCount; }
    int failedCount() const { return _failedCount; }
    double timeOffsetSecs() const { return _timeOffsetSecs; }
    double toleranceSecs() const { return _toleranceSecs; }
    bool previewMode() const { return _previewMode; }
    bool recursiveScan() const { return _recursiveScan; }
    GeoTagImageModel* imageModel() const { return _imageModel; }

    void setLogFile(const QString &file);
    void setImageDirectory(const QString &dir);
    void setSaveDirectory(const QString &dir);
    void setTimeOffsetSecs(double offset);
    void setToleranceSecs(double tolerance);
    void setPreviewMode(bool preview);
    void setRecursiveScan(bool recursive);

signals:
    void logFileChanged(const QString &logFile);
    void imageDirectoryChanged(const QString &imageDirectory);
    void saveDirectoryChanged(const QString &saveDirectory);
    void progressChanged(double progress);
    void inProgressChanged();
    void errorMessageChanged(const QString &errorMessage);
    void taggingCompleteChanged();
    void timeOffsetSecsChanged(double timeOffsetSecs);
    void toleranceSecsChanged(double toleranceSecs);
    void previewModeChanged(bool previewMode);
    void recursiveScanChanged(bool recursiveScan);

private:
    // Processing stages for async state machine
    enum class Stage {
        Idle,
        LoadingImages,
        ParsingExif,
        ParsingLogs,
        Calibrating,
        TaggingImages,
        Finished
    };

    static const char* stageName(Stage stage);

    struct ExifResult {
        qint64 timestamp = 0;
        QString errorMessage;
        bool success = false;
    };

    struct TagResult {
        int imageIndex = -1;
        QString fileName;
        QString errorMessage;
        QGeoCoordinate coordinate;
        bool success = false;
    };

    struct TagTask {
        int imageIndex = -1;
        QFileInfo imageInfo;
        GeoTagData geoTag;
        QString outputDir;
        bool previewMode = false;
    };

    // State machine control
    void _setErrorMessage(const QString &errorMsg);
    void _setProgress(double progress);
    void _transitionTo(Stage stage);
    void _finishWithError(const QString &errorMsg);
    void _finishSuccess();

    // Stage implementations
    void _startLoadImages();
    void _startParseExif();
    void _startParseLogs();
    void _startCalibrate();
    void _startTagImages();

    // Async stage handlers (slots for QFutureWatcher signals)
    void _onExifProgress(int value);
    void _onExifFinished();
    void _onTagProgress(int value);
    void _onTagFinished();

    // Synchronous helpers (called from stage implementations)
    bool _loadImages(QString &errorMsg);
    bool _parseLogs(QString &errorMsg);
    bool _calibrate(QString &errorMsg);
    bool _validateOutputDirectory(const QString &outputDir, QString &errorMsg);
    QList<TagTask> _buildTagTasks(const QString &outputDir, bool preview, QString &errorMsg);
    qint64 _estimateOutputSize() const;

    ExifResult _parseExifForImage(const QFileInfo &imageInfo);
    TagResult _tagImage(const TagTask &task);

    // Image buffer cache (thread-safe)
    QByteArray _readImageCached(const QString &path, QString *errorString = nullptr);
    void _evictUnmatchedImages();
    void _clearImageCache();

    // QML properties
    QString _logFile;
    QString _imageDirectory;
    QString _saveDirectory;
    QString _errorMessage;
    double _progress = 0.;
    int _taggedCount = 0;
    int _skippedCount = 0;
    int _failedCount = 0;
    double _timeOffsetSecs = 0.0;
    double _toleranceSecs = 2.0;
    bool _previewMode = false;
    bool _recursiveScan = false;

    // Processing state
    struct ProcessingState {
        QFileInfoList imageList;
        QList<qint64> imageTimestamps;
        QList<GeoTagData> triggerList;
        QList<int> imageIndices;
        QList<int> triggerIndices;
        int skippedCount = 0;
        int failedCount = 0;

        void clear() {
            imageList.clear();
            imageTimestamps.clear();
            triggerList.clear();
            imageIndices.clear();
            triggerIndices.clear();
            skippedCount = 0;
            failedCount = 0;
        }
    };

    // Async state machine
    Stage _stage = Stage::Idle;
    std::atomic<bool> _cancel{false};
    QElapsedTimer _totalTimer;
    QElapsedTimer _stageTimer;

    // Async watchers for parallel stages
    QFutureWatcher<ExifResult> _exifWatcher;
    QFutureWatcher<TagResult> _tagWatcher;

    ProcessingState _state;

    // Image model for QML display
    GeoTagImageModel *_imageModel = nullptr;

    // Image buffer cache to avoid reading files twice
    mutable QMutex _bufferMutex;
    QHash<QString, QByteArray> _imageBuffers;

    // Progress calculation constants
    static constexpr double kLoadImagesEnd = 20.0;
    static constexpr double kParseExifEnd = 40.0;
    static constexpr double kParseLogsEnd = 60.0;
    static constexpr double kCalibrateEnd = 80.0;
    static constexpr double kTagImagesEnd = 100.0;
};
