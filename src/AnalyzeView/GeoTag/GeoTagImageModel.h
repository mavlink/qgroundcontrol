#pragma once

#include <QtCore/QAbstractListModel>
#include <QtPositioning/QGeoCoordinate>
#include <QtQmlIntegration/QtQmlIntegration>


/// Model for displaying geotagging image status in QML
class GeoTagImageModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Status {
        Pending,
        Processing,
        Tagged,
        Skipped,
        Failed
    };
    Q_ENUM(Status)

    enum Roles {
        FileNameRole = Qt::UserRole + 1,
        FilePathRole,
        StatusRole,
        StatusStringRole,
        ErrorMessageRole,
        CoordinateRole
    };

    explicit GeoTagImageModel(QObject *parent = nullptr);
    ~GeoTagImageModel() override;

    // QAbstractListModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return rowCount(); }

    // Model manipulation
    void clear();
    void addImage(const QString &filePath);
    void setStatus(int index, Status status, const QString &errorMessage = QString());
    void setCoordinate(int index, const QGeoCoordinate &coordinate);
    void setStatusByPath(const QString &filePath, Status status, const QString &errorMessage = QString());

    // Batch operations
    void setAllStatus(Status status);

signals:
    void countChanged();

private:
    struct ImageInfo {
        QString filePath;
        QString fileName;
        Status status = Pending;
        QString errorMessage;
        QGeoCoordinate coordinate;
    };

    QList<ImageInfo> _images;

    static QString statusToString(Status status);
};
