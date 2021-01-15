/*!
 *   @file
 *   @brief Camera Option Classes
 *   @author Gus Grubba <gus@auterion.com>
 *   @author Hugo Trippaers <htrippaers@schubergphilis.com>
 *
 */

#ifndef QGCCAMERAOPTION_H
#define QGCCAMERAOPTION_H

#include "QGCApplication.h"

//-----------------------------------------------------------------------------
/// Camera option exclusions
class QGCCameraOptionExclusion : public QObject
{
public:
    QGCCameraOptionExclusion(QObject* parent, QString param_, QString value_, QStringList exclusions_);
    QString param;
    QString value;
    QStringList exclusions;
};

//-----------------------------------------------------------------------------
/// Camera option ranges
class QGCCameraOptionRange : public QObject
{
public:
    QGCCameraOptionRange(QObject* parent, QString param_, QString value_, QString targetParam_, QString condition_, QStringList optNames_, QStringList optValues_);
    QString param;
    QString value;
    QString targetParam;
    QString condition;
    QStringList  optNames;
    QStringList  optValues;
    QVariantList optVariants;
};



#endif // QGCCAMERAOPTION_H
