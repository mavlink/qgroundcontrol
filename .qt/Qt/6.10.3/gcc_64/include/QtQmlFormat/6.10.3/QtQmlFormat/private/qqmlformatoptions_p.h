// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLFORMATOPTIONS_P_H
#define QQMLFORMATOPTIONS_P_H

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

#include <QtCore/qstring.h>
#include <QtQmlDom/private/qqmldomoutwriter_p.h>
#include <QtQmlDom/private/qqmldomlinewriter_p.h>
#include <QtQmlToolingSettings/private/qqmltoolingsettings_p.h>

#include "qqmlformatsettings_p.h"
#include <bitset>

QT_BEGIN_NAMESPACE

enum QQmlFormatOptionLineEndings {
    Native,
    Windows,
    Unix,
    OldMacOs,
};

class QQmlFormatOptions
{
public:
    QQmlFormatOptions();

    using LineEndings = QQmlJS::Dom::LineWriterOptions::LineEndings;
    using AttributesSequence = QQmlJS::Dom::LineWriterOptions::AttributesSequence;

    static LineEndings detectLineEndings(const QString &code);

    static LineEndings lineEndings(QQmlFormatOptionLineEndings endings, const QString &code)
    {
        switch (endings) {
        case Native:
            return detectLineEndings(code);
        case OldMacOs:
            return LineEndings::OldMacOs;
        case Windows:
            return LineEndings::Windows;
        case Unix:
            return LineEndings::Unix;
        }
        Q_UNREACHABLE_RETURN(LineEndings::Unix);
    }

    bool tabsEnabled() const { return m_options.formatOptions.useTabs; }
    void setTabsEnabled(bool tabs) { m_options.formatOptions.useTabs = tabs; }
    bool normalizeEnabled() const
    {
        return m_options.attributesSequence == AttributesSequence::Normalize;
    }
    void setNormalizeEnabled(bool normalize)
    {
        m_options.attributesSequence =
                (normalize ? AttributesSequence::Normalize : AttributesSequence::Preserve);
    }
    bool objectsSpacing() const { return m_options.objectsSpacing; }
    void setObjectsSpacing(bool spacing) { m_options.objectsSpacing = spacing; }
    bool functionsSpacing() const { return m_options.functionsSpacing; }
    void setFunctionsSpacing(bool spacing) { m_options.functionsSpacing = spacing; }

    bool sortImports() const { return m_options.sortImports; }
    void setSortImports(bool sort) { m_options.sortImports = sort; }

    int indentWidth() const { return m_options.formatOptions.indentSize; }
    void setIndentWidth(int width) { m_options.formatOptions.indentSize = width; }

    int maxColumnWidth() const { return m_options.maxLineLength; }
    void setMaxColumnWidth(int width) { m_options.maxLineLength = width; }
    bool isMaxColumnWidthSet() const { return m_options.maxLineLength > 0; }

    void setSemicolonRule(QQmlJS::Dom::LineWriterOptions::SemicolonRule rule)
    {
        m_options.semicolonRule = rule;
    }

    QQmlJS::Dom::LineWriterOptions::SemicolonRule semicolonRule() const
    {
        return m_options.semicolonRule;
    }

    QQmlJS::Dom::LineWriterOptions optionsForCode(const QString &code) const
    {
        QQmlJS::Dom::LineWriterOptions result = m_options;
        result.lineEndings = lineEndings(m_newline, code);
        return result;
    }

    static QQmlFormatOptionLineEndings parseEndings(const QString &endings);

    QQmlFormatOptionLineEndings newline() const { return m_newline; }
    void setNewline(const QQmlFormatOptionLineEndings &endings) { m_newline = endings; }

    QStringList files() const { return m_files; }
    void setFiles(const QStringList &newFiles) { m_files = newFiles; }
    QStringList arguments() const { return m_arguments; }
    void setArguments(const QStringList &newArguments) { m_arguments = newArguments; }
    bool isVerbose() const { return m_verbose; }
    void setIsVerbose(bool newVerbose) { m_verbose = newVerbose; }
    bool isValid() const { return m_errors.isEmpty(); }
    bool isInplace() const { return m_inplace; }
    void setIsInplace(bool newInplace) { m_inplace = newInplace; }
    bool forceEnabled() const { return m_force; }
    void setForceEnabled(bool newForce) { m_force = newForce; }
    bool ignoreSettingsEnabled() const { return m_ignoreSettings; }
    void setIgnoreSettingsEnabled(bool newIgnoreSettings) { m_ignoreSettings = newIgnoreSettings; }
    bool writeDefaultSettingsEnabled() const { return m_writeDefaultSettings; }
    void setWriteDefaultSettingsEnabled(bool newWriteDefaultSettings)
    {
        m_writeDefaultSettings = newWriteDefaultSettings;
    }

    bool indentWidthSet() const { return m_indentWidthSet; }
    void setIndentWidthSet(bool newIndentWidthSet) { m_indentWidthSet = newIndentWidthSet; }
    QStringList errors() const { return m_errors; }
    void addError(const QString &newError) { m_errors.append(newError); };

    void applySettings(const QQmlFormatSettings &settings);
    static QQmlFormatOptions buildCommandLineOptions(const QStringList &args);
    QQmlFormatOptions optionsForFile(const QString &fileName, QQmlFormatSettings *settings) const;

    // Set of options that can be also passed by settings file.
    // We need to know if the option was set by command line
    enum Settings {
        UseTabs = 0,
        IndentWidth,
        MaxColumnWidth,
        NormalizeOrder,
        NewlineType,
        ObjectsSpacing,
        FunctionsSpacing,
        SortImports,
        SemicolonRule,
        SettingsCount
    };

private:
    // Command line options have the precedence over the values in the .ini file.
    // Mark them if they are set by command line then don't override the options
    // with the values in the .ini file.
    void mark(Settings setting) { m_settingBits.set(setting, true); }
    bool isMarked(Settings setting) const { return m_settingBits.test(setting); }

private:
    QQmlJS::Dom::LineWriterOptions m_options;

    QQmlFormatOptionLineEndings m_newline = Native;

    QStringList m_files;
    QStringList m_arguments;
    QStringList m_errors;

    bool m_verbose = false;
    bool m_inplace = false;
    bool m_force = false;
    bool m_ignoreSettings = false;
    bool m_writeDefaultSettings = false;
    bool m_indentWidthSet = false;
    std::bitset<SettingsCount> m_settingBits;
};

QT_END_NAMESPACE

#endif // QQMLFORMATOPTIONS_P_H
