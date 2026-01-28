#include "ShpPlanDocument.h"
#include "MissionCommandTree.h"
#include "MissionCommandUIInfo.h"
#include "MissionItem.h"
#include "Vehicle.h"
#include "QmlObjectListModel.h"
#include "TransectStyleComplexItem.h"
#include "SHPHelper.h"

#include <QtCore/QFileInfo>

#include "shapefil.h"

ShpPlanDocument::ShpPlanDocument() = default;

QString ShpPlanDocument::_makeFilename(const QString& baseFilename, const QString& suffix)
{
    QFileInfo fi(baseFilename);
    QString baseName = fi.completeBaseName();
    QString path = fi.path();
    return path + "/" + baseName + suffix + ".shp";
}

void ShpPlanDocument::_addFlightPath(Vehicle* vehicle, QList<MissionItem*> rgMissionItems)
{
    if (rgMissionItems.count() == 0) {
        return;
    }

    QGeoCoordinate homeCoord = rgMissionItems[0]->coordinate();

    for (const MissionItem* item : rgMissionItems) {
        const MissionCommandUIInfo* uiInfo = MissionCommandTree::instance()->getUIInfo(vehicle, QGCMAVLink::VehicleClassGeneric, item->command());
        if (uiInfo) {
            double altAdjustment = item->frame() == MAV_FRAME_GLOBAL ? 0 : homeCoord.altitude();

            if (uiInfo->isTakeoffCommand() && !vehicle->fixedWing()) {
                QGeoCoordinate coord = homeCoord;
                coord.setAltitude(item->param7() + altAdjustment);
                _flightPathCoords.append(coord);
            }

            if (uiInfo->specifiesCoordinate() && !uiInfo->isStandaloneCoordinate()) {
                QGeoCoordinate coord = item->coordinate();
                coord.setAltitude(coord.altitude() + altAdjustment);
                _flightPathCoords.append(coord);
            }
        }
    }
}

void ShpPlanDocument::_addWaypoints(Vehicle* vehicle, QList<MissionItem*> rgMissionItems)
{
    if (rgMissionItems.count() == 0) {
        return;
    }

    QGeoCoordinate homeCoord = rgMissionItems[0]->coordinate();

    for (const MissionItem* item : rgMissionItems) {
        const MissionCommandUIInfo* uiInfo = MissionCommandTree::instance()->getUIInfo(vehicle, QGCMAVLink::VehicleClassGeneric, item->command());
        if (uiInfo && uiInfo->specifiesCoordinate()) {
            double altAdjustment = item->frame() == MAV_FRAME_GLOBAL ? 0 : homeCoord.altitude();
            QGeoCoordinate coord = item->coordinate();
            coord.setAltitude(coord.altitude() + altAdjustment);

            WaypointInfo wp;
            wp.coordinate = coord;
            wp.sequenceNumber = item->sequenceNumber();
            wp.command = static_cast<int>(item->command());
            wp.commandName = uiInfo->friendlyName();
            wp.altitudeAMSL = coord.altitude();
            wp.altitudeRelative = coord.altitude() - homeCoord.altitude();
            wp.isStandalone = uiInfo->isStandaloneCoordinate();

            _waypoints.append(wp);
        }
    }
}

void ShpPlanDocument::_addComplexItems(QmlObjectListModel* visualItems)
{
    for (int i = 0; i < visualItems->count(); i++) {
        auto* transectItem = visualItems->value<TransectStyleComplexItem*>(i);
        if (transectItem) {
            QGCMapPolygon* polygon = transectItem->surveyAreaPolygon();
            if (polygon && polygon->count() >= 3) {
                QList<QGeoCoordinate> coords;
                for (int j = 0; j < polygon->count(); j++) {
                    coords.append(polygon->vertexCoordinate(j));
                }
                _areaPolygons.append(coords);
                _areaNames.append(transectItem->patternName());
            }
        }
    }
}

void ShpPlanDocument::addMission(Vehicle* vehicle, QmlObjectListModel* visualItems, QList<MissionItem*> rgMissionItems)
{
    _addFlightPath(vehicle, rgMissionItems);
    _addWaypoints(vehicle, rgMissionItems);
    _addComplexItems(visualItems);
}

