#include "GeoTagController.h"
#include "DataFlashParser.h"
#include "ExifParser.h"
#include "GeoTagImageModel.h"
#include "QGCFileHelper.h"
#include "QGCLoggingCategory.h"
#include "ULogParser.h"

#include <QtConcurrent/QtConcurrent>
#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QMimeDatabase>
#include <QtCore/QMultiMap>
#include <QtCore/QMutexLocker>
#include <QtCore/QSet>

QGC_LOGGING_CATEGORY(GeoTagControllerLog, "AnalyzeView.GeoTagController")

namespace {

// Supported image extensions for geotagging (case variations for QDir filtering)
const QStringList kImageExtensionFilters = {
    QStringLiteral("*.jpg"),  QStringLiteral("*.JPG"),
    QStringLiteral("*.jpeg"), QStringLiteral("*.JPEG"),
    QStringLiteral("*.tiff"), QStringLiteral("*.TIFF"),
    QStringLiteral("*.tif"),  QStringLiteral("*.TIF"),
    QStringLiteral("*.dng"),  QStringLiteral("*.DNG")
};

// Supported MIME types for verification
const QSet<QString> kSupportedMimeTypes = {
    QStringLiteral("image/jpeg"),
    QStringLiteral("image/tiff"),
    QStringLiteral("image/x-adobe-dng"),
    QStringLiteral("image/x-dcraw")
};

/// Check if a MIME type is supported for geotagging
bool isSupportedImageMimeType(const QMimeType &mimeType)
{
    if (kSupportedMimeTypes.contains(mimeType.name())) {
        return true;
    }

    // Check parent MIME types (e.g., image/x-adobe-dng inherits from image/tiff)
    for (const QString &parent : mimeType.parentMimeTypes()) {
        if (kSupportedMimeTypes.contains(parent)) {
            return true;
        }
    }

    // Accept files if MIME detection returns generic type (extension already matched)
    if (mimeType.name() == QStringLiteral("application/octet-stream") ||
        mimeType.name().startsWith(QStringLiteral("application/"))) {
        return true;
    }

    return false;
}

/// Calculate progress within a stage (linear interpolation)
double calculateStageProgress(double stageStart, double stageEnd, int completed, int total)
{
    if (total <= 0) {
        return stageStart;
    }
    return stageStart + (stageEnd - stageStart) * completed / total;
}

} // namespace

// ============================================================================
// Stage name helper for debug logging
// ============================================================================

const char* GeoTagController::stageName(Stage stage)
{
    switch (stage) {
    case Stage::Idle:          return "Idle";
    case Stage::LoadingImages: return "LoadingImages";
    case Stage::ParsingExif:   return "ParsingExif";
    case Stage::ParsingLogs:   return "ParsingLogs";
    case Stage::Calibrating:   return "Calibrating";
    case Stage::TaggingImages: return "TaggingImages";
    case Stage::Finished:      return "Finished";
    }
    return "Unknown";
}

// ============================================================================
// GeoTagCalibrator implementation
// ============================================================================

