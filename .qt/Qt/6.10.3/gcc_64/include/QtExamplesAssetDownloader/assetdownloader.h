// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef ASSETDOWNLOADER_H
#define ASSETDOWNLOADER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/QObject>
#include <QtCore/QUrl>

#include <memory>

QT_BEGIN_NAMESPACE

namespace Assets::Downloader {

class AssetDownloaderPrivate;

class AssetDownloader : public QObject
{
    Q_OBJECT

    Q_PROPERTY(
        QUrl downloadBase
        READ downloadBase
        WRITE setDownloadBase
        NOTIFY downloadBaseChanged)

    Q_PROPERTY(
        QUrl preferredLocalDownloadDir
        READ preferredLocalDownloadDir
        WRITE setPreferredLocalDownloadDir
        NOTIFY preferredLocalDownloadDirChanged)

    Q_PROPERTY(
        QUrl offlineAssetsFilePath
        READ offlineAssetsFilePath
        WRITE setOfflineAssetsFilePath
        NOTIFY offlineAssetsFilePathChanged)

    Q_PROPERTY(
        QString jsonFileName
        READ jsonFileName
        WRITE setJsonFileName
        NOTIFY jsonFileNameChanged)

    Q_PROPERTY(
        QString zipFileName
        READ zipFileName
        WRITE setZipFileName
        NOTIFY zipFileNameChanged)

    Q_PROPERTY(
        QUrl localDownloadDir
        READ localDownloadDir
        NOTIFY localDownloadDirChanged)

public:
    AssetDownloader(QObject *parent = nullptr);
    ~AssetDownloader();

    QUrl downloadBase() const;
    void setDownloadBase(const QUrl &downloadBase);

    QUrl preferredLocalDownloadDir() const;
    void setPreferredLocalDownloadDir(const QUrl &localDir);

    QUrl offlineAssetsFilePath() const;
    void setOfflineAssetsFilePath(const QUrl &offlineAssetsFilePath);

    QString jsonFileName() const;
    void setJsonFileName(const QString &jsonFileName);

    QString zipFileName() const;
    void setZipFileName(const QString &zipFileName);

    QUrl localDownloadDir() const;

    Q_INVOKABLE QStringList networkErrors() const;
    Q_INVOKABLE QStringList sslErrors() const;

public Q_SLOTS:
    void start();

protected:
    virtual QUrl resolvedUrl(const QUrl &url) const;

Q_SIGNALS:
    void started();
    void finished(bool success);
    void progressChanged(int progressValue, int progressMaximum, const QString &progressText);
    void localDownloadDirChanged(const QUrl &url);

    void downloadBaseChanged(const QUrl &);
    void preferredLocalDownloadDirChanged(const QUrl &url);
    void offlineAssetsFilePathChanged(const QUrl &);
    void jsonFileNameChanged(const QString &);
    void zipFileNameChanged(const QString &);

private:
    std::unique_ptr<AssetDownloaderPrivate> d;
};

} // namespace Assets::Downloader

QT_END_NAMESPACE

#endif // ASSETDOWNLOADER_H
