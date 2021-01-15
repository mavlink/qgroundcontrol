/*!
 *   @file
 *   @brief Camera Option Classes
 *   @author Gus Grubba <gus@auterion.com>
 *   @author Hugo Trippaers <htrippaers@schubergphilis.com>
 *
 */

#include "QGCCameraOption.h"

//-----------------------------------------------------------------------------
QGCCameraOptionExclusion::QGCCameraOptionExclusion(QObject* parent, QString param_, QString value_, QStringList exclusions_)
    : QObject(parent)
    , param(param_)
    , value(value_)
    , exclusions(exclusions_)
{
}

//-----------------------------------------------------------------------------
QGCCameraOptionRange::QGCCameraOptionRange(QObject* parent, QString param_, QString value_, QString targetParam_, QString condition_, QStringList optNames_, QStringList optValues_)
    : QObject(parent)
    , param(param_)
    , value(value_)
    , targetParam(targetParam_)
    , condition(condition_)
    , optNames(optNames_)
    , optValues(optValues_)
{
}
