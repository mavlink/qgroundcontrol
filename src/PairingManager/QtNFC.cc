/****************************************************************************
 *
 *   (c) 2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/
#include "PairingManager.h"
#include "QtNFC.h"
#include "QGCApplication.h"
#include <QSoundEffect>

QGC_LOGGING_CATEGORY(PairingNFCLog, "PairingNFCLog")

#include <QNdefNfcTextRecord>

//-----------------------------------------------------------------------------
PairingNFC::PairingNFC()
{
}

//-----------------------------------------------------------------------------
void
PairingNFC::start()
{
    if (manager != nullptr) {
        return;
    }
    qgcApp()->toolbox()->pairingManager()->setStatusMessage(tr("Waiting for NFC connection"));
    qCDebug(PairingNFCLog) << "Waiting for NFC connection";

    manager = new QNearFieldManager(this);
    if (!manager->isAvailable()) {
        qWarning() << "NFC not available";
        delete manager;
        manager = nullptr;
        return;
    }

    QNdefFilter filter;
    filter.setOrderMatch(false);
    filter.appendRecord<QNdefNfcTextRecord>(1, UINT_MAX);
    // type parameter cannot specify substring so filter for "image/" below
    filter.appendRecord(QNdefRecord::Mime, QByteArray(), 0, 1);

    int result = manager->registerNdefMessageHandler(filter, this, SLOT(handleMessage(QNdefMessage, QNearFieldTarget*)));

    if (result < 0)
        qWarning() << "Platform does not support NDEF message handler registration";

    manager->startTargetDetection();
    connect(manager, &QNearFieldManager::targetDetected, this, &PairingNFC::targetDetected);
    connect(manager, &QNearFieldManager::targetLost, this, &PairingNFC::targetLost);
}

//-----------------------------------------------------------------------------
void
PairingNFC::stop()
{
    if (manager != nullptr) {
        qgcApp()->toolbox()->pairingManager()->setStatusMessage("");
        qCDebug(PairingNFCLog) << "NFC: Stop";
        manager->stopTargetDetection();
        delete manager;
        manager = nullptr;
    }
}

//-----------------------------------------------------------------------------
void
PairingNFC::targetDetected(QNearFieldTarget *target)
{
    if (!target) {
        return;
    }

    qgcApp()->toolbox()->pairingManager()->setStatusMessage(tr("Device detected"));
    qCDebug(PairingNFCLog) << "NFC: Device detected";
    connect(target, &QNearFieldTarget::ndefMessageRead, this, &PairingNFC::handlePolledNdefMessage);
    connect(target, SIGNAL(error(QNearFieldTarget::Error,QNearFieldTarget::RequestId)),
            this, SLOT(targetError(QNearFieldTarget::Error,QNearFieldTarget::RequestId)));
    connect(target, &QNearFieldTarget::requestCompleted, this, &PairingNFC::handleRequestCompleted);

    manager->setTargetAccessModes(QNearFieldManager::NdefReadTargetAccess);
    QNearFieldTarget::RequestId id = target->readNdefMessages();
    if (target->waitForRequestCompleted(id)) {
        qCDebug(PairingNFCLog) << "requestCompleted ";
        QVariant res = target->requestResponse(id);
        qCDebug(PairingNFCLog) << "Response:  " << res.toString();
    }
}

//-----------------------------------------------------------------------------
void
PairingNFC::handleRequestCompleted(const QNearFieldTarget::RequestId& id)
{
    Q_UNUSED(id);
    qCDebug(PairingNFCLog) << "handleRequestCompleted ";
}

//-----------------------------------------------------------------------------
void
PairingNFC::targetError(QNearFieldTarget::Error error, const QNearFieldTarget::RequestId& id)
{
    Q_UNUSED(id);
    qCDebug(PairingNFCLog) << "Error: " << error;
}

//-----------------------------------------------------------------------------
void
PairingNFC::targetLost(QNearFieldTarget *target)
{
    qgcApp()->toolbox()->pairingManager()->setStatusMessage(tr("Device removed"));
    qCDebug(PairingNFCLog) << "NFC: Device removed";
    if (target) {
        target->deleteLater();
    }
}

//-----------------------------------------------------------------------------
void
PairingNFC::handlePolledNdefMessage(QNdefMessage message)
{
    qCDebug(PairingNFCLog) << "NFC: Handle NDEF message";
//    QNearFieldTarget *target = qobject_cast<QNearFieldTarget *>(sender());
    for (const QNdefRecord &record : message) {
        if (record.isRecordType<QNdefNfcTextRecord>()) {
            QNdefNfcTextRecord textRecord(record);
            qgcApp()->toolbox()->pairingManager()->jsonReceived(textRecord.text());
        }
    }
}

//-----------------------------------------------------------------------------
