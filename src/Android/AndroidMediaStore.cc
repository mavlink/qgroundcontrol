#include "AndroidMediaStore.h"

#include <QtCore/QCoreApplication>

namespace {
constexpr const char* kMediaStoreClass = "org/mavlink/qgroundcontrol/QGCMediaStore";
}

AndroidMediaStore::~AndroidMediaStore()
{
    if (_entry.isValid()) {
        (void) finish(false);
    }
}

bool AndroidMediaStore::isSupported()
{
    return QNativeInterface::QAndroidApplication::sdkVersion() >= 29;
}

bool AndroidMediaStore::openVideo(const QString& fileName)
{
    return _open("createVideo", fileName);
}

bool AndroidMediaStore::openImage(const QString& fileName)
{
    return _open("createImage", fileName);
}

bool AndroidMediaStore::_open(const char* factoryMethod, const QString& fileName)
{
    if (_entry.isValid() || !isSupported()) {
        return false;
    }
    const QJniObject context = QNativeInterface::QAndroidApplication::context();
    const QJniObject name = QJniObject::fromString(fileName);
    _entry = QJniObject::callStaticObjectMethod(
        kMediaStoreClass, factoryMethod,
        "(Landroid/content/Context;Ljava/lang/String;)Lorg/mavlink/qgroundcontrol/QGCMediaStore;", context.object(),
        name.object<jstring>());
    return _entry.isValid() && fileDescriptor() >= 0;
}

int AndroidMediaStore::fileDescriptor() const
{
    return _entry.isValid() ? _entry.callMethod<jint>("fileDescriptor", "()I") : -1;
}

bool AndroidMediaStore::finish(bool finalized)
{
    if (!_entry.isValid()) {
        return false;
    }
    const bool published = _entry.callMethod<jboolean>("finish", "(Z)Z", static_cast<jboolean>(finalized));
    _entry = QJniObject();
    return published;
}

void AndroidMediaStore::discard()
{
    if (_entry.isValid()) {
        _entry.callMethod<void>("discard", "()V");
        _entry = QJniObject();
    }
}

void AndroidMediaStore::cleanupOldVideos(qint64 maxBytes)
{
    if (!isSupported()) {
        return;
    }
    const QJniObject context = QNativeInterface::QAndroidApplication::context();
    QJniObject::callStaticMethod<void>(kMediaStoreClass, "cleanupOldVideos", "(Landroid/content/Context;J)V",
                                       context.object(), static_cast<jlong>(maxBytes));
}
