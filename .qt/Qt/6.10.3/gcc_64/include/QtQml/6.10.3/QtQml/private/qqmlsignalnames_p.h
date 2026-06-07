// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLSIGNALANDPROPERTYNAMES_P_H
#define QQMLSIGNALANDPROPERTYNAMES_P_H

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

#include <cstddef>
#include <optional>

#include <QtQml/private/qtqmlglobal_p.h>
#include <QtCore/qstringview.h>
#include <QtCore/qstring.h>
#include <type_traits>

QT_BEGIN_NAMESPACE

class Q_QML_EXPORT QQmlSignalNames
{
public:
    static QString propertyNameToChangedSignalName(QStringView property);
    static QByteArray propertyNameToChangedSignalName(QUtf8StringView property);

    static QString propertyNameToChangedHandlerName(QStringView property);

    static QString signalNameToHandlerName(QAnyStringView signal);

    static std::optional<QString> changedSignalNameToPropertyName(QStringView changeSignal);
    static std::optional<QByteArray> changedSignalNameToPropertyName(QUtf8StringView changeSignal);

    static std::optional<QString> changedHandlerNameToPropertyName(QStringView handler);
    static std::optional<QByteArray> changedHandlerNameToPropertyName(QUtf8StringView handler);

    static std::optional<QString> handlerNameToSignalName(QStringView handler);
    static std::optional<QString> changedHandlerNameToSignalName(QStringView changedHandler);

    static bool isChangedHandlerName(QStringView signalName);
    static bool isChangedSignalName(QStringView signalName);
    static bool isHandlerName(QStringView signalName);

    static QString addPrefixToPropertyName(QStringView prefix, QStringView propertyName);

    // ### Qt7: remove this
    static std::optional<QString> badHandlerNameToSignalName(QStringView handler);
};

QT_END_NAMESPACE

#endif // QQMLSIGNALANDPROPERTYNAMES_P_H
