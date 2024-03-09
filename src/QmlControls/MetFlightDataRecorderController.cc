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
#include "QGCCorePlugin.h"
#include "SettingsManager.h"
#include <QSettings>
// for test data only
#include <cstdlib>
#include <ctime>

double generateRandomDouble(double lowerBound, double upperBound) {
    double randomValue = lowerBound + static_cast<double>(rand()) / (static_cast<double>(RAND_MAX/(upperBound-lowerBound)));
    return randomValue;
}

MetFlightDataRecorderController::MetFlightDataRecorderController(QQuickItem* parent)
{

    // test data
    srand(time(nullptr));
    for(int i = 0; i < 15; i++) {
        tempAltLevelMsg_t* tempAltLevelMsg = new tempAltLevelMsg_t();
        tempAltLevelMsg->altitude = generateRandomDouble(0, 1000);
        tempAltLevelMsg->time = generateRandomDouble(0, 2000);
        tempAltLevelMsg->pressure = generateRandomDouble(0, 100);
        tempAltLevelMsg->temperature = generateRandomDouble(-50, 50);
        tempAltLevelMsg->relativeHumidity = generateRandomDouble(0, 100);
        tempAltLevelMsg->windSpeed = generateRandomDouble(0, 200);
        tempAltLevelMsg->windDirection = generateRandomDouble(0, 360);
        _tempAltLevelMsgList.append(tempAltLevelMsg);
    }
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
}

void MetFlightDataRecorderController::goToFile()
{
    // open file explorer window
    // system("explorer C:\\");

    qDebug() << "Opening file explorer window";
    QString savePath = qgcApp()->toolbox()->settingsManager()->appSettings()->messagesRawSavePath();
    QDesktopServices::openUrl(QUrl::fromLocalFile(savePath));
}
