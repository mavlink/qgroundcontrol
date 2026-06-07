// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOLORTRANSFORM_P_H
#define QCOLORTRANSFORM_P_H

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

#include "qcolormatrix_p.h"
#include "qcolorspace_p.h"

#include <QtCore/qshareddata.h>
#include <QtGui/qrgbafloat.h>

QT_BEGIN_NAMESPACE
class QCmyk32;

class QColorTransformPrivate : public QSharedData
{
public:
    QColorMatrix colorMatrix; // Combined colorSpaceIn->toXyz and colorSpaceOut->toXyz.inverted()
    QExplicitlySharedDataPointer<const QColorSpacePrivate> colorSpaceIn;
    QExplicitlySharedDataPointer<const QColorSpacePrivate> colorSpaceOut;

    static QColorTransformPrivate *get(const QColorTransform &q)
    { return q.d.data(); }

    void updateLutsIn() const;
    void updateLutsOut() const;
    bool isIdentity() const;

    Q_GUI_EXPORT void prepare();
    enum TransformFlag {
        Unpremultiplied = 0,
        InputOpaque = 1,
        InputPremultiplied = 2,
        OutputPremultiplied = 4,
        Premultiplied = (InputPremultiplied | OutputPremultiplied)
    };
    Q_DECLARE_FLAGS(TransformFlags, TransformFlag)

    QColorVector map(QColorVector color) const;
    QColorVector mapExtended(QColorVector color) const;

    template<typename D, typename S>
    void apply(D *dst, const S *src, qsizetype count, TransformFlags flags) const;

private:
    void pcsAdapt(QColorVector *buffer, qsizetype len) const;
    template<typename S>
    void applyConvertIn(const S *src, QColorVector *buffer, qsizetype len, TransformFlags flags) const;
    template<typename D, typename S>
    void applyConvertOut(D *dst, const S *src, QColorVector *buffer, qsizetype len, TransformFlags flags) const;
};

QT_END_NAMESPACE

#endif // QCOLORTRANSFORM_P_H
