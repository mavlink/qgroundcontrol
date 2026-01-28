#include "WktPlanDocument.h"
#include "WKTHelper.h"
#include "MissionItem.h"

#include <QtCore/QFile>
#include <QtCore/QTextStream>

void WktPlanDocument::addMission(Vehicle* vehicle, QmlObjectListModel* visualItems, QList<MissionItem*> rgMissionItems)
{
    // Use base class extraction
    PlanDocumentBase::addMission(vehicle, visualItems, rgMissionItems);
}

QString WktPlanDocument::flightPathWkt() const
{
    if (_flightPath.count() < 2) {
        return QString();
    }
    return WKTHelper::toWKTLineString(_flightPath, _includeAltitude);
}

QString WktPlanDocument::waypointsWkt() const
{
    if (_waypoints.isEmpty()) {
        return QString();
    }
    return WKTHelper::toWKTMultiPoint(_waypoints, _includeAltitude);
}

QString WktPlanDocument::surveyAreasWkt() const
{
    if (_surveyAreas.isEmpty()) {
        return QString();
    }
    return WKTHelper::toWKTMultiPolygon(_surveyAreas, _includeAltitude);
}

QString WktPlanDocument::combinedWkt() const
{
    QStringList geometries;

    QString path = flightPathWkt();
    if (!path.isEmpty()) {
        geometries.append(path);
    }

    QString waypoints = waypointsWkt();
    if (!waypoints.isEmpty()) {
        geometries.append(waypoints);
    }

    QString areas = surveyAreasWkt();
    if (!areas.isEmpty()) {
        geometries.append(areas);
    }

    if (geometries.isEmpty()) {
        return QStringLiteral("GEOMETRYCOLLECTION EMPTY");
    }

    return QStringLiteral("GEOMETRYCOLLECTION (%1)").arg(geometries.join(QStringLiteral(", ")));
}

bool WktPlanDocument::saveToFile(const QString& filename, QString& errorString) const
{
    if (isEmpty()) {
        errorString = QObject::tr("No data to export");
        return false;
    }

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        errorString = QObject::tr("Cannot open file for writing: %1").arg(file.errorString());
        return false;
    }

    QTextStream stream(&file);

    // Write each geometry type on its own line with a label
    QString path = flightPathWkt();
    if (!path.isEmpty()) {
        stream << "# Flight Path\n" << path << "\n\n";
    }

    QString waypoints = waypointsWkt();
    if (!waypoints.isEmpty()) {
        stream << "# Waypoints\n" << waypoints << "\n\n";
    }

    QString areas = surveyAreasWkt();
    if (!areas.isEmpty()) {
        stream << "# Survey Areas\n" << areas << "\n\n";
    }

    // Also write combined geometry collection
    stream << "# Combined (GEOMETRYCOLLECTION)\n" << combinedWkt() << "\n";

    return true;
}
