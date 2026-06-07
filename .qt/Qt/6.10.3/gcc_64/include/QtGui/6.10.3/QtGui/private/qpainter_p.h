// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPAINTER_P_H
#define QPAINTER_P_H

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

#include <QtCore/qvarlengtharray.h>
#include <QtGui/private/qtguiglobal_p.h>
#include "QtGui/qbrush.h"
#include "QtGui/qcolorspace.h"
#include "QtGui/qcolortransform.h"
#include "QtGui/qfont.h"
#include "QtGui/qpen.h"
#include "QtGui/qregion.h"
#include "QtGui/qpainter.h"
#include "QtGui/qpainterpath.h"
#include "QtGui/qpaintengine.h"

#include <private/qpen_p.h>

#include <memory>
#include <stack>

QT_BEGIN_NAMESPACE

class QPaintEngine;
class QEmulationPaintEngine;
class QPaintEngineEx;
struct QFixedPoint;

struct QTLWExtra;

struct DataPtrContainer {
    void *ptr;
};

inline const void *data_ptr(const QTransform &t) { return (const DataPtrContainer *) &t; }
inline bool qtransform_fast_equals(const QTransform &a, const QTransform &b) { return data_ptr(a) == data_ptr(b); }

// QPen inline functions...
inline QPen::DataPtr &data_ptr(const QPen &p) { return const_cast<QPen &>(p).data_ptr(); }
inline bool qpen_fast_equals(const QPen &a, const QPen &b) { return data_ptr(a) == data_ptr(b); }
inline QBrush qpen_brush(const QPen &p) { return data_ptr(p)->brush; }
inline qreal qpen_widthf(const QPen &p) { return data_ptr(p)->width; }
inline Qt::PenStyle qpen_style(const QPen &p) { return data_ptr(p)->style; }
inline Qt::PenCapStyle qpen_capStyle(const QPen &p) { return data_ptr(p)->capStyle; }
inline Qt::PenJoinStyle qpen_joinStyle(const QPen &p) { return data_ptr(p)->joinStyle; }

// QBrush inline functions...
inline QBrush::DataPtr &data_ptr(const QBrush &p) { return const_cast<QBrush &>(p).data_ptr(); }
inline bool qbrush_fast_equals(const QBrush &a, const QBrush &b) { return data_ptr(a) == data_ptr(b); }
inline Qt::BrushStyle qbrush_style(const QBrush &b) { return data_ptr(b)->style; }
inline const QColor &qbrush_color(const QBrush &b) { return data_ptr(b)->color; }
inline bool qbrush_has_transform(const QBrush &b) { return data_ptr(b)->transform.type() > QTransform::TxNone; }

class QPainterClipInfo
{
public:
    QPainterClipInfo() { } // for QList, don't use
    enum ClipType { RegionClip, PathClip, RectClip, RectFClip };

    QPainterClipInfo(const QPainterPath &p, Qt::ClipOperation op, const QTransform &m) :
        clipType(PathClip), matrix(m), operation(op), path(p) { }

    QPainterClipInfo(const QRegion &r, Qt::ClipOperation op, const QTransform &m) :
        clipType(RegionClip), matrix(m), operation(op), region(r) { }

    QPainterClipInfo(const QRect &r, Qt::ClipOperation op, const QTransform &m) :
        clipType(RectClip), matrix(m), operation(op), rect(r) { }

    QPainterClipInfo(const QRectF &r, Qt::ClipOperation op, const QTransform &m) :
        clipType(RectFClip), matrix(m), operation(op), rectf(r) { }

    ClipType clipType;
    QTransform matrix;
    Qt::ClipOperation operation;
    QPainterPath path;
    QRegion region;
    QRect rect;
    QRectF rectf;

    // ###
//     union {
//         QRegionData *d;
//         QPainterPathPrivate *pathData;

//         struct {
//             int x, y, w, h;
//         } rectData;
//         struct {
//             qreal x, y, w, h;
//         } rectFData;
//     };

};

Q_DECLARE_TYPEINFO(QPainterClipInfo, Q_RELOCATABLE_TYPE);

class Q_GUI_EXPORT QPainterState : public QPaintEngineState
{
public:
    QPainterState();
    QPainterState(const QPainterState *s);
    virtual ~QPainterState();
    void init(QPainter *p);

