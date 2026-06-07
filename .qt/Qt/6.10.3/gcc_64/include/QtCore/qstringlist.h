// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtCore/qlist.h>

#ifndef QSTRINGLIST_H
#define QSTRINGLIST_H

#include <QtCore/qalgorithms.h>
#include <QtCore/qcontainertools_impl.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringmatcher.h>

QT_BEGIN_NAMESPACE

class QLatin1StringMatcher;
class QRegularExpression;

#if !defined(QT_NO_JAVA_STYLE_ITERATORS)
using QStringListIterator = QListIterator<QString>;
using QMutableStringListIterator = QMutableListIterator<QString>;
#endif


namespace QtPrivate {
    void Q_CORE_EXPORT QStringList_sort(QStringList *that, Qt::CaseSensitivity cs);
    qsizetype Q_CORE_EXPORT QStringList_removeDuplicates(QStringList *that);
    QString Q_CORE_EXPORT QStringList_join(const QStringList *that, QStringView sep);
    QString Q_CORE_EXPORT QStringList_join(const QStringList *that, const QChar *sep, qsizetype seplen);
    Q_CORE_EXPORT QString QStringList_join(const QStringList &list, QLatin1StringView sep);
    QStringList Q_CORE_EXPORT QStringList_filter(const QStringList *that, QStringView str,
                                               Qt::CaseSensitivity cs);
    Q_CORE_EXPORT QStringList QStringList_filter(const QStringList &that, QLatin1StringView needle,
                                                 Qt::CaseSensitivity cs);
    Q_CORE_EXPORT QStringList QStringList_filter(const QStringList &that,
                                                 const QStringMatcher &matcher);
    Q_CORE_EXPORT QStringList QStringList_filter(const QStringList &that,
                                                 const QLatin1StringMatcher &matcher);

    bool Q_CORE_EXPORT QStringList_contains(const QStringList *that, QStringView str, Qt::CaseSensitivity cs);
    bool Q_CORE_EXPORT QStringList_contains(const QStringList *that, QLatin1StringView str, Qt::CaseSensitivity cs);
    void Q_CORE_EXPORT QStringList_replaceInStrings(QStringList *that, QStringView before, QStringView after,
                                      Qt::CaseSensitivity cs);

    qsizetype Q_CORE_EXPORT QStringList_indexOf(const QStringList &that, QStringView str,
                                                qsizetype from, Qt::CaseSensitivity cs);
    qsizetype Q_CORE_EXPORT QStringList_indexOf(const QStringList &that, QLatin1StringView str,
                                                qsizetype from, Qt::CaseSensitivity cs);

    Q_CORE_EXPORT qsizetype QStringList_lastIndexOf(const QStringList &that, QStringView str,
                                                    qsizetype from, Qt::CaseSensitivity cs);
    Q_CORE_EXPORT qsizetype QStringList_lastIndexOf(const QStringList &that, QLatin1StringView str,
                                                    qsizetype from, Qt::CaseSensitivity cs);

#if QT_CONFIG(regularexpression)
    void Q_CORE_EXPORT QStringList_replaceInStrings(QStringList *that, const QRegularExpression &rx, const QString &after);
    QStringList Q_CORE_EXPORT QStringList_filter(const QStringList *that, const QRegularExpression &re);
    qsizetype Q_CORE_EXPORT QStringList_indexOf(const QStringList *that, const QRegularExpression &re, qsizetype from);
    qsizetype Q_CORE_EXPORT QStringList_lastIndexOf(const QStringList *that, const QRegularExpression &re, qsizetype from);
#endif // QT_CONFIG(regularexpression)
}

