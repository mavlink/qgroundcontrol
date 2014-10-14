/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009, 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/


/// @file
///     @brief PX4 Firmware Upgrade UI
///     @author Don Gagne <don@thegagnes.com>

#include <QSerialPort>
#include <QSerialPortInfo>
#include <QtCore>
#include <QDebug>
#include <QFileDialog>
#include <QThread>
#include <QNetworkRequest>
#include <QNetworkReply>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QStandardPaths>
#else
#include <QDesktopServices>
#endif
#include <QFileInfo>
#include <QApplication>
#include <QDesktopWidget>
#include <QTimer>
#include <QWebHistory>
#include <QWebSettings>
#include <QMessageBox>
#include <QSettings>

#include <QGC.h>

#include "PX4FirmwareUpgrade.h"

/// @Brief Constructs a new PX4FirmwareUpgrade Widget. This widget is used within the PX4VehicleConfig set of screens.
PX4FirmwareUpgrade::PX4FirmwareUpgrade(QWidget *parent) :
    QWidget(parent),
    _upgradeState(upgradeStateBegin)
{
    _ui.setupUi(this);

#if 0
    // FIXME
    struct BoardInfo {
        const char* name;
        int         id;
    };
    static const BoardInfo rgBoardInfo[] = {
        { "PX4FMU v1.6+",   5 },
        { "PX4FLOW v1.1+",  6 },
        { "PX4IO v1.3+",    7 },
        { "PX4 board #8",   8 },
        { "PX4 board #9",   9 },
        { "PX4 board #10",  10 },
        { "PX4 board #11",  11 },
    };
#endif
    
    // Connect standard ui elements
    connect(_ui.next, &QPushButton::clicked, this, &PX4FirmwareUpgrade::_nextStep);
    connect(_ui.cancel, &QPushButton::clicked, this, &PX4FirmwareUpgrade::_cancelUpgrade);
    
    _updateStateUI(_upgradeState);
}

/// @brief Updates the progress UI elements according to the new upgrade process state.
void PX4FirmwareUpgrade::_updateStateUI(enum PX4FirmwareUpgrade::upgradeStates newUpgradeState)
{
    switch (newUpgradeState) {
        case upgradeStateBegin:
            _ui.boardFoundCheck->setCheckState(Qt::Unchecked);
            _ui.port->setVisible(false);
            _ui.description->setVisible(false);
            
            _ui.bootloaderFoundCheck->setCheckState(Qt::Unchecked);
            _ui.bootloaderVersion->setVisible(false);
            _ui.boardID->setVisible(false);
            _ui.icon->setVisible(false);
            
            _ui.selectFirmwareCheck->setCheckState(Qt::Unchecked);
            _ui.firmwareCombo->setVisible(false);
            
            _ui.firmwareDownloadedCheck->setCheckState(Qt::Unchecked);
            _ui.boardUpgradedCheck->setCheckState(Qt::Unchecked);

            _ui.tryAgain->setEnabled(false);
            _ui.skip->setEnabled(false);
            _ui.cancel->setEnabled(false);
            _ui.next->setEnabled(true);
            
            _ui.next->setText(tr("Scan"));
            
            _ui.statusLog->setText(tr("Connect your Pixhawk or PX4Flow board via USB. "
                                   "Then click the 'Scan' button to find the board and begin the firmware upgrade process."));
            break;
            
        case upgradeStateBoardNotFound:
            _ui.tryAgain->setEnabled(true);
            _ui.skip->setEnabled(false);
            _ui.cancel->setEnabled(true);
            _ui.next->setEnabled(false);
            
            _ui.statusLog->setText(tr("Unable to detect you board. If the board is currently connected via USB. "
                                   "Disconnect it, reconnect it, wait a few seconds and click the 'Try Again' button."));
            break;
            
        case upgradeStateBoardFound:
            _ui.boardFoundCheck->setCheckState(Qt::Checked);
            
            _ui.port->setText(tr("Port: %1").arg(_port));
            _ui.port->setVisible(true);
            
            _ui.description->setText(tr("Name: %1").arg(_portDescription));
            _ui.description->setVisible(true);
            
            _ui.next->setText(tr("Next"));
            
            _ui.tryAgain->setEnabled(true);
            _ui.skip->setEnabled(false);
            _ui.cancel->setEnabled(true);
            _ui.next->setEnabled(true);
            break;
            
        case upgradeStateBootloaderFound:
            _ui.bootloaderFoundCheck->setCheckState(Qt::Checked);
            
            _ui.bootloaderVersion->setText(QString("Version %1").arg(_bootloaderVersion));
            _ui.bootloaderVersion->setVisible(true);
            
            _ui.boardID->setText(QString("Board ID %1").arg(_boardID));
            _ui.boardID->setVisible(true);
            
            _setBoardIcon(_boardID);
            _ui.icon->setVisible(false);
            
            _setFirmwareCombo(_boardID);
            break;
            
        case upgradeStateFirmwareSelected:
            break;
            
        case upgradeStateFirmwareDownloaded:
            break;
            
        case upgradeStateBoardUpgraded:
            break;
            
    }
}

