// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSHADERDESCRIPTION_H
#define QSHADERDESCRIPTION_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the RHI API, with limited compatibility guarantees.
// Usage of this API may make your code source and binary incompatible with
// future versions of Qt.
//

#include <QtGui/qtguiglobal.h>
#include <QtCore/qstring.h>
#include <QtCore/qlist.h>
#include <array>

QT_BEGIN_NAMESPACE

struct QShaderDescriptionPrivate;
class QDataStream;

class Q_GUI_EXPORT QShaderDescription
{
public:
    QShaderDescription();
    QShaderDescription(const QShaderDescription &other);
    QShaderDescription &operator=(const QShaderDescription &other);
    ~QShaderDescription();
    void detach();

    bool isValid() const;

    void serialize(QDataStream *stream, int version) const;
    QByteArray toJson() const;

    static QShaderDescription deserialize(QDataStream *stream, int version);

    enum VariableType {
        Unknown = 0,

        // do not reorder
        Float,
        Vec2,
        Vec3,
        Vec4,
        Mat2,
        Mat2x3,
        Mat2x4,
        Mat3,
        Mat3x2,
        Mat3x4,
        Mat4,
        Mat4x2,
        Mat4x3,

        Int,
        Int2,
        Int3,
        Int4,

        Uint,
        Uint2,
        Uint3,
        Uint4,

        Bool,
        Bool2,
        Bool3,
        Bool4,

        Double,
        Double2,
        Double3,
        Double4,
        DMat2,
        DMat2x3,
        DMat2x4,
        DMat3,
        DMat3x2,
        DMat3x4,
        DMat4,
        DMat4x2,
        DMat4x3,

        Sampler1D,
        Sampler2D,
        Sampler2DMS,
        Sampler3D,
        SamplerCube,
        Sampler1DArray,
        Sampler2DArray,
        Sampler2DMSArray,
        Sampler3DArray,
        SamplerCubeArray,
        SamplerRect,
        SamplerBuffer,
        SamplerExternalOES,
        Sampler,

        Image1D,
        Image2D,
        Image2DMS,
        Image3D,
        ImageCube,
        Image1DArray,
        Image2DArray,
        Image2DMSArray,
        Image3DArray,
        ImageCubeArray,
        ImageRect,
        ImageBuffer,

        Struct,

        Half,
        Half2,
        Half3,
        Half4
    };

    enum ImageFormat {
        // must match SPIR-V's ImageFormat
        ImageFormatUnknown = 0,
        ImageFormatRgba32f = 1,
        ImageFormatRgba16f = 2,
        ImageFormatR32f = 3,
        ImageFormatRgba8 = 4,
        ImageFormatRgba8Snorm = 5,
        ImageFormatRg32f = 6,
        ImageFormatRg16f = 7,
        ImageFormatR11fG11fB10f = 8,
        ImageFormatR16f = 9,
        ImageFormatRgba16 = 10,
        ImageFormatRgb10A2 = 11,
        ImageFormatRg16 = 12,
        ImageFormatRg8 = 13,
        ImageFormatR16 = 14,
        ImageFormatR8 = 15,
        ImageFormatRgba16Snorm = 16,
        ImageFormatRg16Snorm = 17,
        ImageFormatRg8Snorm = 18,
        ImageFormatR16Snorm = 19,
        ImageFormatR8Snorm = 20,
        ImageFormatRgba32i = 21,
        ImageFormatRgba16i = 22,
        ImageFormatRgba8i = 23,
        ImageFormatR32i = 24,
        ImageFormatRg32i = 25,
        ImageFormatRg16i = 26,
        ImageFormatRg8i = 27,
        ImageFormatR16i = 28,
        ImageFormatR8i = 29,
        ImageFormatRgba32ui = 30,
        ImageFormatRgba16ui = 31,
        ImageFormatRgba8ui = 32,
        ImageFormatR32ui = 33,
        ImageFormatRgb10a2ui = 34,
        ImageFormatRg32ui = 35,
        ImageFormatRg16ui = 36,
        ImageFormatRg8ui = 37,
        ImageFormatR16ui = 38,
        ImageFormatR8ui = 39
    };

