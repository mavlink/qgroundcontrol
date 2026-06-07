// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKDIALOGBUTTONBOX_P_H
#define QQUICKDIALOGBUTTONBOX_P_H

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
#include <QtQuickTemplates2/private/qquickcontainer_p.h>
#include <QtGui/qpa/qplatformdialoghelper.h>

QT_REQUIRE_CONFIG(quicktemplates2_container);

QT_BEGIN_NAMESPACE

class QQmlComponent;
class QQuickDialogButtonBoxPrivate;
class QQuickDialogButtonBoxAttached;
class QQuickDialogButtonBoxAttachedPrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickDialogButtonBox : public QQuickContainer
{
    Q_OBJECT
    Q_PROPERTY(Position position READ position WRITE setPosition NOTIFY positionChanged FINAL)
    Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment RESET resetAlignment NOTIFY alignmentChanged FINAL)
    Q_PROPERTY(QPlatformDialogHelper::StandardButtons standardButtons READ standardButtons WRITE setStandardButtons NOTIFY standardButtonsChanged FINAL)
    Q_PROPERTY(QQmlComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged FINAL)
    // 2.5 (Qt 5.12)
    Q_PROPERTY(QPlatformDialogHelper::ButtonLayout buttonLayout READ buttonLayout WRITE setButtonLayout RESET resetButtonLayout NOTIFY buttonLayoutChanged FINAL REVISION(2, 5))
    QML_NAMED_ELEMENT(DialogButtonBox)
    QML_ATTACHED(QQuickDialogButtonBoxAttached)
    QML_EXTENDED_NAMESPACE(QPlatformDialogHelper)
    QML_ADDED_IN_VERSION(2, 1)

public:
    explicit QQuickDialogButtonBox(QQuickItem *parent = nullptr);
    ~QQuickDialogButtonBox();

    enum Position {
        Header,
        Footer
    };
    Q_ENUM(Position)

    Position position() const;
    void setPosition(Position position);

    Qt::Alignment alignment() const;
    void setAlignment(Qt::Alignment alignment);
    void resetAlignment();

    QPlatformDialogHelper::StandardButtons standardButtons() const;
    void setStandardButtons(QPlatformDialogHelper::StandardButtons buttons);
    Q_INVOKABLE QQuickAbstractButton *standardButton(QPlatformDialogHelper::StandardButton button) const;

    QQmlComponent *delegate() const;
    void setDelegate(QQmlComponent *delegate);

    static QQuickDialogButtonBoxAttached *qmlAttachedProperties(QObject *object);

    QPlatformDialogHelper::ButtonLayout buttonLayout() const;
    void setButtonLayout(QPlatformDialogHelper::ButtonLayout layout);
    void resetButtonLayout();

Q_SIGNALS:
    void accepted();
    void rejected();
    void helpRequested();
    void clicked(QQuickAbstractButton *button);
    void positionChanged();
    void alignmentChanged();
    void standardButtonsChanged();
    void delegateChanged();
    // 2.3 (Qt 5.10)
    Q_REVISION(2, 3) void applied();
    Q_REVISION(2, 3) void reset();
    Q_REVISION(2, 3) void discarded();
    // 2.5 (Qt 5.12)
    Q_REVISION(2, 5) void buttonLayoutChanged();

protected:
    void updatePolish() override;
    void componentComplete() override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void contentItemChange(QQuickItem *newItem, QQuickItem *oldItem) override;
    bool isContent(QQuickItem *item) const override;
    void itemAdded(int index, QQuickItem *item) override;
    void itemRemoved(int index, QQuickItem *item) override;

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
#endif
    bool event(QEvent *e) override;

private:
    Q_DISABLE_COPY(QQuickDialogButtonBox)
    Q_DECLARE_PRIVATE(QQuickDialogButtonBox)
};

class Q_QUICKTEMPLATES2_EXPORT QQuickDialogButtonBoxAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickDialogButtonBox *buttonBox READ buttonBox NOTIFY buttonBoxChanged FINAL)
    Q_PROPERTY(QPlatformDialogHelper::ButtonRole buttonRole READ buttonRole WRITE setButtonRole NOTIFY buttonRoleChanged FINAL)

public:
    explicit QQuickDialogButtonBoxAttached(QObject *parent = nullptr);

    QQuickDialogButtonBox *buttonBox() const;

    QPlatformDialogHelper::ButtonRole buttonRole() const;
    void setButtonRole(QPlatformDialogHelper::ButtonRole role);

Q_SIGNALS:
    void buttonBoxChanged();
    void buttonRoleChanged();

private:
    Q_DISABLE_COPY(QQuickDialogButtonBoxAttached)
    Q_DECLARE_PRIVATE(QQuickDialogButtonBoxAttached)
};

QT_END_NAMESPACE

#endif // QQUICKDIALOGBUTTONBOX_P_H
