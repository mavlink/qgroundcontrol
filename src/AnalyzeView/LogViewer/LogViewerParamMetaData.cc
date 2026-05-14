#include "LogViewerParamMetaData.h"

#ifndef QGC_NO_ARDUPILOT_DIALECT
#include "APMParameterMetaData.h"
#endif
#include "FactMetaData.h"
#include "PX4ParameterMetaData.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QFileInfo>

QGC_LOGGING_CATEGORY(LogViewerParamMetaDataLog, "AnalyzeView.LogViewerParamMetaData")

namespace {

#ifndef QGC_NO_ARDUPILOT_DIALECT

// Map the APM vehicle name as stored in detectedVehicleType to the file-path
// component used in :/FirmwarePlugin/APM/APMParameterFactMetaData.{X}.major.minor.json
QString _apmFileVehicleName(const QString &vehicleType)
{
    if (vehicleType == QStringLiteral("ArduCopter")) { return QStringLiteral("Copter"); }
    if (vehicleType == QStringLiteral("ArduPlane"))  { return QStringLiteral("Plane"); }
    if (vehicleType == QStringLiteral("ArduRover"))  { return QStringLiteral("Rover"); }
    if (vehicleType == QStringLiteral("ArduSub"))    { return QStringLiteral("Sub"); }
    return QString();
}

// Walk down from (major, minor) to find the best available bundled JSON file,
// mirroring APMFirmwarePlugin::_internalParameterMetaDataFile logic.
QString _findAPMMetaDataFile(const QString &fileVehicleName, int major, int minor)
{
    int currMajor = major;
    int currMinor = minor;

    while (currMajor >= 4 && currMinor > 0) {
        const QString file = QStringLiteral(":/FirmwarePlugin/APM/APMParameterFactMetaData.%1.%2.%3.json")
            .arg(fileVehicleName).arg(currMajor).arg(currMinor);
        if (QFileInfo::exists(file)) {
            return file;
        }
        currMinor--;
        if (currMinor == 0) {
            currMinor = 10;
            currMajor--;
        }
    }

    // Fallback: oldest available in the 4.x series
    for (int i = 0; i < 10; i++) {
        const QString file = QStringLiteral(":/FirmwarePlugin/APM/APMParameterFactMetaData.%1.4.%2.json")
            .arg(fileVehicleName).arg(i);
        if (QFileInfo::exists(file)) {
            return file;
        }
    }

    return QString();
}

#endif // QGC_NO_ARDUPILOT_DIALECT

void _enrichRows(QVariantList &parameters, ParameterMetaData *metaData)
{
    for (int i = 0; i < parameters.size(); i++) {
        QVariantMap row = parameters[i].toMap();

        const QString name = row.value(QStringLiteral("name")).toString();
        const bool isFloat = row.value(QStringLiteral("isFloat"), false).toBool();
        const FactMetaData::ValueType_t type = isFloat
            ? FactMetaData::valueTypeFloat
            : FactMetaData::valueTypeInt32;

        FactMetaData *factMeta = metaData->getMetaDataForFact(name, type);
        if (!factMeta) {
            parameters[i] = row;
            continue;
        }

        row[QStringLiteral("decimalPlaces")]   = factMeta->decimalPlaces();
        row[QStringLiteral("units")]           = factMeta->rawUnits();
        row[QStringLiteral("shortDescription")] = factMeta->shortDescription();
        row[QStringLiteral("enumStrings")]     = QVariant::fromValue(factMeta->enumStrings());
        row[QStringLiteral("enumValues")]      = QVariant::fromValue(factMeta->enumValues());

        parameters[i] = row;
    }
}

} // namespace

void LogViewerParamMetaData::enrichForPX4(QVariantList &parameters)
{
    if (parameters.isEmpty()) {
        return;
    }

    auto *metaData = new PX4ParameterMetaData(nullptr);
    metaData->loadParameterFactMetaDataFile(
        QStringLiteral(":/FirmwarePlugin/PX4/PX4ParameterFactMetaData.json"));

    _enrichRows(parameters, metaData);

    delete metaData;
}

#ifndef QGC_NO_ARDUPILOT_DIALECT
void LogViewerParamMetaData::enrichForAPM(QVariantList &parameters,
                                          const QString &vehicleType,
                                          int major,
                                          int minor)
{
    if (parameters.isEmpty()) {
        return;
    }

    const QString fileVehicleName = _apmFileVehicleName(vehicleType);
    if (fileVehicleName.isEmpty()) {
        qCDebug(LogViewerParamMetaDataLog) << "Unknown APM vehicle type:" << vehicleType;
        return;
    }

    // If no version was detected in the log, start from a high version so the
    // walk-down finds the newest available file.
    const int startMajor = (major >= 4) ? major : 9;
    const int startMinor = (minor >= 0) ? minor : 99;

    const QString metaDataFile = _findAPMMetaDataFile(fileVehicleName, startMajor, startMinor);
    if (metaDataFile.isEmpty()) {
        qCDebug(LogViewerParamMetaDataLog) << "No APM metadata file found for" << vehicleType
                                          << major << "." << minor;
        return;
    }

    qCDebug(LogViewerParamMetaDataLog) << "Loading APM metadata:" << metaDataFile;

    auto *metaData = new APMParameterMetaData(nullptr);
    metaData->loadParameterFactMetaDataFile(metaDataFile);

    _enrichRows(parameters, metaData);

    delete metaData;
}

#endif // QGC_NO_ARDUPILOT_DIALECT
