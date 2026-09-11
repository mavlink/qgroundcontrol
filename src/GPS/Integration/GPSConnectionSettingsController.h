#pragma once

#include <QtCore/QObject>

#include <functional>

class GPSReceiverAutoConnect;
class NMEASourceManager;
class SettingsManager;

/// Applies saved connection intent and batches receiver configuration changes.
class GPSConnectionSettingsController : public QObject
{
    Q_OBJECT

public:
    GPSConnectionSettingsController(SettingsManager& settings, GPSReceiverAutoConnect& receiver,
                                    NMEASourceManager& nmea, std::function<bool()> suspended,
                                    QObject* parent = nullptr);
    ~GPSConnectionSettingsController() override;
    void start();
    void shutdown();
    void updateConnections();
    bool connectionsSuspended() const;
    void beginReceiverSettingsBatch();
    void endReceiverSettingsBatch();

signals:
    void receiverSettingsChanged();

private:
    void _updateNmeaSettings();
    void _updateReceiverSettings(bool restart = false);
    SettingsManager& _settings;
    GPSReceiverAutoConnect& _receiver;
    NMEASourceManager& _nmea;
    std::function<bool()> _connectionsSuspended;
    quint64 _nmeaSettingsRevision = 0;
    quint64 _receiverSettingsRevision = 0;
    unsigned _receiverSettingsBatchDepth = 0;
    bool _receiverRestartPending = false;
    bool _initialized = false;
    bool _shutdown = false;
};
