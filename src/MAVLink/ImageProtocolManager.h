/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "MAVLinkLib.h"

#include <QtCore/QByteArray>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtGui/QImage>

Q_DECLARE_LOGGING_CATEGORY(ImageProtocolManagerLog)

/// Supports the Mavlink image transmission protocol (https://mavlink.io/en/services/image_transmission.html).
/// Mainly used by optical flow cameras.
class ImageProtocolManager : public QObject
{
    Q_OBJECT

public:
    ImageProtocolManager(QObject *parent = nullptr);
    ~ImageProtocolManager();

    uint32_t flowImageIndex() const { return _flowImageIndex; }

    bool requestImage(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t &message);
    void cancelRequest(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t &message);

signals:
    void imageReady(const QImage &image);
    void flowImageIndexChanged(uint32_t index);

public slots:
    void mavlinkMessageReceived(const mavlink_message_t &message);

private:
    QImage _getImage();

    mavlink_data_transmission_handshake_t _imageHandshake{0};
    QByteArray _imageBytes;
    uint32_t _flowImageIndex = 0;
};
