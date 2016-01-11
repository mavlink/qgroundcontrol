/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

#include "APMRemoteParamsDownloader.h"

#include <QMessageBox>
#include <QDesktopServices>
#include <QFile>
#include <QFileDialog>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCryptographicHash>
#include <QApplication>

#include "APMRemoteParamsDownloader.h"

#define FRAME_PARAMS_LIST QUrl("https://api.github.com/repos/diydrones/ardupilot/contents/Tools/Frame_params")
#define FRAME_PARAMS_URL "https://raw.github.com/diydrones/ardupilot/master/Tools/Frame_params/"

static QString dataLocation;

APMRemoteParamsDownloader::APMRemoteParamsDownloader(const QString& file) :
    m_fileToDownload(file),
    m_networkReply(NULL),
    m_downloadedParamFile(NULL)
{
    dataLocation = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).at(0)
           + QDir::separator() + qApp->applicationName();
    refreshParamList();
}

QString APMRemoteParamsDownloader::statusText() const
{
    return m_statusText;
}
void APMRemoteParamsDownloader::setStatusText(const QString& text)
{
    m_statusText = text;
}

void APMRemoteParamsDownloader::refreshParamList()
{
    setStatusText(tr("Refresh Param file list"));

    QUrl url = FRAME_PARAMS_LIST;
    m_networkReply->deleteLater();
    m_networkReply = m_networkAccessManager.get(QNetworkRequest(url));
    connect(m_networkReply, &QNetworkReply::finished, this, &APMRemoteParamsDownloader::httpParamListFinished);
    connect(m_networkReply, &QNetworkReply::downloadProgress, this, &APMRemoteParamsDownloader::updateDataReadProgress);
}

/* Returned Json Example
    "_links": {
        "git":"https://api.github.com/repos/diydrones/ardupilot/git/blobs/a7074e606d695566f9a8c87724ad52e5e3baba7d",
        "html":"https://github.com/diydrones/ardupilot/blob/master/Tools/Frame_params/Parrot_Bebop.param",
        "self":"https://api.github.com/repos/diydrones/ardupilot/contents/Tools/Frame_params/Parrot_Bebop.param?ref=master"
    },
    "download_url":"https://raw.githubusercontent.com/diydrones/ardupilot/master/Tools/Frame_params/Parrot_Bebop.param",
    "git_url":"https://api.github.com/repos/diydrones/ardupilot/git/blobs/a7074e606d695566f9a8c87724ad52e5e3baba7d",
    "html_url":"https://github.com/diydrones/ardupilot/blob/master/Tools/Frame_params/Parrot_Bebop.param",
    "name":"Parrot_Bebop.param","path":"Tools/Frame_params/Parrot_Bebop.param",""
    "sha":"a7074e606d695566f9a8c87724ad52e5e3baba7d",
    "size":533,
    "type":"file",
    "url":"https://api.github.com/repos/diydrones/ardupilot/contents/Tools/Frame_params/Parrot_Bebop.param?ref=master"
*/
void APMRemoteParamsDownloader::startFileDownloadRequest()
{
    QUrl url;

    QJsonObject obj;

    // Find the correct file from the json file list.
    while(curr != end) {
        obj = (*curr).toObject();
        url = QUrl(obj["download_url"].toString());
        QString name = obj["name"].toString();
        if (name == m_fileToDownload) {
            break;
        }
        curr++;
    }
    if (curr == end)
        return;

    QDir parameterDir(dataLocation);
    if (!parameterDir.exists())
        parameterDir.mkpath(dataLocation);

    QString filename = parameterDir.absoluteFilePath(obj["name"].toString());

    if(m_downloadedParamFile)
        m_downloadedParamFile->deleteLater();
    m_downloadedParamFile = new QFile(filename);
    m_downloadedParamFile->open(QIODevice::WriteOnly);

    m_networkReply = m_networkAccessManager.get(QNetworkRequest(url));
    connect(m_networkReply, &QNetworkReply::finished, this, &APMRemoteParamsDownloader::httpFinished);
    connect(m_networkReply, &QNetworkReply::readyRead, this, &APMRemoteParamsDownloader::httpReadyRead);
    connect(m_networkReply, &QNetworkReply::downloadProgress, this, &APMRemoteParamsDownloader::updateDataReadProgress);
    curr++;
}

void APMRemoteParamsDownloader::httpFinished()
{
    m_downloadedParamFile->flush();
    m_downloadedParamFile->close();

    if (m_networkReply->error()) {
        m_downloadedParamFile->remove();
        setStatusText(tr("Download failed: %1.").arg(m_networkReply->errorString()));
    }

    m_networkReply->deleteLater();
    m_networkReply = NULL;
    delete m_downloadedParamFile;
    m_downloadedParamFile = NULL;

    emit finished();
}

void APMRemoteParamsDownloader::httpReadyRead()
{
    if (m_downloadedParamFile)
        m_downloadedParamFile->write(m_networkReply->readAll());
}

void APMRemoteParamsDownloader::updateDataReadProgress(qint64 bytesRead, qint64 totalBytes)
{
    Q_UNUSED(bytesRead);
    Q_UNUSED(totalBytes);
}

void APMRemoteParamsDownloader::httpParamListFinished()
{
    if (m_networkReply->error()) {
        qDebug() <<  "Download failed:" << m_networkReply->errorString();
        return;
    }
    processDownloadedVersionObject(m_networkReply->readAll());
    startFileDownloadRequest();
}

void APMRemoteParamsDownloader::processDownloadedVersionObject(const QByteArray &listObject)
{
    QJsonParseError jsonErrorChecker;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(listObject, &jsonErrorChecker);
    if (jsonErrorChecker.error != QJsonParseError::NoError) {
        qDebug() << "Json error while parsing document:" << jsonErrorChecker.errorString();
        return;
    }

    m_documentArray = jsonDocument.array();
    curr = m_documentArray.constBegin();
    end = m_documentArray.constEnd();
}
