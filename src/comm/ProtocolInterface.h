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

#pragma once

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
    virtual ~ProtocolInterface  () {}
    virtual QString getName     () = 0;
    /**
     * Reset the received, error, and dropped counts for the given link. Useful for
     * when reconnecting a link.
     * @param link The link to reset metadata for.
     */
    virtual void resetMetadataForLink(LinkInterface *link) = 0;

public slots:
    virtual void receiveBytes       (LinkInterface *link, QByteArray b) = 0;
    virtual void linkStatusChanged  (bool connected) = 0;

signals:
    /** @brief Update the packet loss from one system */
    void receiveLossChanged         (int uasId, float loss);

};
