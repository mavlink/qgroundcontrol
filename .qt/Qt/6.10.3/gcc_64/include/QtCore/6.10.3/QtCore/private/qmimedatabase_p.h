// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2015 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author David Faure <david.faure@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QMIMEDATABASE_P_H
#define QMIMEDATABASE_P_H

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

#include "qmimedatabase.h"
#include "qmimetype.h"

QT_REQUIRE_CONFIG(mimetype);

#include "qmimetype_p.h"
#include "qmimeglobpattern_p.h"

#include <QtCore/qelapsedtimer.h>
#include <QtCore/qlist.h>
#include <QtCore/qmutex.h>

#include <vector>
#include <memory>

QT_BEGIN_NAMESPACE

class QFileInfo;
class QIODevice;
class QMimeDatabase;
class QMimeProviderBase;

class QMimeDatabasePrivate
{
public:
    Q_DISABLE_COPY_MOVE(QMimeDatabasePrivate)

    QMimeDatabasePrivate();
    ~QMimeDatabasePrivate();

    static QMimeDatabasePrivate *instance();

    const QString &defaultMimeType() const { return m_defaultMimeType; }

    bool inherits(const QString &mime, const QString &parent);

    QList<QMimeType> allMimeTypes();

    QString resolveAlias(const QString &nameOrAlias);
    QStringList parents(const QString &mimeName);
    QMimeType mimeTypeForName(const QString &nameOrAlias);
    QMimeType mimeTypeForFileNameAndData(const QString &fileName, QIODevice *device);
    QMimeType mimeTypeForFileExtension(const QString &fileName);
    QMimeType mimeTypeForData(QIODevice *device);
    QMimeType mimeTypeForFile(const QString &fileName, const QFileInfo &fileInfo, QMimeDatabase::MatchMode mode);
    QMimeType findByData(const QByteArray &data, int *priorityPtr);
    QStringList mimeTypeForFileName(const QString &fileName);
    QMimeGlobMatchResult findByFileName(const QString &fileName);

    // API for QMimeType. Takes care of locking the mutex.
    QMimeTypePrivate::LocaleHash localeComments(const QString &name);
    QStringList globPatterns(const QString &name);
    QString genericIcon(const QString &name);
    QString icon(const QString &name);
    QStringList mimeParents(const QString &mimeName);
    QStringList listAliases(const QString &mimeName);
    bool mimeInherits(const QString &mime, const QString &parent);

private:
    using Providers = std::vector<std::unique_ptr<QMimeProviderBase>>;
    const Providers &providers();
    bool shouldCheck();
    void loadProviders();
    QString fallbackParent(const QString &mimeTypeName) const;

    const QString m_defaultMimeType;
    mutable Providers m_providers; // most local first, most global last
    QElapsedTimer m_lastCheck;

public:
    QMutex mutex;
};

QT_END_NAMESPACE

#endif // QMIMEDATABASE_P_H
