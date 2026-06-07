// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFONTENGINE_P_H
#define QFONTENGINE_P_H

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
#include <QtGui/qfontvariableaxis.h>
#include "QtCore/qatomic.h"
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qhashfunctions.h>
#include "private/qtextengine_p.h"
#include "private/qfont_p.h"

QT_BEGIN_NAMESPACE

class QPainterPath;
class QFontEngineGlyphCache;

struct QGlyphLayout;

// ### this only used in getPointInOutline(), refactor it and then remove these magic numbers
enum HB_Compat_Error {
    Err_Ok                           = 0x0000,
    Err_Not_Covered                  = 0xFFFF,
    Err_Invalid_Argument             = 0x1A66,
    Err_Invalid_SubTable_Format      = 0x157F,
    Err_Invalid_SubTable             = 0x1570
};

typedef void (*qt_destroy_func_t) (void *user_data);
typedef bool (*qt_get_font_table_func_t) (void *user_data, uint tag, uchar *buffer, uint *length);

Q_DECLARE_LOGGING_CATEGORY(lcColrv1)

class Q_GUI_EXPORT QFontEngine
{
public:
    enum Type {
        Box,
        Multi,

        // MS Windows types
        Win,

        // Apple Mac OS types
        Mac,

        // QWS types
        Freetype,
        QPF1,
        QPF2,
        Proxy,

        DirectWrite,

        TestFontEngine = 0x1000
    };

    enum GlyphFormat {
        Format_None,
        Format_Render = Format_None,
        Format_Mono,
        Format_A8,
        Format_A32,
        Format_ARGB
    };

    enum ShaperFlag {
        DesignMetrics = 0x0002,
        GlyphIndicesOnly = 0x0004,
        FullStringFallback = 0x008
    };
    Q_DECLARE_FLAGS(ShaperFlags, ShaperFlag)

    /* Used with the Freetype font engine. */
    struct Glyph {
        Glyph() = default;
        ~Glyph() { delete [] data; }
        short linearAdvance = 0;
        unsigned short width = 0;
        unsigned short height = 0;
        short x = 0;
        short y = 0;
        short advance = 0;
        signed char format = 0;
        uchar *data = nullptr;
    private:
        Q_DISABLE_COPY(Glyph)
    };

    virtual ~QFontEngine();

    inline Type type() const { return m_type; }

    // all of these are in unscaled metrics if the engine supports uncsaled metrics,
    // otherwise in design metrics
    struct Properties {
        QByteArray postscriptName;
        QByteArray copyright;
        QRectF boundingBox;
        QFixed emSquare;
        QFixed ascent;
        QFixed descent;
        QFixed leading;
        QFixed italicAngle;
        QFixed capHeight;
        QFixed lineWidth;
    };
    virtual Properties properties() const;
    virtual void getUnscaledGlyph(glyph_t glyph, QPainterPath *path, glyph_metrics_t *metrics);
    QByteArray getSfntTable(uint tag) const;
    virtual bool getSfntTableData(uint tag, uchar *buffer, uint *length) const;

    struct FaceId {
        FaceId() : index(0), instanceIndex(-1), encoding(0) {}
        QByteArray filename;
        QByteArray uuid;
        int index;
        int instanceIndex;
        int encoding;
        QMap<QFont::Tag, float> variableAxes;
    };
    virtual FaceId faceId() const { return FaceId(); }
    enum SynthesizedFlags {
        SynthesizedItalic = 0x1,
        SynthesizedBold = 0x2,
        SynthesizedStretch = 0x4
    };
    virtual int synthesized() const { return 0; }
    inline bool supportsSubPixelPositions() const
    {
        return supportsHorizontalSubPixelPositions() || supportsVerticalSubPixelPositions();
    }
    virtual bool supportsHorizontalSubPixelPositions() const { return false; }
    virtual bool supportsVerticalSubPixelPositions() const { return false; }
    virtual QFixedPoint subPixelPositionFor(const QFixedPoint &position) const;
    QFixed subPixelPositionForX(QFixed x) const
    {
        return subPixelPositionFor(QFixedPoint(x, 0)).x;
    }

