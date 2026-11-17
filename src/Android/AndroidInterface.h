#pragma once

#include <QtCore/QJniEnvironment>
#include <QtCore/QLoggingCategory>
#include <QtCore/QString>

#include <functional>

namespace AndroidInterface {
void setNativeMethods();
bool checkStoragePermissions();
QString getSDCardPath();
void setKeepScreenOn(bool on);

/// What should go on while the app is in the background, where Android freezes an app and cuts its network.
enum class BackgroundWork
{
    VehicleLink = 1 << 0,
    VideoRecording = 1 << 1,
};

/// While any @p work is @p active, a foreground service with its notification keeps the app's process and network
/// alive in the background, and the CPU and Wi-Fi awake with the screen off. Qt still pauses the main thread
/// there: what other threads do goes on, as a video recording does.
void setBackgroundWork(BackgroundWork work, bool active);
void openFileImportDialog(const QString& destPath, std::function<void(const QString&)> callback);

constexpr const char* kJniQGCActivityClassName = "org/mavlink/qgroundcontrol/QGCActivity";

template <typename T>
class JniLocalRef
{
public:
    JniLocalRef(JNIEnv* env, T ref = nullptr) : _env(env), _ref(ref)
    {
    }

    ~JniLocalRef()
    {
        reset();
    }

    JniLocalRef(const JniLocalRef&) = delete;
    JniLocalRef& operator=(const JniLocalRef&) = delete;

    JniLocalRef(JniLocalRef&& other) noexcept : _env(other._env), _ref(other._ref)
    {
        other._ref = nullptr;
    }

    JniLocalRef& operator=(JniLocalRef&& other) noexcept
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

    T get() const
    {
        return _ref;
    }

    operator T() const
    {
        return _ref;
    }

    void reset(T ref = nullptr)
    {
        if (_env && _ref) {
            _env->DeleteLocalRef(_ref);
        }
        _ref = ref;
    }

private:
    JNIEnv* _env = nullptr;
    T _ref = nullptr;
};

template <typename... Args>
inline bool callStaticIntMethod(QJniEnvironment& env, jclass cls, jmethodID method, const char* caller,
                                const QLoggingCategory& logCategory, jint& result, Args... args)
{
    result = env->CallStaticIntMethod(cls, method, args...);
    if (env.checkAndClearExceptions()) {
        qCWarning(logCategory) << "Exception occurred while calling" << caller;
        return false;
    }

    return true;
}

template <typename... Args>
inline bool callStaticBooleanMethod(QJniEnvironment& env, jclass cls, jmethodID method, const char* caller,
                                    const QLoggingCategory& logCategory, jboolean& result, Args... args)
{
    result = env->CallStaticBooleanMethod(cls, method, args...);
    if (env.checkAndClearExceptions()) {
        qCWarning(logCategory) << "Exception occurred while calling" << caller;
        return false;
    }

    return true;
}
}  // namespace AndroidInterface
