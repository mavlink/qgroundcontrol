// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOMPRESSEDHELPINFO_H
#define QCOMPRESSEDHELPINFO_H

#include <QtHelp/qhelp_global.h>

#include <QtCore/qshareddata.h>

QT_BEGIN_NAMESPACE

class QCompressedHelpInfoPrivate;
class QVersionNumber;

class QHELP_EXPORT QCompressedHelpInfo final
{
public:
    QCompressedHelpInfo();
    QCompressedHelpInfo(const QCompressedHelpInfo &other);
    QCompressedHelpInfo(QCompressedHelpInfo &&other);
    ~QCompressedHelpInfo();

    QCompressedHelpInfo &operator=(const QCompressedHelpInfo &other);
    QCompressedHelpInfo &operator=(QCompressedHelpInfo &&other);

    void swap(QCompressedHelpInfo &other) Q_DECL_NOTHROW
    { d.swap(other.d); }

    QString namespaceName() const;
    QString component() const;
    QVersionNumber version() const;
    bool isNull() const;

    static QCompressedHelpInfo fromCompressedHelpFile(const QString &documentationFileName);

private:
    QSharedDataPointer<QCompressedHelpInfoPrivate> d;
};

QT_END_NAMESPACE

#endif // QCOMPRESSEDHELPINFO_H
