// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKLABSPLATFORMFONTDIALOG_P_H
#define QQUICKLABSPLATFORMFONTDIALOG_P_H

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
#include <QtGui/qfont.h>
#include <QtQml/qqml.h>

#if QT_DEPRECATED_SINCE(6, 9)

QT_BEGIN_NAMESPACE

class QQuickLabsPlatformFontDialog : public QQuickLabsPlatformDialog
{
    Q_OBJECT
    QML_NAMED_ELEMENT(FontDialog)
    QML_EXTENDED_NAMESPACE(QFontDialogOptions)
    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged FINAL)
    Q_PROPERTY(QFont currentFont READ currentFont WRITE setCurrentFont NOTIFY currentFontChanged FINAL)
    Q_PROPERTY(QFontDialogOptions::FontDialogOptions options READ options WRITE setOptions NOTIFY optionsChanged FINAL)

public:
    explicit QQuickLabsPlatformFontDialog(QObject *parent = nullptr);

    QFont font() const;
    void setFont(const QFont &font);

    QFont currentFont() const;
    void setCurrentFont(const QFont &font);

    QFontDialogOptions::FontDialogOptions options() const;
    void setOptions(QFontDialogOptions::FontDialogOptions options);

Q_SIGNALS:
    void fontChanged();
    void currentFontChanged();
    void optionsChanged();

protected:
    bool useNativeDialog() const override;
    void onCreate(QPlatformDialogHelper *dialog) override;
    void onShow(QPlatformDialogHelper *dialog) override;
    void accept() override;

private:
    QFont m_font;
    QFont m_currentFont; // TODO: QFontDialogOptions::initialFont
    QSharedPointer<QFontDialogOptions> m_options;
};

QT_END_NAMESPACE

#endif // QT_DEPRECATED_SINCE(6, 9)

#endif // QQUICKLABSPLATFORMFONTDIALOG_P_H
