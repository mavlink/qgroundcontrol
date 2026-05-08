#pragma once

#include "LogParseResultPrivate.h"
// Note: named LogViewerDataFlashParser to avoid collision with GeoTag/DataFlashParser.h

#include <QtCore/QString>

// Free-function parser for ArduPilot DataFlash (.bin / .log) files.
// Returns a filled LogParseResult on success (result.ok == true) or an error
// message in result.errorMessage on failure.
namespace DataFlashParser {
    LogParseResult parseFile(const QString &filePath);
}
