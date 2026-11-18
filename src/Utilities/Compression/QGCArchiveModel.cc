#include "QGCArchiveModel.h"

#include <algorithm>
#include <iterator>

#include "QGCCompression.h"
#include "QGCFileHelper.h"
#include "QGCFormat.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(QGCArchiveModelLog, "Utilities.QGCArchiveModel")

QGCArchiveModel::QGCArchiveModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

QGCArchiveModel::~QGCArchiveModel() = default;

void QGCArchiveModel::setArchivePath(const QString &path)
{
    if (_archivePath == path) {
        return;
    }

    _archivePath = path;
    emit archivePathChanged();

    _loadArchive();
}

void QGCArchiveModel::setArchiveUrl(const QUrl &url)
{
    const QString path = QGCFileHelper::toLocalPath(url);
    if (!path.isEmpty() && !QGCFileHelper::isLocalPath(path)) {
        qCWarning(QGCArchiveModelLog) << "Unsupported archive URL:" << url;
        setArchivePath({});
        return;
    }

    setArchivePath(path);
}

void QGCArchiveModel::setFilterMode(FilterMode mode)
{
    if (_filterMode == mode) {
        return;
    }

    _filterMode = mode;
    emit filterModeChanged();

    _applyFilter();
}

int QGCArchiveModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(_filteredEntries.size());
}

QVariant QGCArchiveModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= _filteredEntries.size()) {
        return {};
    }

    const QGCCompression::ArchiveEntry &entry = _filteredEntries.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
    case NameRole:
        return entry.name;

    case SizeRole:
        return entry.size;

    case ModifiedRole:
        return entry.modified;

    case IsDirectoryRole:
        return entry.isDirectory;

    case PermissionsRole:
        return entry.permissions;

    case FileNameRole: {
        const int lastSlash = entry.name.lastIndexOf(QLatin1Char('/'));
        if (lastSlash >= 0 && lastSlash < entry.name.size() - 1) {
            return entry.name.mid(lastSlash + 1);
        }
        return entry.name;
    }

    case DirectoryRole: {
        const int lastSlash = entry.name.lastIndexOf(QLatin1Char('/'));
        if (lastSlash > 0) {
            return entry.name.left(lastSlash);
        }
        return QString();
    }

    case FormattedSizeRole:
        return formatSize(entry.size);

    default:
        return {};
    }
}

QHash<int, QByteArray> QGCArchiveModel::roleNames() const
{
    static const QHash<int, QByteArray> roles = {
        { NameRole, "name" },
        { SizeRole, "size" },
        { ModifiedRole, "modified" },
        { IsDirectoryRole, "isDirectory" },
        { PermissionsRole, "permissions" },
        { FileNameRole, "fileName" },
        { DirectoryRole, "directory" },
        { FormattedSizeRole, "formattedSize" },
    };
    return roles;
}

void QGCArchiveModel::refresh()
{
    _loadArchive();
}

void QGCArchiveModel::clear()
{
    if (_filteredEntries.isEmpty() && _allEntries.isEmpty()) {
        return;
    }

    beginResetModel();
    _allEntries.clear();
    _filteredEntries.clear();
    endResetModel();

    const bool hadFiles = _fileCount > 0;
    const bool hadDirs = _directoryCount > 0;
    const bool hadSize = _totalSize > 0;

    _fileCount = 0;
    _directoryCount = 0;
    _totalSize = 0;

    emit countChanged();
    if (hadFiles) emit fileCountChanged();
    if (hadDirs) emit directoryCountChanged();
    if (hadSize) emit totalSizeChanged();

    _setErrorString(QString());
}

