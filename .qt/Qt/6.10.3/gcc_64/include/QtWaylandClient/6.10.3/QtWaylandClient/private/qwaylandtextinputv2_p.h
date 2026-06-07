// Copyright (C) 2016 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#ifndef QWAYLANDINPUTV2_P_H
#define QWAYLANDINPUTV2_P_H

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
#include <QtWaylandClient/private/qwayland-text-input-unstable-v2.h>
#include <qwaylandinputmethodeventbuilder_p.h>

struct wl_callback;
struct wl_callback_listener;

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandDisplay;

class QWaylandTextInputv2 : public QtWayland::zwp_text_input_v2, public QWaylandTextInputInterface
{
public:
    QWaylandTextInputv2(QWaylandDisplay *display, struct ::zwp_text_input_v2 *text_input);
    ~QWaylandTextInputv2() override;

    void reset() override;
    void commit() override;
    void updateState(Qt::InputMethodQueries queries, uint32_t flags) override;

    void setCursorInsidePreedit(int cursor) override;

    bool isInputPanelVisible() const override;
    QRectF keyboardRect() const override;

    QLocale locale() const override;
    Qt::LayoutDirection inputDirection() const override;

    void showInputPanel() override;
    void hideInputPanel() override;
    void enableSurface(::wl_surface *surface) override;
    void disableSurface(::wl_surface *surface) override;

protected:
    void zwp_text_input_v2_enter(uint32_t serial, struct ::wl_surface *surface) override;
    void zwp_text_input_v2_leave(uint32_t serial, struct ::wl_surface *surface) override;
    void zwp_text_input_v2_modifiers_map(wl_array *map) override;
    void zwp_text_input_v2_input_panel_state(uint32_t state, int32_t x, int32_t y, int32_t width, int32_t height) override;
    void zwp_text_input_v2_preedit_string(const QString &text, const QString &commit) override;
    void zwp_text_input_v2_preedit_styling(uint32_t index, uint32_t length, uint32_t style) override;
    void zwp_text_input_v2_preedit_cursor(int32_t index) override;
    void zwp_text_input_v2_commit_string(const QString &text) override;
    void zwp_text_input_v2_cursor_position(int32_t index, int32_t anchor) override;
    void zwp_text_input_v2_delete_surrounding_text(uint32_t before_length, uint32_t after_length) override;
    void zwp_text_input_v2_keysym(uint32_t time, uint32_t sym, uint32_t state, uint32_t modifiers) override;
    void zwp_text_input_v2_language(const QString &language) override;
    void zwp_text_input_v2_text_direction(uint32_t direction) override;
    void zwp_text_input_v2_input_method_changed(uint32_t serial, uint32_t flags) override;

private:
    Qt::KeyboardModifiers modifiersToQtModifiers(uint32_t modifiers);

    QWaylandDisplay *m_display = nullptr;
    QWaylandInputMethodEventBuilder m_builder;

    QList<Qt::KeyboardModifier> m_modifiersMap;

    uint32_t m_serial = 0;
    struct ::wl_surface *m_surface = nullptr;

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

#endif // QWAYLANDTEXTINPUTV2_P_H
