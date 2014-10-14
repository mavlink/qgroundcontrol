//#include <QJsonDocument>
#include <QFile>

#include "qgcfirmwareupgradeworker.h"

#include <QGC.h>
#include "uploader.h"
#include <QSerialPortInfo>

#include <QDebug>

QGCFirmwareUpgradeWorker::QGCFirmwareUpgradeWorker(QObject *parent) :
    QObject(parent),
    _abortUpload(false),
    _filterBoardId(5),
    _port(NULL)
{
}

QGCFirmwareUpgradeWorker* QGCFirmwareUpgradeWorker::putWorkerInThread(const QString &filename, const QString &port, int boardId, bool abortOnFirstError)
{
    QGCFirmwareUpgradeWorker *worker = NULL;
    QThread *thread = NULL;

    worker = new QGCFirmwareUpgradeWorker;
    worker->setFilename(filename);
    worker->setPort(port);
    if (boardId >= 0)
        worker->setBoardId(boardId);
    worker->setAbortOnError(abortOnFirstError);
    thread = new QThread;

    worker->moveToThread(thread);
//    connect(worker, SIGNAL(error(QString)), parent, SLOT(errorString(QString)));
    connect(thread, SIGNAL(started()), worker, SLOT(loadFirmware()));
    connect(worker, SIGNAL(finished()), thread, SLOT(quit()));
    connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();

    // Starts an event loop, and emits workerThread->started()
    thread->start();
    return worker;
}

QGCFirmwareUpgradeWorker* QGCFirmwareUpgradeWorker::putDetectorInThread()
{
    QGCFirmwareUpgradeWorker *worker = NULL;
    QThread *thread = NULL;

    worker = new QGCFirmwareUpgradeWorker;
    thread = new QThread;

    worker->moveToThread(thread);
//    connect(worker, SIGNAL(error(QString)), parent, SLOT(errorString(QString)));
    connect(thread, SIGNAL(started()), worker, SLOT(detectBoards()));
    connect(worker, SIGNAL(finished()), thread, SLOT(quit()));
    connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();

    // Starts an event loop, and emits workerThread->started()
    thread->start();
    return worker;
}

void QGCFirmwareUpgradeWorker::setBoardId(int id) {
     = id;
}

void QGCFirmwareUpgradeWorker::setAbortOnError(bool abort) {
    _abortOnFirstError = abort;
}



void QGCFirmwareUpgradeWorker::setPort(const QString &port) {
    if (port.contains("Automatic")) {
        _fixedPortName = "";
    } else {
        _fixedPortName = port;
    }
}

void QGCFirmwareUpgradeWorker::setFilename(const QString &filename)
{
    this->filename = filename;
}

void QGCFirmwareUpgradeWorker::detectBoards()
{
    QString foundPortName;

    emit upgradeStatusChanged(tr("Detecting boards.."));
    
    // First step is to find which port the board is on

    // FIXME: Should we re-scan after a reboot? Could it show up on a dfferent port on Windows?
    while (!_abortUpload && foundPortName.isEmpty()) {

        qDebug() << "Find board loop";

        QGC::SLEEP::usleep(200000);

        QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();

        foreach (QSerialPortInfo info, ports) {

            // Check for valid handles
            if (info.portName().isEmpty())
                continue;

            // FIXME: Why enumerate if we are given a fixed port name?
            if ((_fixedPortName == info.portName() || (_fixedPortName.isEmpty() &&
                    (info.description().contains("PX4") || info.vendorIdentifier() == 9900 /* 3DR */)))) {

                qDebug() << "Found Board:";
                qDebug() << "\tport name:"       << info.portName();
                qDebug() << "\tdescription:"   << info.description();
                qDebug() << "\tsystem location:"   << info.systemLocation();
                qDebug() << "\tvendor ID:"       << info.vendorIdentifier();
                qDebug() << "\tproduct ID:"      << info.productIdentifier();
                
                foundPortName = info.portName();
                
                // Stupid windows fixes
#ifdef Q_OS_WIN
                // Handle port names
                foundPortName.prepend("\\\\.\\");
#endif
                break;
            }
        }
    }

    // We found the board, now try to open a connection to it

    while (!_abortUpload) {
        
        qDebug() << "UPGRADE LOOP";
        
        QGC::SLEEP::usleep(200000);
        
                
        if (_port == NULL) {
            _port = new QSerialPort(foundPortName);
            _port->setBaudRate(QSerialPort::Baud115200, QSerialPort::AllDirections);
            _port->setDataBits(QSerialPort::Data8);
            _port->setParity(QSerialPort::NoParity);
            _port->setStopBits(QSerialPort::OneStop);
            _port->setFlowControl(QSerialPort::NoFlowControl);
        } else {
            _port->close();
            // FIXME: Really?
            _port->setPortName(foundPortName);
        }
        
        emit upgradeStatusChanged(tr("Starting Uploader to read bootloader data"));
        
        PX4_Uploader uploader(_port);
        // Relay status to top-level UI
        connect(&uploader, SIGNAL(upgradeProgressChanged(int)), this, SIGNAL(upgradeProgressChanged(int)));
        connect(&uploader, SIGNAL(upgradeStatusChanged(QString)), this, SIGNAL(upgradeStatusChanged(QString)));
        
        // Die-hard flash the binary
        
#if 0
        // FIXME: Do we really need to check again?
        if ((info.description().contains("PX4") || info.vendorIdentifier() == 9900 /* 3DR */)) {
            emit upgradeStatusChanged(tr("Found PX4 board on port %1").arg(info.portName()));
        } else {
            emit upgradeStatusChanged(tr("No PX4 board found on port %1 (manual override)").arg(info.portName()));
        }
#else
        emit upgradeStatusChanged(tr("Found PX4 board on port %1").arg(foundPortName));
#endif
        
        //            quint32 board_id;
        //            quint32 board_rev;
        //            quint32 flash_size;
        //            bool insync = false;
        //            QString humanReadable;
        //uploader.get_bl_info(board_id, board_rev, flash_size, humanReadable, insync);
        
        qDebug() << "Beginning detection process";
        
        int board_id = -1;
        int ret = uploader.detect(board_id);
        
        qDebug() << "Detect done, result:" << ret << "Board ID:" << board_id;
        
        // bail out on success
        if (ret == 0 && board_id > 0) {
            
            QString board = uploader.getBoardName();
            QString bootLoader = uploader.getBootloaderName();
            
            emit detectFinished((ret == 0), board_id, board, bootLoader);
            
            emit upgradeStatusChanged(tr("Found board with ID #%1").arg(board_id));
            _port->close();
            return;
        }
    }
    
    emit upgradeStatusChanged(tr("No board found."));

    _abortUpload = false;
    this->deleteLater();

}

