// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTMOCHELPERS_H
#define QTMOCHELPERS_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists to be used by the code that
// moc generates. This file will not change quickly, but it over the long term,
// it will likely change or even be removed.
//
// We mean it.
//

#include <QtCore/qmetatype.h>
#include <QtCore/qtmocconstants.h>

#include <QtCore/q20algorithm.h>    // std::min, std::copy_n
#include <QtCore/q23type_traits.h>  // std::is_scoped_enum
#include <limits>

#if 0
#pragma qt_no_master_include
#endif

QT_BEGIN_NAMESPACE
namespace QtMocHelpers {
// The maximum Size of a string literal is 2 GB on 32-bit and 4 GB on 64-bit
// (but the compiler is likely to give up before you get anywhere near that much)
static constexpr size_t MaxStringSize =
        (std::min)(size_t((std::numeric_limits<uint>::max)()),
                   size_t((std::numeric_limits<qsizetype>::max)()));

template <uint UCount, uint SCount, size_t SSize, uint MCount> struct MetaObjectContents
{
    struct StaticContent {
        uint data[UCount];
        uint stringdata[SCount];
        char strings[SSize];
    } staticData = {};
    struct RelocatingContent {
        const QtPrivate::QMetaTypeInterface *metaTypes[MCount];
    } relocatingData = {};
};

template <int Count, size_t StringSize> struct StringData
{
    static_assert(StringSize <= MaxStringSize, "Meta Object data is too big");
    uint offsetsAndSizes[Count] = {};
    char stringdata0[StringSize] = {};
    constexpr StringData() = default;
};

template <typename... Strings> struct StringRefStorage
{
    static constexpr size_t stringSizeHelper() noexcept
    {
        // same as:
        //   return (0 + ... + std::extent_v<Strings>);
        // but not using the fold expression to avoid exceeding compiler limits
        size_t total = 0;
        int sizes[] = { std::extent_v<Strings>... };
        for (int n : sizes)
            total += n;
        return size_t(total);
    }

    static constexpr int StringCount = sizeof...(Strings);
    static constexpr size_t StringSize = stringSizeHelper();
    static_assert(StringSize <= MaxStringSize, "Meta Object data is too big");
    const char *inputs[StringCount];

    constexpr StringRefStorage(const Strings &... strings) noexcept
        : inputs{ strings... }
    { }

    constexpr void
    writeTo(uint (&offsets)[2 * StringCount], char (&data)[StringSize]) const noexcept
    {
        int sizes[] = { std::extent_v<Strings>... };

        uint offset = 0;
        char *output = data;
        for (size_t i = 0; i < sizeof...(Strings); ++i) {
            // copy the input string, including the terminating null
            int len = sizes[i];
            for (int j = 0; j < len; ++j)
                output[offset + j] = inputs[i][j];
            offsets[2 * i] = offset + sizeof(offsets);
            offsets[2 * i + 1] = len - 1;
            offset += len;
        }
    }

    constexpr auto create() const noexcept
    {
        StringData<2 * StringCount, StringSize>  result;
        writeTo(result.offsetsAndSizes, result.stringdata0);
        return result;
    }
};

template <uint... Nx> constexpr auto stringData(const char (&...strings)[Nx])
{
    return StringRefStorage(strings...).create();
}

template <typename FuncType> inline bool indexOfMethod(void **_a, FuncType f, int index) noexcept
{
    int *result = static_cast<int *>(_a[0]);
    auto candidate = reinterpret_cast<FuncType *>(_a[1]);
    if (*candidate != f)
        return false;
    *result = index;
    return true;
}

template <typename Prop, typename Value> inline bool setProperty(Prop &property, Value &&value)
{
    if (property == value)
        return false;
    property = std::forward<Value>(value);
    return true;
}

struct NoType {};

namespace detail {
template<typename Enum> constexpr int payloadSizeForEnum()
{
    // How many uint blocks do we need to store the values of this enum and the
    // string indices for the enumeration labels? We only support 8- 16-, 32-
    // and 64-bit enums at the time of this writing, so this code is extra
    // pedantic allowing for 48-, 96-, 128-bit, etc.
    int n = int(sizeof(Enum) + sizeof(uint)) - 1;
    return 1 + n / sizeof(uint);
}

template <uint H, uint P> struct UintDataBlock
{
    static constexpr uint headerSize() { return H; }
    static constexpr uint payloadSize() { return P; }
    uint header[H ? H : 1] = {};
    uint payload[P ? P : 1] = {};
};

// By default, we allow metatypes for incomplete types to be stored in the
// metatype array, but we provide a way to require them to be complete by using
// void as the Unique type (used by moc if --require-complete-types is passed
// or some internal heuristic for QML matches) or by using the enum below, for
// properties and enums.
enum TypeCompletenessForMetaType : bool {
    TypeMayBeIncomplete = false,
    TypeMustBeComplete = true,
};

template <bool TypeMustBeComplete, typename... T> struct MetaTypeList
{
    static constexpr int count() { return sizeof...(T); }
    template <typename Unique, typename Result> static constexpr void
    copyTo(Result &result, uint &metatypeoffset)
    {
        if constexpr (count()) {
            using namespace QtPrivate;
            using U = std::conditional_t<TypeMustBeComplete, void, Unique>;
            const QMetaTypeInterface *metaTypes[] = {
                qTryMetaTypeInterfaceForType<U, T>()...
            };
            for (const QMetaTypeInterface *mt : metaTypes)
                result.relocatingData.metaTypes[metatypeoffset++] = mt;
        }
    }
};

template <int Idx, typename T> struct UintDataEntry
{
    T entry;
    constexpr UintDataEntry(T &&entry_) : entry(std::move(entry_)) {}
};

// This storage type is designed similar to libc++'s std::tuple, in that it
// derives from a type unique to each of the types in the template parameter
// pack (even if they are the same type). That way, we can refer to each of
// entries uniquely by just casting *this to that unique type.
//
// Testing reveals this to compile MUCH faster than recursive approaches and
// avoids compiler constexpr-time limits.
template <typename Idx, typename... T> struct UintDataStorage;
template <int... Idx, typename... T> struct UintDataStorage<std::integer_sequence<int, Idx...>, T...>
        : UintDataEntry<Idx, T>...
{
    constexpr UintDataStorage(T &&... data)
        : UintDataEntry<Idx, T>(std::move(data))...
    {}

    template <typename F> constexpr void forEach(F &&f) const
    {
        [[maybe_unused]] auto invoke = [&f](const auto &entry_) { f(entry_.entry); return 0; };
        int dummy[] = {
            0,
            invoke(static_cast<const UintDataEntry<Idx, T> &>(*this))...
        };
        (void) dummy;
    }
};
} // namespace detail

template <typename... Block> struct UintData
{
    constexpr UintData(Block &&... data_)
        : data(std::move(data_)...)
    {}

    static constexpr uint count() { return sizeof...(Block); }
    static constexpr uint headerSize()
    {
        // same as:
        //   return (0 + ... + Block::headerSize());
        // but not using the fold expression to avoid exceeding compiler limits
        // (calculation done using int to get compile-time overflow checking)
        int total = 0;
        int sizes[] = { 0, Block::headerSize()... };
        for (int n : sizes)
            total += n;
        return total;
    }
    static constexpr uint payloadSize()
    {
        // ditto
        int total = 0;
        int sizes[] = { 0, Block::payloadSize()... };
        for (int n : sizes)
            total += n;
        return total;
    }
    static constexpr uint dataSize() { return headerSize() + payloadSize(); }
    static constexpr int metaTypeCount()
    {
        // ditto again
        int total = 0;
        int sizes[] = { 0, decltype(Block::metaTypes())::count()... };
        for (int n : sizes)
            total += n;
        return total;
    }

    template <typename Unique, typename Result> constexpr void
    copyTo(Result &result, size_t dataoffset, uint &metatypeoffset) const
    {
        uint *ptr = result.staticData.data;
        size_t payloadoffset = dataoffset + headerSize();
        data.forEach([&](const auto &input) {
            // copy the uint data
            q20::copy_n(input.header, input.headerSize(), ptr + dataoffset);
            q20::copy_n(input.payload, input.payloadSize(), ptr + payloadoffset);
            input.adjustOffsets(ptr, uint(dataoffset), uint(payloadoffset), metatypeoffset);

            // copy the metatypes
            decltype(input.metaTypes())::template copyTo<Unique>(result, metatypeoffset);

            dataoffset += input.headerSize();
            payloadoffset += input.payloadSize();
        });
    }

    template <typename F> constexpr void forEach(F &&f) const
    {
        data.forEach(std::forward<F>(f));
    }

private:
    detail::UintDataStorage<std::make_integer_sequence<int, count()>, Block...> data;
};

template <int N> struct ClassInfos : detail::UintDataBlock<2 * N, 0>
{
    constexpr ClassInfos() = default;
    constexpr ClassInfos(const std::array<uint, 2> (&infos)[N])
    {
        uint *out = this->header;
        for (int i = 0; i < N; ++i) {
            *out++ = infos[i][0];
            *out++ = infos[i][1];
        }
    }
};

template <typename PropertyType> struct PropertyData : detail::UintDataBlock<5, 0>
{
    constexpr PropertyData(uint nameIndex, uint typeIndex, uint flags, uint notifyId = uint(-1), uint revision = 0)
    {
        this->header[0] = nameIndex;
        this->header[1] = typeIndex;
        this->header[2] = flags;
        this->header[3] = notifyId;
        this->header[4] = revision;
    }

    static constexpr auto metaTypes()
    { return detail::MetaTypeList<detail::TypeMustBeComplete, PropertyType>{}; }

    static constexpr void adjustOffsets(uint *, uint, uint, uint) noexcept {}
};

template <typename Enum, int N = 0>
struct EnumData : detail::UintDataBlock<5, N * detail::payloadSizeForEnum<Enum>()>
{
private:
    static_assert(sizeof(Enum) <= 2 * sizeof(uint), "Cannot store enumeration of this size");
    template <typename T> struct RealEnum { using Type = T; };
    template <typename T> struct RealEnum<QFlags<T>> { using Type = T; };
public:
    struct EnumEntry {
        int nameIndex;
        typename RealEnum<Enum>::Type value;
    };

