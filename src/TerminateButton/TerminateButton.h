/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

/// @file
/// @brief

#pragma once

#include <QtCore/QObject>
#include "LinkConfiguration.h"
#include "SerialLink.h"

class TerminateButton : public QObject
{
    Q_OBJECT
public:
    explicit TerminateButton(QObject* parent = nullptr);
    void setupSerialPort(SerialLink* serialLink);

private:
    SharedLinkConfigurationPtr _link;

signals:
    void terminateSignalReceived();

private slots:
    void handleSerialData(const QByteArray& data);
};
