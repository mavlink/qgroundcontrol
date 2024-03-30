#pragma once
#include <QObject>
#include <QTime>
#include "Vehicle.h"

class MetDataLogManager : public QObject
{
    Q_OBJECT

    public:
        MetDataLogManager(QObject *parent = nullptr);
        ~MetDataLogManager();

    private:

        void _initializeMetRawCsv           ();
        void _writeMetRawCsvLine            ();

        Vehicle*            _activeVehicle;
        QTimer              _metRawCsvTimer;
        QFile               _metRawCsvFile;

        QStringList metFactHeaders = {
            "Time (s)",
            "ASL (m)",
            "Pressure (mB)",
            "Air Temp (C)",
            "Rel Hum (%)",
            "Wind Speed (m/s)",
            "Wind Direction (deg)",
            "Latitude (deg)",
            "Longitude (deg)",
            "Roll (deg)",
            "Roll Rate (deg/s)",
            "Pitch (deg)",
            "Pitch Rate (deg/s)",
            "Yaw (deg)",
            "Yaw Rate (deg/s)",
            "Ascent Rate (m/s)",
            "Speed Over Ground (m/s)"
        };

        QStringList metFactNames = {
            "timeUnixSeconds",
            "altitudeMetersMSL",
            "absolutePressureMillibars",
            "temperatureCelsius",
            "relativeHumidity",
            "windSpeedMetersPerSecond",
            "windBearingDegrees",
            "latitudeDegrees",
            "longitudeDegrees",
            "rollDegrees",
            "rollRateDegreesPerSecond",
            "pitchDegrees",
            "pitchRateDegreesPerSecond",
            "yawDegrees",
            "yawRateDegreesPerSecond",
            "zVelocityMetersPerSecond",
            "groundSpeedMetersPerSecond"
        };

};
