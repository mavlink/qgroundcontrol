#include "OnboardLogFileName.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>

OnboardLogFileName::UniquePath OnboardLogFileName::uniquePath(const QString& directory,
                                                              const QString& preferredFileName)
{
    const QDir targetDirectory(directory);
    QString fileName = preferredFileName;
    QString filePath = targetDirectory.filePath(fileName);
    if (!QFileInfo::exists(filePath)) {
        return {fileName, filePath};
    }

    const QFileInfo preferredInfo(preferredFileName);
    const QString baseName = preferredInfo.completeBaseName();
    const QString suffix = preferredInfo.suffix();
    const QString suffixWithSeparator = suffix.isEmpty() ? QString() : (QStringLiteral(".") + suffix);
    uint duplicateNumber = 0;
    do {
        ++duplicateNumber;
        fileName = baseName + QStringLiteral("_") + QString::number(duplicateNumber) + suffixWithSeparator;
        filePath = targetDirectory.filePath(fileName);
    } while (QFileInfo::exists(filePath));

    return {fileName, filePath};
}
