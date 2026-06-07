// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSVGVISITOR_P_H
#define  QSVGVISITOR_P_H

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

#include <private/qsvgtinydocument_p.h>
#include <private/qsvghandler_p.h>
#include <private/qsvggraphics_p.h>
#include <private/qsvgstructure_p.h>
#include <private/qsvganimate_p.h>

QT_BEGIN_NAMESPACE

class Q_SVG_EXPORT QSvgVisitor {
public:
    void traverse(const QSvgStructureNode *node);
    void traverse(const QSvgNode *node);

    virtual ~QSvgVisitor() {}

protected:
    virtual void visitNode(const QSvgNode *) {}
    virtual bool visitStructureNodeStart(const QSvgStructureNode *node) { visitNode(node); return true; }
    virtual void visitStructureNodeEnd(const QSvgStructureNode *) {}
    virtual void visitAnimateNode(const QSvgAnimateNode *node) { visitNode(node); }
    virtual void visitEllipseNode(const QSvgEllipse *node) { visitNode(node); }
    virtual void visitImageNode(const QSvgImage *node) { visitNode(node); }
    virtual void visitLineNode(const QSvgLine *node) { visitNode(node); }
    virtual void visitPathNode(const QSvgPath *node) { visitNode(node); }
    virtual void visitPolygonNode(const QSvgPolygon *node) { visitNode(node); }
    virtual void visitPolylineNode(const QSvgPolyline *node) { visitNode(node); }
    virtual void visitRectNode(const QSvgRect *node) { visitNode(node); }
    virtual void visitTextNode(const QSvgText *node) { visitNode(node); }
    virtual void visitTspanNode(const QSvgTspan *node) { visitNode(node); }
    virtual void visitUseNode(const QSvgUse *node) { visitNode(node); }
    virtual void visitVideoNode(const QSvgVideo *node) { visitNode(node); }

    virtual bool visitDocumentNodeStart(const QSvgTinyDocument *node) { return visitStructureNodeStart(node); }
    virtual void visitDocumentNodeEnd(const QSvgTinyDocument *node) { visitStructureNodeEnd(node); }
    virtual bool visitGroupNodeStart(const QSvgG *node) { return visitStructureNodeStart(node); }
    virtual void visitGroupNodeEnd(const QSvgG *node)  { visitStructureNodeEnd(node); }
    virtual bool visitDefsNodeStart(const QSvgDefs *node) { return visitStructureNodeStart(node); }
    virtual void visitDefsNodeEnd(const QSvgDefs *node)  { visitStructureNodeEnd(node); };
    virtual bool visitSwitchNodeStart(const QSvgSwitch *node) { return visitStructureNodeStart(node); }
    virtual void visitSwitchNodeEnd(const QSvgSwitch *node)  { visitStructureNodeEnd(node); };
    virtual bool visitMaskNodeStart(const QSvgMask *node) { return visitStructureNodeStart(node); }
    virtual void visitMaskNodeEnd(const QSvgMask *node) { visitStructureNodeEnd(node); }
};

QT_END_NAMESPACE

#endif // QSVGVISITOR_P_H