    bool preferTypoLineMetrics() const;
    bool isColorFont() const { return glyphFormat == Format_ARGB; }
    static bool isIgnorableChar(char32_t ucs4)
    {
        return ucs4 == QChar::LineSeparator
               || ucs4 == QChar::LineFeed
               || ucs4 == QChar::CarriageReturn
               || ucs4 == QChar::ParagraphSeparator
               || (!disableEmojiSegmenter() && (ucs4 & 0xFFF0) == 0xFE00)
               || QChar::category(ucs4) == QChar::Other_Control;
    }

    virtual QFixed emSquareSize() const;

    /* returns 0 as glyph index for non existent glyphs */
    virtual glyph_t glyphIndex(uint ucs4) const = 0;
    virtual int stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs, ShaperFlags flags) const = 0;
    virtual void recalcAdvances(QGlyphLayout *, ShaperFlags) const {}
    virtual void doKerning(QGlyphLayout *, ShaperFlags) const;

    virtual void addGlyphsToPath(glyph_t *glyphs, QFixedPoint *positions, int nglyphs,
                                 QPainterPath *path, QTextItem::RenderFlags flags);

    void getGlyphPositions(const QGlyphLayout &glyphs, const QTransform &matrix, QTextItem::RenderFlags flags,
                           QVarLengthArray<glyph_t> &glyphs_out, QVarLengthArray<QFixedPoint> &positions);

    virtual void addOutlineToPath(qreal, qreal, const QGlyphLayout &, QPainterPath *, QTextItem::RenderFlags flags);
    void addBitmapFontToPath(qreal x, qreal y, const QGlyphLayout &, QPainterPath *, QTextItem::RenderFlags);
    /**
     * Create a qimage with the alpha values for the glyph.
     * Returns an image indexed_8 with index values ranging from 0=fully transparent to 255=opaque
     */
    // ### Refactor this into a smaller and more flexible API.
    virtual QImage alphaMapForGlyph(glyph_t);
    virtual QImage alphaMapForGlyph(glyph_t glyph, const QFixedPoint &subPixelPosition);
    virtual QImage alphaMapForGlyph(glyph_t, const QTransform &t);
    virtual QImage alphaMapForGlyph(glyph_t, const QFixedPoint &subPixelPosition, const QTransform &t);
    virtual QImage alphaRGBMapForGlyph(glyph_t, const QFixedPoint &subPixelPosition, const QTransform &t);
    virtual QImage bitmapForGlyph(glyph_t, const QFixedPoint &subPixelPosition, const QTransform &t, const QColor &color = QColor());
    QImage renderedPathForGlyph(glyph_t glyph, const QColor &color);
    virtual Glyph *glyphData(glyph_t glyph, const QFixedPoint &subPixelPosition, GlyphFormat neededFormat, const QTransform &t);
    virtual bool hasInternalCaching() const { return false; }

    virtual glyph_metrics_t alphaMapBoundingBox(glyph_t glyph, const QFixedPoint &/*subPixelPosition*/, const QTransform &matrix, GlyphFormat /*format*/)
    {
        return boundingBox(glyph, matrix);
    }

    virtual void removeGlyphFromCache(glyph_t);

    virtual glyph_metrics_t boundingBox(const QGlyphLayout &glyphs);
    virtual glyph_metrics_t boundingBox(glyph_t glyph) = 0;
    virtual glyph_metrics_t boundingBox(glyph_t glyph, const QTransform &matrix);
    glyph_metrics_t tightBoundingBox(const QGlyphLayout &glyphs);

    virtual QFixed ascent() const;
    virtual QFixed capHeight() const = 0;
    virtual QFixed descent() const;
    virtual QFixed leading() const;
    virtual QFixed xHeight() const;
    virtual QFixed averageCharWidth() const;

    virtual QFixed lineThickness() const;
    virtual QFixed underlinePosition() const;

    virtual qreal maxCharWidth() const = 0;
    virtual qreal minLeftBearing() const;
    virtual qreal minRightBearing() const;

    virtual void getGlyphBearings(glyph_t glyph, qreal *leftBearing = nullptr, qreal *rightBearing = nullptr);

    inline bool canRender(uint ucs4) const { return glyphIndex(ucs4) != 0; }
    virtual bool canRender(const QChar *str, int len) const;

    virtual bool supportsTransformation(const QTransform &transform) const;

    virtual int glyphCount() const;
    virtual int glyphMargin(GlyphFormat format) { return format == Format_A32 ? 2 : 0; }

    virtual QFontEngine *cloneWithSize(qreal /*pixelSize*/) const { return nullptr; }

    virtual Qt::HANDLE handle() const;

    virtual QList<QFontVariableAxis> variableAxes() const;

    virtual QString glyphName(glyph_t index) const;
    virtual glyph_t findGlyph(QLatin1StringView name) const;

    void *harfbuzzFont() const;
    void *harfbuzzFace() const;
    bool supportsScript(QChar::Script script) const;

    inline static bool scriptRequiresOpenType(QChar::Script script)
    {
        return ((script >= QChar::Script_Syriac && script <= QChar::Script_Sinhala)
                || script == QChar::Script_Khmer || script == QChar::Script_Nko);
    }

    virtual int getPointInOutline(glyph_t glyph, int flags, quint32 point, QFixed *xpos, QFixed *ypos, quint32 *nPoints);

    void clearGlyphCache(const void *key);
    void setGlyphCache(const void *key, QFontEngineGlyphCache *data);
    QFontEngineGlyphCache *glyphCache(const void *key, GlyphFormat format, const QTransform &transform, const QColor &color = QColor()) const;

    static const uchar *getCMap(const uchar *table, uint tableSize, bool *isSymbolFont, int *cmapSize);
    static quint32 getTrueTypeGlyphIndex(const uchar *cmap, int cmapSize, uint unicode);

    static QByteArray convertToPostscriptFontFamilyName(const QByteArray &fontFamily);

    virtual bool hasUnreliableGlyphOutline() const;
    virtual bool expectsGammaCorrectedBlending() const;

    static bool disableEmojiSegmenter();

    enum HintStyle {
        HintNone,
        HintLight,
        HintMedium,
        HintFull
    };
    virtual void setDefaultHintStyle(HintStyle) { }

    enum SubpixelAntialiasingType {
        Subpixel_None,
        Subpixel_RGB,
        Subpixel_BGR,
        Subpixel_VRGB,
        Subpixel_VBGR
    };

