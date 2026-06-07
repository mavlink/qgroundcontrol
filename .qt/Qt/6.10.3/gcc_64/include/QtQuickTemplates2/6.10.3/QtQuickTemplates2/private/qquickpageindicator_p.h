// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKPAGEINDICATOR_P_H
#define QQUICKPAGEINDICATOR_P_H

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

#include <QtQuickTemplates2/private/qquickcontrol_p.h>

QT_BEGIN_NAMESPACE

class QQmlComponent;
class QQuickPageIndicatorPrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickPageIndicator : public QQuickControl
{
    Q_OBJECT
    Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged FINAL)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged FINAL)
    Q_PROPERTY(bool interactive READ isInteractive WRITE setInteractive NOTIFY interactiveChanged FINAL)
    Q_PROPERTY(QQmlComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged FINAL)
    QML_NAMED_ELEMENT(PageIndicator)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickPageIndicator(QQuickItem *parent = nullptr);
    ~QQuickPageIndicator() override;

    int count() const;
    void setCount(int count);

    int currentIndex() const;
    void setCurrentIndex(int index);

    bool isInteractive() const;
    void setInteractive(bool interactive);

    QQmlComponent *delegate() const;
    void setDelegate(QQmlComponent *delegate);

Q_SIGNALS:
    void countChanged();
    void currentIndexChanged();
    void interactiveChanged();
    void delegateChanged();

protected:
    void contentItemChange(QQuickItem *newItem, QQuickItem *oldItem) override;

#if QT_CONFIG(quicktemplates2_multitouch)
    void touchEvent(QTouchEvent *event) override;
#endif

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
#endif

private:
    Q_DISABLE_COPY(QQuickPageIndicator)
    Q_DECLARE_PRIVATE(QQuickPageIndicator)
};

QT_END_NAMESPACE

#endif // QQUICKPAGEINDICATOR_P_H
