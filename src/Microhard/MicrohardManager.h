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

    Q_PROPERTY(bool         connected           READ connected                                  NOTIFY connectedChanged)
    Q_PROPERTY(bool         linkConnected       READ linkConnected                              NOTIFY linkConnectedChanged)
    Q_PROPERTY(bool         needReboot          READ needReboot                                 NOTIFY needRebootChanged)
    Q_PROPERTY(int          uplinkRSSI          READ uplinkRSSI                                 NOTIFY linkChanged)
    Q_PROPERTY(int          downlinkRSSI        READ downlinkRSSI                               NOTIFY linkChanged)
    Q_PROPERTY(QString      localIPAddr         READ localIPAddr                                NOTIFY localIPAddrChanged)
    Q_PROPERTY(QString      remoteIPAddr        READ remoteIPAddr                               NOTIFY remoteIPAddrChanged)
    Q_PROPERTY(QString      netMask             READ netMask                                    NOTIFY netMaskChanged)
    Q_PROPERTY(QString      configPassword      READ configPassword                             NOTIFY configPasswordChanged)

    Q_INVOKABLE bool setIPSettings              (QString localIP, QString remoteIP, QString netMask);

    explicit MicrohardManager                   (QGCApplication* app, QGCToolbox* toolbox);
    ~MicrohardManager                           () override;

    void        setToolbox                      (QGCToolbox* toolbox) override;

    bool        connected                       () { return _isConnected; }
    bool        linkConnected                   () { return _linkConnected; }
    bool        needReboot                      () { return _needReboot; }
    int         uplinkRSSI                      () { return _downlinkRSSI; }
    int         downlinkRSSI                    () { return _uplinkRSSI; }
    QString     localIPAddr                     () { return _localIPAddr; }
    QString     remoteIPAddr                    () { return _remoteIPAddr; }
    QString     netMask                         () { return _netMask; }
    QString     configPassword                  () { return _configPassword; }

signals:
    void    linkChanged                     ();
    void    linkConnectedChanged            ();
    void    infoChanged                     ();
    void    connectedChanged                ();
    void    decodeIndexChanged              ();
    void    rateIndexChanged                ();
    void    localIPAddrChanged              ();
    void    remoteIPAddrChanged             ();
    void    netMaskChanged                  ();
    void    needRebootChanged               ();
    void    configPasswordChanged           ();

private slots:
    void    _connected                      ();
    void    _disconnected                   ();
    void    _checkMicrohard                 ();
    void    _updateSettings                 (QByteArray jSonData);
    void    _setEnabled                     ();

private:
    void    _close                          ();
    void    _reset                          ();
    FactMetaData *_createMetadata           (const char *name, QStringList enums);

private:

    enum {
        REQ_LINK_STATUS         = 1,
        REQ_DEV_INFO            = 2,
        REQ_FREQ_SCAN           = 4,
        REQ_VIDEO_SETTINGS      = 8,
        REQ_RADIO_SETTINGS      = 16,
        REQ_RTSP_SETTINGS       = 32,
        REQ_IP_SETTINGS         = 64,
        REQ_ALL                 = 0xFFFFFFF,
    };

    uint32_t                _reqMask        = static_cast<uint32_t>(REQ_ALL);
    bool                    _running        = false;
    bool                    _isConnected    = false;
    AppSettings*            _appSettings    = nullptr;
    MicrohardSettings*      _mhSettings     = nullptr;
    bool            _enabled                = true;
    bool            _linkConnected          = false;
    bool            _needReboot             = false;
    QTimer          _workTimer;
    int             _downlinkRSSI           = 0;
    int             _uplinkRSSI             = 0;
    QString         _localIPAddr;
    QString         _remoteIPAddr;
    QString         _netMask;
    QString         _configPassword;
    QTime           _timeoutTimer;
};