CalibrationResult GeoTagCalibrator::calibrate(const QList<qint64> &imageTimestamps,
                                               const QList<GeoTagData> &triggers,
                                               qint64 timeOffsetSecs,
                                               qint64 toleranceSecs)
{
    CalibrationResult result;

    if (triggers.isEmpty() || imageTimestamps.isEmpty()) {
        return result;
    }

    // Use offset-from-end approach to handle different time bases (EXIF vs GPS/boot time).
    // Both sequences are normalized to "seconds before end of sequence", making them comparable.
    const qint64 lastImageTimestamp = imageTimestamps.last() + timeOffsetSecs;
    const qint64 lastTriggerTimestamp = triggers.last().timestamp;

    // Build image offset map for efficient searching (QMultiMap is sorted by key)
    // Key: offset from end (lastImageTimestamp - adjustedTimestamp), Value: image index
    // Skip images with timestamp 0 (failed EXIF parsing)
    QMultiMap<qint64, int> imageOffsets;
    QSet<int> invalidImageIndices;
    for (int i = 0; i < imageTimestamps.size(); ++i) {
        if (imageTimestamps[i] == 0) {
            invalidImageIndices.insert(i);
            continue;
        }
        const qint64 adjustedTimestamp = imageTimestamps[i] + timeOffsetSecs;
        const qint64 offset = lastImageTimestamp - adjustedTimestamp;
        imageOffsets.insert(offset, i);
    }

    // Track which images have been matched to avoid duplicates
    QSet<int> usedImageIndices;

    for (int triggerIdx = 0; triggerIdx < triggers.size(); ++triggerIdx) {
        const GeoTagData &trigger = triggers[triggerIdx];
        if (!trigger.isValid() || !trigger.coordinate.isValid()) {
            ++result.skippedTriggers;
            continue;
        }

        const qint64 triggerOffset = lastTriggerTimestamp - trigger.timestamp;

        // Find closest image within tolerance using binary search
        int bestImageIdx = -1;
        qint64 bestDiff = toleranceSecs + 1;

        // Search range: [triggerOffset - tolerance, triggerOffset + tolerance]
        auto it = imageOffsets.lowerBound(triggerOffset - toleranceSecs);

        while (it != imageOffsets.end() && it.key() <= triggerOffset + toleranceSecs) {
            const qint64 diff = qAbs(it.key() - triggerOffset);
            if (diff <= toleranceSecs && diff < bestDiff && !usedImageIndices.contains(it.value())) {
                bestDiff = diff;
                bestImageIdx = it.value();
            }
            ++it;
        }

        if (bestImageIdx >= 0) {
            result.imageIndices.append(bestImageIdx);
            result.triggerIndices.append(triggerIdx);
            usedImageIndices.insert(bestImageIdx);
        }
    }

    // Collect unmatched image indices
    for (int i = 0; i < imageTimestamps.size(); ++i) {
        if (!usedImageIndices.contains(i)) {
            result.unmatchedImages.append(i);
        }
    }

    return result;
}

// ============================================================================
// GeoTagController implementation
// ============================================================================

GeoTagController::GeoTagController(QObject *parent)
    : QObject(parent)
    , _imageModel(new GeoTagImageModel(this))
{
    qCDebug(GeoTagControllerLog) << this;

    // Connect EXIF parsing watcher signals
    (void) connect(&_exifWatcher, &QFutureWatcher<ExifResult>::progressValueChanged,
                   this, &GeoTagController::_onExifProgress);
    (void) connect(&_exifWatcher, &QFutureWatcher<ExifResult>::finished,
                   this, &GeoTagController::_onExifFinished);

    // Connect tagging watcher signals
    (void) connect(&_tagWatcher, &QFutureWatcher<TagResult>::progressValueChanged,
                   this, &GeoTagController::_onTagProgress);
    (void) connect(&_tagWatcher, &QFutureWatcher<TagResult>::finished,
                   this, &GeoTagController::_onTagFinished);
}

GeoTagController::~GeoTagController()
{
    qCDebug(GeoTagControllerLog) << this;

    // Cancel and wait for any running operations
    _cancel = true;
    _exifWatcher.waitForFinished();
    _tagWatcher.waitForFinished();
}

void GeoTagController::setLogFile(const QString &file)
{
    const QString path = QGCFileHelper::toLocalPath(file);

    if (path.isEmpty()) {
        _setErrorMessage(tr("Empty Filename."));
        return;
    }

    const QFileInfo logFileInfo(path);
    if (!logFileInfo.exists() || !logFileInfo.isFile()) {
        _setErrorMessage(tr("Invalid Filename."));
        return;
    }

    if (_logFile != path) {
        _logFile = path;
        emit logFileChanged(_logFile);
    }

    _setErrorMessage(QString());
}

void GeoTagController::setImageDirectory(const QString &dir)
{
    const QString path = QGCFileHelper::toLocalPath(dir);

    if (path.isEmpty()) {
        _setErrorMessage(tr("Invalid Directory."));
        return;
    }

    const QFileInfo imageDirectoryInfo(path);
    if (!imageDirectoryInfo.exists() || !imageDirectoryInfo.isDir()) {
        _setErrorMessage(tr("Invalid Directory."));
        return;
    }

    if (_imageDirectory != path) {
        _imageDirectory = path;
        emit imageDirectoryChanged(_imageDirectory);
    }

    if (_saveDirectory.isEmpty()) {
        const QString taggedPath = QGCFileHelper::joinPath(_imageDirectory, QStringLiteral("TAGGED"));
        if (QGCFileHelper::exists(taggedPath)) {
            _setErrorMessage(tr("Images have already been tagged. Existing images will be removed."));
            return;
        }
    }

    _setErrorMessage(QString());
}

