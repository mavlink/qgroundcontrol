// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKHOVERHANDLER_H
#define QQUICKHOVERHANDLER_H

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

#include <QtCore/qbasictimer.h>
#include <QtGui/qevent.h>
#include <QtQuick/qquickitem.h>

#include "qquicksinglepointhandler_p.h"

QT_BEGIN_NAMESPACE

class QQuickHoverHandlerPrivate;

class Q_QUICK_EXPORT QQuickHoverHandler : public QQuickSinglePointHandler
{
    Q_OBJECT
    Q_PROPERTY(bool hovered READ isHovered NOTIFY hoveredChanged)
    Q_PROPERTY(bool blocking READ isBlocking WRITE setBlocking NOTIFY blockingChanged REVISION(6, 3))
    QML_NAMED_ELEMENT(HoverHandler)
    QML_ADDED_IN_VERSION(2, 12)

public:
    explicit QQuickHoverHandler(QQuickItem *parent = nullptr);
    ~QQuickHoverHandler();

    bool event(QEvent *) override;

    bool isHovered() const { return m_hovered; }

    bool isBlocking() const { return m_blocking; }
    void setBlocking(bool blocking);

Q_SIGNALS:
    void hoveredChanged();
    Q_REVISION(6, 3) void blockingChanged();

protected:
    void componentComplete() override;
    bool wantsPointerEvent(QPointerEvent *event) override;
    void handleEventPoint(QPointerEvent *ev, QEventPoint &point) override;

    Q_DECLARE_PRIVATE(QQuickHoverHandler)

private:
    void setHovered(bool hovered);

private:
    bool m_hovered = false;
    bool m_hoveredTablet = false;
    bool m_blocking = false;
};

QT_END_NAMESPACE

#endif // QQUICKHOVERHANDLER_H
