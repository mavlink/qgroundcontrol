/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

#ifndef UDPLINK_H
#define UDPLINK_H

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

#define QGC_UDP_LOCAL_PORT  14550
#define QGC_UDP_TARGET_PORT 14555

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

    /*!
     * @brief Begin iteration through the list of target hosts
     *
     * @param[out] host Host name
     * @param[out] port Port number
     * @return Returns false if list is empty
     */
    bool firstHost      (QString& host, int& port);

    /*!
     * @brief Continues iteration through the list of target hosts
     *
     * @param[out] host Host name
     * @param[out] port Port number
     * @return Returns false if reached the end of the list (in which case, both host and port are unchanged)
     */
    bool nextHost       (QString& host, int& port);

    /*!
     * @brief Get the number of target hosts
     *
     * @return Number of hosts in list
     */
    int hostCount       () { return _hosts.count(); }

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
    void addHost        (const QString& host, int port);

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

    /// From LinkConfiguration
    LinkType    type            () { return LinkConfiguration::TypeUdp; }
    void        copyFrom        (LinkConfiguration* source);
    void        loadSettings    (QSettings& settings, const QString& root);
    void        saveSettings    (QSettings& settings, const QString& root);
    void        updateSettings  ();
    QString     settingsURL     () { return "UdpSettings.qml"; }

signals:
    void localPortChanged   ();
    void hostListChanged    ();

private:
    void _updateHostList    ();

private:
    QMutex _confMutex;
    QMap<QString, int>::iterator _it;
    QMap<QString, int> _hosts;  ///< ("host", port)
    QStringList _hostList;      ///< Exposed to QML
    quint16 _localPort;
};

class UDPLink : public LinkInterface
{
    Q_OBJECT

    friend class UDPConfiguration;
    friend class LinkManager;

public:
    void requestReset() { }
    bool isConnected() const;
    QString getName() const;

    // Extensive statistics for scientific purposes
    qint64 getConnectionSpeed() const;
    qint64 getCurrentInDataRate() const;
    qint64 getCurrentOutDataRate() const;

    void run();

    // These are left unimplemented in order to cause linker errors which indicate incorrect usage of
    // connect/disconnect on link directly. All connect/disconnect calls should be made through LinkManager.
    bool connect(void);
    bool disconnect(void);

public slots:
    /*! @brief Add a new host to broadcast messages to */
    void addHost    (const QString& host);
    /*! @brief Remove a host from broadcasting messages to */
    void removeHost (const QString& host);

    void readBytes();

private slots:
    void _writeBytes(const QByteArray data);

private:
    // Links are only created/destroyed by LinkManager so constructor/destructor is not public
    UDPLink(SharedLinkConfigurationPointer& config);
    ~UDPLink();

    // From LinkInterface
    virtual bool _connect(void);
    virtual void _disconnect(void);

    bool _hardwareConnect();
    void _restartConnection();

    void _registerZeroconf(uint16_t port, const std::string& regType);
    void _deregisterZeroconf();

#if defined(QGC_ZEROCONF_ENABLED)
    DNSServiceRef  _dnssServiceRef;
#endif

    bool                _running;
    QUdpSocket*         _socket;
    UDPConfiguration*   _udpConfig;
    bool                _connectState;
};

#endif // UDPLINK_H
