// Copyright (C) 2024 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPDFOUTPUTINTENT_H
#define QPDFOUTPUTINTENT_H

#include <QtGui/qtguiglobal.h>

#ifndef QT_NO_PDF

#include <QtCore/qshareddata.h>

QT_BEGIN_NAMESPACE

class QString;
class QUrl;
class QColorSpace;

class QPdfOutputIntentPrivate;
QT_DECLARE_QESDP_SPECIALIZATION_DTOR_WITH_EXPORT(QPdfOutputIntentPrivate, Q_GUI_EXPORT)

class Q_GUI_EXPORT QPdfOutputIntent
{
public:
    QPdfOutputIntent();
    QPdfOutputIntent(const QPdfOutputIntent &other);
    QPdfOutputIntent(QPdfOutputIntent &&other) noexcept = default;
    QPdfOutputIntent &operator=(const QPdfOutputIntent &other);
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(QPdfOutputIntent)
    ~QPdfOutputIntent();

    void swap(QPdfOutputIntent &other) noexcept { d.swap(other.d); }

    QString outputConditionIdentifier() const;
    void setOutputConditionIdentifier(const QString &identifier);

    QString outputCondition() const;
    void setOutputCondition(const QString &condition);

    QUrl registryName() const;
    void setRegistryName(const QUrl &name);

    QColorSpace outputProfile() const;
    void setOutputProfile(const QColorSpace &profile);

private:
    QExplicitlySharedDataPointer<QPdfOutputIntentPrivate> d;
};

Q_DECLARE_SHARED(QPdfOutputIntent)

QT_END_NAMESPACE

#endif // QT_NO_PDF

#endif // QPDFOUTPUTINTENT_H
