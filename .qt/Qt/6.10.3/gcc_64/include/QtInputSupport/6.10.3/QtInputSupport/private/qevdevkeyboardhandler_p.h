// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QEVDEVKEYBOARDHANDLER_P_H
#define QEVDEVKEYBOARDHANDLER_P_H

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

#include <qobject.h>
#include <qloggingcategory.h>
#include <QtInputSupport/private/qfdcontainer_p.h>
#include <QtInputSupport/private/qkeyboardmap_p.h>
#include <QtInputSupport/private/qkeycodeaction_p.h>

#include <private/qglobal_p.h>

#include <memory>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qLcEvdevKey)

class QSocketNotifier;

class QEvdevKeyboardHandler : public QObject
{
public:
    QEvdevKeyboardHandler(const QString &device, QFdContainer &fd, bool disableZap, bool enableCompose, const QString &keymapFile);
    ~QEvdevKeyboardHandler();

    static std::unique_ptr<QEvdevKeyboardHandler> create(const QString &device,
                                         const QString &specification,
                                         const QString &defaultKeymapFile = QString());

    bool loadKeymap(const QString &file);
    void unloadKeymap();

    void readKeycode();
    QKeycodeAction processKeycode(quint16 keycode, bool pressed, bool autorepeat);

    void switchLang();

private:
    void processKeyEvent(int nativecode, int unicode, int qtcode,
                         Qt::KeyboardModifiers modifiers, bool isPress, bool autoRepeat);
    void switchLed(int, bool);

    QString m_device;
    QFdContainer m_fd;
    QSocketNotifier *m_notify;

    // keymap handling
    quint8 m_modifiers;
    quint8 m_locks[3];
    int m_composing;
    quint16 m_dead_unicode;
    quint8 m_langLock;

    bool m_no_zap;
    bool m_do_compose;

    const QKeyboardMap::Mapping *m_keymap;
    int m_keymap_size;
    const QKeyboardMap::Composing *m_keycompose;
    int m_keycompose_size;

    static const QKeyboardMap::Mapping s_keymap_default[];
    static const QKeyboardMap::Composing s_keycompose_default[];
};


QT_END_NAMESPACE

#endif // QEVDEVKEYBOARDHANDLER_P_H
