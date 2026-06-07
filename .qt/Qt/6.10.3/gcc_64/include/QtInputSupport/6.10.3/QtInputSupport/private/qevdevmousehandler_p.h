// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QEVDEVMOUSEHANDLER_P_H
#define QEVDEVMOUSEHANDLER_P_H

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

#include <QObject>
#include <QString>
#include <QPoint>
#include <QEvent>
#include <QLoggingCategory>

#include <private/qglobal_p.h>

#include <memory>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qLcEvdevMouse)

class QSocketNotifier;

class QEvdevMouseHandler : public QObject
{
    Q_OBJECT
public:
    static std::unique_ptr<QEvdevMouseHandler> create(const QString &device, const QString &specification);
    ~QEvdevMouseHandler();

    void readMouseData();

signals:
    void handleMouseEvent(int x, int y, bool abs, Qt::MouseButtons buttons,
                          Qt::MouseButton button, QEvent::Type type);
    void handleWheelEvent(QPoint delta);

private:
    QEvdevMouseHandler(const QString &device, int fd, bool abs, bool compression, int jitterLimit);

    void sendMouseEvent();
#ifndef Q_OS_VXWORKS
    bool getHardwareMaximum();
#endif
    void detectHiResWheelSupport();

    QString m_device;
    int m_fd;
    QSocketNotifier *m_notify = nullptr;
    int m_x = 0, m_y = 0;
    int m_prevx = 0, m_prevy = 0;
    bool m_abs;
    bool m_compression;
    bool m_hiResWheel = false;
    bool m_hiResHWheel = false;
    Qt::MouseButtons m_buttons;
    Qt::MouseButton m_button;
    QEvent::Type m_eventType;
    int m_jitterLimitSquared;
    bool m_prevInvalid = true;
    int m_hardwareWidth;
    int m_hardwareHeight;
    qreal m_hardwareScalerY;
    qreal m_hardwareScalerX;
};

QT_END_NAMESPACE

#endif // QEVDEVMOUSEHANDLER_P_H
