#include "apmremoteparamscontroller.h"

/*===================================================================
APM_PLANNER Open Source Ground Control Station

(c) 2014 APM_PLANNER PROJECT <http://www.diydrones.com>
(c) author: Bill Bonney <billbonney@communistech.com>

This file is part of the APM_PLANNER project

    APM_PLANNER is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    APM_PLANNER is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with APM_PLANNER. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/
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
#include <QApplication>

#include "apmremoteparamscontroller.h"

#define FRAME_PARAMS_LIST QUrl("https://api.github.com/repos/diydrones/ardupilot/contents/Tools/Frame_params")
#define FRAME_PARAMS_URL "https://raw.github.com/diydrones/ardupilot/master/Tools/Frame_params/"

static const QString dataLocation = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).at(0);

APMRemoteParamsController::APMRemoteParamsController() :
    m_networkReply(NULL)
{
    refreshParamList();
}

QString APMRemoteParamsController::statusText() const
{
    return m_statusText;
}
void APMRemoteParamsController::setStatusText(const QString& text)
{
    m_statusText = text;
}

void APMRemoteParamsController::refreshParamList()
{
    qDebug() << "refresh list of param files from server";
    setStatusText(tr("Refresh Param file list"));

    QUrl url = FRAME_PARAMS_LIST;
    qDebug() << "Deletando o networkReply no refreshParamList";
    m_networkReply->deleteLater();
    m_networkReply = m_networkAccessManager.get(QNetworkRequest(url));
    connect(m_networkReply, SIGNAL(finished()), this, SLOT(httpParamListFinished()));
    connect(m_networkReply, SIGNAL(downloadProgress(qint64,qint64)),
            this, SLOT(updateDataReadProgress(qint64,qint64)));
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
void APMRemoteParamsController::startFileDownloadRequest()
{
    if (curr == end)
        return;

    QJsonObject obj = (*curr).toObject();
    QUrl url(obj["download_url"].toString());

    QDir parameterDir(dataLocation);
    if (!parameterDir.exists())
        parameterDir.mkpath(dataLocation);

    QString filename = obj["name"].toString();
    QFile::remove(filename);
    m_downloadedParamFile = new QFile(parameterDir.absoluteFilePath(filename));
    m_downloadedParamFile->open(QIODevice::WriteOnly);

    qDebug() << url;
    m_networkReply = m_networkAccessManager.get(QNetworkRequest(url));
    connect(m_networkReply, SIGNAL(finished()), this, SLOT(httpFinished()));
    connect(m_networkReply, SIGNAL(readyRead()), this, SLOT(httpReadyRead()));
    connect(m_networkReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(updateDataReadProgress(qint64,qint64)));
    curr++;
}

void APMRemoteParamsController::httpFinished()
{
    qDebug() << "Finished:" << m_downloadedParamFile->fileName();
    m_downloadedParamFile->flush();
    m_downloadedParamFile->close();

    QVariant redirectionTarget = m_networkReply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (m_networkReply->error()) {
        m_downloadedParamFile->remove();
        setStatusText(tr("Download failed: %1.").arg(m_networkReply->errorString()));
    } else {
        QString fileName = m_downloadedParamFile->fileName();
        setStatusText(tr("Downloaded to %2.").arg(fileName));
    }

    m_networkReply->deleteLater();
    m_networkReply = NULL;
    delete m_downloadedParamFile;
    m_downloadedParamFile = NULL;

    startFileDownloadRequest();
}

void APMRemoteParamsController::httpReadyRead()
{
    if (m_downloadedParamFile)
        m_downloadedParamFile->write(m_networkReply->readAll());
}

void APMRemoteParamsController::updateDataReadProgress(qint64 bytesRead, qint64 totalBytes)
{
    qDebug() << bytesRead << totalBytes;
}

void APMRemoteParamsController::httpParamListFinished()
{
    if (m_networkReply->error()) {
        qDebug() <<  "Download failed:" << m_networkReply->errorString();
        return;
    }
    processDownloadedVersionObject(m_networkReply->readAll());
    startFileDownloadRequest();
}

void APMRemoteParamsController::processDownloadedVersionObject(const QByteArray &listObject)
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

QVariant APMRemoteParamsController::data(const QModelIndex &index, int role) const
{
    Q_UNUSED(index);
    Q_UNUSED(role);
    return QVariant();
}

int APMRemoteParamsController::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 0;
}
