#include <QTimer>

#include "LinkManager.h"
#include "LinkInterface.h"
#include <QSerialPort>
#include <QSerialPortInfo>
#include "SerialLink.h"

#include "ApmFirmwareConfig.h"

ApmFirmwareConfig::ApmFirmwareConfig(QWidget *parent) : QWidget(parent)
{
    ui.setupUi(this);
    //firmwareStatus = 0;
    m_betaFirmwareChecked = false;
    m_tempFirmwareFile=0;
    //

    //QNetworkRequest req(QUrl("https://raw.github.com/diydrones/binary/master/Firmware/firmware2.xml"));



    m_networkManager = new QNetworkAccessManager(this);

    connect(ui.roverPushButton,SIGNAL(clicked()),this,SLOT(flashButtonClicked()));
    connect(ui.planePushButton,SIGNAL(clicked()),this,SLOT(flashButtonClicked()));
    connect(ui.copterPushButton,SIGNAL(clicked()),this,SLOT(flashButtonClicked()));
    connect(ui.hexaPushButton,SIGNAL(clicked()),this,SLOT(flashButtonClicked()));
    connect(ui.octaQuadPushButton,SIGNAL(clicked()),this,SLOT(flashButtonClicked()));
    connect(ui.octaPushButton,SIGNAL(clicked()),this,SLOT(flashButtonClicked()));
    connect(ui.quadPushButton,SIGNAL(clicked()),this,SLOT(flashButtonClicked()));
    connect(ui.triPushButton,SIGNAL(clicked()),this,SLOT(flashButtonClicked()));
    connect(ui.y6PushButton,SIGNAL(clicked()),this,SLOT(flashButtonClicked()));
    QTimer::singleShot(10000,this,SLOT(requestFirmwares()));
    connect(ui.betaFirmwareButton,SIGNAL(clicked(bool)),this,SLOT(betaFirmwareButtonClicked(bool)));

    ui.progressBar->setMaximum(100);
    ui.progressBar->setValue(0);

    ui.textBrowser->setVisible(false);
    connect(ui.showOutputCheckBox,SIGNAL(clicked(bool)),ui.textBrowser,SLOT(setVisible(bool)));

    /*addBetaLabel(ui.roverPushButton);
    addBetaLabel(ui.planePushButton);
    addBetaLabel(ui.copterPushButton);
    addBetaLabel(ui.quadPushButton);
    addBetaLabel(ui.hexaPushButton);
    addBetaLabel(ui.octaQuadPushButton);
    addBetaLabel(ui.octaPushButton);
    addBetaLabel(ui.triPushButton);
    addBetaLabel(ui.y6PushButton);*/

}
void ApmFirmwareConfig::hideBetaLabels()
{
    for (int i=0;i<m_betaButtonLabelList.size();i++)
    {
        m_betaButtonLabelList[i]->hide();
    }
    ui.warningLabel->hide();
}

void ApmFirmwareConfig::showBetaLabels()
{
    for (int i=0;i<m_betaButtonLabelList.size();i++)
    {
        m_betaButtonLabelList[i]->show();
    }
    ui.warningLabel->show();
}

void ApmFirmwareConfig::addBetaLabel(QWidget *parent)
{
    QLabel *label = new QLabel(parent);
    QVBoxLayout *layout = new QVBoxLayout();
    parent->setLayout(layout);
    label->setAlignment(Qt::AlignRight | Qt::AlignBottom);
    label->setText("<h1><font color=#FFAA00>BETA</font></h1>");
    layout->addWidget(label);
    m_betaButtonLabelList.append(label);
}

