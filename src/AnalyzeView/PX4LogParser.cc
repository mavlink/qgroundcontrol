#include "PX4LogParser.h"
#include <math.h>
#include <QtEndian>
#include <QDateTime>

PX4LogParser::PX4LogParser()
{

}

PX4LogParser::~PX4LogParser()
{

}

bool PX4LogParser::getTagsFromLog(QByteArray& log, QList<GeoTagWorker::cameraFeedbackPacket>& cameraFeedback)
{

     // general message header
    char header[] = {(char)0xA3, (char)0x95, (char)0x00};
    // header for GPOS message header
    char gposHeaderHeader[] = {(char)0xA3, (char)0x95, (char)0x80, (char)0x10, (char)0x00};
    int gposHeaderOffset;
    // header for GPOS message
    char gposHeader[] = {(char)0xA3, (char)0x95, (char)0x10, (char)0x00};
    int gposOffsets[3] = {3, 7, 11};
    int gposLengths[3] = {4, 4, 4};
    // header for trigger message header
    char triggerHeaderHeader[] = {(char)0xA3, (char)0x95, (char)0x80, (char)0x37, (char)0x00};
    int triggerHeaderOffset;
    // header for trigger message
    char triggerHeader[] = {(char)0xA3, (char)0x95, (char)0x37, (char)0x00};
    int triggerOffsets[2] = {3, 11};
    int triggerLengths[2] = {8, 4};

    // extract header information: message lengths
    uint8_t* iptr = reinterpret_cast<uint8_t*>(log.mid(log.indexOf(gposHeaderHeader) + 4, 1).data());
    gposHeaderOffset = static_cast<int>(qFromLittleEndian(*iptr));
    iptr = reinterpret_cast<uint8_t*>(log.mid(log.indexOf(triggerHeaderHeader) + 4, 1).data());
    triggerHeaderOffset = static_cast<int>(qFromLittleEndian(*iptr));

    // extract trigger data
    int index = 1;
    int sequence = -1;
    while(index < log.count() - 1) {

        // first extract trigger
        index = log.indexOf(triggerHeader, index + 1);
        // check for whether last entry has been passed
        if (index < 0) {
            break;
        }

        if (log.indexOf(header, index + 1) != index + triggerHeaderOffset) {
            continue;
        }

        GeoTagWorker::cameraFeedbackPacket feedback;
        memset(&feedback, 0, sizeof(feedback));

        uint64_t* time = reinterpret_cast<uint64_t*>(log.mid(index + triggerOffsets[0], triggerLengths[0]).data());
        double timeDouble = static_cast<double>(qFromLittleEndian(*time)) / 1.0e6;
        uint32_t* seq = reinterpret_cast<uint32_t*>(log.mid(index + triggerOffsets[1], triggerLengths[1]).data());
        int seqInt = static_cast<int>(qFromLittleEndian(*seq));
        if (sequence >= seqInt || sequence + 20 < seqInt) { // assume that logging has not skipped more than 20 triggers. this prevents wrong header detection
            continue;
        }
        feedback.timestamp = timeDouble;
        feedback.imageSequence = seqInt;
        sequence = seqInt;

        // second extract position
        bool lookForGpos = true;
        while (lookForGpos) {

            int gposIndex = log.indexOf(gposHeader, index + 1);
            if (gposIndex < 0) {
                cameraFeedback.append(feedback);
                break;
            }
            index = gposIndex;

            // verify that at an offset of gposHeaderOffset the next log message starts
            if (gposIndex + gposHeaderOffset == log.indexOf(header, gposIndex + 1)) {
                int32_t* lat = reinterpret_cast<int32_t*>(log.mid(gposIndex + gposOffsets[0], gposLengths[0]).data());
                feedback.latitude = static_cast<double>(qFromLittleEndian(*lat))/1.0e7;
                int32_t* lon = reinterpret_cast<int32_t*>(log.mid(gposIndex + gposOffsets[1], gposLengths[1]).data());
                feedback.longitude = static_cast<double>(qFromLittleEndian(*lon))/1.0e7;
                feedback.longitude = fmod(180.0 + feedback.longitude, 360.0) - 180.0;
                float* alt = reinterpret_cast<float*>(log.mid(gposIndex + gposOffsets[2], gposLengths[2]).data());
                feedback.altitude = qFromLittleEndian(*alt);
                cameraFeedback.append(feedback);
                break;
            }
        }
    }

    return true;
}
