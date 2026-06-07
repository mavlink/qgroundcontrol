// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QWAVEFRONTMESH_P_H
#define QWAVEFRONTMESH_P_H

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

#include "qqmlwavefrontmeshglobal_p.h"

#include <QtQuick/private/qquickshadereffectmesh_p.h>

#include <QtCore/qurl.h>
#include <QtGui/qvector3d.h>

QT_BEGIN_NAMESPACE

class QWavefrontMeshPrivate;
class Q_LABSWAVEFRONTMESH_EXPORT QWavefrontMesh : public QQuickShaderEffectMesh
{
    Q_OBJECT
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged FINAL)
    Q_PROPERTY(Error lastError READ lastError NOTIFY lastErrorChanged FINAL)
    Q_PROPERTY(QVector3D projectionPlaneV READ projectionPlaneV WRITE setProjectionPlaneV NOTIFY projectionPlaneVChanged FINAL)
    Q_PROPERTY(QVector3D projectionPlaneW READ projectionPlaneW WRITE setProjectionPlaneW NOTIFY projectionPlaneWChanged FINAL)
    QML_NAMED_ELEMENT(WavefrontMesh)
    QML_ADDED_IN_VERSION(1, 0)

public:
    enum Error {
        NoError,
        InvalidSourceError,
        UnsupportedFaceShapeError,
        UnsupportedIndexSizeError,
        FileNotFoundError,
        NoAttributesError,
        MissingPositionAttributeError,
        MissingTextureCoordinateAttributeError,
        MissingPositionAndTextureCoordinateAttributesError,
        TooManyAttributesError,
        InvalidPlaneDefinitionError
    };
    Q_ENUM(Error)

    QWavefrontMesh(QObject *parent = nullptr);
    ~QWavefrontMesh() override;

    QUrl source() const;
    void setSource(const QUrl &url);

    Error lastError() const;
    void setLastError(Error lastError);

    bool validateAttributes(const QList<QByteArray> &attributes, int *posIndex) override;
    QSGGeometry *updateGeometry(QSGGeometry *geometry, int attrCount, int posIndex,
                                const QRectF &srcRect, const QRectF &rect) override;
    QString log() const override;

    QVector3D projectionPlaneV() const;
    void setProjectionPlaneV(const QVector3D &projectionPlaneV);

    QVector3D projectionPlaneW() const;
    void setProjectionPlaneW(const QVector3D &projectionPlaneW);

Q_SIGNALS:
    void sourceChanged();
    void lastErrorChanged();
    void projectionPlaneVChanged();
    void projectionPlaneWChanged();

protected Q_SLOTS:
    void readData();

private:
    Q_DISABLE_COPY(QWavefrontMesh)
    Q_DECLARE_PRIVATE(QWavefrontMesh)
};

QT_END_NAMESPACE

#endif // QWAVEFRONTMESH_P_H

