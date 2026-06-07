// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
#ifndef QV4STATICVALUE_P_H
#define QV4STATICVALUE_P_H

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

#include <qjsnumbercoercion.h>

#include <QtCore/private/qnumeric_p.h>
#include <private/qtqmlglobal_p.h>

#include <cstring>

#ifdef QT_NO_DEBUG
#define QV4_NEARLY_ALWAYS_INLINE Q_ALWAYS_INLINE
#else
#define QV4_NEARLY_ALWAYS_INLINE inline
#endif

QT_BEGIN_NAMESPACE

namespace QV4 {

// ReturnedValue is used to return values from runtime methods
// the type has to be a primitive type (no struct or union), so that the compiler
// will return it in a register on all platforms.
// It will be returned in rax on x64, [eax,edx] on x86 and [r0,r1] on arm
typedef quint64 ReturnedValue;

namespace Heap {
struct Base;
}

struct StaticValue
{
    using HeapBasePtr = Heap::Base *;

    StaticValue() = default;
    constexpr StaticValue(quint64 val) : _val(val) {}

    StaticValue &operator=(ReturnedValue v)
    {
        _val = v;
        return *this;
    }

    template<typename Value>
    StaticValue &operator=(const Value &);

    template<typename Value>
    const Value &asValue() const;

    template<typename Value>
    Value &asValue();

    /*
        We use 8 bytes for a value. In order to store all possible values we employ a variant of NaN
        boxing. A "special" Double is indicated by a number that has the 11 exponent bits set to 1.
        Those can be NaN, positive or negative infinity. We only store one variant of NaN: The sign
        bit has to be off and the bit after the exponent ("quiet bit") has to be on. However, since
        the exponent bits are enough to identify special doubles, we can use a different bit as
        discriminator to tell us how the rest of the bits (including quiet and sign) are to be
        interpreted. This bit is bit 48. If set, we have an unmanaged value, which includes the
        special doubles and various other values. If unset, we have a managed value, and all of the
        other bits can be used to assemble a pointer.

        On 32bit systems the pointer can just live in the lower 4 bytes. On 64 bit systems the lower
        48 bits can be used for verbatim pointer bits. However, since all our heap objects are
        aligned to 32 bytes, we can use the 5 least significant bits of the pointer to store, e.g.
        pointer tags on android. The same holds for the 3 bits between the double exponent and
        bit 48.

        With that out of the way, we can use the other bits to store different values.

        We xor Doubles with (0x7ff48000 << 32). That has the effect that any double with all the
        exponent bits set to 0 is one of our special doubles. Those special doubles then get the
        other two bits in the mask (Special and Number) set to 1, as they cannot have 1s in those
        positions to begin with.

        We dedicate further bits to integer-convertible and bool-or-int. With those bits we can
        describe all values we need to store.

        Undefined is encoded as a managed pointer with value 0. This is the same as a nullptr.

        Specific bit-sequences:
        0 = always 0
        1 = always 1
        x = stored value
        y = stored value, shifted to different position
        a = xor-ed bits, where at least one bit is set
        b = xor-ed bits

        32109876 54321098 76543210 98765432 10987654 32109876 54321098 76543210 |
        66665555 55555544 44444444 33333333 33222222 22221111 11111100 00000000 | JS Value
        ------------------------------------------------------------------------+--------------
        00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 | Undefined
        y0000000 0000yyy0 xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxyyyyy | Managed (heap pointer)
        00000000 00001101 10000000 00000000 00000000 00000000 00000000 00000000 | NaN
        00000000 00000101 10000000 00000000 00000000 00000000 00000000 00000000 | +Inf
        10000000 00000101 10000000 00000000 00000000 00000000 00000000 00000000 | -Inf
        xaaaaaaa aaaaxbxb bxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx | double
        00000000 00000001 00000000 00000000 00000000 00000000 00000000 00000000 | empty (non-sparse array hole)
        00000000 00000011 00000000 00000000 00000000 00000000 00000000 00000000 | Null
        00000000 00000011 10000000 00000000 00000000 00000000 00000000 0000000x | Bool
        00000000 00000011 11000000 00000000 xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx | Int
        ^             ^^^ ^^
        |             ||| ||
        |             ||| |+-> Number
        |             ||| +--> Int or Bool
        |             ||+----> Unmanaged
        |             |+-----> Integer compatible
        |             +------> Special double
        +--------------------> Double sign, also used for special doubles
    */

