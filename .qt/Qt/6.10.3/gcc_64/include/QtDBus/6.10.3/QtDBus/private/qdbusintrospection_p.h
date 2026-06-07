// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QDBUSINTROSPECTION_P_H
#define QDBUSINTROSPECTION_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLibrary class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtDBus/private/qtdbusglobal_p.h>
#include <QtCore/qlist.h>
#include <QtCore/qmap.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

class Q_DBUS_EXPORT QDBusIntrospection
{
public:
    // forward declarations
    struct Argument;
    struct Method;
    struct Signal;
    struct Property;
    struct Interface;
    struct Object;
    struct ObjectTree;
    struct Annotation;

    // typedefs
    typedef QMap<QString, Annotation> Annotations;
    typedef QList<Argument> Arguments;
    typedef QMultiMap<QString, Method> Methods;
    typedef QMultiMap<QString, Signal> Signals;
    typedef QMap<QString, Property> Properties;
    typedef QMap<QString, QSharedDataPointer<Interface> > Interfaces;
    typedef QMap<QString, QSharedDataPointer<ObjectTree> > Objects;

public:
    // the structs

    // Line and column numbers have the same meaning as in QXmlStreamReader.
    struct SourceLocation
    {
        qint64 lineNumber = 1;
        qint64 columnNumber = 0;
    };

    class Q_DBUS_EXPORT DiagnosticsReporter
    {
        Q_DISABLE_COPY_MOVE(DiagnosticsReporter)
    public:
        DiagnosticsReporter() = default;
        virtual ~DiagnosticsReporter();
        virtual void warning(const SourceLocation &location, const char *msg, ...)
                Q_ATTRIBUTE_FORMAT_PRINTF(3, 4) = 0;
        virtual void error(const SourceLocation &location, const char *msg, ...)
                Q_ATTRIBUTE_FORMAT_PRINTF(3, 4) = 0;
    };

    struct Annotation
    {
        SourceLocation location;
        QString name;
        QString value;

        inline bool operator==(const Annotation &other) const
        {
            return name == other.name && value == other.value;
        }
    };

    struct Argument
    {
        SourceLocation location;
        QString type;
        QString name;

        inline bool operator==(const Argument& other) const
        { return name == other.name && type == other.type; }
    };

    struct Method
    {
        SourceLocation location;
        QString name;
        Arguments inputArgs;
        Arguments outputArgs;
        Annotations annotations;

        inline bool operator==(const Method& other) const
        { return name == other.name && annotations == other.annotations &&
                inputArgs == other.inputArgs && outputArgs == other.outputArgs; }
    };

    struct Signal
    {
        SourceLocation location;
        QString name;
        Arguments outputArgs;
        Annotations annotations;

        inline bool operator==(const Signal& other) const
        { return name == other.name && annotations == other.annotations &&
                outputArgs == other.outputArgs; }
    };

    struct Property
    {
        enum Access { Read, Write, ReadWrite };
        SourceLocation location;
        QString name;
        QString type;
        Access access;
        Annotations annotations;

        inline bool operator==(const Property& other) const
        { return access == other.access && name == other.name &&
                annotations == other.annotations && type == other.type; }
    };

    struct Interface: public QSharedData
    {
        SourceLocation location;
        QString name;
        QString introspection;

        Annotations annotations;
        Methods methods;
        Signals signals_;
        Properties properties;

        inline bool operator==(const Interface &other) const
        { return !name.isEmpty() && name == other.name; }
    };

    struct Object: public QSharedData
    {
        SourceLocation location;
        QString service;
        QString path;

        QStringList interfaces;
        QStringList childObjects;
    };

public:
    static Interface parseInterface(const QString &xml, DiagnosticsReporter *reporter = nullptr);
    static Interfaces parseInterfaces(const QString &xml, DiagnosticsReporter *reporter = nullptr);
    static Object parseObject(const QString &xml, const QString &service = QString(),
                              const QString &path = QString(),
                              DiagnosticsReporter *reporter = nullptr);

private:
    QDBusIntrospection();
};

QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif
