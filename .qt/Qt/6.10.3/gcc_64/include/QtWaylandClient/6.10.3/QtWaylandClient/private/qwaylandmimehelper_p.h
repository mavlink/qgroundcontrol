// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDMIMEHELPER_H
#define QWAYLANDMIMEHELPER_H

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

#include <QString>
#include <QByteArray>
#include <QMimeData>
#include <private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QWaylandMimeHelper
{
public:
    static QByteArray getByteArray(QMimeData *mimeData, const QString &mimeType);
};

QT_END_NAMESPACE

#endif
