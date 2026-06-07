// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSVGTINYDOCUMENT_P_H
#define QSVGTINYDOCUMENT_P_H

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

#include "qsvgstructure_p.h"
#include "qtsvgglobal_p.h"

#include "QtCore/qrect.h"
#include "QtCore/qlist.h"
#include "QtCore/qhash.h"
#include "QtCore/qdatetime.h"
#include "QtCore/qxmlstream.h"
#include "QtCore/qsharedpointer.h"
#include "qsvgstyle_p.h"
#include "qsvgfont_p.h"
#include "private/qsvganimator_p.h"

QT_BEGIN_NAMESPACE

class QPainter;
class QByteArray;
class QSvgFont;
class QTransform;

class Q_SVG_EXPORT QSvgTinyDocument : public QSvgStructureNode
{
public:
    static QSvgTinyDocument *load(const QString &file, QtSvg::Options options = {},
                                  QtSvg::AnimatorType type = QtSvg::AnimatorType::Automatic);
    static QSvgTinyDocument *load(const QByteArray &contents, QtSvg::Options options = {},
                                  QtSvg::AnimatorType type = QtSvg::AnimatorType::Automatic);
    static QSvgTinyDocument *load(QXmlStreamReader *contents, QtSvg::Options options = {},
                                  QtSvg::AnimatorType type = QtSvg::AnimatorType::Automatic);
    static bool isLikelySvg(QIODevice *device, bool *isCompressed = nullptr);
public:
    QSvgTinyDocument(QtSvg::Options options, QtSvg::AnimatorType type);
    ~QSvgTinyDocument();
    Type type() const override;

    inline QSize size() const;
    void setWidth(int len, bool percent);
    void setHeight(int len, bool percent);
    inline int width() const;
    inline int height() const;
    inline bool widthPercent() const;
    inline bool heightPercent() const;

    inline bool preserveAspectRatio() const;
    void setPreserveAspectRatio(bool on);

    inline QRectF viewBox() const;
    void setViewBox(const QRectF &rect);

    QtSvg::Options options() const;

    void drawCommand(QPainter *, QSvgExtraStates &) override;

    void draw(QPainter *p);
    void draw(QPainter *p, const QRectF &bounds);
    void draw(QPainter *p, const QString &id,
              const QRectF &bounds=QRectF());

    QTransform transformForElement(const QString &id) const;
    QRectF boundsOnElement(const QString &id) const;
    bool   elementExists(const QString &id) const;

    void addSvgFont(QSvgFont *);
    QSvgFont *svgFont(const QString &family) const;
    void addNamedNode(const QString &id, QSvgNode *node);
    QSvgNode *namedNode(const QString &id) const;
    void addNamedStyle(const QString &id, QSvgPaintStyleProperty *style);
    QSvgPaintStyleProperty *namedStyle(const QString &id) const;

    void restartAnimation();
    inline qint64 currentElapsed() const;
    bool animated() const;
    void setAnimated(bool a);
    inline int animationDuration() const;
    int currentFrame() const;
    void setCurrentFrame(int);
    void setFramesPerSecond(int num);

    QSharedPointer<QSvgAbstractAnimator> animator() const;

private:
    void mapSourceToTarget(QPainter *p, const QRectF &targetRect, const QRectF &sourceRect = QRectF());
private:
    QSize  m_size;
    bool   m_widthPercent;
    bool   m_heightPercent;

    mutable bool m_implicitViewBox = true;
    mutable QRectF m_viewBox;
    bool m_preserveAspectRatio = false;

    QHash<QString, QSvgRefCounter<QSvgFont> > m_fonts;
    QHash<QString, QSvgNode *> m_namedNodes;
    QHash<QString, QSvgRefCounter<QSvgPaintStyleProperty> > m_namedStyles;

    bool  m_animated;
    int   m_fps;

    QSvgExtraStates m_states;

    const QtSvg::Options m_options;
    QSharedPointer<QSvgAbstractAnimator> m_animator;
};

Q_SVG_EXPORT QDebug operator<<(QDebug debug, const QSvgTinyDocument &doc);

inline QSize QSvgTinyDocument::size() const
{
    if (m_size.isEmpty())
        return viewBox().size().toSize();
    if (m_widthPercent || m_heightPercent) {
        const int width = m_widthPercent ? qRound(0.01 * m_size.width() * viewBox().size().width()) : m_size.width();
        const int height = m_heightPercent ? qRound(0.01 * m_size.height() * viewBox().size().height()) : m_size.height();
        return QSize(width, height);
    }
    return m_size;
}

inline int QSvgTinyDocument::width() const
{
    return size().width();
}

inline int QSvgTinyDocument::height() const
{
    return size().height();
}

inline bool QSvgTinyDocument::widthPercent() const
{
    return m_widthPercent;
}

inline bool QSvgTinyDocument::heightPercent() const
{
    return m_heightPercent;
}

inline QRectF QSvgTinyDocument::viewBox() const
{
    if (m_viewBox.isNull()) {
        m_viewBox = bounds();
        m_implicitViewBox = true;
    }

    return m_viewBox;
}

inline bool QSvgTinyDocument::preserveAspectRatio() const
{
    return m_preserveAspectRatio;
}

inline qint64 QSvgTinyDocument::currentElapsed() const
{
    return m_animator->currentElapsed();
}

inline int QSvgTinyDocument::animationDuration() const
{
    return m_animator->animationDuration();
}

QT_END_NAMESPACE

#endif // QSVGTINYDOCUMENT_P_H
