/*!
 *   @file
 *   @brief Camera Http Download Utility
 *   @author Gus Grubba <gus@auterion.com>
 *   @author Hugo Trippaers <htrippaers@schubergphilis.com>
 *
 */

#ifndef QGCCAMERAHTTPDOWNLOADER_H
#define QGCCAMERAHTTPDOWNLOADER_H

#include "QGCApplication.h"
#include <QLoggingCategory>

class QGCCameraHttpDownloader : public QObject
{
    Q_OBJECT
public:
    explicit QGCCameraHttpDownloader(QObject *);
    ~QGCCameraHttpDownloader();

    void download(const QString &);

protected:
    QNetworkAccessManager*              _netManager         = nullptr;

protected slots:
    virtual void    _downloadFinished       ();

signals:
    void                                dataReady           (QByteArray);

};
#endif // QGCCAMERAHTTPDOWNLOADER_H
