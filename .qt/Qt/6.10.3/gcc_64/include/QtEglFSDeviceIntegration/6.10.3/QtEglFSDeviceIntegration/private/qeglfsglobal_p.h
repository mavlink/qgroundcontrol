// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QEGLFSGLOBAL_H
#define QEGLFSGLOBAL_H

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

#include <QtCore/qglobal.h>

#include <QtGui/private/qt_egl_p.h>

QT_BEGIN_NAMESPACE

#ifdef QT_BUILD_EGL_DEVICE_LIB
#define Q_EGLFS_EXPORT Q_DECL_EXPORT
#else
#define Q_EGLFS_EXPORT Q_DECL_IMPORT
#endif

QT_END_NAMESPACE

#endif
