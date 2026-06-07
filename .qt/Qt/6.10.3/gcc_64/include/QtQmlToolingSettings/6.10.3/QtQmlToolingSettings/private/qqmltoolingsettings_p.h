// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLTOOLINGSETTINGS_P_H
#define QQMLTOOLINGSETTINGS_P_H

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
#include <QtCore/qhash.h>
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

class QQmlToolingSettings
{
public:
    QQmlToolingSettings(const QString &toolName) : m_toolName(toolName) { }

    void addOption(const QString &name, const QVariant defaultValue = QVariant());

    bool writeDefaults() const;
    bool search(const QString &path);

    QVariant value(const QString &name) const;
    QStringList valueAsStringList(const QString &name) const;

    static void resolveRelativeImportPaths(const QString &filePath, QStringList *paths);
    QStringList valueAsAbsolutePathList(const QString &name,
                                        const QString &baseForRelativePaths) const;
    bool isSet(const QString &name) const;

private:
    QString m_toolName;
    QString m_currentSettingsPath;
    QHash<QString, QString> m_seenDirectories;
    QVariantHash m_values;
    QVariantHash m_savedValues;

    bool read(const QString &settingsFilePath);
};

QT_END_NAMESPACE

#endif // QQMLTOOLINGSETTINGS_P_H
