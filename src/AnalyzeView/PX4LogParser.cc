/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "PX4LogParser.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QtEndian>

QGC_LOGGING_CATEGORY(PX4LogParserLog, "qgc.analyzeview.px4logparser")

// general message header
static constexpr const char header[3] = {static_cast<char>(0xA3), static_cast<char>(0x95), static_cast<char>(0x00)};

// header for GPOS message header
static constexpr const char gposHeaderHeader[5] = {static_cast<char>(0xA3), static_cast<char>(0x95), static_cast<char>(0x80), static_cast<char>(0x10), static_cast<char>(0x00)};

// header for GPOS message
static constexpr const char gposHeader[4] = {static_cast<char>(0xA3), static_cast<char>(0x95), static_cast<char>(0x10), static_cast<char>(0x00)};
static constexpr const int gposOffsets[3] = {3, 7, 11};
static constexpr const int gposLengths[3] = {4, 4, 4};

// header for trigger message header
static constexpr const char triggerHeaderHeader[] = {static_cast<char>(0xA3), static_cast<char>(0x95), static_cast<char>(0x80), static_cast<char>(0x37), static_cast<char>(0x00)};

// header for trigger message
static constexpr const char triggerHeader[4] = {static_cast<char>(0xA3), static_cast<char>(0x95), static_cast<char>(0x37), static_cast<char>(0x00)};
static constexpr const int triggerOffsets[2] = {3, 11};
static constexpr const int triggerLengths[2] = {8, 4};

namespace PX4LogParser {

bool getTagsFromLog(const QByteArray& log, QList<GeoTagWorker::CameraFeedbackPacket>& cameraFeedback)
{
    // extract header information: message lengths
    const uint8_t* iptr = reinterpret_cast<const uint8_t*>(log.mid(log.indexOf(gposHeaderHeader) + 4, 1).constData());
    const int gposHeaderOffset = static_cast<int>(qFromLittleEndian(*iptr));

    iptr = reinterpret_cast<const uint8_t*>(log.mid(log.indexOf(triggerHeaderHeader) + 4, 1).constData());
    const int triggerHeaderOffset = static_cast<int>(qFromLittleEndian(*iptr));

    // extract trigger data
    int index = 1;
    int sequence = -1;
    while(index < log.length() - 1) {
        // first extract trigger
        index = log.indexOf(triggerHeader, index + 1);

        // check for whether last entry has been passed
        if (index < 0) {
            break;
        }

        if (log.indexOf(header, index + 1) != (index + triggerHeaderOffset)) {
            continue;
        }

        GeoTagWorker::CameraFeedbackPacket feedback;
        (void) memset(&feedback, 0, sizeof(feedback));

        const uint64_t* const time = reinterpret_cast<const uint64_t*>(log.mid(index + triggerOffsets[0], triggerLengths[0]).data());
        const double timeDouble = static_cast<double>(qFromLittleEndian(*time)) / 1.0e6;
        const uint32_t* const seq = reinterpret_cast<const uint32_t*>(log.mid(index + triggerOffsets[1], triggerLengths[1]).data());
        const int seqInt = static_cast<int>(qFromLittleEndian(*seq));
        // Assume that logging has not skipped more than 20 triggers. This prevents wrong header detection.
        if ((sequence >= seqInt) || ((sequence + 20) < seqInt)) {
            continue;
        }

        feedback.timestamp = timeDouble;
        feedback.imageSequence = seqInt;
        sequence = seqInt;

        // second extract position
        while (true) {
            const int gposIndex = log.indexOf(gposHeader, index + 1);
            if (gposIndex < 0) {
                (void) cameraFeedback.append(feedback);
                break;
            }
            index = gposIndex;

            // verify that at an offset of gposHeaderOffset the next log message starts
            if ((gposIndex + gposHeaderOffset) == log.indexOf(header, gposIndex + 1)) {
                const int32_t* const lat = reinterpret_cast<const int32_t*>(log.mid(gposIndex + gposOffsets[0], gposLengths[0]).constData());
                feedback.latitude = static_cast<double>(qFromLittleEndian(*lat)) / 1.0e7;

                const int32_t* const lon = reinterpret_cast<const int32_t*>(log.mid(gposIndex + gposOffsets[1], gposLengths[1]).constData());
                feedback.longitude = static_cast<double>(qFromLittleEndian(*lon)) / 1.0e7;
                feedback.longitude = fmod(180.0 + feedback.longitude, 360.0) - 180.0;

                const float* const alt = reinterpret_cast<const float*>(log.mid(gposIndex + gposOffsets[2], gposLengths[2]).constData());
                feedback.altitude = qFromLittleEndian(*alt);

                (void) cameraFeedback.append(feedback);
                break;
            }
        }
    }

    return true;
}

} // namespace PX4LogParser
