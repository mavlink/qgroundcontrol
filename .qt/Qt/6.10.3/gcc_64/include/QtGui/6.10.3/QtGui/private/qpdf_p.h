// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPDF_P_H
#define QPDF_P_H

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

#ifndef QT_NO_PDF

#include "QtCore/qlist.h"
#include "QtCore/qstring.h"
#include "QtCore/quuid.h"
#include "private/qfontengine_p.h"
#include "private/qfontsubset_p.h"
#include "private/qpaintengine_p.h"
#include "private/qstroker_p.h"
#include "qpagelayout.h"
#include "qpdfoutputintent.h"

QT_BEGIN_NAMESPACE

const char *qt_real_to_string(qreal val, char *buf);
const char *qt_int_to_string(int val, char *buf);

namespace QPdf {

    class ByteStream
    {
    public:
        // fileBacking means that ByteStream will buffer the contents on disk
        // if the size exceeds a certain threshold. In this case, if a byte
        // array was passed in, its contents may no longer correspond to the
        // ByteStream contents.
        explicit ByteStream(bool fileBacking = false);
        explicit ByteStream(QByteArray *ba, bool fileBacking = false);
        ~ByteStream();
        ByteStream &operator <<(char chr);
        ByteStream &operator <<(const char *str);
        ByteStream &operator <<(const QByteArray &str);
        ByteStream &operator <<(const ByteStream &src);
        ByteStream &operator <<(qreal val);
        ByteStream &operator <<(int val);
        ByteStream &operator <<(uint val) { return (*this << int(val)); }
        ByteStream &operator <<(qint64 val) { return (*this << int(val)); }
        ByteStream &operator <<(const QPointF &p);
        // Note that the stream may be invalidated by calls that insert data.
        QIODevice *stream();
        void clear();

        static inline int maxMemorySize() { return 100000000; }
        static inline int chunkSize()     { return 10000000; }

    private:
        void prepareBuffer();

    private:
        QIODevice *dev;
        QByteArray ba;
        bool fileBackingEnabled;
        bool fileBackingActive;
        bool handleDirty;
    };

    enum PathFlags {
        ClipPath,
        FillPath,
        StrokePath,
        FillAndStrokePath
    };
    QByteArray generatePath(const QPainterPath &path, const QTransform &matrix, PathFlags flags);
    QByteArray generateMatrix(const QTransform &matrix);
    QByteArray generateDashes(const QPen &pen);
    QByteArray patternForBrush(const QBrush &b);

    struct Stroker {
        Stroker();
        void setPen(const QPen &pen, QPainter::RenderHints hints);
        void strokePath(const QPainterPath &path);
        ByteStream *stream;
        bool first;
        QTransform matrix;
        bool cosmeticPen;
    private:
        QStroker basicStroker;
        QDashStroker dashStroker;
        QStrokerOps *stroker;
    };

    QByteArray ascii85Encode(const QByteArray &input);

    const char *toHex(ushort u, char *buffer);
    const char *toHex(uchar u, char *buffer);

}


class QPdfPage : public QPdf::ByteStream
{
public:
    QPdfPage();

    QList<uint> images;
    QList<uint> graphicStates;
    QList<uint> patterns;
    QList<uint> fonts;
    QList<uint> annotations;

    void streamImage(int w, int h, uint object);

    QSize pageSize;
private:
};

class QPdfWriter;
class QPdfEnginePrivate;

class Q_GUI_EXPORT QPdfEngine : public QPaintEngine
{
    Q_DECLARE_PRIVATE(QPdfEngine)
    friend class QPdfWriter;
public:
    // keep in sync with QPagedPaintDevice::PdfVersion and QPdfEnginePrivate::writeHeader()::mapping!
    enum PdfVersion
    {
        Version_1_4,
        Version_A1b,
        Version_1_6,
        Version_X4,
    };

    QPdfEngine();
    explicit QPdfEngine(QPdfEnginePrivate &d);
    ~QPdfEngine() {}

    void setOutputFilename(const QString &filename);

    void setResolution(int resolution);
    int resolution() const;

    void setPdfVersion(PdfVersion version);

    void setDocumentXmpMetadata(const QByteArray &xmpMetadata);
    QByteArray documentXmpMetadata() const;

    void addFileAttachment(const QString &fileName, const QByteArray &data, const QString &mimeType);

    // keep in sync with QPdfWriter
    enum class ColorModel
    {
        RGB,
        Grayscale,
        CMYK,
        Auto,
    };

    ColorModel colorModel() const;
    void setColorModel(ColorModel model);

    // reimplementations QPaintEngine
    bool begin(QPaintDevice *pdev) override;
    bool end() override;

    void drawPoints(const QPointF *points, int pointCount) override;
    void drawLines(const QLineF *lines, int lineCount) override;
    void drawRects(const QRectF *rects, int rectCount) override;
    void drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode) override;
    void drawPath (const QPainterPath & path) override;

    void drawTextItem(const QPointF &p, const QTextItem &textItem) override;

    void drawPixmap (const QRectF & rectangle, const QPixmap & pixmap, const QRectF & sr) override;
    void drawImage(const QRectF &r, const QImage &pm, const QRectF &sr,
                   Qt::ImageConversionFlags flags = Qt::AutoColor) override;
    void drawTiledPixmap (const QRectF & rectangle, const QPixmap & pixmap, const QPointF & point) override;

    void drawHyperlink(const QRectF &r, const QUrl &url);

    void updateState(const QPaintEngineState &state) override;

    int metric(QPaintDevice::PaintDeviceMetric metricType) const;
    Type type() const override;
    // end reimplementations QPaintEngine

    // Printer stuff...
    bool newPage();

    // Page layout stuff
    void setPageLayout(const QPageLayout &pageLayout);
    void setPageSize(const QPageSize &pageSize);
    void setPageOrientation(QPageLayout::Orientation orientation);
    void setPageMargins(const QMarginsF &margins, QPageLayout::Unit units = QPageLayout::Point);

    QPageLayout pageLayout() const;

    void setPen();
    void setBrush();
    void setupGraphicsState(QPaintEngine::DirtyFlags flags);

