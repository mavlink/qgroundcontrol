#pragma once

#include <QtCore/QString>
#include <QtCore/QLoggingCategory>
#include <QtCore/QJniEnvironment>

Q_DECLARE_LOGGING_CATEGORY(AndroidInterfaceLog)

namespace AndroidInterface
{
    void setNativeMethods();
    bool checkStoragePermissions();
    QString getSDCardPath();
    void setKeepScreenOn(bool on);

    constexpr const char *kJniQGCActivityClassName = "org/mavlink/qgroundcontrol/QGCActivity";

    template<typename T>
    class JniLocalRef
    {
    public:
        JniLocalRef(JNIEnv *env, T ref = nullptr)
            : _env(env)
            , _ref(ref)
        {
        }

        ~JniLocalRef()
        {
            reset();
        }

        JniLocalRef(const JniLocalRef &) = delete;
        JniLocalRef &operator=(const JniLocalRef &) = delete;

        JniLocalRef(JniLocalRef &&other) noexcept
            : _env(other._env)
            , _ref(other._ref)
        {
            other._ref = nullptr;
        }

        JniLocalRef &operator=(JniLocalRef &&other) noexcept
        {
            if (this == &other) {
                return *this;
            }

            reset();
            _env = other._env;
            _ref = other._ref;
            other._ref = nullptr;
            return *this;
        }

        T get() const { return _ref; }
        operator T() const { return _ref; }

        void reset(T ref = nullptr)
        {
            if (_env && _ref) {
                _env->DeleteLocalRef(_ref);
            }
            _ref = ref;
        }

    private:
        JNIEnv *_env = nullptr;
        T _ref = nullptr;
    };

    template<typename... Args>
    static bool callStaticIntMethod(QJniEnvironment &env, jclass cls, jmethodID method,
                                    const char *caller, const QLoggingCategory &logCategory,
                                    jint &result, Args... args)
    {
        result = env->CallStaticIntMethod(cls, method, args...);
        if (env.checkAndClearExceptions()) {
            qCWarning(logCategory) << "Exception occurred while calling" << caller;
            return false;
        }

        return true;
    }

    template<typename... Args>
    static bool callStaticBooleanMethod(QJniEnvironment &env, jclass cls, jmethodID method,
                                        const char *caller, const QLoggingCategory &logCategory,
                                        jboolean &result, Args... args)
    {
        result = env->CallStaticBooleanMethod(cls, method, args...);
        if (env.checkAndClearExceptions()) {
            qCWarning(logCategory) << "Exception occurred while calling" << caller;
            return false;
        }

        return true;
    }
};
