// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKLABSPLATFORMCOLORDIALOG_P_H
#define QQUICKLABSPLATFORMCOLORDIALOG_P_H

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

#include "qquicklabsplatformdialog_p.h"
#include <QtGui/qcolor.h>
#include <QtQml/qqml.h>

#if QT_DEPRECATED_SINCE(6, 9)

QT_BEGIN_NAMESPACE

class QQuickLabsPlatformColorDialog : public QQuickLabsPlatformDialog
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ColorDialog)
    QML_EXTENDED_NAMESPACE(QColorDialogOptions)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged FINAL)
    Q_PROPERTY(QColor currentColor READ currentColor WRITE setCurrentColor NOTIFY currentColorChanged FINAL)
    Q_PROPERTY(QColorDialogOptions::ColorDialogOptions options READ options WRITE setOptions NOTIFY optionsChanged FINAL)

public:
    explicit QQuickLabsPlatformColorDialog(QObject *parent = nullptr);

    QColor color() const;
    void setColor(const QColor &color);

    QColor currentColor() const;
    void setCurrentColor(const QColor &color);

    QColorDialogOptions::ColorDialogOptions options() const;
    void setOptions(QColorDialogOptions::ColorDialogOptions options);

Q_SIGNALS:
    void colorChanged();
    void currentColorChanged();
    void optionsChanged();

protected:
    bool useNativeDialog() const override;
    void onCreate(QPlatformDialogHelper *dialog) override;
    void onShow(QPlatformDialogHelper *dialog) override;
    void accept() override;

private:
    QColor m_color;
    QColor m_currentColor; // TODO: QColorDialogOptions::initialColor
    QSharedPointer<QColorDialogOptions> m_options;
};

QT_END_NAMESPACE

#endif // QT_DEPRECATED_SINCE(6, 9)

#endif // QQUICKLABSPLATFORMCOLORDIALOG_P_H