    quint64 _val;

    QV4_NEARLY_ALWAYS_INLINE constexpr quint64 &rawValueRef() { return _val; }
    QV4_NEARLY_ALWAYS_INLINE constexpr quint64 rawValue() const { return _val; }
    QV4_NEARLY_ALWAYS_INLINE constexpr void setRawValue(quint64 raw) { _val = raw; }

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    static inline int valueOffset() { return 0; }
    static inline int tagOffset() { return 4; }
#else // !Q_LITTLE_ENDIAN
    static inline int valueOffset() { return 4; }
    static inline int tagOffset() { return 0; }
#endif
    static inline constexpr quint64 tagValue(quint32 tag, quint32 value) { return quint64(tag) << Tag_Shift | value; }
    QV4_NEARLY_ALWAYS_INLINE constexpr void setTagValue(quint32 tag, quint32 value) { _val = quint64(tag) << Tag_Shift | value; }
    QV4_NEARLY_ALWAYS_INLINE constexpr quint32 value() const { return _val & quint64(~quint32(0)); }
    QV4_NEARLY_ALWAYS_INLINE constexpr quint32 tag() const { return _val >> Tag_Shift; }
    QV4_NEARLY_ALWAYS_INLINE constexpr void setTag(quint32 tag) { setTagValue(tag, value()); }

    QV4_NEARLY_ALWAYS_INLINE constexpr int int_32() const
    {
        return int(value());
    }
    QV4_NEARLY_ALWAYS_INLINE constexpr void setInt_32(int i)
    {
        setTagValue(quint32(QuickType::Integer), quint32(i));
    }
    QV4_NEARLY_ALWAYS_INLINE uint uint_32() const { return value(); }

    QV4_NEARLY_ALWAYS_INLINE constexpr void setEmpty()
    {
        setTagValue(quint32(QuickType::Empty), 0);
    }

    enum class TagBit {
        // s: sign bit
        // e: double exponent bit
        // u: upper 3 bits if managed
        // m: bit 48, denotes "unmanaged" if 1
        // p: significant pointer bits (some re-used for non-managed)
        //                  seeeeeeeeeeeuuumpppp
        SpecialNegative = 0b10000000000000000000 << 12,
        SpecialQNaN     = 0b00000000000010000000 << 12,
        Special         = 0b00000000000001000000 << 12,
        IntCompat       = 0b00000000000000100000 << 12,
        Unmanaged       = 0b00000000000000010000 << 12,
        IntOrBool       = 0b00000000000000001000 << 12,
        Number          = 0b00000000000000000100 << 12,
    };

    static inline constexpr quint64 tagBitMask(TagBit bit) { return quint64(bit) << Tag_Shift; }

    enum Type {
        // Managed, Double and undefined are not directly encoded
        Managed_Type   = 0,
        Double_Type    = 1,
        Undefined_Type = 2,

        Empty_Type     = quint32(TagBit::Unmanaged),
        Null_Type      = Empty_Type   | quint32(TagBit::IntCompat),
        Boolean_Type   = Null_Type    | quint32(TagBit::IntOrBool),
        Integer_Type   = Boolean_Type | quint32(TagBit::Number)
    };

    enum {
        Tag_Shift = 32,

        IsIntegerConvertible_Shift = 48,
        IsIntegerConvertible_Value =  3, // Unmanaged | IntCompat after shifting

        IsIntegerOrBool_Shift = 47,
        IsIntegerOrBool_Value =  7, // Unmanaged | IntCompat | IntOrBool after shifting
    };

    static_assert(IsIntegerConvertible_Value ==
            (quint32(TagBit::IntCompat) | quint32(TagBit::Unmanaged))
                >> (IsIntegerConvertible_Shift - Tag_Shift));

    static_assert(IsIntegerOrBool_Value ==
            (quint32(TagBit::IntOrBool) | quint32(TagBit::IntCompat) | quint32(TagBit::Unmanaged))
                >> (IsIntegerOrBool_Shift - Tag_Shift));

