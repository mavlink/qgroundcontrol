// Copyright (C) 2016 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGFXSOURCEPROXY_P_H
#define QGFXSOURCEPROXY_P_H

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

#include <QtQuick/QQuickItem>
#include <QtCore/private/qglobal_p.h>
#include <QtCore/qrect.h>

QT_BEGIN_NAMESPACE

class QQuickShaderEffectSource;

class QGfxSourceProxyME : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QQuickItem *input READ input WRITE setInput NOTIFY inputChanged RESET resetInput FINAL)
    Q_PROPERTY(QQuickItem *output READ output NOTIFY outputChanged FINAL)
    Q_PROPERTY(QRectF sourceRect READ sourceRect WRITE setSourceRect NOTIFY sourceRectChanged FINAL)
    Q_PROPERTY(bool active READ isActive NOTIFY activeChanged FINAL)
    Q_PROPERTY(Interpolation interpolation READ interpolation WRITE setInterpolation NOTIFY interpolationChanged FINAL)

public:
    enum class Interpolation {
        Any,
        Nearest,
        Linear
    };
    Q_ENUM(Interpolation)

    QGfxSourceProxyME(QQuickItem *parentItem = nullptr);
    ~QGfxSourceProxyME();

    QQuickItem *input() const { return m_input; }
    void setInput(QQuickItem *input);
    void resetInput() { setInput(nullptr); }

    QQuickItem *output() const { return m_output; }

    QRectF sourceRect() const { return m_sourceRect; }
    void setSourceRect(const QRectF &sourceRect);

    bool isActive() const { return m_output && m_output != m_input; }

    void setInterpolation(Interpolation i);
    Interpolation interpolation() const { return m_interpolation; }

protected:
    void updatePolish() override;

Q_SIGNALS:
    void inputChanged();
    void outputChanged();
    void sourceRectChanged();
    void activeChanged();
    void interpolationChanged();

private Q_SLOTS:
    void repolish();

private:
    void setOutput(QQuickItem *output);
    void useProxy();
    static QObject *findLayer(QQuickItem *);

    QRectF m_sourceRect;
    QQuickItem *m_input = nullptr;
    QQuickItem *m_output = nullptr;
    QQuickShaderEffectSource *m_proxy = nullptr;

    Interpolation m_interpolation = Interpolation::Any;
};

QT_END_NAMESPACE

#endif // QGFXSOURCEPROXY_P_H
