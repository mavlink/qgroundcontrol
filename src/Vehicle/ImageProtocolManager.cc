/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ImageProtocolManager.h"
#include "QGC.h"
#include "MAVLinkProtocol.h"
#include "Vehicle.h"
#include "QGCApplication.h"

#include <QFile>
#include <QDir>
#include <string>

QGC_LOGGING_CATEGORY(ImageProtocolManagerLog, "ImageProtocolManagerLog")

ImageProtocolManager::ImageProtocolManager(void)
{
    memset(&_imageHandshake, 0, sizeof(_imageHandshake));
}

void ImageProtocolManager::mavlinkMessageReceived(const mavlink_message_t& message)
{
    switch (message.msgid) {
    case MAVLINK_MSG_ID_DATA_TRANSMISSION_HANDSHAKE:
    {
        if (_imageHandshake.packets) {
            qCWarning(ImageProtocolManagerLog) << "DATA_TRANSMISSION_HANDSHAKE: Previous image transmission incomplete.";
        }
        _imageBytes.clear();
        mavlink_msg_data_transmission_handshake_decode(&message, &_imageHandshake);
        qCDebug(ImageProtocolManagerLog) << QStringLiteral("DATA_TRANSMISSION_HANDSHAKE: type(%1) width(%2) height (%3)").arg(_imageHandshake.type).arg(_imageHandshake.width).arg(_imageHandshake.height);
    }
        break;

    case MAVLINK_MSG_ID_ENCAPSULATED_DATA:
    {
        if (_imageHandshake.packets == 0) {
            qCWarning(ImageProtocolManagerLog) << "ENCAPSULATED_DATA: received with no prior DATA_TRANSMISSION_HANDSHAKE.";
            break;
        }

        mavlink_encapsulated_data_t encapsulatedData;
        mavlink_msg_encapsulated_data_decode(&message, &encapsulatedData);

        int bytePosition = encapsulatedData.seqnr * _imageHandshake.payload;
        if (bytePosition >= static_cast<int>(_imageHandshake.size)) {
            qCWarning(ImageProtocolManagerLog) << "ENCAPSULATED_DATA: seqnr is past end of image size. seqnr:" << encapsulatedData.seqnr << "_imageHandshake.size" << _imageHandshake.size;
            break;
        }

        for (uint8_t i=0; i<_imageHandshake.payload; i++) {
            _imageBytes[bytePosition] = encapsulatedData.data[i];
            bytePosition++;
        }

        // We use the packets field to track completion
        _imageHandshake.packets--;

        if (_imageHandshake.packets == 0) {
            // We have all the packets
            emit imageReady();
        }
    }
        break;

    default:
        break;
    }
}

QImage ImageProtocolManager::getImage(void)
{
    QImage image;

    if (_imageBytes.isEmpty()) {
        qCWarning(ImageProtocolManagerLog) << "getImage: Called when no image available";
    } else if (_imageHandshake.packets) {
        qCWarning(ImageProtocolManagerLog) << "getImage: Called when image is imcomplete. _imageHandshake.packets:" << _imageHandshake.packets;
    } else {
        switch (_imageHandshake.type) {
        case MAVLINK_DATA_STREAM_IMG_RAW8U:
        case MAVLINK_DATA_STREAM_IMG_RAW32U:
        {
            // Construct PGM header
            QString header("P5\n%1 %2\n%3\n");
            header = header.arg(_imageHandshake.width).arg(_imageHandshake.height).arg(255 /* image colors */);

            QByteArray tmpImage(header.toStdString().c_str(), header.length());
            tmpImage.append(_imageBytes);

            if (!image.loadFromData(tmpImage, "PGM")) {
                qCWarning(ImageProtocolManagerLog) << "getImage: IMG_RAW8U QImage::loadFromData failed";
            }
        }
            break;

        case MAVLINK_DATA_STREAM_IMG_BMP:
        case MAVLINK_DATA_STREAM_IMG_JPEG:
        case MAVLINK_DATA_STREAM_IMG_PGM:
        case MAVLINK_DATA_STREAM_IMG_PNG:
            if (!image.loadFromData(_imageBytes)) {
                qCWarning(ImageProtocolManagerLog) << "getImage: Known header QImage::loadFromData failed";
            }
            break;

        default:
            qCWarning(ImageProtocolManagerLog) << "getImage: Unsupported image type:" << _imageHandshake.type;
            break;
        }
    }

    return image;
}