void ApmFirmwareConfig::requestBetaFirmwares()
{
    m_betaFirmwareChecked = true;
    showBetaLabels();
    QNetworkReply *reply1 = m_networkManager->get(QNetworkRequest(QUrl("http://firmware.diydrones.com/Copter/beta/apm2-heli/git-version.txt")));
    QNetworkReply *reply2 = m_networkManager->get(QNetworkRequest(QUrl("http://firmware.diydrones.com/Copter/beta/apm2-quad/git-version.txt")));
    QNetworkReply *reply3 = m_networkManager->get(QNetworkRequest(QUrl("http://firmware.diydrones.com/Copter/beta/apm2-hexa/git-version.txt")));
    QNetworkReply *reply4 = m_networkManager->get(QNetworkRequest(QUrl("http://firmware.diydrones.com/Copter/beta/apm2-octa/git-version.txt")));
    QNetworkReply *reply5 = m_networkManager->get(QNetworkRequest(QUrl("http://firmware.diydrones.com/Copter/beta/apm2-octa-quad/git-version.txt")));
    QNetworkReply *reply6 = m_networkManager->get(QNetworkRequest(QUrl("http://firmware.diydrones.com/Copter/beta/apm2-tri/git-version.txt")));
    QNetworkReply *reply7 = m_networkManager->get(QNetworkRequest(QUrl("http://firmware.diydrones.com/Copter/beta/apm2-y6/git-version.txt")));
    QNetworkReply *reply8 = m_networkManager->get(QNetworkRequest(QUrl("http://firmware.diydrones.com/Plane/beta/apm2/git-version.txt")));
    QNetworkReply *reply9 = m_networkManager->get(QNetworkRequest(QUrl("http://firmware.diydrones.com/Rover/beta/apm2/git-version.txt")));

    m_buttonToUrlMap[ui.roverPushButton] = "http://firmware.diydrones.com/Rover/beta/apm2/APMrover2.hex";
    m_buttonToUrlMap[ui.planePushButton] = "http://firmware.diydrones.com/Plane/beta/apm2/ArduPlane.hex";
    m_buttonToUrlMap[ui.copterPushButton] = "http://firmware.diydrones.com/Copter/beta/apm2-heli/ArduCopter.hex";
    m_buttonToUrlMap[ui.hexaPushButton] = "http://firmware.diydrones.com/Copter/beta/apm2-hexa/ArduCopter.hex";
    m_buttonToUrlMap[ui.octaQuadPushButton] = "http://firmware.diydrones.com/Copter/beta/apm2-octa-quad/ArduCopter.hex";
    m_buttonToUrlMap[ui.octaPushButton] = "http://firmware.diydrones.com/Copter/beta/apm2-octa/ArduCopter.hex";
    m_buttonToUrlMap[ui.quadPushButton] = "http://firmware.diydrones.com/Copter/beta/apm2-quad/ArduCopter.hex";
    m_buttonToUrlMap[ui.triPushButton] = "http://firmware.diydrones.com/Copter/beta/apm2-tri/ArduCopter.hex";
    m_buttonToUrlMap[ui.y6PushButton] = "http://firmware.diydrones.com/Copter/beta/apm2-y6/ArduCopter.hex";

    //http://firmware.diydrones.com/Plane/stable/apm2/ArduPlane.hex
    connect(reply1,SIGNAL(finished()),this,SLOT(firmwareListFinished()));
    connect(reply1,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(firmwareListError(QNetworkReply::NetworkError)));
    connect(reply2,SIGNAL(finished()),this,SLOT(firmwareListFinished()));
    connect(reply2,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(firmwareListError(QNetworkReply::NetworkError)));
    connect(reply3,SIGNAL(finished()),this,SLOT(firmwareListFinished()));
    connect(reply3,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(firmwareListError(QNetworkReply::NetworkError)));
    connect(reply4,SIGNAL(finished()),this,SLOT(firmwareListFinished()));
    connect(reply4,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(firmwareListError(QNetworkReply::NetworkError)));
    connect(reply5,SIGNAL(finished()),this,SLOT(firmwareListFinished()));
    connect(reply5,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(firmwareListError(QNetworkReply::NetworkError)));
    connect(reply6,SIGNAL(finished()),this,SLOT(firmwareListFinished()));
    connect(reply6,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(firmwareListError(QNetworkReply::NetworkError)));
    connect(reply7,SIGNAL(finished()),this,SLOT(firmwareListFinished()));
    connect(reply7,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(firmwareListError(QNetworkReply::NetworkError)));
    connect(reply8,SIGNAL(finished()),this,SLOT(firmwareListFinished()));
    connect(reply8,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(firmwareListError(QNetworkReply::NetworkError)));
    connect(reply9,SIGNAL(finished()),this,SLOT(firmwareListFinished()));
    connect(reply9,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(firmwareListError(QNetworkReply::NetworkError)));
    qDebug() << "Getting Beta firmware...";
}

