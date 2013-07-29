#ifndef APMFIRMWARECONFIG_H
#define APMFIRMWARECONFIG_H

#include <QWidget>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDebug>
#include <QTemporaryFile>
#include <QProcess>
#include <QXmlStreamReader>
#include <QMessageBox>
#include <QProcess>
#include <QScrollBar>

#include "qserialport.h"
#include "ui_ApmFirmwareConfig.h"

class ApmFirmwareConfig : public QWidget
{
    Q_OBJECT
    
public:
    explicit ApmFirmwareConfig(QWidget *parent = 0);
    ~ApmFirmwareConfig();
private slots:
    void firmwareListFinished();
    void firmwareListError(QNetworkReply::NetworkError error);
    void burnButtonClicked();
    void betaFirmwareButtonClicked(bool betafirmwareenabled);
    void downloadFinished();
    void firmwareProcessFinished(int status);
    void firmwareProcessReadyRead();
    void firmwareProcessError(QProcess::ProcessError error);
    void firmwareDownloadProgress(qint64 received,qint64 total);
private:
    //ApmFirmwareStatus *firmwareStatus;
    QString m_detectedComPort;
    QTemporaryFile *m_tempFirmwareFile;
    QNetworkAccessManager *m_networkManager;
    void requestFirmwares();
    void requestBetaFirmwares();
    bool stripVersionFromGitReply(QString url,QString reply,QString type,QString stable,QString *out);
    bool m_betaFirmwareChecked;
    QMap<QPushButton*,QString> m_buttonToUrlMap;
    Ui::ApmFirmwareConfig ui;
    class FirmwareDef
    {
    public:
        QString url;
        QString url2560;
        QString url25602;
        QString urlpx4;
        QString type;
        QString name;
        QString desc;
        int version;
    };
    QList<FirmwareDef> m_firmwareList;
};

#endif // APMFIRMWARECONFIG_H