void GeoTagController::setSaveDirectory(const QString &dir)
{
    const QString path = QGCFileHelper::toLocalPath(dir);

    if (path.isEmpty()) {
        _setErrorMessage(tr("Invalid Directory."));
        return;
    }

    const QFileInfo saveDirectoryInfo(path);
    if (!saveDirectoryInfo.exists() || !saveDirectoryInfo.isDir()) {
        _setErrorMessage(tr("Invalid Directory."));
        return;
    }

    if (_saveDirectory != path) {
        _saveDirectory = path;
        emit saveDirectoryChanged(_saveDirectory);
    }

    QDir saveDir(path);
    saveDir.setFilter(QDir::Files | QDir::Readable | QDir::NoSymLinks | QDir::Writable);
    saveDir.setNameFilters(kImageExtensionFilters);

    if (!saveDir.entryList().isEmpty()) {
        _setErrorMessage(tr("The save folder already contains images."));
        return;
    }

    _setErrorMessage(QString());
}

void GeoTagController::setTimeOffsetSecs(double offset)
{
    if (!qFuzzyCompare(_timeOffsetSecs, offset)) {
        _timeOffsetSecs = offset;
        emit timeOffsetSecsChanged(_timeOffsetSecs);
    }
}

void GeoTagController::setToleranceSecs(double tolerance)
{
    // Clamp to reasonable range: 0.1 to 60 seconds
    tolerance = qBound(0.1, tolerance, 60.0);
    if (!qFuzzyCompare(_toleranceSecs, tolerance)) {
        _toleranceSecs = tolerance;
        emit toleranceSecsChanged(_toleranceSecs);
    }
}

void GeoTagController::setPreviewMode(bool preview)
{
    if (_previewMode != preview) {
        _previewMode = preview;
        emit previewModeChanged(_previewMode);
    }
}

void GeoTagController::setRecursiveScan(bool recursive)
{
    if (_recursiveScan != recursive) {
        _recursiveScan = recursive;
        emit recursiveScanChanged(_recursiveScan);
    }
}

void GeoTagController::_setErrorMessage(const QString &errorMsg)
{
    if (errorMsg != _errorMessage) {
        _errorMessage = errorMsg;
        emit errorMessageChanged(_errorMessage);
    }
}

void GeoTagController::_setProgress(double progress)
{
    if (progress != _progress) {
        _progress = progress;
        emit progressChanged(_progress);
    }
}

// ============================================================================
// State machine control
// ============================================================================

void GeoTagController::startTagging()
{
    if (inProgress()) {
        qCWarning(GeoTagControllerLog) << "Tagging already in progress";
        return;
    }

    _setErrorMessage(QString());
    _setProgress(0.);
    _taggedCount = 0;
    _skippedCount = 0;
    _failedCount = 0;
    emit taggingCompleteChanged();

    if (_imageDirectory.isEmpty()) {
        _setErrorMessage(tr("Please select an image directory."));
        return;
    }

    if (_logFile.isEmpty()) {
        _setErrorMessage(tr("Please select a log file."));
        return;
    }

    if (!QGCFileHelper::exists(_imageDirectory)) {
        _setErrorMessage(tr("Cannot find the image directory."));
        return;
    }

    if (!_saveDirectory.isEmpty() && !QGCFileHelper::exists(_saveDirectory)) {
        _setErrorMessage(tr("Cannot find the save directory."));
        return;
    }

    // Clear previous state
    _state.clear();
    _clearImageCache();
    _cancel = false;

    // Start timing and begin processing
    _totalTimer.start();
    _transitionTo(Stage::LoadingImages);
}

void GeoTagController::cancelTagging()
{
    if (!inProgress()) {
        return;
    }

    _cancel = true;

    // Cancel any running futures
    if (_exifWatcher.isRunning()) {
        _exifWatcher.cancel();
    }
    if (_tagWatcher.isRunning()) {
        _tagWatcher.cancel();
    }

    _finishWithError(tr("Tagging cancelled"));
}

