/****************************************************************************
 *
 *   (c) 2009-2018 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once

#include <QString>
#include <QList>
#include <QMap>
#include <QMutex>
#include <QUdpSocket>
#include <QMutexLocker>
#include <QQueue>
#include <QByteArray>
#include <QTcpServer>
#include <QTcpSocket>
#include <QVariant>

#include "QGCConfig.h"
#include "LinkManager.h"

class TaisyncTelemetry;
class TaisyncSettings;
#if defined(__ios__) || defined(__android__)
class TaisyncVideoReceiver;
#endif

//-----------------------------------------------------------------------------
class TaisyncConfiguration : public LinkConfiguration
{
    Q_OBJECT
public:

    Q_PROPERTY(bool videoEnabled     READ videoEnabled    WRITE setVideoEnabled  NOTIFY enableVideoChanged)

    TaisyncConfiguration    (const QString& name);
    TaisyncConfiguration    (TaisyncConfiguration* source);
    ~TaisyncConfiguration   ();

    bool    videoEnabled    () { return _enableVideo; }

    void    setVideoEnabled (bool enable);

    /// From LinkConfiguration
    LinkType    type                () { return LinkConfiguration::TypeTaisync; }
    void        copyFrom            (LinkConfiguration* source);
    void        loadSettings        (QSettings& settings, const QString& root);
    void        saveSettings        (QSettings& settings, const QString& root);
    void        updateSettings      ();
    bool        isAutoConnectAllowed() { return true; }
    bool        isHighLatencyAllowed() { return false; }
    QString     settingsURL         () { return "TaisyncSettings.qml"; }
    QString     settingsTitle       () { return tr("Taisync Link Settings"); }

signals:
    void        enableVideoChanged  ();

private:
    void        _copyFrom           (LinkConfiguration *source);

private:
    bool        _enableVideo        = true;
};

//-----------------------------------------------------------------------------
class TaisyncLink : public LinkInterface
{
    Q_OBJECT

    friend class TaisyncConfiguration;
    friend class LinkManager;

public:
    void    requestReset            () override { }
    bool    isConnected             () const override;
    QString getName                 () const override;

    // Extensive statistics for scientific purposes
    qint64  getConnectionSpeed      () const override;
    qint64  getCurrentInDataRate    () const;
    qint64  getCurrentOutDataRate   () const;

    // Thread
    void    run                     () override;

    // These are left unimplemented in order to cause linker errors which indicate incorrect usage of
    // connect/disconnect on link directly. All connect/disconnect calls should be made through LinkManager.
    bool    connect                 (void);
    bool    disconnect              (void);

private slots:
    void    _writeBytes             (const QByteArray data) override;
    void    _readBytes              (QByteArray bytes);

private:
    // Links are only created/destroyed by LinkManager so constructor/destructor is not public
    TaisyncLink                     (SharedLinkConfigurationPointer& config);
    ~TaisyncLink                    () override;

    // From LinkInterface
    bool    _connect                () override;
    void    _disconnect             () override;

    bool    _hardwareConnect        ();
    void    _hardwareDisconnect     ();
    void    _restartConnection      ();
    void    _writeDataGram          (const QByteArray data);

private:
    bool                    _running            = false;
    TaisyncConfiguration*   _taiConfig          = nullptr;
    TaisyncTelemetry*       _taiTelemetery      = nullptr;
    TaisyncSettings*        _taiSettings        = nullptr;
#if defined(__ios__) || defined(__android__)
    TaisyncVideoReceiver*   _taiVideo           = nullptr;
#endif
    bool                    _savedVideoState    = true;
    QVariant                _savedVideoSource;
    QVariant                _savedVideoUDP;
    QVariant                _savedAR;
};

