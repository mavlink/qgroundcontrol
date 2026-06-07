// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKCOLORDIALOG_P_H
#define QQUICKCOLORDIALOG_P_H

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

#include "qquickabstractdialog_p.h"

#include <QtGui/qcolor.h>

QT_BEGIN_NAMESPACE

class Q_QUICKDIALOGS2_EXPORT QQuickColorDialog : public QQuickAbstractDialog
{
    Q_OBJECT
    Q_PROPERTY(QColor selectedColor READ selectedColor WRITE setSelectedColor NOTIFY selectedColorChanged)
    Q_PROPERTY(QColorDialogOptions::ColorDialogOptions options READ options WRITE setOptions RESET resetOptions NOTIFY optionsChanged)
    QML_EXTENDED_NAMESPACE(QColorDialogOptions)
    QML_NAMED_ELEMENT(ColorDialog)
    QML_ADDED_IN_VERSION(6, 4)

public:
    explicit QQuickColorDialog(QObject *parent = nullptr);

    QColor selectedColor() const;
    void setSelectedColor(const QColor &color);

    QColorDialogOptions::ColorDialogOptions options() const;
    void setOptions(QColorDialogOptions::ColorDialogOptions options);
    void resetOptions();

Q_SIGNALS:
    void selectedColorChanged();
    void optionsChanged();

protected:
    bool useNativeDialog() const override;
    void onCreate(QPlatformDialogHelper *dialog) override;
    void onShow(QPlatformDialogHelper *dialog) override;

private:
    QSharedPointer<QColorDialogOptions> m_options;
    QColor m_selectedColor;
};

QT_END_NAMESPACE

#endif // QQUICKCOLORDIALOG_P_H
