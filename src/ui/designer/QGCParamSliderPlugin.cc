#include "QGCParamSliderPlugin.h"
#include "QGCParamSlider.h"

#include <QtPlugin>

QGCParamSliderPlugin::QGCParamSliderPlugin(QObject *parent) :
    QObject(parent)
{
    initialized = false;
}

void QGCParamSliderPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if (initialized)
        return;

    initialized = true;
}

bool QGCParamSliderPlugin::isInitialized() const
{
    return initialized;
}

QWidget *QGCParamSliderPlugin::createWidget(QWidget *parent)
{
    return new QGCParamSlider(parent);
}

QString QGCParamSliderPlugin::name() const
{
    return "QGCParamSlider";
}

QString QGCParamSliderPlugin::group() const
{
    return "QGroundControl";
}

QIcon QGCParamSliderPlugin::icon() const
{
    return QIcon();
}

QString QGCParamSliderPlugin::toolTip() const
{
    return "";
}

QString QGCParamSliderPlugin::whatsThis() const
{
    return "";
}

bool QGCParamSliderPlugin::isContainer() const
{
    return false;
}

QString QGCParamSliderPlugin::domXml() const
{
    return "<ui language=\"c++\">\n"
           " <widget class=\"QGCParamSlider\" name=\"paramSlider\">\n"
           "  <property name=\"geometry\">\n"
           "   <rect>\n"
           "    <x>0</x>\n"
           "    <y>0</y>\n"
           "    <width>150</width>\n"
           "    <height>16</height>\n"
           "   </rect>\n"
           "  </property>\n"
           " </widget>\n"
           "</ui>";
}

QString QGCParamSliderPlugin::includeFile() const
{
    return "QGCParamSlider.h";
}

Q_EXPORT_PLUGIN2(qgcparamsliderplugin, QGCParamSliderPlugin)