void ApmFirmwareConfig::requestFirmwares()
{
    m_betaFirmwareChecked = false;
    hideBetaLabels();
    QNetworkReply *reply1 = m_networkManager->get(QNetworkRequest(QUrl("http://firmware.diydrones.com/Copter/stable/apm2-heli/git-version.txt")));
    QNetworkReply *reply2 = m_networkManager->get(QNetworkRequest(QUrl("http://firmware.diydrones.com/Copter/stable/apm2-quad/git-version.txt")));
    QNetworkReply *reply3 = m_networkManager->get(QNetworkRequest(QUrl("http://firmware.diydrones.com/Copter/stable/apm2-hexa/git-version.txt")));
    QNetworkReply *reply4 = m_networkManager->get(QNetworkRequest(QUrl("http://firmware.diydrones.com/Copter/stable/apm2-octa/git-version.txt")));
    QNetworkReply *reply5 = m_networkManager->get(QNetworkRequest(QUrl("http://firmware.diydrones.com/Copter/stable/apm2-octa-quad/git-version.txt")));
    QNetworkReply *reply6 = m_networkManager->get(QNetworkRequest(QUrl("http://firmware.diydrones.com/Copter/stable/apm2-tri/git-version.txt")));
    QNetworkReply *reply7 = m_networkManager->get(QNetworkRequest(QUrl("http://firmware.diydrones.com/Copter/stable/apm2-y6/git-version.txt")));
    QNetworkReply *reply8 = m_networkManager->get(QNetworkRequest(QUrl("http://firmware.diydrones.com/Plane/stable/apm2/git-version.txt")));
    QNetworkReply *reply9 = m_networkManager->get(QNetworkRequest(QUrl("http://firmware.diydrones.com/Rover/stable/apm2/git-version.txt")));

    m_buttonToUrlMap[ui.roverPushButton] = "http://firmware.diydrones.com/Rover/stable/apm2/APMrover2.hex";
    m_buttonToUrlMap[ui.planePushButton] = "http://firmware.diydrones.com/Plane/stable/apm2/ArduPlane.hex";
    m_buttonToUrlMap[ui.copterPushButton] = "http://firmware.diydrones.com/Copter/stable/apm2-heli/ArduCopter.hex";
    m_buttonToUrlMap[ui.hexaPushButton] = "http://firmware.diydrones.com/Copter/stable/apm2-hexa/ArduCopter.hex";
    m_buttonToUrlMap[ui.octaQuadPushButton] = "http://firmware.diydrones.com/Copter/stable/apm2-octa-quad/ArduCopter.hex";
    m_buttonToUrlMap[ui.octaPushButton] = "http://firmware.diydrones.com/Copter/stable/apm2-octa/ArduCopter.hex";
    m_buttonToUrlMap[ui.quadPushButton] = "http://firmware.diydrones.com/Copter/stable/apm2-quad/ArduCopter.hex";
    m_buttonToUrlMap[ui.triPushButton] = "http://firmware.diydrones.com/Copter/stable/apm2-tri/ArduCopter.hex";
    m_buttonToUrlMap[ui.y6PushButton] = "http://firmware.diydrones.com/Copter/stable/apm2-y6/ArduCopter.hex";

    //http://firmware.diydrones.com/Plane/stable/apm2/ArduPlane.hex
    connect(reply1,SIGNAL(finished()),this,SLOT(firmwareListFinished()));
    connect(reply1,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(firmwareListError(QNetworkReply::NetworkError)));
    connect(reply2,SIGNAL(finished()),this,SLOT(firmwareListFinished()));
    connect(reply2,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(firmwareListError(QNetworkReply::NetworkError)));
    connect(reply3,SIGNAL(finished()),this,SLOT(firmwareListFinished()));
    connect(reply3,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(firmwareListError(QNetworkReply::NetworkError)));
    connect(reply4,SIGNAL(finished()),this,SLOT(firmwareListFinished()));
    connect(reply4,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(firmwareListError(QNetworkReply::NetworkError)));
    connect(reply5,SIGNAL(finished()),this,SLOT(firmwareListFinished()));
    connect(reply5,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(firmwareListError(QNetworkReply::NetworkError)));
    connect(reply6,SIGNAL(finished()),this,SLOT(firmwareListFinished()));
    connect(reply6,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(firmwareListError(QNetworkReply::NetworkError)));
    connect(reply7,SIGNAL(finished()),this,SLOT(firmwareListFinished()));
    connect(reply7,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(firmwareListError(QNetworkReply::NetworkError)));
    connect(reply8,SIGNAL(finished()),this,SLOT(firmwareListFinished()));
    connect(reply8,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(firmwareListError(QNetworkReply::NetworkError)));
    connect(reply9,SIGNAL(finished()),this,SLOT(firmwareListFinished()));
    connect(reply9,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(firmwareListError(QNetworkReply::NetworkError)));
    qDebug() << "Getting Stable firmware...";
}