/// @brief Sets the board image into the icon label according to the board id.
void PX4FirmwareUpgrade::_setBoardIcon(int boardID)
{
    QString imageFile;
    
    // FIXME: Magic numbers
    switch (boardID) {
        case 5:
            imageFile = ":/files/images/px4/boards/px4fmu_1.x.png";
            break;
            
        case 6:
            imageFile = ":/files/images/px4/boards/px4flow_1.x.png";
            break;
            
        case 9:
            imageFile = ":/files/images/px4/boards/px4fmu_2.x.png";
            break;
            
        default:
            qDebug() << "Invalid board ID";
            Q_ASSERT(false);
            break;
    }
    
    _boardIcon.load(imageFile);
    
    int w = _ui.icon->width();
    int h = _ui.icon->height();
    
    _ui.icon->setPixmap(_boardIcon.scaled(w, h, Qt::KeepAspectRatio));
}

/// @brief Sets up the selections in the firmware combox box associated with the specified
///     board id.
void PX4FirmwareUpgrade::_setFirmwareCombo(int boardID)
{
    _ui.firmwareCombo->clear();
    
    static const char* rgPX4FMUV1Firmware[3] =
    {
        "http://px4.oznet.ch/stable/px4fmu-v1_default.px4",
        "http://px4.oznet.ch/beta/px4fmu-v1_default.px4",
        "http://px4.oznet.ch/continuous/px4fmu-v1_default.px4"
    };
    
    static const char* rgPX4FMUV2Firmware[3] =
    {
        "http://px4.oznet.ch/stable/px4fmu-v2_default.px4",
        "http://px4.oznet.ch/beta/px4fmu-v2_default.px4",
        "http://px4.oznet.ch/continuous/px4fmu-v2_default.px4"
    };
    
    static const char* rgPX4FlowFirmware[3] =
    {
        "http://px4.oznet.ch/stable/px4flow.px4",
        "http://px4.oznet.ch/beta/px4flow.px4",
        "http://px4.oznet.ch/continuous/px4flow.px4"
    };
    
    // FIXME: Magic numbers
    const char** prgFirmware;
    switch (boardID) {
        case 5:
            prgFirmware = rgPX4FMUV1Firmware;
            break;

        case 6:
            prgFirmware = rgPX4FlowFirmware;
            break;

        case 9:
            prgFirmware = rgPX4FMUV2Firmware;
            break;
            
        default:
            qDebug() << "Invalid board ID";
            Q_ASSERT(false);
            break;
    }
    
    _ui.firmwareCombo->addItem(tr("Standard Version (stable)"), prgFirmware[0]);
    _ui.firmwareCombo->addItem(tr("Beta Testing (beta)"), prgFirmware[1]);
    _ui.firmwareCombo->addItem(tr("Developer Build (master)"), prgFirmware[2]);
}

void PX4FirmwareUpgrade::_nextStep(void)
{
    _updateStateUI(_findBoard() ? upgradeStateBoardFound : upgradeStateBoardNotFound);
}

void PX4FirmwareUpgrade::_cancelUpgrade(void)
{
    _updateStateUI(upgradeStateBegin);
}

