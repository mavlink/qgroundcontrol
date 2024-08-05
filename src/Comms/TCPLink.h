/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once


#include "LinkInterface.h"
#include "LinkConfiguration.h"

#include <QtCore/QString>
#include <QtCore/QMutex>
#include <QtNetwork/QAbstractSocket>

//#define TCPLINK_READWRITE_DEBUG   // Use to debug data reads/writes

class TCPLinkTest;
class LinkManager;
class QTcpSocket;

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
    QString             host        (void) const                         { return _host; }
    void                setPort     (quint16 port);
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
    QString         _host;
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
    bool isConnected        (void) const override;
    void disconnect         (void) override;
    bool isSecureConnection (void) override;

private slots:
    void _socketError   (QAbstractSocket::SocketError socketError);
    void _readBytes     (void);

    // LinkInterface overrides
    void _writeBytes(const QByteArray &data) override;

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

