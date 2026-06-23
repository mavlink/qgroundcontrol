#include "DataFlashTestGenerator.h"

#include <QtCore/QDataStream>
#include <QtCore/QFile>

#include <cstring>

namespace DataFlashTestGenerator {

namespace {

// DataFlash constants
constexpr uint8_t kHeaderByte1 = 0xA3;
constexpr uint8_t kHeaderByte2 = 0x95;
constexpr uint8_t kFmtMessageType = 128;
constexpr uint8_t kCamMessageType = 129;

// FMT message is 86 bytes after header (89 total)
// Type(1) + Length(1) + Name(4) + Format(16) + Columns(64) = 86 bytes payload

void writeByte(QDataStream& stream, uint8_t value)
{
    stream.writeRawData(reinterpret_cast<const char*>(&value), 1);
}

void writeHeader(QDataStream& stream)
{
    writeByte(stream, kHeaderByte1);
    writeByte(stream, kHeaderByte2);
}

void writeFmtMessage(QDataStream& stream, uint8_t type, uint8_t length, const char* name, const char* format,
                     const char* columns)
{
    writeHeader(stream);
    writeByte(stream, kFmtMessageType);

    writeByte(stream, type);
    writeByte(stream, length);

    // Name (4 bytes)
    char nameBuf[4] = {0};
    strncpy(nameBuf, name, 4);
    stream.writeRawData(nameBuf, 4);

    // Format (16 bytes)
    char formatBuf[16] = {0};
    strncpy(formatBuf, format, 16);
    stream.writeRawData(formatBuf, 16);

    // Columns (64 bytes)
    char columnsBuf[64] = {0};
    strncpy(columnsBuf, columns, 64);
    stream.writeRawData(columnsBuf, 64);
}

void writeCamMessage(QDataStream& stream, const CameraCaptureEvent& event)
{
    writeHeader(stream);
    writeByte(stream, kCamMessageType);

    // TimeUS (Q = uint64)
    stream << static_cast<quint64>(event.timestamp_us);

    // Img (I = uint32)
    stream << static_cast<quint32>(event.seq);

    // Lat (L = int32 * 1e7)
    qint32 lat = static_cast<qint32>(event.coordinate.latitude() * 1.0e7);
    stream << lat;

    // Lng (L = int32 * 1e7)
    qint32 lon = static_cast<qint32>(event.coordinate.longitude() * 1.0e7);
    stream << lon;

    // Alt (f = float)
    float alt = static_cast<float>(event.coordinate.altitude());
    stream.writeRawData(reinterpret_cast<const char*>(&alt), sizeof(alt));

    // R (f = float)
    stream.writeRawData(reinterpret_cast<const char*>(&event.roll), sizeof(event.roll));

    // P (f = float)
    stream.writeRawData(reinterpret_cast<const char*>(&event.pitch), sizeof(event.pitch));

    // Y (f = float)
    stream.writeRawData(reinterpret_cast<const char*>(&event.yaw), sizeof(event.yaw));
}

}  // namespace

bool generateDataFlashLog(const QString& filename, const QList<CameraCaptureEvent>& events)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    // Write FMT message for FMT itself
    // FMT: Type=128, Length=89, Name="FMT", Format="BBnNZ", Columns="Type,Length,Name,Format,Columns"
    writeFmtMessage(stream, kFmtMessageType, 89, "FMT", "BBnNZ", "Type,Length,Name,Format,Columns");

    // Write FMT message for CAM
    // CAM: TimeUS(Q) + Img(I) + Lat(L) + Lng(L) + Alt(f) + R(f) + P(f) + Y(f)
    // Total: 8 + 4 + 4 + 4 + 4 + 4 + 4 + 4 = 36 bytes + 3 header = 39 bytes
    writeFmtMessage(stream, kCamMessageType, 39, "CAM", "QILLffff", "TimeUS,Img,Lat,Lng,Alt,R,P,Y");

    // Write CAM messages
    for (const CameraCaptureEvent& event : events) {
        writeCamMessage(stream, event);
    }

    file.close();
    return true;
}

QList<CameraCaptureEvent> generateSampleEvents(int count, uint64_t startTimestamp_us, double intervalSec)
{
    QList<CameraCaptureEvent> events;
    events.reserve(count);

    // Start near San Francisco
    constexpr double baseLat = 37.7749;
    constexpr double baseLon = -122.4194;
    constexpr double baseAlt = 100.0;

    for (int i = 0; i < count; ++i) {
        CameraCaptureEvent event;
        event.timestamp_us = startTimestamp_us + static_cast<uint64_t>(i * intervalSec * 1e6);
        event.seq = static_cast<uint32_t>(i);

        // Generate coordinates in a line moving north-east
        const double lat = baseLat + (i * 0.0001);  // ~11m per step
        const double lon = baseLon + (i * 0.0001);
        const double alt = baseAlt + (i * 0.5);     // Slight altitude increase

        event.coordinate = QGeoCoordinate(lat, lon, alt);

        // Slight attitude variations
        event.roll = static_cast<float>(i * 0.1);
        event.pitch = static_cast<float>(i * -0.05);
        event.yaw = static_cast<float>(90.0 + i * 0.5);  // Heading roughly east

        events.append(event);
    }

    return events;
}

}  // namespace DataFlashTestGenerator
