// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGRHIINTERNALTEXTNODE_P_H
#define QSGRHIINTERNALTEXTNODE_P_H

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

#include <private/qsginternaltextnode_p.h>

QT_BEGIN_NAMESPACE

class QSGRhiInternalTextNode : public QSGInternalTextNode
{
public:
    QSGRhiInternalTextNode(QSGRenderContext *renderContext);
    void addDecorationNode(const QRectF &rect, const QColor &color) override;
};

QT_END_NAMESPACE

#endif // QSGRHIINTERNALTEXTNODE_P_H
