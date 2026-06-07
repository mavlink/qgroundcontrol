// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKNATIVEICON_P_H
#define QQUICKNATIVEICON_P_H

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

#include <QtCore/qurl.h>
#include <QtCore/qstring.h>

#include <QtQml/qqmlengine.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QObject;

class QQuickNativeIcon
{
    Q_GADGET
    QML_ANONYMOUS
    Q_PROPERTY(QUrl source READ source WRITE setSource FINAL)
    Q_PROPERTY(QString name READ name WRITE setName FINAL)
    Q_PROPERTY(bool mask READ isMask WRITE setMask FINAL)

public:
    QUrl source() const;
    void setSource(const QUrl &source);

    QString name() const;
    void setName(const QString &name);

    bool isMask() const;
    void setMask(bool mask);

    bool operator==(const QQuickNativeIcon &other) const;
    bool operator!=(const QQuickNativeIcon &other) const;

private:
    bool m_mask = false;
    QUrl m_source;
    QString m_name;
};

QT_END_NAMESPACE

#endif // QQUICKNATIVEICON_P_H