void GeoTagController::_transitionTo(Stage stage)
{
    if (_cancel && stage != Stage::Idle && stage != Stage::Finished) {
        _finishWithError(tr("Tagging cancelled"));
        return;
    }

    _stage = stage;
    _stageTimer.start();

    qCDebug(GeoTagControllerLog) << "Transitioning to stage:" << stageName(stage);

    switch (stage) {
    case Stage::Idle:
        break;
    case Stage::LoadingImages:
        emit inProgressChanged();
        _startLoadImages();
        break;
    case Stage::ParsingExif:
        _startParseExif();
        break;
    case Stage::ParsingLogs:
        _startParseLogs();
        break;
    case Stage::Calibrating:
        _startCalibrate();
        break;
    case Stage::TaggingImages:
        _startTagImages();
        break;
    case Stage::Finished:
        _finishSuccess();
        break;
    }
}

void GeoTagController::_finishWithError(const QString &errorMsg)
{
    qCDebug(GeoTagControllerLog) << "Finishing with error:" << errorMsg;
    qCDebug(GeoTagControllerLog) << "Total processing time:" << _totalTimer.elapsed() << "ms";

    _clearImageCache();
    _stage = Stage::Idle;
    _setErrorMessage(errorMsg);
    emit inProgressChanged();
}

void GeoTagController::_finishSuccess()
{
    qCDebug(GeoTagControllerLog) << "Finishing successfully";
    qCDebug(GeoTagControllerLog) << "Total processing time:" << _totalTimer.elapsed() << "ms";

    _clearImageCache();
    _stage = Stage::Idle;

    const auto matchedCount = std::min(_state.imageIndices.count(), _state.triggerIndices.count());
    _taggedCount = static_cast<int>(matchedCount) - _state.failedCount;
    _skippedCount = _state.skippedCount;
    _failedCount = _state.failedCount;

    _setProgress(100.0);
    emit taggingCompleteChanged();
    emit inProgressChanged();

    if (_state.failedCount > 0) {
        _setErrorMessage(tr("%1 image(s) failed to tag").arg(_state.failedCount));
    }
}

// ============================================================================
// Stage implementations
// ============================================================================

void GeoTagController::_startLoadImages()
{
    QString errorMsg;
    if (!_loadImages(errorMsg)) {
        _finishWithError(errorMsg);
        return;
    }

    qCDebug(GeoTagControllerLog) << "Stage: loadImages took" << _stageTimer.elapsed() << "ms";
    _setProgress(kLoadImagesEnd);
    _transitionTo(Stage::ParsingExif);
}

void GeoTagController::_startParseExif()
{
    // Launch parallel EXIF parsing
    QFuture<ExifResult> future = QtConcurrent::mapped(_state.imageList,
        [this](const QFileInfo &info) { return _parseExifForImage(info); });

    _exifWatcher.setFuture(future);
    // Progress and completion handled by _onExifProgress and _onExifFinished
}

void GeoTagController::_onExifProgress(int value)
{
    const int total = _state.imageList.size();
    const double progress = calculateStageProgress(kLoadImagesEnd, kParseExifEnd, value, total);
    _setProgress(progress);
}

void GeoTagController::_onExifFinished()
{
    qCDebug(GeoTagControllerLog) << "Stage: parseExif took" << _stageTimer.elapsed() << "ms";

    if (_cancel) {
        _finishWithError(tr("Tagging cancelled"));
        return;
    }

    // Process results - skip images that fail EXIF parsing instead of failing entirely
    _state.imageTimestamps.clear();
    _state.imageTimestamps.reserve(_state.imageList.size());

    const QList<ExifResult> results = _exifWatcher.future().results();
    QList<int> failedIndices;

    for (int i = 0; i < results.size(); ++i) {
        const ExifResult &result = results[i];
        if (result.success) {
            _state.imageTimestamps.append(result.timestamp);
        } else {
            // Mark as skipped and use placeholder timestamp
            failedIndices.append(i);
            _state.imageTimestamps.append(0);
            qCWarning(GeoTagControllerLog) << "Skipping image with EXIF error:" << result.errorMessage;
        }
    }

    // Mark failed images in the model
    for (int idx : failedIndices) {
        if (idx < _state.imageList.size()) {
            _imageModel->setStatus(idx, GeoTagImageModel::Skipped,
                                   tr("Could not read EXIF timestamp"));
        }
        ++_state.skippedCount;
    }

    if (failedIndices.size() == results.size()) {
        _finishWithError(tr("Could not read EXIF data from any images"));
        return;
    }

    if (!failedIndices.isEmpty()) {
        qCDebug(GeoTagControllerLog) << "Skipped" << failedIndices.size() << "images with EXIF errors";
    }

    _setProgress(kParseExifEnd);
    _transitionTo(Stage::ParsingLogs);
}