bool PX4FirmwareUpgrade::_findBoard(void)
{
    foreach (QSerialPortInfo info, QSerialPortInfo::availablePorts()) {
        
        // Check for valid handles
        if (info.portName().isEmpty())
            continue;

        // FIXME: Won't this OR find a 3dr radio as well?
        // FIXME: Magic number
        if (info.description().contains("PX4") || info.vendorIdentifier() == 9900 /* 3DR */) {
            
            qDebug() << "Found Board:";
            qDebug() << "\tport name:" << info.portName();
            qDebug() << "\tdescription:" << info.description();
            qDebug() << "\tsystem location:" << info.systemLocation();
            qDebug() << "\tvendor ID:" << info.vendorIdentifier();
            qDebug() << "\tproduct ID:" << info.productIdentifier();
            
            _port = info.portName();
            _portDescription = info.description();
            
#ifdef Q_OS_WIN
            // Stupid windows fixes
            _port.prepend("\\\\.\\");
#endif
            
            return true;
        }
    }
    
    return false;
}

#if 0
void PX4FirmwareUpgrade::updateBoardId(const QString &fileName) {
    // XXX this should be moved in separe classes

    // Attempt to decode JSON
    QFile json(lastFilename);
    json.open(QIODevice::ReadOnly | QIODevice::Text);
    QByteArray jbytes = json.readAll();

    if (fileName.endsWith(".px4")) {

        int checkBoardId;

        QJsonDocument doc = QJsonDocument::fromJson(jbytes);

        if (doc.isNull()) {
            // Error, bail out
            ui->upgradeLog->appendPlainText(tr("supplied file is not a valid JSON document"));
            return;
        }

        QJsonObject px4 = doc.object();

        checkBoardId = (int) (px4.value(QString("board_id")).toDouble());
        ui->upgradeLog->appendPlainText(tr("loaded file for board ID %1").arg(checkBoardId));

        // Set correct board ID
        int index = ui->boardComboBox->findData(checkBoardId);

        if (index >= 0) {
            ui->boardComboBox->setCurrentIndex(index);
        } else {
            qDebug() << "Combo box: board not found:" << index;
        }
    }
}

