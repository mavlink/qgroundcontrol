// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSVGSTYLESELECTOR_P_H
#define QSVGSTYLESELECTOR_P_H

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

#include <QtSvg/qtsvgglobal.h>
#include <QtGui/private/qcssparser_p.h>

QT_BEGIN_NAMESPACE

class QSvgNode;
class QSvgStructureNode;
class QSvgStyleSelector : public QCss::StyleSelector
{
public:
    QSvgStyleSelector();
    virtual ~QSvgStyleSelector();

    QString nodeToName(QSvgNode *node) const;

    QSvgNode *svgNode(NodePtr node) const;
    QSvgStructureNode *nodeToStructure(QSvgNode *n) const;

    QSvgStructureNode *svgStructure(NodePtr node) const;

    bool nodeNameEquals(NodePtr node, const QString& nodeName) const override;
    QString attributeValue(NodePtr node, const QCss::AttributeSelector &asel) const override;
    bool hasAttributes(NodePtr node) const override;

    QStringList nodeIds(NodePtr node) const override;

    QStringList nodeNames(NodePtr node) const override;

    bool isNullNode(NodePtr node) const override;

    NodePtr parentNode(NodePtr node) const override;
    NodePtr previousSiblingNode(NodePtr node) const override;
    NodePtr duplicateNode(NodePtr node) const override;
    void freeNode(NodePtr node) const override;
};

QT_END_NAMESPACE

#endif //QSVGSTYLESELECTOR_P_H