    enum ImageFlag {
        ReadOnlyImage = 1 << 0,
        WriteOnlyImage = 1 << 1
    };
    Q_DECLARE_FLAGS(ImageFlags, ImageFlag)

    enum QualifierFlag {
        QualifierReadOnly = 1 << 0,
        QualifierWriteOnly = 1 << 1,
        QualifierCoherent = 1 << 2,
        QualifierVolatile = 1 << 3,
        QualifierRestrict = 1 << 4,
    };
    Q_DECLARE_FLAGS(QualifierFlags, QualifierFlag)

    // Optional data (like decorations) usually default to an otherwise invalid value (-1 or 0). This is intentional.

    struct BlockVariable {
        QByteArray name;
        VariableType type = Unknown;
        int offset = 0;
        int size = 0;
        QList<int> arrayDims;
        int arrayStride = 0;
        int matrixStride = 0;
        bool matrixIsRowMajor = false;
        QList<BlockVariable> structMembers;
    };

    struct InOutVariable {
        QByteArray name;
        VariableType type = Unknown;
        int location = -1;
        int binding = -1;
        int descriptorSet = -1;
        ImageFormat imageFormat = ImageFormatUnknown;
        ImageFlags imageFlags;
        QList<int> arrayDims;
        bool perPatch = false;
        QList<BlockVariable> structMembers;
    };

    struct UniformBlock {
        QByteArray blockName;
        QByteArray structName; // instanceName
        int size = 0;
        int binding = -1;
        int descriptorSet = -1;
        QList<BlockVariable> members;
    };

    struct PushConstantBlock {
        QByteArray name;
        int size = 0;
        QList<BlockVariable> members;
    };

    struct StorageBlock {
        QByteArray blockName;
        QByteArray instanceName;
        int knownSize = 0;
        int binding = -1;
        int descriptorSet = -1;
        QList<BlockVariable> members;
        int runtimeArrayStride = 0;
        QualifierFlags qualifierFlags;
    };

    QList<InOutVariable> inputVariables() const;
    QList<InOutVariable> outputVariables() const;
    QList<UniformBlock> uniformBlocks() const;
    QList<PushConstantBlock> pushConstantBlocks() const;
    QList<StorageBlock> storageBlocks() const;
    QList<InOutVariable> combinedImageSamplers() const;
    QList<InOutVariable> separateImages() const;
    QList<InOutVariable> separateSamplers() const;
    QList<InOutVariable> storageImages() const;

    enum BuiltinType {
        // must match SpvBuiltIn
        PositionBuiltin = 0,
        PointSizeBuiltin = 1,
        ClipDistanceBuiltin = 3,
        CullDistanceBuiltin = 4,
        VertexIdBuiltin = 5,
        InstanceIdBuiltin = 6,
        PrimitiveIdBuiltin = 7,
        InvocationIdBuiltin = 8,
        LayerBuiltin = 9,
        ViewportIndexBuiltin = 10,
        TessLevelOuterBuiltin = 11,
        TessLevelInnerBuiltin = 12,
        TessCoordBuiltin = 13,
        PatchVerticesBuiltin = 14,
        FragCoordBuiltin = 15,
        PointCoordBuiltin = 16,
        FrontFacingBuiltin = 17,
        SampleIdBuiltin = 18,
        SamplePositionBuiltin = 19,
        SampleMaskBuiltin = 20,
        FragDepthBuiltin = 22,
        NumWorkGroupsBuiltin = 24,
        WorkgroupSizeBuiltin = 25,
        WorkgroupIdBuiltin = 26,
        LocalInvocationIdBuiltin = 27,
        GlobalInvocationIdBuiltin = 28,
        LocalInvocationIndexBuiltin = 29,
        VertexIndexBuiltin = 42,
        InstanceIndexBuiltin = 43
    };

    struct BuiltinVariable {
        BuiltinType type;
        VariableType varType;
        QList<int> arrayDims;
    };

    QList<BuiltinVariable> inputBuiltinVariables() const;
    QList<BuiltinVariable> outputBuiltinVariables() const;

    std::array<uint, 3> computeShaderLocalSize() const;

    uint tessellationOutputVertexCount() const;

    enum TessellationMode {
        UnknownTessellationMode,
        TrianglesTessellationMode,
        QuadTessellationMode,
        IsolineTessellationMode
    };