private:
    void updateClipPath(const QPainterPath & path, Qt::ClipOperation op);
};

class Q_GUI_EXPORT QPdfEnginePrivate : public QPaintEnginePrivate
{
    Q_DECLARE_PUBLIC(QPdfEngine)
public:
    QPdfEnginePrivate();
    ~QPdfEnginePrivate();

    inline uint requestObject() { return currentObject++; }

    void writeHeader();
    void writeTail();

    int addImage(const QImage &image, bool *bitmap, bool lossless, qint64 serial_no);
    int addConstantAlphaObject(int brushAlpha, int penAlpha = 255);
    int addBrushPattern(const QTransform &matrix, bool *specifyColor, int *gStateObject);

    void drawTextItem(const QPointF &p, const QTextItemInt &ti);

    QTransform pageMatrix() const;

    void newPage();

    int currentObject;

    QPdfPage* currentPage;
    QPdf::Stroker stroker;

    QPointF brushOrigin;
    QBrush brush;
    QPen pen;
    QList<QPainterPath> clips;
    bool clipEnabled;
    bool allClipped;
    bool hasPen;
    bool hasBrush;
    bool simplePen;
    bool needsTransform;
    qreal opacity;
    QPdfEngine::PdfVersion pdfVersion;
    QPdfEngine::ColorModel colorModel;

    QHash<QFontEngine::FaceId, QFontSubset *> fonts;

    QPaintDevice *pdev;

    // the device the output is in the end streamed to.
    QIODevice *outDevice;
    bool ownsDevice;

    // printer options
    QString outputFileName;
    QString title;
    QString creator;
    QString author;
    QUuid documentId = QUuid::createUuid();
    bool embedFonts;
    int resolution;
    QPdfOutputIntent outputIntent;

    // Page layout: size, orientation and margins
    QPageLayout m_pageLayout;

private:
    int gradientBrush(const QBrush &b, const QTransform &matrix, int *gStateObject);
    int generateGradientShader(const QGradient *gradient, const QTransform &matrix, bool alpha = false);
    int generateLinearGradientShader(const QLinearGradient *lg, const QTransform &matrix, bool alpha);
    int generateRadialGradientShader(const QRadialGradient *gradient, const QTransform &matrix, bool alpha);
    struct ShadingFunctionResult
    {
        int function;
        QPdfEngine::ColorModel colorModel;
        void writeColorSpace(QPdf::ByteStream *stream) const;
    };
    ShadingFunctionResult createShadingFunction(const QGradient *gradient, int from, int to, bool reflect, bool alpha);

    enum class ColorDomain {
        Stroking,
        NonStroking,
        NonStrokingPattern,
    };

    QPdfEngine::ColorModel colorModelForColor(const QColor &color) const;
    void writeColor(ColorDomain domain, const QColor &color);
    void writeInfo(const QDateTime &date);
    int writeXmpDocumentMetaData(const QDateTime &date);
    int writeOutputIntent();
    void writePageRoot();
    void writeDestsRoot();
    void writeAttachmentRoot();
    void writeNamesRoot();
    void writeFonts();
    void embedFont(QFontSubset *font);
    qreal calcUserUnit() const;

    QList<int> xrefPositions;
    QDataStream* stream;
    int streampos;

    enum class WriteImageOption
    {
        Monochrome,
        Grayscale,
        RGB,
        CMYK,
    };

    int writeImage(const QByteArray &data, int width, int height, WriteImageOption option,
                   int maskObject, int softMaskObject, bool dct = false, bool isMono = false);
    void writePage();

    int addXrefEntry(int object, bool printostr = true);
    void printString(QStringView string);
    void xprintf(const char* fmt, ...) Q_ATTRIBUTE_FORMAT_PRINTF(2, 3);
    inline void write(QByteArrayView data) {
        stream->writeRawData(data.constData(), data.size());
        streampos += data.size();
    }

    int writeCompressed(const char *src, int len);
    inline int writeCompressed(const QByteArray &data) { return writeCompressed(data.constData(), data.size()); }
    int writeCompressed(QIODevice *dev);

    struct AttachmentInfo
    {
        AttachmentInfo (const QString &fileName, const QByteArray &data, const QString &mimeType)
            : fileName(fileName), data(data), mimeType(mimeType) {}
        QString fileName;
        QByteArray data;
        QString mimeType;
    };

    struct DestInfo
    {
        QString anchor;
        uint pageObj;
        QPointF coords;
    };

    // various PDF objects
    int pageRoot, namesRoot, destsRoot, attachmentsRoot, catalog, info;
    int graphicsState;
    int patternColorSpaceRGB;
    int patternColorSpaceGrayscale;
    int patternColorSpaceCMYK;
    QList<uint> pages;
    QHash<qint64, uint> imageCache;
    QHash<std::pair<uint, uint>, uint > alphaCache;
    QList<DestInfo> destCache;
    QList<AttachmentInfo> fileCache;
    QByteArray xmpDocumentMetadata;
};

QT_END_NAMESPACE

#endif // QT_NO_PDF

#endif // QPDF_P_H

