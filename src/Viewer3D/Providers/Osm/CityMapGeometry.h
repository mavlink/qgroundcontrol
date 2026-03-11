#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtQmlIntegration/QtQmlIntegration>
#include <QtQuick3D/QQuick3DGeometry>

Q_DECLARE_LOGGING_CATEGORY(CityMapGeometryLog)

class OsmParser;
class Viewer3DMapProvider;

class CityMapGeometry : public QQuick3DGeometry
{
    Q_OBJECT
    QML_ELEMENT
    Q_MOC_INCLUDE("Viewer3DMapProvider.h")

    friend class CityMapGeometryTest;

    Q_PROPERTY(QString              modelName   READ modelName   WRITE setModelName   NOTIFY modelNameChanged)
    Q_PROPERTY(Viewer3DMapProvider *mapProvider  READ mapProvider WRITE setMapProvider NOTIFY mapProviderChanged)

public:
    CityMapGeometry();

    QString modelName() const { return _modelName; }
    void setModelName(const QString &modelName);

    QString osmFilePath() const { return _osmFilePath; }

    Viewer3DMapProvider *mapProvider() const { return _mapProvider; }
    void setMapProvider(Viewer3DMapProvider *newMapProvider);

signals:
    void modelNameChanged();
    void osmFilePathChanged();
    void mapProviderChanged();

private:
    void _setOsmFilePath(const QVariant &value);
    void _updateViewer();
    void _clearViewer();
    bool _loadOsmMap();

    Viewer3DMapProvider *_mapProvider = nullptr;
    OsmParser *_osmParser = nullptr;

    QString _modelName;
    QString _osmFilePath;
    QByteArray _vertexData;
};
