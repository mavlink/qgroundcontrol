#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QLoggingCategory>

#include "GeoTagWorker.h"

Q_DECLARE_LOGGING_CATEGORY(PX4LogParserLog)

class PX4LogParser
{
public:
    PX4LogParser() = default;
    ~PX4LogParser() = default;

    bool getTagsFromLog(QByteArray& log, QList<GeoTagWorker::cameraFeedbackPacket>& cameraFeedback);

private:
    // general message header
    static constexpr const char header[] = {(char)0xA3, (char)0x95, (char)0x00};

    // header for GPOS message header
    static constexpr const char gposHeaderHeader[] = {(char)0xA3, (char)0x95, (char)0x80, (char)0x10, (char)0x00};

    // header for GPOS message
    static constexpr const char gposHeader[] = {(char)0xA3, (char)0x95, (char)0x10, (char)0x00};
    static constexpr const int gposOffsets[3] = {3, 7, 11};
    static constexpr const int gposLengths[3] = {4, 4, 4};

    // header for trigger message header
    static constexpr const char triggerHeaderHeader[] = {(char)0xA3, (char)0x95, (char)0x80, (char)0x37, (char)0x00};

    // header for trigger message
    static constexpr const char triggerHeader[] = {(char)0xA3, (char)0x95, (char)0x37, (char)0x00};
    static constexpr const int triggerOffsets[2] = {3, 11};
    static constexpr const int triggerLengths[2] = {8, 4};
};
