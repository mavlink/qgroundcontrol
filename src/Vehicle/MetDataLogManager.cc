#include "QGCApplication.h"
#include "MetDataLogManager.h"
#include "SettingsManager.h"

MetDataLogManager::MetDataLogManager(QGCApplication* app, QGCToolbox* toolbox) : QGCTool(app, toolbox)
{
    connect(&_metRawCsvTimer, &QTimer::timeout, this, &MetDataLogManager::_writeMetRawCsvLine);
    connect(&_metAlmCsvTimer, &QTimer::timeout, this, &MetDataLogManager::_writeMetAlmCsvLine);
    _metRawCsvTimer.start(20); // set below nyquist rate for 50ms balancedDataFrequency to ensure no data is missed
    _metAlmCsvTimer.start(20); // set below nyquist rate for 50ms balancedDataFrequency to ensure no data is missed
}

MetDataLogManager::~MetDataLogManager()
{
    _metRawCsvFile.close();
    _metAlmCsvFile.close();
}

void MetDataLogManager::_initializeMetRawCsv()
{
    QString now = QDateTime::currentDateTime().toString("MM-dd-yyyy_hh-mm-ss");
    QString metRawFileName = QString("RAW_%1.txt").arg(now);
    QDir saveDir(qgcApp()->toolbox()->settingsManager()->appSettings()->messagesRawSavePath());
    _metRawCsvFile.setFileName(saveDir.absoluteFilePath(metRawFileName));

    if (!_metRawCsvFile.open(QIODevice::Append)) {
        qCWarning(VehicleLog) << "unable to open raw message file for text logging, Stopping text logging!";
        return;
    }

    QTextStream stream(&_metRawCsvFile);

    stream << metRawFactHeaders.join(",") << "\r\n";
    stream << metRawFactUnits.join(",") << "\r\n";
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

    // make sure we're not logging the same data again
    QString timestamp = factGroup->getFact("timeUnixSeconds")->rawValueString();
    if (timestamp == _latestRawTimestamp) {
        return;
    } else {
        _latestRawTimestamp = timestamp;
    }

    for (const auto &factName : metRawFactNames) {
        if(!factGroup->factExists(factName)) {
            qCWarning(VehicleLog) << "Fact does not exist: " << factName;
            continue;
        }
        metFactValues << factGroup->getFact(factName)->rawValueString();
    }

    stream << metFactValues.join(",") << "\r\n";
}

void MetDataLogManager::setAscentNumber(int number)
{
    _ascentNumber = number;
    // close the file to allow next logging to start with new ascent number
    if(_metAlmCsvFile.isOpen()) {
        _metAlmCsvFile.close();
    }
}

void MetDataLogManager::setFlightFileName(QString flightName)
{
    // allow next logging to start with new flight name
    if(_metAlmCsvFile.isOpen()) {
        _metAlmCsvFile.close();
    }
    _flightName = flightName;
    setAscentNumber(1);
}

void MetDataLogManager::_initializeMetAlmCsv()
{
    int copyNumber = 1;
    QString metAlmFileName = QString("%1_%2_%3.txt").arg(_flightName).arg(copyNumber).arg(_ascentNumber);
    QDir saveDir(qgcApp()->toolbox()->settingsManager()->appSettings()->messagesAltLevelSavePath());
    _metAlmCsvFile.setFileName(saveDir.absoluteFilePath(metAlmFileName));

    while (_metAlmCsvFile.exists()) {
        copyNumber++;
        metAlmFileName = QString("%1_%2_%3.txt").arg(_flightName).arg(copyNumber).arg(_ascentNumber);
        _metAlmCsvFile.setFileName(saveDir.absoluteFilePath(metAlmFileName));
    }

    if (!_metAlmCsvFile.open(QIODevice::Append)) {
        qCWarning(VehicleLog) << "unable to open ALM message file for text logging, Stopping text logging!";
        return;
    }
    QTextStream stream(&_metAlmCsvFile);

    qCDebug(VehicleLog) << "Facts logged to csv:" << metAlmFactHeaders;
    stream << metAlmFactHeaders.join(",") << "\r\n";
    stream << metAlmFactUnits.join(",") << "\r\n";
}

void MetDataLogManager::_writeMetAlmCsvLine()
{
    FactGroup* factGroup = nullptr;
    Vehicle* _activeVehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
    if(!_activeVehicle || !_activeVehicle->armed()) {
        return;
    }
    factGroup = _activeVehicle->getFactGroup("temperature");

    if (!factGroup) {
        return;
    }

    // make sure we're not logging the same data again
    QString timestamp = factGroup->getFact("time")->rawValueString();
    if (timestamp == _latestAlmTimestamp) {
        return;
    } else {
        _latestAlmTimestamp = timestamp;
    }

    // only capture ALM data during ascent
    if(factGroup->getFact("ascending")->rawValue().toBool() == false) {
        return;
    }

    if(!_metAlmCsvFile.isOpen()) {
        _initializeMetAlmCsv();
    }



    QStringList metFactValues;
    QTextStream stream(&_metAlmCsvFile);

    for (const auto &factName : metAlmFactNames) {
        if(!factGroup->factExists(factName)) {
            qCWarning(VehicleLog) << "Fact does not exist: " << factName;
            continue;
        }
        metFactValues << factGroup->getFact(factName)->rawValueString();
    }

    stream << metFactValues.join(",") << "\r\n";
}

