#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(LogDownloadControllerLog)

struct LogDownloadData;
class LogDownloadStateMachine;
class QGCLogEntry;
class QmlObjectListModel;
class Vehicle;
class LogDownloadTest;

class LogDownloadController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_MOC_INCLUDE("Vehicle.h")
    Q_MOC_INCLUDE("QmlObjectListModel.h")
    Q_PROPERTY(QmlObjectListModel *model          READ _getModel            CONSTANT)
    Q_PROPERTY(bool               requestingList  READ _getRequestingList   NOTIFY requestingListChanged)
    Q_PROPERTY(bool               downloadingLogs READ _getDownloadingLogs  NOTIFY downloadingLogsChanged)
    Q_PROPERTY(bool               compressLogs    READ compressLogs         WRITE setCompressLogs NOTIFY compressLogsChanged)
    Q_PROPERTY(bool               compressing     READ compressing          NOTIFY compressingChanged)
    Q_PROPERTY(float              compressionProgress READ compressionProgress NOTIFY compressionProgressChanged)

    friend class LogDownloadTest;
    friend class LogDownloadStateMachine;
    friend class LogDownloadStateMachineTest;

public:
    explicit LogDownloadController(QObject *parent = nullptr);
    ~LogDownloadController();

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void download(const QString &path = QString());
    Q_INVOKABLE void eraseAll();
    Q_INVOKABLE void cancel();

    bool compressLogs() const { return _compressLogs; }
    void setCompressLogs(bool compress);
    bool compressing() const { return _compressing; }
    float compressionProgress() const { return _compressionProgress; }

    /// Compress a single log file
    Q_INVOKABLE bool compressLogFile(const QString &logPath);

    /// Cancel compression
    Q_INVOKABLE void cancelCompression();

signals:
    void requestingListChanged();
    void downloadingLogsChanged();
    void selectionChanged();
    void compressLogsChanged();
    void compressingChanged();
    void compressionProgressChanged();
    void compressionComplete(const QString &outputPath, const QString &error);

private slots:
    void _setActiveVehicle(Vehicle *vehicle);
    void _logEntry(uint32_t time_utc, uint32_t size, uint16_t id, uint16_t num_logs, uint16_t last_log_num);
    void _logData(uint32_t ofs, uint16_t id, uint8_t count, const uint8_t *data);
    void _handleCompressionProgress(qreal progress);
    void _handleCompressionFinished(bool success);

private:
    QmlObjectListModel *_getModel() const { return _logEntriesModel; }
    bool _getRequestingList() const;
    bool _getDownloadingLogs() const;

    void _resetSelection(bool canceled = false);
    void _updateDataRate();

    QGCLogEntry *_getNextSelected() const;

    QmlObjectListModel *_logEntriesModel = nullptr;
    std::unique_ptr<LogDownloadData> _downloadData;
    QString _downloadPath;
    Vehicle *_vehicle = nullptr;
    LogDownloadStateMachine *_stateMachine = nullptr;
    bool _compressLogs = false;
    bool _compressing = false;
    float _compressionProgress = 0.0F;

    static constexpr uint32_t kGUIRateMs = 17; ///< 1000ms / 60fps
};
