// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGTRANSFORM_P_H
#define QSGTRANSFORM_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QSharedPointer>
#include <QMatrix4x4>
#include <QtQuick/qtquickexports.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QSGTransform
{
public:
    void setMatrix(const QMatrix4x4 &matrix)
    {
        if (matrix.isIdentity())
            m_matrixPtr.clear();
        else
            m_matrixPtr = QSharedPointer<QMatrix4x4>::create(matrix);
        m_invertedPtr.clear();
    }

    QMatrix4x4 matrix() const
    {
        return m_matrixPtr ? *m_matrixPtr : m_identity;
    }

    bool isIdentity() const
    {
        return !m_matrixPtr;
    }

    bool operator==(const QMatrix4x4 &other) const
    {
        return m_matrixPtr ? (other == *m_matrixPtr) : other.isIdentity();
    }

    bool operator!=(const QMatrix4x4 &other) const
    {
        return !(*this == other);
    }

    bool operator==(const QSGTransform &other) const
    {
        return (m_matrixPtr == other.m_matrixPtr)
                || (m_matrixPtr && other.m_matrixPtr && *m_matrixPtr == *other.m_matrixPtr);
    }

    bool operator!=(const QSGTransform &other) const
    {
        return !(*this == other);
    }

    int compareTo(const QSGTransform &other) const
    {
        int diff = 0;
        if (m_matrixPtr != other.m_matrixPtr) {
            if (m_matrixPtr.isNull()) {
                diff = -1;
            } else if (other.m_matrixPtr.isNull()) {
                diff = 1;
            } else {
                const float *ptr1 = m_matrixPtr->constData();
                const float *ptr2 = other.m_matrixPtr->constData();
                for (int i = 0; i < 16 && !diff; i++) {
                    float d = ptr1[i] - ptr2[i];
                    if (d != 0)
                        diff = (d > 0) ? 1 : -1;
                }
            }
        }
        return diff;
    }

    const float *invertedData() const
    {
        if (!m_matrixPtr)
            return m_identity.constData();
        if (!m_invertedPtr)
            m_invertedPtr = QSharedPointer<QMatrix4x4>::create(m_matrixPtr->inverted());
        return m_invertedPtr->constData();
    }

private:
    static QMatrix4x4 m_identity;
    QSharedPointer<QMatrix4x4> m_matrixPtr;
    mutable QSharedPointer<QMatrix4x4> m_invertedPtr;
};

QT_END_NAMESPACE

#endif // QSGTRANSFORM_P_H
