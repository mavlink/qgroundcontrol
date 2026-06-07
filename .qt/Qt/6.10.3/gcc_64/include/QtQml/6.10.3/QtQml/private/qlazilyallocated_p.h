// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QLAZILYALLOCATED_P_H
#define QLAZILYALLOCATED_P_H

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
#include <QtCore/qtaggedpointer.h>

QT_BEGIN_NAMESPACE

template<typename T, typename Tag = typename QtPrivate::TagInfo<T>::TagType>
class QLazilyAllocated {
public:
    inline QLazilyAllocated();
    inline ~QLazilyAllocated();

    inline bool isAllocated() const;

    inline T *operator->() const;

    inline T &value();
    inline const T &value() const;

    inline Tag tag() const;
    inline void setTag(Tag t);
private:
    mutable QTaggedPointer<T, Tag> d;
};

template<typename T, typename Tag>
QLazilyAllocated<T, Tag>::QLazilyAllocated()
{
}

template<typename T, typename Tag>
QLazilyAllocated<T, Tag>::~QLazilyAllocated()
{
    delete d.data();
}

template<typename T, typename Tag>
bool QLazilyAllocated<T, Tag>::isAllocated() const
{
    return !d.isNull();
}

template<typename T, typename Tag>
T &QLazilyAllocated<T, Tag>::value()
{
    if (d.isNull()) d = new T;
    return *d;
}

template<typename T, typename Tag>
const T &QLazilyAllocated<T, Tag>::value() const
{
    if (d.isNull()) d = new T;
    return *d;
}

template<typename T, typename Tag>
T *QLazilyAllocated<T, Tag>::operator->() const
{
    return d.data();
}

template<typename T, typename Tag>
Tag QLazilyAllocated<T, Tag>::tag() const
{
    return d.tag();
}

template<typename T, typename Tag>
void QLazilyAllocated<T, Tag>::setTag(Tag t)
{
    d.setTag(t);
}

QT_END_NAMESPACE

#endif // QLAZILYALLOCATED_P_H
