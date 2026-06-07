// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKPOPUPANCHORS_P_H
#define QQUICKPOPUPANCHORS_P_H

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

#include <QtCore/qobject.h>
#include <QtQml/qqml.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuickTemplates2/private/qtquicktemplates2global_p.h>

QT_BEGIN_NAMESPACE

class QQuickItem;
class QQuickPopupAnchorsPrivate;
class QQuickPopup;

class Q_QUICKTEMPLATES2_EXPORT QQuickPopupAnchors : public QObject, public QQuickItemChangeListener
{
    Q_OBJECT
    Q_PROPERTY(QQuickItem *centerIn READ centerIn WRITE setCenterIn RESET resetCenterIn NOTIFY centerInChanged FINAL)
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 5)

public:
    explicit QQuickPopupAnchors(QQuickPopup *popup);
    ~QQuickPopupAnchors();

    QQuickItem *centerIn() const;
    void setCenterIn(QQuickItem *item);
    void resetCenterIn();

Q_SIGNALS:
    void centerInChanged();

private:
    void itemDestroyed(QQuickItem *item) override;

    Q_DISABLE_COPY(QQuickPopupAnchors)
    Q_DECLARE_PRIVATE(QQuickPopupAnchors)
};

QT_END_NAMESPACE

#endif // QQUICKPOPUPANCHORS_P_H