bool ShpPlanDocument::_exportWaypoints(const QString& filename, QString& errorString) const
{
    if (_waypoints.isEmpty()) {
        return true;  // Nothing to export is not an error
    }

    // Check if any coordinate has altitude
    bool useZ = false;
    for (const WaypointInfo& wp : _waypoints) {
        if (!qIsNaN(wp.coordinate.altitude())) {
            useZ = true;
            break;
        }
    }

    const int shapeType = useZ ? SHPT_POINTZ : SHPT_POINT;

    // Create SHP file
    SHPHandle shpHandle = SHPCreate(filename.toUtf8().constData(), shapeType);
    if (!shpHandle) {
        errorString = QObject::tr("Unable to create waypoints SHP file: %1").arg(filename);
        return false;
    }

    // Create DBF file for attributes
    QString dbfFilename = filename;
    dbfFilename.replace(".shp", ".dbf", Qt::CaseInsensitive);
    DBFHandle dbfHandle = DBFCreate(dbfFilename.toUtf8().constData());
    if (!dbfHandle) {
        SHPClose(shpHandle);
        errorString = QObject::tr("Unable to create waypoints DBF file: %1").arg(dbfFilename);
        return false;
    }

    // Add DBF fields
    int fldSeq = DBFAddField(dbfHandle, "SEQ", FTInteger, 10, 0);
    int fldCmd = DBFAddField(dbfHandle, "CMD", FTInteger, 10, 0);
    int fldCmdName = DBFAddField(dbfHandle, "CMD_NAME", FTString, 32, 0);
    int fldAltAMSL = DBFAddField(dbfHandle, "ALT_AMSL", FTDouble, 12, 2);
    int fldAltRel = DBFAddField(dbfHandle, "ALT_REL", FTDouble, 12, 2);
    int fldStandalone = DBFAddField(dbfHandle, "STANDALONE", FTInteger, 1, 0);

    // Write waypoints
    int recordIndex = 0;
    for (const WaypointInfo& wp : _waypoints) {
        double x = wp.coordinate.longitude();
        double y = wp.coordinate.latitude();
        double z = useZ ? (qIsNaN(wp.coordinate.altitude()) ? 0.0 : wp.coordinate.altitude()) : 0.0;

        SHPObject* shpObject = SHPCreateObject(
            shapeType,
            -1,
            0, nullptr, nullptr,
            1,
            &x, &y,
            useZ ? &z : nullptr,
            nullptr
        );

        if (shpObject) {
            SHPWriteObject(shpHandle, -1, shpObject);
            SHPDestroyObject(shpObject);

            // Write attributes
            DBFWriteIntegerAttribute(dbfHandle, recordIndex, fldSeq, wp.sequenceNumber);
            DBFWriteIntegerAttribute(dbfHandle, recordIndex, fldCmd, wp.command);
            DBFWriteStringAttribute(dbfHandle, recordIndex, fldCmdName, wp.commandName.toUtf8().constData());
            DBFWriteDoubleAttribute(dbfHandle, recordIndex, fldAltAMSL, wp.altitudeAMSL);
            DBFWriteDoubleAttribute(dbfHandle, recordIndex, fldAltRel, wp.altitudeRelative);
            DBFWriteIntegerAttribute(dbfHandle, recordIndex, fldStandalone, wp.isStandalone ? 1 : 0);

            recordIndex++;
        }
    }

    SHPClose(shpHandle);
    DBFClose(dbfHandle);

    // Write PRJ file
    SHPHelper::writePrjFile(filename, errorString);

    _createdFiles.append(filename);
    return true;
}

bool ShpPlanDocument::_exportFlightPath(const QString& filename, QString& errorString) const
{
    if (_flightPathCoords.count() < 2) {
        return true;  // Nothing to export is not an error
    }

    if (!SHPHelper::savePolylineToFile(filename, _flightPathCoords, errorString)) {
        return false;
    }

    _createdFiles.append(filename);
    return true;
}

