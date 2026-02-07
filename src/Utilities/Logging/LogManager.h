#pragma once

#include "LogFormatter.h"

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtQmlIntegration/QtQmlIntegration>

#include <atomic>

class LogModel;
class LogRemoteSink;
class QGCFileWriter;
struct LogEntry;

/// Central logging coordinator for QGroundControl.
/// Captures Qt log messages and routes them to in-memory model, disk, and remote.
class LogManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_MOC_INCLUDE("LogModel.h")

    // Core properties
    Q_PROPERTY(LogModel* model READ model CONSTANT)
    Q_PROPERTY(bool hasError READ hasError NOTIFY errorChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(QString lastRemoteError READ lastRemoteError NOTIFY lastRemoteErrorChanged)
    Q_PROPERTY(QString lastTlsError READ lastTlsError NOTIFY lastTlsErrorChanged)
    Q_PROPERTY(qint64 remoteBytesSent READ remoteBytesSent NOTIFY remoteBytesSentChanged)

    // Disk logging
    Q_PROPERTY(bool diskLoggingEnabled READ isDiskLoggingEnabled WRITE setDiskLoggingEnabled NOTIFY diskLoggingEnabledChanged)

    // Remote logging - basic
    Q_PROPERTY(bool remoteLoggingEnabled READ isRemoteLoggingEnabled WRITE setRemoteLoggingEnabled NOTIFY remoteLoggingEnabledChanged)
    Q_PROPERTY(QString remoteEndpoint READ remoteEndpoint WRITE setRemoteEndpoint NOTIFY remoteEndpointChanged)
    Q_PROPERTY(QString remoteVehicleId READ remoteVehicleId WRITE setRemoteVehicleId NOTIFY remoteVehicleIdChanged)

    // Remote logging - protocol
    Q_PROPERTY(RemoteProtocol remoteProtocol READ remoteProtocol WRITE setRemoteProtocol NOTIFY remoteProtocolChanged)
    Q_PROPERTY(bool remoteTcpConnected READ isRemoteTcpConnected NOTIFY remoteTcpConnectedChanged)

    // Remote logging - TLS
    Q_PROPERTY(bool remoteTlsEnabled READ isRemoteTlsEnabled WRITE setRemoteTlsEnabled NOTIFY remoteTlsEnabledChanged)
    Q_PROPERTY(bool remoteTlsVerifyPeer READ remoteTlsVerifyPeer WRITE setRemoteTlsVerifyPeer NOTIFY remoteTlsVerifyPeerChanged)

    // Remote logging - compression
    Q_PROPERTY(bool remoteCompressionEnabled READ isRemoteCompressionEnabled WRITE setRemoteCompressionEnabled NOTIFY remoteCompressionEnabledChanged)
    Q_PROPERTY(int remoteCompressionLevel READ remoteCompressionLevel WRITE setRemoteCompressionLevel NOTIFY remoteCompressionLevelChanged)

public:
    enum class RemoteProtocol {
        UDP,            ///< UDP only (fire-and-forget)
        TCP,            ///< TCP only (reliable, connection-based)
        AutoFallback    ///< Start with UDP, fall back to TCP on failures
    };
    Q_ENUM(RemoteProtocol)

    enum class ExportFormat {
        ExportText = static_cast<int>(LogFormatter::PlainText),
        ExportJSON = static_cast<int>(LogFormatter::JSON),
        ExportCSV = static_cast<int>(LogFormatter::CSV),
        ExportJSONLines = static_cast<int>(LogFormatter::JSONLines)
    };
    Q_ENUM(ExportFormat)

    explicit LogManager(QObject* parent = nullptr);
    ~LogManager() override;

    /// Returns the singleton instance.
    static LogManager* instance();

    /// Installs the Qt message handler. Call once at startup.
    static void installHandler();

    /// Returns the log model for QML binding.
    LogModel* model() const { return _model; }

    /// Disk logging control.
    bool isDiskLoggingEnabled() const;
    void setDiskLoggingEnabled(bool enabled);

    /// Sets the directory for log files.
    void setLogDirectory(const QString& path);

    /// Remote logging control.
    bool isRemoteLoggingEnabled() const;
    void setRemoteLoggingEnabled(bool enabled);

    /// Remote endpoint (host:port format).
    QString remoteEndpoint() const;
    void setRemoteEndpoint(const QString& endpoint);

    /// Remote vehicle identifier.
    QString remoteVehicleId() const;
    void setRemoteVehicleId(const QString& id);

    /// Remote transport protocol.
    RemoteProtocol remoteProtocol() const;
    void setRemoteProtocol(RemoteProtocol protocol);

    /// Returns true if TCP is currently connected.
    bool isRemoteTcpConnected() const;

    /// Remote TLS encryption.
    bool isRemoteTlsEnabled() const;
    void setRemoteTlsEnabled(bool enabled);

    /// Remote TLS peer verification.
    bool remoteTlsVerifyPeer() const;
    void setRemoteTlsVerifyPeer(bool verify);

    /// Remote compression.
    bool isRemoteCompressionEnabled() const;
    void setRemoteCompressionEnabled(bool enabled);

    /// Remote compression level (1-9).
    int remoteCompressionLevel() const;
    void setRemoteCompressionLevel(int level);

    /// Format binary data as hex string for logging.
    Q_INVOKABLE static QString formatHex(const QByteArray& data, int maxBytes = 64);

    /// Returns true if a disk I/O error has occurred.
    bool hasError() const;

    /// Returns the last disk I/O error message.
    QString lastError() const { return _lastError; }

    /// Returns the last remote logging error message.
    QString lastRemoteError() const { return _lastRemoteError; }

    /// Returns the last TLS-specific error message.
    QString lastTlsError() const { return _lastTlsError; }

    /// Returns total bytes sent to remote endpoint.
    qint64 remoteBytesSent() const { return _remoteBytesSent; }

    /// Resets the remote bytes sent counter to zero.
    Q_INVOKABLE void resetRemoteBytesSent();

    /// Loads CA certificates from a PEM file for remote TLS.
    Q_INVOKABLE bool loadRemoteTlsCaCertificates(const QString& filePath);

    /// Loads client certificate and key from PEM files for mutual TLS.
    Q_INVOKABLE bool loadRemoteTlsClientCertificate(const QString& certPath, const QString& keyPath);

    /// Clears the error state and attempts recovery.
    Q_INVOKABLE void clearError();

    /// Clears all in-memory log entries.
    Q_INVOKABLE void clear();

    /// Exports current logs to a file asynchronously.
    Q_INVOKABLE void exportToFile(const QString& destFile, ExportFormat format = ExportFormat::ExportText);

    /// Exports filtered logs to a file asynchronously.
    Q_INVOKABLE void exportFilteredToFile(const QString& destFile, ExportFormat format = ExportFormat::ExportText);

    /// Returns suggested file filter for export dialogs.
    Q_INVOKABLE static QString exportFileFilter();

    /// Forces immediate flush of pending disk writes.
    Q_INVOKABLE void flush();

    /// Routes a log entry to all sinks. Called by Qt message handler.
    void handleLogEntry(const LogEntry& entry);

signals:
    void diskLoggingEnabledChanged();
    void remoteLoggingEnabledChanged();
    void remoteEndpointChanged();
    void remoteVehicleIdChanged();
    void remoteProtocolChanged();
    void remoteTcpConnectedChanged();
    void remoteTlsEnabledChanged();
    void remoteTlsVerifyPeerChanged();
    void remoteCompressionEnabledChanged();
    void remoteCompressionLevelChanged();
    void errorChanged();
    void lastErrorChanged();
    void lastRemoteErrorChanged();
    void lastTlsErrorChanged();
    void errorOccurred(const QString& message);
    void tlsErrorOccurred(const QString& message);
    void remoteBytesSentChanged();
    void exportStarted();
    void exportFinished(bool success);

private:
    friend class LogManagerTest;

    bool _prepareLogFile(const QString& path) const;
    void _rotateLogFiles(const QString& path) const;
    void _applyDiskWriterState();

    void _loadSettings();
    void _persistSettings();
    void _scheduleSave();
    void _exportEntries(const QList<LogEntry>& entries, const QString& destFile, ExportFormat format);

    LogModel* _model = nullptr;
    QGCFileWriter* _fileWriter = nullptr;
    LogRemoteSink* _remoteSink = nullptr;

    std::atomic<bool> _diskLoggingEnabled{false};
    QTimer* _saveTimer = nullptr;

    QString _lastError;
    QString _lastRemoteError;
    QString _lastTlsError;
    qint64 _remoteBytesSent = 0;

    static constexpr const char* kSettingsGroup = "LogManager";
    static constexpr qint64 kMaxLogFileSize = 10LL * 1024 * 1024; // 10MB
    static constexpr int kMaxBackupFiles = 5;
};
