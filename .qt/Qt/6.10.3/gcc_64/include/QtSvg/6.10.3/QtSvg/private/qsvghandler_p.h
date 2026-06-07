// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSVGHANDLER_P_H
#define QSVGHANDLER_P_H

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

#include "QtCore/qxmlstream.h"
#include "QtCore/qstack.h"
#include <QtCore/QLoggingCategory>
#include "qsvgstyle_p.h"
#if QT_CONFIG(cssparser)
#include <QtSvg/private/qsvgcsshandler_p.h>
#include <QtSvg/private/qsvgcssproperties_p.h>
#endif
#include "qsvggraphics_p.h"
#include "qtsvgglobal_p.h"
#include "qsvgutils_p.h"

QT_BEGIN_NAMESPACE

class QSvgNode;
class QSvgTinyDocument;
class QSvgHandler;
class QColor;

class Q_SVG_EXPORT QSvgHandler
{
public:
    QSvgHandler(QIODevice *device, QtSvg::Options options = {},
                QtSvg::AnimatorType type = QtSvg::AnimatorType::Automatic);
    QSvgHandler(const QByteArray &data, QtSvg::Options options = {},
                QtSvg::AnimatorType type = QtSvg::AnimatorType::Automatic);
    QSvgHandler(QXmlStreamReader *const data, QtSvg::Options options = {},
                QtSvg::AnimatorType type = QtSvg::AnimatorType::Automatic);
    ~QSvgHandler();

    QIODevice *device() const;
    QSvgTinyDocument *document() const;

    inline bool ok() const {
        return document() != 0 && !xml->hasError();
    }

    inline QString errorString() const { return xml->errorString(); }
    inline int lineNumber() const { return xml->lineNumber(); }

    void setDefaultCoordinateSystem(QSvgUtils::LengthType type);
    QSvgUtils::LengthType defaultCoordinateSystem() const;

    void pushColor(const QColor &color);
    void pushColorCopy();
    void popColor();
    QColor currentColor() const;

#ifndef QT_NO_CSSPARSER
    void setInStyle(bool b);
    bool inStyle() const;

    QSvgCssHandler &cssHandler();
#endif

    void setAnimPeriod(int start, int end);
    int animationDuration() const;

    inline QPen defaultPen() const
    { return m_defaultPen; }

    QtSvg::Options options() const;
    QtSvg::AnimatorType animatorType() const;
    bool trustedSourceMode() const;

public:
    bool startElement(const QStringView localName, const QXmlStreamAttributes &attributes);
    bool endElement(const QStringView localName);
    bool characters(const QStringView str);
    bool processingInstruction(const QStringView target, const QStringView data);

private:
    void init();

    QSvgTinyDocument *m_doc;
    QStack<QSvgNode *> m_nodes;
    // TODO: This is only needed during parsing, so it unnecessarily takes up space after that.
    // Temporary container for :
    // - <use> nodes which haven't been resolved yet.
    // - <filter> nodes to be checked for unsupported filter primitives.
    QList<QSvgNode *> m_toBeResolved;

    enum CurrentNode
    {
        Unknown,
        Graphics,
        Style,
        Doc
    };
    QStack<CurrentNode> m_skipNodes;

    /*!
        Follows the depths of elements. The top is current xml:space
        value that applies for a given element.
     */
    QStack<QSvgText::WhitespaceMode> m_whitespaceMode;

    QSvgRefCounter<QSvgStyleProperty> m_style;

    QSvgUtils::LengthType m_defaultCoords;

    QStack<QColor> m_colorStack;
    QStack<int>    m_colorTagCount;

    int m_animEnd;

    QXmlStreamReader *const xml;
#ifndef QT_NO_CSSPARSER
    bool m_inStyle;
    QSvgCssHandler m_cssHandler;
#endif
    void parse();
    void resolvePaintServers(QSvgNode *node, int nestedDepth = 0);
    void resolveNodes();

    QPen m_defaultPen;
    /**
     * Whether we own the variable xml, and hence whether
     * we need to delete it.
     */
    const bool m_ownsReader;

    const QtSvg::Options m_options;
    const QtSvg::AnimatorType m_animatorType;
};

Q_DECLARE_LOGGING_CATEGORY(lcSvgHandler)

QT_END_NAMESPACE

#endif // QSVGHANDLER_P_H
