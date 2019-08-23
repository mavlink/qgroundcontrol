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
#include <QThread>

#include "QGCLoggingCategory.h"
extern "C" {
#include <NfcLibrary/inc/Nfc.h>
}

Q_DECLARE_LOGGING_CATEGORY(PairingNFCLog)

class PairingNFC : public QThread
{
    Q_OBJECT

public:
    PairingNFC();

    virtual void start();

    virtual void stop();

signals:
    void parsePairingJson(QString json);

private:
    bool         _exitThread = false;    ///< true: signal thread to exit

    // Override from QThread
    virtual void run(void);

    void task_nfc_reader(NxpNci_RfIntf_t RfIntf);
};
