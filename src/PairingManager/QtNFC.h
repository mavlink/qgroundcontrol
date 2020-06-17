/****************************************************************************
 *
 *   (c) 2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QNdefMessage>
#include <QNearFieldManager>
#include <QNearFieldTarget>

#include "QGCLoggingCategory.h"

Q_DECLARE_LOGGING_CATEGORY(PairingNFCLog)

class PairingNFC : public QObject
{
    Q_OBJECT

public:
    PairingNFC();

    void start();

    void stop();

signals:
    void parsePairingJson(QString json);

public slots:
    void targetDetected(QNearFieldTarget *target);
    void targetLost(QNearFieldTarget *target);
    void handlePolledNdefMessage(QNdefMessage message);
    void targetError(QNearFieldTarget::Error error, const QNearFieldTarget::RequestId& id);
    void handleRequestCompleted(const QNearFieldTarget::RequestId& id);

private:
    bool _exitThread = false;    ///< true: signal thread to exit
    QNearFieldManager *manager = nullptr;
};
