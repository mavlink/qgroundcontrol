#pragma once

#include <QtCore/QString>
#include <QtCore/QVariantList>

/// \brief Helper that enriches parameter rows with metadata from the bundled
///
/// PX4 / APM FactMetaData JSON files. Each row in @a parameters is a
/// QVariantMap; on return the following keys are added (or left absent if
/// no metadata was found for that parameter):
///   decimalPlaces  int       — from FactMetaData::decimalPlaces(); -1 = unknown
///   units          QString   — raw unit string (e.g. "m/s")
///   shortDescription QString — one-line summary
///   enumStrings    QStringList — ordered enum labels  (empty if not an enum)
///   enumValues     QVariantList — corresponding numeric values (parallel array)
///
class LogViewerParamMetaData
{
public:
    /// Enrich parameters from a PX4 ULog file.
    static void enrichForPX4(QVariantList &parameters);

#ifndef QGC_NO_ARDUPILOT_DIALECT
    /// Enrich parameters from an APM DataFlash file.
    /// @param vehicleType  APM vehicle name: "ArduCopter", "ArduPlane", "ArduRover", "ArduSub"
    /// @param major / minor  Firmware version numbers parsed from the log (pass -1 if unknown).
    static void enrichForAPM(QVariantList &parameters,
                             const QString &vehicleType,
                             int major,
                             int minor);
#endif
};
