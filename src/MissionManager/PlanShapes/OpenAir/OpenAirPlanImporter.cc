#include "OpenAirPlanImporter.h"
#include "OpenAirParser.h"
#include "QGCLoggingCategory.h"

OpenAirPlanImporter::OpenAirPlanImporter(QObject* parent)
    : PlanImporter(parent)
{
    // Register for .air extension (the singleton macro registers for .txt via fileExtension())
    registerImporter(QStringLiteral("air"), this);
}

IMPLEMENT_PLAN_IMPORTER_SINGLETON(OpenAirPlanImporter)

PlanImportResult OpenAirPlanImporter::importFromFile(const QString& filename)
{
    PlanImportResult result;

    OpenAirParser::ParseResult parseResult = OpenAirParser::parseFile(filename);

    if (!parseResult.success) {
        result.errorString = parseResult.errorString;
        return result;
    }

    if (parseResult.airspaces.isEmpty()) {
        result.errorString = tr("No airspaces found in file");
        return result;
    }

    // Convert airspaces to polygons with names
    for (const OpenAirParser::Airspace& airspace : parseResult.airspaces) {
        if (!airspace.boundary.isEmpty()) {
            result.polygons.append(airspace.boundary);
            result.waypointNames.append(airspace.name);
        }
    }

    result.success = !result.polygons.isEmpty();
    if (result.success) {
        result.formatDescription = tr("OpenAir (%1 features)").arg(result.polygons.count());
    } else {
        result.errorString = tr("No valid airspace boundaries found");
    }

    qCDebug(PlanImporterLog) << "Imported" << result.polygons.count() << "airspace polygons from OpenAir file";

    return result;
}
