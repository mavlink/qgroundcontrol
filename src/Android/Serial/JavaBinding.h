#pragma once

#include <type_traits>

#include <QtCore/QJniEnvironment>
#include <QtCore/QtGlobal>
#include <QtCore/QtJniTypes>

// The Java methods and fields that C++ binds through JNI, each declared by its C++ types, from which its JNI
// signature is derived. AndroidSerialJni.cc declares the ones of the USB serial ports and looks them all up.

namespace AndroidSerial {

template <typename Id>
struct JavaBinding;

/// The Java methods or fields that C++ binds, in the order in which they are declared.
template <typename Id>
struct JavaBindingList
{
    JavaBinding<Id>* first = nullptr;
    JavaBinding<Id>* last = nullptr;
};

/// A method or field that JNI finds in a Java class by name and signature, with the ID it is found under. Each
/// one adds itself to the list of its kind, over which all of them are looked up, so that none is listed twice.
template <typename Id>
struct JavaBinding
{
    JavaBinding(const char* javaName, const char* jniSignature, JavaBindingList<Id>& list)
        : name(javaName)
        , signature(jniSignature)
    {
        (list.last ? list.last->next : list.first) = this;
        list.last = this;
    }

    Q_DISABLE_COPY_MOVE(JavaBinding)

    const char* const name;
    const char* const signature;
    // Null until the IDs are cached, and where Java has no such method or field.
    Id id = nullptr;
    JavaBinding* next = nullptr;
};

// One list of each kind for the whole program, whichever files declare the bindings.
inline JavaBindingList<jmethodID> s_javaMethods;
inline JavaBindingList<jfieldID> s_javaFields;

/// A static method of QGCUsbSerialManager.java, declared by the JNI types of what it returns and of what it
/// takes. JNI takes the arguments of a call as C varargs, so arguments that disagree with the signature the
/// method was found under would compile, and corrupt the call. Here the signature is derived from the declared
/// types, and a call compiles only with arguments of exactly those types.
template <typename Ret, typename... Args>
struct JavaMethod : JavaBinding<jmethodID>
{
    static constexpr auto kSignature = QtJniTypes::methodSignature<Ret, Args...>();
    /// What JNI returns from a call: an array of a Java class is a jobjectArray to it.
    using Returned = std::conditional_t<std::is_array_v<Ret>, jobjectArray, Ret>;

    explicit JavaMethod(const char* javaName)
        : JavaBinding<jmethodID>(javaName, kSignature.data(), s_javaMethods)
    {}

    /// Calls the method, which the caller has found available. The caller then checks whether Java threw.
    Returned call(JNIEnv* env, jclass javaClass, Args... args) const
    {
        if constexpr (std::is_same_v<Ret, jboolean>) {
            return env->CallStaticBooleanMethod(javaClass, id, args...);
        } else if constexpr (std::is_same_v<Ret, jlong>) {
            return env->CallStaticLongMethod(javaClass, id, args...);
        } else if constexpr (std::is_same_v<Ret, jint>) {
            return env->CallStaticIntMethod(javaClass, id, args...);
        } else {
            return static_cast<Returned>(env->CallStaticObjectMethod(javaClass, id, args...));
        }
    }

    // Arguments that the call above would have to convert select this instead: a conversion could hide a mix-up.
    template <typename... Other>
    Returned call(JNIEnv*, jclass, Other...) const = delete;
};

/// A field of QGCUsbSerialPortInfo.java, declared by its JNI type, from which its signature is derived.
template <typename T>
struct JavaField : JavaBinding<jfieldID>
{
    static constexpr auto kSignature = QtJniTypes::fieldSignature<T>();

    explicit JavaField(const char* javaName)
        : JavaBinding<jfieldID>(javaName, kSignature.data(), s_javaFields)
    {}
};

}  // namespace AndroidSerial