void QGCFirmwareUpgradeWorker::loadFirmware()
{
#if 0
    // FIXME
    qDebug() << __FILE__ << __LINE__ << "LOADING FW" << filename;

    emit upgradeStatusChanged(tr("Attempting to upload file:\n%1").arg(filename));
    emit upgradeProgressChanged(0);

    while (!_abortUpload) {

        QGC::SLEEP::usleep(200000);

        QList<QextPortInfo> ports = QextSerialEnumerator::getPorts();

        int ret;
        bool boardFound = false;
        bool wrongBoard = false;

        foreach (QextPortInfo info, ports) {

            // Check for valid handles
            if (info.portName.isEmpty())
                continue;

            if ((filename.endsWith(".bin") || filename.endsWith(".px4")) && (_fixedPortName == info.portName || (_fixedPortName.isEmpty() &&
                    (info.physName.contains("PX4") || info.vendorID == 9900 /* 3DR */)))) {

                qDebug() << "port name:"       << info.portName;
                qDebug() << "friendly name:"   << info.friendName;
                qDebug() << "physical name:"   << info.physName;
                qDebug() << "enumerator name:" << info.enumName;
                qDebug() << "vendor ID:"       << info.vendorID;
                qDebug() << "product ID:"      << info.productID;

                qDebug() << "===================================";

                QString openString = info.portName;

                // Stupid windows fixes
#ifdef Q_OS_WIN
                // Handle port names
                openString.prepend("\\\\.\\");
#endif


                qDebug() << "UPLOAD ATTEMPT";

                // Spawn upload thread

                if (port == NULL) {
                    PortSettings settings = {BAUD115200, DATA_8, PAR_NONE, STOP_1, FLOW_OFF, 10};

                    port = new QextSerialPort(openString, settings, QextSerialPort::Polling);
                    port->setTimeout(0);
                    port->setQueryMode(QextSerialPort::Polling);
                    // XXX black magic to convince Qextserialport to cooperate on first attempt
                    port->close();
                    port->setPortName(openString);
                } else {
                    port->close();
                    port->setPortName(openString);
                }

                qDebug() << "Starting uploader";

                PX4_Uploader uploader(port);
                // Relay status to top-level UI
                connect(&uploader, SIGNAL(upgradeProgressChanged(int)), this, SIGNAL(upgradeProgressChanged(int)));
                connect(&uploader, SIGNAL(upgradeStatusChanged(QString)), this, SIGNAL(upgradeStatusChanged(QString)));

                // Die-hard flash the binary

                if ((info.physName.contains("PX4") || info.vendorID == 9900 /* 3DR */)) {
                    emit upgradeStatusChanged(tr("Found PX4 board on port %1").arg(info.portName));
                } else {
                    emit upgradeStatusChanged(tr("No PX4 board found on port %1 (manual override)").arg(info.portName));
                }

                ret = uploader.upload(filename, _filterBoardId);

                if (ret == -200) {
                    // Image is corrupt
                    emit upgradeStatusChanged(tr("Unable to load binary, aborting. Missing Internet connection?"));
                    emit loadFinished(false);
                    emit finished();
                    port->close();
                    return;
                }

                qDebug() << "RET: " << ret << "ABORT" << _abortOnFirstError;

                if (ret == 0 || ret == -1 || ret == -2) {
                    boardFound = true;
                }

                if (ret == -1 || ret == -2) {
                    wrongBoard = true;
                }

                // bail out on success
                if (ret == 0) {
                    emit loadFinished(true);
                    emit finished();
                    port->close();
                    return;
                }
            } else if ((filename.endsWith(".ihx") || filename.endsWith(".ihex") || filename.endsWith(".hex")) && (_fixedPortName == info.portName || (_fixedPortName.isEmpty() &&
                                  (info.physName.contains("3DR Radio") || info.vendorID == 9900 /* 3DR */)))) {

            }
        }

        if (boardFound && wrongBoard && _abortOnFirstError) {
            // Error, abort
            emit upgradeStatusChanged(tr("Wrong board ID, aborting."));
            emit loadFinished(false);
            emit finished();
            port->close();
            return;
        }

    }

    _abortUpload = false;
    emit loadFinished(false);
    this->deleteLater();
#endif
}

void QGCFirmwareUpgradeWorker::abortUpload()
{
    qDebug() << "Worker asked to abort upload";
    _abortUpload = true;
}

void QGCFirmwareUpgradeWorker::abort()
{
    _abortUpload = true;
    exitThread = true;
}
