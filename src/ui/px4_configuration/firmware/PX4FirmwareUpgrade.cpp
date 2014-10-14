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
#include "qgcfirmwareupgradeworker.h"
#include "boardwidget.h"
#include "dialog_bare.h"
#include "ui_dialog_bare.h"

/// @brief Group to store settings under.
const char* PX4FirmwareUpgrade::_settingsGroup = "PX4FirmwareUpgrade";


/// @Brief Constructs a new PX4FirmwareUpgrade Widget. This widget is used within the PX4VehicleConfig set of screens.
PX4FirmwareUpgrade::PX4FirmwareUpgrade(QWidget *parent) :
    QWidget(parent),
    _loading(false),
    _worker(NULL),
    _boardFoundWidget(NULL)
{
    _ui.setupUi(this);

    // Load the port list for the advanced combo box
    foreach (QSerialPortInfo info, QSerialPortInfo::availablePorts()) {
        if (!info.portName().isEmpty()) {
            _ui.advancedPortBox->addItem(info.portName());
        }
    }
    
#if 0
    // FIXME: Why?
    //make sure user can input their own port name!
    ui->portBox->setEditable(true);
#endif

#if 0
    // FIXME
    enumerator = new QextSerialEnumerator(this);
    enumerator->setUpNotifications();
    // FIXME: Won't pick up on new ports being added
    connect(enumerator, SIGNAL(deviceDiscovered(QextPortInfo)), SLOT(onPortAddedOrRemoved()));
    connect(enumerator, SIGNAL(deviceRemoved(QextPortInfo)), SLOT(onPortAddedOrRemoved()));
#endif

    struct BoardInfo {
        const char* name;
        int         id;
    }
    static const struct BoardInfo rgBoardInfo[] = {
        { "PX4FMU v1.6+",   5 },
        { "PX4FLOW v1.1+",  6 },
        { "PX4IO v1.3+",    7 },
        { "PX4 board #8",   8 },
        { "PX4 board #9",   9 },
        { "PX4 board #10",  10 },
        { "PX4 board #11",  11 },
    };
    
    for (size_t i=0; i<sizeof(rgBoardInfo)/sizeof(rgBoardInfo[0]); i++) {
        _ui.advancedBoardCombo->addItem(rgBoardInfo[i].name, rgBoardInfo[i].id);
    }

    // Connect up advanced ui elements
    connect(_ui.advancedCheckBox, &QCheckBox::clicked, this, &PX4FirmwareUpgrade::_showAdvancedMode);
    connect(_ui.selectFileButton, &QPushButton::clicked, this, &PX4FirmwareUpgrade::_selectAdvancedFirmwareFilename);
    connect(_ui.advancedFlashButton, &QPushButton::clicked, this, &PX4FirmwareUpgrade::_advancedFlash);
    
    // Connect standard ui elements
    connect(_ui.scanButton, &QPushButton::clicked, this, &PX4FirmwareUpgrade::_scanForBoard);
    connect(_ui.cancelButton, &QPushButton::clicked, this, &PX4FirmwareUpgrade::onCancelButtonClicked);

    // FIXME: Does this title show up anywhere?
    setWindowTitle(tr("QUpgrade Firmware Upload / Configuration Tool"));

    // FIXME: Huh?
    // Adjust the size
    const int screenHeight = qMin(1000, QApplication::desktop()->height() - 100);

    resize(700, qMax(screenHeight, 550));

    _loadSettings();
    
    // FIXME: No UI to show filename?

    // Set up initial state
    ui->flashButton->setEnabled(!_advancedFirmwareFilename.isEmpty());
}

PX4FirmwareUpgrade::~PX4FirmwareUpgrade()
{
    _storeSettings();
}

/// @brief Called to show or hide the Advanced mode of the UI.
void PX4FirmwareUpgrade::_showAdvancedMode(bool show)
{
    // Advanced mode UI
    _ui.advancedSelectFileButton->setVisible(show);
    _ui.advancedPortCombo->setVisible(show);
    _ui.advancedPortLabel->setVisible(show);
    _ui.advancedBoardIdLabel->setVisible(show);
    _ui.advancedBoardCombo->setVisible(show);
    _ui.flashButton->setVisible(show);
    
    // Standard UI
    ui.scanButton->setVisible(!show);

    QString msg;
    if (show) {
        msg = tr("Advanced Mode. Please select a file to upload and click flash.");
    } else {
        msg = tr("Please scan to upgrade PX4 boards.");
    }
    _ui.boardListLabel->setText(msg);
}