void PX4FirmwareUpgrade::onLinkClicked(const QUrl &url)
{
    QString firmwareFile = QFileInfo(url.toString()).fileName();

    QString filename;

    // If a IO firmware file, open save as PX4FirmwareUpgrade
    if (firmwareFile.endsWith(".bin") && firmwareFile.contains("px4io")) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        QString path = QString(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
#else
        QString path = QString(QDesktopServices::storageLocation(QDesktopServices::DesktopLocation));
#endif
        qDebug() << path;
        filename = QFileDialog::getExistingDirectory(this, tr("Select folder (microSD Card)"),
                                                path);
        filename.append("/" + firmwareFile);
    } else {

        // Make sure the user doesn't screw up flashing
        //QMessageBox msgBox;
        //msgBox.setText("Please unplug your PX4 board now");
        //msgBox.exec();

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        filename = QString(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
#else
        filename = QString(QDesktopServices::storageLocation(QDesktopServices::DesktopLocation));
#endif

        if (filename.isEmpty()) {
            qDebug() << "Falling back to temp dir";
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        QString filename = QString(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
#else
        QString filename = QString(QDesktopServices::storageLocation(QDesktopServices::TempLocation));
#endif
            // If still empty, bail out
            if (filename.isEmpty())
                ui->upgradeLog->appendHtml(tr("FAILED storing firmware to downloads or temp directory. Harddisk not writable."));
                return;
        }

        filename += "/" + firmwareFile;
    }

    if (filename == "") {
        return;
    }

    // Else, flash the firmware
    lastFilename = filename;

    ui->upgradeLog->appendHtml(tr("Downloading firmware file <a href=\"%1\">%1</a>").arg(url.toString()));

    QNetworkRequest newRequest(url);
    newRequest.setUrl(url);
    newRequest.setAttribute(QNetworkRequest::User, filename);

    // XXX rework
    QNetworkAccessManager* networkManager = new QNetworkAccessManager(this); //ui->webView->page()->networkAccessManager();
    QNetworkReply *reply = networkManager->get(newRequest);
    connect(reply, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(onDownloadProgress(qint64, qint64)));
    connect(reply, SIGNAL(finished()), this, SLOT(onDownloadFinished()));
}

void PX4FirmwareUpgrade::onDownloadRequested(const QNetworkRequest &request)
{
    onLinkClicked(request.url());
}

void PX4FirmwareUpgrade::onDownloadFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(QObject::sender());

    if (!reply) {
        // bail out, nothing to do
        ui->upgradeLog->appendPlainText("Download failed, invalid context");
        return;
    }

    if (loading) {
        onCancelButtonClicked();
        ui->upgradeLog->appendPlainText(tr("Still flashing. Waiting for firmware flashing to complete.."));
    } else {

        // Reset progress
        ui->upgradeProgressBar->setValue(0);

        // Pick file
        QString fileName = lastFilename;

        qDebug() << "Handling filename:" << fileName;

        // Store file in download location
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly)) {
            ui->upgradeLog->appendPlainText(tr("Could not open %s for writing: %s\n").arg(fileName).arg(file.errorString()));
            return;
        }

        file.write(reply->readAll());
        file.close();

        if (lastFilename.contains("px4io")) {
            // Done, bail out
            return;
        }

        if (fileName.length() > 0) {
            // Got a filename, upload
            onLoadStart();
            lastFilename = fileName;

            // Try to reserve links for us
            emit disconnectLinks();

            worker = QGCFirmwareUpgradeWorker::putWorkerInThread(fileName, "", 0, true);
            // Hook up status from worker to progress bar
            connect(worker, SIGNAL(upgradeProgressChanged(int)), ui->upgradeProgressBar, SLOT(setValue(int)));
            // Hook up text from worker to label
            connect(worker, SIGNAL(upgradeStatusChanged(QString)), ui->upgradeLog, SLOT(appendPlainText(QString)));
            // Hook up error handling
            //connect(worker, SIGNAL(error(QString)), ui->upgradeLog, SLOT(appendPlainText(QString)));
            // Hook up status from worker to this class
            connect(worker, SIGNAL(loadFinished(bool)), this, SLOT(onLoadFinished(bool)));

            if (boardFoundWidget)
                connect(worker, SIGNAL(upgradeStatusChanged(QString)), boardFoundWidget, SLOT(updateStatus(QString)));

//            // Make sure user gets the board going now
//            QMessageBox msgBox;
//            msgBox.setText("Please unplug your PX4 board now and plug it back in");
//            msgBox.exec();
        }
    }
}

/// @brief Displays a file picker dialog to allow the user to select the firmware file to flash.
void PX4FirmwareUpgrade::_selectAdvancedFirmwareFilename(void)
{
    // FIXME: If loading we should disable the advanced stuff
    if (loading) {
        if (worker)
            worker->abortUpload();
        loading = false;
    }

    // Pick file
    QString fileName = QFileDialog::getOpenFileName(this,                                   // parent
                                                    tr("Open Firmware File"),               // window title
                                                    _advancedFirmwareFilename,              // current filename
                                                    tr("Firmware Files (*.px4 *.bin)"));    // filename filter

    if (!fileName.isEmpty()) {
        _advancedFirmwareFilename = fileName;
        _ui.advancedFlashButton->setEnabled(true);
        updateBoardId(_advancedFirmwareFilename);
    }
}

void PX4FirmwareUpgrade::onCancelButtonClicked()
{
    QASSERT(_worker);
    
    if (!worker) {
        _worker->abortUpload();
    }
    _ui.cancelButton->setEnabled(false);
    _ui.upgradeLog->appendPlainText(tr("Waiting for last operation to abort.."));
}

