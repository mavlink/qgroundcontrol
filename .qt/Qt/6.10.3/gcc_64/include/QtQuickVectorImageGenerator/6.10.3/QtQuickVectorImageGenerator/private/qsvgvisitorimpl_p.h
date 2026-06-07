// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSVGVISITORIMPL_P_H
#define QSVGVISITORIMPL_P_H

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

#include <QtSvg/private/qsvgvisitor_p.h>
#include "qquickgenerator_p.h"
#include "qquickanimatedproperty_p.h"

QT_BEGIN_NAMESPACE

class QTextStream;
class QSvgTinyDocument;
class QString;
class QQuickItem;
class QSvgStyleResolver;

class QSvgVisitorImpl : public QSvgVisitor
{
public:
    QSvgVisitorImpl(const QString svgFileName, QQuickGenerator *generator, bool assumeTrustedSource);
    ~QSvgVisitorImpl() override;
    bool traverse();

protected:
    void visitNode(const QSvgNode *node) override;
    void visitImageNode(const QSvgImage *node) override;
    void visitRectNode(const QSvgRect *node) override;
    void visitEllipseNode(const QSvgEllipse *node) override;
    void visitPathNode(const QSvgPath *node) override;
    void visitLineNode(const QSvgLine *node) override;
    void visitPolygonNode(const QSvgPolygon *node) override;
    void visitPolylineNode(const QSvgPolyline *node) override;
    void visitTextNode(const QSvgText *node) override;
    void visitUseNode(const QSvgUse *node) override;
    bool visitDefsNodeStart(const QSvgDefs *node) override;
    bool visitStructureNodeStart(const QSvgStructureNode *node) override;
    void visitStructureNodeEnd(const QSvgStructureNode *node) override;

    bool visitDocumentNodeStart(const QSvgTinyDocument *node) override;
    void visitDocumentNodeEnd(const QSvgTinyDocument *node) override;

    bool visitSwitchNodeStart(const QSvgSwitch *node) override;
    void visitSwitchNodeEnd(const QSvgSwitch *node) override;

private:
    typedef std::pair<const QSvgAbstractAnimation *, const QSvgAbstractAnimatedProperty *> AnimationPair;
    QList<AnimationPair> collectAnimations(const QSvgNode *node, const QString &propertyName);
    void applyAnimationsToProperty(const QList<AnimationPair> &animations,
                                   QQuickAnimatedProperty *property,
                                   std::function<QVariant(const QSvgAbstractAnimatedProperty *, int index, int subtype)> calculateValue);

    void fillCommonNodeInfo(const QSvgNode *node, NodeInfo &info);
    void fillPathAnimationInfo(const QSvgNode *node, PathNodeInfo &info);
    void fillAnimationInfo(const QSvgNode *node, NodeInfo &info);
    void fillColorAnimationInfo(const QSvgNode *node, PathNodeInfo &info);
    void fillTransformAnimationInfo(const QSvgNode *node, NodeInfo &info);
    void handleBaseNodeSetup(const QSvgNode *node);
    void handleBaseNode(const QSvgNode *node);
    void handleBaseNodeEnd(const QSvgNode *node);
    void handlePathNode(const QSvgNode *node, const QPainterPath &path);
    void outputShapePath(QPainterPath pathCopy, const PathNodeInfo &info);
    static QString gradientCssDescription(const QGradient *gradient);
    static QString colorCssDescription(QColor color);

private:
    QString m_svgFileName;
    QQuickGenerator *m_generator;
    bool m_assumeTrustedSource;
    std::unique_ptr<QSvgStyleResolver> m_styleResolver;
};

QT_END_NAMESPACE

#endif // QSVGVISITORIMPL_P_H
