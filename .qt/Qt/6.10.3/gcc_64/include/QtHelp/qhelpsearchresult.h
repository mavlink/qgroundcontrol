// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QHELPSEARCHRESULT_H
#define QHELPSEARCHRESULT_H

#include <QtHelp/qhelp_global.h>

#include <QtCore/qshareddata.h>

QT_BEGIN_NAMESPACE

class QHelpSearchResultData;
class QString;
class QUrl;

class QHELP_EXPORT QHelpSearchResult
{
public:
    QHelpSearchResult();
    QHelpSearchResult(const QHelpSearchResult &other);
    QHelpSearchResult(const QUrl &url, const QString &title, const QString &snippet);
    ~QHelpSearchResult();

    QHelpSearchResult &operator=(const QHelpSearchResult &other);

    QString title() const;
    QUrl url() const;
    QString snippet() const;

private:
    QSharedDataPointer<QHelpSearchResultData> d;
};

QT_END_NAMESPACE

#endif // QHELPSEARCHRESULT_H