void GeoTagController::_startParseLogs()
{
    QString errorMsg;
    if (!_parseLogs(errorMsg)) {
        _finishWithError(errorMsg);
        return;
    }

    qCDebug(GeoTagControllerLog) << "Stage: parseLogs took" << _stageTimer.elapsed() << "ms";
    _setProgress(kParseLogsEnd);
    _transitionTo(Stage::Calibrating);
}

void GeoTagController::_startCalibrate()
{
    QString errorMsg;
    if (!_calibrate(errorMsg)) {
        _finishWithError(errorMsg);
        return;
    }

    // Evict unmatched images from cache to save memory
    _evictUnmatchedImages();

    qCDebug(GeoTagControllerLog) << "Stage: calibrate took" << _stageTimer.elapsed() << "ms";
    _setProgress(kCalibrateEnd);
    _transitionTo(Stage::TaggingImages);
}

void GeoTagController::_startTagImages()
{
    const bool preview = _previewMode;
    const QString outputDir = _saveDirectory.isEmpty()
        ? QGCFileHelper::joinPath(_imageDirectory, QStringLiteral("TAGGED"))
        : _saveDirectory;

    QString errorMsg;

    // Validate output directory (skip in preview mode)
    if (!preview && !_validateOutputDirectory(outputDir, errorMsg)) {
        _finishWithError(errorMsg);
        return;
    }

    // Build task list (local - copied into future)
    QList<TagTask> tagTasks = _buildTagTasks(outputDir, preview, errorMsg);
    if (tagTasks.isEmpty()) {
        if (!errorMsg.isEmpty()) {
            _finishWithError(errorMsg);
        } else {
            _transitionTo(Stage::Finished);  // No tasks is valid (no matches)
        }
        return;
    }

    // Mark images being processed
    for (const TagTask &task : tagTasks) {
        _imageModel->setStatus(task.imageIndex, GeoTagImageModel::Processing);
    }

    // Launch parallel tagging (tagTasks copied into future)
    QFuture<TagResult> future = QtConcurrent::mapped(tagTasks,
        [this](const TagTask &task) { return _tagImage(task); });

    _tagWatcher.setFuture(future);
    // Progress and completion handled by _onTagProgress and _onTagFinished
}

void GeoTagController::_onTagProgress(int value)
{
    const int total = _tagWatcher.progressMaximum();
    const double progress = calculateStageProgress(kCalibrateEnd, kTagImagesEnd, value, total);
    _setProgress(progress);
}

void GeoTagController::_onTagFinished()
{
    qCDebug(GeoTagControllerLog) << "Stage: tagImages took" << _stageTimer.elapsed() << "ms";

    if (_cancel) {
        _finishWithError(tr("Tagging cancelled"));
        return;
    }

    // Process results
    const QList<TagResult> results = _tagWatcher.future().results();

    for (const TagResult &result : results) {
        if (result.success) {
            _imageModel->setStatus(result.imageIndex, GeoTagImageModel::Tagged);
            _imageModel->setCoordinate(result.imageIndex, result.coordinate);
        } else {
            _imageModel->setStatus(result.imageIndex, GeoTagImageModel::Failed, result.errorMessage);
            ++_state.failedCount;
            qCWarning(GeoTagControllerLog) << "Failed to tag image:" << result.fileName << "-" << result.errorMessage;
        }
    }

    // Only fail completely if ALL images failed
    if (_state.failedCount > 0 && _state.failedCount == results.count()) {
        _finishWithError(tr("All images failed to tag"));
        return;
    }

    _transitionTo(Stage::Finished);
}

// ============================================================================
// Synchronous helper methods
// ============================================================================

