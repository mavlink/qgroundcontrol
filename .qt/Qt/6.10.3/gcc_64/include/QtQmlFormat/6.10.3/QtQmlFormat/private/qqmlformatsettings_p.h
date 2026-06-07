// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLFORMATSETTINGS_P_H
#define QQMLFORMATSETTINGS_P_H

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

#include <QtQmlToolingSettings/private/qqmltoolingsettings_p.h>

QT_BEGIN_NAMESPACE

class QQmlFormatSettings : public QQmlToolingSettings
{
public:
    QQmlFormatSettings(const QString &toolName = QLatin1String("qmlformat"));
    static const inline QLatin1StringView s_useTabsSetting = QLatin1String("UseTabs");
    static const inline QLatin1StringView s_indentWidthSetting = QLatin1String("IndentWidth");
    static const inline QLatin1StringView s_maxColumnWidthSetting = QLatin1String("MaxColumnWidth");
    static const inline QLatin1StringView s_normalizeSetting = QLatin1String("NormalizeOrder");
    static const inline QLatin1StringView s_newlineSetting = QLatin1String("NewlineType");
    static const inline QLatin1StringView s_objectsSpacingSetting = QLatin1String("ObjectsSpacing");
    static const inline QLatin1StringView s_functionsSpacingSetting = QLatin1String("FunctionsSpacing");
    static const inline QLatin1StringView s_sortImportsSetting = QLatin1String("SortImports");
    static const inline QLatin1StringView s_semiColonRuleSetting = QLatin1String("SemicolonRule");
};

QT_END_NAMESPACE
#endif // QQMLFORMATSETTINGS_P_H
