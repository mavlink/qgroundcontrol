#ifndef APMREMOTEPARAMSCONTROLLER_H
#define APMREMOTEPARAMSCONTROLLER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QJsonArray>

class QNetworkReply;
class QFile;
class QUrl;

class APMRemoteParamsDownloader : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString statusText READ statusText)
public:
    explicit APMRemoteParamsDownloader(const QString& file);
    QString statusText() const;
    void refreshParamList();
    void httpParamListFinished();
    void httpFinished();
    void httpReadyRead();
    void updateDataReadProgress(qint64 bytesRead, qint64 totalBytes);

private:
    void setStatusText(const QString& text);
    void startFileDownloadRequest();
    void manualListSetup();
    void processDownloadedVersionObject(const QByteArray& listObject);
    void startDownloadingRemoteParams();

signals:
    void finished();
private:
    QString m_fileToDownload;
    QString m_statusText;
    QNetworkAccessManager m_networkAccessManager;
    QNetworkReply* m_networkReply;
    QFile* m_downloadedParamFile;

    // the list of needed documents.
    QJsonArray m_documentArray;
    QJsonArray::const_iterator curr;
    QJsonArray::const_iterator end;
};

#endif // APMREMOTEPARAMSCONTROLLER_H
