// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QCONTAINERFWD_H
#define QCONTAINERFWD_H

#include <QtCore/qtconfigmacros.h>
#include <QtCore/qtypes.h>

#if 0
#pragma qt_class(QtContainerFwd)
#endif

// std headers can unfortunately not be forward declared
#include <cstddef> // std::size_t
#include <utility>
#include <limits>

QT_BEGIN_NAMESPACE

template <typename Key, typename T> class QCache;
template <typename Key, typename T> class QHash;
template <typename Key, typename T> class QMap;
template <typename Key, typename T> class QMultiHash;
template <typename Key, typename T> class QMultiMap;
#ifndef QT_NO_QPAIR
template <typename T1, typename T2>
using QPair = std::pair<T1, T2>;
#endif
template <typename T> class QQueue;
template <typename T> class QSet;
template <typename T, std::size_t E = std::size_t(-1) /* = std::dynamic_extent*/> class QSpan;
template <typename T> class QStack;
constexpr qsizetype QVarLengthArrayDefaultPrealloc = 256;
template <typename T, qsizetype Prealloc = QVarLengthArrayDefaultPrealloc> class QVarLengthArray;
template <typename T> class QList;
class QString;
#ifndef Q_QDOC
template<typename T> using QVector = QList<T>;
using QStringList = QList<QString>;
class QByteArray;
using QByteArrayList = QList<QByteArray>;
#else
template<typename T> class QVector;
class QStringList;
class QByteArrayList;
#endif
class QMetaType;
class QVariant;

using QVariantList = QList<QVariant>;
using QVariantMap = QMap<QString, QVariant>;
using QVariantHash = QHash<QString, QVariant>;
using QVariantPair = std::pair<QVariant, QVariant>;

namespace QtPrivate
{
[[maybe_unused]]
constexpr qsizetype MaxAllocSize = (std::numeric_limits<qsizetype>::max)();
}

QT_END_NAMESPACE

#endif // QCONTAINERFWD_H
