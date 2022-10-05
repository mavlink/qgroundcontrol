/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
#include <QHostAddress>
#include <LinkInterface.h>
#include "QGCConfig.h"

// Even though QAbstractSocket::SocketError is used in a signal by Qt, Qt doesn't declare it as a meta type.
// This in turn causes debug output to be kicked out about not being able to queue the signal. We declare it
// as a meta type to silence that.
#include <QMetaType>
#include <QTcpSocket>

//#define TCPLINK_READWRITE_DEBUG   // Use to debug data reads/writes

class TCPLinkTest;
class LinkManager;

#define QGC_TCP_PORT 5760

class TCPConfiguration : public LinkConfiguration
{
    Q_OBJECT

public:

    Q_PROPERTY(quint16 port READ port WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(QString host READ host WRITE setHost NOTIFY hostChanged)

    TCPConfiguration(const QString& name);
    TCPConfiguration(TCPConfiguration* source);

    quint16             port        (void) const                         { return _port; }
    const QHostAddress& address     (void)                          { return _address; }
    const QString       host        (void)                          { return _address.toString(); }
    void                setPort     (quint16 port);
    void                setAddress  (const QHostAddress& address);
    void                setHost     (const QString host);

    //LinkConfiguration overrides
    LinkType    type                (void) override                                         { return LinkConfiguration::TypeTcp; }
    void        copyFrom            (LinkConfiguration* source) override;
    void        loadSettings        (QSettings& settings, const QString& root) override;
    void        saveSettings        (QSettings& settings, const QString& root) override;
    QString     settingsURL         (void) override                                         { return "TcpSettings.qml"; }
    QString     settingsTitle       (void) override                                         { return tr("TCP Link Settings"); }

signals:
    void portChanged(void);
    void hostChanged(void);

private:
    QHostAddress    _address;
    quint16         _port;
};

class TCPLink : public LinkInterface
{
    Q_OBJECT

public:
    TCPLink(SharedLinkConfigurationPtr& config);
    virtual ~TCPLink();

    QTcpSocket* getSocket           (void) { return _socket; }
    void        signalBytesWritten  (void);

    // LinkInterface overrides
    bool isConnected(void) const override;
    void disconnect (void) override;

private slots:
    void _socketError   (QAbstractSocket::SocketError socketError);
    void _readBytes     (void);

    // LinkInterface overrides
    void _writeBytes(const QByteArray data) override;

private:
    // LinkInterface overrides
    bool _connect(void) override;

    bool _hardwareConnect   (void);
#ifdef TCPLINK_READWRITE_DEBUG
    void _writeDebugBytes   (const QByteArray data);
#endif

    TCPConfiguration* _tcpConfig;
    QTcpSocket*       _socket;
    bool              _socketIsConnected;

    quint64 _bitsSentTotal;
    quint64 _bitsSentCurrent;
    quint64 _bitsSentMax;
    quint64 _bitsReceivedTotal;
    quint64 _bitsReceivedCurrent;
    quint64 _bitsReceivedMax;
    quint64 _connectionStartTime;
    QMutex  _statisticsMutex;
};