#ifdef Q_QDOC
class QStringList : public QList<QString>
#else
template <> struct QListSpecialMethods<QString> : QListSpecialMethodsBase<QString>
#endif
{
#ifdef Q_QDOC
public:
    using QList<QString>::QList;
    QStringList(const QString &str);
    QStringList(const QList<QString> &other);
    QStringList(QList<QString> &&other);

    QStringList &operator=(const QList<QString> &other);
    QStringList &operator=(QList<QString> &&other);
    QStringList operator+(const QStringList &other) const;
    QStringList &operator<<(const QString &str);
    QStringList &operator<<(const QStringList &other);
    QStringList &operator<<(const QList<QString> &other);
private:
#endif

public:
    inline void sort(Qt::CaseSensitivity cs = Qt::CaseSensitive)
    { QtPrivate::QStringList_sort(self(), cs); }
    inline qsizetype removeDuplicates()
    { return QtPrivate::QStringList_removeDuplicates(self()); }

    inline QString join(QStringView sep) const
    { return QtPrivate::QStringList_join(self(), sep); }
    inline QString join(QLatin1StringView sep) const
    { return QtPrivate::QStringList_join(*self(), sep); }
    inline QString join(QChar sep) const
    { return QtPrivate::QStringList_join(self(), &sep, 1); }

    QStringList filter(const QStringMatcher &matcher) const
    { return QtPrivate::QStringList_filter(*self(), matcher); }
    QStringList filter(const QLatin1StringMatcher &matcher) const
    { return QtPrivate::QStringList_filter(*self(), matcher); }

    QStringList filter(QLatin1StringView needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) const
    { return QtPrivate::QStringList_filter(*self(), needle, cs); }
    inline QStringList filter(QStringView str, Qt::CaseSensitivity cs = Qt::CaseSensitive) const
    { return QtPrivate::QStringList_filter(self(), str, cs); }
    inline QStringList &replaceInStrings(QStringView before, QStringView after, Qt::CaseSensitivity cs = Qt::CaseSensitive)
    {
        QtPrivate::QStringList_replaceInStrings(self(), before, after, cs);
        return *self();
    }

    inline QString join(const QString &sep) const
    { return QtPrivate::QStringList_join(self(), sep.constData(), sep.size()); }
    inline QStringList filter(const QString &str, Qt::CaseSensitivity cs = Qt::CaseSensitive) const
    { return QtPrivate::QStringList_filter(self(), str, cs); }
    inline QStringList &replaceInStrings(const QString &before, const QString &after, Qt::CaseSensitivity cs = Qt::CaseSensitive)
    {
        QtPrivate::QStringList_replaceInStrings(self(), before, after, cs);
        return *self();
    }
    inline QStringList &replaceInStrings(const QString &before, QStringView after, Qt::CaseSensitivity cs = Qt::CaseSensitive)
    {
        QtPrivate::QStringList_replaceInStrings(self(), before, after, cs);
        return *self();
    }
    inline QStringList &replaceInStrings(QStringView before, const QString &after, Qt::CaseSensitivity cs = Qt::CaseSensitive)
    {
        QtPrivate::QStringList_replaceInStrings(self(), before, after, cs);
        return *self();
    }
    using QListSpecialMethodsBase<QString>::contains;
    using QListSpecialMethodsBase<QString>::indexOf;
    using QListSpecialMethodsBase<QString>::lastIndexOf;

    inline bool contains(QLatin1StringView str, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::QStringList_contains(self(), str, cs); }
    inline bool contains(QStringView str, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::QStringList_contains(self(), str, cs); }

    inline bool contains(const QString &str, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::QStringList_contains(self(), str, cs); }

    qsizetype indexOf(const QString &str, qsizetype from = 0,
                      Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return indexOf(QStringView(str), from, cs); }
    qsizetype indexOf(QStringView needle, qsizetype from = 0,
                      Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::QStringList_indexOf(*self(), needle, from, cs); }
    qsizetype indexOf(QLatin1StringView needle, qsizetype from = 0,
                      Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::QStringList_indexOf(*self(), needle, from, cs); }

    qsizetype lastIndexOf(const QString &str, qsizetype from = -1,
                          Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return lastIndexOf(QStringView(str), from, cs); }
    qsizetype lastIndexOf(QStringView str, qsizetype from = -1,
                          Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::QStringList_lastIndexOf(*self(), str, from, cs); }
    qsizetype lastIndexOf(QLatin1StringView needle, qsizetype from = -1,
                          Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::QStringList_lastIndexOf(*self(), needle, from, cs); }

#if QT_CONFIG(regularexpression)
    inline QStringList filter(const QRegularExpression &re) const
    { return QtPrivate::QStringList_filter(self(), re); }
    inline QStringList &replaceInStrings(const QRegularExpression &re, const QString &after)
    {
        QtPrivate::QStringList_replaceInStrings(self(), re, after);
        return *self();
    }
    inline qsizetype indexOf(const QRegularExpression &re, qsizetype from = 0) const
    { return QtPrivate::QStringList_indexOf(self(), re, from); }
    inline qsizetype lastIndexOf(const QRegularExpression &re, qsizetype from = -1) const
    { return QtPrivate::QStringList_lastIndexOf(self(), re, from); }
#endif // QT_CONFIG(regularexpression)
};

QT_END_NAMESPACE

#endif // QSTRINGLIST_H
