// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKCOLORINPUTS_P_H
#define QQUICKCOLORINPUTS_P_H

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

#include <QtGui/qcolor.h>
#include <QtQmlModels/private/qqmldelegatemodel_p.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuickTemplates2/private/qquicktextfield_p.h>
#include <QtQuickTemplates2/private/qquickcontainer_p.h>

#include "qtquickdialogs2quickimplglobal_p.h"

#include "qquickcolordialogutils_p.h"

#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

class QQuickColorInputsPrivate;

class Q_QUICKDIALOGS2QUICKIMPL_EXPORT QQuickColorInputs : public QQuickContainer
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged FINAL)
    Q_PROPERTY(int red READ red NOTIFY colorChanged FINAL)
    Q_PROPERTY(int green READ green NOTIFY colorChanged FINAL)
    Q_PROPERTY(int blue READ blue NOTIFY colorChanged FINAL)
    Q_PROPERTY(qreal hue READ hue NOTIFY colorChanged FINAL)
    Q_PROPERTY(qreal hslSaturation READ hslSaturation NOTIFY colorChanged FINAL)
    Q_PROPERTY(qreal hsvSaturation READ hsvSaturation NOTIFY colorChanged FINAL)
    Q_PROPERTY(qreal value READ value NOTIFY colorChanged FINAL)
    Q_PROPERTY(qreal lightness READ lightness NOTIFY colorChanged FINAL)
    Q_PROPERTY(qreal alpha READ alpha NOTIFY colorChanged FINAL)
    Q_PROPERTY(bool showAlpha READ showAlpha WRITE setShowAlpha NOTIFY showAlphaChanged FINAL)
    Q_PROPERTY(Mode mode READ currentMode WRITE setCurrentMode NOTIFY currentModeChanged FINAL)
    Q_PROPERTY(QQmlComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged FINAL)
    QML_NAMED_ELEMENT(ColorInputsImpl)
    QML_ADDED_IN_VERSION(6, 9)

public:
    explicit QQuickColorInputs(QQuickItem *parent = nullptr);

    enum Mode {
        Hex = 0,
        Rgb,
        Hsv,
        Hsl
    };
    Q_ENUM(Mode);

    Mode currentMode() const;
    void setCurrentMode(Mode mode);

    QQmlComponent *delegate() const;
    void setDelegate(QQmlComponent *delegate);

    QColor color() const;
    void setColor(const QColor &c);
    int red() const;
    int green() const;
    int blue() const;
    qreal alpha() const;
    qreal hue() const;
    qreal hslSaturation() const;
    qreal hsvSaturation() const;
    qreal value() const;
    qreal lightness() const;

    bool showAlpha() const;
    void setShowAlpha(bool showAlpha);

Q_SIGNALS:
    void colorChanged(const QColor &c);
    void colorModified(const QColor &c);
    void hslChanged();
    void showAlphaChanged(bool);
    void currentModeChanged();
    void delegateChanged();

protected:
    void componentComplete() override;

private:
    Q_DISABLE_COPY(QQuickColorInputs)
    Q_DECLARE_PRIVATE(QQuickColorInputs)
};

QT_END_NAMESPACE

#endif // QQUICKCOLORINPUTS_P_H
