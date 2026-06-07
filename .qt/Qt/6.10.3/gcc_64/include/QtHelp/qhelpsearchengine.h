// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QHELPSEARCHENGINE_H
#define QHELPSEARCHENGINE_H

#include <QtHelp/qhelp_global.h>
#include <QtHelp/qhelpsearchresult.h>

#include <QtCore/qobject.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE

class QHelpEngineCore;
class QHelpSearchEnginePrivate;
class QHelpSearchQueryWidget;
class QHelpSearchResultWidget;

#if QT_DEPRECATED_SINCE(6, 7)
class QHELP_EXPORT QHelpSearchQuery
{
public:
    enum FieldName { DEFAULT = 0, FUZZY, WITHOUT, PHRASE, ALL, ATLEAST };

    QT_DEPRECATED_VERSION_X_6_7("Use QString instead")
    QHelpSearchQuery()
        : fieldName(DEFAULT) { wordList.clear(); }
    QT_DEPRECATED_VERSION_X_6_7("Use QString instead")
    QHelpSearchQuery(FieldName field, const QStringList &wordList_)
        : fieldName(field), wordList(wordList_) {}

    FieldName fieldName;
    QStringList wordList;
};
#endif // QT_DEPRECATED_SINCE(6, 7)

class QHELP_EXPORT QHelpSearchEngine : public QObject
{
    Q_OBJECT

public:
    explicit QHelpSearchEngine(QHelpEngineCore *helpEngine, QObject *parent = nullptr);
    ~QHelpSearchEngine();

    QHelpSearchQueryWidget *queryWidget();
    QHelpSearchResultWidget *resultWidget();

#if QT_DEPRECATED_SINCE(5, 9)
    typedef QPair<QString, QString> SearchHit;

    QT_DEPRECATED int hitsCount() const;
    QT_DEPRECATED int hitCount() const;
    QT_DEPRECATED QList<SearchHit> hits(int start, int end) const;
    QT_DEPRECATED QList<QHelpSearchQuery> query() const;
#endif

    int searchResultCount() const;
    QList<QHelpSearchResult> searchResults(int start, int end) const;
    QString searchInput() const;

public Q_SLOTS:
    void reindexDocumentation();
    void cancelIndexing();

#if QT_DEPRECATED_SINCE(5, 9)
    QT_DEPRECATED void search(const QList<QHelpSearchQuery> &queryList);
#endif

    void search(const QString &searchInput);
    void cancelSearching();

    void scheduleIndexDocumentation();

Q_SIGNALS:
    void indexingStarted();
    void indexingFinished();

    void searchingStarted();
    void searchingFinished(int searchResultCount);

private Q_SLOTS:
    void indexDocumentation();

private:
    QHelpSearchEnginePrivate *d;
};

QT_END_NAMESPACE

#endif // QHELPSEARCHENGINE_H