void PX4FirmwareUpgrade::_loadSettings(void)
{
    QSettings setttings;
    
    settings.beginGroup(_settingsGroup);
    
    lastFilename = settings.value("LAST_FILENAME", lastFilename).toString();
    ui->advancedCheckBox->setChecked(set.value("ADVANCED_MODE", false).toBool());

    int boardIndex = ui->boardComboBox->findData(set.value("BOARD_ID", 5));
    if (boardIndex >= 0)
        ui->boardComboBox->setCurrentIndex(boardIndex);

    if (set.value("PORT_NAME", "").toString().trimmed().length() > 0) {
        int portIndex = ui->portBox->findText(set.value("PORT_NAME", "").toString());
        if (portIndex >= 0) {
            ui->portBox->setCurrentIndex(portIndex);
        } else {
            qDebug() << "could not find port" << set.value("PORT_NAME", "");
        }
    }

    onToggleAdvancedMode(ui->advancedCheckBox->isChecked());

    // Check if in advanced mode
    if (!lastFilename.isEmpty() && ui->advancedCheckBox->isChecked()) {
        ui->upgradeLog->appendPlainText(tr("Pre-selected file %1\nfor flashing (click 'Flash' to upgrade)").arg(lastFilename));

        updateBoardId(lastFilename);
    }
    
    settings.endGroup(_settingsGroup);
}

void PX4FirmwareUpgrade::_storeSettings(void)
{
    QSettings setttings;
    
    settings.beginGroup(_settingsGroup);
    
    if (lastFilename != "")
        set.setValue("LAST_FILENAME", lastFilename);
    set.setValue("ADVANCED_MODE", ui->advancedCheckBox->isChecked());
    set.setValue("BOARD_ID", ui->boardComboBox->itemData(ui->boardComboBox->currentIndex()));
    set.setValue("PORT_NAME", ui->portBox->currentText());
    
    settings.endGroup(_settingsGroup);
}

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

void PX4FirmwareUpgrade::onPortAddedOrRemoved()
{
#if 0
    // FIXME: Will need to do this on a timer
    ui->portBox->blockSignals(true);

    // Delete old ports
    for (int i = 0; i < ui->portBox->count(); i++)
    {
        bool found = false;
        foreach (QextPortInfo info, QextSerialEnumerator::getPorts())
            if (info.portName == ui->portBox->itemText(i))
                found = true;

        if (!found && !ui->portBox->itemText(i).contains("Automatic"))
            ui->portBox->removeItem(i);
    }

    // Add new ports
    foreach (QextPortInfo info, QextSerialEnumerator::getPorts())
        if (ui->portBox->findText(info.portName) < 0) {
            if (!info.portName.isEmpty())
                ui->portBox->addItem(info.portName);
        }

    ui->portBox->blockSignals(false);
#endif
}

void PX4FirmwareUpgrade::onLoadStart()
{
    loading = true;
    ui->flashButton->setEnabled(false);
    ui->cancelButton->setEnabled(true);
}

void PX4FirmwareUpgrade::onUserAbort()
{
    loading = false;
    ui->flashButton->setEnabled(true);
    ui->cancelButton->setEnabled(false);

    ui->upgradeLog->appendPlainText(tr("Canceled by user request"));
    ui->boardListLabel->show();
    ui->boardListLabel->setText(tr("Upgrade canceled. To start another upgrade attempt, click scan."));
    ui->scanButton->show();
    if (boardFoundWidget) {
        ui->boardListLayout->removeWidget(boardFoundWidget);
        delete boardFoundWidget;
        boardFoundWidget = NULL;
    }

    ui->upgradeProgressBar->setValue(0);

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

void PX4FirmwareUpgrade::onDetectFinished(bool success, int board_id, const QString &boardName, const QString &bootLoader)
{   
    loading = false;
    worker = NULL;

    if (success) {
        ui->flashButton->setEnabled(true);
        ui->cancelButton->setEnabled(false);
        ui->upgradeLog->appendPlainText(tr("Board found with ID #%1.").arg(board_id));

        switch (board_id) {
        case 5:
        case 6:
        case 9:
        {
            if (boardFoundWidget) {
                ui->boardListLayout->removeWidget(boardFoundWidget);
                delete boardFoundWidget;
                boardFoundWidget = NULL;
            }
            // Instantiate the appropriate board widget
            BoardWidget* w = new BoardWidget(this);
            w->setBoardInfo(board_id, boardName, bootLoader);
            connect(w, SIGNAL(flashFirmwareURL(QString)), this, SLOT(onFlashURL(QString)));
            connect(w, SIGNAL(cancelFirmwareUpload()), this, SLOT(onUserAbort()));

            boardFoundWidget = w;

            w->updateStatus(tr("Ready for Firmware upload. Choose a firmware and flash."));

            ui->boardListLabel->hide();
            ui->scanButton->hide();
            ui->boardListLayout->addWidget(w, 0, 0);

        }
            break;
        }

    } else {
        ui->upgradeLog->appendPlainText(tr("No suitable device to upgrade."));
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
