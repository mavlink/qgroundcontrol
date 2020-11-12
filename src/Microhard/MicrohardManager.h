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

#include <QTimer>
#include <QTime>

class AppSettings;
class QGCApplication;

//-----------------------------------------------------------------------------
class MicrohardManager : public QGCTool
{
    Q_OBJECT
public:

    Q_PROPERTY(int          connected           READ connected                                  NOTIFY connectedChanged)
    Q_PROPERTY(int          linkConnected       READ linkConnected                              NOTIFY linkConnectedChanged)
    Q_PROPERTY(int          uplinkRSSI          READ uplinkRSSI                                 NOTIFY linkChanged)
    Q_PROPERTY(int          downlinkRSSI        READ downlinkRSSI                               NOTIFY linkChanged)
    Q_PROPERTY(QString      localIPAddr         READ localIPAddr      WRITE setLocalIPAddr      NOTIFY localIPAddrChanged)
    Q_PROPERTY(QString      remoteIPAddr        READ remoteIPAddr     WRITE setRemoteIPAddr     NOTIFY remoteIPAddrChanged)
    Q_PROPERTY(QString      netMask             READ netMask                                    NOTIFY netMaskChanged)
    Q_PROPERTY(QString      configUserName      READ configUserName                             NOTIFY configUserNameChanged)
    Q_PROPERTY(QString      configPassword      READ configPassword                             NOTIFY configPasswordChanged)
    Q_PROPERTY(QString      encryptionKey       READ encryptionKey                              NOTIFY encryptionKeyChanged)

    Q_INVOKABLE bool setIPSettings              (QString localIP, QString remoteIP, QString netMask, QString cfgUserName, QString cfgPassword, QString encyrptionKey);

    explicit MicrohardManager                   (QGCApplication* app, QGCToolbox* toolbox);
    ~MicrohardManager                           () override;

    void        setToolbox                      (QGCToolbox* toolbox) override;

    int         connected                       () { return _connectedStatus; }
    int         linkConnected                   () { return _linkConnectedStatus; }
    int         uplinkRSSI                      () { return _downlinkRSSI; }
    int         downlinkRSSI                    () { return _uplinkRSSI; }
    QString     localIPAddr                     () { return _localIPAddr; }
    QString     remoteIPAddr                    () { return _remoteIPAddr; }
    QString     netMask                         () { return _netMask; }
    QString     configUserName                  () { return _configUserName; }
    QString     configPassword                  () { return _configPassword; }
    QString     encryptionKey                   () { return _encryptionKey; }

    void        setLocalIPAddr                  (QString val) { _localIPAddr = val; emit localIPAddrChanged(); }
    void        setRemoteIPAddr                 (QString val) { _remoteIPAddr = val; emit remoteIPAddrChanged(); }
    void        setConfigUserName               (QString val) { _configUserName = val; emit configUserNameChanged(); }
    void        setConfigPassword               (QString val) { _configPassword = val; emit configPasswordChanged(); }
    void        setEncryptionKey                (QString val) { _encryptionKey = val; emit encryptionKeyChanged(); }
    void        updateSettings                  ();
    void        setEncryptionKey                ();
    void        switchToPairingEncryptionKey    ();
    void        switchToConnectionEncryptionKey (QString encryptionKey);

signals:
    void    linkChanged                     ();
    void    linkConnectedChanged            ();
    void    connectedChanged                ();
    void    localIPAddrChanged              ();
    void    remoteIPAddrChanged             ();
    void    netMaskChanged                  ();
    void    configUserNameChanged           ();
    void    configPasswordChanged           ();
    void    encryptionKeyChanged            ();

private slots:
    void    _connectedLoc                   (int status);
    void    _rssiUpdatedLoc                 (int rssi);
    void    _connectedRem                   (int status);
    void    _rssiUpdatedRem                 (int rssi);
    void    _checkMicrohard                 ();
    void    _setEnabled                     ();
    void    _locTimeout                     ();
    void    _remTimeout                     ();

private:
    void    _close                          ();
    void    _reset                          ();
    FactMetaData *_createMetadata           (const char *name, QStringList enums);

private:
    int                _connectedStatus = 0;
    AppSettings*       _appSettings = nullptr;
    MicrohardSettings* _mhSettingsLoc = nullptr;
    MicrohardSettings* _mhSettingsRem = nullptr;
    bool               _enabled  = true;
    int                _linkConnectedStatus = 0;
    QTimer             _workTimer;
    QTimer             _locTimer;
    QTimer             _remTimer;
    int                _downlinkRSSI = 0;
    int                _uplinkRSSI = 0;
    QString            _localIPAddr;
    QString            _remoteIPAddr;
    QString            _netMask;
    QString            _configUserName;
    QString            _configPassword;
    QString            _encryptionKey;
    bool               _useCommunicationEncryptionKey = false;
    QString            _communicationEncryptionKey;
    QTime              _timeoutTimer;
};
