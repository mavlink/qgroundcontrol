/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

#ifndef QGCFileDownload_H
#define QGCFileDownload_H

#include <QNetworkReply>

class QGCFileDownload : public QNetworkAccessManager
{
    Q_OBJECT
    
public:
    QGCFileDownload(QObject* parent = NULL);
    
    /// Download the specified remote file.
    ///     @param remoteFile File to download. Can be http address or file system path.
    /// @return true: Asynchronous download has started, false: Download initialization failed
    bool download(const QString& remoteFile);

signals:
    void downloadProgress(qint64 curr, qint64 total);
    void downloadFinished(QString remoteFile, QString localFile);
    void error(QString errorMsg);

private:
    void _downloadFinished(void);
    void _downloadError(QNetworkReply::NetworkError code);
};

#endif
