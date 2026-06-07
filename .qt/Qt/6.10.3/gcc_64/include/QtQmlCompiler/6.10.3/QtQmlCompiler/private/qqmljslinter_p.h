// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QMLJSLINTER_P_H
#define QMLJSLINTER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <qtqmlcompilerexports.h>

#include <QtQmlCompiler/private/qqmljslogger_p.h>
#include <QtQmlCompiler/private/qqmljsimporter_p.h>
#include <QtQmlCompiler/private/qqmljscontextproperties_p.h>

#include <QtQml/private/qqmljssourcelocation_p.h>

#include <QtCore/qjsonarray.h>
#include <QtCore/qstring.h>
#include <QtCore/qmap.h>
#include <QtCore/qscopedpointer.h>

#include <vector>

QT_BEGIN_NAMESPACE

class QPluginLoader;
struct QStaticPlugin;

namespace QQmlSA {
class LintPlugin;
}

class Q_QMLCOMPILER_EXPORT QQmlJSLinter
{
public:
    QQmlJSLinter(const QStringList &importPaths, const QStringList &extraPluginPaths = {},
                 bool useAbsolutePath = false);

    enum LintResult { FailedToOpen, FailedToParse, HasWarnings, HasErrors, LintSuccess };
    enum FixResult { NothingToFix, FixError, FixSuccess };

    class Q_QMLCOMPILER_EXPORT Plugin
    {
        Q_DISABLE_COPY(Plugin)
    public:
        Plugin() = default;
        Plugin(Plugin &&plugin) noexcept;

#if QT_CONFIG(library)
        Plugin(QString path);
#endif
        Plugin(const QStaticPlugin &plugin);
        ~Plugin();

        const QString &name() const { return m_name; }
        const QString &description() const { return m_description; }
        const QString &version() const { return m_version; }
        const QString &author() const { return m_author; }
        const QList<QQmlJS::LoggerCategory> categories() const
        {
            return m_categories;
        }
        bool isBuiltin() const { return m_isBuiltin; }
        bool isValid() const { return m_isValid; }
        bool isInternal() const
        {
            return m_isInternal;
        }

        bool isEnabled() const
        {
            return m_isEnabled;
        }
        void setEnabled(bool isEnabled)
        {
            m_isEnabled = isEnabled;
        }

    private:
        friend class QQmlJSLinter;

        bool parseMetaData(const QJsonObject &metaData, QString pluginName);

        QString m_name;
        QString m_description;
        QString m_version;
        QString m_author;

        QList<QQmlJS::LoggerCategory> m_categories;
        QQmlSA::LintPlugin *m_instance;
        std::unique_ptr<QPluginLoader> m_loader;
        bool m_isBuiltin = false;
        bool m_isInternal =
                false; // Internal plugins are those developed and maintained inside the Qt project
        bool m_isValid = false;
        bool m_isEnabled = true;
    };

    static std::vector<Plugin> loadPlugins(QStringList paths);

    LintResult lintFile(const QString &filename, const QString *fileContents, const bool silent,
                        QJsonArray *json, const QStringList &qmlImportPaths,
                        const QStringList &qmldirFiles, const QStringList &resourceFiles,
                        const QList<QQmlJS::LoggerCategory> &categories,
                        const QQmlJS::ContextProperties &contextProperties = {});

    LintResult lintModule(const QString &uri, const bool silent, QJsonArray *json,
                          const QStringList &qmlImportPaths, const QStringList &resourceFiles);

    FixResult applyFixes(QString *fixedCode, bool silent);

    const QQmlJSLogger *logger() const { return m_logger.get(); }

    std::vector<Plugin> &plugins()
    {
        return m_plugins;
    }
    void setPlugins(std::vector<Plugin> plugins) { m_plugins = std::move(plugins); }

    void setPluginsEnabled(bool enablePlugins) { m_enablePlugins = enablePlugins; }
    bool pluginsEnabled() const { return m_enablePlugins; }

    void clearCache() { m_importer.clearCache(); }

private:
    void parseComments(QQmlJSLogger *logger, const QList<QQmlJS::SourceLocation> &comments);
    void processMessages(QJsonArray &warnings);

    bool m_useAbsolutePath;
    bool m_enablePlugins;
    QQmlJSImporter m_importer;
    QScopedPointer<QQmlJSLogger> m_logger;
    QString m_fileContents;
    std::vector<Plugin> m_plugins;
};

QT_END_NAMESPACE

#endif // QMLJSLINTER_P_H
