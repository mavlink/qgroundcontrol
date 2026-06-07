// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOLORSPACE_P_H
#define QCOLORSPACE_P_H

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

#include "qcolorspace.h"
#include "qcolorclut_p.h"
#include "qcolormatrix_p.h"
#include "qcolortrc_p.h"
#include "qcolortrclut_p.h"

#include <QtCore/qmutex.h>
#include <QtCore/qpoint.h>
#include <QtCore/qshareddata.h>

#include <memory>

QT_BEGIN_NAMESPACE

bool qColorSpacePrimaryPointsAreValid(const QColorSpace::PrimaryPoints &primaries);

class QColorSpacePrivate : public QSharedData
{
public:
    QColorSpacePrivate();
    QColorSpacePrivate(QColorSpace::NamedColorSpace namedColorSpace);
    QColorSpacePrivate(QColorSpace::Primaries primaries, QColorSpace::TransferFunction transferFunction, float gamma);
    QColorSpacePrivate(QColorSpace::Primaries primaries, const QList<uint16_t> &transferFunctionTable);
    QColorSpacePrivate(const QColorSpace::PrimaryPoints &primaries, QColorSpace::TransferFunction transferFunction, float gamma);
    QColorSpacePrivate(const QColorSpace::PrimaryPoints &primaries, const QList<uint16_t> &transferFunctionTable);
    QColorSpacePrivate(const QColorSpace::PrimaryPoints &primaries,
                       const QList<uint16_t> &redTransferFunctionTable,
                       const QList<uint16_t> &greenTransferFunctionTable,
                       const QList<uint16_t> &blueRransferFunctionTable);
    QColorSpacePrivate(QPointF whitePoint, QColorSpace::TransferFunction transferFunction, float gamma);
    QColorSpacePrivate(QPointF whitePoint, const QList<uint16_t> &transferFunctionTable);
    QColorSpacePrivate(const QColorSpacePrivate &other) = default;

    static const QColorSpacePrivate *get(const QColorSpace &colorSpace)
    {
        return colorSpace.d_ptr.get();
    }

    static QColorSpacePrivate *get(QColorSpace &colorSpace)
    {
        return colorSpace.d_ptr.get();
    }

    bool equals(const QColorSpacePrivate *other) const;
    bool isValid() const noexcept;

    void initialize();
    void setToXyzMatrix();
    void setTransferFunction();
    void identifyColorSpace();
    void setTransferFunctionTable(const QList<uint16_t> &transferFunctionTable);
    void setTransferFunctionTables(const QList<uint16_t> &redTransferFunctionTable,
                                   const QList<uint16_t> &greenTransferFunctionTable,
                                   const QList<uint16_t> &blueTransferFunctionTable);
    QColorTransform transformationToColorSpace(const QColorSpacePrivate *out) const;
    QColorTransform transformationToXYZ() const;

    bool isThreeComponentMatrix() const;
    void clearElementListProcessingForEdit();

    static constexpr QColorSpace::NamedColorSpace Unknown = QColorSpace::NamedColorSpace(0);
    QColorSpace::NamedColorSpace namedColorSpace = Unknown;

    QColorSpace::Primaries primaries = QColorSpace::Primaries::Custom;
    QColorSpace::TransferFunction transferFunction = QColorSpace::TransferFunction::Custom;
    QColorSpace::TransformModel transformModel = QColorSpace::TransformModel::ThreeComponentMatrix;
    QColorSpace::ColorModel colorModel = QColorSpace::ColorModel::Undefined;
    float gamma = 0.0f;
    QColorVector whitePoint;

    // Three component matrix data:
    QColorTrc trc[3];
    QColorMatrix toXyz;
    QColorMatrix chad;

    // Element list processing data:
    struct TransferElement {
        QColorTrc trc[4];
    };
    using Element = std::variant<TransferElement, QColorMatrix, QColorVector, QColorCLUT>;
    bool isPcsLab = false;
    // A = device, B = PCS
    QList<Element> mAB, mBA;

    // Metadata
    QString description;
    QString userDescription;
    QByteArray iccProfile;

    // Cached tables for three component matrix transform:
    Q_CONSTINIT static QBasicMutex s_lutWriteLock;
    struct LUT {
        LUT() = default;
        ~LUT() = default;
        LUT(const LUT &other)
        {
            if (other.generated.loadAcquire()) {
                table[0] = other.table[0];
                table[1] = other.table[1];
                table[2] = other.table[2];
                generated.storeRelaxed(1);
            }
        }
        std::shared_ptr<QColorTrcLut> &operator[](int i) { return table[i]; }
        const std::shared_ptr<QColorTrcLut> &operator[](int i) const  { return table[i]; }
        std::shared_ptr<QColorTrcLut> table[3];
        QAtomicInt generated;
    } mutable lut;
};

QT_END_NAMESPACE

#endif // QCOLORSPACE_P_H
