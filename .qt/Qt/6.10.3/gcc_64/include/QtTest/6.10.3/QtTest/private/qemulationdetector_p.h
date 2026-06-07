// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QEMULATIONDETECTOR_P_H
#define QEMULATIONDETECTOR_P_H

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

#include <QtCore/private/qglobal_p.h>

#if defined(Q_OS_LINUX) && defined(Q_PROCESSOR_ARM)
#define SHOULD_CHECK_ARM_ON_X86

#include <QFileInfo>

#if QT_CONFIG(process) && QT_CONFIG(regularexpression)
#include <QProcess>
#include <QRegularExpression>
#endif

#endif

QT_BEGIN_NAMESPACE

// Helper functions for detecting if running emulated
namespace QTestPrivate {

#ifdef SHOULD_CHECK_ARM_ON_X86
static bool isX86SpecificFileAvailable(void);
static bool isReportedArchitectureX86(void);
#endif

/*
 * Check if we are running Arm binary on x86 machine.
 *
 * Currently this is only able to check on Linux. If not able to
 * detect, return false.
 */
[[maybe_unused]] static bool isRunningArmOnX86()
{
#ifdef SHOULD_CHECK_ARM_ON_X86
    if (isX86SpecificFileAvailable())
        return true;

    if (isReportedArchitectureX86())
        return true;
#endif
    return false;
}

#ifdef SHOULD_CHECK_ARM_ON_X86
/*
 * Check if we can find a file that's only available on x86
 */
static bool isX86SpecificFileAvailable()
{
    using namespace Qt::StringLiterals;

    // MTRR (Memory Type Range Registers) are a feature of the x86 architecture
    // and /proc/mtrr is only present (on Linux) for that family.
    // However, it's an optional kernel feature, so the absence of the file is
    // not sufficient to conclude we're on real hardware.
    QFileInfo mtrr(u"/proc/mtrr"_s);
    if (mtrr.exists())
        return true;
    return false;
}

/*
 * Check if architecture reported by the OS is x86
 */
static bool isReportedArchitectureX86(void)
{
    using namespace Qt::StringLiterals;

#if QT_CONFIG(process) && QT_CONFIG(regularexpression)
    QProcess unamer;
    QString machineString;

    // Using syscall "uname" is not possible since that would be captured by
    // QEMU and result would be the architecture being emulated (e.g. armv7l).
    // By using QProcess we get the architecture used by the host.
    unamer.start(u"uname -a"_s);
    if (!unamer.waitForFinished()) {
        return false;
    }
    machineString = QString::fromLocal8Bit(unamer.readAll());

    // Is our current host cpu x86?
    if (machineString.contains(QRegularExpression(u"i386|i686|x86"_s))) {
        return true;
    }
#endif

    return false;
}
#endif // SHOULD_CHECK_ARM_ON_X86

} // QTestPrivate namespace

QT_END_NAMESPACE

#endif

