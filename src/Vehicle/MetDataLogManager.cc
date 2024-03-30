#include "QGCApplication.h"
#include "MetDataLogManager.h"
#include "SettingsManager.h"

MetDataLogManager::MetDataLogManager(QObject *parent) : QObject(parent)
{
    connect(&_metRawCsvTimer, &QTimer::timeout, this, &MetDataLogManager::_writeMetRawCsvLine);
    _metRawCsvTimer.start(1000);
}

MetDataLogManager::~MetDataLogManager()
{
    _metRawCsvFile.close();
}

void MetDataLogManager::_initializeMetRawCsv()
{
    QString now = QDateTime::currentDateTime().toString("MM-dd-yyyy_hh-mm-ss");
    QString metRawFileName = QString("RAW_%1.csv").arg(now);
    QDir saveDir(qgcApp()->toolbox()->settingsManager()->appSettings()->messagesRawSavePath());
    _metRawCsvFile.setFileName(saveDir.absoluteFilePath(metRawFileName));

    if (!_metRawCsvFile.open(QIODevice::Append)) {
        qCWarning(VehicleLog) << "unable to open raw message file for csv logging, Stopping csv logging!";
        return;
    }

    QTextStream stream(&_metRawCsvFile);

    qCDebug(VehicleLog) << "Facts logged to csv:" << metFactHeaders;
    stream << metFactHeaders.join(",") << "\r\n";
}

void MetDataLogManager::_writeMetRawCsvLine()
{
    // Only save the logs after the the vehicle gets armed, unless "Save logs even if vehicle was not armed" is checked
    Vehicle* _activeVehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
    if(!_metRawCsvFile.isOpen() && _activeVehicle && _activeVehicle->armed()) {
        _initializeMetRawCsv();
    }

    if(!_metRawCsvFile.isOpen() || !_activeVehicle || !_activeVehicle->armed()) {
        return;
    }

    QStringList metFactValues;
    QTextStream stream(&_metRawCsvFile);

    FactGroup* factGroup = nullptr;
    factGroup = _activeVehicle->getFactGroup("temperature");

    if (!factGroup) {
        return;
    }

    // Write timestamp to csv file
    for (const auto &factName : metFactNames) {
        if(!factGroup->factExists(factName)) {
            qCWarning(VehicleLog) << "Fact does not exist: " << factName;
            continue;
        }
        metFactValues << factGroup->getFact(factName)->rawValueString();
    }

    stream << metFactValues.join(",") << "\r\n";
}
