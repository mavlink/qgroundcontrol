#pragma once
#include <QObject>
#include <QSettings>
#include <QtQml/qqmlregistration.h>   // <-- required for QML_* macros

class MainWindowPrefs : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(MainWindowPrefs) // Name visible in QML
    QML_SINGLETON                      // Make it a QML singleton

    Q_PROPERTY(bool startFullScreen READ startFullScreen WRITE setStartFullScreen NOTIFY startFullScreenChanged)
    Q_PROPERTY(int  normalX READ normalX WRITE setNormalX NOTIFY normalXChanged)
    Q_PROPERTY(int  normalY READ normalY WRITE setNormalY NOTIFY normalYChanged)
    Q_PROPERTY(int  normalW READ normalW WRITE setNormalW NOTIFY normalWChanged)
    Q_PROPERTY(int  normalH READ normalH WRITE setNormalH NOTIFY normalHChanged)

public:
    explicit MainWindowPrefs(QObject* parent = nullptr);

    bool startFullScreen() const { return _startFullScreen; }
    int  normalX() const { return _normalX; }
    int  normalY() const { return _normalY; }
    int  normalW() const { return _normalW; }
    int  normalH() const { return _normalH; }

public slots:
    void setStartFullScreen(bool v);
    void setNormalX(int v);
    void setNormalY(int v);
    void setNormalW(int v);
    void setNormalH(int v);

signals:
    void startFullScreenChanged();
    void normalXChanged();
    void normalYChanged();
    void normalWChanged();
    void normalHChanged();

private:
    void _load();
    void _saveOne(const char* key, const QVariant& v) const;

    bool _startFullScreen = false;
    int  _normalX = 0;
    int  _normalY = 0;
    int  _normalW = 1280;
    int  _normalH = 800;
};
