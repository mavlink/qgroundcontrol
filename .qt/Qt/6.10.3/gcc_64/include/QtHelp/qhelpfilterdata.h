// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QHELPFILTERDATA_H
#define QHELPFILTERDATA_H

#include <QtHelp/qhelp_global.h>

#include <QtCore/qshareddata.h>

QT_BEGIN_NAMESPACE

class QVersionNumber;
class QHelpFilterDataPrivate;

class QHELP_EXPORT QHelpFilterData final
{
public:
    QHelpFilterData();
    QHelpFilterData(const QHelpFilterData &other);
    QHelpFilterData(QHelpFilterData &&other);
    ~QHelpFilterData();

    QHelpFilterData &operator=(const QHelpFilterData &other);
    QHelpFilterData &operator=(QHelpFilterData &&other);
    bool operator==(const QHelpFilterData &other) const;

    void swap(QHelpFilterData &other) Q_DECL_NOTHROW
    { d.swap(other.d); }

    void setComponents(const QStringList &components);
    void setVersions(const QList<QVersionNumber> &versions);

    QStringList components() const;
    QList<QVersionNumber> versions() const;
private:
    QSharedDataPointer<QHelpFilterDataPrivate> d;
};

QT_END_NAMESPACE

#endif // QHELPFILTERDATA_H
