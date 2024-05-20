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
    _altLevelMsgTimer.start(20); // below nyquist rate for 50ms balancedDataFrequency to ensure no data is missed
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
    /* TAD 4/24/24 break if the current ALM has already been processed */
    QString s = factGroup->getFact("update")->rawValueString();

    if (s != QString("1")){
        return;
    }

    // only capture ALM data during ascent
    if(factGroup->getFact("ascending")->rawValue().toBool() == false) {
        return;
    }

    //update ascent number
    if(factGroup->getFact("ascents")->rawValue().toInt() != ascentNumber) {
        ascentNumber = factGroup->getFact("ascents")->rawValue().toInt();
        emit ascentNumberChanged();
        // clear previous data points
        _tempAltLevelMsgList.clear();
    }

    // store exact time to compare with previous time, skip logging if same
    QString time = factGroup->getFact("time")->rawValueString();
    if(QString::compare(prevTime, time) == 0) {
        return;
    }
    prevTime = time;
    QString displayTime = time;
    // truncate time string to nearest second
    if(displayTime.indexOf('.') != -1) {
        displayTime.truncate(time.indexOf('.'));
    }
    tempAltLevelMsg_t* tempAltLevelMsg = new tempAltLevelMsg_t();
    tempAltLevelMsg->altitude         = factGroup->getFact("asl"          )->rawValueString();
    tempAltLevelMsg->time             = displayTime;
    tempAltLevelMsg->pressure         = factGroup->getFact("pressure"     )->rawValueString();
    tempAltLevelMsg->temperature      = factGroup->getFact("airTemp"      )->rawValueString();
    tempAltLevelMsg->relativeHumidity = factGroup->getFact("relHum"       )->rawValueString();
    tempAltLevelMsg->windSpeed        = factGroup->getFact("windSpeed"    )->rawValueString();
    tempAltLevelMsg->windDirection    = factGroup->getFact("windDirection")->rawValueString();
    _tempAltLevelMsgList.append(tempAltLevelMsg);

    /* record that we processed the current ALM */
    factGroup->getFact("update")->setRawValue(0);

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
