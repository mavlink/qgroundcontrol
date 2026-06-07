// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKGENERATOR_P_H
#define QQUICKGENERATOR_P_H

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

#include <private/qquickvectorimageglobal_p.h>
#include <QtCore/qstring.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qrect.h>

QT_BEGIN_NAMESPACE

QT_DECLARE_EXPORTED_QT_LOGGING_CATEGORY(lcQuickVectorImage, Q_QUICKVECTORIMAGEGENERATOR_EXPORT)

class QPainterPath;
class QGradient;
class QQuickShapePath;
class QQuadPath;
class QQuickItem;
class QQuickShape;
class QRectF;

namespace QQuickVectorImageGenerator {
struct NodeInfo;
struct ImageNodeInfo;
struct PathNodeInfo;
struct TextNodeInfo;
struct UseNodeInfo;
struct StructureNodeInfo;
struct AnimateColorNodeInfo;
}

using namespace QQuickVectorImageGenerator;

class Q_QUICKVECTORIMAGEGENERATOR_EXPORT QQuickGenerator
{
public:
    QQuickGenerator(const QString fileName, QQuickVectorImageGenerator::GeneratorFlags flags);
    virtual ~QQuickGenerator();

    void setGeneratorFlags(QQuickVectorImageGenerator::GeneratorFlags flags);
    QQuickVectorImageGenerator::GeneratorFlags generatorFlags();

    bool generate();

    virtual QString generateNodeBase(const NodeInfo &info) = 0;
    virtual bool generateDefsNode(const NodeInfo &info) = 0;
    virtual void generateImageNode(const ImageNodeInfo &info) = 0;
    virtual void generatePath(const PathNodeInfo &info, const QRectF &overrideBoundingRect = QRectF{}) = 0;
    virtual void generateNode(const NodeInfo &info) = 0;
    virtual void generateTextNode(const TextNodeInfo &info) = 0;
    virtual void generateUseNode(const UseNodeInfo &info) = 0;
    virtual bool generateStructureNode(const StructureNodeInfo &info) = 0;
    virtual bool generateRootNode(const StructureNodeInfo &info) = 0;
    virtual void outputShapePath(const PathNodeInfo &info, const QPainterPath *path, const QQuadPath *quadPath, QQuickVectorImageGenerator::PathSelector pathSelector, const QRectF &boundingRect) = 0;
    void optimizePaths(const PathNodeInfo &info, const QRectF &overrideBoundingRect);
    bool isNodeVisible(const NodeInfo &info);

protected:
    QQuickVectorImageGenerator::GeneratorFlags m_flags;

private:
    QString m_fileName;
};

QT_END_NAMESPACE

#endif // QQUICKGENERATOR_P_H
