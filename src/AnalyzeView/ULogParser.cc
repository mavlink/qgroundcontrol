#include "ULogParser.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(ULogParserLog, "qgc.analyzeview.ulogparser")

namespace {

#define ULOG_FILE_HEADER_LEN 16
#define ULOG_MSG_HEADER_LEN 3

enum class ULogMessageType : uint8_t {
    FORMAT = 'F',
    DATA = 'D',
    INFO = 'I',
    PARAMETER = 'P',
    ADD_LOGGED_MSG = 'A',
    REMOVE_LOGGED_MSG = 'R',
    SYNC = 'S',
    DROPOUT = 'O',
    LOGGING = 'L',
};

struct ULogMessageHeader {
    uint16_t msgSize;
    uint8_t msgType;
};

struct ULogMessageFormat {
    uint16_t msgSize;
    uint8_t msgType;

    char format[2096];
};

struct ULogMessageAddLogged {
    uint16_t msgSize;
    uint8_t msgType;

    uint8_t multiID;
    uint16_t msgID;
    char msgName[255];
};

constexpr const char _ULogMagic[8] = {'U', 'L', 'o', 'g', static_cast<char>(0x01), static_cast<char>(0x12), static_cast<char>(0x35)};

int sizeOfType(QStringView typeName)
{
    if (typeName == QLatin1String("int8_t") || typeName == QLatin1String("uint8_t")) {
        return 1;

    } else if (typeName == QLatin1String("int16_t") || typeName == QLatin1String("uint16_t")) {
        return 2;

    } else if (typeName == QLatin1String("int32_t") || typeName == QLatin1String("uint32_t")) {
        return 4;

    } else if (typeName == QLatin1String("int64_t") || typeName == QLatin1String("uint64_t")) {
        return 8;

    } else if (typeName == QLatin1String("float")) {
        return 4;

    } else if (typeName == QLatin1String("double")) {
        return 8;

    } else if (typeName == QLatin1String("char") || typeName == QLatin1String("bool")) {
        return 1;
    }

    qCWarning(ULogParserLog) << "Unknown type in ULog : " << typeName;
    return 0;
}

QString extractArraySize(const QString &typeNameFull, int &arraySize)
{
    const qsizetype startPos = typeNameFull.indexOf('[');
    const qsizetype endPos = typeNameFull.indexOf(']');

    if (startPos == -1 || endPos == -1) {
        arraySize = 1;
        return typeNameFull;
    }

    arraySize = typeNameFull.mid(startPos + 1, endPos - startPos - 1).toInt();
    return typeNameFull.mid(0, startPos);
}

int sizeOfFullType(const QString &typeNameFull)
{
    int arraySize;
    const QString typeName = extractArraySize(typeNameFull, arraySize);
    return (sizeOfType(typeName) * arraySize);
}

void parseFieldFormat(const QString &fields, QMap<QString, int> &cameraCaptureOffsets)
{
    int prevFieldEnd = 0;
    int fieldEnd = fields.indexOf(';');
    int offset = 0;

    while (fieldEnd != -1) {
        const int spacePos = fields.indexOf(' ', prevFieldEnd);

        if (spacePos != -1) {
            const QString typeNameFull = fields.mid(prevFieldEnd, spacePos - prevFieldEnd);
            const QString fieldName = fields.mid(spacePos + 1, fieldEnd - spacePos - 1);

            if (!fieldName.contains(QLatin1String("_padding"))) {
                (void) cameraCaptureOffsets.insert(fieldName, offset);
                offset += sizeOfFullType(typeNameFull);
            }
        }

        prevFieldEnd = fieldEnd + 1;
        fieldEnd = fields.indexOf(';', prevFieldEnd);
    }
}

} // namespace

