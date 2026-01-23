#include "CsvPlanImporter.h"
#include "CSVHelper.h"

CsvPlanImporter::CsvPlanImporter(QObject* parent)
    : PlanImporter(parent)
{
}

IMPLEMENT_PLAN_IMPORTER_SINGLETON(CsvPlanImporter)

PlanImportResult CsvPlanImporter::importFromFile(const QString& filename)
{
    PlanImportResult result;

    CSVHelper::LoadResult loadResult = CSVHelper::loadPointsFromFile(filename, CSVHelper::ParseOptions());

    if (!loadResult.success) {
        result.errorString = loadResult.errorString;
        return result;
    }

    result.waypoints = loadResult.points;

    // Use names from CSV if available, otherwise generate them
    for (int i = 0; i < result.waypoints.count(); ++i) {
        if (i < loadResult.names.count() && !loadResult.names[i].isEmpty()) {
            result.waypointNames.append(loadResult.names[i]);
        } else {
            result.waypointNames.append(QStringLiteral("WPT%1").arg(i + 1, 3, 10, QLatin1Char('0')));
        }
    }

    result.success = !result.waypoints.isEmpty();
    if (result.success) {
        result.formatDescription = tr("CSV (%1 features)").arg(result.waypoints.count());
    } else {
        result.errorString = tr("No features found in CSV file");
    }

    return result;
}
