// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLLISTACCESSOR_H
#define QQMLLISTACCESSOR_H

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

#include <QtCore/QVariant>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QQmlEngine;
class Q_AUTOTEST_EXPORT QQmlListAccessor
{
public:
    QQmlListAccessor();
    ~QQmlListAccessor();

    QVariant list() const;
    void setList(const QVariant &);

    bool isValid() const;

    qsizetype count() const;
    QVariant at(qsizetype) const;
    void set(qsizetype, const QVariant &);

    enum Type {
        Invalid,
        StringList,
        UrlList,
        VariantList,
        ObjectList,
        ListProperty,
        Instance,
        Integer,
        ObjectSequence,
        ValueSequence,
    };

    Type type() const { return m_type; }

private:
    Type m_type;
    QMetaSequence m_metaSequence;
    QVariant d;
};

QT_END_NAMESPACE

#endif // QQMLLISTACCESSOR_H
