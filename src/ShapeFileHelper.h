/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QList>
#include <QGeoCoordinate>
#include <QVariant>

/// Routines for loading polygons or polylines from KML or SHP files.
class ShapeFileHelper : public QObject
{
    Q_OBJECT

public:
    enum ShapeType {
        Polygon,
        Polyline,
        Error
    };
    Q_ENUM(ShapeType)

    Q_PROPERTY(QStringList fileDialogKMLFilters         READ fileDialogKMLFilters       CONSTANT) ///< File filter list for load/save KML file dialogs
    Q_PROPERTY(QStringList fileDialogKMLOrSHPFilters    READ fileDialogKMLOrSHPFilters  CONSTANT) ///< File filter list for load/save shape file dialogs

    /// Loads the file and returns shape type and error string in a variant array.
    /// ShapeType is in index 0, error string is in index 1.
    Q_INVOKABLE static QVariantList determineShapeType(const QString& file);

    QStringList fileDialogKMLFilters        (void) const;
    QStringList fileDialogKMLOrSHPFilters   (void) const;

    static ShapeType determineShapeType(const QString& file, QString& errorString);
    static bool loadPolygonFromFile(const QString& file, QList<QGeoCoordinate>& vertices, QString& errorString);
    static bool loadPolylineFromFile(const QString& file, QList<QGeoCoordinate>& coords, QString& errorString);

private:
    static bool _fileIsKML(const QString& file, QString& errorString);

    static const char* _errorPrefix;
};
