#include "ApmFirmwareConfig.h"
#include <QXmlStreamReader>
#include <QMessageBox>
#include <QProcess>
#include "LinkManager.h"
#include "LinkInterface.h"
#include "qserialport.h"
#include "qserialportinfo.h"
#include "SerialLink.h"
ApmFirmwareConfig::ApmFirmwareConfig(QWidget *parent) : QWidget(parent)
{
    ui.setupUi(this);
    firmwareStatus = 0;
    m_betaFirmwareChecked = false;
    m_tempFirmwareFile=0;
    //

    //QNetworkRequest req(QUrl("https://raw.github.com/diydrones/binary/master/Firmware/firmware2.xml"));



    m_networkManager = new QNetworkAccessManager(this);

    connect(ui.roverPushButton,SIGNAL(clicked()),this,SLOT(burnButtonClicked()));
    connect(ui.planePushButton,SIGNAL(clicked()),this,SLOT(burnButtonClicked()));
    connect(ui.copterPushButton,SIGNAL(clicked()),this,SLOT(burnButtonClicked()));
    connect(ui.hexaPushButton,SIGNAL(clicked()),this,SLOT(burnButtonClicked()));
    connect(ui.octaQuadPushButton,SIGNAL(clicked()),this,SLOT(burnButtonClicked()));
    connect(ui.octaPushButton,SIGNAL(clicked()),this,SLOT(burnButtonClicked()));
    connect(ui.quadPushButton,SIGNAL(clicked()),this,SLOT(burnButtonClicked()));
    connect(ui.triPushButton,SIGNAL(clicked()),this,SLOT(burnButtonClicked()));
    connect(ui.y6PushButton,SIGNAL(clicked()),this,SLOT(burnButtonClicked()));
    requestFirmwares(false);
    connect(ui.betaFirmwareButton,SIGNAL(clicked()),this,SLOT(betaFirmwareButtonClicked()));
}
void ApmFirmwareConfig::requestFirmwares(bool beta)
{

    if (!beta)
    {
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
    else
    {
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
}

void ApmFirmwareConfig::betaFirmwareButtonClicked()
{
    if (!m_betaFirmwareChecked)
    {
        m_betaFirmwareChecked = true;
        requestFirmwares(true);
    }
    else
    {
        m_betaFirmwareChecked = false;
        requestFirmwares(false);
    }
}
void ApmFirmwareConfig::firmwareProcessFinished(int status)
{
    QProcess *proc = qobject_cast<QProcess*>(sender());
    if (!proc)
    {
        return;
    }
    //qDebug() << "Error:" << proc->errorString();
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
    QString error = proc->readAllStandardError() + proc->readAllStandardOutput();
    if (error.contains("Writing"))
    {
        firmwareStatus->resetProgress();
    }
    else if (error.contains("Reading"))
    {
        firmwareStatus->resetProgress();
    }
    if (error.startsWith("#"))
    {
        firmwareStatus->progressTick();
    }
    else
    {
        firmwareStatus->passMessage(error);
    }
    qDebug() << "E:" << error;
    //qDebug() << "AVR Output:" << proc->readAllStandardOutput();
    //qDebug() << "AVR Output:" << proc->readAllStandardError();
}

void ApmFirmwareConfig::downloadFinished()
{
    qDebug() << "Download finished, burning firmware";
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
    bool foundconnected = false;
    QString detectedcomport = "COM4";
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
            detectedcomport = link->getPortName();
            link->requestReset();
            foundconnected = true;
            link->disconnect();
            link->wait(1000); // Wait 1 second for it to disconnect.
        }
    }
    if (!foundconnected)
    {
        QMessageBox::information(0,"Error","You must be connected to a MAV over serial link to flash firmware");
        return;
    }
    qDebug() << "Attempting to reset port";

    QSerialPort port;

    port.setPortName(detectedcomport);
    port.open(QIODevice::ReadWrite);
    port.setDataTerminalReady(true);
    port.waitForBytesWritten(250);
    port.setDataTerminalReady(false);
    port.close();

    firmwareStatus->setStatus("Burning");
#ifdef Q_OS_WIN
    process->start("avrdude/avrdude.exe",QStringList() << "-Cavrdude/avrdude.conf" << "-pm2560" << "-cstk500" << QString("-P").append(detectedcomport) << QString("-Uflash:w:").append(m_tempFirmwareFile->fileName()).append(":i"));
#else
    process->start("avrdude",QStringList() << "-Cavrdude/avrdude.conf" << "-pm2560" << "-cstk500" << QString("-P").append(detectedcomport) << QString("-Uflash:w:").append(m_tempFirmwareFile->fileName()).append(":i"));
#endif
}
void ApmFirmwareConfig::firmwareProcessError(QProcess::ProcessError error)
{
    qDebug() << "Error:" << error;
}

