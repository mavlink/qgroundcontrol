/****************************************************************************
 *
 *   (c) 2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/
#include "PairingManager.h"
#include "PairingNFC.h"
#include "QGCApplication.h"
#include <QSoundEffect>

QGC_LOGGING_CATEGORY(PairingNFCLog, "PairingNFCLog")

extern "C" {
#include <TML/inc/tool.h>
#include <NfcTask/inc/ndef_helper.h>
//-----------------------------------------------------------------------------
void logmsg(char *buf)
{
    QString s = buf;
    s.replace("\n", "");
    qCDebug(PairingNFCLog) << s;
}

//-----------------------------------------------------------------------------
void NdefPull_Cb(unsigned char *pNdefMessage, unsigned short NdefMessageSize)
{
    Q_UNUSED(NdefMessageSize);
    unsigned char *pNdefRecord = pNdefMessage;
    NdefRecord_t NdefRecord;
    unsigned char save;

    if (!pNdefMessage)
    {
        qCDebug(PairingNFCLog) << "Issue during NDEF message reception (check provisioned buffer size)";
        return;
    }

    QSoundEffect _nfcSound;
    _nfcSound.setSource(QUrl::fromUserInput("qrc:/auterion/wav/beep.wav"));
    _nfcSound.setVolume(0.9);
    _nfcSound.play();

    while (pNdefRecord)
    {
        qCDebug(PairingNFCLog) << "NDEF record received";

        NdefRecord = DetectNdefRecordType(pNdefRecord);

        switch(NdefRecord.recordType)
        {
        case MEDIA_VCARD:
            save = NdefRecord.recordPayload[NdefRecord.recordPayloadSize];
            NdefRecord.recordPayload[NdefRecord.recordPayloadSize] = '\0';
            qCDebug(PairingNFCLog) << "   vCard: " << reinterpret_cast<char *>(NdefRecord.recordPayload);
            NdefRecord.recordPayload[NdefRecord.recordPayloadSize] = save;
            break;

        case WELL_KNOWN_SIMPLE_TEXT:
        {
            save = NdefRecord.recordPayload[NdefRecord.recordPayloadSize];
            NdefRecord.recordPayload[NdefRecord.recordPayloadSize] = '\0';
            QString text = reinterpret_cast<char *>(&NdefRecord.recordPayload[NdefRecord.recordPayload[0]+1]);
            qCDebug(PairingNFCLog) << "   Text: " << text;
            qgcApp()->toolbox()->pairingManager()->jsonReceived(text);
            NdefRecord.recordPayload[NdefRecord.recordPayloadSize] = save;
            break;
        }

        default:
            qCDebug(PairingNFCLog) << "   Unsupported NDEF record, cannot parse";
            break;
        }
        pNdefRecord = GetNextRecord(pNdefRecord);
    }
}
//-----------------------------------------------------------------------------
}

// Discovery loop configuration according to the targeted modes of operation
static unsigned char discoveryTechnologies[] = {
    MODE_POLL | TECH_PASSIVE_NFCA,
    MODE_POLL | TECH_PASSIVE_NFCF,
    MODE_POLL | TECH_PASSIVE_NFCB,
    MODE_POLL | TECH_PASSIVE_15693,
};

//-----------------------------------------------------------------------------
PairingNFC::PairingNFC()
{
}

//-----------------------------------------------------------------------------
void PairingNFC::start()
{
    if (!isRunning()) {
        _exitThread = false;
        QThread::start();
    }
}

//-----------------------------------------------------------------------------
void PairingNFC::stop()
{
    if (isRunning()) {
        _exitThread = true;
    }
}

//-----------------------------------------------------------------------------
void PairingNFC::run()
{
    NxpNci_RfIntf_t RfInterface;

    // Register callback for reception of NDEF message from remote cards
    RW_NDEF_RegisterPullCallback(reinterpret_cast<void*>(*NdefPull_Cb));

    // Open connection to NXPNCI device
    if (NxpNci_Connect() == NFC_ERROR) {
        qCDebug(PairingNFCLog) << "Error: cannot connect to NXPNCI device";
        return;
    }

    if (NxpNci_ConfigureSettings() == NFC_ERROR) {
        qCDebug(PairingNFCLog) << "Error: cannot configure NXPNCI settings";
        return;
    }

    if (NxpNci_ConfigureMode(NXPNCI_MODE_RW) == NFC_ERROR)
    {
        qCDebug(PairingNFCLog) << "Error: cannot configure NXPNCI";
        return;
    }

    // Start Discovery
    if (NxpNci_StartDiscovery(discoveryTechnologies,sizeof(discoveryTechnologies)) != NFC_SUCCESS)
    {
        qCDebug(PairingNFCLog) << "Error: cannot start discovery";
        return;
    }

    while(!_exitThread)
    {
        qCDebug(PairingNFCLog) << "Waiting for NFC connection";
        qgcApp()->toolbox()->pairingManager()->setStatusMessage(PairingManager::PairingActive, tr("Waiting for NFC connection"));

        // Wait until a peer is discovered
        while(NxpNci_WaitForDiscoveryNotification(&RfInterface) != NFC_SUCCESS);

        if ((RfInterface.ModeTech & MODE_MASK) == MODE_POLL)
        {
            task_nfc_reader(RfInterface);
        }
        else
        {
            qCDebug(PairingNFCLog) << "Wrong discovery";
        }
    }
    qgcApp()->toolbox()->pairingManager()->setStatusMessage(PairingManager::PairingIdle, "");
    qCDebug(PairingNFCLog) << "NFC: Stop";
}

//-----------------------------------------------------------------------------
void PairingNFC::task_nfc_reader(NxpNci_RfIntf_t RfIntf)
{
    qgcApp()->toolbox()->pairingManager()->setStatusMessage(PairingManager::PairingActive, tr("Device detected"));
    qCDebug(PairingNFCLog) << "NFC: Device detected";
    // For each discovered cards
    while(!_exitThread) {
        // What's the detected card type ?
        switch(RfIntf.Protocol) {
        case PROT_T1T:
        case PROT_T2T:
        case PROT_T3T:
        case PROT_ISODEP:
            // Process NDEF message read
            NxpNci_ProcessReaderMode(RfIntf, READ_NDEF);
            break;

        case PROT_ISO15693:
            break;

        case PROT_MIFARE:
            break;

        default:
            break;
        }

        // If more cards (or multi-protocol card) were discovered (only same technology are supported) select next one
        if (RfIntf.MoreTags) {
            if (NxpNci_ReaderActivateNext(&RfIntf) == NFC_ERROR)
                break;
        }
        // Otherwise leave
        else
            break;
    }

    // Wait for card removal
    NxpNci_ProcessReaderMode(RfIntf, PRESENCE_CHECK);

    qgcApp()->toolbox()->pairingManager()->setStatusMessage(PairingManager::PairingActive, tr("Device removed"));
    qCDebug(PairingNFCLog) << "NFC device removed";

    // Restart discovery loop
    NxpNci_StopDiscovery();
    NxpNci_StartDiscovery(discoveryTechnologies, sizeof(discoveryTechnologies));
}

//-----------------------------------------------------------------------------

