// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QHELPSEARCHINDEXWRITER_H
#define QHELPSEARCHINDEXWRITER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists for the convenience
// of the help generator tools. This header file may change from version
// to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qmutex.h>
#include <QtCore/qthread.h>

QT_BEGIN_NAMESPACE

class QSqlDatabase;

namespace fulltextsearch {

// TODO: Employ QFuture / QtConcurrent::run() ?
class QHelpSearchIndexWriter : public QThread
{
    Q_OBJECT

public:
    ~QHelpSearchIndexWriter() override;

    void cancelIndexing();
    void updateIndex(const QString &collectionFile, const QString &indexFilesFolder, bool reindex);

signals:
    void indexingStarted();
    void indexingFinished();

private:
    void run() override;

private:
    QMutex m_mutex;

    bool m_cancel = false;
    bool m_reindex;
    QString m_collectionFile;
    QString m_indexFilesFolder;
};

} // namespace fulltextsearch

QT_END_NAMESPACE

#endif // QHELPSEARCHINDEXWRITER_H
