/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QByteArray>
#include <QImage>

#include "QGCLoggingCategory.h"
#include "QGCMAVLink.h"

Q_DECLARE_LOGGING_CATEGORY(ImageProtocolManagerLog)

// Supports the Mavlink image transmission protocol (https://mavlink.io/en/services/image_transmission.html).
// Mainly used by optical flow cameras.
class ImageProtocolManager : public QObject
{
    Q_OBJECT
    
public:
    ImageProtocolManager(void);

    void    mavlinkMessageReceived  (const mavlink_message_t& message);
    QImage  getImage                (void);

signals:
    void imageReady(void);

private:
    mavlink_data_transmission_handshake_t   _imageHandshake;
    QByteArray                              _imageBytes;

};
