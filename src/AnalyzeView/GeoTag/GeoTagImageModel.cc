#include "GeoTagImageModel.h"
#include <QtCore/QLoggingCategory>

#include <QtCore/QFileInfo>

Q_STATIC_LOGGING_CATEGORY(GeoTagImageModelLog, "AnalyzeView.GeoTagImageModel")

GeoTagImageModel::GeoTagImageModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

GeoTagImageModel::~GeoTagImageModel()
{
}

int GeoTagImageModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return static_cast<int>(_images.count());
}

QVariant GeoTagImageModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= _images.count()) {
        return QVariant();
    }

    const ImageInfo &info = _images.at(index.row());

    switch (role) {
    case FileNameRole:
        return info.fileName;
    case FilePathRole:
        return info.filePath;
    case StatusRole:
        return static_cast<int>(info.status);
    case StatusStringRole:
        return statusToString(info.status);
    case ErrorMessageRole:
        return info.errorMessage;
    case CoordinateRole:
        return QVariant::fromValue(info.coordinate);
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> GeoTagImageModel::roleNames() const
{
    return {
        {FileNameRole, "fileName"},
        {FilePathRole, "filePath"},
        {StatusRole, "status"},
        {StatusStringRole, "statusString"},
        {ErrorMessageRole, "errorMessage"},
        {CoordinateRole, "coordinate"}
    };
}

void GeoTagImageModel::clear()
{
    if (_images.isEmpty()) {
        return;
    }

    beginResetModel();
    _images.clear();
    endResetModel();

    emit countChanged();
}

void GeoTagImageModel::addImage(const QString &filePath)
{
    const int row = _images.count();
    beginInsertRows(QModelIndex(), row, row);

    ImageInfo info;
    info.filePath = filePath;
    info.fileName = QFileInfo(filePath).fileName();
    info.status = Pending;
    _images.append(info);

    endInsertRows();
    emit countChanged();
}

void GeoTagImageModel::setStatus(int index, Status status, const QString &errorMessage)
{
    if (index < 0 || index >= _images.count()) {
        return;
    }

    ImageInfo &info = _images[index];
    if (info.status != status || info.errorMessage != errorMessage) {
        info.status = status;
        info.errorMessage = errorMessage;

        const QModelIndex modelIndex = createIndex(index, 0);
        emit dataChanged(modelIndex, modelIndex, {StatusRole, StatusStringRole, ErrorMessageRole});
    }
}

void GeoTagImageModel::setCoordinate(int index, const QGeoCoordinate &coordinate)
{
    if (index < 0 || index >= _images.count()) {
        return;
    }

    ImageInfo &info = _images[index];
    if (info.coordinate != coordinate) {
        info.coordinate = coordinate;

        const QModelIndex modelIndex = createIndex(index, 0);
        emit dataChanged(modelIndex, modelIndex, {CoordinateRole});
    }
}

void GeoTagImageModel::setStatusByPath(const QString &filePath, Status status, const QString &errorMessage)
{
    for (int i = 0; i < _images.count(); ++i) {
        if (_images.at(i).filePath == filePath) {
            setStatus(i, status, errorMessage);
            return;
        }
    }
}

void GeoTagImageModel::setAllStatus(Status status)
{
    if (_images.isEmpty()) {
        return;
    }

    for (int i = 0; i < _images.count(); ++i) {
        _images[i].status = status;
        _images[i].errorMessage.clear();
    }

    emit dataChanged(createIndex(0, 0), createIndex(_images.count() - 1, 0),
                     {StatusRole, StatusStringRole, ErrorMessageRole});
}

QString GeoTagImageModel::statusToString(Status status)
{
    switch (status) {
    case Pending:
        return tr("Pending");
    case Processing:
        return tr("Processing");
    case Tagged:
        return tr("Tagged");
    case Skipped:
        return tr("Skipped");
    case Failed:
        return tr("Failed");
    }
    return QString();
}
