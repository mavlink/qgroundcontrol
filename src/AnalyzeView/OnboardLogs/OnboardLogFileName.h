#pragma once

#include <QtCore/QString>

namespace OnboardLogFileName {

struct UniquePath
{
    QString fileName;
    QString filePath;
};

/// Returns a collision-free file name and path without creating the file.
UniquePath uniquePath(const QString& directory, const QString& preferredFileName);

}  // namespace OnboardLogFileName
