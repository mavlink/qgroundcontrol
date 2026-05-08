#pragma once

#include "LogParseResultPrivate.h"

#include <QtCore/QString>

// Free-function parser for PX4 ULog (.ulg) files.
// Returns a filled LogParseResult on success (result.ok == true) or an error
// message in result.errorMessage on failure.
namespace ULogParser {
    LogParseResult parseFile(const QString &filePath);
}
