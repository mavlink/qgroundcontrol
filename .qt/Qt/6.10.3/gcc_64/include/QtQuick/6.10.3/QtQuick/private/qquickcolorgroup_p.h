// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default
#ifndef QQUICKCOLORGROUP_H
#define QQUICKCOLORGROUP_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of QQuickColorGroup. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/qpalette.h>

#include <QtCore/private/qobject_p.h>

#include <QtQuick/private/qtquickglobal_p.h>

#include <QtQml/qqml.h>

#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

class QQuickPalette;
class QQuickPaletteColorProvider;

class Q_QUICK_EXPORT QQuickColorGroup : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QColor alternateBase   READ alternateBase   WRITE setAlternateBase   RESET resetAlternateBase   NOTIFY alternateBaseChanged   FINAL)
    Q_PROPERTY(QColor base            READ base            WRITE setBase            RESET resetBase            NOTIFY baseChanged            FINAL)
    Q_PROPERTY(QColor brightText      READ brightText      WRITE setBrightText      RESET resetBrightText      NOTIFY brightTextChanged      FINAL)
    Q_PROPERTY(QColor button          READ button          WRITE setButton          RESET resetButton          NOTIFY buttonChanged          FINAL)
    Q_PROPERTY(QColor buttonText      READ buttonText      WRITE setButtonText      RESET resetButtonText      NOTIFY buttonTextChanged      FINAL)
    Q_PROPERTY(QColor dark            READ dark            WRITE setDark            RESET resetDark            NOTIFY darkChanged            FINAL)
    Q_PROPERTY(QColor highlight       READ highlight       WRITE setHighlight       RESET resetHighlight       NOTIFY highlightChanged       FINAL)
    Q_PROPERTY(QColor highlightedText READ highlightedText WRITE setHighlightedText RESET resetHighlightedText NOTIFY highlightedTextChanged FINAL)
    Q_PROPERTY(QColor light           READ light           WRITE setLight           RESET resetLight           NOTIFY lightChanged           FINAL)
    Q_PROPERTY(QColor link            READ link            WRITE setLink            RESET resetLink            NOTIFY linkChanged            FINAL)
    Q_PROPERTY(QColor linkVisited     READ linkVisited     WRITE setLinkVisited     RESET resetLinkVisited     NOTIFY linkVisitedChanged     FINAL)
    Q_PROPERTY(QColor mid             READ mid             WRITE setMid             RESET resetMid             NOTIFY midChanged             FINAL)
    Q_PROPERTY(QColor midlight        READ midlight        WRITE setMidlight        RESET resetMidlight        NOTIFY midlightChanged        FINAL)
    Q_PROPERTY(QColor shadow          READ shadow          WRITE setShadow          RESET resetShadow          NOTIFY shadowChanged          FINAL)
    Q_PROPERTY(QColor text            READ text            WRITE setText            RESET resetText            NOTIFY textChanged            FINAL)
    Q_PROPERTY(QColor toolTipBase     READ toolTipBase     WRITE setToolTipBase     RESET resetToolTipBase     NOTIFY toolTipBaseChanged     FINAL)
    Q_PROPERTY(QColor toolTipText     READ toolTipText     WRITE setToolTipText     RESET resetToolTipText     NOTIFY toolTipTextChanged     FINAL)
    Q_PROPERTY(QColor window          READ window          WRITE setWindow          RESET resetWindow          NOTIFY windowChanged          FINAL)
    Q_PROPERTY(QColor windowText      READ windowText      WRITE setWindowText      RESET resetWindowText      NOTIFY windowTextChanged      FINAL)
    Q_PROPERTY(QColor placeholderText READ placeholderText WRITE setPlaceholderText
               RESET resetPlaceholderText NOTIFY placeholderTextChanged REVISION(6, 2) FINAL)
    Q_PROPERTY(QColor accent READ accent WRITE setAccent RESET resetAccent NOTIFY accentChanged REVISION(6, 6) FINAL)

    QML_NAMED_ELEMENT(ColorGroup)
    QML_ADDED_IN_VERSION(6, 0)