namespace ULogParser {

bool getTagsFromLog(const QByteArray &log, QList<GeoTagWorker::cameraFeedbackPacket> &cameraFeedback, QString &errorMessage)
{
    errorMessage.clear();

    if (!log.contains(_ULogMagic)) {
        errorMessage = QT_TR_NOOP("Could not detect ULog file header magic");
        return false;
    }

    int index = ULOG_FILE_HEADER_LEN;
    bool geotagFound = false;
    QMap<QString /*fieldname*/, int /*fieldOffset*/> cameraCaptureOffsets;
    int cameraCaptureMsgID = -1;

    while(index < (log.length() - 1)) {

        ULogMessageHeader header;
        (void) memset(&header, 0, sizeof(header));
        (void) memcpy(&header, log.constData() + index, ULOG_MSG_HEADER_LEN);

        switch (header.msgType) {
            case static_cast<int>(ULogMessageType::FORMAT):
            {
                ULogMessageFormat format_msg;
                (void) memset(&format_msg, 0, sizeof(format_msg));
                (void) memcpy(&format_msg, log.constData() + index, ULOG_MSG_HEADER_LEN + header.msgSize);

                const QString fmt(format_msg.format);
                const int posSeparator = fmt.indexOf(':');
                const QString messageName = fmt.left(posSeparator);
                const QString messageFields = fmt.mid(posSeparator + 1, header.msgSize - posSeparator - 1);

                if(messageName == QLatin1String("camera_capture")) {
                    parseFieldFormat(messageFields, cameraCaptureOffsets);
                }

                break;
            }

            case static_cast<int>(ULogMessageType::ADD_LOGGED_MSG):
            {
                ULogMessageAddLogged addLoggedMsg;
                (void) memset(&addLoggedMsg, 0, sizeof(addLoggedMsg));
                (void) memcpy(&addLoggedMsg, log.constData() + index, ULOG_MSG_HEADER_LEN + header.msgSize);

                const QString messageName(addLoggedMsg.msgName);

                if(messageName.contains(QLatin1String("camera_capture"))) {
                    cameraCaptureMsgID = addLoggedMsg.msgID;
                    geotagFound = true;
                }

                break;
            }

            case static_cast<int>(ULogMessageType::DATA):
            {
                uint16_t msgID = -1;
                (void) memcpy(&msgID, log.constData() + index + ULOG_MSG_HEADER_LEN, 2);

                if (geotagFound && (msgID == cameraCaptureMsgID)) {
                    // Completely dynamic parsing, so that changing/reordering the message format will not break the parser
                    GeoTagWorker::cameraFeedbackPacket feedback;
                    (void) memset(&feedback, 0, sizeof(feedback));

                    const char* const data = log.constData() + index + 5;

                    (void) memcpy(&feedback.timestamp, data + cameraCaptureOffsets.value(QStringLiteral("timestamp")), 8);
                    feedback.timestamp /= 1.0e6; // to seconds

                    (void) memcpy(&feedback.timestampUTC, data + cameraCaptureOffsets.value(QStringLiteral("timestamp_utc")), 8);
                    feedback.timestampUTC /= 1.0e6; // to seconds

                    (void) memcpy(&feedback.imageSequence, data + cameraCaptureOffsets.value(QStringLiteral("seq")), 4);
                    (void) memcpy(&feedback.latitude, data + cameraCaptureOffsets.value(QStringLiteral("lat")), 8);
                    (void) memcpy(&feedback.longitude, data + cameraCaptureOffsets.value(QStringLiteral("lon")), 8);

                    feedback.longitude = fmod(180.0 + feedback.longitude, 360.0) - 180.0;

                    (void) memcpy(&feedback.altitude, data + cameraCaptureOffsets.value(QStringLiteral("alt")), 4);
                    (void) memcpy(&feedback.groundDistance, data + cameraCaptureOffsets.value(QStringLiteral("ground_distance")), 4);
                    (void) memcpy(&feedback.captureResult, data + cameraCaptureOffsets.value(QStringLiteral("result")), 1);

                    (void) cameraFeedback.append(feedback);
                }

                break;
            }

            default:
                break;
        }

        index += (3 + header.msgSize);

    }

    if (cameraFeedback.isEmpty()) {
        errorMessage = QT_TR_NOOP("Could not detect camera_capture packets in ULog");
        return false;
    }

    return true;
}

} // namespace ULogParser
