// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTNOOP_H
#define QTNOOP_H

#if 0
#pragma qt_sync_stop_processing
#endif

#ifdef __cplusplus
[[maybe_unused]] constexpr
#endif
static inline void qt_noop(void)
#ifdef __cplusplus
    noexcept
#endif
{}

#endif // QTNOOP_H