public: // Types
    using GroupPtr = QPointer<QQuickColorGroup>;

public:
    Q_DISABLE_COPY_MOVE(QQuickColorGroup)

    explicit QQuickColorGroup(QObject *parent = nullptr);

    QColor alternateBase() const;
    void setAlternateBase(const QColor &color);
    void resetAlternateBase();

    QColor base() const;
    void setBase(const QColor &color);
    void resetBase();

    QColor brightText() const;
    void setBrightText(const QColor &color);
    void resetBrightText();

    QColor button() const;
    void setButton(const QColor &color);
    void resetButton();

    QColor buttonText() const;
    void setButtonText(const QColor &color);
    void resetButtonText();

    QColor dark() const;
    void setDark(const QColor &color);
    void resetDark();

    QColor highlight() const;
    void setHighlight(const QColor &color);
    void resetHighlight();

    QColor highlightedText() const;
    void setHighlightedText(const QColor &color);
    void resetHighlightedText();

    QColor light() const;
    void setLight(const QColor &color);
    void resetLight();

    QColor link() const;
    void setLink(const QColor &color);
    void resetLink();

    QColor linkVisited() const;
    void setLinkVisited(const QColor &color);
    void resetLinkVisited();

    QColor mid() const;
    void setMid(const QColor &color);
    void resetMid();

    QColor midlight() const;
    void setMidlight(const QColor &color);
    void resetMidlight();

    QColor shadow() const;
    void setShadow(const QColor &color);
    void resetShadow();

    QColor text() const;
    void setText(const QColor &color);
    void resetText();

    QColor toolTipBase() const;
    void setToolTipBase(const QColor &color);
    void resetToolTipBase();

    QColor toolTipText() const;
    void setToolTipText(const QColor &color);
    void resetToolTipText();

    QColor window() const;
    void setWindow(const QColor &color);
    void resetWindow();

    QColor windowText() const;
    void setWindowText(const QColor &color);
    void resetWindowText();

    QColor placeholderText() const;
    void setPlaceholderText(const QColor &color);
    void resetPlaceholderText();

    QColor accent() const;
    void setAccent(const QColor &color);
    void resetAccent();

    QPalette::ColorGroup groupTag() const;
    void setGroupTag(QPalette::ColorGroup tag);

    const QQuickPaletteColorProvider &colorProvider() const;
    QQuickPaletteColorProvider &colorProvider();

    static QQuickColorGroup* createWithParent(QQuickPalette &parent);

Q_SIGNALS:
    void alternateBaseChanged();
    void baseChanged();
    void brightTextChanged();
    void buttonChanged();
    void buttonTextChanged();
    void darkChanged();
    void highlightChanged();
    void highlightedTextChanged();
    void lightChanged();
    void linkChanged();
    void linkVisitedChanged();
    void midChanged();
    void midlightChanged();
    void shadowChanged();
    void textChanged();
    void toolTipBaseChanged();
    void toolTipTextChanged();
    void windowChanged();
    void windowTextChanged();
    Q_REVISION(6, 2) void placeholderTextChanged();
    Q_REVISION(6, 6) void accentChanged();

    void changed();

protected:
    explicit QQuickColorGroup(QQuickPalette &parent);

    static constexpr QPalette::ColorGroup defaultGroupTag() { return QPalette::All; }

    virtual QPalette::ColorGroup currentColorGroup() const;

private:
    using Notifier = void (QQuickColorGroup::* )();

    QColor color(QPalette::ColorRole role) const;
    void setColor(QPalette::ColorRole role, QColor color, Notifier notifier);
    void resetColor(QPalette::ColorRole role, Notifier notifier);

private:
    QPalette::ColorGroup m_groupTag;
    std::shared_ptr<QQuickPaletteColorProvider> m_colorProvider;
};

QT_END_NAMESPACE

#endif // QQUICKCOLORGROUP_H
