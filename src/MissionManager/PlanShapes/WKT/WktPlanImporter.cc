#include "WktPlanImporter.h"
#include "WKTHelper.h"

#include <QtCore/QFile>
#include <QtCore/QTextStream>

WktPlanImporter::WktPlanImporter(QObject* parent)
    : PlanImporter(parent)
{
}

IMPLEMENT_PLAN_IMPORTER_SINGLETON(WktPlanImporter)

PlanImportResult WktPlanImporter::importFromFile(const QString& filename)
{
    PlanImportResult result;

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.errorString = tr("Cannot open file: %1").arg(file.errorString());
        return result;
    }

    QString content = QString::fromUtf8(file.readAll());
    file.close();

    QString errorString;

    // Try parsing as different WKT types
    QGeoCoordinate point;
    if (WKTHelper::parsePoint(content, point, errorString)) {
        result.waypoints.append(point);
    }

    QList<QGeoCoordinate> lineString;
    if (WKTHelper::parseLineString(content, lineString, errorString)) {
        result.trackPoints.append(lineString);
    }

    QList<QGeoCoordinate> polygon;
    if (WKTHelper::parsePolygon(content, polygon, errorString)) {
        result.polygons.append(polygon);
    }

    // Try multi-geometries
    QList<QGeoCoordinate> multiPoint;
    if (WKTHelper::parseMultiPoint(content, multiPoint, errorString)) {
        result.waypoints.append(multiPoint);
    }

    QList<QList<QGeoCoordinate>> multiLineString;
    if (WKTHelper::parseMultiLineString(content, multiLineString, errorString)) {
        for (const auto& line : multiLineString) {
            result.trackPoints.append(line);
        }
    }

    QList<QList<QGeoCoordinate>> multiPolygon;
    if (WKTHelper::parseMultiPolygon(content, multiPolygon, errorString)) {
        result.polygons.append(multiPolygon);
    }

    result.success = (result.itemCount() > 0);
    if (result.success) {
        result.formatDescription = tr("WKT (%1 features)").arg(result.itemCount());
    } else {
        result.errorString = errorString.isEmpty() ? tr("No valid WKT features found") : errorString;
    }

    return result;
}
