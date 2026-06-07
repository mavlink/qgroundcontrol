// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of Qt Designer.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#ifndef ICONLOADER_H
#define ICONLOADER_H

#include "shared_global_p.h"

#include <QtGui/qicon.h>

QT_BEGIN_NAMESPACE

class QString;
class QIcon;

namespace qdesigner_internal {

QDESIGNER_SHARED_EXPORT bool isDarkMode();
QDESIGNER_SHARED_EXPORT QIcon createIconSet(QStringView name);
QDESIGNER_SHARED_EXPORT QIcon createIconSet(QLatin1StringView name);
QDESIGNER_SHARED_EXPORT QIcon createIconSet(QIcon::ThemeIcon themeIcon,
                                            QLatin1StringView name);
QDESIGNER_SHARED_EXPORT QIcon emptyIcon();
QDESIGNER_SHARED_EXPORT QIcon qtLogoIcon();

} // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // ICONLOADER_H
