// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDESIGNEREXPORTWIDGET_H
#define QDESIGNEREXPORTWIDGET_H

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

#if 0
// pragma for syncqt, don't remove.
#pragma qt_class(QDesignerExportWidget)
#pragma qt_deprecates(QtDesigner/qdesignerexportwidget.h)
#pragma qt_deprecates(QtDesigner/QDesignerExportWidget)
#endif

#if defined(QDESIGNER_EXPORT_WIDGETS)
#  define QDESIGNER_WIDGET_EXPORT Q_DECL_EXPORT
#else
#  define QDESIGNER_WIDGET_EXPORT Q_DECL_IMPORT
#endif

QT_END_NAMESPACE

#endif //QDESIGNEREXPORTWIDGET_H
