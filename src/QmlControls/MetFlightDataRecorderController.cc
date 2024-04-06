/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include <AppSettings.h>
#include "MetFlightDataRecorderController.h"
#include "MetDataLogManager.h"
#include "QGCCorePlugin.h"
#include "SettingsManager.h"
#include <QSettings>

double generateRandomDouble(double lowerBound, double upperBound) {
    double randomValue = lowerBound + static_cast<double>(rand()) / (static_cast<double>(RAND_MAX/(upperBound-lowerBound)));
    return randomValue;
}

MetFlightDataRecorderController::MetFlightDataRecorderController(QQuickItem* parent)
{
    connect(this, &MetFlightDataRecorderController::flightFileNameChanged, qgcApp()->toolbox()->metDataLogManager(), &MetDataLogManager::setFlightFileName);
    connect(&_altLevelMsgTimer, &QTimer::timeout, this, &MetFlightDataRecorderController::addAltLevelMsg);
    _altLevelMsgTimer.start(1000);
}

void MetFlightDataRecorderController::addAltLevelMsg()
{

    Vehicle* _activeVehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
    if(!_activeVehicle || !_activeVehicle->armed()) {
        return;
    }
    FactGroup* factGroup = nullptr;
    factGroup = _activeVehicle->getFactGroup("temperature");
    if(!factGroup) {
        return;
    }

    // // truncate time string to nearest second
    QString time = factGroup->getFact("timeUnixSeconds")->rawValueString();
    if(time.indexOf('.') != -1) {
        time.truncate(time.indexOf('.'));
    }

    tempAltLevelMsg_t* tempAltLevelMsg = new tempAltLevelMsg_t();
    tempAltLevelMsg->altitude         = factGroup->getFact("altitudeMetersMSL"        )->rawValueString();
    tempAltLevelMsg->time             = time;
    tempAltLevelMsg->pressure         = factGroup->getFact("absolutePressureMillibars")->rawValueString();
    tempAltLevelMsg->temperature      = factGroup->getFact("temperatureCelsius"       )->rawValueString();
    tempAltLevelMsg->relativeHumidity = factGroup->getFact("relativeHumidity"         )->rawValueString();
    tempAltLevelMsg->windSpeed        = factGroup->getFact("windSpeedMetersPerSecond" )->rawValueString();
    tempAltLevelMsg->windDirection    = factGroup->getFact("windBearingDegrees"       )->rawValueString();
    _tempAltLevelMsgList.append(tempAltLevelMsg);

    emit tempAltLevelMsgListChanged();
}

void MetFlightDataRecorderController::setFlightFileName(QString _flightFileName)
{
    this->flightFileName = _flightFileName;
    bool isValid = this->flightFileName.length() > 0;
    if(isValid) {
        for (const QChar ch : _flightFileName) {
            if (!this->flightNameValidChars.contains(ch)) {
                isValid = false;
            }
        }
    }
    if(isValid != this->flightNameValid) {
        this->flightNameValid = isValid;
        emit flightNameValidChanged();
    }
    if(isValid) {
        emit flightFileNameChanged(_flightFileName);
    }
}

void MetFlightDataRecorderController::goToFile()
{
    QString savePath = qgcApp()->toolbox()->settingsManager()->appSettings()->messagesAltLevelSavePath();
    QDesktopServices::openUrl(QUrl::fromLocalFile(savePath));
}
