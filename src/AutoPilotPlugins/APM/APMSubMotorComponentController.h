#pragma once

#include <QtQmlIntegration/QtQmlIntegration>

#include "FactPanelController.h"

/// MVC Controller for APMSubMotorComponent.qml.
class APMSubMotorComponentController : public FactPanelController
{
    Q_OBJECT
    QML_ELEMENT
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
