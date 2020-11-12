/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCToolbox.h"
#include "QGCLoggingCategory.h"
#include "TaisyncSettings.h"
#include "Fact.h"
#if defined(__ios__) || defined(__android__)
#include "TaisyncTelemetry.h"
#include "TaisyncVideoReceiver.h"
#endif

#include <QTimer>
#include <QTime>

class AppSettings;
class QGCApplication;

//-----------------------------------------------------------------------------
class TaisyncManager : public QGCTool
{
    Q_OBJECT
public:

    Q_PROPERTY(bool         connected           READ connected                                  NOTIFY connectedChanged)
    Q_PROPERTY(bool         linkConnected       READ linkConnected                              NOTIFY linkConnectedChanged)
    Q_PROPERTY(bool         needReboot          READ needReboot                                 NOTIFY needRebootChanged)
    Q_PROPERTY(QString      linkVidFormat       READ linkVidFormat                              NOTIFY linkChanged)
    Q_PROPERTY(int          uplinkRSSI          READ uplinkRSSI                                 NOTIFY linkChanged)
    Q_PROPERTY(int          downlinkRSSI        READ downlinkRSSI                               NOTIFY linkChanged)
    Q_PROPERTY(QString      serialNumber        READ serialNumber                               NOTIFY infoChanged)
    Q_PROPERTY(QString      fwVersion           READ fwVersion                                  NOTIFY infoChanged)
    Q_PROPERTY(Fact*        radioMode           READ radioMode                                  CONSTANT)
    Q_PROPERTY(Fact*        radioChannel        READ radioChannel                               CONSTANT)
    Q_PROPERTY(Fact*        videoOutput         READ videoOutput                                CONSTANT)
    Q_PROPERTY(Fact*        videoMode           READ videoMode                                  CONSTANT)
    Q_PROPERTY(Fact*        videoRate           READ videoRate                                  CONSTANT)
    Q_PROPERTY(QString      rtspURI             READ rtspURI                                    NOTIFY rtspURIChanged)
    Q_PROPERTY(QString      rtspAccount         READ rtspAccount                                NOTIFY rtspAccountChanged)
    Q_PROPERTY(QString      rtspPassword        READ rtspPassword                               NOTIFY rtspPasswordChanged)
    Q_PROPERTY(QString      localIPAddr         READ localIPAddr                                NOTIFY localIPAddrChanged)
    Q_PROPERTY(QString      remoteIPAddr        READ remoteIPAddr                               NOTIFY remoteIPAddrChanged)
    Q_PROPERTY(QString      netMask             READ netMask                                    NOTIFY netMaskChanged)

    Q_INVOKABLE bool setRTSPSettings            (QString uri, QString account, QString password);
    Q_INVOKABLE bool setIPSettings              (QString localIP, QString remoteIP, QString netMask);

    explicit TaisyncManager                 (QGCApplication* app, QGCToolbox* toolbox);
    ~TaisyncManager                         () override;

    void        setToolbox                      (QGCToolbox* toolbox) override;

    bool        connected                       () { return _isConnected; }
    bool        linkConnected                   () { return _linkConnected; }
    bool        needReboot                      () { return _needReboot; }
    QString     linkVidFormat                   () { return _linkVidFormat; }
    int         uplinkRSSI                      () { return _downlinkRSSI; }
    int         downlinkRSSI                    () { return _uplinkRSSI; }
    QString     serialNumber                    () { return _serialNumber; }
    QString     fwVersion                       () { return _fwVersion; }
    Fact*       radioMode                       () { return _radioMode; }
    Fact*       radioChannel                    () { return _radioChannel; }
    Fact*       videoOutput                     () { return _videoOutput; }
    Fact*       videoMode                       () { return _videoMode; }
    Fact*       videoRate                       () { return _videoRate; }
    QString     rtspURI                         () { return _rtspURI; }
    QString     rtspAccount                     () { return _rtspAccount; }
    QString     rtspPassword                    () { return _rtspPassword; }
    QString     localIPAddr                     () { return _localIPAddr; }
    QString     remoteIPAddr                    () { return _remoteIPAddr; }
    QString     netMask                         () { return _netMask; }

signals:
    void    linkChanged                     ();
    void    linkConnectedChanged            ();
    void    infoChanged                     ();
    void    connectedChanged                ();
    void    decodeIndexChanged              ();
    void    rateIndexChanged                ();
    void    rtspURIChanged                  ();
    void    rtspAccountChanged              ();
    void    rtspPasswordChanged             ();
    void    localIPAddrChanged              ();
    void    remoteIPAddrChanged             ();
    void    netMaskChanged                  ();
    void    needRebootChanged               ();

private slots:
    void    _connected                      ();
    void    _disconnected                   ();
    void    _checkTaisync                   ();
    void    _updateSettings                 (QByteArray jSonData);
    void    _setEnabled                     ();
    void    _setVideoEnabled                ();
    void    _radioSettingsChanged           (QVariant);
    void    _videoSettingsChanged           (QVariant);
#if defined(__ios__) || defined(__android__)
    void    _readUDPBytes                   ();
    void    _readTelemBytes                 (QByteArray bytesIn);
#endif

private:
    void    _close                          ();
    void    _reset                          ();
    void    _restoreVideoSettings           (Fact* setting);
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
    TaisyncManager*         _taiManager     = nullptr;
    TaisyncSettings*        _taiSettings    = nullptr;
#if defined(__ios__) || defined(__android__)
    TaisyncTelemetry*       _taiTelemetery  = nullptr;
    TaisyncVideoReceiver*   _taiVideo       = nullptr;
    QUdpSocket*             _telemetrySocket= nullptr;
#endif
    bool            _enableVideo            = true;
    bool            _enabled                = true;
    bool            _linkConnected          = false;
    bool            _needReboot             = false;
    QTimer          _workTimer;
    QString         _linkVidFormat;
    int             _downlinkRSSI           = 0;
    int             _uplinkRSSI             = 0;
    QStringList     _decodeList;
    int             _decodeIndex            = 0;
    QStringList     _rateList;
    int             _rateIndex              = 0;
    QString         _serialNumber;
    QString         _fwVersion;
    Fact*           _radioMode              = nullptr;
    Fact*           _radioChannel           = nullptr;
    Fact*           _videoOutput            = nullptr;
    Fact*           _videoMode              = nullptr;
    Fact*           _videoRate              = nullptr;
    QStringList     _radioModeList;
    QStringList     _videoOutputList;
    QStringList     _videoRateList;
    QString         _rtspURI;
    QString         _rtspAccount;
    QString         _rtspPassword;
    QString         _localIPAddr;
    QString         _remoteIPAddr;
    QString         _netMask;
    QTime           _timeoutTimer;
};
