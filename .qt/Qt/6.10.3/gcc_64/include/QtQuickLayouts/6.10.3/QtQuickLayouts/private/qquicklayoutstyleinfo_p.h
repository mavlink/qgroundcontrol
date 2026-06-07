// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKLAYOUTSTYLEINFO_P_H
#define QQUICKLAYOUTSTYLEINFO_P_H

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

#include <QtGui/private/qabstractlayoutstyleinfo_p.h>

QT_BEGIN_NAMESPACE

class QQuickLayoutStyleInfo : public QAbstractLayoutStyleInfo
{
public:
    QQuickLayoutStyleInfo();

    qreal spacing(Qt::Orientation orientation) const override;
    qreal windowMargin(Qt::Orientation orientation) const override;
    bool hasChangedCore() const override;

};

QT_END_NAMESPACE

#endif // QQUICKLAYOUTSTYLEINFO_P_H
