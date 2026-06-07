// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSHADER_P_H
#define QSHADER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>
#include <rhi/qshader.h>
#include <QtCore/QAtomicInt>
#include <QtCore/QMap>

QT_BEGIN_NAMESPACE

struct Q_GUI_EXPORT QShaderPrivate
{
    static const int QSB_VERSION = 9;
    static const int QSB_VERSION_WITHOUT_INPUT_OUTPUT_INTERFACE_BLOCKS = 8;
    static const int QSB_VERSION_WITHOUT_EXTENDED_STORAGE_BUFFER_INFO = 7;
    static const int QSB_VERSION_WITHOUT_NATIVE_SHADER_INFO = 6;
    static const int QSB_VERSION_WITHOUT_SEPARATE_IMAGES_AND_SAMPLERS = 5;
    static const int QSB_VERSION_WITHOUT_VAR_ARRAYDIMS = 4;
    static const int QSB_VERSION_WITH_CBOR = 3;
    static const int QSB_VERSION_WITH_BINARY_JSON = 2;
    static const int QSB_VERSION_WITHOUT_BINDINGS = 1;

    enum MslNativeShaderInfoExtraBufferBindings {
        MslTessVertIndicesBufferBinding = 0,
        MslTessVertTescOutputBufferBinding,
        MslTessTescTessLevelBufferBinding,
        MslTessTescPatchOutputBufferBinding,
        MslTessTescParamsBufferBinding,
        MslTessTescInputBufferBinding,
        MslBufferSizeBufferBinding,
        MslMultiViewMaskBufferBinding
    };

    QShaderPrivate()
        : ref(1)
    {
    }

    QShaderPrivate(const QShaderPrivate &other)
        : ref(1),
          qsbVersion(other.qsbVersion),
          stage(other.stage),
          desc(other.desc),
          shaders(other.shaders),
          bindings(other.bindings),
          combinedImageMap(other.combinedImageMap),
          nativeShaderInfoMap(other.nativeShaderInfoMap)
    {
    }

    static QShaderPrivate *get(QShader *s) { return s->d; }
    static const QShaderPrivate *get(const QShader *s) { return s->d; }
    static int qtQsbVersion(QShader::SerializedFormatVersion qtVersion) {
        switch (qtVersion) {
        case QShader::SerializedFormatVersion::Qt_6_4:
            return (QShaderPrivate::QSB_VERSION_WITHOUT_SEPARATE_IMAGES_AND_SAMPLERS + 1);
        case QShader::SerializedFormatVersion::Qt_6_5:
            return (QShaderPrivate::QSB_VERSION_WITHOUT_EXTENDED_STORAGE_BUFFER_INFO + 1);
        default:
            return QShaderPrivate::QSB_VERSION;
        }
    }

    QAtomicInt ref;
    int qsbVersion = QSB_VERSION;
    QShader::Stage stage = QShader::VertexStage;
    QShaderDescription desc;
    // QMap not QHash because we need to be able to iterate based on sorted keys
    QMap<QShaderKey, QShaderCode> shaders;
    QMap<QShaderKey, QShader::NativeResourceBindingMap> bindings;
    QMap<QShaderKey, QShader::SeparateToCombinedImageSamplerMappingList> combinedImageMap;
    QMap<QShaderKey, QShader::NativeShaderInfo> nativeShaderInfoMap;
};

QT_END_NAMESPACE

#endif
