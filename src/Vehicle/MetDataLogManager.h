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

        QStringList metFactHeaders = {"Air Temp A"};
        QStringList metFactNames   = {"temperature4"};

};
