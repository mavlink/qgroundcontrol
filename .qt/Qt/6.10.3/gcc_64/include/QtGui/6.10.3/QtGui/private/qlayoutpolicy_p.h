// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QLAYOUTPOLICY_H
#define QLAYOUTPOLICY_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include <QtCore/qobject.h>
#include <QtCore/qnamespace.h>

#ifndef QT_NO_DATASTREAM
# include <QtCore/qdatastream.h>
#endif

QT_BEGIN_NAMESPACE


class QVariant;

class QLayoutPolicy
{
    Q_GADGET_EXPORT(Q_GUI_EXPORT)

public:
    enum PolicyFlag {
        GrowFlag = 1,
        ExpandFlag = 2,
        ShrinkFlag = 4,
        IgnoreFlag = 8
    };
    Q_DECLARE_FLAGS(Policy, PolicyFlag)
    Q_FLAG(Policy)

    static constexpr inline Policy Fixed = {};
    static constexpr inline Policy Minimum = GrowFlag;
    static constexpr inline Policy Maximum = ShrinkFlag;
    static constexpr inline Policy Preferred = Minimum | Maximum;
    static constexpr inline Policy MinimumExpanding = Minimum | ExpandFlag;
    static constexpr inline Policy Expanding = Preferred | ExpandFlag;
    static constexpr inline Policy Ignored = Preferred | IgnoreFlag;

    enum ControlType {
        DefaultType      = 0x00000001,
        ButtonBox        = 0x00000002,
        CheckBox         = 0x00000004,
        ComboBox         = 0x00000008,
        Frame            = 0x00000010,
        GroupBox         = 0x00000020,
        Label            = 0x00000040,
        Line             = 0x00000080,
        LineEdit         = 0x00000100,
        PushButton       = 0x00000200,
        RadioButton      = 0x00000400,
        Slider           = 0x00000800,
        SpinBox          = 0x00001000,
        TabWidget        = 0x00002000,
        ToolButton       = 0x00004000
    };
    Q_DECLARE_FLAGS(ControlTypes, ControlType)

    QLayoutPolicy() : data(0) { }

    QLayoutPolicy(Policy horizontal, Policy vertical, ControlType type = DefaultType)
        : data(0) {
        bits.horPolicy = horizontal;
        bits.verPolicy = vertical;
        setControlType(type);
    }
    Policy horizontalPolicy() const { return static_cast<Policy>(bits.horPolicy); }
    Policy verticalPolicy() const { return static_cast<Policy>(bits.verPolicy); }
    Q_GUI_EXPORT ControlType controlType() const;

    void setHorizontalPolicy(Policy d) { bits.horPolicy = d; }
    void setVerticalPolicy(Policy d) { bits.verPolicy = d; }
    Q_GUI_EXPORT void setControlType(ControlType type);

    Qt::Orientations expandingDirections() const {
        Qt::Orientations result;
        if (verticalPolicy() & ExpandFlag)
            result |= Qt::Vertical;
        if (horizontalPolicy() & ExpandFlag)
            result |= Qt::Horizontal;
        return result;
    }

    void setHeightForWidth(bool b) { bits.hfw = b;  }
    bool hasHeightForWidth() const { return bits.hfw; }
    void setWidthForHeight(bool b) { bits.wfh = b;  }
    bool hasWidthForHeight() const { return bits.wfh; }

    bool operator==(const QLayoutPolicy& s) const { return data == s.data; }
    bool operator!=(const QLayoutPolicy& s) const { return data != s.data; }

    int horizontalStretch() const { return static_cast<int>(bits.horStretch); }
    int verticalStretch() const { return static_cast<int>(bits.verStretch); }
    void setHorizontalStretch(int stretchFactor) { bits.horStretch = static_cast<quint32>(qBound(0, stretchFactor, 255)); }
    void setVerticalStretch(int stretchFactor) { bits.verStretch = static_cast<quint32>(qBound(0, stretchFactor, 255)); }

    inline void transpose();


private:
#ifndef QT_NO_DATASTREAM
    friend QDataStream &operator<<(QDataStream &, const QLayoutPolicy &);
    friend QDataStream &operator>>(QDataStream &, QLayoutPolicy &);
#endif
    QLayoutPolicy(int i) : data(i) { }

    union {
        struct {
            quint32 horStretch : 8;
            quint32 verStretch : 8;
            quint32 horPolicy : 4;
            quint32 verPolicy : 4;
            quint32 ctype : 5;
            quint32 hfw : 1;
            quint32 wfh : 1;
            quint32 padding : 1;   // feel free to use
        } bits;
        quint32 data;
    };
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QLayoutPolicy::Policy)
Q_DECLARE_OPERATORS_FOR_FLAGS(QLayoutPolicy::ControlTypes)

#ifndef QT_NO_DATASTREAM
QDataStream &operator<<(QDataStream &, const QLayoutPolicy &);
QDataStream &operator>>(QDataStream &, QLayoutPolicy &);
#endif

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QLayoutPolicy &);
#endif

inline void QLayoutPolicy::transpose() {
    Policy hData = horizontalPolicy();
    Policy vData = verticalPolicy();
    int hStretch = horizontalStretch();
    int vStretch = verticalStretch();
    setHorizontalPolicy(vData);
    setVerticalPolicy(hData);
    setHorizontalStretch(vStretch);
    setVerticalStretch(hStretch);
}

QT_END_NAMESPACE

#endif // QLAYOUTPOLICY_H
