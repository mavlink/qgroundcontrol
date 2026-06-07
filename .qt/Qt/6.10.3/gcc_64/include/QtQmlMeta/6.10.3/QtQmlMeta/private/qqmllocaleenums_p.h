// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLLOCALEENUMS_H
#define QQMLLOCALEENUMS_H

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

#include <private/qtqmlglobal_p.h>
#include <private/qqmllocale_p.h>

#include <QtQmlMeta/qtqmlmetaexports.h>
#include <QtQml/qqml.h>

QT_REQUIRE_CONFIG(qml_locale);

QT_BEGIN_NAMESPACE

// Derive again so that we don't expose QQmlLocale as two different QML types
// as that would be bad style.
struct Q_QMLMETA_EXPORT QQmlLocaleEnums : public QQmlLocale
{
    Q_GADGET
};

// Use QML_FOREIGN_NAMESPACE so that we can expose QQmlLocaleEnums as a namespace
// rather than a value type.
namespace QQmlLocaleEnumsForeign
{
Q_NAMESPACE_EXPORT(Q_QMLMETA_EXPORT)
QML_NAMED_ELEMENT(Locale)
QML_ADDED_IN_VERSION(2, 2)
QML_FOREIGN_NAMESPACE(QQmlLocaleEnums)
};

QT_END_NAMESPACE

#endif // QQMLLOCALEENUMS_H

