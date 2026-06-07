// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QRAWFONTPRIVATE_P_H
#define QRAWFONTPRIVATE_P_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include "qrawfont.h"

#include "qfontengine_p.h"
#include <QtCore/qthread.h>
#include <QtCore/qthreadstorage.h>

#if !defined(QT_NO_RAWFONT)

QT_BEGIN_NAMESPACE

namespace { class CustomFontFileLoader; }
class Q_GUI_EXPORT QRawFontPrivate
{
public:
    QRawFontPrivate()
        : fontEngine(nullptr)
        , hintingPreference(QFont::PreferDefaultHinting)
        , thread(nullptr)
    {}

    QRawFontPrivate(const QRawFontPrivate &other)
        : fontEngine(other.fontEngine)
        , hintingPreference(other.hintingPreference)
        , thread(other.thread)
    {
#ifndef QT_NO_DEBUG
        Q_ASSERT(fontEngine == nullptr || thread == QThread::currentThread());
#endif
        if (fontEngine != nullptr)
            fontEngine->ref.ref();
    }

    ~QRawFontPrivate()
    {
#ifndef QT_NO_DEBUG
        Q_ASSERT(ref.loadRelaxed() == 0);
#endif
        cleanUp();
    }

    inline void cleanUp()
    {
        setFontEngine(nullptr);
        hintingPreference = QFont::PreferDefaultHinting;
    }

    inline bool isValid() const
    {
#ifndef QT_NO_DEBUG
        Q_ASSERT(fontEngine == nullptr || thread == QThread::currentThread());
#endif
        return fontEngine != nullptr;
    }

    inline void setFontEngine(QFontEngine *engine)
    {
#ifndef QT_NO_DEBUG
        Q_ASSERT(fontEngine == nullptr || thread == QThread::currentThread());
#endif
        if (fontEngine == engine)
            return;

        if (fontEngine != nullptr) {
            if (!fontEngine->ref.deref())
                delete fontEngine;
#ifndef QT_NO_DEBUG
            thread = nullptr;
#endif
        }

        fontEngine = engine;

        if (fontEngine != nullptr) {
            fontEngine->ref.ref();
#ifndef QT_NO_DEBUG
            thread = QThread::currentThread();
            Q_ASSERT(thread);
#endif
        }
    }

    void loadFromData(const QByteArray &fontData,
                              qreal pixelSize,
                              QFont::HintingPreference hintingPreference);

    static QRawFontPrivate *get(const QRawFont &font) { return font.d.data(); }

    QFontEngine *fontEngine;
    QFont::HintingPreference hintingPreference;
    QAtomicInt ref;

private:
    QThread *thread;
};

QT_END_NAMESPACE

#endif // QT_NO_RAWFONT

#endif // QRAWFONTPRIVATE_P_H