void ApmFirmwareConfig::betaFirmwareButtonClicked(bool betafirmwareenabled)
{
    if (betafirmwareenabled)
    {
        ui.label->setText(tr("<h2>Beta Firmware</h2>"));
        ui.betaFirmwareButton->setText(tr("Stable Firmware"));
        requestBetaFirmwares();
    }
    else
    {
        ui.label->setText(tr("<h2>Firmware</h2>"));
        ui.betaFirmwareButton->setText(tr("Beta Firmware"));
        requestFirmwares();
    }
}
void ApmFirmwareConfig::firmwareProcessFinished(int status)
{
    QProcess *proc = qobject_cast<QProcess*>(sender());
    if (!proc)
    {
        return;
    }
    if (status != 0)
    {
        //Error of some kind
        QMessageBox::information(0,tr("Error"),tr("An error has occured during the upload process. See window for details"));
        ui.textBrowser->setVisible(true);
        ui.showOutputCheckBox->setChecked(true);
        ui.textBrowser->setPlainText(ui.textBrowser->toPlainText().append("\n\nERROR!!\n" + proc->errorString()));
        QScrollBar *sb = ui.textBrowser->verticalScrollBar();
        if (sb)
        {
            sb->setValue(sb->maximum());
        }
        ui.statusLabel->setText(tr("Error during upload"));
    }
    else
    {
        //Ensure we're reading 100%
        ui.progressBar->setValue(100);
        ui.statusLabel->setText(tr("Upload complete"));
    }
    //qDebug() << "Upload finished!" << QString::number(status);
    m_tempFirmwareFile->deleteLater(); //This will remove the temporary file.
    m_tempFirmwareFile = 0;

}
void ApmFirmwareConfig::firmwareProcessReadyRead()
{
    QProcess *proc = qobject_cast<QProcess*>(sender());
    if (!proc)
    {
        return;
    }
    QString output = proc->readAllStandardError() + proc->readAllStandardOutput();
    if (output.contains("Writing"))
    {
        //firmwareStatus->resetProgress();
        ui.progressBar->setValue(0);
    }
    else if (output.contains("Reading"))
    {
        ui.progressBar->setValue(50);
    }
    if (output.startsWith("#"))
    {
        ui.progressBar->setValue(ui.progressBar->value()+1);

        ui.textBrowser->setPlainText(ui.textBrowser->toPlainText().append(output));
        QScrollBar *sb = ui.textBrowser->verticalScrollBar();
        if (sb)
        {
            sb->setValue(sb->maximum());
        }
    }
    else
    {
        ui.textBrowser->setPlainText(ui.textBrowser->toPlainText().append(output + "\n"));
        QScrollBar *sb = ui.textBrowser->verticalScrollBar();
        if (sb)
        {
            sb->setValue(sb->maximum());
        }
    }

    qDebug() << "E:" << output;
    //qDebug() << "AVR Output:" << proc->readAllStandardOutput();
    //qDebug() << "AVR Output:" << proc->readAllStandardError();
}

