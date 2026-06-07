// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSHADEREFFECT_P_P_H
#define QQUICKSHADEREFFECT_P_P_H

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

#include <private/qtquickglobal_p.h>

QT_REQUIRE_CONFIG(quick_shadereffect);

#include <private/qquickshadereffect_p.h>
#include <private/qquickitem_p.h>
#include <private/qquickshadereffectmesh_p.h>

QT_BEGIN_NAMESPACE

namespace QtPrivate {
class EffectSlotMapper;
}

class QQuickShaderEffectPrivate : public QQuickItemPrivate
{
    Q_DECLARE_PUBLIC(QQuickShaderEffect)

public:
    QQuickShaderEffectPrivate();
    ~QQuickShaderEffectPrivate();

    void updatePolish() override;

    QUrl fragmentShader() const { return m_fragShader; }
    void setFragmentShader(const QUrl &fileUrl);

    QUrl vertexShader() const { return m_vertShader; }
    void setVertexShader(const QUrl &fileUrl);

    bool blending() const { return m_blending; }
    void setBlending(bool enable);

    QVariant mesh() const;
    void setMesh(const QVariant &mesh);

    QQuickShaderEffect::CullMode cullMode() const { return m_cullMode; }
    void setCullMode(QQuickShaderEffect::CullMode face);

    QString log() const;
    QQuickShaderEffect::Status status() const;

    bool supportsAtlasTextures() const { return m_supportsAtlasTextures; }
    void setSupportsAtlasTextures(bool supports);

    QString parseLog();

    void handleEvent(QEvent *);
    void handleGeometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry);
    QSGNode *handleUpdatePaintNode(QSGNode *, QQuickItem::UpdatePaintNodeData *);
    void handleComponentComplete();
    void handleItemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &value);
    void maybeUpdateShaders();
    bool updateUniformValue(const QByteArray &name, const QVariant &value,
                            QSGShaderEffectNode *node);

    void propertyChanged(int mappedId);
    void sourceDestroyed(QObject *object);
    void markGeometryDirtyAndUpdate();
    void markGeometryDirtyAndUpdateIfSupportsAtlas();
    void shaderCodePrepared(bool ok, QSGGuiThreadShaderEffectManager::ShaderInfo::Type typeHint,
                            const QUrl &loadUrl, QSGGuiThreadShaderEffectManager::ShaderInfo *result);

private:
    QSGGuiThreadShaderEffectManager *shaderEffectManager() const;

    enum Shader {
        Vertex,
        Fragment,

        NShader
    };
    bool updateShader(Shader shaderType, const QUrl &fileUrl);
    void updateShaderVars(Shader shaderType);
    void disconnectSignals(Shader shaderType);
    void clearMappers(Shader shaderType);
    std::optional<int> findMappedShaderVariableId(const QByteArray &name) const;
    std::optional<int> findMappedShaderVariableId(const QByteArray &name, Shader shaderType) const;
    bool sourceIsUnique(QQuickItem *source, Shader typeToSkip, int indexToSkip) const;

    bool inDestructor = false;
    const QMetaObject *m_itemMetaObject = nullptr;
    QSize m_meshResolution;
    QQuickShaderEffectMesh *m_mesh;
    QMetaObject::Connection m_meshConnection;
    QQuickGridMesh m_defaultMesh;
    QQuickShaderEffect::CullMode m_cullMode;
    bool m_blending;
    bool m_supportsAtlasTextures;
    mutable QSGGuiThreadShaderEffectManager *m_mgr;
    QUrl m_fragShader;
    bool m_fragNeedsUpdate;
    QUrl m_vertShader;
    bool m_vertNeedsUpdate;

    QSGShaderEffectNode::ShaderData m_shaders[NShader];
    QSGShaderEffectNode::DirtyShaderFlags m_dirty;
    QSet<int> m_dirtyConstants[NShader];
    QSet<int> m_dirtyTextures[NShader];
    QSGGuiThreadShaderEffectManager::ShaderInfo *m_inProgress[NShader];

    QVector<QtPrivate::EffectSlotMapper *> m_mappers[NShader];

    QHash<QQuickItem *, QMetaObject::Connection> m_destroyedConnections;
};

QT_END_NAMESPACE

#endif // QQUICKSHADEREFFECT_P_P_H