/// @brief This will flash the firmware as specified by the advanced settings to the board.
void PX4FirmwareUpgrade::_advancedFlash(void)
{
    Q_ASSERT(!_loading);
    Q_ASSERT(!_advancedFirmwareFilename.isEmpty());
        
    // Got a filename, upload
    loading = true;
    ui->flashButton->setEnabled(false);
    ui->cancelButton->setEnabled(true);

    int id = -1;

    // Try to reserve links for us
    emit disconnectLinks();

    // Set board ID for worker
    if (ui->boardComboBox->isVisible()) {
        bool ok;
        int tmp = ui->boardComboBox->itemData(ui->boardComboBox->currentIndex()).toInt(&ok);
        if (ok)
            id = tmp;
    }

    worker = QGCFirmwareUpgradeWorker::putWorkerInThread(lastFilename, ui->portBox->currentText(), id, true);

    connect(ui->portBox, SIGNAL(editTextChanged(QString)), worker, SLOT(setPort(QString)));

    // Hook up status from worker to progress bar
    connect(worker, SIGNAL(upgradeProgressChanged(int)), ui->upgradeProgressBar, SLOT(setValue(int)));
    // Hook up text from worker to label
    connect(worker, SIGNAL(upgradeStatusChanged(QString)), ui->upgradeLog, SLOT(appendPlainText(QString)));
    // Hook up status from worker to this class
    connect(worker, SIGNAL(loadFinished(bool)), this, SLOT(onLoadFinished(bool)));

    if (boardFoundWidget)
        connect(worker, SIGNAL(upgradeStatusChanged(QString)), boardFoundWidget, SLOT(updateStatus(QString)));
}

/// @brief Begins the process of automatically scanning serial ports for a connect PX4 board.
void PX4FirmwareUpgrade::_scanForBoard(void)
{
    if (loading) {
        // FIXME: Huh?
        onCancelButtonClicked();
    } else {

            ui->flashButton->setEnabled(false);
            ui->cancelButton->setEnabled(true);

            // Try to reserve links for us
            emit disconnectLinks();

            worker = QGCFirmwareUpgradeWorker::putDetectorInThread();

            connect(ui->portBox, SIGNAL(editTextChanged(QString)), worker, SLOT(setPort(QString)));

            // Hook up status from worker to progress bar
            connect(worker, SIGNAL(upgradeProgressChanged(int)), ui->upgradeProgressBar, SLOT(setValue(int)));
            // Hook up text from worker to label
            connect(worker, SIGNAL(upgradeStatusChanged(QString)), ui->upgradeLog, SLOT(appendPlainText(QString)));
            // Hook up status from worker to this class
            connect(worker, SIGNAL(detectFinished(bool, int, QString, QString)), this, SLOT(onDetectFinished(bool, int, QString, QString)));
    }
}

void PX4FirmwareUpgrade::onLoadFinished(bool success)
{
    loading = false;
    worker = NULL;

    ui->flashButton->setEnabled(true);
    ui->cancelButton->setEnabled(false);

    if (success) {
        ui->upgradeLog->appendPlainText(tr("Upload succeeded."));
        ui->boardListLabel->show();
        if (ui->advancedCheckBox->checkState() == Qt::Checked) {
            ui->boardListLabel->setText(tr("Upgrade succeeded. The re-flash, just click flash again."));
        } else {
            ui->boardListLabel->setText(tr("Upgrade succeeded. To flash another board firmware, click scan."));
            ui->scanButton->show();
        }
        if (boardFoundWidget) {
            ui->boardListLayout->removeWidget(boardFoundWidget);
            delete boardFoundWidget;
            boardFoundWidget = NULL;
        }

        // Reconnect links after upload
        QTimer::singleShot(3000, this, SIGNAL(connectLinks()));

    } else {
        ui->upgradeLog->appendPlainText(tr("Upload aborted and failed."));
        ui->boardListLabel->setText(tr("Upload aborted and failed."));
        ui->upgradeProgressBar->setValue(0);
    }

}

void PX4FirmwareUpgrade::onFlashURL(const QString &url)
{
    onLinkClicked(QUrl(url));
}

void PX4FirmwareUpgrade::onDownloadProgress(qint64 curr, qint64 total)
{
    // Take care of cases where 0 / 0 is emitted as error return value
    if (total > 0)
        ui->upgradeProgressBar->setValue((curr*100) / total);
}
#endif