QVariantMap QGCArchiveModel::get(int index) const
{
    QVariantMap result;

    if (index < 0 || index >= _filteredEntries.size()) {
        return result;
    }

    const QGCCompression::ArchiveEntry &entry = _filteredEntries.at(index);

    result[QStringLiteral("name")] = entry.name;
    result[QStringLiteral("size")] = entry.size;
    result[QStringLiteral("modified")] = entry.modified;
    result[QStringLiteral("isDirectory")] = entry.isDirectory;
    result[QStringLiteral("permissions")] = entry.permissions;
    result[QStringLiteral("formattedSize")] = formatSize(entry.size);

    // Compute fileName and directory
    const int lastSlash = entry.name.lastIndexOf(QLatin1Char('/'));
    if (lastSlash >= 0 && lastSlash < entry.name.size() - 1) {
        result[QStringLiteral("fileName")] = entry.name.mid(lastSlash + 1);
        result[QStringLiteral("directory")] = entry.name.left(lastSlash);
    } else {
        result[QStringLiteral("fileName")] = entry.name;
        result[QStringLiteral("directory")] = QString();
    }

    return result;
}

bool QGCArchiveModel::contains(const QString &fileName) const
{
    return !fileName.isEmpty() &&
           std::ranges::any_of(_allEntries, [&fileName](const auto& entry) { return entry.name == fileName; });
}

QString QGCArchiveModel::formatSize(qint64 bytes)
{
    return (bytes < 0) ? QStringLiteral("--") : QGC::bigSizeToString(static_cast<quint64>(bytes));
}

void QGCArchiveModel::_loadArchive()
{
    clear();

    if (_archivePath.isEmpty()) {
        emit loadingComplete(true);
        return;
    }

    _setLoading(true);
    _setErrorString(QString());

    qCDebug(QGCArchiveModelLog) << "Loading archive:" << _archivePath;

    // Load entries using QGCCompression (already sorted with QCollator)
    QList<QGCCompression::ArchiveEntry> entries;
    const QGCCompression::Outcome listed = QGCCompression::listArchiveDetailed(_archivePath, entries);
    if (!listed) {
        _setErrorString(listed.message);
        _setLoading(false);
        emit loadingComplete(false);
        return;
    }

    // Calculate statistics
    QGCCompression::ArchiveStats stats;
    for (const auto &entry : entries) {
        QGCCompression::addTo(stats, entry);
    }

    // Store all entries
    _allEntries = std::move(entries);

    // Update statistics, which clear() has set to zero: one that is not zero now has changed
    _fileCount = stats.fileCount;
    _directoryCount = stats.directoryCount;
    _totalSize = stats.totalUncompressedSize;

    // Apply current filter (this will emit countChanged via beginResetModel/endResetModel)
    _applyFilter();

    // Emit property changes
    if (_fileCount != 0) {
        emit fileCountChanged();
    }
    if (_directoryCount != 0) {
        emit directoryCountChanged();
    }
    if (_totalSize != 0) {
        emit totalSizeChanged();
    }

    _setLoading(false);

    qCDebug(QGCArchiveModelLog) << "Loaded" << _allEntries.size() << "entries:"
                                 << _fileCount << "files," << _directoryCount << "dirs,"
                                 << _totalSize << "bytes";

    emit loadingComplete(true);
}

void QGCArchiveModel::_applyFilter()
{
    beginResetModel();

    switch (_filterMode) {
    case AllEntries:
        _filteredEntries = _allEntries;
        break;

    case FilesOnly:
    case DirectoriesOnly:
        _filteredEntries.clear();
        std::ranges::copy_if(_allEntries, std::back_inserter(_filteredEntries), [this](const auto& entry) {
            return entry.isDirectory == (_filterMode == DirectoriesOnly);
        });
        break;
    }

    endResetModel();
    emit countChanged();
}

void QGCArchiveModel::_setLoading(bool loading)
{
    if (_loading == loading) {
        return;
    }

    _loading = loading;
    emit loadingChanged();
}

void QGCArchiveModel::_setErrorString(const QString &error)
{
    if (_errorString == error) {
        return;
    }

    _errorString = error;
    emit errorStringChanged();
}
