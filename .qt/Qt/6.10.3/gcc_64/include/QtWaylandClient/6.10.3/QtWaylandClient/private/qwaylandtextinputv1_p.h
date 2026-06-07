// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#ifndef QWAYLANDTEXTINPUTV1_H
#define QWAYLANDTEXTINPUTV1_H

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

#include "qwaylandtextinputinterface_p.h"
#include <QtWaylandClient/private/qwayland-text-input-unstable-v1.h>
#include <qwaylandinputmethodeventbuilder_p.h>

struct wl_callback;
struct wl_callback_listener;

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandDisplay;

class QWaylandTextInputv1 : public QtWayland::zwp_text_input_v1, public QWaylandTextInputInterface
{
public:
    QWaylandTextInputv1(QWaylandDisplay *display, struct ::zwp_text_input_v1 *text_input);
    ~QWaylandTextInputv1() override;

    void setSeat(struct ::wl_seat *seat) { m_seat = seat; }

    void reset() override;
    void commit() override;
    void updateState(Qt::InputMethodQueries queries, uint32_t flags) override;

    void setCursorInsidePreedit(int cursor) override;

    bool isInputPanelVisible() const override;
    QRectF keyboardRect() const override;

    QLocale locale() const override;
    Qt::LayoutDirection inputDirection() const override;

    void showInputPanel() override
    {
        show_input_panel();
    }
    void hideInputPanel() override
    {
        hide_input_panel();
    }
    void enableSurface(::wl_surface *surface) override
    {
        activate(m_seat, surface);
    }
    void disableSurface(::wl_surface *surface) override
    {
        Q_UNUSED(surface);
        deactivate(m_seat);
    }

protected:
    void zwp_text_input_v1_enter(struct ::wl_surface *surface) override;
    void zwp_text_input_v1_leave() override;
    void zwp_text_input_v1_modifiers_map(wl_array *map) override;
    void zwp_text_input_v1_input_panel_state(uint32_t state) override;
    void zwp_text_input_v1_preedit_string(uint32_t serial, const QString &text, const QString &commit) override;
    void zwp_text_input_v1_preedit_styling(uint32_t index, uint32_t length, uint32_t style) override;
    void zwp_text_input_v1_preedit_cursor(int32_t index) override;
    void zwp_text_input_v1_commit_string(uint32_t serial, const QString &text) override;
    void zwp_text_input_v1_cursor_position(int32_t index, int32_t anchor) override;
    void zwp_text_input_v1_delete_surrounding_text(int32_t before_length, uint32_t after_length) override;
    void zwp_text_input_v1_keysym(uint32_t serial, uint32_t time, uint32_t sym, uint32_t state, uint32_t modifiers) override;
    void zwp_text_input_v1_language(uint32_t serial, const QString &language) override;
    void zwp_text_input_v1_text_direction(uint32_t serial, uint32_t direction) override;

private:
    Qt::KeyboardModifiers modifiersToQtModifiers(uint32_t modifiers);

    QWaylandInputMethodEventBuilder m_builder;

    QList<Qt::KeyboardModifier> m_modifiersMap;

    uint32_t m_serial = 0;
    struct ::wl_surface *m_surface = nullptr;
    struct ::wl_seat *m_seat = nullptr;

    QString m_preeditCommit;

    bool m_inputPanelVisible = false;
    QRectF m_keyboardRectangle;
    QLocale m_locale;
    Qt::LayoutDirection m_inputDirection = Qt::LayoutDirectionAuto;

    struct ::wl_callback *m_resetCallback = nullptr;
    static const wl_callback_listener callbackListener;
    static void resetCallback(void *data, struct wl_callback *wl_callback, uint32_t time);
};

}

QT_END_NAMESPACE
#endif // QWAYLANDTEXTINPUTV1_H

