// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLTYPEMODULEVERSION_P_H
#define QQMLTYPEMODULEVERSION_P_H

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

#include <QtQml/qtqmlglobal.h>
#include <QtQml/private/qqmltype_p.h>
#include <QtQml/private/qqmltypemodule_p.h>
#include <QtCore/qversionnumber.h>

QT_BEGIN_NAMESPACE

class QQmlTypeModule;
class QQmlType;
class QHashedStringRef;

namespace QV4 {
struct String;
}

class QQmlTypeModuleVersion
{
public:
    QQmlTypeModuleVersion();
    QQmlTypeModuleVersion(QQmlTypeModule *, QTypeRevision);
    QQmlTypeModuleVersion(const QQmlTypeModuleVersion &);
    QQmlTypeModuleVersion &operator=(const QQmlTypeModuleVersion &);

    template<typename String>
    QQmlType type(String name) const
    {
        if (!m_module)
            return QQmlType();
        return m_module->type(name, QTypeRevision::isValidSegment(m_minor)
                              ? QTypeRevision::fromMinorVersion(m_minor)
                              : QTypeRevision());
    }

private:
    QQmlTypeModule *m_module;
    quint8 m_minor;
};

QT_END_NAMESPACE

#endif // QQMLTYPEMODULEVERSION_P_H
