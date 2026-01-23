#pragma once

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtPositioning/QGeoCoordinate>

class MissionItem;
class Vehicle;
class QmlObjectListModel;

/// Row data for CSV export
struct CsvWaypointRow {
    int index = 0;
    QString name;
    double latitude = 0.0;
    double longitude = 0.0;
    double altitude = 0.0;
    QString type;
    int command = 0;
    int frame = 0;
    double param1 = 0.0;
    double param2 = 0.0;
    double param3 = 0.0;
    double param4 = 0.0;
};

/// Used to convert a Plan to a CSV document
class CsvPlanDocument
{
public:
    CsvPlanDocument();

    void addMission(Vehicle* vehicle, QmlObjectListModel* visualItems, QList<MissionItem*> rgMissionItems);

    bool saveToFile(const QString& filename, QString& errorString) const;

    /// Get CSV content as string
    QString toCsv() const;

    /// Configure delimiter (default: comma)
    void setDelimiter(QChar delimiter) { _delimiter = delimiter; }

    /// Configure whether to include header row (default: true)
    void setIncludeHeader(bool include) { _includeHeader = include; }

private:
    void _addMissionItems(Vehicle* vehicle, QList<MissionItem*> rgMissionItems);
    QString _escapeField(const QString& field) const;

    QList<CsvWaypointRow> _rows;
    QChar _delimiter = ',';
    bool _includeHeader = true;
};