void ApmFirmwareConfig::downloadFinished()
{
    qDebug() << "Download finished, flashing firmware";
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply)
    {
        return;
    }
    QByteArray hex = reply->readAll();
    m_tempFirmwareFile = new QTemporaryFile();
    m_tempFirmwareFile->open();
    m_tempFirmwareFile->write(hex);
    m_tempFirmwareFile->flush();
    m_tempFirmwareFile->close();
    //tempfirmware.fileName()
    QProcess *process = new QProcess(this);
    connect(process,SIGNAL(finished(int)),this,SLOT(firmwareProcessFinished(int)));
    connect(process,SIGNAL(readyReadStandardOutput()),this,SLOT(firmwareProcessReadyRead()));
    connect(process,SIGNAL(readyReadStandardError()),this,SLOT(firmwareProcessReadyRead()));
    connect(process,SIGNAL(error(QProcess::ProcessError)),this,SLOT(firmwareProcessError(QProcess::ProcessError)));
    QList<QSerialPortInfo> portList =  QSerialPortInfo::availablePorts();


    foreach (const QSerialPortInfo &info, portList)
    {
        qDebug() << "PortName    : " << info.portName()
               << "Description : " << info.description();
        qDebug() << "Manufacturer: " << info.manufacturer();


    }

    //info.manufacturer() == "Arduino LLC (www.arduino.cc)"
    //info.description() == "%mega2560.name%"

    qDebug() << "Attempting to reset port";

    QSerialPort port;

    port.setPortName(m_detectedComPort);
    port.open(QIODevice::ReadWrite);
    port.setDataTerminalReady(true);
    port.waitForBytesWritten(250);
    port.setDataTerminalReady(false);
    port.close();

    QString avrdudeExecutable;
    QStringList stringList;

    ui.statusLabel->setText(tr("Flashing"));
#ifdef Q_OS_WIN
    stringList = QStringList() << "-Cavrdude/avrdude.conf" << "-pm2560"
                               << "-cstk500" << QString("-P").append(m_detectedComPort)
                               << QString("-Uflash:w:").append(m_tempFirmwareFile->fileName()).append(":i");

    avrdudeExecutable = "avrdude/avrdude.exe";
#endif
#ifdef Q_OS_MAC
    stringList = QStringList() << "-v" << "-pm2560"
                                           << "-cstk500" << QString("-P/dev/cu.").append(m_detectedComPort)
                                           << QString("-Uflash:w:").append(m_tempFirmwareFile->fileName()).append(":i");
    avrdudeExecutable = "/usr/local/CrossPack-AVR/bin/avrdude";
#endif

    // Start the Flashing
    qDebug() << avrdudeExecutable << stringList;
    process->start(avrdudeExecutable,stringList);
}
void ApmFirmwareConfig::firmwareProcessError(QProcess::ProcessError error)
{
    qDebug() << "Error:" << error;
}
void ApmFirmwareConfig::firmwareDownloadProgress(qint64 received,qint64 total)
{
    ui.progressBar->setValue( 100.0 * ((double)received/(double)total));
}

void ApmFirmwareConfig::flashButtonClicked()
{
    QPushButton *senderbtn = qobject_cast<QPushButton*>(sender());
    if (m_buttonToUrlMap.contains(senderbtn))
    {
        bool foundconnected = false;
        for (int i=0;i<LinkManager::instance()->getLinks().size();i++)
        {
            if (LinkManager::instance()->getLinks()[i]->isConnected())
            {
                //This is likely the serial link we want.
                SerialLink *link = qobject_cast<SerialLink*>(LinkManager::instance()->getLinks()[i]);
                if (!link)
                {
                    qDebug() << "Eror, trying to program over a non serial link. This should not happen";
                    return;
                }
                if (!(QMessageBox::question(this,tr("WARNING"),tr("You are about to upload new firmware to your board. This will disconnect you if you are currently connected. Be sure the MAV is on the ground, and connected over USB/Serial link.\n\nDo you wish to proceed?"),QMessageBox::Yes,QMessageBox::No) == QMessageBox::Yes))
                {
                    return;
                }

                m_detectedComPort = link->getPortName();
                link->requestReset();
                foundconnected = true;
                link->disconnect();
                link->wait(1000); // Wait 1 second for it to disconnect.
            }
        }
        if (!foundconnected)
        {
            QMessageBox::information(0,tr("Error"),tr("You must be connected to a MAV over serial link to flash firmware. Please connect to a MAV then try again"));
            return;
        }

        qDebug() << "Go download:" << m_buttonToUrlMap[senderbtn];
        QNetworkReply *reply = m_networkManager->get(QNetworkRequest(QUrl(m_buttonToUrlMap[senderbtn])));
        //http://firmware.diydrones.com/Plane/stable/apm2/ArduPlane.hex
        connect(reply,SIGNAL(finished()),this,SLOT(downloadFinished()));

        connect(reply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(firmwareListError(QNetworkReply::NetworkError)));
        connect(reply,SIGNAL(downloadProgress(qint64,qint64)),this,SLOT(firmwareDownloadProgress(qint64,qint64)));
        ui.statusLabel->setText("Downloading");
    }
}

