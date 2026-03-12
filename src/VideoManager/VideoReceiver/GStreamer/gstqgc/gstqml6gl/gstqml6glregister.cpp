#include "gstqml6glregister.h"
#include "qt6/qt6glitem.h"
#include <QtQml/qqml.h>

void gstQml6GLRegisterQmlTypes()
{
    qmlRegisterType<Qt6GLVideoItem>(
        "org.freedesktop.gstreamer.Qt6GLVideoItem", 1, 0, "GstGLQt6VideoItem");
}
