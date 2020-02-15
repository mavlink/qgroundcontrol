/****************************************************************************
 *
 *   (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef APMSubMotorComponentController_H
#define APMSubMotorComponentController_H

#include <QObject>
#include "FactPanelController.h"
#include "Vehicle.h"

/// MVC Controller for APMSubMotorComponent.qml.
class APMSubMotorComponentController : public FactPanelController
{
    Q_OBJECT

public:
    APMSubMotorComponentController(void);
    Q_PROPERTY(QString motorDetectionMessages READ motorDetectionMessages NOTIFY motorDetectionMessagesChanged);
    QString motorDetectionMessages() const {return _motorDetectionMessages;};

signals:
    void motorDetectionMessagesChanged();

private slots:
    void handleNewMessages(int uasid, int componentid, int severity, QString text);

private:
    QString _motorDetectionMessages;
};

#endif