void ApmFirmwareConfig::firmwareListError(QNetworkReply::NetworkError error)
{
    Q_UNUSED(error);
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    qDebug() << "Error!" << reply->errorString();
}
bool ApmFirmwareConfig::stripVersionFromGitReply(QString url, QString reply,QString type,QString stable,QString *out)
{
    if (url.contains(type) && url.contains("git-version.txt") && url.contains(stable))
    {
        QString version = reply.mid(reply.indexOf("APMVERSION:")+12).replace("\n","").replace("\r","").trimmed();
        *out = version;
        return true;
    }
    return false;

}

void ApmFirmwareConfig::firmwareListFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    QString replystr = reply->readAll();
    QString outstr = "";
    if (stripVersionFromGitReply(reply->url().toString(),replystr,"apm2-heli",(m_betaFirmwareChecked ? "beta" : "stable"),&outstr))
    {
        ui.copterLabel->setText((m_betaFirmwareChecked ? "BETA " : "") + outstr);
        return;
    }
    if (stripVersionFromGitReply(reply->url().toString(),replystr,"apm2-quad",(m_betaFirmwareChecked ? "beta" : "stable"),&outstr))
    {
        ui.quadLabel->setText((m_betaFirmwareChecked ? "BETA " : "") + outstr);
        return;
    }
    if (stripVersionFromGitReply(reply->url().toString(),replystr,"apm2-hexa",(m_betaFirmwareChecked ? "beta" : "stable"),&outstr))
    {
        ui.hexaLabel->setText((m_betaFirmwareChecked ? "BETA " : "") + outstr);
        return;
    }
    if (stripVersionFromGitReply(reply->url().toString(),replystr,"apm2-octa-quad",(m_betaFirmwareChecked ? "beta" : "stable"),&outstr))
    {
        ui.octaQuadLabel->setText((m_betaFirmwareChecked ? "BETA " : "") + outstr);
        return;
    }
    if (stripVersionFromGitReply(reply->url().toString(),replystr,"apm2-octa",(m_betaFirmwareChecked ? "beta" : "stable"),&outstr))
    {
        ui.octaLabel->setText((m_betaFirmwareChecked ? "BETA " : "") + outstr);
        return;
    }
    if (stripVersionFromGitReply(reply->url().toString(),replystr,"apm2-tri",(m_betaFirmwareChecked ? "beta" : "stable"),&outstr))
    {
        ui.triLabel->setText((m_betaFirmwareChecked ? "BETA " : "") + outstr);
        return;
    }
    if (stripVersionFromGitReply(reply->url().toString(),replystr,"apm2-y6",(m_betaFirmwareChecked ? "beta" : "stable"),&outstr))
    {
        ui.y6Label->setText((m_betaFirmwareChecked ? "BETA " : "") + outstr);
        return;
    }
    if (stripVersionFromGitReply(reply->url().toString(),replystr,"Plane",(m_betaFirmwareChecked ? "beta" : "stable"),&outstr))
    {
        ui.planeLabel->setText((m_betaFirmwareChecked ? "BETA " : "") + outstr);
        return;
    }
    if (stripVersionFromGitReply(reply->url().toString(),replystr,"Rover",(m_betaFirmwareChecked ? "beta" : "stable"),&outstr))
    {
        ui.roverLabel->setText((m_betaFirmwareChecked ? "BETA " : "") + outstr);
        return;
    }
    //qDebug() << "Match not found for:" << reply->url();
    //qDebug() << "Git version line:" <<  replystr;
}

ApmFirmwareConfig::~ApmFirmwareConfig()
{
}
