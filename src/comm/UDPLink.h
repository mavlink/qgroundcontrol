/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/*!
 * @file
 *   @brief UDP connection (server) for unmanned vehicles
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#pragma once

#include <QString>
#include <QList>
#include <QMap>
#include <QMutex>
#include <QUdpSocket>
#include <QMutexLocker>
#include <QQueue>
#include <QByteArray>

#if defined(QGC_ZEROCONF_ENABLED)
#include <dns_sd.h>
#endif

#include "QGCConfig.h"
#include "LinkManager.h"

class UDPCLient {
public:
    UDPCLient(const QHostAddress& address_, quint16 port_)
        : address(address_)
        , port(port_)
    {}
    UDPCLient(const UDPCLient* other)
        : address(other->address)
        , port(other->port)
    {}
    QHostAddress    address;
    quint16         port;
};

class UDPConfiguration : public LinkConfiguration
{
    Q_OBJECT
public:

    Q_PROPERTY(quint16      localPort   READ localPort  WRITE setLocalPort  NOTIFY localPortChanged)
    Q_PROPERTY(QStringList  hostList    READ hostList                       NOTIFY  hostListChanged)

    /*!
     * @brief Regular constructor
     *
     * @param[in] name Configuration (user friendly) name
     */
    UDPConfiguration(const QString& name);

    /*!
     * @brief Copy contructor
     *
     * When manipulating data, you create a copy of the configuration, edit it
     * and then transfer its content to the original (using copyFrom() below). Use this
     * contructor to create an editable copy.
     *
     * @param[in] source Original configuration
     */
    UDPConfiguration(UDPConfiguration* source);

    ~UDPConfiguration();

    /*!
     * @brief The UDP port we bind to
     *
     * @return Port number
     */
    quint16 localPort   () { return _localPort; }

    /*!
     * @brief Add a target host
     *
     * @param[in] host Host name in standard formatt, e.g. localhost:14551 or 192.168.1.1:14551
     */
    Q_INVOKABLE void addHost (const QString host);

    /*!
     * @brief Add a target host
     *
     * @param[in] host Host name, e.g. localhost or 192.168.1.1
     * @param[in] port Port number
     */
    void addHost        (const QString& host, quint16 port);

    /*!
     * @brief Remove a target host from the list
     *
     * @param[in] host Host name, e.g. localhost or 192.168.1.1
     */
    Q_INVOKABLE void removeHost  (const QString host);

    /*!
     * @brief Set the UDP port we bind to
     *
     * @param[in] port Port number
     */
    void setLocalPort   (quint16 port);

    /*!
     * @brief QML Interface
     */
    QStringList hostList    () { return _hostList; }

    const QList<UDPCLient*> targetHosts() { return _targetHosts; }

    /// From LinkConfiguration
    LinkType    type                 () { return LinkConfiguration::TypeUdp; }
    void        copyFrom             (LinkConfiguration* source);
    void        loadSettings         (QSettings& settings, const QString& root);
    void        saveSettings         (QSettings& settings, const QString& root);
    void        updateSettings       ();
    bool        isAutoConnectAllowed () { return true; }
    bool        isHighLatencyAllowed () { return true; }
    QString     settingsURL          () { return "UdpSettings.qml"; }
    QString     settingsTitle        () { return tr("UDP Link Settings"); }

signals:
    void localPortChanged   ();
    void hostListChanged    ();

private:
    void _updateHostList    ();
    void _clearTargetHosts  ();
    void _copyFrom          (LinkConfiguration *source);

private:
    QList<UDPCLient*>   _targetHosts;
    QStringList         _hostList;      ///< Exposed to QML
    quint16             _localPort;
};

class UDPLink : public LinkInterface
{
    Q_OBJECT

    friend class UDPConfiguration;
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

public slots:
    void    readBytes               ();

private slots:
    void    _writeBytes             (const QByteArray data) override;

private:
    // Links are only created/destroyed by LinkManager so constructor/destructor is not public
    UDPLink(SharedLinkConfigurationPointer& config);
    ~UDPLink();

    // From LinkInterface
    bool    _connect                (void) override;
    void    _disconnect             (void) override;

    bool    _isIpLocal              (const QHostAddress& add);
    bool    _hardwareConnect        ();
    void    _restartConnection      ();
    void    _registerZeroconf       (uint16_t port, const std::string& regType);
    void    _deregisterZeroconf     ();
    void    _writeDataGram          (const QByteArray data, const UDPCLient* target);

#if defined(QGC_ZEROCONF_ENABLED)
    DNSServiceRef  _dnssServiceRef;
#endif

    bool                    _running;
    QUdpSocket*             _socket;
    UDPConfiguration*       _udpConfig;
    bool                    _connectState;
    QList<UDPCLient*>       _sessionTargets;
    QList<QHostAddress>     _localAddress;

};

