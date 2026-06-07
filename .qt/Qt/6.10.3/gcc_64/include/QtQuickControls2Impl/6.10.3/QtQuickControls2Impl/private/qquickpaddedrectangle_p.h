// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKPADDEDRECTANGLE_P_H
#define QQUICKPADDEDRECTANGLE_P_H

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

#include <QtQuick/private/qquickrectangle_p.h>
#include <QtQuickControls2Impl/private/qtquickcontrols2implglobal_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICKCONTROLS2IMPL_EXPORT QQuickPaddedRectangle : public QQuickRectangle
{
    Q_OBJECT
    Q_PROPERTY(qreal padding READ padding WRITE setPadding RESET resetPadding NOTIFY paddingChanged FINAL)
    Q_PROPERTY(qreal topPadding READ topPadding WRITE setTopPadding RESET resetTopPadding NOTIFY topPaddingChanged FINAL)
    Q_PROPERTY(qreal leftPadding READ leftPadding WRITE setLeftPadding RESET resetLeftPadding NOTIFY leftPaddingChanged FINAL)
    Q_PROPERTY(qreal rightPadding READ rightPadding WRITE setRightPadding RESET resetRightPadding NOTIFY rightPaddingChanged FINAL)
    Q_PROPERTY(qreal bottomPadding READ bottomPadding WRITE setBottomPadding RESET resetBottomPadding NOTIFY bottomPaddingChanged FINAL)
    QML_NAMED_ELEMENT(PaddedRectangle)
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickPaddedRectangle(QQuickItem *parent = nullptr);

    qreal padding() const;
    void setPadding(qreal padding);
    void resetPadding();

    qreal topPadding() const;
    void setTopPadding(qreal padding);
    void resetTopPadding();

    qreal leftPadding() const;
    void setLeftPadding(qreal padding);
    void resetLeftPadding();

    qreal rightPadding() const;
    void setRightPadding(qreal padding);
    void resetRightPadding();

    qreal bottomPadding() const;
    void setBottomPadding(qreal padding);
    void resetBottomPadding();

Q_SIGNALS:
    void paddingChanged();
    void topPaddingChanged();
    void leftPaddingChanged();
    void rightPaddingChanged();
    void bottomPaddingChanged();

protected:
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;

private:
    void setTopPadding(qreal padding, bool has);
    void setLeftPadding(qreal padding, bool has);
    void setRightPadding(qreal padding, bool has);
    void setBottomPadding(qreal padding, bool has);

    qreal m_padding = 0;
    qreal m_topPadding = 0;
    qreal m_leftPadding = 0;
    qreal m_rightPadding = 0;
    qreal m_bottomPadding = 0;
    bool m_hasTopPadding = false;
    bool m_hasLeftPadding = false;
    bool m_hasRightPadding = false;
    bool m_hasBottomPadding = false;
};

QT_END_NAMESPACE

#endif // QQUICKPADDEDRECTANGLE_P_H
