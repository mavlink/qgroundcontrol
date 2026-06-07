// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2014 John Layt <jlayt@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QPRINT_P_H
#define QPRINT_P_H

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

#include <QtPrintSupport/private/qtprintsupportglobal_p.h>
#include <QtPrintSupport/qprinter.h>

#include <QtCore/qstring.h>
#include <QtCore/qlist.h>

#if (defined Q_OS_MACOS) || (defined Q_OS_UNIX && QT_CONFIG(cups))
#include <cups/ppd.h>  // Use for type defs only, don't want to actually link in main module
// ### QT_DECL_METATYPE_EXTERN_TAGGED once there's a qprint.cpp TU
Q_DECLARE_METATYPE(ppd_file_t *)
#endif

QT_BEGIN_NAMESPACE

#ifndef QT_NO_PRINTER

// From windgdi.h
#define DMBIN_UPPER           1
#define DMBIN_ONLYONE         1
#define DMBIN_LOWER           2
#define DMBIN_MIDDLE          3
#define DMBIN_MANUAL          4
#define DMBIN_ENVELOPE        5
#define DMBIN_ENVMANUAL       6
#define DMBIN_AUTO            7
#define DMBIN_TRACTOR         8
#define DMBIN_SMALLFMT        9
#define DMBIN_LARGEFMT       10
#define DMBIN_LARGECAPACITY  11
#define DMBIN_CASSETTE       14
#define DMBIN_FORMSOURCE     15
#define DMBIN_USER          256

namespace QPrint {

    // Note: Keep in sync with QPrinter::PrinterState for now
    // Replace later with more detailed status reporting
    enum DeviceState {
        Idle,
        Active,
        Aborted,
        Error
    };

    // Note: Keep in sync with QPrinter::DuplexMode
    enum DuplexMode {
        DuplexNone = 0,
        DuplexAuto,
        DuplexLongSide,
        DuplexShortSide
    };

    // Note: Keep in sync with QPrinter::ColorMode
    enum ColorMode {
        GrayScale,
        Color
    };

    // Note: Keep in sync with QPrinter::PaperSource for now
    // If/when made public, rearrange and rename
    enum InputSlotId {
        Upper,
        Lower,
        Middle,
        Manual,
        Envelope,
        EnvelopeManual,
        Auto,
        Tractor,
        SmallFormat,
        LargeFormat,
        LargeCapacity,
        Cassette,
        FormSource,
        MaxPageSource, // Deprecated, kept for compatibility to QPrinter
        CustomInputSlot,
        LastInputSlot = CustomInputSlot,
        OnlyOne = Upper
    };

    struct InputSlot {
        QByteArray key;
        QString name;
        QPrint::InputSlotId id;
        int windowsId;
    };

    enum OutputBinId {
        AutoOutputBin,
        UpperBin,
        LowerBin,
        RearBin,
        CustomOutputBin,
        LastOutputBin = CustomOutputBin
    };

    struct OutputBin {
        QByteArray key;
        QString name;
        QPrint::OutputBinId id;
    };

}

struct InputSlotMap {
    QPrint::InputSlotId id;
    int windowsId;
    const char *key;
};

struct OutputBinMap {
    QPrint::OutputBinId id;
    const char *key;
};

// Print utilities shared by print plugins

namespace QPrintUtils {

Q_PRINTSUPPORT_EXPORT QPrint::InputSlotId inputSlotKeyToInputSlotId(const QByteArray &key);
Q_PRINTSUPPORT_EXPORT QByteArray inputSlotIdToInputSlotKey(QPrint::InputSlotId id);
Q_PRINTSUPPORT_EXPORT int inputSlotIdToWindowsId(QPrint::InputSlotId id);
Q_PRINTSUPPORT_EXPORT QPrint::OutputBinId outputBinKeyToOutputBinId(const QByteArray &key);
Q_PRINTSUPPORT_EXPORT QByteArray outputBinIdToOutputBinKey(QPrint::OutputBinId id);
Q_PRINTSUPPORT_EXPORT QPrint::InputSlot paperBinToInputSlot(int windowsId, const QString &name);

#    if (defined Q_OS_MACOS) || (defined Q_OS_UNIX && QT_CONFIG(cups))
// PPD utilities shared by CUPS and Mac plugins requiring CUPS headers
// May turn into a proper internal QPpd class if enough shared between Mac and CUPS,
// but where would it live?  Not in base module as don't want to link to CUPS.
// May have to have two copies in plugins to keep in sync.
Q_PRINTSUPPORT_EXPORT QPrint::InputSlot ppdChoiceToInputSlot(const ppd_choice_t &choice);
Q_PRINTSUPPORT_EXPORT QPrint::OutputBin ppdChoiceToOutputBin(const ppd_choice_t &choice);
Q_PRINTSUPPORT_EXPORT int parsePpdResolution(const QByteArray &value);
Q_PRINTSUPPORT_EXPORT QPrint::DuplexMode ppdChoiceToDuplexMode(const QByteArray &choice);
#    endif // Mac and CUPS PPD Utilities
};

#endif // QT_NO_PRINTER

QT_END_NAMESPACE

#endif // QPRINT_P_H
