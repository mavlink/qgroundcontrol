// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDSHAREDMEMORYFORMATHELPER_H
#define QWAYLANDSHAREDMEMORYFORMATHELPER_H

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

#include <QtGui/QImage>
#include <QtCore/private/qglobal_p.h>

//the correct protocol header for the wayland server or wayland client has to be
//included before this file is included

QT_BEGIN_NAMESPACE

class QWaylandSharedMemoryFormatHelper
{
public:
    static inline wl_shm_format fromQImageFormat(QImage::Format format);
    static inline QImage::Format fromWaylandShmFormat(wl_shm_format format)
    {
        switch (format) {
        case WL_SHM_FORMAT_XRGB8888: return QImage::Format_RGB32;
        case WL_SHM_FORMAT_ARGB8888: return QImage::Format_ARGB32_Premultiplied;
        case WL_SHM_FORMAT_RGB565: return QImage::Format_RGB16;
        case WL_SHM_FORMAT_XRGB1555: return QImage::Format_RGB555;
        case WL_SHM_FORMAT_RGB888: return QImage::Format_RGB888;
        case WL_SHM_FORMAT_BGR888: return QImage::Format_BGR888;
        case WL_SHM_FORMAT_XRGB4444: return QImage::Format_RGB444;
        case WL_SHM_FORMAT_ARGB4444: return QImage::Format_ARGB4444_Premultiplied;
        case WL_SHM_FORMAT_XBGR8888: return QImage::Format_RGBX8888;
        case WL_SHM_FORMAT_ABGR8888: return QImage::Format_RGBA8888_Premultiplied;
        case WL_SHM_FORMAT_XBGR2101010: return QImage::Format_BGR30;
        case WL_SHM_FORMAT_ABGR2101010: return QImage::Format_A2BGR30_Premultiplied;
        case WL_SHM_FORMAT_XRGB2101010: return QImage::Format_RGB30;
        case WL_SHM_FORMAT_ARGB2101010: return QImage::Format_A2RGB30_Premultiplied;
        case WL_SHM_FORMAT_C8: return QImage::Format_Alpha8;
        default: return QImage::Format_Invalid;
        }
    }

private:
//IMPLEMENTATION (which has to be inline in the header because of the include trick)
    struct Array
    {
        Array(const size_t size, const wl_shm_format *data)
            : size(size)
            , data(data)
        { }
        const size_t size;
        const wl_shm_format *data = nullptr;
    };

    static const Array getData()
    {
        static wl_shm_format formats_array[] = {
            wl_shm_format(INT_MIN),    //Format_Invalid,
            wl_shm_format(INT_MIN),    //Format_Mono,
            wl_shm_format(INT_MIN),    //Format_MonoLSB,
            wl_shm_format(INT_MIN),    //Format_Indexed8,
            WL_SHM_FORMAT_XRGB8888,    //Format_RGB32,
            WL_SHM_FORMAT_ARGB8888,    //Format_ARGB32,
            WL_SHM_FORMAT_ARGB8888,    //Format_ARGB32_Premultiplied,
            WL_SHM_FORMAT_RGB565,      //Format_RGB16,
            wl_shm_format(INT_MIN),    //Format_ARGB8565_Premultiplied,
            wl_shm_format(INT_MIN),    //Format_RGB666,
            wl_shm_format(INT_MIN),    //Format_ARGB6666_Premultiplied,
            WL_SHM_FORMAT_XRGB1555,    //Format_RGB555,
            wl_shm_format(INT_MIN),    //Format_ARGB8555_Premultiplied,
            WL_SHM_FORMAT_RGB888,      //Format_RGB888,
            WL_SHM_FORMAT_XRGB4444,    //Format_RGB444,
            WL_SHM_FORMAT_ARGB4444,    //Format_ARGB4444_Premultiplied,
            WL_SHM_FORMAT_XBGR8888,    //Format_RGBX8888,
            WL_SHM_FORMAT_ABGR8888,    //Format_RGBA8888,
            WL_SHM_FORMAT_ABGR8888,    //Format_RGBA8888_Premultiplied,
            WL_SHM_FORMAT_XBGR2101010, //Format_BGR30,
            WL_SHM_FORMAT_ABGR2101010, //Format_A2BGR30_Premultiplied,
            WL_SHM_FORMAT_XRGB2101010, //Format_RGB30,
            WL_SHM_FORMAT_ARGB2101010, //Format_A2RGB30_Premultiplied,
            WL_SHM_FORMAT_C8,          //Format_Alpha8,
            WL_SHM_FORMAT_C8,          //Format_Grayscale8,
            wl_shm_format(INT_MIN),    //Format_RGBX64,
            wl_shm_format(INT_MIN),    //Format_RGBA64,
            wl_shm_format(INT_MIN),    //Format_RGBA64_Premultiplied,
            wl_shm_format(INT_MIN),    //Format_Grayscale16,
            WL_SHM_FORMAT_BGR888,      //Format_BGR888

        };
        const size_t size = sizeof(formats_array) / sizeof(*formats_array);
        return Array(size, formats_array);
    }
};

wl_shm_format QWaylandSharedMemoryFormatHelper::fromQImageFormat(QImage::Format format)
{
    Array array = getData();
    if (array.size <= size_t(format))
        return wl_shm_format(INT_MIN);
    return array.data[format];
}

QT_END_NAMESPACE

#endif //QWAYLANDSHAREDMEMORYFORMATHELPER_H
