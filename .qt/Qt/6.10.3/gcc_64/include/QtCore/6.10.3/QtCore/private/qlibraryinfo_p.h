// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QLIBRARYINFO_P_H
#define QLIBRARYINFO_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include "QtCore/qlibraryinfo.h"
#include "QtCore/private/qglobal_p.h"

#if QT_CONFIG(settings)
#    include "QtCore/qsettings.h"
#endif
#include "QtCore/qstring.h"

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QLibraryInfoPrivate final
{
public:
#if QT_CONFIG(settings)
    static QSettings *configuration();
    static void reload();
    static void setQtconfManualPath(const QString *qtconfManualPath);
#endif

    struct LocationInfo
    {
        QString key;
        QString defaultValue;
        QString fallbackKey;
    };

    static LocationInfo locationInfo(QLibraryInfo::LibraryPath loc);

    enum UsageMode {
        RegularUsage,
        UsedFromQtBinDir
    };

    static QString path(QLibraryInfo::LibraryPath p, UsageMode usageMode = RegularUsage);
    static QList<QString> paths(QLibraryInfo::LibraryPath p, UsageMode usageMode = RegularUsage);
};

QT_END_NAMESPACE

#endif // QLIBRARYINFO_P_H
