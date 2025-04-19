/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(QGCFileDialogControllerLog)

class QGCFileDialogController : public QObject
{
    Q_OBJECT
    // TODO: Q_NAMESPACE
    // QML_ELEMENT

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
};
