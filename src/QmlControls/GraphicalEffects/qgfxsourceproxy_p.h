// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2016 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGFXSOURCEPROXY_P_H
#define QGFXSOURCEPROXY_P_H

#include <QtQuick/QQuickItem>
#include <QtQml/qqmlregistration.h>

#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QQuickShaderEffectSource;

class QGfxSourceProxy : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY(QQuickItem *input READ input WRITE setInput NOTIFY inputChanged RESET resetInput)
    Q_PROPERTY(QQuickItem *output READ output NOTIFY outputChanged)
    Q_PROPERTY(QRectF sourceRect READ sourceRect WRITE setSourceRect NOTIFY sourceRectChanged)

    Q_PROPERTY(bool active READ isActive NOTIFY activeChanged)
    Q_PROPERTY(Interpolation interpolation READ interpolation WRITE setInterpolation NOTIFY interpolationChanged)

    Q_ENUMS(Interpolation)

    QML_NAMED_ELEMENT(SourceProxy)
    QML_ADDED_IN_VERSION(5, 0)

public:
    enum Interpolation {
        AnyInterpolation,
        NearestInterpolation,
        LinearInterpolation
    };

    QGfxSourceProxy(QQuickItem *item = 0);
    ~QGfxSourceProxy();

    QQuickItem *input() const { return m_input; }
    void setInput(QQuickItem *input);
    void resetInput() { setInput(0); }

    QQuickItem *output() const { return m_output; }

    QRectF sourceRect() const { return m_sourceRect; }
    void setSourceRect(const QRectF &sourceRect);

    bool isActive() const { return m_output && m_output != m_input; }

    void setInterpolation(Interpolation i);
    Interpolation interpolation() const { return m_interpolation; }

protected:
    void updatePolish() override;

signals:
    void inputChanged();
    void outputChanged();
    void sourceRectChanged();
    void activeChanged();
    void interpolationChanged();

private slots:
    void repolish();

private:
    void setOutput(QQuickItem *output);
    void useProxy();
    static QObject *findLayer(QQuickItem *);

    QRectF m_sourceRect;
    QQuickItem *m_input;
    QQuickItem *m_output;
    QQuickShaderEffectSource *m_proxy;

    Interpolation m_interpolation;
};

QT_END_NAMESPACE

#endif // QGFXSOURCEPROXY_P_H