bool GeoTagController::_loadImages(QString &errorMsg)
{
    _state.imageList.clear();

    // Collect candidate files (optionally recursive)
    QFileInfoList candidateFiles;
    QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags;
    if (_recursiveScan) {
        flags = QDirIterator::Subdirectories;
    }

    QDirIterator it(_imageDirectory, kImageExtensionFilters,
                    QDir::Files | QDir::Readable | QDir::NoSymLinks, flags);
    while (it.hasNext()) {
        it.next();
        candidateFiles.append(it.fileInfo());
    }

    // Sort by name for consistent ordering
    std::sort(candidateFiles.begin(), candidateFiles.end(),
              [](const QFileInfo &a, const QFileInfo &b) { return a.fileName() < b.fileName(); });

    if (candidateFiles.isEmpty()) {
        errorMsg = tr("The image directory doesn't contain supported images. Supported formats: JPEG, TIFF, DNG");
        return false;
    }

    // Verify MIME types to filter out misnamed files
    static const QMimeDatabase mimeDb;
    for (const QFileInfo &fileInfo : candidateFiles) {
        const QMimeType mimeType = mimeDb.mimeTypeForFile(fileInfo);
        if (isSupportedImageMimeType(mimeType)) {
            _state.imageList.append(fileInfo);
        } else {
            qCDebug(GeoTagControllerLog) << "Skipping misnamed file:" << fileInfo.fileName()
                                          << "MIME:" << mimeType.name();
        }
    }

    if (_state.imageList.isEmpty()) {
        errorMsg = tr("The image directory doesn't contain supported images. Supported formats: JPEG, TIFF, DNG");
        return false;
    }

    // Populate image model
    _imageModel->clear();
    for (const QFileInfo &info : _state.imageList) {
        _imageModel->addImage(info.absoluteFilePath());
    }

    qCDebug(GeoTagControllerLog) << "Found" << _state.imageList.count() << "images"
                                  << (_recursiveScan ? "(recursive)" : "");

    return true;
}

GeoTagController::ExifResult GeoTagController::_parseExifForImage(const QFileInfo &imageInfo)
{
    ExifResult result;
    result.success = false;

    if (_cancel) {
        result.errorMessage = tr("Tagging cancelled");
        return result;
    }

    QString errorString;
    const QByteArray imageBuffer = _readImageCached(imageInfo.absoluteFilePath(), &errorString);
    if (imageBuffer.isEmpty()) {
        result.errorMessage = tr("Geotagging failed. Couldn't open image: %1").arg(imageInfo.fileName());
        return result;
    }

    const QDateTime imageTime = ExifParser::readTime(imageBuffer);
    if (!imageTime.isValid()) {
        result.errorMessage = tr("Geotagging failed. Couldn't extract time from image: %1").arg(imageInfo.fileName());
        return result;
    }

    result.timestamp = imageTime.toSecsSinceEpoch();
    result.success = true;
    return result;
}

bool GeoTagController::_parseLogs(QString &errorMsg)
{
    _state.triggerList.clear();

    QFile logFile(_logFile);
    if (!logFile.open(QIODevice::ReadOnly)) {
        errorMsg = tr("Geotagging failed. Couldn't open log file.");
        return false;
    }

    const qint64 fileSize = logFile.size();
    if (fileSize == 0) {
        errorMsg = tr("Geotagging failed. Log file is empty.");
        return false;
    }

    // Memory-map the file for efficient parsing of large logs
    const uchar *mappedData = logFile.map(0, fileSize);
    const char *data = nullptr;
    qint64 dataSize = fileSize;
    QByteArray fallbackBuffer;

    if (mappedData) {
        data = reinterpret_cast<const char*>(mappedData);
        qCDebug(GeoTagControllerLog) << "Memory-mapped log file:" << fileSize << "bytes";
    } else {
        // Fallback to reading into memory if mapping fails
        qCDebug(GeoTagControllerLog) << "Memory mapping failed, reading file into memory";
        fallbackBuffer = logFile.readAll();
        if (fallbackBuffer.isEmpty()) {
            errorMsg = tr("Geotagging failed. Couldn't read log file.");
            return false;
        }
        data = fallbackBuffer.constData();
        dataSize = fallbackBuffer.size();
    }

    // Auto-detect log format based on file extension
    const QString logFileLower = _logFile.toLower();
    bool parseSuccess = false;
    QString errorString;

    if (logFileLower.endsWith(QStringLiteral(".bin"))) {
        qCDebug(GeoTagControllerLog) << "Parsing DataFlash log:" << _logFile;
        parseSuccess = DataFlashParser::getTagsFromLog(data, dataSize, _state.triggerList, errorString);
    } else if (logFileLower.endsWith(QStringLiteral(".ulg"))) {
        qCDebug(GeoTagControllerLog) << "Parsing ULog:" << _logFile;
        parseSuccess = ULogParser::getTagsFromLog(data, dataSize, _state.triggerList, errorString);
    } else {
        // Try ULog first (PX4), then DataFlash (ArduPilot) as fallback
        qCDebug(GeoTagControllerLog) << "Unknown extension, trying ULog parser first";
        parseSuccess = ULogParser::getTagsFromLog(data, dataSize, _state.triggerList, errorString);
        if (!parseSuccess) {
            qCDebug(GeoTagControllerLog) << "ULog failed, trying DataFlash parser";
            errorString.clear();
            parseSuccess = DataFlashParser::getTagsFromLog(data, dataSize, _state.triggerList, errorString);
        }
    }

    // Unmap the file (if mapped)
    if (mappedData) {
        logFile.unmap(const_cast<uchar*>(mappedData));
    }

    if (!parseSuccess) {
        errorMsg = errorString.isEmpty() ? tr("Log parsing failed") : errorString;
        return false;
    }

    qCDebug(GeoTagControllerLog) << "Found" << _state.triggerList.count() << "camera capture events";

    if (_state.imageList.count() > _state.triggerList.count()) {
        qCDebug(GeoTagControllerLog) << "Detected missing feedback packets:"
                                      << (_state.imageList.count() - _state.triggerList.count()) << "images without triggers";
    } else if (_state.imageList.count() < _state.triggerList.count()) {
        qCDebug(GeoTagControllerLog) << "Detected missing image frames:"
                                      << (_state.triggerList.count() - _state.imageList.count()) << "triggers without images";
    }

    return true;
}

