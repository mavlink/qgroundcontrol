/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

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

#include "QGCConfig.h"
#include "LinkManager.h"

#define QGC_UDP_LOCAL_PORT  14550
#define QGC_UDP_TARGET_PORT 14555

class UDPConfiguration : public LinkConfiguration
{
public:

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
    void addHost        (const QString& host);

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
    void removeHost     (const QString& host);

    /*!
     * @brief Set the UDP port we bind to
     *
     * @param[in] port Port number
     */
    void setLocalPort   (quint16 port);

    /// From LinkConfiguration
    int  type() { return LinkConfiguration::TypeUdp; }
    void copyFrom(LinkConfiguration* source);
    void loadSettings(QSettings& settings, const QString& root);
    void saveSettings(QSettings& settings, const QString& root);
    void updateSettings();

private:
    QMutex _confMutex;
    QMap<QString, int>::iterator _it;
    QMap<QString, int> _hosts;  ///< ("host", port)
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

    LinkConfiguration* getLinkConfiguration() { return _config; }

public slots:

    /*! @brief Add a new host to broadcast messages to */
    void addHost    (const QString& host);
    /*! @brief Remove a host from broadcasting messages to */
    void removeHost (const QString& host);

    void readBytes();

    /*!
     * @brief Write a number of bytes to the interface.
     *
     * @param data Pointer to the data byte array
     * @param size The size of the bytes array
     **/
    void writeBytes(const char* data, qint64 length);

protected:

    QUdpSocket*         _socket;
    UDPConfiguration*   _config;
    bool                _connectState;

private:
    // Links are only created/destroyed by LinkManager so constructor/destructor is not public
    UDPLink(UDPConfiguration* config);
    ~UDPLink();
    
    // From LinkInterface
    virtual bool _connect(void);
    virtual bool _disconnect(void);

    bool _hardwareConnect();
    void _restartConnection();

signals:
    //Signals are defined by LinkInterface

};

#endif // UDPLINK_H
