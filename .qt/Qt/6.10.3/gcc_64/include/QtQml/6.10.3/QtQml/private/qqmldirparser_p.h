// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLDIRPARSER_P_H
#define QQMLDIRPARSER_P_H

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

#include <QtCore/QUrl>
#include <QtCore/QHash>
#include <QtCore/QDebug>
#include <QtCore/QTypeRevision>
#include <private/qtqmlcompilerglobal_p.h>
#include <private/qqmljsdiagnosticmessage_p.h>

QT_BEGIN_NAMESPACE

class QQmlEngine;
class Q_QML_COMPILER_EXPORT QQmlDirParser
{
public:
    void clear();
    bool parse(const QString &source);
    void disambiguateFileSelectors();

    bool hasError() const { return !_errors.isEmpty(); }
    void setError(const QQmlJS::DiagnosticMessage &);
    QList<QQmlJS::DiagnosticMessage> errors(const QString &uri) const;

    QString typeNamespace() const { return _typeNamespace; }
    void setTypeNamespace(const QString &s) { _typeNamespace = s; }

    static void checkNonRelative(const char *item, const QString &typeName, const QString &fileName)
    {
        if (fileName.startsWith(QLatin1Char('/'))) {
            qWarning() << item << typeName
                       << "is specified with non-relative URL" << fileName << "in a qmldir file."
                       << "URLs in qmldir files should be relative to the qmldir file's directory.";
        }
    }

    struct Plugin
    {
        Plugin() = default;

        Plugin(const QString &name, const QString &path, bool optional)
            : name(name), path(path), optional(optional)
        {
            checkNonRelative("Plugin", name, path);
        }

        QString name;
        QString path;
        bool optional = false;
    };

    struct Component
    {
        Component() = default;

        Component(const QString &typeName, const QString &fileName, QTypeRevision version)
            : typeName(typeName), fileName(fileName), version(version),
            internal(false), singleton(false)
        {
            checkNonRelative("Component", typeName, fileName);
        }

        QString typeName;
        QString fileName;
        QTypeRevision version = QTypeRevision::zero();
        bool internal = false;
        bool singleton = false;
    };

    struct Script
    {
        Script() = default;

        Script(const QString &nameSpace, const QString &fileName, QTypeRevision version)
            : nameSpace(nameSpace), fileName(fileName), version(version)
        {
            checkNonRelative("Script", nameSpace, fileName);
        }

        QString nameSpace;
        QString fileName;
        QTypeRevision version = QTypeRevision::zero();
    };

    struct Import
    {
        enum Flag {
            Default = 0x0,
            Auto = 0x1, // forward the version of the importing module
            Optional = 0x2, // is not automatically imported but only a tooling hint
            OptionalDefault =
                    0x4, // tooling hint only, denotes this entry should be imported by tooling
        };
        Q_DECLARE_FLAGS(Flags, Flag)

        Import() = default;
        Import(QString module, QTypeRevision version, Flags flags)
            : module(module), version(version), flags(flags)
        {
        }

        QString module;
        QTypeRevision version;     // invalid version is latest version, unless Flag::Auto
        Flags flags;

        friend bool operator==(const Import &a, const Import &b)
        {
            return a.module == b.module && a.version == b.version && a.flags == b.flags;
        }
    };

    QMultiHash<QString,Component> components() const { return _components; }
    QList<Import> dependencies() const { return _dependencies; }
    QList<Import> imports() const { return _imports; }
    QList<Script> scripts() const { return _scripts; }
    QList<Plugin> plugins() const { return _plugins; }
    bool designerSupported() const { return _designerSupported; }

    // A static module has side effects outside the mere importing of types. We shall not warn
    // about it being being "unused". The builtins are also a static module.
    bool isStaticModule() const { return _isStaticModule; }

    // A system module includes the JavaScript root object
    bool isSystemModule() const { return _isSystemModule; }

    QStringList typeInfos() const { return _typeInfos; }
    QStringList classNames() const { return _classNames; }
    QString preferredPath() const { return _preferredPath; }
    QString linkTarget() const { return _linkTarget; }

private:
    bool maybeAddComponent(const QString &typeName, const QString &fileName, const QString &version, QHash<QString,Component> &hash, int lineNumber = -1, bool multi = true);
    void reportError(quint16 line, quint16 column, const QString &message);
    QString scanQuotedWord(const QChar *&ch, quint16 lineNumber, quint16 columnNumber);
    void insertComponentOrScript(
            const QString &name, const QString &fileName, QTypeRevision version);

private:
    QList<QQmlJS::DiagnosticMessage> _errors;
    QString _typeNamespace;
    QString _preferredPath;
    QMultiHash<QString,Component> _components;
    QList<Import> _dependencies;
    QList<Import> _imports;
    QList<Script> _scripts;
    QList<Plugin> _plugins;
    bool _designerSupported = false;
    bool _isStaticModule = false;
    bool _isSystemModule = false;
    QStringList _typeInfos;
    QStringList _classNames;
    QString _linkTarget;
};

using QQmlDirComponents = QMultiHash<QString,QQmlDirParser::Component>;
using QQmlDirScripts = QList<QQmlDirParser::Script>;
using QQmlDirPlugins = QList<QQmlDirParser::Plugin>;
using QQmlDirImports = QList<QQmlDirParser::Import>;

QDebug &operator<< (QDebug &, const QQmlDirParser::Component &);
QDebug &operator<< (QDebug &, const QQmlDirParser::Script &);

QT_END_NAMESPACE

#endif // QQMLDIRPARSER_P_H