    QPointF brushOrigin;
    QFont font;
    QFont deviceFont;
    QPen pen;
    QBrush brush;
    QBrush bgBrush = Qt::white; // background brush
    QRegion clipRegion;
    QPainterPath clipPath;
    Qt::ClipOperation clipOperation = Qt::NoClip;
    QPainter::RenderHints renderHints;
    QList<QPainterClipInfo> clipInfo; // ### Make me smaller and faster to copy around...
    QTransform worldMatrix;       // World transformation matrix, not window and viewport
    QTransform matrix;            // Complete transformation matrix,
    QTransform redirectionMatrix;
    int wx = 0, wy = 0, ww = 0, wh = 0; // window rectangle
    int vx = 0, vy = 0, vw = 0, vh = 0; // viewport rectangle
    qreal opacity = 1;

    uint WxF:1;                 // World transformation
    uint VxF:1;                 // View transformation
    uint clipEnabled:1;

    Qt::BGMode bgMode = Qt::TransparentMode;
    QPainter *painter = nullptr;
    Qt::LayoutDirection layoutDirection;
    QPainter::CompositionMode composition_mode = QPainter::CompositionMode_SourceOver;
    uint emulationSpecifier = 0;
    uint changeFlags = 0;
};

struct QPainterDummyState
{
    QFont font;
    QPen pen;
    QBrush brush;
    QTransform transform;
};

class QRawFont;
class QPainterPrivate
{
    Q_DECLARE_PUBLIC(QPainter)
public:
    explicit QPainterPrivate(QPainter *painter);
    ~QPainterPrivate();

    QPainter *q_ptr;
    // Allocate space for 4 d-pointers (enough for up to 4 sub-sequent
    // redirections within the same paintEvent(), which should be enough
    // in 99% of all cases). E.g: A renders B which renders C which renders D.
    static constexpr qsizetype NDPtrs = 4;
    QVarLengthArray<std::unique_ptr<QPainterPrivate>, NDPtrs> d_ptrs;

    std::unique_ptr<QPainterState> state;
    template <typename T, std::size_t N = 8>
    struct SmallStack : std::stack<T, QVarLengthArray<T, N>> {
        void clear() { this->c.clear(); }
    };
    SmallStack<std::unique_ptr<QPainterState>> savedStates;

    mutable std::unique_ptr<QPainterDummyState> dummyState;

    QTransform invMatrix;
    uint txinv:1;
    uint inDestructor : 1;
    uint refcount = 1;

    enum DrawOperation { StrokeDraw        = 0x1,
                         FillDraw          = 0x2,
                         StrokeAndFillDraw = 0x3
    };

    QPainterDummyState *fakeState() const {
        if (!dummyState)
            dummyState = std::make_unique<QPainterDummyState>();
        return dummyState.get();
    }

    void updateEmulationSpecifier(QPainterState *s);
    void updateStateImpl(QPainterState *state);
    void updateState(QPainterState *state);
    void updateState(std::unique_ptr<QPainterState> &state) { updateState(state.get()); }

    void draw_helper(const QPainterPath &path, DrawOperation operation = StrokeAndFillDraw);
    void drawStretchedGradient(const QPainterPath &path, DrawOperation operation);
    void drawOpaqueBackground(const QPainterPath &path, DrawOperation operation);
    void drawTextItem(const QPointF &p, const QTextItem &_ti, QTextEngine *textEngine);

#if !defined(QT_NO_RAWFONT)
    void drawGlyphs(const QPointF &decorationPosition, const quint32 *glyphArray, QFixedPoint *positionArray, int glyphCount,
                    QFontEngine *fontEngine, bool overline = false, bool underline = false,
                    bool strikeOut = false);
#endif

    void updateMatrix();
    void updateInvMatrix();

    void checkEmulation();

    static QPainterPrivate *get(QPainter *painter)
    {
        return painter->d_ptr.get();
    }

    QTransform viewTransform() const;
    qreal effectiveDevicePixelRatio() const;
    QTransform hidpiScaleTransform() const;
    static bool attachPainterPrivate(QPainter *q, QPaintDevice *pdev);
    void detachPainterPrivate(QPainter *q);
    void initFrom(const QPaintDevice *device);

    QPaintDevice *device = nullptr;
    QPaintDevice *original_device = nullptr;
    QPaintDevice *helper_device = nullptr;

    struct QPaintEngineDestructor {
        void operator()(QPaintEngine *pe) const noexcept
        {
            if (pe && pe->autoDestruct())
                delete pe;
        }
    };
    std::unique_ptr<QPaintEngine, QPaintEngineDestructor> engine;

    std::unique_ptr<QEmulationPaintEngine> emulationEngine;
    QPaintEngineEx *extended = nullptr;
    QBrush colorBrush;          // for fill with solid color
};

Q_GUI_EXPORT void qt_draw_helper(QPainterPrivate *p, const QPainterPath &path, QPainterPrivate::DrawOperation operation);

QString qt_generate_brush_key(const QBrush &brush);


QT_END_NAMESPACE

#endif // QPAINTER_P_H