bool ShpPlanDocument::_exportAreas(const QString& filename, QString& errorString) const
{
    if (_areaPolygons.isEmpty()) {
        return true;  // Nothing to export is not an error
    }

    // Check if any coordinate has altitude
    bool useZ = false;
    for (const QList<QGeoCoordinate>& poly : _areaPolygons) {
        for (const QGeoCoordinate& coord : poly) {
            if (!qIsNaN(coord.altitude())) {
                useZ = true;
                break;
            }
        }
        if (useZ) break;
    }

    const int shapeType = useZ ? SHPT_POLYGONZ : SHPT_POLYGON;

    // Create SHP file
    SHPHandle shpHandle = SHPCreate(filename.toUtf8().constData(), shapeType);
    if (!shpHandle) {
        errorString = QObject::tr("Unable to create areas SHP file: %1").arg(filename);
        return false;
    }

    // Create DBF file for attributes
    QString dbfFilename = filename;
    dbfFilename.replace(".shp", ".dbf", Qt::CaseInsensitive);
    DBFHandle dbfHandle = DBFCreate(dbfFilename.toUtf8().constData());
    if (!dbfHandle) {
        SHPClose(shpHandle);
        errorString = QObject::tr("Unable to create areas DBF file: %1").arg(dbfFilename);
        return false;
    }

    // Add DBF fields
    int fldName = DBFAddField(dbfHandle, "NAME", FTString, 64, 0);
    int fldIndex = DBFAddField(dbfHandle, "INDEX", FTInteger, 10, 0);

    // Write polygons
    int recordIndex = 0;
    for (int polyIdx = 0; polyIdx < _areaPolygons.count(); polyIdx++) {
        const QList<QGeoCoordinate>& coords = _areaPolygons[polyIdx];
        if (coords.count() < 3) {
            continue;
        }

        // Close the polygon (first == last)
        const int nVertices = static_cast<int>(coords.count()) + 1;
        QVector<double> padfX(nVertices);
        QVector<double> padfY(nVertices);
        QVector<double> padfZ(useZ ? nVertices : 0);

        for (int i = 0; i < coords.count(); i++) {
            padfX[i] = coords[i].longitude();
            padfY[i] = coords[i].latitude();
            if (useZ) {
                padfZ[i] = qIsNaN(coords[i].altitude()) ? 0.0 : coords[i].altitude();
            }
        }
        // Close the ring
        padfX[coords.count()] = coords[0].longitude();
        padfY[coords.count()] = coords[0].latitude();
        if (useZ) {
            padfZ[coords.count()] = qIsNaN(coords[0].altitude()) ? 0.0 : coords[0].altitude();
        }

        SHPObject* shpObject = SHPCreateObject(
            shapeType,
            -1,
            0, nullptr, nullptr,
            nVertices,
            padfX.data(),
            padfY.data(),
            useZ ? padfZ.data() : nullptr,
            nullptr
        );

        if (shpObject) {
            SHPWriteObject(shpHandle, -1, shpObject);
            SHPDestroyObject(shpObject);

            // Write attributes
            QString name = (polyIdx < _areaNames.count()) ? _areaNames[polyIdx] : QString("Area %1").arg(polyIdx + 1);
            DBFWriteStringAttribute(dbfHandle, recordIndex, fldName, name.toUtf8().constData());
            DBFWriteIntegerAttribute(dbfHandle, recordIndex, fldIndex, polyIdx);

            recordIndex++;
        }
    }

    SHPClose(shpHandle);
    DBFClose(dbfHandle);

    // Write PRJ file
    SHPHelper::writePrjFile(filename, errorString);

    _createdFiles.append(filename);
    return true;
}

bool ShpPlanDocument::exportToFiles(const QString& baseFilename, QString& errorString) const
{
    _createdFiles.clear();
    errorString.clear();

    // Export waypoints
    QString waypointsFile = _makeFilename(baseFilename, "_waypoints");
    if (!_exportWaypoints(waypointsFile, errorString)) {
        return false;
    }

    // Export flight path
    QString pathFile = _makeFilename(baseFilename, "_path");
    if (!_exportFlightPath(pathFile, errorString)) {
        return false;
    }

    // Export survey areas
    QString areasFile = _makeFilename(baseFilename, "_areas");
    if (!_exportAreas(areasFile, errorString)) {
        return false;
    }

    if (_createdFiles.isEmpty()) {
        errorString = QObject::tr("No data to export");
        return false;
    }

    return true;
}
