// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGSOFTWARERENDERER_H
#define QSGSOFTWARERENDERER_H

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

#include "qsgabstractsoftwarerenderer_p.h"

QT_BEGIN_NAMESPACE

class QPaintDevice;
class QBackingStore;

class Q_QUICK_EXPORT QSGSoftwareRenderer : public QSGAbstractSoftwareRenderer
{
public:
    QSGSoftwareRenderer(QSGRenderContext *context);
    virtual ~QSGSoftwareRenderer();

    void setCurrentPaintDevice(QPaintDevice *device);
    QPaintDevice *currentPaintDevice() const;
    void setBackingStore(QBackingStore *backingStore);
    QRegion flushRegion() const;

protected:
    void renderScene() final;
    void render() final;

private:
    enum PartialUpdateMode {
        AutoPartialUpdate,
        ForcePartialUpdate,
        DisablePartialUpdate
    } m_partialUpdateMode = AutoPartialUpdate;

    QPaintDevice* m_paintDevice;
    QBackingStore* m_backingStore;
    QRegion m_flushRegion;
};

QT_END_NAMESPACE

#endif // QSGSOFTWARERENDERER_H
