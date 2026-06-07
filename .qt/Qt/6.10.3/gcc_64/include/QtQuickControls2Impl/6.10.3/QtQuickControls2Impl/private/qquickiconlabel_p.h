// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKICONLABEL_P_H
#define QQUICKICONLABEL_P_H

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

#include <QtQuick/qquickitem.h>
#include <QtQuickControls2Impl/private/qtquickcontrols2implglobal_p.h>
#include <QtQuickTemplates2/private/qquickicon_p.h>

QT_BEGIN_NAMESPACE

class QQuickIconLabelPrivate;

class Q_QUICKCONTROLS2IMPL_EXPORT QQuickIconLabel : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QQuickIcon icon READ icon WRITE setIcon FINAL)
    Q_PROPERTY(QString text READ text WRITE setText FINAL)
    Q_PROPERTY(QFont font READ font WRITE setFont FINAL)
    Q_PROPERTY(QColor color READ color WRITE setColor FINAL)
    Q_PROPERTY(Display display READ display WRITE setDisplay FINAL)
    Q_PROPERTY(qreal spacing READ spacing WRITE setSpacing FINAL)
    Q_PROPERTY(bool mirrored READ isMirrored WRITE setMirrored FINAL)
    Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment FINAL)
    Q_PROPERTY(qreal topPadding READ topPadding WRITE setTopPadding RESET resetTopPadding FINAL)
    Q_PROPERTY(qreal leftPadding READ leftPadding WRITE setLeftPadding RESET resetLeftPadding FINAL)
    Q_PROPERTY(qreal rightPadding READ rightPadding WRITE setRightPadding RESET resetRightPadding FINAL)
    Q_PROPERTY(qreal bottomPadding READ bottomPadding WRITE setBottomPadding RESET resetBottomPadding FINAL)
    QML_NAMED_ELEMENT(IconLabel)
    QML_ADDED_IN_VERSION(2, 3)

public:
    enum Display {
        IconOnly,
        TextOnly,
        TextBesideIcon,
        TextUnderIcon
    };
    Q_ENUM(Display)

    explicit QQuickIconLabel(QQuickItem *parent = nullptr);
    ~QQuickIconLabel();

    QQuickIcon icon() const;
    void setIcon(const QQuickIcon &icon);

    QString text() const;
    void setText(const QString &text);

    QFont font() const;
    void setFont(const QFont &font);

    QColor color() const;
    void setColor(const QColor &color);

    Display display() const;
    void setDisplay(Display display);

    qreal spacing() const;
    void setSpacing(qreal spacing);

    bool isMirrored() const;
    void setMirrored(bool mirrored);

    Qt::Alignment alignment() const;
    void setAlignment(Qt::Alignment alignment);

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

protected:
    void componentComplete() override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private:
    Q_DISABLE_COPY(QQuickIconLabel)
    Q_DECLARE_PRIVATE(QQuickIconLabel)
};

QT_END_NAMESPACE

#endif // QQUICKICONLABEL_P_H
