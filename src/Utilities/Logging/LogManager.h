#pragma once

#include "LogFormatter.h"

#include <QtCore/QObject>

class LogDiskWriter;
class LogModel;
class LogRemoteSink;
struct QGCLogEntry;

/// Central logging coordinator for QGroundControl.
/// Captures Qt log messages and routes them to in-memory model, disk, and remote.
class LogManager : public QObject
{
    Q_OBJECT
    Q_MOC_INCLUDE("LogModel.h")

    // Core properties
    Q_PROPERTY(LogModel* model READ model CONSTANT)
    Q_PROPERTY(bool hasError READ hasError NOTIFY errorChanged)

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
    enum RemoteProtocol {
        UDP,            ///< UDP only (fire-and-forget)
        TCP,            ///< TCP only (reliable, connection-based)
        AutoFallback    ///< Start with UDP, fall back to TCP on failures
    };
    Q_ENUM(RemoteProtocol)

    enum ExportFormat {
        ExportText = LogFormatter::PlainText,
        ExportJSON = LogFormatter::JSON,
        ExportCSV = LogFormatter::CSV,
        ExportJSONLines = LogFormatter::JSONLines
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

    /// Returns true if a disk I/O error has occurred.
    bool hasError() const;

    /// Clears the error state and attempts recovery.
    Q_INVOKABLE void clearError();

    /// Clears all in-memory log entries.
    Q_INVOKABLE void clear();

    /// Exports current logs to a file asynchronously.
    /// @param destFile Output file path
    /// @param format Export format (default: auto-detect from extension, or Text)
    Q_INVOKABLE void exportToFile(const QString& destFile, ExportFormat format = ExportText);

    /// Exports filtered logs to a file asynchronously.
    Q_INVOKABLE void exportFilteredToFile(const QString& destFile, ExportFormat format = ExportText);

    /// Returns suggested file filter for export dialogs.
    Q_INVOKABLE static QString exportFileFilter();

    /// Forces immediate flush of pending disk writes.
    Q_INVOKABLE void flush();

    /// Routes a log entry to all sinks. Called by Qt message handler.
    void handleLogEntry(const QGCLogEntry& entry);

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
    void errorOccurred(const QString& message);
    void exportStarted();
    void exportFinished(bool success);

private:
    friend class LogManagerTest;

    void _loadSettings();
    void _saveRemoteSettings();

    LogModel* _model = nullptr;
    LogDiskWriter* _diskWriter = nullptr;
    LogRemoteSink* _remoteSink = nullptr;

    static constexpr const char* kSettingsGroup = "LogManager";
};
