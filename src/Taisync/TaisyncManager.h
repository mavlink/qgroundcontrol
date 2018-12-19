/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCToolbox.h"
#include "QGCLoggingCategory.h"
#include "TaisyncSettings.h"
#if defined(__ios__) || defined(__android__)
#include "TaisyncTelemetry.h"
#include "TaisyncVideoReceiver.h"
#endif

#include <QTimer>

class AppSettings;
class QGCApplication;

//-----------------------------------------------------------------------------
class TaisyncManager : public QGCTool
{
    Q_OBJECT
public:

    Q_PROPERTY(bool         connected           READ connected                                  NOTIFY connectedChanged)
    Q_PROPERTY(bool         linkConnected       READ linkConnected                              NOTIFY linkChanged)
    Q_PROPERTY(QString      linkVidFormat       READ linkVidFormat                              NOTIFY linkChanged)
    Q_PROPERTY(int          uplinkRSSI          READ uplinkRSSI                                 NOTIFY linkChanged)
    Q_PROPERTY(int          downlinkRSSI        READ downlinkRSSI                               NOTIFY linkChanged)
    Q_PROPERTY(QString      serialNumber        READ serialNumber                               NOTIFY linkChanged)
    Q_PROPERTY(QString      fwVersion           READ fwVersion                                  NOTIFY linkChanged)
    Q_PROPERTY(QStringList  decodeList          READ decodeList                                 NOTIFY decodeIndexChanged)
    Q_PROPERTY(int          decodeIndex         READ decodeIndex        WRITE setDecodeIndex    NOTIFY decodeIndexChanged)
    Q_PROPERTY(QStringList  rateList            READ rateList                                   NOTIFY rateIndexChanged)
    Q_PROPERTY(int          rateIndex           READ rateIndex          WRITE setRateIndex      NOTIFY rateIndexChanged)

    explicit TaisyncManager                 (QGCApplication* app, QGCToolbox* toolbox);
    ~TaisyncManager                         () override;

    void        setToolbox                      (QGCToolbox* toolbox) override;

    bool        connected                       () { return _isConnected; }
    bool        linkConnected                   () { return _linkConnected; }
    QString     linkVidFormat                   () { return _linkVidFormat; }
    int         uplinkRSSI                      () { return _downlinkRSSI; }
    int         downlinkRSSI                    () { return _uplinkRSSI; }
    QString     serialNumber                    () { return _serialNumber; }
    QString     fwVersion                       () { return _fwVersion; }
    QStringList decodeList                      () { return _decodeList; }
    int         decodeIndex                     () { return _decodeIndex; }
    void        setDecodeIndex                  (int idx);
    QStringList rateList                        () { return _rateList; }
    int         rateIndex                       () { return _rateIndex; }
    void        setRateIndex                    (int idx);

signals:
    void    linkChanged                     ();
    void    connectedChanged                ();
    void    decodeIndexChanged              ();
    void    rateIndexChanged                ();

private slots:
    void    _connected                      ();
    void    _checkTaisync                   ();
    void    _updateSettings                 (QByteArray jSonData);
    void    _setEnabled                     ();
    void    _setVideoEnabled                ();
#if defined(__ios__) || defined(__android__)
    void    _readUDPBytes                   ();
    void    _readTelemBytes                 (QByteArray bytesIn);
#endif

private:

    enum {
        REQ_LINK_STATUS,
        REQ_DEV_INFO,
        REQ_FREQ_SCAN,
        REQ_VIDEO_SETTINGS,
        REQ_LAST
    };

    int                     _currReq        = REQ_LAST;
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
    bool        _enableVideo                = true;
    bool        _enabled                    = true;
    bool        _linkConnected              = false;
    QTimer      _workTimer;
    QString     _linkVidFormat;
    int         _downlinkRSSI               = 0;
    int         _uplinkRSSI                 = 0;
    QStringList _decodeList;
    int         _decodeIndex                = 0;
    QStringList _rateList;
    int         _rateIndex                  = 0;
    bool        _savedVideoState            = true;
    QVariant    _savedVideoSource;
    QVariant    _savedVideoUDP;
    QVariant    _savedAR;
    QString     _serialNumber;
    QString     _fwVersion;
};
