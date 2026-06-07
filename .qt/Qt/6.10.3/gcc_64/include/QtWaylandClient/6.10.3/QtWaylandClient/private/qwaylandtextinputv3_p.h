// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDTEXTINPUTV3_P_H
#define QWAYLANDTEXTINPUTV3_P_H

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
#include <QtWaylandClient/private/qwayland-text-input-unstable-v3.h>
#include <QLoggingCategory>

struct wl_callback;
struct wl_callback_listener;

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qLcQpaWaylandTextInput)

namespace QtWaylandClient {

class QWaylandDisplay;

class QWaylandTextInputv3 : public QtWayland::zwp_text_input_v3, public QWaylandTextInputInterface
{
public:
    QWaylandTextInputv3(QWaylandDisplay *display, struct ::zwp_text_input_v3 *text_input);
    ~QWaylandTextInputv3() override;

    void reset() override;
    void commit() override;
    void updateState(Qt::InputMethodQueries queries, uint32_t flags) override;
    // TODO: not supported yet
    void setCursorInsidePreedit(int cursor) override;

    bool isInputPanelVisible() const override;
    QRectF keyboardRect() const override;

    QLocale locale() const override;
    Qt::LayoutDirection inputDirection() const override;

    void enableSurface(::wl_surface *) override;
    void disableSurface(::wl_surface *) override;

protected:
    void zwp_text_input_v3_enter(struct ::wl_surface *surface) override;
    void zwp_text_input_v3_leave(struct ::wl_surface *surface) override;
    void zwp_text_input_v3_preedit_string(const QString &text, int32_t cursor_begin, int32_t cursor_end) override;
    void zwp_text_input_v3_commit_string(const QString &text) override;
    void zwp_text_input_v3_delete_surrounding_text(uint32_t before_length, uint32_t after_length) override;
    void zwp_text_input_v3_done(uint32_t serial) override;

private:
    ::wl_surface *m_surface = nullptr; // ### Here for debugging purposes

    struct PreeditInfo {
        QString text;
        int cursorBegin = 0;
        int cursorEnd = 0;

        void clear() {
            text.clear();
            cursorBegin = 0;
            cursorEnd = 0;
        }
        friend bool operator==(const PreeditInfo& lhs, const PreeditInfo& rhs) {
            return (lhs.text == rhs.text)
                    && (lhs.cursorBegin == rhs.cursorBegin)
                    && (lhs.cursorEnd == rhs.cursorEnd);
        }
    };

    PreeditInfo m_pendingPreeditString;
    PreeditInfo m_currentPreeditString;
    QString m_pendingCommitString;
    uint m_pendingDeleteBeforeText = 0; // byte length
    uint m_pendingDeleteAfterText = 0;  // byte length

    QString m_surroundingText;
    int m_cursor = 0; // cursor position in QString
    int m_cursorPos = 0; // cursor position in wayland index
    int m_anchorPos = 0; // anchor position in wayland index
    uint32_t m_contentHint = 0;
    uint32_t m_contentPurpose = 0;
    QRect m_cursorRect;

    uint m_currentSerial = 0;

    bool m_condReselection = false;
};

}

QT_END_NAMESPACE

#endif // QWAYLANDTEXTINPUTV3_P_H
