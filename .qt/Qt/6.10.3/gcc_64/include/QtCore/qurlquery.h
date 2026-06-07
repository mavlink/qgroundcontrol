// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QURLQUERY_H
#define QURLQUERY_H

#include <QtCore/qcompare.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qurl.h>

#include <initializer_list>

QT_BEGIN_NAMESPACE

Q_CORE_EXPORT size_t qHash(const QUrlQuery &key, size_t seed = 0) noexcept;

class QUrlQueryPrivate;
class Q_CORE_EXPORT QUrlQuery
{
public:
    QUrlQuery();
    explicit QUrlQuery(const QUrl &url);
    explicit QUrlQuery(const QString &queryString);
    QUrlQuery(std::initializer_list<std::pair<QString, QString>> list)
        : QUrlQuery()
    {
        for (const std::pair<QString, QString> &item : list)
            addQueryItem(item.first, item.second);
    }

    QUrlQuery(const QUrlQuery &other);
    QUrlQuery(QUrlQuery &&other) noexcept;
    QUrlQuery &operator=(const QUrlQuery &other);
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(QUrlQuery)
    ~QUrlQuery();

#if QT_CORE_REMOVED_SINCE(6, 8)
    bool operator==(const QUrlQuery &other) const;
    bool operator!=(const QUrlQuery &other) const
    { return !operator==(other); }
#endif

    void swap(QUrlQuery &other) noexcept { d.swap(other.d); }

    bool isEmpty() const;
    bool isDetached() const;
    void clear();

    QString query(QUrl::ComponentFormattingOptions encoding = QUrl::PrettyDecoded) const;
    void setQuery(const QString &queryString);
    QString toString(QUrl::ComponentFormattingOptions encoding = QUrl::PrettyDecoded) const
    { return query(encoding); }

    void setQueryDelimiters(QChar valueDelimiter, QChar pairDelimiter);
    QChar queryValueDelimiter() const;
    QChar queryPairDelimiter() const;

    void setQueryItems(const QList<std::pair<QString, QString> > &query);
    QList<std::pair<QString, QString> > queryItems(QUrl::ComponentFormattingOptions encoding = QUrl::PrettyDecoded) const;

    bool hasQueryItem(const QString &key) const;
    void addQueryItem(const QString &key, const QString &value);
    void removeQueryItem(const QString &key);
    QString queryItemValue(const QString &key, QUrl::ComponentFormattingOptions encoding = QUrl::PrettyDecoded) const;
    QStringList allQueryItemValues(const QString &key, QUrl::ComponentFormattingOptions encoding = QUrl::PrettyDecoded) const;
    void removeAllQueryItems(const QString &key);

    static constexpr char16_t defaultQueryValueDelimiter() noexcept { return u'='; }
    static constexpr char16_t defaultQueryPairDelimiter() noexcept { return u'&'; }

private:
    friend Q_CORE_EXPORT bool comparesEqual(const QUrlQuery &lhs, const QUrlQuery &rhs);
    Q_DECLARE_EQUALITY_COMPARABLE_NON_NOEXCEPT(QUrlQuery)
    friend class QUrl;
    friend Q_CORE_EXPORT size_t qHash(const QUrlQuery &key, size_t seed) noexcept;
    QSharedDataPointer<QUrlQueryPrivate> d;

public:
    typedef QSharedDataPointer<QUrlQueryPrivate> DataPtr;
    inline DataPtr &data_ptr() { return d; }
};

Q_DECLARE_SHARED(QUrlQuery)

QT_END_NAMESPACE

#endif // QURLQUERY_H
