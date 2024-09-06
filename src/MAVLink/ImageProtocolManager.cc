/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ImageProtocolManager.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(ImageProtocolManagerLog, "qgc.mavlink.imageprotocolmanager")

ImageProtocolManager::ImageProtocolManager(QObject *parent)
    : QObject(parent)
{
    // qCDebug(ImageProtocolManagerLog) << Q_FUNC_INFO << this;
}

ImageProtocolManager::~ImageProtocolManager()
{
    // qCDebug(ImageProtocolManagerLog) << Q_FUNC_INFO << this;
}

bool ImageProtocolManager::requestImage(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t &message)
{
    // Check if there is already an image transmission going on
    if (_imageHandshake.packets != 0) {
        return false;
    }

    constexpr mavlink_data_transmission_handshake_t data = {
        0, 0, 0, 0,
        MAVLINK_DATA_STREAM_IMG_JPEG,
        0,
        50
    };
    (void) mavlink_msg_data_transmission_handshake_encode_chan(system_id, component_id, chan, &message, &data);

    return true;
}

void ImageProtocolManager::cancelRequest(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t &message)
{
    constexpr mavlink_data_transmission_handshake_t data{0};
    (void) mavlink_msg_data_transmission_handshake_encode_chan(system_id, component_id, chan, &message, &data);
}

void ImageProtocolManager::mavlinkMessageReceived(const mavlink_message_t &message)
{
    switch (message.msgid) {
    case MAVLINK_MSG_ID_DATA_TRANSMISSION_HANDSHAKE:
    {
        if (_imageHandshake.packets > 0) {
            qCWarning(ImageProtocolManagerLog) << "DATA_TRANSMISSION_HANDSHAKE: Previous image transmission incomplete.";
        }
        _imageBytes.clear();
        mavlink_msg_data_transmission_handshake_decode(&message, &_imageHandshake);
        qCDebug(ImageProtocolManagerLog) << QStringLiteral("DATA_TRANSMISSION_HANDSHAKE: type(%1) width(%2) height (%3)").arg(_imageHandshake.type).arg(_imageHandshake.width).arg(_imageHandshake.height);
        break;
    }
    case MAVLINK_MSG_ID_ENCAPSULATED_DATA:
    {
        if (_imageHandshake.packets == 0) {
            qCWarning(ImageProtocolManagerLog) << "ENCAPSULATED_DATA: received with no prior DATA_TRANSMISSION_HANDSHAKE.";
            break;
        }

        mavlink_encapsulated_data_t encapsulatedData;
        mavlink_msg_encapsulated_data_decode(&message, &encapsulatedData);

        uint32_t bytePosition = encapsulatedData.seqnr * _imageHandshake.payload;
        if (bytePosition >= _imageHandshake.size) {
            qCWarning(ImageProtocolManagerLog) << "ENCAPSULATED_DATA: seqnr is past end of image size. seqnr:" << encapsulatedData.seqnr << "_imageHandshake.size:" << _imageHandshake.size;
            break;
        }

        for (uint8_t i = 0; i < _imageHandshake.payload; i++) {
            _imageBytes[bytePosition] = encapsulatedData.data[i];
            bytePosition++;
        }

        // We use the packets field to track completion
        _imageHandshake.packets--;
        if (_imageHandshake.packets == 0) {
            // We have all the packets
            emit imageReady(_getImage());

            _flowImageIndex++;
            emit flowImageIndexChanged(_flowImageIndex);
        }
        break;
    }
    default:
        break;
    }
}

QImage ImageProtocolManager::_getImage()
{
    QImage image;

    if (_imageBytes.isEmpty()) {
        qCWarning(ImageProtocolManagerLog) << Q_FUNC_INFO << "Called when no image available";
        return image;
    }

    if (_imageHandshake.packets > 0) {
        qCWarning(ImageProtocolManagerLog) << Q_FUNC_INFO << "Called when image is imcomplete. _imageHandshake.packets:" << _imageHandshake.packets;
        return image;
    }

    switch (_imageHandshake.type) {
    case MAVLINK_DATA_STREAM_IMG_RAW8U:
    case MAVLINK_DATA_STREAM_IMG_RAW32U:
    {
        // Construct PGM header
        const QString header = QStringLiteral("P5\n%1 %2\n255\n").arg(_imageHandshake.width).arg(_imageHandshake.height);

        QByteArray tempImage(header.toStdString().c_str(), header.length());
        (void) tempImage.append(_imageBytes);

        if (!image.loadFromData(tempImage, "PGM")) {
            qCWarning(ImageProtocolManagerLog) << Q_FUNC_INFO << "IMG_RAW8U QImage::loadFromData failed";
        }
        break;
    }
    case MAVLINK_DATA_STREAM_IMG_BMP:
    case MAVLINK_DATA_STREAM_IMG_JPEG:
    case MAVLINK_DATA_STREAM_IMG_PGM:
    case MAVLINK_DATA_STREAM_IMG_PNG:
        if (!image.loadFromData(_imageBytes)) {
            qCWarning(ImageProtocolManagerLog) << Q_FUNC_INFO << "Known header QImage::loadFromData failed";
        }
        break;

    default:
        qCWarning(ImageProtocolManagerLog) << Q_FUNC_INFO << "Unsupported image type:" << _imageHandshake.type;
        break;
    }

    return image;
}
