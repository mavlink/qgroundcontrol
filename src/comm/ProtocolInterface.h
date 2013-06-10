/*=====================================================================

PIXHAWK Micro Air Vehicle Flying Robotics Toolkit

(c) 2009, 2010 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>

This file is part of the PIXHAWK project

    PIXHAWK is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PIXHAWK is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with PIXHAWK. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Interface class for protocols
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef _PROTOCOLINTERFACE_H_
#define _PROTOCOLINTERFACE_H_

#include <QThread>
#include <QString>
#include <QByteArray>
#include "LinkInterface.h"

/**
 * @brief Interface for all protocols.
 *
 * This class defines the interface for
 * communication packets transported by the LinkManager.
 *
 * @see LinkManager.
 *
 **/
class ProtocolInterface : public QObject
{
    Q_OBJECT
public:
    virtual ~ProtocolInterface () {}
    virtual QString getName() = 0;
    /**
     * Retrieve a total of all successfully parsed packets for the specified link.
     * @param link The link to return metadata about.
     * @returns -1 if this is not available for this protocol, # of packets otherwise.
     */
    virtual qint32 getReceivedPacketCount(const LinkInterface *link) const = 0;
    /**
     * Retrieve a total of all parsing errors for the specified link.
     * @param link The link to return metadata about.
     * @returns -1 if this is not available for this protocol, # of errors otherwise.
     */
    virtual qint32 getParsingErrorCount(const LinkInterface *link) const = 0;
    /**
     * Retrieve a total of all dropped packets for the specified link.
     * @param link The link to return metadata about.
     * @returns -1 if this is not available for this protocol, # of packets otherwise.
     */
    virtual qint32 getDroppedPacketCount(const LinkInterface *link) const = 0;
    /**
     * Reset the received, error, and dropped counts for the given link. Useful for
     * when reconnecting a link.
     * @param link The link to reset metadata for.
     */
    virtual void resetMetadataForLink(const LinkInterface *link) = 0;

public slots:
    virtual void receiveBytes(LinkInterface *link, QByteArray b) = 0;
    virtual void linkStatusChanged(bool connected) = 0;

signals:
    /** @brief Update the packet loss from one system */
    void receiveLossChanged(int uasId, float loss);

};

#endif // _PROTOCOLINTERFACE_H_
