// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QIMAGESCALE_P_H
#define QIMAGESCALE_P_H

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

#include <qimage.h>
#include <private/qglobal_p.h>

QT_BEGIN_NAMESPACE

/*
  This version accepts only supported formats.
*/
QImage qSmoothScaleImage(const QImage &img, int w, int h);

namespace QImageScale {
    struct QImageScaleInfo {
        int *xpoints{nullptr};
        const unsigned int **ypoints{nullptr};
        int *xapoints{nullptr};
        int *yapoints{nullptr};
        int xup_yup{0};
        int sh = 0;
        int sw = 0;
    };
}

QT_END_NAMESPACE

#endif
