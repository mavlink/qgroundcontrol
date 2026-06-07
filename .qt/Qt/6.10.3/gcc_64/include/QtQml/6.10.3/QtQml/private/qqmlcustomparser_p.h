// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLCUSTOMPARSER_H
#define QQMLCUSTOMPARSER_H

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

#include <QtQml/qqmlerror.h>
#include <QtQml/private/qqmlbinding_p.h>
#include <private/qv4compileddata_p.h>

#include <QtCore/qbytearray.h>

QT_BEGIN_NAMESPACE

class QQmlPropertyValidator;
class QQmlEnginePrivate;

class Q_QML_EXPORT QQmlCustomParser
{
public:
    enum Flag {
        NoFlag                    = 0x00000000,
        AcceptsAttachedProperties = 0x00000001,
        AcceptsSignalHandlers     = 0x00000002
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    QQmlCustomParser() : engine(nullptr), validator(nullptr), m_flags(NoFlag) {}
    QQmlCustomParser(Flags f) : engine(nullptr), validator(nullptr), m_flags(f) {}
    virtual ~QQmlCustomParser() {}

    void clearErrors();
    Flags flags() const { return m_flags; }

    virtual void verifyBindings(
            const QQmlRefPointer<QV4::CompiledData::CompilationUnit> &,
            const QList<const QV4::CompiledData::Binding *> &) = 0;
    virtual void applyBindings(
            QObject *, const QQmlRefPointer<QV4::ExecutableCompilationUnit> &,
            const QList<const QV4::CompiledData::Binding *> &) = 0;

    QVector<QQmlError> errors() const { return exceptions; }

protected:
    void error(const QV4::CompiledData::Binding *binding, const QString& description)
    { error(binding->location, description); }
    void error(const QV4::CompiledData::Object *object, const QString& description)
    { error(object->location, description); }
    void error(const QV4::CompiledData::Location &location, const QString& description);

    int evaluateEnum(const QString &, bool *ok) const;

    const QMetaObject *resolveType(const QString&) const;
    QQmlTypeLoader *typeLoader() const;

private:
    QVector<QQmlError> exceptions;
    QQmlEnginePrivate *engine;
    const QQmlPropertyValidator *validator;
    Flags m_flags;
    QBiPointer<const QQmlImports, QQmlTypeNameCache> imports;
    friend class QQmlPropertyValidator;
    friend class QQmlObjectCreator;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(QQmlCustomParser::Flags)

QT_END_NAMESPACE

#endif