private:
    const Type m_type;

public:
    QAtomicInt ref;
    QFontDef fontDef;

    class Holder { // replace by std::unique_ptr once available
        void *ptr;
        qt_destroy_func_t destroy_func;
    public:
        Holder() : ptr(nullptr), destroy_func(nullptr) {}
        explicit Holder(void *p, qt_destroy_func_t d) : ptr(p), destroy_func(d) {}
        ~Holder() { if (ptr && destroy_func) destroy_func(ptr); }
        Holder(Holder &&other) noexcept
            : ptr(std::exchange(other.ptr, nullptr)),
              destroy_func(std::exchange(other.destroy_func, nullptr))
        {
        }
        QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(Holder)

        void swap(Holder &other) noexcept
        {
            qSwap(ptr, other.ptr);
            qSwap(destroy_func, other.destroy_func);
        }

        void *get() const noexcept { return ptr; }
        void *release() noexcept {
            void *result = ptr;
            ptr = nullptr;
            destroy_func = nullptr;
            return result;
        }
        void reset() noexcept { Holder().swap(*this); }
        qt_destroy_func_t get_deleter() const noexcept { return destroy_func; }

        bool operator!() const noexcept { return !ptr; }
    };

    mutable Holder font_; // \ NOTE: Declared before m_glyphCaches, so font_, face_
    mutable Holder face_; // / are destroyed _after_ m_glyphCaches is destroyed.

    struct FaceData {
        void *user_data;
        qt_get_font_table_func_t get_font_table;
    } faceData;

    uint cache_cost; // amount of mem used in bytes by the font
    uint fsType : 16;
    bool symbol;
    bool isSmoothlyScalable;
    struct KernPair {
        uint left_right;
        QFixed adjust;

        inline bool operator<(const KernPair &other) const
        {
            return left_right < other.left_right;
        }
    };
    QList<KernPair> kerning_pairs;
    void loadKerningPairs(QFixed scalingFactor);

    GlyphFormat glyphFormat;
    int m_subPixelPositionCount; // Number of positions within a single pixel for this cache

