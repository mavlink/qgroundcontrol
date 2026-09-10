#pragma once

#include <QtCore/QJniObject>
#include <QtCore/QString>

class AndroidMediaStore
{
public:
    AndroidMediaStore() = default;
    ~AndroidMediaStore();
    Q_DISABLE_COPY_MOVE(AndroidMediaStore)

    static bool isSupported();
    static void cleanupOldVideos(qint64 maxBytes);

    bool openVideo(const QString& fileName);
    bool openImage(const QString& fileName);
    int fileDescriptor() const;
    bool finish(bool finalized);
    void discard();

private:
    bool _open(const char* factoryMethod, const QString& fileName);

    QJniObject _entry;
};
