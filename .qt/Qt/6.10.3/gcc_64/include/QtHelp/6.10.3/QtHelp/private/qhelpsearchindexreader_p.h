// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QHELPSEARCHINDEXREADER_H
#define QHELPSEARCHINDEXREADER_H

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

#include "qhelpsearchresult.h"

#include <QtCore/qlist.h>
#include <QtCore/qmutex.h>
#include <QtCore/qthread.h>

QT_BEGIN_NAMESPACE

namespace fulltextsearch {

// TODO: Employ QFuture / QtConcurrent::run() ?
class QHelpSearchIndexReader : public QThread
{
    Q_OBJECT

public:
    ~QHelpSearchIndexReader() override;

    void cancelSearching();
    void search(const QString &collectionFile, const QString &indexFilesFolder,
                const QString &searchInput, bool usesFilterEngine = false);
    int searchResultCount() const;
    QList<QHelpSearchResult> searchResults(int start, int end) const;

signals:
    void searchingStarted();
    void searchingFinished();

private:
    void run() override;

    mutable QMutex m_mutex;
    QList<QHelpSearchResult> m_searchResults;
    bool m_cancel = false;
    QString m_collectionFile;
    QString m_searchInput;
    QString m_indexFilesFolder;
    bool m_usesFilterEngine = false;
};

} // namespace fulltextsearch

QT_END_NAMESPACE

#endif // QHELPSEARCHINDEXREADER_H
