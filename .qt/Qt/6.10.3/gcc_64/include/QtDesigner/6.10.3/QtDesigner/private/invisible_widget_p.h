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

#ifndef INVISIBLE_WIDGET_H
#define INVISIBLE_WIDGET_H

#include "shared_global_p.h"

#include <QtWidgets/qwidget.h>

QT_BEGIN_NAMESPACE

namespace qdesigner_internal {

class QDESIGNER_SHARED_EXPORT InvisibleWidget: public QWidget
{
    Q_OBJECT
public:
    InvisibleWidget(QWidget *parent = nullptr);
};

} // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // INVISIBLE_WIDGET_H
