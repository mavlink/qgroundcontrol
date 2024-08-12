/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "TerminateButton.h"
#include "QGCApplication.h"
#include "QGCToolbox.h"
#include "LinkManager.h"
#include "LinkConfiguration.h"
#include "SerialLink.h"

TerminateButton::TerminateButton(QObject* parent)
    : QObject(parent) {}

void TerminateButton::setupSerialPort(SerialLink* serialLink) {

    // connect(serialLink, &SerialLink::bytesReceived, this, &TerminateButton::handleSerialData);
}

void TerminateButton::handleSerialData(const QByteArray& data) {
    if (data.contains("TERMINATE")) {
        emit terminateSignalReceived();
    }
}
