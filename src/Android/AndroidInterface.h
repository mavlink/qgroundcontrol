#pragma once

#include <QtCore/QJniEnvironment>
#include <QtCore/QString>

#include <functional>

namespace AndroidInterface {
void setNativeMethods();
bool checkStoragePermissions();
QString getSDCardPath();
void setKeepScreenOn(bool on);
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
}  // namespace AndroidInterface
