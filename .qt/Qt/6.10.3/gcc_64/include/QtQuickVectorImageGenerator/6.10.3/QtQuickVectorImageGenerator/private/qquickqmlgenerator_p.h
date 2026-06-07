// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKQMLGENERATOR_P_H
#define QQUICKQMLGENERATOR_P_H

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

#include "qquickgenerator_p.h"
#include "qquicknodeinfo_p.h"
#include "qquickanimatedproperty_p.h"

#include <QtCore/qtextstream.h>
#include <QtCore/qbuffer.h>

QT_BEGIN_NAMESPACE

class Q_QUICKVECTORIMAGEGENERATOR_EXPORT QQuickQmlGenerator : public QQuickGenerator
{
public:
    QQuickQmlGenerator(const QString fileName, QQuickVectorImageGenerator::GeneratorFlags flags, const QString &outFileName);
    ~QQuickQmlGenerator();

    bool save();

    void setShapeTypeName(const QString &name);
    QString shapeTypeName() const;

    void setCommentString(const QString commentString);
    QString commentString() const;

    void setRetainFilePaths(bool retainFilePaths)
    {
        m_retainFilePaths = retainFilePaths;
    }

    bool retainFilePaths() const
    {
        return m_retainFilePaths;
    }

    void setAssetFileDirectory(const QString &assetFileDirectory)
    {
        m_assetFileDirectory = assetFileDirectory;
    }

    QString assetFileDirectory() const
    {
        return m_assetFileDirectory;
    }

    void setAssetFilePrefix(const QString &assetFilePrefix)
    {
        m_assetFilePrefix = assetFilePrefix;
    }

    QString assetFilePrefix() const
    {
        return m_assetFilePrefix;
    }

    void setUrlPrefix(const QString &prefix)
    {
        m_urlPrefix = prefix;
    }

    QString urlPrefix() const
    {
        return m_urlPrefix;
    }

    void addExtraImport(const QString &import)
    {
        m_extraImports.append(import);
    }

    QStringList extraImports() const
    {
        return m_extraImports;
    }

    bool isRuntimeGenerator() const
    {
        return !m_urlPrefix.isEmpty();
    }

protected:
    QString generateNodeBase(const NodeInfo &info) override;
    bool generateDefsNode(const NodeInfo &info) override;
    void generateImageNode(const ImageNodeInfo &info) override;
    void generatePath(const PathNodeInfo &info, const QRectF &overrideBoundingRect) override;
    void generateNode(const NodeInfo &info) override;
    void generateTextNode(const TextNodeInfo &info) override;
    void generateUseNode(const UseNodeInfo &info) override;
    bool generateStructureNode(const StructureNodeInfo &info) override;
    bool generateRootNode(const StructureNodeInfo &info) override;
    void outputShapePath(const PathNodeInfo &info, const QPainterPath *path, const QQuadPath *quadPath, QQuickVectorImageGenerator::PathSelector pathSelector, const QRectF &boundingRect) override;

private:
    enum class AnimationType {
        Auto = 0,
        ColorOpacity = 1
    };

    void generateGradient(const QGradient *grad);
    void generateTransform(const QTransform &xf);
    void generatePathContainer(const StructureNodeInfo &info);
    void generateAnimateTransform(const QString &targetName, const NodeInfo &info);
    void generateAnimationBindings();
    void generateEasing(const QQuickAnimatedProperty::PropertyAnimation &animation, int time);
    void generateAnimatedPropertySetter(const QString &targetName,
                                        const QString &propertyName,
                                        const QVariant &value,
                                        const QQuickAnimatedProperty::PropertyAnimation &animation,
                                        int time,
                                        int frameTime,
                                        AnimationType animationType = AnimationType::Auto);
    void generatePropertyAnimation(const QQuickAnimatedProperty &property,
                                   const QString &targetName,
                                   const QString &propertyName,
                                   AnimationType animationType = AnimationType::Auto);

    QStringView indent();
    enum StreamFlags { NoFlags = 0x0, SameLine = 0x1 };
    QTextStream &stream(int flags = NoFlags);
    const char *shapeName() const;

protected:
    QBuffer m_result;

private:
    int m_indentLevel = 0;
    QTextStream m_stream;
    QString outputFileName;
    int m_inShapeItemLevel = 0;
    QByteArray m_shapeTypeName;
    QString m_commentString;
    bool m_retainFilePaths = false;
    QString m_assetFileDirectory;
    QString m_assetFilePrefix;
    QString m_urlPrefix;
    QString m_topLevelIdString;
    QStringList m_extraImports;
};

QT_END_NAMESPACE

#endif // QQUICKQMLGENERATOR_P_H