void ApmFirmwareConfig::burnButtonClicked()
{
    QPushButton *senderbtn = qobject_cast<QPushButton*>(sender());
    if (m_buttonToUrlMap.contains(senderbtn))
    {
        qDebug() << "Go download:" << m_buttonToUrlMap[senderbtn];
        QNetworkReply *reply = m_networkManager->get(QNetworkRequest(QUrl(m_buttonToUrlMap[senderbtn])));
        //http://firmware.diydrones.com/Plane/stable/apm2/ArduPlane.hex
        connect(reply,SIGNAL(finished()),this,SLOT(downloadFinished()));

        connect(reply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(firmwareListError(QNetworkReply::NetworkError)));
        firmwareStatus = new ApmFirmwareStatus();
        firmwareStatus->show();
        firmwareStatus->setStatus("Downloading");
    }
}

void ApmFirmwareConfig::firmwareListError(QNetworkReply::NetworkError error)
{
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
        ui.copterLabel->setText(outstr);
        return;
    }
    if (stripVersionFromGitReply(reply->url().toString(),replystr,"apm2-quad",(m_betaFirmwareChecked ? "beta" : "stable"),&outstr))
    {
        ui.quadLabel->setText(outstr);
        return;
    }
    if (stripVersionFromGitReply(reply->url().toString(),replystr,"apm2-hexa",(m_betaFirmwareChecked ? "beta" : "stable"),&outstr))
    {
        ui.hexaLabel->setText(outstr);
        return;
    }
    if (stripVersionFromGitReply(reply->url().toString(),replystr,"apm2-octa",(m_betaFirmwareChecked ? "beta" : "stable"),&outstr))
    {
        ui.octaLabel->setText(outstr);
        return;
    }
    if (stripVersionFromGitReply(reply->url().toString(),replystr,"apm2-octa-quad",(m_betaFirmwareChecked ? "beta" : "stable"),&outstr))
    {
        ui.octaQuadLabel->setText(outstr);
        return;
    }
    if (stripVersionFromGitReply(reply->url().toString(),replystr,"apm2-tri",(m_betaFirmwareChecked ? "beta" : "stable"),&outstr))
    {
        ui.triLabel->setText(outstr);
        return;
    }
    if (stripVersionFromGitReply(reply->url().toString(),replystr,"apm2-y6",(m_betaFirmwareChecked ? "beta" : "stable"),&outstr))
    {
        ui.y6Label->setText(outstr);
        return;
    }
    if (stripVersionFromGitReply(reply->url().toString(),replystr,"Plane",(m_betaFirmwareChecked ? "beta" : "stable"),&outstr))
    {
        ui.planeLabel->setText(outstr);
        return;
    }
    if (stripVersionFromGitReply(reply->url().toString(),replystr,"Rover",(m_betaFirmwareChecked ? "beta" : "stable"),&outstr))
    {
        ui.roverLabel->setText(outstr);
        return;
    }
    //qDebug() << replystr;
    /*
    QXmlStreamReader xml(replystr);
    while (!xml.atEnd())
    {
        if (xml.name() == "options" && xml.isStartElement())
        {
            xml.readNext();
            while (xml.name() != "options")
            {
                if (xml.name() == "Firmware" && xml.isStartElement())
                {
                    xml.readNext();
                    FirmwareDef def;
                    while (xml.name() != "Firmware")
                    {
                        if (xml.name() == "url" && xml.isStartElement())
                        {
                            xml.readNext();
                            def.url = xml.text().toString();
                        }
                        else if (xml.name() == "url2560" && xml.isStartElement())
                        {
                            xml.readNext();
                            def.url2560 = xml.text().toString();
                        }
                        else if (xml.name() == "url2560-2" && xml.isStartElement())
                        {
                            xml.readNext();
                            def.url25602 = xml.text().toString();
                        }
                        else if (xml.name() == "urlpx4" && xml.isStartElement())
                        {
                            xml.readNext();
                            def.urlpx4 = xml.text().toString();
                        }
                        else if (xml.name() == "name" && xml.isStartElement())
                        {
                            xml.readNext();
                            def.name = xml.text().toString();
                        }
                        else if (xml.name() == "desc" && xml.isStartElement())
                        {
                            xml.readNext();
                            def.desc = xml.text().toString();
                        }
                        else if (xml.name() == "format_version" && xml.isStartElement())
                        {
                            xml.readNext();
                            def.version = xml.text().toString().toInt();
                        }

                        xml.readNext();
                    }
                }
                xml.readNext();
            }
        }
        xml.readNext();
    }*/
}

ApmFirmwareConfig::~ApmFirmwareConfig()
{
}
