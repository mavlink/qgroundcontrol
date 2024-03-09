/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>

#include "QmlObjectListModel.h"
#include "QGCApplication.h"

class tempAltLevelMsg_t : public QObject
{
    Q_OBJECT

    Q_PROPERTY(double altitude MEMBER altitude NOTIFY msgChanged)
    Q_PROPERTY(double time MEMBER time NOTIFY msgChanged)
    Q_PROPERTY(double pressure MEMBER pressure NOTIFY msgChanged)
    Q_PROPERTY(double temperature MEMBER temperature NOTIFY msgChanged)
    Q_PROPERTY(double relativeHumidity MEMBER relativeHumidity NOTIFY msgChanged)
    Q_PROPERTY(double windSpeed MEMBER windSpeed NOTIFY msgChanged)
    Q_PROPERTY(double windDirection MEMBER windDirection NOTIFY msgChanged)

    public:
        double altitude;
        double time;
        double pressure;
        double temperature;
        double relativeHumidity;
        double windSpeed;
        double windDirection;

    signals:
        void msgChanged();
};

class MetFlightDataRecorderController : public QObject
{
    Q_OBJECT

public:
    MetFlightDataRecorderController(QQuickItem *parent = nullptr);

    Q_PROPERTY(QString flightFileName MEMBER flightFileName WRITE setFlightFileName)
    Q_PROPERTY(bool flightNameValid MEMBER flightNameValid NOTIFY flightNameValidChanged)
    Q_PROPERTY(int ascentNumber MEMBER ascentNumber NOTIFY ascentNumberChanged)
    Q_PROPERTY(QmlObjectListModel* tempAltLevelMsgList READ tempAltLevelMsgList NOTIFY tempAltLevelMsgListChanged)

    Q_INVOKABLE void goToFile();

    QString flightFileName;
    bool flightNameValid = false;
    int ascentNumber = 1;
    QmlObjectListModel* tempAltLevelMsgList() { return &_tempAltLevelMsgList; }

public slots:
    void setFlightFileName(QString flightFileName);

signals:
    void flightNameValidChanged();
    void ascentNumberChanged();
    void tempAltLevelMsgListChanged();

private:
    Q_DISABLE_COPY(MetFlightDataRecorderController)
    const QString flightNameValidChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_,+=(`~!@#$%^&(){}[];\"'. ";
    QmlObjectListModel _tempAltLevelMsgList;
};

QML_DECLARE_TYPE(MetFlightDataRecorderController)
