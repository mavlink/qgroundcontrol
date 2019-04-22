#ifndef INPUTEVENTREADER_H
#define INPUTEVENTREADER_H

#include <QObject>
#include <QThread>
#include <QMap>
#include <QTimer>
#include <sys/poll.h>

enum {
    // Raw keycodes we want to listen
    // If you add a new keycode, please refer to frameworks/base/data/keyboards/Generic.kl for the full list.
    BREAK = 119,
    BACK  = 158,
    VOLUME_UP = 115,
    VOLUME_DOWN = 114,
    CAMERA = 212
};

enum {
    // Android keycodes we want to listen
    // If you add a new keycode, please refer to frameworks/base/core/java/android/view/KeyEvent.java for the full list.
    AKEYCODE_BREAK           = 121,
    AKEYCODE_BACK            = 4,
    AKEYCODE_VOLUME_UP       = 24,
    AKEYCODE_VOLUME_DOWN     = 25,
    AKEYCODE_CAMERA          = 27
};

enum {
    // Raw axis codes we want to listen
    // If you add a new axis code, please refer to frameworks/base/data/keyboards/Generic.kl for the full list.
    X = 0,
    Y = 1,
    Z = 2,
    RZ = 5,
    WHEEL = 8
};

enum {
    // Android axis codes we want to listen
    // If you add a new axis code, please refer to frameworks/base/core/java/android/view/MotionEvent.java for the full list.
    AMOTION_EVENT_AXIS_X = 0,
    AMOTION_EVENT_AXIS_Y = 1,
    AMOTION_EVENT_AXIS_Z = 11,
    AMOTION_EVENT_AXIS_RZ = 14,
    AMOTION_EVENT_AXIS_WHEEL = 21
};

struct EventMap {
    int srcCode;
    int dstCode;
};
static const EventMap KEYCODES[] = {
    {BREAK, AKEYCODE_BREAK},
    {BACK, AKEYCODE_BACK},
    {VOLUME_UP, AKEYCODE_VOLUME_UP},
    {VOLUME_DOWN, AKEYCODE_VOLUME_DOWN},
    {CAMERA, AKEYCODE_CAMERA},
    {-1, -1}
};

static const EventMap AXES[] = {
    {X, AMOTION_EVENT_AXIS_X},
    {Y, AMOTION_EVENT_AXIS_Y},
    {Z, AMOTION_EVENT_AXIS_Z},
    {RZ, AMOTION_EVENT_AXIS_RZ},
    {WHEEL, AMOTION_EVENT_AXIS_WHEEL},
    {-1, -1}
};

class InputEventReader : public QThread
{
    Q_OBJECT
public:
    InputEventReader();
    ~InputEventReader();

    enum KeyAction {
        ACTION_DOWN = 0,
        ACTION_UP = 1
    };

    struct KeyEvent {
        int keycode;
        int action;
    };

    struct AxisInfo {
        float scale;
        float offset;
        float min;
        float max;
    };

    struct AxisEvent {
        int axisCode;
        float value;
    };

signals:
    void keyEventRecieved(int keycode, int action);
    void axisEventRecieved(int axiscode, float value);

private:
    void initialize();
    int scanDir(const char *dirname);
    int findDevice(const char *devicePath);
    int getAxisInfo(int fd);
    int lookupEventCode(int rawCode, const EventMap *list);
    KeyEvent convertToKeyEvent(int rawCode, int value);
    AxisEvent convertToAxisEvent(int rawCode, int value);
    // Override from QThread
    virtual void run(void);

    char **devicePaths;
    int nfds;
    struct pollfd *ufds;

    QMap<int, AxisInfo> mAxes;
};
#endif//INPUTEVENTREADER_H
