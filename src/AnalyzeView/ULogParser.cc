 #include "ULogParser.h"
#include <math.h>
#include <QDateTime>

ULogParser::ULogParser()
{

}

ULogParser::~ULogParser()
{

}

int ULogParser::sizeOfType(QString& typeName)
{
    if (typeName == "int8_t" || typeName == "uint8_t") {
        return 1;

    } else if (typeName == "int16_t" || typeName == "uint16_t") {
        return 2;

    } else if (typeName == "int32_t" || typeName == "uint32_t") {
        return 4;

    } else if (typeName == "int64_t" || typeName == "uint64_t") {
        return 8;

    } else if (typeName == "float") {
        return 4;

    } else if (typeName == "double") {
        return 8;

    } else if (typeName == "char" || typeName == "bool") {
        return 1;
    }

    qWarning() << "Unkown type in ULog : " << typeName;
    return 0;
}

int ULogParser::sizeOfFullType(QString& typeNameFull)
{
    int arraySize;
    QString typeName = extractArraySize(typeNameFull, arraySize);
    return sizeOfType(typeName) * arraySize;
}

QString ULogParser::extractArraySize(QString &typeNameFull, int &arraySize)
{
    int startPos = typeNameFull.indexOf('[');
    int endPos = typeNameFull.indexOf(']');

    if (startPos == -1 || endPos == -1) {
        arraySize = 1;
        return typeNameFull;
    }

    arraySize = typeNameFull.mid(startPos + 1, endPos - startPos - 1).toInt();
    return typeNameFull.mid(0, startPos);
}

bool ULogParser::parseFieldFormat(QString& fields)
{
    int prevFieldEnd = 0;
    int fieldEnd = fields.indexOf(';');
    int offset = 0;

    while (fieldEnd != -1) {
        int spacePos = fields.indexOf(' ', prevFieldEnd);

        if (spacePos != -1) {
            QString typeNameFull = fields.mid(prevFieldEnd, spacePos - prevFieldEnd);
            QString fieldName = fields.mid(spacePos + 1, fieldEnd - spacePos - 1);

            if (!fieldName.contains("_padding")) {
                _cameraCaptureOffsets.insert(fieldName, offset);
                offset += sizeOfFullType(typeNameFull);
            }
        }

        prevFieldEnd = fieldEnd + 1;
        fieldEnd = fields.indexOf(';', prevFieldEnd);
    }
    return false;
}

bool ULogParser::getTagsFromLog(QByteArray& log, QList<GeoTagWorker::cameraFeedbackPacket>& cameraFeedback)
{
    //verify it's an ULog file
    if(!log.contains(_ULogMagic)) {
        qWarning() << "Could not detect ULog file header magic";
        return false;
    }

    int index = ULOG_FILE_HEADER_LEN;
    bool geotagFound = false;

    while(index < log.count() - 1) {

        ULogMessageHeader header{};
        memcpy(&header, log.data() + index, ULOG_MSG_HEADER_LEN);

        switch (header.msgType) {
            case (int)ULogMessageType::FORMAT:
            {
                ULogMessageFormat format_msg{};
                memcpy(&format_msg, log.data() + index, ULOG_MSG_HEADER_LEN + header.msgSize);

                QString fmt(format_msg.format);
                int posSeparator = fmt.indexOf(':');
                QString messageName = fmt.left(posSeparator);
                QString messageFields = fmt.mid(posSeparator + 1, header.msgSize - posSeparator - 1);

                if(messageName == "camera_capture") {
                    parseFieldFormat(messageFields);
                }
                break;
            }

            case (int)ULogMessageType::ADD_LOGGED_MSG:
            {
                ULogMessageAddLogged addLoggedMsg{};
                memcpy(&addLoggedMsg, log.data() + index, ULOG_MSG_HEADER_LEN + header.msgSize);

                QString messageName(addLoggedMsg.msgName);

                if(messageName.contains("camera_capture")) {
                    _cameraCaptureMsgID = addLoggedMsg.msgID;
                    geotagFound = true;
                }

                break;
            }

            case (int)ULogMessageType::DATA:
            {
                if (!geotagFound) {
                    qWarning() << "Could not detect geotag packets in ULog";
                    return false;
                }

                uint16_t msgID = -1;
                memcpy(&msgID, log.data() + index + ULOG_MSG_HEADER_LEN, 2);

                if(msgID == _cameraCaptureMsgID) {

                    // Completely dynamic parsing, so that changing/reordering the message format will not break the parser
                    GeoTagWorker::cameraFeedbackPacket feedback{};
                    memcpy(&feedback.timestamp, log.data() + index + 5 + _cameraCaptureOffsets.value("timestamp"), 8);
                    feedback.timestamp /= 1.0e6; // to seconds
                    memcpy(&feedback.timestampUTC, log.data() + index + 5 + _cameraCaptureOffsets.value("timestamp_utc"), 8);
                    feedback.timestampUTC /= 1.0e6; // to seconds
                    memcpy(&feedback.imageSequence, log.data() + index + 5 + _cameraCaptureOffsets.value("seq"), 4);
                    memcpy(&feedback.latitude, log.data() + index + 5 + _cameraCaptureOffsets.value("lat"), 8);
                    memcpy(&feedback.longitude, log.data() + index + 5 + _cameraCaptureOffsets.value("lon"), 8);
                    feedback.longitude = fmod(180.0 + feedback.longitude, 360.0) - 180.0;
                    memcpy(&feedback.altitude, log.data() + index + 5 + _cameraCaptureOffsets.value("alt"), 4);
                    memcpy(&feedback.groundDistance, log.data() + index + 5 + _cameraCaptureOffsets.value("ground_distance"), 4);
                    memcpy(&feedback.captureResult, log.data() + index + 5 + _cameraCaptureOffsets.value("result"), 1);

                    cameraFeedback.append(feedback);

                }

                break;
            }

            default:
                break;
        }

        index += (3 + header.msgSize);

    }

    return true;
}
