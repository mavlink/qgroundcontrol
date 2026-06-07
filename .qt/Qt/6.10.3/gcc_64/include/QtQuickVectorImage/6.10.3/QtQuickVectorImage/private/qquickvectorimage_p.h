// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKVECTORIMAGE_P_H
#define QQUICKVECTORIMAGE_P_H

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

#include <QQuickItem>
#include <QtQuick/private/qquickanimation_p.h>
#include <QtQuickVectorImage/qtquickvectorimageexports.h>

QT_BEGIN_NAMESPACE

class QQuickVectorImagePrivate;
class QQuickVectorImageAnimations;

class Q_QUICKVECTORIMAGE_EXPORT QQuickVectorImage : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(FillMode fillMode READ fillMode WRITE setFillMode NOTIFY fillModeChanged)
    Q_PROPERTY(RendererType preferredRendererType READ preferredRendererType WRITE setPreferredRendererType NOTIFY preferredRendererTypeChanged)
    Q_PROPERTY(QQuickVectorImageAnimations *animations READ animations CONSTANT REVISION(6, 10) FINAL)
    Q_PROPERTY(bool assumeTrustedSource READ assumeTrustedSource WRITE setAssumeTrustedSource NOTIFY assumeTrustedSourceChanged FINAL)
    QML_NAMED_ELEMENT(VectorImage)

public:
    enum FillMode {
        NoResize,
        PreserveAspectFit,
        PreserveAspectCrop,
        Stretch
    };
    Q_ENUM(FillMode)

    enum RendererType {
        GeometryRenderer,
        CurveRenderer
    };
    Q_ENUM(RendererType)

    QQuickVectorImage(QQuickItem *parent = nullptr);

    QUrl source() const;
    void setSource(const QUrl &source);

    FillMode fillMode() const;
    void setFillMode(FillMode newFillMode);

    RendererType preferredRendererType() const;
    void setPreferredRendererType(RendererType newPreferredRendererType);

    QQuickVectorImageAnimations *animations();

    bool assumeTrustedSource() const;
    void setAssumeTrustedSource(bool assumeTrustedSource);

    void componentComplete() override;

signals:
    void sourceChanged();
    void fillModeChanged();

    void preferredRendererTypeChanged();
    void assumeTrustedSourceChanged();

private slots:
    void updateRootItemScale();
    void updateAnimationProperties();

private:
    Q_DISABLE_COPY(QQuickVectorImage)
    Q_DECLARE_PRIVATE(QQuickVectorImage)
};

class Q_QUICKVECTORIMAGE_EXPORT QQuickVectorImageAnimations : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int loops READ loops WRITE setLoops NOTIFY loopsChanged FINAL)
    Q_PROPERTY(bool paused READ paused WRITE setPaused NOTIFY pausedChanged FINAL)

    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(6, 10)
public:
    QQuickVectorImageAnimations(QObject *parent = nullptr) : QObject(parent) {}

    int loops() const;

    void setLoops(int loops);

    bool paused() const;
    void setPaused(bool paused);

    Q_INVOKABLE void restart();

Q_SIGNALS:
    void loopsChanged();
    void enabledChanged();
    void pausedChanged();

private:
    int m_loops = 1;
    bool m_paused = false;
};

QT_END_NAMESPACE

#endif // QQUICKVECTORIMAGE_P_H

