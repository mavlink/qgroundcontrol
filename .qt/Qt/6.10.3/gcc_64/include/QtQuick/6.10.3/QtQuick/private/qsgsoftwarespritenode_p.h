// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGSOFTWARESPRITENODE_H
#define QSGSOFTWARESPRITENODE_H

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

QT_REQUIRE_CONFIG(quick_sprite);

#include <private/qsgadaptationlayer_p.h>

QT_BEGIN_NAMESPACE

class QSGSoftwarePixmapTexture;
class QSGSoftwareSpriteNode : public QSGSpriteNode
{
public:
    QSGSoftwareSpriteNode();
    ~QSGSoftwareSpriteNode() override;

    void setTexture(QSGTexture *texture) override;
    void setTime(float time) override;
    void setSourceA(const QPoint &source) override;
    void setSourceB(const QPoint &source) override;
    void setSpriteSize(const QSize &size) override;
    void setSheetSize(const QSize &size) override;
    void setSize(const QSizeF &size) override;
    void setFiltering(QSGTexture::Filtering filtering) override;
    void update() override;

    void paint(QPainter *painter);
    bool isOpaque() const;
    QRectF rect() const;

private:

    QSGSoftwarePixmapTexture *m_texture = nullptr;
    float m_time;
    QPoint m_sourceA;
    QPoint m_sourceB;
    QSize m_spriteSize;
    QSize m_sheetSize;
    QSizeF m_size;

};

QT_END_NAMESPACE

#endif // QSGSOFTWARESPRITENODE_H
