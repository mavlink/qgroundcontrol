#include "CsvPlanDocument.h"
#include "MissionCommandTree.h"
#include "MissionCommandUIInfo.h"
#include "MissionItem.h"
#include "Vehicle.h"
#include "QmlObjectListModel.h"

#include <QtCore/QFile>
#include <QtCore/QTextStream>

CsvPlanDocument::CsvPlanDocument() = default;

void CsvPlanDocument::_addMissionItems(Vehicle* vehicle, QList<MissionItem*> rgMissionItems)
{
    if (rgMissionItems.count() == 0) {
        return;
    }

    QGeoCoordinate homeCoord = rgMissionItems[0]->coordinate();
    int waypointIndex = 0;

    for (const MissionItem* item : rgMissionItems) {
        const MissionCommandUIInfo* uiInfo = MissionCommandTree::instance()->getUIInfo(vehicle, QGCMAVLink::VehicleClassGeneric, item->command());

        CsvWaypointRow row;
        row.index = waypointIndex++;
        row.command = static_cast<int>(item->command());
        row.frame = static_cast<int>(item->frame());
        row.param1 = item->param1();
        row.param2 = item->param2();
        row.param3 = item->param3();
        row.param4 = item->param4();

        if (uiInfo) {
            row.type = uiInfo->friendlyName();

            double altAdjustment = item->frame() == MAV_FRAME_GLOBAL ? 0 : homeCoord.altitude();

            if (uiInfo->specifiesCoordinate()) {
                QGeoCoordinate coord = item->coordinate();
                row.latitude = coord.latitude();
                row.longitude = coord.longitude();
                row.altitude = coord.altitude() + altAdjustment;
            }
        } else {
            row.type = QStringLiteral("CMD_%1").arg(row.command);
        }

        row.name = QStringLiteral("WPT%1").arg(row.index, 3, 10, QLatin1Char('0'));

        _rows.append(row);
    }
}

void CsvPlanDocument::addMission(Vehicle* vehicle, QmlObjectListModel* visualItems, QList<MissionItem*> rgMissionItems)
{
    Q_UNUSED(visualItems);
    _addMissionItems(vehicle, rgMissionItems);
}

QString CsvPlanDocument::_escapeField(const QString& field) const
{
    // If field contains delimiter, quote, or newline, wrap in quotes and escape internal quotes
    if (field.contains(_delimiter) || field.contains('"') || field.contains('\n')) {
        QString escaped = field;
        escaped.replace('"', QStringLiteral("\"\""));
        return QStringLiteral("\"%1\"").arg(escaped);
    }
    return field;
}

QString CsvPlanDocument::toCsv() const
{
    QStringList lines;

    if (_includeHeader) {
        QStringList headers = {
            QStringLiteral("Index"),
            QStringLiteral("Name"),
            QStringLiteral("Latitude"),
            QStringLiteral("Longitude"),
            QStringLiteral("Altitude"),
            QStringLiteral("Type"),
            QStringLiteral("Command"),
            QStringLiteral("Frame"),
            QStringLiteral("Param1"),
            QStringLiteral("Param2"),
            QStringLiteral("Param3"),
            QStringLiteral("Param4")
        };
        lines.append(headers.join(_delimiter));
    }

    for (const CsvWaypointRow& row : _rows) {
        QStringList fields = {
            QString::number(row.index),
            _escapeField(row.name),
            QString::number(row.latitude, 'f', 8),
            QString::number(row.longitude, 'f', 8),
            QString::number(row.altitude, 'f', 2),
            _escapeField(row.type),
            QString::number(row.command),
            QString::number(row.frame),
            QString::number(row.param1, 'f', 6),
            QString::number(row.param2, 'f', 6),
            QString::number(row.param3, 'f', 6),
            QString::number(row.param4, 'f', 6)
        };
        lines.append(fields.join(_delimiter));
    }

    return lines.join(QStringLiteral("\n"));
}

bool CsvPlanDocument::saveToFile(const QString& filename, QString& errorString) const
{
    if (_rows.isEmpty()) {
        errorString = QObject::tr("No data to export");
        return false;
    }

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        errorString = QObject::tr("Cannot open file for writing: %1").arg(file.errorString());
        return false;
    }

    QTextStream stream(&file);
    stream << toCsv();

    return true;
}
