#include "InputEventReader.h"
#include "QGC.h"
#include "QGCApplication.h"

#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>
#include <sys/poll.h>
#include<sys/types.h>
#include<dirent.h>

static const char *DEVICE_PATH = "/dev/input";
//if you want to add a new device to listen, please add its name here
static const char *INPUT_DEVICES_NAME[] = { "gpio-keys", "mlx_joystick" };

inline static float avg(float x, float y) {
    return (x + y) / 2;
}

InputEventReader::InputEventReader()
    :nfds(0)
    ,ufds(NULL)
{
    devicePaths = new char*[sizeof(INPUT_DEVICES_NAME)/sizeof(char *)];
    initialize();
}

InputEventReader::~InputEventReader()
{
    if(ufds) {
        for(int i = 0; i < nfds; i++) {
            close(ufds[i].fd);
            delete devicePaths[i];
        }
        delete ufds;
    }
    delete devicePaths;
}

void InputEventReader::initialize()
{
    scanDir(DEVICE_PATH);
    if(nfds == 0) {
        qWarning() << "No input devices found";
        return;
    }
    ufds = new struct pollfd[nfds];

    //open devices here
    for(int i = 0; i < nfds; i++) {
        int fd = open(devicePaths[i], O_RDWR | O_CLOEXEC);
        ufds[i].fd = fd;
        ufds[i].events = POLLIN;

        getAxisInfo(fd);
    }
}

int InputEventReader::scanDir(const char *dirname)
{
    char devname[20];
    char *filename;
    DIR *dir;
    struct dirent *de;
    dir = opendir(dirname);
    if(dir == NULL)
        return -1;
    strcpy(devname, dirname);
    filename = devname + strlen(devname);
    *filename++ = '/';
    while((de = readdir(dir))) {
        if(de->d_name[0] == '.' &&
           (de->d_name[1] == '\0' ||
            (de->d_name[1] == '.' && de->d_name[2] == '\0')))
            continue;
        strcpy(filename, de->d_name);
        findDevice(devname);
    }
    closedir(dir);
    return 0;
}

int InputEventReader::findDevice(const char *devicePath)
{
    char name[80];
    int fd = open(devicePath, O_RDWR | O_CLOEXEC);

    if(fd < 0) {
        qWarning() << "Could not open " << devicePath << ":" << strerror(errno);
        return -1;
    }

    name[sizeof(name) - 1] = '\0';
    if(ioctl(fd, EVIOCGNAME(sizeof(name) - 1), &name) < 1) {
        name[0] = '\0';
    }

    for(size_t i = 0; i < sizeof(INPUT_DEVICES_NAME)/sizeof(char *); i++) {
        if(!strcmp(name, INPUT_DEVICES_NAME[i])) {
            devicePaths[nfds] = new char[20];
            strcpy(devicePaths[nfds], devicePath);
            nfds++;
            break;
        }
    }
    close(fd);

    return 0;
}

int InputEventReader::getAxisInfo(int fd)
{
    struct input_absinfo info;
    const EventMap *axisMap = AXES;

    while(axisMap->srcCode >= 0) {
        if(mAxes.contains(axisMap->dstCode)) {
            axisMap++;
            continue;
        }
        AxisInfo axis;
        if(ioctl(fd, EVIOCGABS(axisMap->srcCode), &info)) {
            axisMap++;
            continue;
        }
        if (info.minimum != info.maximum) {
            axis.scale = 2.0f / (info.maximum - info.minimum);
            axis.offset = avg(info.minimum, info.maximum) * -axis.scale;
            axis.min = -1.0f;
            axis.max = 1.0f;
            mAxes[axisMap->dstCode] = axis;
            axisMap++;
        }
    }
    return 0;
}

int InputEventReader::lookupEventCode(int rawCode, const EventMap *list)
{
    while(list->srcCode >= 0) {
        if(list->srcCode == rawCode) {
            return list->dstCode;
        }
        list++;
    }
    return list->dstCode;
}

InputEventReader::KeyEvent InputEventReader::convertToKeyEvent(int rawCode, int value)
{
    KeyEvent event;

    event.keycode = lookupEventCode(rawCode, KEYCODES);
    if(value == 1) {
        event.action = ACTION_DOWN;
    } else if(value == 0) {
        event.action = ACTION_UP;
    }

    return event;
}

InputEventReader::AxisEvent InputEventReader::convertToAxisEvent(int rawCode, int value)
{
    AxisEvent event;

    event.axisCode = lookupEventCode(rawCode, AXES);
    if(mAxes.contains(event.axisCode)) {
        event.value = value * mAxes[event.axisCode].scale + mAxes[event.axisCode].offset;
    } else {
        event.value = 0;
    }

    return event;
}

void InputEventReader::run(void)
{
    int i;
    struct input_event event;
    if(nfds == 0) {
        qWarning() << "No fds to poll, end poll thread";
        return;
    }

    while(1) {
        poll(ufds, nfds, -1);
        for(i = 0; i < nfds; i++) {
            if(ufds[i].revents) {
                if(ufds[i].revents & POLLIN) {
                    int res = read(ufds[i].fd, &event, sizeof(event));
                    if(res < (int)sizeof(event)) {
                        qWarning() << "Could not get event, end poll thread";
                        return;
                    }
                    if(event.type == EV_KEY) {
                        KeyEvent keyEvent = convertToKeyEvent(event.code, event.value);
                        if(keyEvent.keycode < 0) {
                            qWarning() << "Unsupported key event, code: " << event.code;
                            continue;
                        }
                        emit keyEventRecieved(keyEvent.keycode, keyEvent.action);
                    } else if(event.type == EV_ABS) {
                        AxisEvent axisEvent = convertToAxisEvent(event.code, event.value);
                        if(axisEvent.axisCode < 0) {
                            qWarning() << "Unsupported motion event, code: " << event.code;
                            continue;
                        }
                        emit axisEventRecieved(axisEvent.axisCode, axisEvent.value);
                    }
                }
            }
        }
    }
}