    static constexpr quint64 ExponentMask  = 0b0111111111110000ull << 48;

    static constexpr quint64 Top1Mask      = 0b1000000000000000ull << 48;
    static constexpr quint64 Upper3Mask    = 0b0000000000001110ull << 48;
    static constexpr quint64 Lower5Mask    = 0b0000000000011111ull;

    static constexpr quint64 ManagedMask   = ExponentMask | quint64(TagBit::Unmanaged) << Tag_Shift;
    static constexpr quint64 DoubleMask    = ManagedMask  | quint64(TagBit::Special)   << Tag_Shift;
    static constexpr quint64 NumberMask    = ManagedMask  | quint64(TagBit::Number)    << Tag_Shift;
    static constexpr quint64 IntOrBoolMask = ManagedMask  | quint64(TagBit::IntOrBool) << Tag_Shift;
    static constexpr quint64 IntCompatMask = ManagedMask  | quint64(TagBit::IntCompat) << Tag_Shift;

    static constexpr quint64 EncodeMask    = DoubleMask | NumberMask;

    static constexpr quint64 DoubleDiscriminator
            = ((quint64(TagBit::Unmanaged) | quint64(TagBit::Special)) << Tag_Shift);
    static constexpr quint64 NumberDiscriminator
            = ((quint64(TagBit::Unmanaged) | quint64(TagBit::Number)) << Tag_Shift);

    // Things we can immediately determine by just looking at the upper 4 bytes.
    enum class QuickType : quint32 {
        // Managed takes precedence over all others. That is, other bits may be set if it's managed.
        // However, since all others include the Unmanaged bit, we can still check them with simple
        // equality operations.
        Managed   = Managed_Type,

        Empty     = Empty_Type,
        Null      = Null_Type,
        Boolean   = Boolean_Type,
        Integer   = Integer_Type,

        PlusInf   = quint32(TagBit::Number) | quint32(TagBit::Special) | quint32(TagBit::Unmanaged),
        MinusInf  = PlusInf | quint32(TagBit::SpecialNegative),
        NaN       = PlusInf | quint32(TagBit::SpecialQNaN),
        MinusNaN  = NaN | quint32(TagBit::SpecialNegative), // Can happen with UMinus on NaN
        // All other values are doubles
    };

    // Aliases for easier porting. Remove those when possible
    using ValueTypeInternal = QuickType;
    enum {
        QT_Empty              = Empty_Type,
        QT_Null               = Null_Type,
        QT_Bool               = Boolean_Type,
        QT_Int                = Integer_Type,
        QuickType_Shift       = Tag_Shift,
    };

    inline Type type() const
    {
        const quint64 masked = _val & DoubleMask;
        if (masked >= DoubleDiscriminator)
            return Double_Type;

        // Any bit set in the exponent would have been caught above, as well as both bits being set.
        // None of them being set as well as only Special being set means "managed".
        // Only Unmanaged being set means "unmanaged". That's all remaining options.
        if (masked != tagBitMask(TagBit::Unmanaged)) {
            Q_ASSERT((_val & tagBitMask(TagBit::Unmanaged)) == 0);
            return isUndefined() ? Undefined_Type : Managed_Type;
        }

        const Type ret = Type(tag());
        Q_ASSERT(
                ret == Empty_Type ||
                ret == Null_Type ||
                ret == Boolean_Type ||
                ret == Integer_Type);
        return ret;
    }

    inline quint64 quickType() const { return (_val >> QuickType_Shift); }

    // used internally in property
    inline bool isEmpty() const { return tag() == quint32(ValueTypeInternal::Empty); }
    inline bool isNull() const { return tag() == quint32(ValueTypeInternal::Null); }
    inline bool isBoolean() const { return tag() == quint32(ValueTypeInternal::Boolean); }
    inline bool isInteger() const { return tag() == quint32(ValueTypeInternal::Integer); }
    inline bool isNullOrUndefined() const { return isNull() || isUndefined(); }
    inline bool isUndefined() const { return _val == 0; }

