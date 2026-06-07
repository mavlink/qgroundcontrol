// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDTEXTINPUTINTERFACE_P_H
#define QWAYLANDTEXTINPUTINTERFACE_P_H

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

#include <QtCore/qlocale.h>
#include <QtCore/qrect.h>
#include <QtCore/private/qglobal_p.h>

struct wl_surface;

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandTextInputInterface
{
public:
    virtual ~QWaylandTextInputInterface() {}
    virtual void reset() = 0;
    virtual void commit() = 0;
    virtual void disableSurface(::wl_surface *surface) = 0;
    virtual void enableSurface(::wl_surface *surface) = 0;
    virtual void updateState(Qt::InputMethodQueries queries, uint32_t flags) = 0;
    virtual void showInputPanel() {}
    virtual void hideInputPanel() {}
    virtual bool isInputPanelVisible() const = 0;
    virtual QRectF keyboardRect() const = 0;
    virtual QLocale locale() const = 0;
    virtual Qt::LayoutDirection inputDirection() const = 0;
    virtual void setCursorInsidePreedit(int cursor) = 0;

    // This enum should be compatible with update_state of text-input-unstable-v2.
    // Higher versions of text-input-* protocol may not use it directly
    // but QtWaylandClient can determine clients' states based on the values
    enum TextInputState {
        update_state_change = 0, // updated state because it changed
        update_state_full = 1, // full state after enter or input_method_changed event
        update_state_reset = 2, // full state after reset
        update_state_enter = 3, // full state after switching focus to a different widget on client side
    };
};

}

QT_END_NAMESPACE

#endif // QWAYLANDTEXTINPUTINTERFACE_P_H

