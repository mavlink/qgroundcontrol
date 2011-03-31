/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

/**
 * @file
 *   @brief Definition of class QGCNMEAProtocol
 *   @author Lorenz Meier <mail@qgroundcontrol.org>
 */

#ifndef QGCNMEAPROTOCOL_H_
#define QGCNMEAPROTOCOL_H_

#include <QObject>
#include <QMutex>
#include <QString>
#include <QByteArray>
#include <QMap>
#include "nmea/parse.h"
#include "ProtocolInterface.h"
#include "LinkInterface.h"

/**
 * @brief MAVLink micro air vehicle protocol reference implementation.
 *
 * MAVLink is a generic communication protocol for micro air vehicles.
 * for more information, please see the official website.
 * @ref http://pixhawk.ethz.ch/software/mavlink/
 **/
class QGCNMEAProtocol : public ProtocolInterface
{
    Q_OBJECT

public:
    QGCNMEAProtocol();
    ~QGCNMEAProtocol();

    void run();
    /** @brief Get the human-friendly name of this protocol */
    QString getName() {
        return QString("NMEA (GPS)");
    }

public slots:
    /** @brief Receive bytes from a communication interface */
    void receiveBytes(LinkInterface* link, QByteArray b);

protected:
    QMutex receiveMutex;       ///< Mutex to protect receiveBytes function
    QMap

signals:
    /** @brief Message received and directly copied via signal */
    void positionReceived(double lat, double lon, double alt);
    /** @brief Emitted if a message from the protocol should reach the user */
    void protocolStatusMessage(const QString& title, const QString& message);
};

#endif // QGCNMEAPROTOCOL_H_
