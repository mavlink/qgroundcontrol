// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFDCONTAINER_P_H
#define QFDCONTAINER_P_H

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

#include <QtCore/qcompilerdetection.h>
#include <QtCore/qtclasshelpermacros.h>
#include <QtCore/qtconfigmacros.h>

QT_BEGIN_NAMESPACE

class QFdContainer
{
    int m_fd;
    Q_DISABLE_COPY_MOVE(QFdContainer)
public:
    Q_NODISCARD_CTOR explicit QFdContainer(int fd = -1) noexcept : m_fd(fd) {}
    ~QFdContainer() { reset(); }

    int get() const noexcept { return m_fd; }

    int release() noexcept { int result = m_fd; m_fd = -1; return result; }
    void reset() noexcept;
    void reset(int fd) { reset(); m_fd = fd; }
};

QT_END_NAMESPACE

#endif // QFDCONTAINER_P_H