bool GeoTagController::_calibrate(QString &errorMsg)
{
    _state.imageIndices.clear();
    _state.triggerIndices.clear();
    _state.skippedCount = 0;

    if (_state.triggerList.isEmpty() || _state.imageTimestamps.isEmpty()) {
        errorMsg = tr("Calibration failed: No triggers or images available.");
        return false;
    }

    // Run the matching algorithm
    const qint64 timeOffsetSec = static_cast<qint64>(_timeOffsetSecs);
    const qint64 toleranceSec = static_cast<qint64>(_toleranceSecs);
    const CalibrationResult calibration = GeoTagCalibrator::calibrate(
        _state.imageTimestamps, _state.triggerList, timeOffsetSec, toleranceSec);

    // Store results in processing state
    _state.imageIndices = calibration.imageIndices;
    _state.triggerIndices = calibration.triggerIndices;
    _state.skippedCount = calibration.skippedTriggers;

    qCDebug(GeoTagControllerLog) << "Calibration complete:"
                                  << _state.imageIndices.count() << "matched,"
                                  << _state.skippedCount << "skipped (invalid),"
                                  << calibration.unmatchedImages.count() << "unmatched";

    // Mark unmatched images as Skipped in the model
    if (!calibration.unmatchedImages.isEmpty()) {
        for (int idx : calibration.unmatchedImages) {
            _imageModel->setStatus(idx, GeoTagImageModel::Skipped, tr("No matching trigger"));
        }
        _state.skippedCount += calibration.unmatchedImages.count();
    }

    if (_state.imageIndices.isEmpty()) {
        errorMsg = tr("Calibration failed: No matching triggers found for images.");
        return false;
    }

    return true;
}

bool GeoTagController::_validateOutputDirectory(const QString &outputDir, QString &errorMsg)
{
    const qint64 requiredSpace = _estimateOutputSize();
    if (!QGCFileHelper::hasSufficientDiskSpace(outputDir, requiredSpace)) {
        errorMsg = tr("Geotagging failed. Insufficient disk space. Need approximately %1 MB.")
                   .arg(requiredSpace / (1024 * 1024));
        return false;
    }

    if (!QGCFileHelper::ensureDirectoryExists(outputDir)) {
        errorMsg = tr("Geotagging failed. Couldn't create output directory: %1").arg(outputDir);
        return false;
    }

    return true;
}

