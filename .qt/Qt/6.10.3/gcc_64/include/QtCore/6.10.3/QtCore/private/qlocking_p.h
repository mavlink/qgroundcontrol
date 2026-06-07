// Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QLOCKING_P_H
#define QLOCKING_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists for the convenience of
// qmutex.cpp and qmutex_unix.cpp. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qmutex.h>
#include <QtCore/private/qglobal_p.h>

#include <mutex>

QT_BEGIN_NAMESPACE

//
// This API is bridging the time until we can depend on C++17:
//
// - qt_scoped_lock returns a lock that cannot be unlocked again before the end of the scope
// - qt_unique_lock returns a lock that can be unlock()ed and moved around
// - for compat with QMutexLocker, qt_unique_lock supports passing by pointer.
//   Do NOT use this overload lightly; it's only for cases such as where a Q_GLOBAL_STATIC
//   may have already been deleted. In particular, do NOT port from
//       QMutexLocker locker(&mutex);
//   to
//       auto locker = qt_unique_lock(&mutex);
//   as this will not port automatically to std::unique_lock come C++17!
//
// The intent, come C++17, is to replace
//     qt_scoped_lock(mutex);
//     qt_unique_lock(mutex); // except qt_unique_lock(&mutex)
// with
//     std::scoped_lock(mutex);
//     std::unique_lock(mutex);
// resp. (C++17 meaning CTAD, guaranteed copy elision + scoped_lock available on all platforms),
// so please use these functions only in ways which don't break this mechanical search & replace.
//

namespace {

template <typename Mutex, typename Lock =
# if defined(__cpp_lib_scoped_lock) && __cpp_lib_scoped_lock >= 201703L
          std::scoped_lock
# else
          std::lock_guard
# endif
          <typename std::decay<Mutex>::type>
>
Lock qt_scoped_lock(Mutex &mutex)
{
    return Lock(mutex);
}

template <typename Mutex, typename Lock = std::unique_lock<typename std::decay<Mutex>::type>>
Lock qt_unique_lock(Mutex &mutex)
{
    return Lock(mutex);
}

template <typename Mutex, typename Lock = std::unique_lock<typename std::decay<Mutex>::type>>
Lock qt_unique_lock(Mutex *mutex)
{
    return mutex ? Lock(*mutex) : Lock() ;
}

} // unnamed namespace

QT_END_NAMESPACE

#endif // QLOCKING_P_H
