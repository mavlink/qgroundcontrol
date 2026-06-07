// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QCOLOROUTPUT_H
#define QCOLOROUTPUT_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <qtqmlcompilerexports.h>

#include <QtCore/private/qglobal_p.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

class QColorOutputPrivate;

class Q_QMLCOMPILER_EXPORT QColorOutput
{
    enum
    {
        ForegroundShift = 10,
        BackgroundShift = 20,
        SpecialShift    = 20,
        ForegroundMask  = 0x1f << ForegroundShift,
        BackgroundMask  = 0x7 << BackgroundShift
    };

public:
    enum ColorCodeComponent
    {
        BlackForeground         = 1 << ForegroundShift,
        BlueForeground          = 2 << ForegroundShift,
        GreenForeground         = 3 << ForegroundShift,
        CyanForeground          = 4 << ForegroundShift,
        RedForeground           = 5 << ForegroundShift,
        PurpleForeground        = 6 << ForegroundShift,
        BrownForeground         = 7 << ForegroundShift,
        LightGrayForeground     = 8 << ForegroundShift,
        DarkGrayForeground      = 9 << ForegroundShift,
        LightBlueForeground     = 10 << ForegroundShift,
        LightGreenForeground    = 11 << ForegroundShift,
        LightCyanForeground     = 12 << ForegroundShift,
        LightRedForeground      = 13 << ForegroundShift,
        LightPurpleForeground   = 14 << ForegroundShift,
        YellowForeground        = 15 << ForegroundShift,
        WhiteForeground         = 16 << ForegroundShift,

        BlackBackground         = 1 << BackgroundShift,
        BlueBackground          = 2 << BackgroundShift,
        GreenBackground         = 3 << BackgroundShift,
        CyanBackground          = 4 << BackgroundShift,
        RedBackground           = 5 << BackgroundShift,
        PurpleBackground        = 6 << BackgroundShift,
        BrownBackground         = 7 << BackgroundShift,
        DefaultColor            = 1 << SpecialShift
    };

    using ColorCode = QFlags<ColorCodeComponent>;
    using ColorMapping = QHash<int, ColorCode>;

    QColorOutput();
    ~QColorOutput();

    bool isSilent() const;
    void setSilent(bool silent);

    void insertMapping(int colorID, ColorCode colorCode);

    void writeUncolored(const QString &message);
    void write(const QStringView message, int color = -1);
    // handle QStringBuilder case
    Q_WEAK_OVERLOAD void write(const QString &message, int color = -1) { write(QStringView(message), color); }
    void writePrefixedMessage(const QString &message, QtMsgType type,
                              const QString &prefix = QString());
    QString colorify(QStringView message, int color = -1) const;

    void flushBuffer();
    void discardBuffer();

private:
    QScopedPointer<QColorOutputPrivate> d;
    Q_DISABLE_COPY_MOVE(QColorOutput)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QColorOutput::ColorCode)

QT_END_NAMESPACE

#endif // QCOLOROUTPUT_H
