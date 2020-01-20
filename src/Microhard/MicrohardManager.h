/****************************************************************************
 *
 *   (c) 2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCToolbox.h"
#include "QGCLoggingCategory.h"
#include "MicrohardSettings.h"
#include "Fact.h"

#include <QThread>

class AppSettings;
class QGCApplication;

#define DEFAULT_PAIRING_CHANNEL 36
#define DEFAULT_PAIRING_BANDWIDTH 1
#define DEFAULT_CONNECTING_BANDWIDTH 1
#define DEFAULT_CONNECTING_POWER 30

//-----------------------------------------------------------------------------
class MicrohardManager : public QGCTool
{
    Q_OBJECT
public:

    Q_PROPERTY(QString      connected           READ connected                                     NOTIFY connectedChanged)
    Q_PROPERTY(QString      linkConnected       READ linkConnected                                 NOTIFY linkConnectedChanged)
    Q_PROPERTY(bool         showRemote          READ showRemote           WRITE setShowRemote      NOTIFY showRemoteChanged)
    Q_PROPERTY(int          uplinkRSSI          READ uplinkRSSI           WRITE setUplinkRSSI      NOTIFY linkChanged)
    Q_PROPERTY(int          downlinkRSSI        READ downlinkRSSI         WRITE setDownlinkRSSI    NOTIFY linkChanged)
    Q_PROPERTY(int          downlinkRSSIPct     READ downlinkRSSIPct                               NOTIFY linkChanged)
    Q_PROPERTY(QString      localIPAddr         READ localIPAddr          WRITE setLocalIPAddr     NOTIFY localIPAddrChanged)
    Q_PROPERTY(QString      remoteIPAddr        READ remoteIPAddr         WRITE setRemoteIPAddr    NOTIFY remoteIPAddrChanged)
    Q_PROPERTY(QString      netMask             READ netMask                                       NOTIFY netMaskChanged)
    Q_PROPERTY(QString      configUserName      READ configUserName                                NOTIFY configUserNameChanged)
    Q_PROPERTY(QString      configPassword      READ configPassword                                NOTIFY configPasswordChanged)
    Q_PROPERTY(QString      encryptionKey       READ encryptionKey       WRITE setEncryptionKey    NOTIFY encryptionKeyChanged)
    Q_PROPERTY(QString      networkId           READ networkId           WRITE setNetworkId        NOTIFY networkIdChanged)
    Q_PROPERTY(int          pairingChannel      READ pairingChannel      WRITE setPairingChannel   NOTIFY pairingChannelChanged)
    Q_PROPERTY(int          connectingChannel   READ connectingChannel   WRITE setConnectChannel   NOTIFY connectingChannelChanged)
    Q_PROPERTY(int          connectingBandwidth READ connectingBandwidth WRITE setConnectBandwidth NOTIFY connectingBandwidthChanged)
    Q_PROPERTY(int          connectingPower     READ connectingPower     WRITE setConnectPower     NOTIFY connectingPowerChanged)
    Q_PROPERTY(QString      connectingNetworkId READ connectingNetworkId WRITE setConnectNetworkId NOTIFY connectingNetworkIdChanged)
    Q_PROPERTY(QStringList  channelLabels       READ channelLabels                                 NOTIFY channelLabelsChanged)
    Q_PROPERTY(QStringList  bandwidthLabels     READ bandwidthLabels                               NOTIFY bandwidthLabelsChanged)
    Q_PROPERTY(int          channelMin          READ channelMin                                    NOTIFY channelMinChanged)
    Q_PROPERTY(int          channelMax          READ channelMax                                    NOTIFY channelMaxChanged)
    Q_PROPERTY(int          pairingPower        READ pairingPower                                  CONSTANT)


    Q_INVOKABLE bool setIPSettings              (QString localIP, QString remoteIP, QString netMask, QString cfgUserName,
                                                 QString cfgPassword, QString encyrptionKey, QString networkId, int channel, int bandwidth);

    explicit MicrohardManager                   (QGCApplication* app, QGCToolbox* toolbox);
    ~MicrohardManager                           () override;

    void        setToolbox                      (QGCToolbox* toolbox) override;

    QString     connected                       () { return _connectedStatus; }
    bool        showRemote                      () { return _showRemote; }
    QString     linkConnected                   () { return _linkConnectedStatus; }
    int         uplinkRSSI                      () { return _uplinkRSSI; }
    int         downlinkRSSI                    () { return _downlinkRSSI; }
    int         downlinkRSSIPct                 ();
    QString     localIPAddr                     () { return _localIPAddr; }
    QString     remoteIPAddr                    () { return _remoteIPAddr; }
    QString     netMask                         () { return _netMask; }
    QString     configUserName                  () { return _configUserName; }
    QString     configPassword                  () { return _configPassword; }
    QString     encryptionKey                   () { return _encryptionKey; }
    QString     networkId                       () { return _networkId; }
    int         pairingChannel                  () { return _pairingChannel; }
    int         connectingChannel               () { return _connectingChannel; }
    int         connectingBandwidth             () { return _connectingBandwidth; }
    QString     connectingNetworkId             () { return _connectingNetworkId; }
    QStringList channelLabels                   () { return _channelLabels; }
    QStringList bandwidthLabels                 () { return _bandwidthLabels; }
    int         channelMin                      () { return _channelMin; }
    int         channelMax                      () { return _channelMax; }
    int         pairingPower                    () const { return _pairingPower; }
    int         connectingPower                 () const { return _connectingPower; }
    int         getChannelFrequency             (int channel) { return channel - _channelMin + _frequencyStart; }
    void        setShowRemote                   (bool val);
    void        setDownlinkRSSI                 (int rssi)    { _downlinkRSSI = rssi; emit linkChanged(); }
    void        setUplinkRSSI                   (int rssi)    { _uplinkRSSI = rssi; emit linkChanged(); }
    void        setLocalIPAddr                  (QString val) { _localIPAddr = val; emit localIPAddrChanged(); }
    void        setRemoteIPAddr                 (QString val) { _remoteIPAddr = val; emit remoteIPAddrChanged(); }
    void        setConfigUserName               (QString val) { _configUserName = val; emit configUserNameChanged(); }
    void        setConfigPassword               (QString val) { _configPassword = val; }
    void        setEncryptionKey                (QString val) { _encryptionKey = val; emit encryptionKeyChanged(); }
    void        setNetworkId                    (QString val) { _networkId = val; emit networkIdChanged(); }
    void        setPairingChannel               (int val)     { _pairingChannel = val; emit pairingChannelChanged(); }
    void        setConnectChannel               (int val)     { _connectingChannel = val; }
    void        setConnectBandwidth             (int val)     { _connectingBandwidth = val; }
    void        setConnectPower                 (int val)     { _connectingPower = val; }
    void        setConnectNetworkId             (QString val) { _connectingNetworkId = val; emit connectingNetworkIdChanged(); }
    void        updateSettings                  ();
    void        configure                       ();
    void        switchToPairingEncryptionKey    (QString pairingKey);
    void        switchToConnectionEncryptionKey (QString encryptionKey);
    void        setProductName                  (QString product);
    int         adjustChannelToBandwitdh        (int channel, int bandwidth);

signals:
    void linkChanged();
    void linkConnectedChanged();
    void showRemoteChanged();
    void connectedChanged();
    void localIPAddrChanged();
    void remoteIPAddrChanged();
    void netMaskChanged();
    void configUserNameChanged();
    void configPasswordChanged();
    void encryptionKeyChanged();
    void networkIdChanged();
    void pairingChannelChanged();
    void connectingChannelChanged();
    void connectingBandwidthChanged();
    void connectingPowerChanged();
    void connectingNetworkIdChanged();
    void channelLabelsChanged();
    void bandwidthLabelsChanged();
    void channelMinChanged();
    void channelMaxChanged();
    void run();
    void configureMicrohard(QString key, int power, int channel, int bandwidth, QString networkId);

private slots:
    void    _connectedLoc                   (const QString& status);
    void    _rssiUpdatedLoc                 (int rssi);
    void    _connectedRem                   (const QString& status);
    void    _rssiUpdatedRem                 (int rssi);
    void    _setEnabled                     ();

private:
    void    _close                          ();
    void    _reset                          ();
    FactMetaData *_createMetadata           (const char *name, QStringList enums);

private:
    QString            _connectedStatus;
    bool               _showRemote = true;
    AppSettings*       _appSettings = nullptr;
    MicrohardSettings* _mhSettingsLoc = nullptr;
    QThread*           _mhSettingsLocThread = nullptr;
    MicrohardSettings* _mhSettingsRem = nullptr;
    QThread*           _mhSettingsRemThread = nullptr;
    bool               _enabled  = true;
    QString            _linkConnectedStatus;
    int                _downlinkRSSI = 0;
    int                _uplinkRSSI = 0;
    QString            _localIPAddr;
    QString            _remoteIPAddr;
    QString            _netMask;
    QString            _configUserName;
    QString            _configPassword;
    QString            _networkId;
    QString            _encryptionKey;
    QString            _communicationEncryptionKey;
    bool               _usePairingSettings = true;
    const int          _pairingPower = 7;
    int                _connectingPower = DEFAULT_CONNECTING_POWER;
    int                _pairingChannel = DEFAULT_PAIRING_CHANNEL;
    int                _connectingChannel = DEFAULT_PAIRING_CHANNEL;
    int                _connectingBandwidth = DEFAULT_CONNECTING_BANDWIDTH;
    int                _pairingBandwidth = DEFAULT_PAIRING_BANDWIDTH;
    QString            _connectingNetworkId;
    QStringList        _channelLabels;
    QStringList        _bandwidthLabels;
    int                _frequencyStart = 2405;
    int                _channelMin = 1;
    int                _channelMax = 81;
    QList<int>         _bandwidthChannelMin;
    QList<int>         _bandwidthChannelMax;
    QString            _modemName = "pDDL1800";
    void               _updateSettings();
};