protected:
    explicit QFontEngine(Type type);

    QFixed firstLeftBearing(const QGlyphLayout &glyphs);
    QFixed lastRightBearing(const QGlyphLayout &glyphs);

    QFixed calculatedCapHeight() const;

    mutable QFixed m_ascent;
    mutable QFixed m_descent;
    mutable QFixed m_leading;
    mutable bool m_heightMetricsQueried;

    virtual void initializeHeightMetrics() const;
    bool processHheaTable() const;
    bool processOS2Table() const;

private:
    struct GlyphCacheEntry {
        GlyphCacheEntry();
        GlyphCacheEntry(const GlyphCacheEntry &);
        ~GlyphCacheEntry();

        GlyphCacheEntry &operator=(const GlyphCacheEntry &);

        QExplicitlySharedDataPointer<QFontEngineGlyphCache> cache;
        bool operator==(const GlyphCacheEntry &other) const { return cache == other.cache; }
    };
    typedef std::list<GlyphCacheEntry> GlyphCaches;
    mutable QHash<const void *, GlyphCaches> m_glyphCaches;

private:
    mutable qreal m_minLeftBearing;
    mutable qreal m_minRightBearing;
};
Q_DECLARE_TYPEINFO(QFontEngine::KernPair, Q_PRIMITIVE_TYPE);

Q_DECLARE_OPERATORS_FOR_FLAGS(QFontEngine::ShaperFlags)

inline bool operator ==(const QFontEngine::FaceId &f1, const QFontEngine::FaceId &f2)
{
    return f1.index == f2.index
            && f1.encoding == f2.encoding
            && f1.filename == f2.filename
            && f1.uuid == f2.uuid
            && f1.instanceIndex == f2.instanceIndex
            && f1.variableAxes == f2.variableAxes;
}

inline size_t qHash(const QFontEngine::FaceId &f, size_t seed = 0)
    noexcept(noexcept(qHash(f.filename)))
{
    return qHashMulti(seed, f.filename, f.uuid, f.index, f.instanceIndex, f.encoding, f.variableAxes.keys(), f.variableAxes.values());
}


class QGlyph;



class QFontEngineBox : public QFontEngine
{
public:
    QFontEngineBox(int size);
    ~QFontEngineBox();

    virtual glyph_t glyphIndex(uint ucs4) const override;
    virtual int stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs, ShaperFlags flags) const override;
    virtual void recalcAdvances(QGlyphLayout *, ShaperFlags) const override;

    void draw(QPaintEngine *p, qreal x, qreal y, const QTextItemInt &si);
    virtual void addOutlineToPath(qreal x, qreal y, const QGlyphLayout &glyphs, QPainterPath *path, QTextItem::RenderFlags flags) override;

    virtual glyph_metrics_t boundingBox(const QGlyphLayout &glyphs) override;
    virtual glyph_metrics_t boundingBox(glyph_t glyph) override;
    virtual QFontEngine *cloneWithSize(qreal pixelSize) const override;

    virtual QFixed emSquareSize() const override { return _size; }
    virtual QFixed ascent() const override;
    virtual QFixed capHeight() const override;
    virtual QFixed descent() const override;
    virtual QFixed leading() const override;
    virtual qreal maxCharWidth() const override;
    virtual qreal minLeftBearing() const override { return 0; }
    virtual qreal minRightBearing() const override { return 0; }
    virtual QImage alphaMapForGlyph(glyph_t) override;

    virtual bool canRender(const QChar *string, int len) const override;

    inline int size() const { return _size; }

