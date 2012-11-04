#include "PX4FirmwareUpgradeWorker.h"

#include <SerialLink.h>
#include <QGC.h>

#include <QDebug>

// protocol bytes
#define INSYNC		0x12
#define EOC		0x20

// reply bytes
#define OK		0x10
#define FAILED		0x11
#define INVALID		0x13	// rev3+

// command bytes
#define NOP		0x00	// guaranteed to be discarded by the bootloader
#define GET_SYNC	0x21
#define GET_DEVICE	0x22
#define CHIP_ERASE	0x23
#define CHIP_VERIFY	0x24	// rev2 only
#define PROG_MULTI	0x27
#define READ_MULTI	0x28	// rev2 only
#define GET_CRC		0x29	// rev3+
#define REBOOT		0x30

#define INFO_BL_REV	1	// bootloader protocol revision
#define BL_REV_MIN	2	// minimum supported bootloader protocol
#define BL_REV_MAX	3	// maximum supported bootloader protocol
#define INFO_BOARD_ID	2	// board type
#define INFO_BOARD_REV	3	// board revision
#define INFO_FLASH_SIZE	4	// max firmware size in bytes

PX4FirmwareUpgradeWorker::PX4FirmwareUpgradeWorker(QObject *parent) :
    QObject(parent),
    link(NULL)
{
}

PX4FirmwareUpgradeWorker* PX4FirmwareUpgradeWorker::putWorkerInThread(QObject *parent)
{
    PX4FirmwareUpgradeWorker *worker = new PX4FirmwareUpgradeWorker;
    QThread *workerThread = new QThread(parent);

    connect(workerThread, SIGNAL(started()), worker, SLOT(startContinousScan()));
    connect(workerThread, SIGNAL(finished()), worker, SLOT(deleteLater()));
    worker->moveToThread(workerThread);

    // Starts an event loop, and emits workerThread->started()
    workerThread->start();
}


bool PX4FirmwareUpgradeWorker::startContinousScan()
{
    while (true) {
        if (detect()) {
            break;
        }
    }

    return true;
}

bool PX4FirmwareUpgradeWorker::detect()
{
    if (!link)
    {
        link = new SerialLink("", 921600);
        connect(link, SIGNAL(bytesReceived(LinkInterface*,QByteArray)), this, SLOT(receiveBytes(LinkInterface*,QByteArray)));
    }

    // Get a list of ports
    QVector<QString>* ports = link->getCurrentPorts();

    // Scan
    for (int i = 0; i < ports->size(); i++)
    {
        // Ignore known wrong link names

        if (ports->at(i).contains("Bluetooth")) {
            continue;
        }

        link->setPortName(ports->at(i));
        // Open port and talk to it
        link->connect();
        char buf[2] = { GET_SYNC, EOC };
        if (!link->isConnected()) {
            continue;
        }
        // Send sync request
        insync = false;
        link->writeBytes(buf, 2);
        // Wait for response
        QGC::SLEEP::msleep(20);

        if (insync)
            emit validPortFound(ports->at(i));
            break;
    }

    if (insync) {
        return true;
    } else {
        return false;
    }

    //ui.portName->setCurrentIndex(ui.baudRate->findText(QString("%1").arg(this->link->getPortName())));

    // Set port

    // Load current link config

}

void PX4FirmwareUpgradeWorker::receiveBytes(LinkInterface* link, QByteArray b)
{
    for (int position = 0; position < b.size(); position++) {
        qDebug() << "BYTES";
        qDebug() << std::hex << (char)(b[position]);
        if (((const char)b[position]) == INSYNC)
        {
            qDebug() << "SYNC";
            insync = true;
            emit detectionStatusChanged("Found PX4 board");
        }
    }

    printf("\n");
}

bool PX4FirmwareUpgradeWorker::upgrade(const QString &filename)
{

}
