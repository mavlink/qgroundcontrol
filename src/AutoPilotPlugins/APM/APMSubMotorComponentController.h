/****************************************************************************
 *
 *   (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "FactPanelController.h"

/// MVC Controller for APMSubMotorComponent.qml.
class APMSubMotorComponentController : public FactPanelController
{
    Q_OBJECT
    Q_PROPERTY(QString motorDetectionMessages READ motorDetectionMessages NOTIFY motorDetectionMessagesChanged)

public:
    explicit APMSubMotorComponentController(QObject *parent = nullptr);

    QString motorDetectionMessages() const { return _motorDetectionMessages; };

signals:
    void motorDetectionMessagesChanged();

private slots:
    void _handleNewMessages(int sysid, int componentid, int severity, const QString &text, const QString &description);

private:
    QString _motorDetectionMessages;
};

