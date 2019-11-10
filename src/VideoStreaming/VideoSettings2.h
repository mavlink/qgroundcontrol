#ifndef VIDEOSETTINGS_2_H
#define VIDEOSETTINGS_2_H

#include <QObject>
#include <QUrl>

/* Original video settings only worked for a single camera and I
 * have no idea how to use the settings fact mechanism
 *
 * This one ignores the settings fact, and is a more direct approach
 * I believe that this will not be accepted as is but it's a good
 * starting point.
 *
 * ideally I would like to remove the Fact mechanism and use
 * the KConfigXT classes, as they are much leaner and easier to handle.
 *
 * And initially this is also a cut down version of the Video Settings
 * just with UDP, source, and a few options.
 */

class VideoSettings2 : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString uuid READ uuid WRITE setUuid NOTIFY uuidChanged)
    Q_PROPERTY(QString streamType READ streamType WRITE setStreamType NOTIFY streamTypeChanged)
    Q_PROPERTY(QUrl host READ host WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(QString caps READ caps WRITE setCaps NOTIFY capsChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)

public:
    VideoSettings2();

    QString uuid() const;
    Q_SLOT void setUuid(const QString& uuid);
    Q_SIGNAL void uuidChanged();

    Q_INVOKABLE QStringList streamTypes() const;

    QUrl host() const;
    Q_SLOT void setHost(const QUrl& value);
    Q_SIGNAL void hostChanged();

    int port() const;
    Q_SLOT void setPort(int value);
    Q_SIGNAL void portChanged();

    QString streamType() const;
    Q_SLOT void setStreamType(const QString& value);
    Q_SIGNAL void streamTypeChanged();

    QString caps() const;
    Q_SLOT void setCaps(const QString& type);
    Q_SIGNAL void capsChanged();

    QString name() const;
    Q_SLOT void setName(const QString& name);
    Q_SIGNAL void nameChanged();

    Q_INVOKABLE void save();
    Q_INVOKABLE void cancel();

    /* Creates a new video entry in the settings, returns the uuid of the settings in disk. */
    QString createNewEntry();
    void removeEntry();

Q_SIGNALS:
    void settingsChanged();
private:
    QUrl _host;
    int _port;
    QString _streamType;
    QString _caps;
    QString _name;
    QString _uuid;
};

Q_DECLARE_METATYPE(VideoSettings2*);

#endif
