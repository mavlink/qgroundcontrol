// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLASSETDOWNLOADER_P_H
#define QQMLASSETDOWNLOADER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtExamplesAssetDownloader/assetdownloader.h>
#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

namespace Assets::Downloader {

class AssetDownloaderHelper : public AssetDownloader
{
    Q_OBJECT

public:
    AssetDownloaderHelper(QObject *parent = nullptr);

protected:
    virtual QUrl resolvedUrl(const QUrl &url) const override;
};

struct QQmlAssetDownloader
{
    Q_GADGET
    QML_FOREIGN(AssetDownloaderHelper)
    QML_NAMED_ELEMENT(AssetDownloader)
    QML_ADDED_IN_VERSION(6, 8)

public:
    static AssetDownloaderHelper *create(QQmlEngine *, QJSEngine *);
};

} // namespace Assets::Downloader

QT_END_NAMESPACE

#endif
