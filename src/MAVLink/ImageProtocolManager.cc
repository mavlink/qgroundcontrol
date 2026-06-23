#include "ImageProtocolManager.h"
#include "QGCLoggingCategory.h"

#include <cstring>

QGC_LOGGING_CATEGORY(ImageProtocolManagerLog, "MAVLink.ImageProtocolManager")

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
    constexpr mavlink_data_transmission_handshake_t data{};
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

        // Validate handshake fields to prevent out-of-bounds writes on subsequent ENCAPSULATED_DATA
        static constexpr uint32_t kMaxImageSize = 1u * 1024u * 1024u; // 1 MB upper bound (optical flow images are typically small grayscale frames)
        if (_imageHandshake.size == 0 || _imageHandshake.payload == 0 || _imageHandshake.packets == 0) {
            qCWarning(ImageProtocolManagerLog) << "DATA_TRANSMISSION_HANDSHAKE: Invalid field(s) - size:" << _imageHandshake.size
                                               << "payload:" << _imageHandshake.payload << "packets:" << _imageHandshake.packets;
            _imageHandshake = {};
            break;
        }
        if (_imageHandshake.size > kMaxImageSize) {
            qCWarning(ImageProtocolManagerLog) << "DATA_TRANSMISSION_HANDSHAKE: Image size exceeds limit. size:" << _imageHandshake.size;
            _imageHandshake = {};
            break;
        }
        if (_imageHandshake.payload > sizeof(mavlink_encapsulated_data_t::data)) {
            qCWarning(ImageProtocolManagerLog) << "DATA_TRANSMISSION_HANDSHAKE: payload exceeds ENCAPSULATED_DATA data field size. payload:" << _imageHandshake.payload;
            _imageHandshake = {};
            break;
        }

        _imageBytes.resize(_imageHandshake.size, '\0');
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

        const uint32_t bytePosition = static_cast<uint32_t>(encapsulatedData.seqnr) * _imageHandshake.payload;
        if (bytePosition >= _imageHandshake.size) {
            qCWarning(ImageProtocolManagerLog) << "ENCAPSULATED_DATA: seqnr is past end of image size. seqnr:" << encapsulatedData.seqnr << "_imageHandshake.size:" << _imageHandshake.size;
            break;
        }

        // Clamp the number of bytes to copy so we never write past the declared image size
        const uint32_t bytesRemaining = _imageHandshake.size - bytePosition;
        const uint32_t bytesToCopy = qMin(static_cast<uint32_t>(_imageHandshake.payload), bytesRemaining);

        (void) memcpy(_imageBytes.data() + bytePosition, encapsulatedData.data, bytesToCopy);

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
