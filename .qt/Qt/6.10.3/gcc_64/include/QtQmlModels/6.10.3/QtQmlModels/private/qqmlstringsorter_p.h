// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLSTRINGSORTER_H
#define QQMLSTRINGSORTER_H

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

#include <QCollator>
#include <QtQmlModels/private/qqmlrolesorter_p.h>

QT_BEGIN_NAMESPACE

class QQmlSortFilterProxyModel;
class QQmlStringSorterPrivate;

class Q_QMLMODELS_EXPORT QQmlStringSorter : public QQmlRoleSorter
{
    Q_OBJECT
    Q_PROPERTY(Qt::CaseSensitivity caseSensitivity READ caseSensitivity WRITE setCaseSensitivity NOTIFY caseSensitivityChanged)
    Q_PROPERTY(bool ignorePunctuation READ ignorePunctuation WRITE setIgnorePunctuation NOTIFY ignorePunctuationChanged)
    Q_PROPERTY(QLocale locale READ locale WRITE setLocale NOTIFY localeChanged)
    Q_PROPERTY(bool numericMode READ numericMode WRITE setNumericMode NOTIFY numericModeChanged)
    QML_NAMED_ELEMENT(StringSorter)
    QML_ADDED_IN_VERSION(6, 10)

public:
    explicit QQmlStringSorter(QObject *parent = nullptr);
    ~QQmlStringSorter() = default;

    Qt::CaseSensitivity caseSensitivity() const;
    void setCaseSensitivity(Qt::CaseSensitivity caseSensitivity);

    bool ignorePunctuation() const;
    void setIgnorePunctuation(bool ignorePunctation);

    QLocale locale() const;
    void setLocale(const QLocale& locale);

    bool numericMode() const;
    void setNumericMode(bool numericMode);

    QPartialOrdering compare(const QModelIndex& sourceLeft, const QModelIndex& sourceRight, const QQmlSortFilterProxyModel *proxyModel) const override;

Q_SIGNALS:
    void caseSensitivityChanged();
    void ignorePunctuationChanged();
    void localeChanged();
    void numericModeChanged();

private:
    Q_DECLARE_PRIVATE(QQmlStringSorter)
};

class QQmlStringSorterPrivate : public QQmlRoleSorterPrivate
{
    Q_DECLARE_PUBLIC (QQmlStringSorter)

public:
    QCollator m_collator;
};

QT_END_NAMESPACE

#endif // QQMLSTRINGSORTER_H
