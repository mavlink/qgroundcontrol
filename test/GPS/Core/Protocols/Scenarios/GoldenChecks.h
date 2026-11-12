#pragma once

// The decode table of the golden transcripts, and the checks of every scenario and decoder against the files under
// test/GPS/Core/Protocols/golden/. GPSGoldenTranscriptTest runs the checks in the application, and
// QGCGPSGoldenTranscripts (GPSGoldenTranscripts.cc) without it.

#include <vector>

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include "Protocols/Scenarios/GoldenScenarios.h"

namespace GPSTest::GoldenScenario {

// ---------------------------------------------------------------------------------------------------------------
// Decode table: every corpus and fixture file through each receiver family that can meet it.

struct DecoderDef
{
    const char* name;
    GPSType type;
    BenchFactory bench;
    GPSReceiverConfig config;
    /// A decoder whose transcript this one's must match; it has no golden of its own.
    const char* sameAs = nullptr;
};

const std::vector<DecoderDef>& decoders();

/// The names of the decoders that decode the data file @a file. Empty for a file no decoder claims, which
/// checkGoldenFiles() rejects.
QStringList decodersFor(const QString& file);

struct DataFile
{
    QString directory;
    QString name;
    QByteArray bytes;
};

/// The receiver traffic under corpus/ and fixtures/, each directory in the order of its file names.
const std::vector<DataFile>& dataFiles();

// ---------------------------------------------------------------------------------------------------------------
// Checks. Each returns nothing when it passes, else what differs. With QGC_GPS_GOLDEN_UPDATE=1 a golden file that
// differs is rewritten, and one nothing produces is removed.

/// The golden of @a scenario under GPSTest::GOLDEN_DIR, without its suffix: its family and its name.
QString goldenName(const ScenarioDef& scenario);
QString goldenName(const DecoderDef& decoder);

/// @a scenario runs the same twice, as its golden records or as the scenario it is the same as behaves.
[[nodiscard]] QString checkScenario(const ScenarioDef& scenario);

/// @a decoder decodes its data files the same twice and in chunks of 1 and 7 bytes, as its golden records or as the
/// decoder it is the same as does.
[[nodiscard]] QString checkDecoder(const DecoderDef& decoder);

/// Every data file has a decoder, no two scenarios share a name, and the golden files are those of the scenarios and
/// decoders.
[[nodiscard]] QString checkGoldenFiles();

}  // namespace GPSTest::GoldenScenario
