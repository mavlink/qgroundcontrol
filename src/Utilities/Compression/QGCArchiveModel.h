#pragma once

#include "QGCCompressionTypes.h"

#include <QtCore/QAbstractListModel>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtQml/QQmlEngine>

/// List model for archive contents, suitable for QML ListView binding.
/// Provides natural sorting, filtering by type, and lazy loading support.
///
/// Usage in QML:
/// @code
/// import QGroundControl.Utilities
///
/// ListView {
///     model: QGCArchiveModel {
///         archivePath: "/path/to/archive.zip"
///     }
///     delegate: ItemDelegate {
///         text: model.name
///         icon.name: model.isDirectory ? "folder" : "file"
///     }
/// }
/// @endcode
class QGCArchiveModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    /// Path to the archive file to display (QString)
    Q_PROPERTY(QString archivePath READ archivePath WRITE setArchivePath NOTIFY archivePathChanged)

    /// URL to the archive file (QUrl convenience for QML file dialogs)
    /// Supports file://, qrc:/, and plain local paths
    Q_PROPERTY(QUrl archiveUrl READ archiveUrl WRITE setArchiveUrl NOTIFY archivePathChanged)

    /// Number of entries in the archive (files + directories)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

    /// Number of files in the archive (excludes directories)
    Q_PROPERTY(int fileCount READ fileCount NOTIFY fileCountChanged)

    /// Number of directories in the archive
    Q_PROPERTY(int directoryCount READ directoryCount NOTIFY directoryCountChanged)

    /// Total uncompressed size of all files in bytes
    Q_PROPERTY(qint64 totalSize READ totalSize NOTIFY totalSizeChanged)

    /// Whether the model is currently loading archive contents
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)

    /// Error message if loading failed (empty on success)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)

    /// Filter to show only files, only directories, or all entries
    Q_PROPERTY(FilterMode filterMode READ filterMode WRITE setFilterMode NOTIFY filterModeChanged)

public:
    /// Roles for accessing entry data from QML/delegates
    enum Role {
        NameRole = Qt::UserRole + 1,    ///< Entry name/path (QString)
        SizeRole,                        ///< Uncompressed size in bytes (qint64)
        ModifiedRole,                    ///< Last modified date (QDateTime)
        IsDirectoryRole,                 ///< Whether entry is a directory (bool)
        PermissionsRole,                 ///< Unix permissions (quint32)
        FileNameRole,                    ///< Just the filename without path (QString)
        DirectoryRole,                   ///< Parent directory path (QString)
        FormattedSizeRole,               ///< Human-readable size string (QString)
    };
    Q_ENUM(Role)

    /// Filter modes for showing subsets of entries
    enum FilterMode {
        AllEntries,     ///< Show all files and directories
        FilesOnly,      ///< Show only files (no directories)
        DirectoriesOnly ///< Show only directories
    };
    Q_ENUM(FilterMode)

    explicit QGCArchiveModel(QObject *parent = nullptr);
    ~QGCArchiveModel() override;

    // Property accessors
    QString archivePath() const { return _archivePath; }
    void setArchivePath(const QString &path);

    QUrl archiveUrl() const { return QUrl::fromLocalFile(_archivePath); }
    void setArchiveUrl(const QUrl &url);

    int count() const { return static_cast<int>(_filteredEntries.size()); }
    int fileCount() const { return _fileCount; }
    int directoryCount() const { return _directoryCount; }
    qint64 totalSize() const { return _totalSize; }
    bool loading() const { return _loading; }
    QString errorString() const { return _errorString; }

    FilterMode filterMode() const { return _filterMode; }
    void setFilterMode(FilterMode mode);

    // QAbstractListModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    /// Reload archive contents (useful after external changes)
    Q_INVOKABLE void refresh();

    /// Clear the model and reset all properties
    Q_INVOKABLE void clear();

    /// Get entry at index (for C++ usage)
    Q_INVOKABLE QVariantMap get(int index) const;

    /// Check if a file exists in the archive
    Q_INVOKABLE bool contains(const QString &fileName) const;

    /// Format bytes as human-readable string (e.g., "1.5 MB")
    Q_INVOKABLE static QString formatSize(qint64 bytes);

signals:
    void archivePathChanged();
    void countChanged();
    void fileCountChanged();
    void directoryCountChanged();
    void totalSizeChanged();
    void loadingChanged();
    void errorStringChanged();
    void filterModeChanged();

    /// Emitted when archive loading completes (success or failure)
    void loadingComplete(bool success);

private:
    void _loadArchive();
    void _applyFilter();
    void _setLoading(bool loading);
    void _setErrorString(const QString &error);

    QString _archivePath;
    QList<QGCCompression::ArchiveEntry> _allEntries;      ///< All entries from archive
    QList<QGCCompression::ArchiveEntry> _filteredEntries; ///< Entries after filter applied
    int _fileCount = 0;
    int _directoryCount = 0;
    qint64 _totalSize = 0;
    bool _loading = false;
    QString _errorString;
    FilterMode _filterMode = AllEntries;
};
