#include "gstqml6d3d11register.h"
#include "gstqt6d3d11videoitem.h"
#include <QtQml/qqml.h>

void gstQml6D3D11RegisterQmlTypes()
{
    qmlRegisterType<GstQt6D3D11VideoItem>(
        "org.freedesktop.gstreamer.Qt6D3D11VideoItem", 1, 0, "GstD3D11Qt6VideoItem");
}
