#pragma once

#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtPositioning/QGeoCoordinate>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(ShapeFileHelperLog)

/// Routines for loading polygons or polylines from KML or SHP files.
class ShapeFileHelper : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QStringList fileDialogKMLFilters         READ fileDialogKMLFilters       CONSTANT) ///< File filter list for load/save KML file dialogs
    Q_PROPERTY(QStringList fileDialogKMLOrSHPFilters    READ fileDialogKMLOrSHPFilters  CONSTANT) ///< File filter list for load/save shape file dialogs

public:
    static QStringList fileDialogKMLFilters();
    static QStringList fileDialogKMLOrSHPFilters();

    enum class ShapeType {
        Polygon,
        Polyline,
        Point,
        Error
    };

    /// Default distance threshold for filtering nearby vertices (meters)
    static constexpr double kDefaultVertexFilterMeters = 5.0;

    static ShapeType determineShapeType(const QString &file, QString &errorString);

    /// Get the number of geometry entities in the file
    static int getEntityCount(const QString &file, QString &errorString);

    /// Load first polygon entity (convenience wrapper)
    /// @param filterMeters Filter vertices closer than this distance (0 to disable)
    static bool loadPolygonFromFile(const QString &file, QList<QGeoCoordinate> &vertices, QString &errorString,
                                    double filterMeters = kDefaultVertexFilterMeters);

    /// Load all polygon entities
    /// @param filterMeters Filter vertices closer than this distance (0 to disable)
    static bool loadPolygonsFromFile(const QString &file, QList<QList<QGeoCoordinate>> &polygons, QString &errorString,
                                     double filterMeters = kDefaultVertexFilterMeters);

    /// Load first polyline entity (convenience wrapper)
    /// @param filterMeters Filter vertices closer than this distance (0 to disable)
    static bool loadPolylineFromFile(const QString &file, QList<QGeoCoordinate> &coords, QString &errorString,
                                     double filterMeters = kDefaultVertexFilterMeters);

    /// Load all polyline entities
    /// @param filterMeters Filter vertices closer than this distance (0 to disable)
    static bool loadPolylinesFromFile(const QString &file, QList<QList<QGeoCoordinate>> &polylines, QString &errorString,
                                      double filterMeters = kDefaultVertexFilterMeters);

    /// Load point entities
    static bool loadPointsFromFile(const QString &file, QList<QGeoCoordinate> &points, QString &errorString);

    static constexpr const char *kmlFileExtension = ".kml";
    static constexpr const char *shpFileExtension = ".shp";

private:
    enum class ShapeFileType {
        None,
        KML,
        SHP
    };
    static ShapeFileType _getShapeFileType(const QString &file, QString &errorString);
};