QList<GeoTagController::TagTask> GeoTagController::_buildTagTasks(const QString &outputDir, bool preview, QString &errorMsg)
{
    const qsizetype maxIndex = std::min(_state.imageIndices.count(), _state.triggerIndices.count());

    QList<TagTask> tasks;
    tasks.reserve(maxIndex);

    for (qsizetype i = 0; i < maxIndex; ++i) {
        const int imageIndex = _state.imageIndices[i];
        const int triggerIndex = _state.triggerIndices[i];

        if (imageIndex >= _state.imageList.count()) {
            errorMsg = tr("Geotagging failed. Requesting image #%1, but only %2 images present.")
                       .arg(imageIndex).arg(_state.imageList.count());
            return {};
        }

        if (triggerIndex >= _state.triggerList.count()) {
            errorMsg = tr("Geotagging failed. Requesting trigger #%1, but only %2 triggers present.")
                       .arg(triggerIndex).arg(_state.triggerList.count());
            return {};
        }

        TagTask task;
        task.imageIndex = imageIndex;
        task.imageInfo = _state.imageList.at(imageIndex);
        task.geoTag = _state.triggerList[triggerIndex];
        task.outputDir = outputDir;
        task.previewMode = preview;
        tasks.append(task);
    }

    return tasks;
}

qint64 GeoTagController::_estimateOutputSize() const
{
    qint64 totalSize = 0;
    const qsizetype maxIndex = std::min(_state.imageIndices.count(), _state.triggerIndices.count());

    for (qsizetype i = 0; i < maxIndex; ++i) {
        const int imageIndex = _state.imageIndices[i];
        if (imageIndex < _state.imageList.count()) {
            totalSize += _state.imageList.at(imageIndex).size();
        }
    }

    // Add 10% margin for EXIF data additions
    return static_cast<qint64>(totalSize * 1.1);
}

GeoTagController::TagResult GeoTagController::_tagImage(const TagTask &task)
{
    TagResult result;
    result.imageIndex = task.imageIndex;
    result.fileName = task.imageInfo.fileName();
    result.success = false;

    if (_cancel) {
        result.errorMessage = tr("Tagging cancelled");
        return result;
    }

    QString errorString;
    QByteArray imageBuffer = _readImageCached(task.imageInfo.absoluteFilePath(), &errorString);
    if (imageBuffer.isEmpty()) {
        result.errorMessage = tr("Geotagging failed. Couldn't open image: %1").arg(result.fileName);
        return result;
    }

    // In preview mode, skip actual EXIF modification and file writing
    if (!task.previewMode) {
        // Detach from cache before modifying (copy-on-write)
        imageBuffer.detach();

        if (!ExifParser::write(imageBuffer, task.geoTag)) {
            result.errorMessage = tr("Geotagging failed. Couldn't write EXIF to image: %1").arg(result.fileName);
            return result;
        }

        const QString outputPath = QGCFileHelper::joinPath(task.outputDir, result.fileName);
        if (!QGCFileHelper::atomicWrite(outputPath, imageBuffer)) {
            result.errorMessage = tr("Geotagging failed. Couldn't save image: %1").arg(outputPath);
            return result;
        }
    }

    result.coordinate = task.geoTag.coordinate;
    result.success = true;
    return result;
}

// ============================================================================
// Image cache methods
// ============================================================================

QByteArray GeoTagController::_readImageCached(const QString &path, QString *errorString)
{
    {
        QMutexLocker locker(&_bufferMutex);
        auto it = _imageBuffers.constFind(path);
        if (it != _imageBuffers.constEnd()) {
            return it.value();
        }
    }

    // Read from disk (outside lock to allow parallel reads)
    QString error;
    QByteArray data = QGCFileHelper::readFile(path, &error);

    if (data.isEmpty()) {
        if (errorString) {
            *errorString = error;
        }
        return {};
    }

    // Cache the data
    {
        QMutexLocker locker(&_bufferMutex);
        _imageBuffers.insert(path, data);
    }

    return data;
}

void GeoTagController::_evictUnmatchedImages()
{
    QSet<QString> matchedPaths;
    for (int idx : _state.imageIndices) {
        if (idx < _state.imageList.count()) {
            matchedPaths.insert(_state.imageList[idx].absoluteFilePath());
        }
    }

    QMutexLocker locker(&_bufferMutex);
    auto it = _imageBuffers.begin();
    while (it != _imageBuffers.end()) {
        if (!matchedPaths.contains(it.key())) {
            it = _imageBuffers.erase(it);
        } else {
            ++it;
        }
    }

    qCDebug(GeoTagControllerLog) << "Evicted unmatched images, cache size:" << _imageBuffers.count();
}

void GeoTagController::_clearImageCache()
{
    QMutexLocker locker(&_bufferMutex);
    _imageBuffers.clear();
}
