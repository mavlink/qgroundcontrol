// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QHELPSEARCHENGINECORE_H
#define QHELPSEARCHENGINECORE_H

#include <QtHelp/qhelp_global.h>
#include <QtHelp/qhelpsearchresult.h>

#include <QtCore/qobject.h>
#include <QtCore/qshareddata.h>

QT_BEGIN_NAMESPACE

class QHelpEngineCore;
class QHelpSearchEngineCorePrivate;

class QHELP_EXPORT QHelpSearchEngineCore : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QHelpSearchEngineCore)

public:
    explicit QHelpSearchEngineCore(QHelpEngineCore *helpEngine, QObject *parent = nullptr);
    ~QHelpSearchEngineCore() override;

    int searchResultCount() const;
    QList<QHelpSearchResult> searchResults(int start, int end) const;
    QString searchInput() const;

public Q_SLOTS:
    void reindexDocumentation();
    void cancelIndexing();

    void search(const QString &searchInput);
    void cancelSearching();

    void scheduleIndexDocumentation();

Q_SIGNALS:
    void indexingStarted();
    void indexingFinished();

    void searchingStarted();
    void searchingFinished();
};

QT_END_NAMESPACE

#endif // QHELPSEARCHENGINECORE_H
