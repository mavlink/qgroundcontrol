// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default
#ifndef QQMLJSGLOBAL_P_H
#define QQMLJSGLOBAL_P_H

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

#include <QtCore/private/qglobal_p.h>

#ifndef QT_STATIC
#  if defined(QT_BUILD_QML_LIB)
#    define QML_PARSER_EXPORT Q_DECL_EXPORT
#  else
#    define QML_PARSER_EXPORT Q_DECL_IMPORT
#  endif
#else
#    define QML_PARSER_EXPORT
#endif

#endif // QQMLJSGLOBAL_P_H
