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
    QString metRawFileName = QString("RAW%1.csv").arg(now);
    QDir saveDir(qgcApp()->toolbox()->settingsManager()->appSettings()->messagesRawSavePath());
    _metRawCsvFile.setFileName(saveDir.absoluteFilePath(metRawFileName));

    if (!_metRawCsvFile.open(QIODevice::Append)) {
        qCWarning(VehicleLog) << "unable to open raw message file for csv logging, Stopping csv logging!";
        return;
    }

    QTextStream stream(&_metRawCsvFile);

    qCDebug(VehicleLog) << "Facts logged to csv:" << metFactHeaders;
    stream << "Time," << metFactHeaders.join(",") << "\n";
}

void MetDataLogManager::_writeMetRawCsvLine()
{
    // Only save the logs after the the vehicle gets armed, unless "Save logs even if vehicle was not armed" is checked
    Vehicle* _activeVehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
    if(!_metRawCsvFile.isOpen() &&
        (_activeVehicle->armed() || qgcApp()->toolbox()->settingsManager()->appSettings()->telemetrySaveNotArmed()->rawValue().toBool())){
        _initializeMetRawCsv();
    }

    if(!_metRawCsvFile.isOpen()){
        return;
    }

    QStringList metFactValues;
    QTextStream stream(&_metRawCsvFile);

    FactGroup* factGroup = nullptr;
    //if (_factGroupName == "Vehicle") {
    factGroup = _activeVehicle->getFactGroup("Vehicle");
    //} else {
    //    factGroup = _activeVehicle->getFactGroup(_factGroupName);
    //}

    if (!factGroup) {
        return;
    }


    // Write timestamp to csv file
    metFactValues << QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"));
    // Write Vehicle's own facts
    for (const auto &factName : metFactNames) {
        metFactValues << factGroup->getFact(factName)->cookedValueString();
    }
    // write facts from Vehicle's FactGroups
    //for (const QString& groupName: factGroupNames()) {
        for (const QString& factName : factGroup->factNames()) {
            metFactValues << factGroup->getFact(factName)->cookedValueString();
        }
    //}

    stream << metFactValues.join(",") << "\n";
}