    TessellationMode tessellationMode() const;

    enum TessellationWindingOrder {
        UnknownTessellationWindingOrder,
        CwTessellationWindingOrder,
        CcwTessellationWindingOrder
    };

    TessellationWindingOrder tessellationWindingOrder() const;

    enum TessellationPartitioning {
        UnknownTessellationPartitioning,
        EqualTessellationPartitioning,
        FractionalEvenTessellationPartitioning,
        FractionalOddTessellationPartitioning
    };

    TessellationPartitioning tessellationPartitioning() const;

private:
    QShaderDescriptionPrivate *d;
    friend struct QShaderDescriptionPrivate;
#ifndef QT_NO_DEBUG_STREAM
    friend Q_GUI_EXPORT QDebug operator<<(QDebug, const QShaderDescription &);
#endif
    friend Q_GUI_EXPORT bool operator==(const QShaderDescription &lhs, const QShaderDescription &rhs) noexcept;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QShaderDescription::ImageFlags)
Q_DECLARE_OPERATORS_FOR_FLAGS(QShaderDescription::QualifierFlags)

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug, const QShaderDescription &);
Q_GUI_EXPORT QDebug operator<<(QDebug, const QShaderDescription::InOutVariable &);
Q_GUI_EXPORT QDebug operator<<(QDebug, const QShaderDescription::BlockVariable &);
Q_GUI_EXPORT QDebug operator<<(QDebug, const QShaderDescription::UniformBlock &);
Q_GUI_EXPORT QDebug operator<<(QDebug, const QShaderDescription::PushConstantBlock &);
Q_GUI_EXPORT QDebug operator<<(QDebug, const QShaderDescription::StorageBlock &);
Q_GUI_EXPORT QDebug operator<<(QDebug, const QShaderDescription::BuiltinVariable &);
#endif

Q_GUI_EXPORT bool operator==(const QShaderDescription &lhs, const QShaderDescription &rhs) noexcept;
Q_GUI_EXPORT bool operator==(const QShaderDescription::InOutVariable &lhs, const QShaderDescription::InOutVariable &rhs) noexcept;
Q_GUI_EXPORT bool operator==(const QShaderDescription::BlockVariable &lhs, const QShaderDescription::BlockVariable &rhs) noexcept;
Q_GUI_EXPORT bool operator==(const QShaderDescription::UniformBlock &lhs, const QShaderDescription::UniformBlock &rhs) noexcept;
Q_GUI_EXPORT bool operator==(const QShaderDescription::PushConstantBlock &lhs, const QShaderDescription::PushConstantBlock &rhs) noexcept;
Q_GUI_EXPORT bool operator==(const QShaderDescription::StorageBlock &lhs, const QShaderDescription::StorageBlock &rhs) noexcept;
Q_GUI_EXPORT bool operator==(const QShaderDescription::BuiltinVariable &lhs, const QShaderDescription::BuiltinVariable &rhs) noexcept;

inline bool operator!=(const QShaderDescription &lhs, const QShaderDescription &rhs) noexcept
{
    return !(lhs == rhs);
}

inline bool operator!=(const QShaderDescription::InOutVariable &lhs, const QShaderDescription::InOutVariable &rhs) noexcept
{
    return !(lhs == rhs);
}

inline bool operator!=(const QShaderDescription::BlockVariable &lhs, const QShaderDescription::BlockVariable &rhs) noexcept
{
    return !(lhs == rhs);
}

inline bool operator!=(const QShaderDescription::UniformBlock &lhs, const QShaderDescription::UniformBlock &rhs) noexcept
{
    return !(lhs == rhs);
}

inline bool operator!=(const QShaderDescription::PushConstantBlock &lhs, const QShaderDescription::PushConstantBlock &rhs) noexcept
{
    return !(lhs == rhs);
}

inline bool operator!=(const QShaderDescription::StorageBlock &lhs, const QShaderDescription::StorageBlock &rhs) noexcept
{
    return !(lhs == rhs);
}

inline bool operator!=(const QShaderDescription::BuiltinVariable &lhs, const QShaderDescription::BuiltinVariable &rhs) noexcept
{
    return !(lhs == rhs);
}

QT_END_NAMESPACE

#endif
