#include "Q3DWidgetFactory.h"

#include "Pixhawk3DWidget.h"

QPointer<Q3DWidget>
Q3DWidgetFactory::get(const std::string& type)
{
    if (type == "PIXHAWK")
    {
        return QPointer<Q3DWidget>(new Pixhawk3DWidget);
    }
    else
    {
        return QPointer<Q3DWidget>(new Q3DWidget);
    }
}
