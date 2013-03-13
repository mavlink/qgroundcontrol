//#include <QJsonDocument>
#include <QFile>

#include "PX4FirmwareUpgradeWorker.h"

#include <SerialLink.h>
#include <QGC.h>
#include "uploader.h"

#include <QDebug>

#define PROTO_GET_SYNC		0x21
#define PROTO_EOC            0x20
#define PROTO_NOP		0x00
#define PROTO_OK		0x10
#define PROTO_FAILED		0x11
#define PROTO_INSYNC		0x12

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
    return worker;
}


void PX4FirmwareUpgradeWorker::startContinousScan()
{
    exitThread = false;
    while (!exitThread) {
//        if (detect()) {
//            break;
//        }
        QGC::SLEEP::msleep(20);
    }

    if (exitThread) {
        link->disconnect();
        delete link;
        exit(0);
    }
}

void PX4FirmwareUpgradeWorker::detect()
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
            //continue;
        }

        link->setPortName(ports->at(i));
        // Open port and talk to it
        link->connect();
        char buf[2] = { PROTO_GET_SYNC, PROTO_EOC };
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

    //ui.portName->setCurrentIndex(ui.baudRate->findText(QString("%1").arg(this->link->getPortName())));

    // Set port

    // Load current link config

}

void PX4FirmwareUpgradeWorker::receiveBytes(LinkInterface* link, QByteArray b)
{
    for (int position = 0; position < b.size(); position++) {
        qDebug() << "BYTES";
        qDebug() << (char)(b[position]);
        if (((const char)b[position]) == PROTO_INSYNC)
        {
            qDebug() << "SYNC";
            insync = true;
        }

        if (insync && ((const char)b[position]) == PROTO_OK)
        {
            emit detectionStatusChanged("Found PX4 board");
        }
    }

    printf("\n");
}

void PX4FirmwareUpgradeWorker::loadFirmware(const QString &filename)
{
    qDebug() << __FILE__ << __LINE__ << "LOADING FW" << filename;

    PX4_Uploader uploader;
    const char* filenames[2];
    filenames[0] = filename.toStdString().c_str();
    filenames[1] = NULL;
    uploader.upload(filenames, "/dev/tty.usbmodem1");

//    QFile f(filename);
//    if (f.open(QIODevice::ReadOnly))
//    {
//        QByteArray buf = f.readAll();
//        f.close();
//        firmware = QJsonDocument::fromBinaryData(buf);
//        if (firmware.isNull()) {
//            emit upgradeStatusChanged(tr("Failed decoding file %1").arg(filename));
//        } else {
//            emit upgradeStatusChanged(tr("Ready to flash %1").arg(filename));
//        }
//    } else {
//        emit upgradeStatusChanged(tr("Failed opening file %1").arg(filename));
//    }
}

void PX4FirmwareUpgradeWorker::upgrade()
{
    emit upgradeStatusChanged(tr("Starting firmware upgrade.."));
}

void PX4FirmwareUpgradeWorker::abort()
{
    exitThread = true;
}
