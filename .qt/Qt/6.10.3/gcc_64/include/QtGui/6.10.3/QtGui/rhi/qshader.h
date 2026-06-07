// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSHADER_H
#define QSHADER_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the RHI API, with limited compatibility guarantees.
// Usage of this API may make your code source and binary incompatible with
// future versions of Qt.
//

#include <QtGui/qtguiglobal.h>
#include <QtCore/qhash.h>
#include <QtCore/qmap.h>
#include <rhi/qshaderdescription.h>

QT_BEGIN_NAMESPACE

struct QShaderPrivate;
class QShaderKey;

#ifdef Q_OS_INTEGRITY
  class QShaderVersion;
  size_t qHash(const QShaderVersion &, size_t = 0) noexcept;
#endif

class Q_GUI_EXPORT QShaderVersion
{
public:
    enum Flag {
        GlslEs = 0x01
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    QShaderVersion() = default;
    QShaderVersion(int v, Flags f = Flags());

    int version() const { return m_version; }
    void setVersion(int v) { m_version = v; }

    Flags flags() const { return m_flags; }
    void setFlags(Flags f) { m_flags = f; }

private:
    int m_version = 100;
    Flags m_flags;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QShaderVersion::Flags)
Q_DECLARE_TYPEINFO(QShaderVersion, Q_RELOCATABLE_TYPE);

class QShaderCode;
Q_GUI_EXPORT size_t qHash(const QShaderCode &, size_t = 0) noexcept;

class Q_GUI_EXPORT QShaderCode
{
public:
    QShaderCode() = default;
    QShaderCode(const QByteArray &code, const QByteArray &entry = QByteArray());

    QByteArray shader() const { return m_shader; }
    void setShader(const QByteArray &code) { m_shader = code; }

    QByteArray entryPoint() const { return m_entryPoint; }
    void setEntryPoint(const QByteArray &entry) { m_entryPoint = entry; }

private:
    friend Q_GUI_EXPORT size_t qHash(const QShaderCode &, size_t) noexcept;

    QByteArray m_shader;
    QByteArray m_entryPoint;
};

Q_DECLARE_TYPEINFO(QShaderCode, Q_RELOCATABLE_TYPE);

class Q_GUI_EXPORT QShader
{
public:
    enum Stage {
        VertexStage = 0,
        TessellationControlStage,
        TessellationEvaluationStage,
        GeometryStage,
        FragmentStage,
        ComputeStage
    };

    enum Source {
        SpirvShader = 0,
        GlslShader,
        HlslShader,
        DxbcShader, // fxc
        MslShader,
        DxilShader, // dxc
        MetalLibShader, // xcrun metal + xcrun metallib
        WgslShader
    };

    enum Variant {
        StandardShader = 0,
        BatchableVertexShader,
        UInt16IndexedVertexAsComputeShader,
        UInt32IndexedVertexAsComputeShader,
        NonIndexedVertexAsComputeShader,
        HdrCapableFragmentShader,
    };

    enum class SerializedFormatVersion {
        Latest = 0,
        Qt_6_5,
        Qt_6_4
    };

    QShader();
    QShader(const QShader &other);
    QShader &operator=(const QShader &other);
    QShader(QShader &&other) noexcept : d(std::exchange(other.d, nullptr)) {}
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(QShader)
    ~QShader();

    void swap(QShader &other) noexcept { qt_ptr_swap(d, other.d); }
    void detach();

    bool isValid() const;

    Stage stage() const;
    void setStage(Stage stage);

    QShaderDescription description() const;
    void setDescription(const QShaderDescription &desc);

    QList<QShaderKey> availableShaders() const;
    QShaderCode shader(const QShaderKey &key) const;
    void setShader(const QShaderKey &key, const QShaderCode &shader);
    void removeShader(const QShaderKey &key);

    QByteArray serialized(SerializedFormatVersion version = SerializedFormatVersion::Latest) const;
    static QShader fromSerialized(const QByteArray &data);

    using NativeResourceBindingMap = QMap<int, std::pair<int, int>>; // binding -> native_binding[, native_binding]
    NativeResourceBindingMap nativeResourceBindingMap(const QShaderKey &key) const;
    void setResourceBindingMap(const QShaderKey &key, const NativeResourceBindingMap &map);
    void removeResourceBindingMap(const QShaderKey &key);

