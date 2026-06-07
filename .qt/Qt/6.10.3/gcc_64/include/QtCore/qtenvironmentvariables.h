// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTENVIRONMENTVARIABLES_H
#define QTENVIRONMENTVARIABLES_H

#include <QtCore/qtconfigmacros.h>
#include <QtCore/qtcoreexports.h>
#include <QtCore/qtdeprecationmarkers.h>
#include <QtCore/qtypes.h>

#include <optional>

#if 0
#pragma qt_class(QtEnvironmentVariables)
#pragma qt_sync_stop_processing
#endif

QT_BEGIN_NAMESPACE

class QByteArray;
class QByteArrayView;
class QString;

Q_CORE_EXPORT QByteArray qgetenv(const char *varName);
// need it as two functions because QString is only forward-declared here
Q_CORE_EXPORT QString qEnvironmentVariable(const char *varName);
Q_CORE_EXPORT QString qEnvironmentVariable(const char *varName, const QString &defaultValue);
#if QT_CORE_REMOVED_SINCE(6, 5)
Q_CORE_EXPORT bool qputenv(const char *varName, const QByteArray &value);
#endif
Q_CORE_EXPORT bool qputenv(const char *varName, QByteArrayView value);
Q_CORE_EXPORT bool qunsetenv(const char *varName);

Q_CORE_EXPORT bool qEnvironmentVariableIsEmpty(const char *varName) noexcept;
Q_CORE_EXPORT bool qEnvironmentVariableIsSet(const char *varName) noexcept;
Q_CORE_EXPORT int  qEnvironmentVariableIntValue(const char *varName, bool *ok=nullptr) noexcept;
Q_CORE_EXPORT std::optional<qint64> qEnvironmentVariableIntegerValue(const char *varName) noexcept;

QT_END_NAMESPACE

#endif /* QTENVIRONMENTVARIABLES_H */
