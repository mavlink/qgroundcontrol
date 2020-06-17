/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "QGCFileDialogController.h"
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "AppSettings.h"

#include <QStandardPaths>
#include <QDebug>
#include <QDir>

QGC_LOGGING_CATEGORY(QGCFileDialogControllerLog, "QGCFileDialogControllerLog")

QStringList QGCFileDialogController::getFiles(const QString& directoryPath, const QStringList& fileExtensions)
{
    qCDebug(QGCFileDialogControllerLog) << "getFiles" << directoryPath << fileExtensions;
    QStringList files;

    QDir fileDir(directoryPath);

    QStringList infoListExtensions;
    for (const QString& extension: fileExtensions) {
        infoListExtensions.append(QStringLiteral("*.%1").arg(extension));
    }

    QFileInfoList fileInfoList = fileDir.entryInfoList(infoListExtensions,  QDir::Files, QDir::Name);

    for (const QFileInfo& fileInfo: fileInfoList) {
        qCDebug(QGCFileDialogControllerLog) << "getFiles found" << fileInfo.fileName();
        files << fileInfo.fileName();
    }

    return files;
}

QString QGCFileDialogController::filenameWithExtension(const QString& filename, const QStringList& rgFileExtensions)
{
    QString filenameWithExtension(filename);

    bool matchFound = false;
    for (const QString& extension : rgFileExtensions) {
        QString dotExtension = QStringLiteral(".%1").arg(extension);
        matchFound = filenameWithExtension.endsWith(dotExtension);
        if (matchFound) {
            break;
        }
    }

    if (!matchFound) {
        filenameWithExtension += QStringLiteral(".%1").arg(rgFileExtensions[0]);
    }

    return filenameWithExtension;
}

bool QGCFileDialogController::fileExists(const QString& filename)
{
    return QFile(filename).exists();
}

QString QGCFileDialogController::fullyQualifiedFilename(const QString& directoryPath, const QString& filename, const QStringList& rgFileExtensions)
{
    return directoryPath + QStringLiteral("/") + filenameWithExtension(filename, rgFileExtensions);
}

void QGCFileDialogController::deleteFile(const QString& filename)
{
    QFile::remove(filename);
}

QString QGCFileDialogController::fullFolderPathToShortMobilePath(const QString& fullFolderPath)
{
#ifdef __mobile__
    QString defaultSavePath = qgcApp()->toolbox()->settingsManager()->appSettings()->savePath()->rawValueString();
    if (fullFolderPath.startsWith(defaultSavePath)) {
        int lastDirSepIndex = fullFolderPath.lastIndexOf(QStringLiteral("/"));
        return qgcApp()->applicationName() + QStringLiteral("/") + fullFolderPath.right(fullFolderPath.length() - lastDirSepIndex);
    } else {
        return fullFolderPath;
    }
#else
    qWarning() << "QGCFileDialogController::fullFolderPathToShortMobilePath should only be used in mobile builds";
    return fullFolderPath;
#endif
}
