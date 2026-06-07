// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2013 Konstantin Ritt
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QHARFBUZZNG_P_H
#define QHARFBUZZNG_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>

QT_REQUIRE_CONFIG(harfbuzz);

#include <QtCore/qchar.h>

#if defined(QT_BUILD_GUI_LIB)
#  include <hb.h>
#else
// a minimal set of HB types required for Qt libs other than Gui

typedef struct hb_face_t hb_face_t;
typedef struct hb_font_t hb_font_t;

#endif // QT_BUILD_GUI_LIB

QT_BEGIN_NAMESPACE

class QFontEngine;

#if defined(QT_BUILD_GUI_LIB)

// Unicode

hb_script_t hb_qt_script_to_script(QChar::Script script);
QChar::Script hb_qt_script_from_script(hb_script_t script);

hb_unicode_funcs_t *hb_qt_get_unicode_funcs();

#endif // QT_BUILD_GUI_LIB

// Font

Q_GUI_EXPORT hb_face_t *hb_qt_face_get_for_engine(QFontEngine *fe);
Q_GUI_EXPORT hb_font_t *hb_qt_font_get_for_engine(QFontEngine *fe);

Q_GUI_EXPORT void hb_qt_font_set_use_design_metrics(hb_font_t *font, uint value);
Q_GUI_EXPORT uint hb_qt_font_get_use_design_metrics(hb_font_t *font);

QT_END_NAMESPACE

#endif // QHARFBUZZNG_P_H
