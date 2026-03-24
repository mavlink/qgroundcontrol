#include "QGCFileDialogController.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"
#include "AppSettings.h"

#include <QtCore/QDir>

#ifdef Q_OS_ANDROID
#include <QtCore/QPointer>
#include "AndroidInterface.h"
#endif

QGC_LOGGING_CATEGORY(QGCFileDialogControllerLog, "QMLControls.QGCFileDialogController")

QGCFileDialogController::QGCFileDialogController(QObject *parent)
    : QObject(parent)
{
    qCDebug(QGCFileDialogControllerLog) << this;
}

QGCFileDialogController::~QGCFileDialogController()
{
    qCDebug(QGCFileDialogControllerLog) << this;
}

QStringList QGCFileDialogController::getFiles(const QString &directoryPath, const QStringList &nameFilters)
{
    qCDebug(QGCFileDialogControllerLog) << "getFiles" << directoryPath << nameFilters;

    QDir fileDir(directoryPath);
    const QFileInfoList fileInfoList = fileDir.entryInfoList(nameFilters,  QDir::Files, QDir::Name);

    QStringList files;
    for (const QFileInfo &fileInfo: fileInfoList) {
        qCDebug(QGCFileDialogControllerLog) << "getFiles found" << fileInfo.fileName();
        files << fileInfo.fileName();
    }

    return files;
}

bool QGCFileDialogController::fileExists(const QString &filename)
{
    return QFile(filename).exists();
}

QString QGCFileDialogController::fullyQualifiedFilename(const QString& directoryPath, const QString& filename, const QStringList& nameFilters)
{
    QString firstFileExtention;

    // Check that the filename has one of the specified file extensions

    bool extensionFound = true;
    if (nameFilters.count()) {
        extensionFound = false;
        for (const QString& nameFilter: nameFilters) {
            if (nameFilter.startsWith("*.")) {
                const QString fileExtension = nameFilter.right(nameFilter.length() - 2);
                if (fileExtension != "*") {
                    if (firstFileExtention.isEmpty()) {
                        firstFileExtention = fileExtension;
                    }
                    if (filename.endsWith(fileExtension)) {
                        extensionFound = true;
                        break;
                    }
                }
            } else if (nameFilter != "*") {
                qCWarning(QGCFileDialogControllerLog) << "unsupported name filter format" << nameFilter;
            }
        }
    }

    // Add the extension if it is missing
    QString filenameWithExtension = filename;
    if (!extensionFound) {
        filenameWithExtension = QStringLiteral("%1.%2").arg(filename).arg(firstFileExtention);
    }

    return (directoryPath + QStringLiteral("/") + filenameWithExtension);
}

void QGCFileDialogController::deleteFile(const QString &filename)
{
    QFile::remove(filename);
}

QString QGCFileDialogController::fullFolderPathToShortMobilePath(const QString &fullFolderPath)
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    const QString defaultSavePath = SettingsManager::instance()->appSettings()->savePath()->rawValueString();
    if (fullFolderPath.startsWith(defaultSavePath)) {
        const int lastDirSepIndex = fullFolderPath.lastIndexOf(QStringLiteral("/"));
        return (QCoreApplication::applicationName() + QStringLiteral("/") + fullFolderPath.right(fullFolderPath.length() - lastDirSepIndex));
    }
#else
    qCWarning(QGCFileDialogControllerLog) << Q_FUNC_INFO << "should only be used in mobile builds";
#endif
    return fullFolderPath;
}

QString QGCFileDialogController::urlToLocalFile(QUrl url)
{
    // For some strange reason on Qt6 running on Linux files returned by FileDialog are not returned as local file urls.
    // Seems to be new behavior with Qt6.
    if (url.isLocalFile()) {
        return url.toLocalFile();
    }

    return url.toString();
}

void QGCFileDialogController::importFromNativePicker()
{
#ifdef Q_OS_ANDROID
    const QString missionPath = SettingsManager::instance()->appSettings()->missionSavePath();
    if (missionPath.isEmpty()) {
        qCWarning(QGCFileDialogControllerLog) << "Missions save path is empty";
        emit importFailed(tr("Missions directory is not configured"));
        return;
    }

    QDir dir(missionPath);
    if (!dir.exists()) {
        qCWarning(QGCFileDialogControllerLog) << "Missions save path does not exist";
        emit importFailed(tr("Missions save path does not exist"));
        return;
    }

    QPointer<QGCFileDialogController> self = this;
    AndroidInterface::openFileImportDialog(missionPath, [self](const QString& filePath) {
        if (self) {
            QMetaObject::invokeMethod(
                self,
                [filePath, self]() { self->_handleImportResult(filePath); },
                Qt::QueuedConnection);
        }
    });
#else
    qCWarning(QGCFileDialogControllerLog) << Q_FUNC_INFO << "only supported on Android";
#endif
}

#ifdef Q_OS_ANDROID

void QGCFileDialogController::_handleImportResult(const QString& filePath)
{
    if (filePath.isEmpty()) {
        qCWarning(QGCFileDialogControllerLog) << "Import failed: empty file path received from Java";
        emit importFailed(tr("Failed to import file"));
        return;
    }

    qCDebug(QGCFileDialogControllerLog) << "File imported successfully to:" << filePath;
    emit fileImported(filePath);
}

#endif // Q_OS_ANDROID