    struct SeparateToCombinedImageSamplerMapping {
        QByteArray combinedSamplerName;
        int textureBinding;
        int samplerBinding;
    };
    using SeparateToCombinedImageSamplerMappingList = QList<SeparateToCombinedImageSamplerMapping>;
    SeparateToCombinedImageSamplerMappingList separateToCombinedImageSamplerMappingList(const QShaderKey &key) const;
    void setSeparateToCombinedImageSamplerMappingList(const QShaderKey &key,
                                                      const SeparateToCombinedImageSamplerMappingList &list);
    void removeSeparateToCombinedImageSamplerMappingList(const QShaderKey &key);

    struct NativeShaderInfo {
        int flags = 0;
        QMap<int, int> extraBufferBindings;
    };
    NativeShaderInfo nativeShaderInfo(const QShaderKey &key) const;
    void setNativeShaderInfo(const QShaderKey &key, const NativeShaderInfo &info);
    void removeNativeShaderInfo(const QShaderKey &key);

private:
    QShaderPrivate *d;
    friend struct QShaderPrivate;
    friend Q_GUI_EXPORT bool operator==(const QShader &, const QShader &) noexcept;
    friend Q_GUI_EXPORT size_t qHash(const QShader &, size_t) noexcept;
#ifndef QT_NO_DEBUG_STREAM
    friend Q_GUI_EXPORT QDebug operator<<(QDebug, const QShader &);
#endif
};

class Q_GUI_EXPORT QShaderKey
{
public:
    QShaderKey() = default;
    QShaderKey(QShader::Source s,
               const QShaderVersion &sver,
               QShader::Variant svar = QShader::StandardShader);

    QShader::Source source() const { return m_source; }
    void setSource(QShader::Source s) { m_source = s; }

    QShaderVersion sourceVersion() const { return m_sourceVersion; }
    void setSourceVersion(const QShaderVersion &sver) { m_sourceVersion = sver; }

    QShader::Variant sourceVariant() const { return m_sourceVariant; }
    void setSourceVariant(QShader::Variant svar) { m_sourceVariant = svar; }

private:
    QShader::Source m_source = QShader::SpirvShader;
    QShaderVersion m_sourceVersion;
    QShader::Variant m_sourceVariant = QShader::StandardShader;
};

Q_DECLARE_TYPEINFO(QShaderKey, Q_RELOCATABLE_TYPE);

Q_GUI_EXPORT bool operator==(const QShader &lhs, const QShader &rhs) noexcept;
Q_GUI_EXPORT size_t qHash(const QShader &s, size_t seed = 0) noexcept;

inline bool operator!=(const QShader &lhs, const QShader &rhs) noexcept
{
    return !(lhs == rhs);
}

Q_GUI_EXPORT bool operator==(const QShaderVersion &lhs, const QShaderVersion &rhs) noexcept;
Q_GUI_EXPORT bool operator<(const QShaderVersion &lhs, const QShaderVersion &rhs) noexcept;
Q_GUI_EXPORT bool operator==(const QShaderKey &lhs, const QShaderKey &rhs) noexcept;
Q_GUI_EXPORT bool operator<(const QShaderKey &lhs, const QShaderKey &rhs) noexcept;
Q_GUI_EXPORT bool operator==(const QShaderCode &lhs, const QShaderCode &rhs) noexcept;

inline bool operator!=(const QShaderVersion &lhs, const QShaderVersion &rhs) noexcept
{
    return !(lhs == rhs);
}

inline bool operator!=(const QShaderKey &lhs, const QShaderKey &rhs) noexcept
{
    return !(lhs == rhs);
}

inline bool operator!=(const QShaderCode &lhs, const QShaderCode &rhs) noexcept
{
    return !(lhs == rhs);
}

Q_GUI_EXPORT size_t qHash(const QShaderKey &k, size_t seed = 0) noexcept;

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug, const QShader &);
Q_GUI_EXPORT QDebug operator<<(QDebug dbg, const QShaderKey &k);
Q_GUI_EXPORT QDebug operator<<(QDebug dbg, const QShaderVersion &v);
#endif

QT_END_NAMESPACE

#endif
