// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QNETWORKFILE_H
#define QNETWORKFILE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Network Access API.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>
#include <QFile>
#include <qnetworkreply.h>

QT_BEGIN_NAMESPACE

class QNetworkFile : public QFile
{
    Q_OBJECT
public:
    QNetworkFile();
    QNetworkFile(const QString &name);
    using QFile::open;

public Q_SLOTS:
    void open();
    void close() override;

Q_SIGNALS:
    void finished(bool ok);
    void headerRead(QHttpHeaders::WellKnownHeader, const QByteArray &value);
    void networkError(QNetworkReply::NetworkError error, const QString &message);
};

QT_END_NAMESPACE

#endif // QNETWORKFILE_H