    constexpr EnumData(uint nameOffset, uint aliasOffset, uint flags)
    {
        this->header[0] = nameOffset;
        this->header[1] = aliasOffset;
        this->header[2] = flags;
        this->header[3] = N;
        this->header[4] = 0;        // will be set in adjustOffsets()

        if (nameOffset != aliasOffset || QtPrivate::IsQFlags<Enum>::value)
            this->header[2] |= QtMocConstants::EnumIsFlag;
        if constexpr (q23::is_scoped_enum_v<Enum>)
            this->header[2] |= QtMocConstants::EnumIsScoped;
    }

    template <int Added> constexpr auto add(const EnumEntry (&entries)[Added]) const
    {
        EnumData<Enum, N + Added> result(this->header[0], this->header[1], this->header[2]);

        q20::copy_n(this->payload, this->payloadSize(), result.payload);
        uint o = this->payloadSize();
        for (auto entry : entries) {
            result.payload[o++] = uint(entry.nameIndex);
            auto value = qToUnderlying(entry.value);
            result.payload[o++] = uint(value);
        }

        if constexpr (sizeof(Enum) > sizeof(uint)) {
            static_assert(N == 0, "Unimplemented: merging with non-empty EnumData");
            result.header[2] |= QtMocConstants::EnumIs64Bit;
            for (auto entry : entries) {
                auto value = qToUnderlying(entry.value);
                result.payload[o++] = uint(value >> 32);
            }
        }
        return result;
    }