    inline bool isDouble() const
    {
        // If any of the flipped exponent bits are 1, it's a regular double, and the masked tag is
        // larger than Unmanaged | Special.
        //
        // If all (flipped) exponent bits are 0:
        // 1. If Unmanaged bit is 0, it's managed
        // 2. If the Unmanaged bit it is 1, and the Special bit is 0, it's not a special double
        // 3. If both are 1, it is a special double and the masked tag equals Unmanaged | Special.

        return (_val & DoubleMask) >= DoubleDiscriminator;
    }

    inline bool isNumber() const
    {
        // If any of the flipped exponent bits are 1, it's a regular double, and the masked tag is
        // larger than Unmanaged | Number.
        //
        // If all (flipped) exponent bits are 0:
        // 1. If Unmanaged bit is 0, it's managed
        // 2. If the Unmanaged bit it is 1, and the Number bit is 0, it's not number
        // 3. If both are 1, it is a number and masked tag equals Unmanaged | Number.

        return (_val & NumberMask) >= NumberDiscriminator;
    }

    inline bool isManagedOrUndefined() const { return (_val & ManagedMask) == 0; }

    // If any other bit is set in addition to the managed mask, it's not undefined.
    inline bool isManaged() const
    {
        return isManagedOrUndefined() && !isUndefined();
    }

    inline bool isIntOrBool() const
    {
        // It's an int or bool if all the exponent bits are 0,
        // and the "int or bool" bit as well as the "umanaged" bit are set,
        return (_val >> IsIntegerOrBool_Shift) == IsIntegerOrBool_Value;
    }

    inline bool integerCompatible() const {
        Q_ASSERT(!isEmpty());
        return (_val >> IsIntegerConvertible_Shift) == IsIntegerConvertible_Value;
    }

    static inline bool integerCompatible(StaticValue a, StaticValue b) {
        return a.integerCompatible() && b.integerCompatible();
    }

    static inline bool bothDouble(StaticValue a, StaticValue b) {
        return a.isDouble() && b.isDouble();
    }

    inline bool isNaN() const
    {
        switch (QuickType(tag())) {
        case QuickType::NaN:
        case QuickType::MinusNaN:
            return true;
        default:
            return false;
        }
    }

    inline bool isPositiveInt() const {
        return isInteger() && int_32() >= 0;
    }

    QV4_NEARLY_ALWAYS_INLINE double doubleValue() const {
        Q_ASSERT(isDouble());
        double d;
        const quint64 unmasked = _val ^ EncodeMask;
        memcpy(&d, &unmasked, 8);
        return d;
    }

    QV4_NEARLY_ALWAYS_INLINE void setDouble(double d) {
        if (qt_is_nan(d)) {
            // We cannot store just any NaN. It has to be a NaN with only the quiet bit
            // set in the upper bits of the mantissa and the sign bit either on or off.
            // qt_qnan() happens to produce such a thing via std::numeric_limits,
            // but this is actually not guaranteed. Therefore, we make our own.
            _val = (quint64(std::signbit(d) ? QuickType::MinusNaN : QuickType::NaN) << Tag_Shift);
            Q_ASSERT(isNaN());
        } else {
            memcpy(&_val, &d, 8);
            _val ^= EncodeMask;
        }

        Q_ASSERT(isDouble());
    }

    inline bool isInt32() {
        if (tag() == quint32(QuickType::Integer))
            return true;
        if (isDouble()) {
            double d = doubleValue();
            if (isInt32(d)) {
                setInt_32(int(d));
                return true;
            }
        }
        return false;
    }

    QV4_NEARLY_ALWAYS_INLINE static bool isInt32(double d) {
        int i = QJSNumberCoercion::toInteger(d);
        return (i == d && !(d == 0 && std::signbit(d)));
    }

    double asDouble() const {
        if (tag() == quint32(QuickType::Integer))
            return int_32();
        return doubleValue();
    }

    bool booleanValue() const {
        return int_32();
    }

    int integerValue() const {
        return int_32();
    }

    inline bool tryIntegerConversion() {
        bool b = integerCompatible();
        if (b)
            setTagValue(quint32(QuickType::Integer), value());
        return b;
    }

    bool toBoolean() const {
        if (integerCompatible())
            return static_cast<bool>(int_32());

        if (isManagedOrUndefined())
            return false;

        // double
        const double d = doubleValue();
        return d && !std::isnan(d);
    }

