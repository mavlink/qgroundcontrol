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
            "ASL",
            "Time",
            "Pressure",
            "Air Temp",
            "Rel Hum",
            "Wind Speed",
            "Wind Direction",
            "Latitude",
            "Longitude",
            "Roll",
            "Roll Rate",
            "Pitch",
            "Pitch Rate",
            "Yaw",
            "Yaw Rate",
            "Ascent Rate",
            "Speed Over Ground"
        };

        QStringList metFactNames = {
            "altitudeMetersMSL",
            "timeUnixMilliseconds",
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