    static constexpr auto metaTypes()
    { return detail::MetaTypeList<detail::TypeMustBeComplete, Enum>{}; }

    static constexpr void
    adjustOffsets(uint *ptr, uint dataoffset, uint payloadoffset, uint metatypeoffset) noexcept
    {
        ptr[dataoffset + 4] += uint(payloadoffset);
        (void) metatypeoffset;
    }
};

template <typename F, uint ExtraFlags> struct FunctionData;
template <typename Ret, typename... Args, uint ExtraFlags>
struct FunctionData<Ret (Args...), ExtraFlags>
    : detail::UintDataBlock<6, 2 * sizeof...(Args) + 1 + (ExtraFlags & QtMocConstants::MethodRevisioned ? 1 : 0)>
{
    static constexpr bool IsRevisioned = (ExtraFlags & QtMocConstants::MethodRevisioned) != 0;
    struct FunctionParameter {
        uint typeIdx;   // or static meta type ID
        uint nameIdx;
    };
    using ParametersArray = std::array<FunctionParameter, sizeof...(Args)>;

    static auto metaTypes()
    {
        using namespace QtMocConstants;
        if constexpr (std::is_same_v<Ret, NoType>) {
            // constructors have no return type
            static_assert((ExtraFlags & MethodConstructor) == MethodConstructor,
                    "NoType return type used on a non-constructor");
            static_assert((ExtraFlags & MethodIsConst) == 0,
                    "Constructors cannot be const");
            return detail::MetaTypeList<detail::TypeMayBeIncomplete, Args...>{};
        } else {
            static_assert((ExtraFlags & MethodConstructor) != MethodConstructor,
                    "Constructors must use NoType as return type");
            return detail::MetaTypeList<detail::TypeMayBeIncomplete, Ret, Args...>{};
        }
    }

    static constexpr void
    adjustOffsets(uint *ptr, uint dataoffset, uint payloadoffset, uint metatypeoffset) noexcept
    {
        if constexpr (IsRevisioned)
            ++payloadoffset;
        ptr[dataoffset + 2] += uint(payloadoffset);
        ptr[dataoffset + 5] = metatypeoffset;
    }

    constexpr
    FunctionData(uint nameIndex, uint tagIndex, uint flags,
                 uint returnType, ParametersArray params = {})
    {
        this->header[0] = nameIndex;
        this->header[1] = sizeof...(Args);
        this->header[2] = 0;        // will be set in adjustOffsets()
        this->header[3] = tagIndex;
        this->header[4] = flags | ExtraFlags;
        this->header[5] = 0;        // will be set in adjustOffsets()

        uint *p = this->payload;
        if constexpr (ExtraFlags & QtMocConstants::MethodRevisioned)
            ++p;
        *p++ = returnType;
        if constexpr (sizeof...(Args)) {
            for (uint i = 0; i < sizeof...(Args); ++i)
                *p++ = params[i].typeIdx;
            for (uint i = 0; i < sizeof...(Args); ++i)
                *p++ = params[i].nameIdx;
        } else {
            Q_UNUSED(params);
        }
    }

    constexpr
    FunctionData(uint nameIndex, uint tagIndex, uint flags, uint revision,
                 uint returnType, ParametersArray params = {})
#ifdef __cpp_concepts
            requires(IsRevisioned)
#endif
        : FunctionData(nameIndex, tagIndex, flags, returnType, params)
    {
        // note: we place the revision differently from meta object revision 12
        this->payload[0] = revision;
    }
};

template <typename Ret, typename... Args, uint ExtraFlags>
struct FunctionData<Ret (Args...) const, ExtraFlags>
    : FunctionData<Ret (Args...), ExtraFlags | QtMocConstants::MethodIsConst>
{
    using FunctionData<Ret (Args...), ExtraFlags | QtMocConstants::MethodIsConst>::FunctionData;
};

template <typename F> struct MethodData : FunctionData<F, QtMocConstants::MethodMethod>
{
    using FunctionData<F, QtMocConstants::MethodMethod>::FunctionData;
};

template <typename F> struct SignalData : FunctionData<F, QtMocConstants::MethodSignal>
{
    using FunctionData<F, QtMocConstants::MethodSignal>::FunctionData;
};

template <typename F> struct SlotData : FunctionData<F, QtMocConstants::MethodSlot>
{
    using FunctionData<F, QtMocConstants::MethodSlot>::FunctionData;
};

template <typename F> struct ConstructorData : FunctionData<F, QtMocConstants::MethodConstructor>
{
    using Base = FunctionData<F, QtMocConstants::MethodConstructor>;

    // the name for a constructor is always the class name (string index zero)
    // and it has no return type
    constexpr ConstructorData(uint tagIndex, uint flags, typename Base::ParametersArray params = {})
        : Base(0, tagIndex, flags, QMetaType::UnknownType, params)
    {}
};

template <typename F> struct RevisionedMethodData :
        FunctionData<F, QtMocConstants::MethodRevisioned | QtMocConstants::MethodMethod>
{
    using FunctionData<F, QtMocConstants::MethodRevisioned | QtMocConstants::MethodMethod>::FunctionData;
};

template <typename F> struct RevisionedSignalData :
        FunctionData<F, QtMocConstants::MethodRevisioned | QtMocConstants::MethodSignal>
{
    using FunctionData<F, QtMocConstants::MethodRevisioned | QtMocConstants::MethodSignal>::FunctionData;
};

template <typename F> struct RevisionedSlotData :
        FunctionData<F, QtMocConstants::MethodRevisioned | QtMocConstants::MethodSlot>
{
    using FunctionData<F, QtMocConstants::MethodRevisioned | QtMocConstants::MethodSlot>::FunctionData;
};

template <typename F> struct RevisionedConstructorData :
        FunctionData<F, QtMocConstants::MethodRevisioned | QtMocConstants::MethodConstructor>
{
    using Base = FunctionData<F, QtMocConstants::MethodRevisioned | QtMocConstants::MethodConstructor>;

    // the name for a constructor is always the class name (string index zero)
    // and it has no return type
    constexpr RevisionedConstructorData(uint tagIndex, uint flags, uint revision,
                                        typename Base::ParametersArray params = {})
        : Base(0, tagIndex, flags, revision, QMetaType::UnknownType, params)
    {}
};

template <typename ObjectType, typename Unique, typename Strings,
          typename Methods, typename Properties, typename Enums,
          typename Constructors = UintData<>, typename ClassInfo = detail::UintDataBlock<0, 0>>
constexpr auto metaObjectData(uint flags, const Strings &strings,
                              const Methods &methods, const Properties &properties,
                              const Enums &enums, const Constructors &constructors = {},
                              const ClassInfo &classInfo = {})
{
    constexpr uint MetaTypeCount = Properties::metaTypeCount()
            + Enums::metaTypeCount()
            + 1     // the gadget's or void
            + Methods::metaTypeCount()
            + Constructors::metaTypeCount();

    constexpr uint HeaderSize = 14;
    constexpr uint TotalSize = HeaderSize
            + Properties::dataSize()
            + Enums::dataSize()
            + Methods::dataSize()
            + Constructors::dataSize()
            + ClassInfo::headerSize() // + ClassInfo::payloadSize()
            + 1;    // empty EOD

    MetaObjectContents<TotalSize, 2 * Strings::StringCount, Strings::StringSize,
            MetaTypeCount> result = {};
    strings.writeTo(result.staticData.stringdata, result.staticData.strings);

    uint dataoffset = HeaderSize;
    uint metatypeoffset = 0;
    uint *data = result.staticData.data;

    data[0] = QtMocConstants::OutputRevision;
    data[1] = 0;     // class name index (it's always 0)

    data[2] = ClassInfo::headerSize() / 2;
    data[3] = ClassInfo::headerSize() ? dataoffset : 0;
    q20::copy_n(classInfo.header, classInfo.headerSize(), data + dataoffset);
    dataoffset += ClassInfo::headerSize();

    data[6] = properties.count();
    data[7] = properties.count() ? dataoffset : 0;
    properties.template copyTo<Unique>(result, dataoffset, metatypeoffset);
    dataoffset += properties.dataSize();

    data[8] = enums.count();
    data[9] = enums.count() ? dataoffset : 0;
    enums.template copyTo<Unique>(result, dataoffset, metatypeoffset);
    dataoffset += enums.dataSize();

    // the meta type referring to the object itself
    result.relocatingData.metaTypes[metatypeoffset++] = QMetaType::fromType<ObjectType>().iface();

    data[4] = methods.count();
    data[5] = methods.count() ? dataoffset : 0;
    methods.template copyTo<Unique>(result, dataoffset, metatypeoffset);
    dataoffset += methods.dataSize();

    data[10] = constructors.count();
    data[11] = constructors.count() ? dataoffset : 0;
    constructors.template copyTo<Unique>(result, dataoffset, metatypeoffset);
    dataoffset += constructors.dataSize();

    data[12] = flags;

    // count the number of signals
    if constexpr (Methods::count()) {
        constexpr uint MethodHeaderSize = Methods::headerSize() / Methods::count();
        const uint *ptr = &data[data[5]];
        const uint *end = &data[data[5] + MethodHeaderSize * Methods::count()];
        for ( ; ptr < end; ptr += MethodHeaderSize) {
            if ((ptr[4] & QtMocConstants::MethodSignal) == 0)
                break;
            ++data[13];
        }
    }

    return result;
}

template <typename T> inline std::enable_if_t<std::is_enum_v<T>> assignFlags(void *v, T t) noexcept
{
    *static_cast<T *>(v) = t;
}

template <typename T> inline std::enable_if_t<QtPrivate::IsQFlags<T>::value> assignFlags(void *v, T t) noexcept
{
    *static_cast<T *>(v) = t;
}

#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
template <typename T>
Q_DECL_DEPRECATED_X("Returning int/uint from a Q_PROPERTY that is a Q_FLAG is deprecated; "
                    "please update to return the actual property's type")
inline void assignFlagsFromInteger(QFlags<T> &f, int i) noexcept
{
     f = QFlag(i);
}

template <typename T, typename I>
inline std::enable_if_t<QtPrivate::IsQFlags<T>::value && sizeof(T) == sizeof(int) && std::is_integral_v<I>>
assignFlags(void *v, I i) noexcept
{
    assignFlagsFromInteger(*static_cast<T *>(v), i);
}
#endif  // Qt 7

} // namespace QtMocHelpers
QT_END_NAMESPACE

QT_USE_NAMESPACE

#endif // QTMOCHELPERS_H
