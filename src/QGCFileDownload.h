/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef QGCFileDownload_H
#define QGCFileDownload_H

#include <QNetworkReply>

class QGCFileDownload : public QNetworkAccessManager
{
    Q_OBJECT
    
public:
    QGCFileDownload(QObject* parent = nullptr);
    
    /// Download the specified remote file.
    ///     @param remoteFile File to download. Can be http address or file system path.
    ///     @param redirect true: call is internal due to redirect
    /// @return true: Asynchronous download has started, false: Download initialization failed
    bool download(const QString& remoteFile, bool redirect = false);

signals:
    void downloadProgress(qint64 curr, qint64 total);
    void downloadFinished(QString remoteFile, QString localFile);
    void error(QString errorMsg);

private:
    void _downloadFinished(void);
    void _downloadError(QNetworkReply::NetworkError code);

    QString _originalRemoteFile;
};

#endif
