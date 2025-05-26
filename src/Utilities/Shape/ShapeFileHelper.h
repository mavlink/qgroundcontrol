/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtPositioning/QGeoCoordinate>

/// Routines for loading polygons or polylines from KML or SHP files.
class ShapeFileHelper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList fileDialogKMLFilters         READ fileDialogKMLFilters       CONSTANT) ///< File filter list for load/save KML file dialogs
    Q_PROPERTY(QStringList fileDialogKMLOrSHPFilters    READ fileDialogKMLOrSHPFilters  CONSTANT) ///< File filter list for load/save shape file dialogs

public:
    static QStringList fileDialogKMLFilters();
    static QStringList fileDialogKMLOrSHPFilters();

    enum class ShapeType {
        Polygon,
        Polyline,
        Error
    };
    static ShapeType determineShapeType(const QString &file, QString &errorString);
    static bool loadPolygonFromFile(const QString &file, QList<QGeoCoordinate> &vertices, QString &errorString);
    static bool loadPolylineFromFile(const QString &file, QList<QGeoCoordinate> &coords, QString &errorString);

    static constexpr const char *kmlFileExtension = "kml";
    static constexpr const char *shpFileExtension = "shp";

private:
    enum class ShapeFileType {
        None,
        KML,
        SHP
    };
    static ShapeFileType _getShapeFileType(const QString &file, QString &errorString);
    static bool _fileIsKML(const QString &file, QString &errorString);
    static bool _fileIsSHP(const QString &file, QString &errorString);

    static constexpr const char *_errorPrefix = QT_TR_NOOP("Shape file load failed. %1");
};
