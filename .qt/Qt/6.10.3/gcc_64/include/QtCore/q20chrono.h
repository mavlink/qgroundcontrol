// Copyright (C) 2023 Ahmad Samir <a.samirh78@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef Q20CHRONO_H
#define Q20CHRONO_H

#include <QtCore/qtconfigmacros.h>

#include <chrono>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. Types and functions defined in this
// file can reliably be replaced by their std counterparts, once available.
// You may use these definitions in your own code, but be aware that we
// will remove them once Qt depends on the C++ version that supports
// them in namespace std. There will be NO deprecation warning, the
// definitions will JUST go away.
//
// If you can't agree to these terms, don't use these definitions!
//
// We mean it.
//

QT_BEGIN_NAMESPACE

namespace q20 {
namespace chrono {

#if defined(__GLIBCXX__)
// https://gcc.gnu.org/git/?p=gcc.git;a=blob;f=libstdc%2B%2B-v3/include/bits/chrono.h
using IntRep = int64_t;
#else
// https://github.com/llvm/llvm-project/blob/main/libcxx/include/__chrono/duration.h
// https://github.com/microsoft/STL/blob/main/stl/inc/__msvc_chrono.hpp
using IntRep = int;
#endif

// INTEGRITY incident-85878 (timezone and clock_cast are not supported)
#if __cpp_lib_chrono >= 201907L && !defined(Q_OS_INTEGRITY)
using std::chrono::days;
using std::chrono::weeks;
using std::chrono::years;
using std::chrono::months;

static_assert(std::is_same_v<days::rep, IntRep>);
static_assert(std::is_same_v<weeks::rep, IntRep>);
static_assert(std::is_same_v<years::rep, IntRep>);
static_assert(std::is_same_v<months::rep, IntRep>);
#else
using days = std::chrono::duration<IntRep, std::ratio<86400>>;
using weeks = std::chrono::duration<IntRep, std::ratio_multiply<std::ratio<7>, days::period>>;
using years = std::chrono::duration<IntRep, std::ratio_multiply<std::ratio<146097, 400>, days::period>>;
using months = std::chrono::duration<IntRep, std::ratio_divide<years::period, std::ratio<12>>>;
#endif // __cpp_lib_chrono >= 201907L
} // namespace chrono
} // namespace q20

QT_END_NAMESPACE

#endif /* Q20CHRONO_H */
