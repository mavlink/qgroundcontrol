#pragma once

#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtQmlIntegration/QtQmlIntegration>

class QGCFileDialogController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit QGCFileDialogController(QObject *parent = nullptr);
    ~QGCFileDialogController();

    /// Return all file in the specified path which match the specified extension
    Q_INVOKABLE static QStringList getFiles(const QString &directoryPath, const QStringList &nameFilters);

    /// Returns the fully qualified file name from the specified parts.
    /// If filename has no file extension the first file extension is nameFilters is added to the filename.
    Q_INVOKABLE static QString fullyQualifiedFilename(const QString &directoryPath, const QString &filename, const QStringList &nameFilters = QStringList());

    /// Check for file existence of specified fully qualified file name
    Q_INVOKABLE static bool fileExists(const QString &filename);

    /// Deletes the file specified by the fully qualified file name
    Q_INVOKABLE static void deleteFile(const QString &filename);

    Q_INVOKABLE static QString urlToLocalFile(QUrl url);

    /// Important: Should only be used in mobile builds where default save location cannot be changed.
    /// Returns the standard QGC location portion of a fully qualified folder path.
    /// Example: "/Users/Don/Document/QGroundControl/Missions" returns "QGroundControl/Missions"
    Q_INVOKABLE static QString fullFolderPathToShortMobilePath(const QString &fullFolderPath);

    /// Opens Android's native file picker (ACTION_OPEN_DOCUMENT).
    /// On non-Android platforms this is a no-op.
    Q_INVOKABLE void importFromNativePicker();

signals:
    /// Emitted when the selected file has been successfully imported to the Missions directory.
    /// @param filePath Fully-qualified path of the imported file in the Missions directory.
    void fileImported(const QString& filePath);

    /// Emitted when the import operation fails.
    /// @param errorMessage Human-readable description of the error.
    void importFailed(const QString& errorMessage);

private:
#ifdef Q_OS_ANDROID
    void _handleImportResult(const QString& filePath);
#endif
};