    inline int toInt32() const
    {
        switch (type()) {
        case Null_Type:
        case Boolean_Type:
        case Integer_Type:
            return int_32();
        case Double_Type:
            return QJSNumberCoercion::toInteger(doubleValue());
        case Empty_Type:
        case Undefined_Type:
        case Managed_Type:
            return 0; // Coercion of NaN to int, results in 0;
        }

        Q_UNREACHABLE_RETURN(0);
    }

    ReturnedValue *data_ptr() { return &_val; }
    constexpr ReturnedValue asReturnedValue() const { return _val; }
    constexpr static StaticValue fromReturnedValue(ReturnedValue val) { return {val}; }

    inline static constexpr StaticValue emptyValue() { return { tagValue(quint32(QuickType::Empty), 0) }; }
    static inline constexpr StaticValue fromBoolean(bool b) { return { tagValue(quint32(QuickType::Boolean), b) }; }
    static inline constexpr StaticValue fromInt32(int i) { return { tagValue(quint32(QuickType::Integer), quint32(i)) }; }
    inline static constexpr StaticValue undefinedValue() { return { 0 }; }
    static inline constexpr StaticValue nullValue() { return { tagValue(quint32(QuickType::Null), 0) }; }

    static inline StaticValue fromDouble(double d)
    {
        StaticValue v;
        v.setDouble(d);
        return v;
    }

    static inline StaticValue fromUInt32(uint i)
    {
        StaticValue v;
        if (i < uint(std::numeric_limits<int>::max())) {
            v.setTagValue(quint32(QuickType::Integer), i);
        } else {
            v.setDouble(i);
        }
        return v;
    }

    static double toInteger(double d)
    {
        return QJSNumberCoercion::roundTowards0(d);
    }

    static int toInt32(double d)
    {
        return QJSNumberCoercion::toInteger(d);
    }

    static unsigned int toUInt32(double d)
    {
        return static_cast<uint>(toInt32(d));
    }

    // While a value containing a Heap::Base* is not actually static, we still implement
    // the setting and retrieving of heap pointers here in order to have the encoding
    // scheme completely in one place.

#if QT_POINTER_SIZE == 8

    // All pointer shifts are from more significant to less significant bits.
    // When encoding, we shift right by that amount. When decoding, we shift left.
    // Negative numbers mean shifting the other direction. 0 means no shifting.
    //
    // The IA64 and Sparc64 cases are mostly there to demonstrate the idea. Sparc64
    // and IA64 are not officially supported, but we can expect more platforms with
    // similar "problems" in the future.
    enum PointerShift {
#if 0 && defined(Q_OS_ANDROID) && defined(Q_PROCESSOR_ARM_64)
        // We used to assume that Android on arm64 uses the top byte to store pointer tags.
        // However, at least currently, the pointer tags are only applied on new/malloc and
        // delete/free, not on mmap() and munmap(). We manage the JS heap directly using
        // mmap, so we don't have to preserve any tags.
        //
        // If this ever changes, here is how to preserve the top byte:
        // Move it to Upper3 and Lower5.
        Top1Shift   = 0,
        Upper3Shift = 12,
        Lower5Shift = 56,
#elif defined(Q_PROCESSOR_IA64)
        // On ia64, bits 63-61 in a 64-bit pointer are used to store the virtual region
        // number. We can move those to Upper3.
        Top1Shift   = 0,
        Upper3Shift = 12,
        Lower5Shift = 0,
#elif defined(Q_PROCESSOR_SPARC_64)
        // Sparc64 wants to use 52 bits for pointers.
        // Upper3 can stay where it is, bit48 moves to the top bit.
        Top1Shift   = -15,
        Upper3Shift = 0,
        Lower5Shift = 0,
#elif 0 // TODO: Once we need 5-level page tables, add the appropriate check here.
        // With 5-level page tables (as possible on linux) we need 57 address bits.
        // Upper3 can stay where it is, bit48 moves to the top bit, the rest moves to Lower5.
        Top1Shift   = -15,
        Upper3Shift = 0,
        Lower5Shift = 52,
#else
        Top1Shift   = 0,
        Upper3Shift = 0,
        Lower5Shift = 0
#endif
    };