protected:
    explicit QFontEngineBox(Type type, int size);

private:
    friend class QFontPrivate;
    int _size;
};

class Q_GUI_EXPORT QFontEngineMulti : public QFontEngine
{
public:
    explicit QFontEngineMulti(QFontEngine *engine, int script, const QStringList &fallbackFamilies = QStringList());
    ~QFontEngineMulti();

    virtual int glyphCount() const override;
    virtual glyph_t glyphIndex(uint ucs4) const override;
    virtual int stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs, ShaperFlags flags) const override;

    virtual glyph_metrics_t boundingBox(const QGlyphLayout &glyphs) override;
    virtual glyph_metrics_t boundingBox(glyph_t glyph) override;

    virtual void recalcAdvances(QGlyphLayout *, ShaperFlags) const override;
    virtual void doKerning(QGlyphLayout *, ShaperFlags) const override;
    virtual void addOutlineToPath(qreal, qreal, const QGlyphLayout &, QPainterPath *, QTextItem::RenderFlags flags) override;
    virtual void getGlyphBearings(glyph_t glyph, qreal *leftBearing = nullptr, qreal *rightBearing = nullptr) override;

    virtual QFixed emSquareSize() const override;
    virtual QFixed ascent() const override;
    virtual QFixed capHeight() const override;
    virtual QFixed descent() const override;
    virtual QFixed leading() const override;
    virtual QFixed xHeight() const override;
    virtual QFixed averageCharWidth() const override;
    virtual QImage alphaMapForGlyph(glyph_t) override;
    virtual QImage alphaMapForGlyph(glyph_t glyph, const QFixedPoint &subPixelPosition) override;
    virtual QImage alphaMapForGlyph(glyph_t, const QTransform &t) override;
    virtual QImage alphaMapForGlyph(glyph_t, const QFixedPoint &subPixelPosition, const QTransform &t) override;
    virtual QImage alphaRGBMapForGlyph(glyph_t, const QFixedPoint &subPixelPosition, const QTransform &t) override;

    virtual QFixed lineThickness() const override;
    virtual QFixed underlinePosition() const override;
    virtual qreal maxCharWidth() const override;
    virtual qreal minLeftBearing() const override;
    virtual qreal minRightBearing() const override;

    virtual QList<QFontVariableAxis> variableAxes() const override;

    virtual bool canRender(const QChar *string, int len) const override;
    QString glyphName(glyph_t glyph) const override;
    glyph_t findGlyph(QLatin1StringView name) const override;

    inline int fallbackFamilyCount() const { return m_fallbackFamilies.size(); }
    inline QString fallbackFamilyAt(int at) const { return m_fallbackFamilies.at(at); }

    void setFallbackFamiliesList(const QStringList &fallbackFamilies);

    static uchar highByte(glyph_t glyph); // Used for determining engine

    inline QFontEngine *engine(int at) const
    { Q_ASSERT(at < m_engines.size()); return m_engines.at(at); }

    void ensureEngineAt(int at);

    static QFontEngine *createMultiFontEngine(QFontEngine *fe, int script);

protected:
    virtual void ensureFallbackFamiliesQueried();
    virtual bool shouldLoadFontEngineForCharacter(int at, uint ucs4) const;
    virtual QFontEngine *loadEngine(int at);

private:
    QList<QFontEngine *> m_engines;
    QStringList m_fallbackFamilies;
    const int m_script;
    bool m_fallbackFamiliesQueried;
};

class QTestFontEngine : public QFontEngineBox
{
public:
    QTestFontEngine(int size);
};

QT_END_NAMESPACE



#endif // QFONTENGINE_P_H
