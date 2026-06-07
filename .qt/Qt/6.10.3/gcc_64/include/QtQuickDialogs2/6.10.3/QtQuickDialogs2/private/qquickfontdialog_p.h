// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKFONTDIALOG_P_H
#define QQUICKFONTDIALOG_P_H

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

#include <QtGui/qfont.h>

#include "qquickabstractdialog_p.h"

QT_BEGIN_NAMESPACE

class Q_QUICKDIALOGS2_EXPORT QQuickFontDialog : public QQuickAbstractDialog
{
    Q_OBJECT
    Q_PROPERTY(QFont selectedFont READ selectedFont WRITE setSelectedFont NOTIFY selectedFontChanged)
    Q_PROPERTY(QFont currentFont READ currentFont WRITE setCurrentFont NOTIFY currentFontChanged FINAL)
    Q_PROPERTY(QFontDialogOptions::FontDialogOptions options READ options WRITE setOptions
               RESET resetOptions NOTIFY optionsChanged)
    QML_EXTENDED_NAMESPACE(QFontDialogOptions)
    QML_NAMED_ELEMENT(FontDialog)
    QML_ADDED_IN_VERSION(6, 2)

public:
    explicit QQuickFontDialog(QObject *parent = nullptr);

    void setCurrentFont(const QFont &font);
    QFont currentFont() const;

    void setSelectedFont(const QFont &font);
    QFont selectedFont() const;

    QFontDialogOptions::FontDialogOptions options() const;
    void setOptions(QFontDialogOptions::FontDialogOptions options);
    void resetOptions();

Q_SIGNALS:
    void selectedFontChanged();
    void currentFontChanged();
    void optionsChanged();

protected:
    bool useNativeDialog() const override;
    void onCreate(QPlatformDialogHelper *dialog) override;
    void onShow(QPlatformDialogHelper *dialog) override;

private:
    QSharedPointer<QFontDialogOptions> m_options;
    QFont m_selectedFont;
};

QT_END_NAMESPACE

#endif // QQUICKFONTDIALOG_P_H