    template<int Offset, quint64 Mask>
    static constexpr quint64 movePointerBits(quint64 val)
    {
        if constexpr (Offset > 0)
            return (val & ~Mask) | ((val & Mask) >> Offset);
        if constexpr (Offset < 0)
            return (val & ~Mask) | ((val & Mask) << -Offset);
        return val;
    }

    template<int Offset, quint64 Mask>
    static constexpr quint64 storePointerBits(quint64 val)
    {
        constexpr quint64 OriginMask = movePointerBits<-Offset, Mask>(Mask);
        return movePointerBits<Offset, OriginMask>(val);
    }

    template<int Offset, quint64 Mask>
    static constexpr quint64 retrievePointerBits(quint64 val)
    {
        return movePointerBits<-Offset, Mask>(val);
    }

    QML_NEARLY_ALWAYS_INLINE HeapBasePtr m() const
    {
        Q_ASSERT(!(_val & ManagedMask));

        // Re-assemble the pointer from its fragments.
        const quint64 tmp = retrievePointerBits<Top1Shift, Top1Mask>(
                            retrievePointerBits<Upper3Shift, Upper3Mask>(
                            retrievePointerBits<Lower5Shift, Lower5Mask>(_val)));

        HeapBasePtr b;
        memcpy(&b, &tmp, 8);
        return b;
    }
    QML_NEARLY_ALWAYS_INLINE void setM(HeapBasePtr b)
    {
        quint64 tmp;
        memcpy(&tmp, &b, 8);

        // Has to be aligned to 32 bytes
        Q_ASSERT(!(tmp & Lower5Mask));

        // MinGW produces a bogus warning about array bounds.
        // There is no array access here.
        QT_WARNING_PUSH
        QT_WARNING_DISABLE_GCC("-Warray-bounds")

        // Encode the pointer.
        _val = storePointerBits<Top1Shift, Top1Mask>(
               storePointerBits<Upper3Shift, Upper3Mask>(
               storePointerBits<Lower5Shift, Lower5Mask>(tmp)));

        QT_WARNING_POP
    }
#elif QT_POINTER_SIZE == 4
    QML_NEARLY_ALWAYS_INLINE HeapBasePtr m() const
    {
        Q_STATIC_ASSERT(sizeof(HeapBasePtr) == sizeof(quint32));
        HeapBasePtr b;
        quint32 v = value();
        memcpy(&b, &v, 4);
        return b;
    }
    QML_NEARLY_ALWAYS_INLINE void setM(HeapBasePtr b)
    {
        quint32 v;
        memcpy(&v, &b, 4);
        setTagValue(quint32(QuickType::Managed), v);
    }
#else
#  error "unsupported pointer size"
#endif
};
static_assert(std::is_trivially_copyable_v<StaticValue>);
static_assert(std::is_trivially_default_constructible_v<StaticValue>);

struct Encode {
    static constexpr ReturnedValue undefined() {
        return StaticValue::undefinedValue().asReturnedValue();
    }
    static constexpr ReturnedValue null() {
        return StaticValue::nullValue().asReturnedValue();
    }

    explicit constexpr Encode(bool b)
        : val(StaticValue::fromBoolean(b).asReturnedValue())
    {
    }
    explicit Encode(double d) {
        val = StaticValue::fromDouble(d).asReturnedValue();
    }
    explicit constexpr Encode(int i)
        : val(StaticValue::fromInt32(i).asReturnedValue())
    {
    }
    explicit Encode(uint i) {
        val = StaticValue::fromUInt32(i).asReturnedValue();
    }
    explicit constexpr Encode(ReturnedValue v)
        : val(v)
    {
    }
    constexpr Encode(StaticValue v)
        : val(v.asReturnedValue())
    {
    }

    template<typename HeapBase>
    explicit Encode(HeapBase *o);

    explicit Encode(StaticValue *o) {
        Q_ASSERT(o);
        val = o->asReturnedValue();
    }

    static ReturnedValue smallestNumber(double d) {
        if (StaticValue::isInt32(d))
            return Encode(static_cast<int>(d));
        else
            return Encode(d);
    }

    constexpr operator ReturnedValue() const {
        return val;
    }
    quint64 val;
private:
    explicit Encode(void *);
};

}

QT_END_NAMESPACE

#endif // QV4STATICVALUE_P_H
