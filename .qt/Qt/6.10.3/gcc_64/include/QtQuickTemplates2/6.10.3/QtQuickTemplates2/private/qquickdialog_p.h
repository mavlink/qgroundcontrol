// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKDIALOG_P_H
#define QQUICKDIALOG_P_H

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

#include <QtQuickTemplates2/private/qquickabstractbutton_p.h>
#include <QtQuickTemplates2/private/qquickpopup_p.h>
#include <QtGui/qpa/qplatformdialoghelper.h>

QT_REQUIRE_CONFIG(quicktemplates2_container);

QT_BEGIN_NAMESPACE

class QQuickDialogPrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickDialog : public QQuickPopup
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged FINAL)
    Q_PROPERTY(QQuickItem *header READ header WRITE setHeader NOTIFY headerChanged FINAL)
    Q_PROPERTY(QQuickItem *footer READ footer WRITE setFooter NOTIFY footerChanged FINAL)
    Q_PROPERTY(QPlatformDialogHelper::StandardButtons standardButtons READ standardButtons WRITE setStandardButtons NOTIFY standardButtonsChanged FINAL)
    // 2.3 (Qt 5.10)
    Q_PROPERTY(int result READ result WRITE setResult NOTIFY resultChanged FINAL REVISION(2, 3))
    QML_EXTENDED_NAMESPACE(QPlatformDialogHelper)
    // 2.5 (Qt 5.12)
    Q_PROPERTY(qreal implicitHeaderWidth READ implicitHeaderWidth NOTIFY implicitHeaderWidthChanged FINAL REVISION(2, 5))
    Q_PROPERTY(qreal implicitHeaderHeight READ implicitHeaderHeight NOTIFY implicitHeaderHeightChanged FINAL REVISION(2, 5))
    Q_PROPERTY(qreal implicitFooterWidth READ implicitFooterWidth NOTIFY implicitFooterWidthChanged FINAL REVISION(2, 5))
    Q_PROPERTY(qreal implicitFooterHeight READ implicitFooterHeight NOTIFY implicitFooterHeightChanged FINAL REVISION(2, 5))
    QML_NAMED_ELEMENT(Dialog)
    QML_ADDED_IN_VERSION(2, 1)

public:
    explicit QQuickDialog(QObject *parent = nullptr);
    ~QQuickDialog();

    QString title() const;
    void setTitle(const QString &title);

    QQuickItem *header() const;
    void setHeader(QQuickItem *header);

    QQuickItem *footer() const;
    void setFooter(QQuickItem *footer);

    QPlatformDialogHelper::StandardButtons standardButtons() const;
    void setStandardButtons(QPlatformDialogHelper::StandardButtons buttons);
    Q_REVISION(2, 3) Q_INVOKABLE QQuickAbstractButton *standardButton(QPlatformDialogHelper::StandardButton button) const;

    // 2.3 (Qt 5.10)
    enum StandardCode { Rejected, Accepted };
    Q_ENUM(StandardCode)

    int result() const;
    void setResult(int result);

    // 2.5 (Qt 5.12)
    qreal implicitHeaderWidth() const;
    qreal implicitHeaderHeight() const;

    qreal implicitFooterWidth() const;
    qreal implicitFooterHeight() const;

    void setOpacity(qreal opacity) override;

public Q_SLOTS:
    virtual void accept();
    virtual void reject();
    virtual void done(int result);

Q_SIGNALS:
    void accepted();
    void rejected();
    void titleChanged();
    void headerChanged();
    void footerChanged();
    void standardButtonsChanged();
    // 2.3 (Qt 5.10)
    Q_REVISION(2, 3) void applied();
    Q_REVISION(2, 3) void reset();
    Q_REVISION(2, 3) void discarded();
    Q_REVISION(2, 3) void helpRequested();
    Q_REVISION(2, 3) void resultChanged();
    // 2.5 (Qt 5.12)
    void implicitHeaderWidthChanged();
    void implicitHeaderHeightChanged();
    void implicitFooterWidthChanged();
    void implicitFooterHeightChanged();

protected:
    QQuickDialog(QQuickDialogPrivate &dd, QObject *parent);

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
    void accessibilityActiveChanged(bool active) override;
#endif

private:
    Q_DISABLE_COPY(QQuickDialog)
    Q_DECLARE_PRIVATE(QQuickDialog)
};

// The dialog options are registered here because they conceptually belong to QPlatformDialogHelper
// used as extension above. They may not be used QtQuick.Templates itself, but not registering them
// here would cause every downstream module that uses them to produce a redundant registration.

struct QColorDialogOptionsForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN_NAMESPACE(QColorDialogOptions)
};

struct QFileDialogOptionsForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN_NAMESPACE(QFileDialogOptions)
};

struct QFontDialogOptionsForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN_NAMESPACE(QFontDialogOptions)
};

QT_END_NAMESPACE

#endif // QQUICKDIALOG_P_H
