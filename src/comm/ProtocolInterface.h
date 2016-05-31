/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
class ProtocolInterface : public QThread
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
