#ifndef APMREMOTEPARAMSCONTROLLER_H
#define APMREMOTEPARAMSCONTROLLER_H

#include <QAbstractListModel>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QJsonArray>

class QNetworkReply;
class QFile;
class QUrl;

class APMRemoteParamsController : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString statusText READ statusText)
public:
    explicit APMRemoteParamsController();
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QString statusText() const;
public slots:
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

private:
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
