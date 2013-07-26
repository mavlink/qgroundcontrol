#ifndef APMFIRMWARECONFIG_H
#define APMFIRMWARECONFIG_H

#include <QWidget>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDebug>
#include <QTemporaryFile>
#include <QProcess>
#include "qserialport.h"
#include "ui_ApmFirmwareConfig.h"
#include "ApmFirmwareStatus.h"

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
    void betaFirmwareButtonClicked();
    void downloadFinished();
    void firmwareProcessFinished(int status);
    void firmwareProcessReadyRead();
    void firmwareProcessError(QProcess::ProcessError error);
private:
    ApmFirmwareStatus *firmwareStatus;
    QTemporaryFile *m_tempFirmwareFile;
    QNetworkAccessManager *m_networkManager;
    void requestFirmwares(bool beta);
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
